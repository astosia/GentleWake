#include "mainwin.h"
#include "common.h"
#include "commonwin.h"

enum onoff_modes {
  MODE_OFF,
  MODE_ON,
  MODE_ACTIVE
};
  
static char current_time[] = "00:00 AM";
static char s_time_digits[8];
static char s_time_ampm[3];
static bool s_alarms_on;
static char s_info[48];
static char s_onoff_text[40];
static enum onoff_modes s_onoff_mode;
static uint8_t s_autoclose_timeout;
static AppTimer *s_autoclose_timer;
static bool s_konami_on = false;

// Second line of the info box while an alarm is ringing, snoozing or monitoring. With the
// Konami code on, a double click opens the code screen rather than stopping the alarm.
#define STOP_HINT() (s_konami_on ? "2 clicks for code" : "2 clicks to stop")

static GBitmap *s_res_img_snooze;

static Window *s_window;
static GBitmap *s_res_img_standby;
static GBitmap *s_res_img_settings;
static GFont s_res_clock_font;
static GFont s_res_ampm_font;
static GFont s_res_box_font;
static GFont s_res_box_font_small;
static ActionBarLayer *action_layer;
static Layer *clock_layer;
static Layer *onoff_layer;
static Layer *info_layer;

// Horizontal inset for text inside the top and bottom boxes. On the Round 2 the boxes run
// edge to edge, so the text is pulled in to keep it clear of the curved screen edge.
#if defined(PBL_ROUND) && BIG_SCREEN
#define BOX_TEXT_INSET 48
#else
#define BOX_TEXT_INSET 5
#endif

// Height of the AM/PM text box next to the clock digits

#if defined(PBL_ROUND) && BIG_SCREEN
  #define AMPM_HEIGHT 34
  #define TIME_HEIGHT 84
#else
  #define AMPM_HEIGHT IF_BIG_ELSE(34, 22)
  #define TIME_HEIGHT IF_BIG_ELSE(80, 36+10+16)
#endif

