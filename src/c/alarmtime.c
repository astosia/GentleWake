#include "alarmtime.h"
#include "common.h"
#include "commonwin.h"
#include <pebble.h>

// Screen for setting alarm times

#define LEN_HOUR 4
#define LEN_MIN 4
#define MAX_TITLE 16

// Enum for indicating which alarm time part is selected
enum part {HOUR, MINUTE};
  
static int8_t s_day;
static uint8_t s_hour;
static uint8_t s_minute;
static char s_hourstr[LEN_HOUR];
static char s_minutestr[LEN_MIN];
static char s_alarmtitle[MAX_TITLE];
static enum part s_selected = HOUR;
static AlarmTimeCallBack s_set_event;

// Aplite is very short of memory once the main window and Settings are open, so there the
// action bar is drawn directly by the time layer, loading each icon only while it is drawn,
// instead of keeping an ActionBarLayer and three bitmaps in memory
#ifdef PBL_PLATFORM_APLITE
#define DRAW_OWN_ACTION_BAR 1
#define OWN_BAR_WIDTH ACTION_BAR_WIDTH
#else
#define OWN_BAR_WIDTH 0
#endif

static Window *s_window;
#ifndef DRAW_OWN_ACTION_BAR
static GBitmap *s_res_img_upaction;
static GBitmap *s_res_img_nextaction;
static GBitmap *s_res_img_downaction;
#endif
static GFont s_res_title_font;
static GFont s_res_time_font;
static GFont s_res_ampm_font;
#ifndef DRAW_OWN_ACTION_BAR
static ActionBarLayer *action_layer;
#endif
static Layer *time_layer;

// Layout of the time picker, relative to the centre of the time layer
#if BIG_SCREEN
#define TIME_BOX_W 64
#define TIME_BOX_H 50
#define TIME_BOX_Y (-24)
#define HOUR_GAP 8
#define MINUTE_GAP 8
#define SEP_W 16
#define TITLE_Y (-80)
#define TITLE_H 34
#define AMPM_Y 30
#define AMPM_W 90
#define AMPM_H 52
#define TIME_TEXT_DY 0   // nudge the Roboto digits up (-) or down (+) inside the highlight boxes
#else
#define TIME_BOX_W 48
#define TIME_BOX_H 36
#define TIME_BOX_Y (-16)
#define HOUR_GAP 6
#define MINUTE_GAP 5
#define SEP_W 12
#define TITLE_Y (-56)
#define TITLE_H 37
#define AMPM_Y 25
#define AMPM_W 52
#define AMPM_H 37
#define TIME_TEXT_DY 0
#endif

// Text is drawn in a rect a little taller than the highlight box, because Pebble skips drawing
// a line of text entirely if the rect is shorter than the font's line height
#define TEXT_RECT(box) GRect((box).origin.x, (box).origin.y + TIME_TEXT_DY, (box).size.w, (box).size.h + 10)

#ifdef DRAW_OWN_ACTION_BAR
// Loads one icon, draws it centred on (cx, cy), and frees it again straight away
static void draw_bar_icon(GContext *ctx, uint32_t resource_id, int16_t cx, int16_t cy) {
  GBitmap *bmp = gbitmap_create_with_resource(resource_id);
  if (bmp == NULL) return;
  GRect img = gbitmap_get_bounds(bmp);
  graphics_draw_bitmap_in_rect(ctx, bmp, GRect(cx - (img.size.w / 2), cy - (img.size.h / 2), img.size.w, img.size.h));
  gbitmap_destroy(bmp);
}

// Draws a white action bar with the up/next/down icons down the right-hand edge
static void draw_own_action_bar(GContext *ctx, GRect bounds) {
  int16_t bar_x = bounds.size.w - OWN_BAR_WIDTH;
  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_fill_rect(ctx, GRect(bar_x, 0, OWN_BAR_WIDTH, bounds.size.h), 0, GCornerNone);
  
  graphics_context_set_compositing_mode(ctx, GCompOpAssign);
  int16_t icon_x = bar_x + (OWN_BAR_WIDTH / 2);
  int16_t cy = bounds.size.h / 2;
  draw_bar_icon(ctx, RESOURCE_ID_IMAGE_UPACTION2, icon_x, cy - 50);
  draw_bar_icon(ctx, RESOURCE_ID_IMG_NEXTACTION, icon_x, cy);
  draw_bar_icon(ctx, RESOURCE_ID_IMAGE_DOWNACTION2, icon_x, cy + 50);
}
#endif

