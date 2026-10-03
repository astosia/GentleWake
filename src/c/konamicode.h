#pragma once
#include <pebble.h>

typedef void (*CodeSuccessCallBack)();

void show_konamicode(CodeSuccessCallBack callback);
void hide_konamicode(void);