static void draw_box(Layer *layer, GContext *ctx, GColor border_color, GColor back_color, GColor text_color, char *text) {
  GRect bounds = layer_get_bounds(layer);
  graphics_context_set_fill_color(ctx, back_color);
  graphics_fill_rect(ctx, bounds, PBL_IF_RECT_ELSE(8, 0), GCornersAll);
  IF_3(graphics_context_set_stroke_width(ctx, 3)); 
  graphics_context_set_stroke_color(ctx, border_color);
  graphics_draw_round_rect(ctx, bounds, PBL_IF_RECT_ELSE(8, 0));
  graphics_context_set_text_color(ctx, text_color);
  
  int16_t text_w = bounds.size.w - (2 * BOX_TEXT_INSET);
  int16_t max_h = bounds.size.h - 2;
  
  // Measure the text with the normal font. If it is too tall for the box (e.g. the 3-line
  // "SNOOZING / GET OUT OF BED / MONITORING" status), fall back to the smaller font.
  GFont font = s_res_box_font;
  GSize text_size = graphics_text_layout_get_content_size(text, font, GRect(0, 0, text_w, 500),
                                                          GTextOverflowModeWordWrap, GTextAlignmentCenter);
  if (text_size.h > max_h) {
    font = s_res_box_font_small;
    text_size = graphics_text_layout_get_content_size(text, font, GRect(0, 0, text_w, 500),
                                                      GTextOverflowModeWordWrap, GTextAlignmentCenter);
  }
  if (text_size.h > max_h) text_size.h = max_h;
  
  graphics_draw_text(ctx, text, font, 
                     GRect(BOX_TEXT_INSET, ((bounds.size.h-text_size.h)/2)-4, text_w, text_size.h), 
                     GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
}

static void draw_onoff(Layer *layer, GContext *ctx) {
  GColor border_color;
  GColor fill_color;
#ifdef PBL_COLOR
  switch (s_onoff_mode) {
    case MODE_OFF:
      border_color = GColorRed;
      fill_color = GColorMelon;
      break;
    case MODE_ON:
      border_color = GColorJaegerGreen;
      fill_color = GColorMintGreen;
      break;
    case MODE_ACTIVE:
      border_color = GColorChromeYellow;
      fill_color = GColorPastelYellow;
      break;
  }
#else
  border_color = GColorWhite;
  fill_color = GColorBlack;
#endif
  draw_box(layer, ctx, border_color, fill_color, COLOR_FALLBACK(GColorBlack, GColorWhite), s_onoff_text);
}

static void draw_clock(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  
#ifdef PBL_RECT
  // Cover middle section of action bar to give more room for clock
  graphics_context_set_fill_color(ctx, GColorBlack);
#if BIG_SCREEN
  //graphics_fill_rect(ctx, GRect(bounds.size.w-ACTION_BAR_WIDTH-4, 6, ACTION_BAR_WIDTH+12, bounds.size.h), 0, GCornerNone);
  graphics_fill_rect(ctx, GRect(bounds.size.w-ACTION_BAR_WIDTH, 0, ACTION_BAR_WIDTH, bounds.size.h), 0, GCornerNone);
#else
  graphics_fill_rect(ctx, GRect(bounds.size.w-ACTION_BAR_WIDTH, 0, ACTION_BAR_WIDTH, bounds.size.h), 0, GCornerNone);
#endif
#endif
  
  graphics_context_set_text_color(ctx, GColorWhite);
  
  if (!clock_is_24h_style() && s_time_ampm[0] != '\0') {
    // 12-hour mode: draw the digits in the large font with AM/PM in a smaller font right
    // after them, and centre the pair as one group so AM/PM never ends up under the action bar
    GSize digits_size = graphics_text_layout_get_content_size(s_time_digits, s_res_clock_font, bounds,
                                                              GTextOverflowModeWordWrap, GTextAlignmentLeft);
    GSize ampm_size = graphics_text_layout_get_content_size(s_time_ampm, s_res_ampm_font, bounds,
                                                            GTextOverflowModeWordWrap, GTextAlignmentLeft);
    const int16_t gap = IF_BIG_ELSE(4,2);
    int16_t x = (bounds.size.w - (digits_size.w + gap + ampm_size.w)) / 2;
    if (x < 0) x = 0;

    graphics_draw_text(ctx, s_time_digits, s_res_clock_font, GRect(x, (bounds.size.h - TIME_HEIGHT)/2, bounds.size.w - x, TIME_HEIGHT),
                       GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);
    
    int16_t ampm_x = x + digits_size.w + gap;
    graphics_draw_text(ctx, s_time_ampm, s_res_ampm_font, 
                       GRect(ampm_x, (bounds.size.h - AMPM_HEIGHT) / 2, bounds.size.w - ampm_x, AMPM_HEIGHT),
                       GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);
  } else {
    

    // 24-hour mode: draw full time string with large font

    graphics_draw_text(ctx, s_time_digits, s_res_clock_font, GRect(0, (bounds.size.h - TIME_HEIGHT)/2, bounds.size.w, TIME_HEIGHT),
                       GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
  }
}

static void draw_info(Layer *layer, GContext *ctx) {
  draw_box(layer, ctx, COLOR_FALLBACK(GColorBlueMoon, GColorWhite), COLOR_FALLBACK(GColorPictonBlue, GColorBlack),
          COLOR_FALLBACK(GColorBlack, GColorWhite), s_info);
}

static void initialise_ui(void) {
  
  Layer *root_layer = NULL;
  GRect bounds; 
  s_window = window_create_fullscreen(&root_layer, &bounds);
  
  
#if BIG_SCREEN
  // Pebble Time 2 and Pebble Round 2
  // Custom Roboto Bold for the clock digits, scaled up from the Roboto Bold 49 used on the
  // older watches (49 x 260/180 on the Round 2, 49 x 196/144 on the Time 2, rounded down)
#ifdef PBL_ROUND
  s_res_clock_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_ROBOTO_BOLD_70));
  s_res_ampm_font  = fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD);
#else
  s_res_clock_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_ROBOTO_BOLD_64));
  s_res_ampm_font  = fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD);