static void draw_time(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer); 
#ifdef DRAW_OWN_ACTION_BAR
  draw_own_action_bar(ctx, bounds);
#endif
  // Centre the picker in the area left of the action bar
  bounds.size.w -= OWN_BAR_WIDTH;
  int16_t cx = bounds.size.w / 2;
  int16_t cy = bounds.size.h / 2;
  
  graphics_context_set_text_color(ctx, GColorWhite);
  // Draw title
  graphics_draw_text(ctx, s_alarmtitle, s_res_title_font, 
                     GRect(3+PBL_IF_ROUND_ELSE(IF_BIG_ELSE(14, 10), 0), cy+TITLE_Y, bounds.size.w-6, TITLE_H), 
                     GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
  
  // Draw separater
  graphics_draw_text(ctx, ":", s_res_time_font, TEXT_RECT(GRect(cx-(SEP_W/2), cy+TIME_BOX_Y, SEP_W, TIME_BOX_H)), 
                     GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL);
  
  // Draw AM/PM indicator
  if (!clock_is_24h_style())
    graphics_draw_text(ctx, s_hour >= 12 ? "PM" : "AM", s_res_ampm_font, GRect(cx-(AMPM_W/2), cy+AMPM_Y, AMPM_W, AMPM_H), 
                     GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL);
  
  GRect hour_rect = GRect(cx-TIME_BOX_W-HOUR_GAP, cy+TIME_BOX_Y, TIME_BOX_W, TIME_BOX_H);
  GRect minute_rect = GRect(cx+MINUTE_GAP, cy+TIME_BOX_Y, TIME_BOX_W, TIME_BOX_H);
  
  // Set highlighted component
  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_fill_rect(ctx, (s_selected == HOUR) ? hour_rect : minute_rect, 0, GCornerNone);
  
  // Draw hour
  graphics_context_set_text_color(ctx, (s_selected == HOUR) ? GColorBlack : GColorWhite);
  graphics_draw_text(ctx, s_hourstr, s_res_time_font, TEXT_RECT(hour_rect), 
                     GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL);
  
  // Draw minutes
  graphics_context_set_text_color(ctx, (s_selected == MINUTE) ? GColorBlack : GColorWhite);
  graphics_draw_text(ctx, s_minutestr, s_res_time_font, TEXT_RECT(minute_rect), 
                     GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL);
  
}

static void initialise_ui(void) {
  
  GRect bounds;
  Layer *root_layer = NULL;
  s_window = window_create_fullscreen(&root_layer, &bounds);
  
#if BIG_SCREEN
  s_res_title_font = fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD);
  // Custom Roboto Bold 42 for the hour and minute digits (bundled font, freed in destroy_ui)
  s_res_time_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_ROBOTO_BOLD_42));
  // AM/PM uses the same Roboto as the digits (as the older watches use Bitham 30 for both)
  s_res_ampm_font = s_res_time_font;
#else
  s_res_title_font = fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD);
  s_res_time_font = fonts_get_system_font(FONT_KEY_BITHAM_30_BLACK);
  s_res_ampm_font = s_res_time_font;
#endif
#ifdef DRAW_OWN_ACTION_BAR
  // Full-width layer; draw_time() draws the action bar itself on the right
  time_layer = layer_create_with_proc(root_layer, draw_time, GRect(0, 0, bounds.size.w, bounds.size.h));
#else
  s_res_img_upaction = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_UPACTION2);
  s_res_img_nextaction = gbitmap_create_with_resource(RESOURCE_ID_IMG_NEXTACTION);
  s_res_img_downaction = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_DOWNACTION2);
  
  // action_layer
  action_layer = actionbar_create(s_window, root_layer, &bounds, s_res_img_upaction, s_res_img_nextaction, s_res_img_downaction);
  
  time_layer = layer_create_with_proc(root_layer, draw_time, 
                                     GRect(0, 0, bounds.size.w-ACTION_BAR_WIDTH, bounds.size.h));
