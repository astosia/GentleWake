#include <pebble.h>
#include "msg.h"
#include "common.h"
#include "commonwin.h"

// Simple message window that can be set to auto-hide after a certain time

#define MAX_TITLE 20
#define MAX_MSG 150

static char *s_title;
static char *s_msg;
static AppTimer *s_autohide;

static Window *s_window;
static GFont s_res_title_font;
static GFont s_res_msg_font;
static GFont s_res_msg_font_small;
static Layer *s_msg_layer;
static GTextAttributes *s_attributes;

static void draw_msg(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer); 
  
  graphics_context_set_text_color(ctx, GColorBlack);
  // Draw title (pushed further down on the Round 2 so it isn't clipped by the circle)
  graphics_draw_text(ctx, s_title, s_res_title_font, 
                     GRect(5, PBL_IF_RECT_ELSE(-4, IF_BIG_ELSE(30, 19)), bounds.size.w-10, 32), 
                     GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
  
  // Message area
#if defined(PBL_ROUND) && BIG_SCREEN
  GRect msg_rect = GRect(3, 64, bounds.size.w-6, bounds.size.h-78);
#else
  GRect msg_rect = GRect(3, 25+PBL_IF_ROUND_ELSE(19, 0), bounds.size.w-6, bounds.size.h-28-PBL_IF_ROUND_ELSE(15, 0));
#endif
  
  // Use the larger message font if the message fits, otherwise fall back to the smaller one
  GFont font = s_res_msg_font;
  if (s_res_msg_font_small != s_res_msg_font) {
#ifdef PBL_ROUND
    GSize size = graphics_text_layout_get_content_size_with_attributes(s_msg, font, msg_rect, 
                   GTextOverflowModeWordWrap, GTextAlignmentCenter, s_attributes);
#else
    GSize size = graphics_text_layout_get_content_size(s_msg, font, msg_rect, 
                   GTextOverflowModeWordWrap, GTextAlignmentCenter);
#endif
    if (size.h >= msg_rect.size.h - 4) font = s_res_msg_font_small;
  }
  
  // Draw msg
  graphics_draw_text(ctx, s_msg, font, msg_rect, 
                     GTextOverflowModeWordWrap, GTextAlignmentCenter, s_attributes);
}

static void initialise_ui(void) {
  s_title = malloc(MAX_TITLE);
  s_msg = malloc(MAX_MSG);
  
  Layer *root_layer = NULL;
  GRect bounds; 
  s_window = window_create_fullscreen(&root_layer, &bounds);
  window_set_background_color(s_window, GColorWhite);
  
  s_res_title_font = fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD);
  s_res_msg_font = fonts_get_system_font(IF_BIG_ELSE(FONT_KEY_GOTHIC_24_BOLD, FONT_KEY_GOTHIC_18_BOLD));
  s_res_msg_font_small = fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD);
  
#ifdef PBL_ROUND
  s_attributes = graphics_text_attributes_create();
  // Enable text flow with an inset so the text follows the round screen edge
  graphics_text_attributes_enable_screen_text_flow(s_attributes, IF_BIG_ELSE(8, 5));
#endif
  
  s_msg_layer = layer_create_with_proc(root_layer, draw_msg, bounds);
}

static void destroy_ui(void) {
  window_destroy(s_window);
  s_window = NULL;
  layer_destroy(s_msg_layer);
#ifdef PBL_ROUND
  graphics_text_attributes_destroy(s_attributes);
#endif
  
  free(s_title);
  free(s_msg);
}

static void cancel_autohide(void) {
  if (s_autohide != NULL) {
    app_timer_cancel(s_autohide);
    s_autohide = NULL;
  }
}

static void handle_window_unload(Window* window) {
  cancel_autohide();
  destroy_ui();
}

static void auto_hide(void *data) {
  s_autohide = NULL;
  hide_msg();
}

// Show or update window with the given title, message, and seconds to hide the message after (0 means no auto-hide)
void show_msg(char *title, char *msg, uint8_t hide_after, bool vibe) {
  
  if (s_window == NULL) {
    initialise_ui();
    window_set_window_handlers(s_window, (WindowHandlers) {
      .unload = handle_window_unload,
    });
  }
  
  strncpy(s_title, title, MAX_TITLE);
  s_title[MAX_TITLE-1] = '\0';
  strncpy(s_msg, msg, MAX_MSG);
  s_msg[MAX_MSG-1] = '\0';
  layer_mark_dirty(s_msg_layer);
  
  window_stack_push(s_window, true);
  
  if (vibe) vibes_long_pulse();
  
  // Set auto-hide timer
  if (hide_after == 0) {
    cancel_autohide();
  } else {
    if (s_autohide != NULL)
      app_timer_reschedule(s_autohide, hide_after * 1000);
    else
      s_autohide = app_timer_register(hide_after * 1000, auto_hide, NULL);
  }
}

void hide_msg(void) {
  cancel_autohide();
  if (s_window != NULL && window_stack_contains_window(s_window)) window_stack_remove(s_window, true);
}