#endif
  s_res_box_font = fonts_get_system_font(PBL_IF_RECT_ELSE(FONT_KEY_GOTHIC_28_BOLD, FONT_KEY_GOTHIC_24_BOLD));
  s_res_box_font_small = fonts_get_system_font(PBL_IF_RECT_ELSE(FONT_KEY_GOTHIC_24_BOLD, FONT_KEY_GOTHIC_18_BOLD));
#else
  // Custom Roboto Bold 49 (same size as the built-in FONT_KEY_ROBOTO_BOLD_SUBSET_49, but bundled
  // with the app so the clock looks the same whatever firmware the watch is running)
  s_res_clock_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_ROBOTO_BOLD_49));
  s_res_ampm_font  = fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD);
  s_res_box_font = fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD);
  s_res_box_font_small = fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD);
#endif
  
  // action_layer
  action_layer = action_bar_layer_create();
  action_bar_layer_add_to_window(action_layer, s_window);
  action_bar_layer_set_background_color(action_layer, GColorWhite);
  Layer *action_bar_root = action_bar_layer_get_layer(action_layer);
  
#ifdef PBL_RECT
  // Narrow the action bar and put it underneath the other layers on rectangular Pebbles
#if BIG_SCREEN
  layer_set_frame(action_bar_root, GRect(bounds.size.w-ACTION_BAR_WIDTH, 0, ACTION_BAR_WIDTH+3, bounds.size.h));
  layer_set_bounds(action_bar_root, GRect(-6, 0, ACTION_BAR_WIDTH+15, bounds.size.h));
#else
  layer_set_frame(action_bar_root, GRect(bounds.size.w-ACTION_BAR_WIDTH, 0, ACTION_BAR_WIDTH, bounds.size.h));
  IF_3(layer_set_bounds(action_bar_root, GRect(-6, 0, ACTION_BAR_WIDTH+11, bounds.size.h)));
#endif
  layer_add_child(root_layer, action_bar_root);
  
  // Top and bottom boxes sit left of the action bar, clock in the middle
  const int16_t box_h = IF_BIG_ELSE(80, 56);
  const int16_t box_w = bounds.size.w - ACTION_BAR_WIDTH - 5;
  
  clock_layer = layer_create_with_proc(root_layer, draw_clock,
                                      //  GRect(0, (bounds.size.h/2) - IF_BIG_ELSE(46, 32), 
                                      //        bounds.size.w, IF_BIG_ELSE(86, 65)));
                                        GRect(0, (bounds.size.h/2) - IF_BIG_ELSE(32, 26), 
                                             bounds.size.w, IF_BIG_ELSE(32*2, (26*2))));
  onoff_layer = layer_create_with_proc(root_layer, draw_onoff, GRect(2, 2, box_w, box_h));
  info_layer = layer_create_with_proc(root_layer, draw_info, GRect(2, bounds.size.h - box_h - 2, box_w, box_h));
#else
  // Round: boxes run edge to edge (clipped by the circle), clock is centred left of the action bar
  const int16_t box_h = IF_BIG_ELSE(78, 56);
  const int16_t box_w = bounds.size.w + IF_BIG_ELSE(20, 11);
  
  clock_layer = layer_create_with_proc(root_layer, draw_clock,
                                       GRect(0 - ACTION_BAR_WIDTH/2, (bounds.size.h/2) - IF_BIG_ELSE(48, 34), 
                                             bounds.size.w, IF_BIG_ELSE(92, 65)));
  onoff_layer = layer_create_with_proc(root_layer, draw_onoff,
                                       GRect(-10, IF_BIG_ELSE(14, (bounds.size.h/2)-82), box_w, box_h));
  info_layer = layer_create_with_proc(root_layer, draw_info,
                                      GRect(-10, IF_BIG_ELSE(bounds.size.h - box_h - 14, (bounds.size.h/2)+24), 
                                            box_w, box_h));
  
  // Put Action Bar on top for round Pebbles
  layer_remove_from_parent(action_bar_root);
  layer_add_child(root_layer, action_bar_root);
#endif
}

static void destroy_ui(void) {
  window_destroy(s_window);
  action_bar_layer_destroy(action_layer);
  layer_destroy(clock_layer);
  layer_destroy(onoff_layer);
  layer_destroy(info_layer);
  // Custom fonts have to be freed (system fonts don't)
  fonts_unload_custom_font(s_res_clock_font);
}