#endif
}

static void destroy_ui(void) {
  window_destroy(s_window);
  layer_destroy(time_layer);
#if BIG_SCREEN
  // Custom fonts have to be freed (system fonts don't)
  fonts_unload_custom_font(s_res_time_font);
#endif
#ifndef DRAW_OWN_ACTION_BAR
  action_bar_layer_destroy(action_layer);
  gbitmap_destroy(s_res_img_upaction);
  gbitmap_destroy(s_res_img_nextaction);
  gbitmap_destroy(s_res_img_downaction);
#endif
}

static void handle_window_unload(Window* window) {
  destroy_ui();
}

// Redraws the currently set alarm time
static void update_alarmtime() {
  
  if (clock_is_24h_style()) {
    snprintf(s_hourstr, LEN_HOUR, "%d", s_hour);
  } else {
    snprintf(s_hourstr, LEN_HOUR, "%d", s_hour > 12 ? s_hour - 12 : s_hour == 0 ? 12 : s_hour);
  }
  snprintf(s_minutestr, LEN_MIN, "%.2d", s_minute);
  
  layer_mark_dirty(time_layer);
}

static void select_click_handler(ClickRecognizerRef recognizer, void *context) {
  
  if (s_selected == HOUR) {
    // Move to the minute part
    s_selected = MINUTE;
    update_alarmtime();
  } else {
    // Close this screen
    hide_alarmtime();
    // Pass the alarm day and time back
    s_set_event(s_day, s_hour, s_minute);
  }
  
}

static void up_click_handler(ClickRecognizerRef recognizer, void *context) {
  
  if (s_selected == HOUR) {
    // Increment hour (wrap around)
    s_hour++;
    s_hour %= 24;
  } else {
    // Increment minute (wrap around)
    s_minute++;
    s_minute %= 60;
  }
  
  update_alarmtime();
  
}

static void down_click_handler(ClickRecognizerRef recognizer, void *context) {
  
  if (s_selected == HOUR) {
    // Decrement hour (wrap around)
    s_hour += 23;
    s_hour %= 24;
  } else {
    // Decrement minute (wrap around)
    s_minute += 59;
    s_minute %= 60;
  }
  
  update_alarmtime();
  
}

static void click_config_provider(void *context) {
  // (Repeating subscriptions also handle single presses, so Up/Down are only subscribed once)
  window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
  window_single_repeating_click_subscribe(BUTTON_ID_UP, 50, up_click_handler);
  window_single_repeating_click_subscribe(BUTTON_ID_DOWN, 50, down_click_handler);
}

void show_alarmtime(int8_t day, uint8_t hour, uint8_t minute, AlarmTimeCallBack set_event) {
  initialise_ui();
  window_set_window_handlers(s_window, (WindowHandlers) {
    .unload = handle_window_unload,
  });
  
  s_selected = HOUR;
  // Store the passed in parameters
  s_day = day;
  s_hour = hour;
  s_minute = minute;
  
  // Store pointer to callback for when done
  s_set_event = set_event;
  
  char daystr[10];
  
  // Generate alarm screen time
  switch (day) {
    case -2:
      strncpy(s_alarmtitle, "One-Time Alarm", MAX_TITLE);
      break;
    case -1:
      strncpy(s_alarmtitle, "Alarm Every Day", MAX_TITLE);
      break;
    default:
      dayname(day, daystr, 10);
      snprintf(s_alarmtitle, MAX_TITLE, "%s Alarm", daystr);
      //strncpy(s_alarmtitle, "Alarm", MAX_TITLE);
      break;
  }
  
  update_alarmtime();
  
  window_set_click_config_provider(s_window, click_config_provider);
  
  window_stack_push(s_window, true);
}

void hide_alarmtime(void) {
  window_stack_remove(s_window, true);
}