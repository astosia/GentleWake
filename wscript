
# #
# # This file is the default set of rules to compile a Pebble project.
# #
# # Feel free to customize this to your needs.
# #

# import os.path
# try:
#     from sh import CommandNotFound, jshint, cat, ErrorReturnCode_2
#     hint = jshint
# except (ImportError, CommandNotFound):
#     hint = None

# top = '.'
# out = 'build'

# def options(ctx):
#     ctx.load('pebble_sdk')

# def configure(ctx):
#     ctx.load('pebble_sdk')
#     global hint
#     if hint is not None:
#         hint = hint.bake(['--config', 'pebble-jshintrc'])

# def build(ctx):
#     if False and hint is not None:
#         try:
#             hint([node.abspath() for node in ctx.path.ant_glob("src/**/*.js")], _tty_out=False) # no tty because there are none in the cloudpebble sandbox.
#         except ErrorReturnCode_2 as e:
#             ctx.fatal("\nJavaScript linting failed (you can disable this in Project Settings):\n" + e.stdout)

#     # Concatenate all our JS files (but not recursively), and only if any JS exists in the first place.
#     ctx.path.make_node('src/js/').mkdir()
#     js_paths = ctx.path.ant_glob(['src/*.js', 'src/**/*.js'])
#     if js_paths:
#         ctx(rule='cat ${SRC} > ${TGT}', source=js_paths, target='pebble-js-app.js')
#         has_js = True
#     else:
#         has_js = False

#     ctx.load('pebble_sdk')

#     ctx.pbl_program(source=ctx.path.ant_glob('src/**/*.c'),
#                     target='pebble-app.elf')

#     if os.path.exists('worker_src'):
#         ctx.pbl_worker(source=ctx.path.ant_glob('worker_src/**/*.c'),
#                         target='pebble-worker.elf')
#         ctx.pbl_bundle(elf='pebble-app.elf',
#                         worker_elf='pebble-worker.elf',
#                         js='pebble-js-app.js' if has_js else [])
#     else:
#         ctx.pbl_bundle(elf='pebble-app.elf',
#                        js='pebble-js-app.js' if has_js else [])


#
# This file is the default set of rules to compile a Pebble application.
#
# Feel free to customize this to your needs.
#
import os.path

top = '.'
out = 'build'


def options(ctx):
    ctx.load('pebble_sdk')


def configure(ctx):
    ctx.load('pebble_sdk')


def build(ctx):
    ctx.load('pebble_sdk')

    build_worker = os.path.exists('worker_src')
    binaries = []

    cached_env = ctx.env
    for platform in ctx.env.TARGET_PLATFORMS:
        ctx.env = ctx.all_envs[platform]
        ctx.set_group(ctx.env.PLATFORM_NAME)
        app_elf = '{}/pebble-app.elf'.format(ctx.env.BUILD_DIR)
        ctx.pbl_build(source=ctx.path.ant_glob('src/**/*.c'), target=app_elf, bin_type='app')

        if build_worker:
            worker_elf = '{}/pebble-worker.elf'.format(ctx.env.BUILD_DIR)
            binaries.append({'platform': platform, 'app_elf': app_elf, 'worker_elf': worker_elf})
            ctx.pbl_build(source=ctx.path.ant_glob('worker_src/**/*.c'),
                          target=worker_elf,
                          bin_type='worker')
        else:
            binaries.append({'platform': platform, 'app_elf': app_elf})
    ctx.env = cached_env

    ctx.set_group('bundle')
    ctx.pbl_bundle(binaries=binaries, js=[])