// Loads the action bar icons (if not already loaded)
static void load_icons(void) {
  if (s_res_img_standby == NULL) s_res_img_standby = gbitmap_create_with_resource(RESOURCE_ID_IMG_STANDBY);
  if (s_res_img_settings == NULL) s_res_img_settings = gbitmap_create_with_resource(RESOURCE_ID_IMG_SETTINGS);
  if (s_res_img_snooze == NULL) s_res_img_snooze = gbitmap_create_with_resource(RESOURCE_ID_IMG_SNOOZE);
}

// Frees the action bar icons (the action bar must not be showing them when this is called)
static void unload_icons(void) {
  if (s_res_img_standby != NULL) { gbitmap_destroy(s_res_img_standby); s_res_img_standby = NULL; }
  if (s_res_img_settings != NULL) { gbitmap_destroy(s_res_img_settings); s_res_img_settings = NULL; }
  if (s_res_img_snooze != NULL) { gbitmap_destroy(s_res_img_snooze); s_res_img_snooze = NULL; }
}

static void set_bar_icon(ButtonId button, GBitmap *icon) {
  if (icon != NULL)
    action_bar_layer_set_icon(action_layer, button, icon);
  else
    action_bar_layer_clear_icon(action_layer, button);
}

// Shows the snooze icons while an alarm is active (ringing, snoozing or monitoring),
// otherwise the standby and settings icons
static void refresh_icons(void) {
  bool active = (s_onoff_mode == MODE_ACTIVE);
  set_bar_icon(BUTTON_ID_UP, active ? s_res_img_snooze : s_res_img_standby);
  set_bar_icon(BUTTON_ID_DOWN, active ? s_res_img_snooze : s_res_img_settings);
}

static void handle_window_unload(Window* window) {
  destroy_ui();
  unload_icons();
}

// Handles timer event when app has been idle for X minutes and auto-closes app
static void autoclose_handler(void *data) {
  if (s_autoclose_timer != NULL) {
    s_autoclose_timer = NULL;
    if (s_autoclose_timeout != 0 && window_stack_get_top_window() == s_window) {
      // If autoclose is still enabled and the main window is on top, then exit the app
      window_stack_pop_all(false);
    }
  } 
}

// Stops any active auto-close timer
static void stop_autoclose_timer() {
  if (s_autoclose_timer != NULL) {
    app_timer_cancel(s_autoclose_timer);
    s_autoclose_timer = NULL;
  }
}

// Starts or restarts any acive auto-close timer if auto-close is still enabled
static void restart_autoclose_timer() {
  if (s_autoclose_timeout == 0 || s_onoff_mode == MODE_ACTIVE)
    // Stop auto-close if it has been disabled or an alarm is active
    stop_autoclose_timer();
  else {
    // Start or restart auto-close timer (timeout is stored in minutes)
    if (s_autoclose_timer == NULL)
      s_autoclose_timer = app_timer_register(s_autoclose_timeout * 60 * 1000, autoclose_handler, NULL);
    else
      app_timer_reschedule(s_autoclose_timer, s_autoclose_timeout * 60 * 1000); 
  }
}

static void handle_window_appear(Window* window) {
#ifdef PBL_PLATFORM_APLITE
  // Reload the icons that were freed while another window was covering this one
  load_icons();
  refresh_icons();
#endif
  restart_autoclose_timer();
}

static void handle_window_disappear(Window* window) {
  stop_autoclose_timer();
#ifdef PBL_PLATFORM_APLITE
  // Aplite is very short of memory, so free the action bar icons while another window
  // (Settings, alarm picker, etc.) is covering this one. Clear them from the action bar first.
  action_bar_layer_clear_icon(action_layer, BUTTON_ID_UP);
  action_bar_layer_clear_icon(action_layer, BUTTON_ID_DOWN);
  unload_icons();
#endif
}

static void set_onoff_text(const char *onoff_text) {
  strncpy(s_onoff_text, onoff_text, sizeof(s_onoff_text));
  layer_mark_dirty(onoff_layer);
}

