// vim:expandtab:ts=2:softtabstop=2:shiftwidth=2
/**
 * Personal watchface for Pebble Time Steel.
 *
 * This is free and unencumbered software released into the public domain.
 *
 * Anyone is free to copy, modify, publish, use, compile, sell, or
 * distribute this software, either in source code form or as a compiled
 * binary, for any purpose, commercial or non-commercial, and by any
 * means.
 *
 * In jurisdictions that recognize copyright laws, the author or authors
 * of this software dedicate any and all copyright interest in the
 * software to the public domain. We make this dedication for the benefit
 * of the public at large and to the detriment of our heirs and
 * successors. We intend this dedication to be an overt act of
 * relinquishment in perpetuity of all present and future rights to this
 * software under copyright law.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * For more information, please refer to <http://unlicense.org/> 
 */
#include <pebble.h>

// Time ///////////////////////////////////////////////////////////////////////
static TextLayer* s_time_layer;

static void time_layer_update(struct tm* tick_time, TimeUnits changed) {
  // Draw logic for the time display.
  static char s_time_buffer[8];
  clock_copy_time_string(s_time_buffer, sizeof(s_time_buffer));
  text_layer_set_text(s_time_layer, s_time_buffer);
}

static void time_layer_create(Window* window) {
  Layer* window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
  s_time_layer = text_layer_create(GRect(0, 90, bounds.size.w, 40));

  text_layer_set_background_color(s_time_layer, GColorBlack);
  text_layer_set_text_color(s_time_layer, GColorWhite);
  text_layer_set_text(s_time_layer, "00:00");
  text_layer_set_font(s_time_layer, fonts_get_system_font(FONT_KEY_BITHAM_34_MEDIUM_NUMBERS));
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);

  layer_add_child(window_layer, text_layer_get_layer(s_time_layer));

  time_t temp = time(NULL);
  struct tm* tick_time = localtime(&temp);
  time_layer_update(tick_time, MINUTE_UNIT);

  tick_timer_service_subscribe(MINUTE_UNIT, time_layer_update);
}

static void time_layer_destroy() {
  text_layer_destroy(s_time_layer);
}

// Main Window ////////////////////////////////////////////////////////////////
static Window* s_main_window;

static void main_window_load(Window* window) {
  // Always default to a black background.
  window_set_background_color(window, GColorBlack);

  // Create the layers needed to display the watchface information.
  time_layer_create(window);
}

static void main_window_unload(Window* window) {
  time_layer_destroy();
}

// Helper Functions ///////////////////////////////////////////////////////////
static void init() {
  s_main_window = window_create();
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload
  });
  window_stack_push(s_main_window, true);
}

static void deinit() {
  window_destroy(s_main_window);
}

// Entry Point ////////////////////////////////////////////////////////////////
int main(void) {
  init();
  app_event_loop();
  deinit();
  return 0;
}

// End ////////////////////////////////////////////////////////////////////////