// Updates the clock time
void update_clock() {
  clock_copy_time_string(current_time, sizeof(current_time));
  // Split "HH:MM AM" into digits and AM/PM for separate rendering
  char *space = strchr(current_time, ' ');
  if (space != NULL) {
    strncpy(s_time_digits, current_time, space - current_time);
    s_time_digits[space - current_time] = '\0';
    strncpy(s_time_ampm, space + 1, sizeof(s_time_ampm) - 1);
    s_time_ampm[sizeof(s_time_ampm) - 1] = '\0';
  } else {
    strncpy(s_time_digits, current_time, sizeof(s_time_digits) - 1);
    s_time_digits[sizeof(s_time_digits) - 1] = '\0';
    s_time_ampm[0] = '\0';
  }
  layer_mark_dirty(clock_layer);
}

void init_click_events(ClickConfigProvider click_config_provider) {
  window_set_click_config_provider(s_window, click_config_provider);
}

// Sets the alarms to show as Enabled or Disbaled
void update_onoff(bool on) {
  s_alarms_on = on;
  if (on) {
    s_onoff_mode = MODE_ON;
    set_onoff_text("Alarms Enabled");
    layer_set_hidden(info_layer, false);
  } else {
    s_onoff_mode = MODE_OFF;
    set_onoff_text("Alarms DISABLED");
    layer_set_hidden(info_layer, true);
  }
  restart_autoclose_timer();
}

// Updates the info at the bottom of the main window
void update_info(char* text) {
  strncpy(s_info, text, sizeof(s_info));
  layer_mark_dirty(info_layer);
}

// Updates the timeout period for the auto-close time and restarts the timer if appropriate
void update_autoclose_timeout(uint8_t timeout) {
  s_autoclose_timeout = timeout;
  restart_autoclose_timer();
}

// Updates the UI to show alarm as active or not
void show_alarm_ui(bool on, bool goob) {
  if (on) {
    s_onoff_mode = MODE_ACTIVE;
    stop_autoclose_timer();
    if (goob)
      set_onoff_text("GET UP!");
    else
      set_onoff_text("WAKEY! WAKEY!");
    char info[40];
    snprintf(info, sizeof(info), "Click to snooze\n%s", STOP_HINT());
    update_info(info);
  } else {
    update_onoff(s_alarms_on);
  }
  refresh_icons();
}

// Update the main window to show snoozing, smart alarm monitoring, or Get Out Of Bed alarm monitoring
void show_status(time_t alarm_time, status_enum status) {
  s_onoff_mode = MODE_ACTIVE;
  stop_autoclose_timer();
  switch (status) {
    case S_Snoozing:
      set_onoff_text("SNOOZING");
      break;
    case S_SmartMonitoring:
      set_onoff_text("SMART ALARM\nACTIVE");
      break;
    case S_GooBMonitoring:
      set_onoff_text("GET OUT OF BED\nMONITORING");
      break;
    case S_GooBSnooze:
      set_onoff_text("SNOOZING\nGET OUT OF BED\nMONITORING");
      break;
  }
  
  struct tm *t = localtime(&alarm_time);
  
  char time_str[8];
  gen_time_str(t->tm_hour, t->tm_min, time_str, sizeof(time_str));
  
  char info[40];
  snprintf(info, sizeof(info), "%s: %s\n%s", (status == S_Snoozing ? "Until" : "Alarm"), time_str, STOP_HINT());
  update_info(info);
  
  refresh_icons();
}

// Tells the main window whether the Konami code is needed to stop an alarm,
// so it can show the right instructions
void update_konami_mode(bool konami_on) {
  s_konami_on = konami_on;
}

// Show the main application window
void show_mainwin(uint8_t autoclose_timeout) {
  initialise_ui();
  load_icons();
  refresh_icons();
  s_autoclose_timeout = autoclose_timeout;
  window_set_window_handlers(s_window, (WindowHandlers) {
    .unload = handle_window_unload,
    .appear = handle_window_appear,
    .disappear = handle_window_disappear
  });
  
  update_clock();
  
  window_stack_push(s_window, true);
}

// Close the main application window
void hide_mainwin(void) {
  window_stack_remove(s_window, true);
}