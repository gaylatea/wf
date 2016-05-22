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
static TextLayer *s_time_layer;

static void time_layer_update(struct tm *tick_time, TimeUnits changed) {
  // Draw logic for the time display.
  static char s_time_buffer[6];
  strftime(s_time_buffer, sizeof(s_time_buffer),
           clock_is_24h_style() ? "%H:%M" : "%I:%M", tick_time);
  text_layer_set_text(s_time_layer, s_time_buffer);
}

static void time_layer_create(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  s_time_layer = text_layer_create(GRect(9, 62, (bounds.size.w - 18), 43));

  text_layer_set_background_color(s_time_layer, GColorBlack);
  text_layer_set_text_color(s_time_layer, GColorWhite);
  text_layer_set_text(s_time_layer, "00:00");
  text_layer_set_font(s_time_layer,
                      fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD));
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);

  layer_add_child(window_layer, text_layer_get_layer(s_time_layer));

  time_t temp = time(NULL);
  struct tm *tick_time = localtime(&temp);
  time_layer_update(tick_time, MINUTE_UNIT);

  tick_timer_service_subscribe(MINUTE_UNIT, time_layer_update);
}

static void time_layer_destroy() { text_layer_destroy(s_time_layer); }

// Date Display ///////////////////////////////////////////////////////////////
// Turns out this is useful to have on a watchface. Who would have guessed.
static TextLayer *s_date_layer;

static void date_layer_update(struct tm *tick_time, TimeUnits changed) {
  // Draw logic for the time display.
  static char s_date_buffer[12];
  strftime(s_date_buffer, sizeof(s_date_buffer), "%a %b %d", tick_time);
  text_layer_set_text(s_date_layer, s_date_buffer);
}

static void date_layer_create(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  s_date_layer = text_layer_create(GRect(14, 107, (bounds.size.w - 28), 20));

  text_layer_set_background_color(s_date_layer, GColorBlack);
  text_layer_set_text_color(s_date_layer, GColorWhite);
  text_layer_set_text(s_date_layer, "Sat May 21");
  text_layer_set_font(s_date_layer,
                      fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
  text_layer_set_text_alignment(s_date_layer, GTextAlignmentCenter);

  layer_add_child(window_layer, text_layer_get_layer(s_date_layer));

  time_t temp = time(NULL);
  struct tm *tick_time = localtime(&temp);
  date_layer_update(tick_time, DAY_UNIT);

  tick_timer_service_subscribe(DAY_UNIT, date_layer_update);
}

static void date_layer_destroy() { text_layer_destroy(s_date_layer); }

// Battery Monitoring /////////////////////////////////////////////////////////
// We store the full battery state instead of just the level, because we want
// to do colour switching based on if the charger is connected.
static BatteryChargeState s_battery_state;
static Layer *s_battery_layer;

static void battery_state_callback(BatteryChargeState state) {
  s_battery_state = state;
  layer_mark_dirty(s_battery_layer);
}

static void battery_layer_update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);

  // I don't think I've ever seen this many casts in a single line before.
  int width = (int)(float)(((float)s_battery_state.charge_percent / 100.0F) *
                           (float)bounds.size.w);

  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);

  // Select the proper colour for the current battery state.
  GColor battery_display_color = GColorWhite;
  if (s_battery_state.is_charging || s_battery_state.charge_percent == 100) {
    battery_display_color = GColorGreen;
  } else {
    if (s_battery_state.charge_percent >= 50) {
      battery_display_color = GColorWhite;
    } else {
      if (s_battery_state.charge_percent >= 30) {
        battery_display_color = GColorYellow;
      } else {
        // Less than 30% power left.
        battery_display_color = GColorRed;
      }
    }
  }
  graphics_context_set_fill_color(ctx, battery_display_color);
  graphics_fill_rect(ctx, GRect(0, 0, width, 2), 0, GCornerNone);
}

static void battery_layer_create(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  s_battery_layer = layer_create(GRect(14, 135, bounds.size.w - 28, 4));
  layer_set_update_proc(s_battery_layer, battery_layer_update_proc);
  layer_add_child(window_layer, s_battery_layer);

  battery_state_service_subscribe(battery_state_callback);
  battery_state_callback(battery_state_service_peek());
}

static void battery_layer_destroy() { layer_destroy(s_battery_layer); }

// Traffic Information ////////////////////////////////////////////////////////
// Displays travel-time (with traffic) to two configured locations, with the
// help of Google Maps API. Used to tell how bad commute traffic is at present.
static TextLayer *s_location_1_layer;
static TextLayer *s_location_2_layer;

static void location_layer_create(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
  int midpoint = (int)(bounds.size.w / 2);

  s_location_1_layer =
      text_layer_create(GRect(14, 141, (midpoint - 14 - 2), 25));

  text_layer_set_background_color(s_location_1_layer, GColorBlack);
  text_layer_set_text_color(s_location_1_layer, GColorWhite);
  text_layer_set_text(s_location_1_layer, "1000 min");
  text_layer_set_font(s_location_1_layer,
                      fonts_get_system_font(FONT_KEY_GOTHIC_18));
  text_layer_set_text_alignment(s_location_1_layer, GTextAlignmentCenter);

  layer_add_child(window_layer, text_layer_get_layer(s_location_1_layer));

  s_location_2_layer =
      text_layer_create(GRect(midpoint + 1, 141, midpoint - 14 - 1, 25));

  text_layer_set_background_color(s_location_2_layer, GColorBlack);
  text_layer_set_text_color(s_location_2_layer, GColorWhite);
  text_layer_set_text(s_location_2_layer, "1000 min");
  text_layer_set_font(s_location_2_layer,
                      fonts_get_system_font(FONT_KEY_GOTHIC_18));
  text_layer_set_text_alignment(s_location_2_layer, GTextAlignmentCenter);

  layer_add_child(window_layer, text_layer_get_layer(s_location_2_layer));
}

static void location_layer_destroy() {
  text_layer_destroy(s_location_1_layer);
  text_layer_destroy(s_location_2_layer);
}

// Ornamentation Layer ////////////////////////////////////////////////////////
// This layer is static, and contains UI elements not associated with any
// dynamic part of the watchface.
static Layer *s_ornamentation_layer;

static void ornamentation_layer_update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  int midpoint = (int)(bounds.size.w / 2);

  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_fill_rect(ctx, GRect(1, 0, bounds.size.w, 1), 0, GCornerNone);
  graphics_fill_rect(ctx, GRect(midpoint, 0, 1, bounds.size.h), 0, GCornerNone);
}

static void ornamentation_layer_create(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  s_ornamentation_layer = layer_create(
      GRect(13, 140, bounds.size.w - 27, bounds.size.h - 135 - 14));
  layer_set_update_proc(s_ornamentation_layer, ornamentation_layer_update_proc);
  layer_add_child(window_layer, s_ornamentation_layer);

  // Make sure this gets drawn after setup.
  layer_mark_dirty(s_ornamentation_layer);
}

static void ornamentation_layer_destroy() {
  layer_destroy(s_ornamentation_layer);
}

// Main Window ////////////////////////////////////////////////////////////////
static Window *s_main_window;

static void main_window_load(Window *window) {
  // Always default to a black background.
  window_set_background_color(window, GColorBlack);

  // Create the layers needed to display the watchface information.
  ornamentation_layer_create(window);
  time_layer_create(window);
  date_layer_create(window);
  battery_layer_create(window);
  location_layer_create(window);
}

static void main_window_unload(Window *window) {
  ornamentation_layer_destroy();
  time_layer_destroy();
  date_layer_destroy();
  battery_layer_destroy();
  location_layer_destroy();
}

// Helper Functions ///////////////////////////////////////////////////////////
static void init() {
  s_main_window = window_create();
  window_set_window_handlers(
      s_main_window,
      (WindowHandlers){.load = main_window_load, .unload = main_window_unload});
  window_stack_push(s_main_window, true);
}

static void deinit() { window_destroy(s_main_window); }

// Entry Point ////////////////////////////////////////////////////////////////
int main(void) {
  init();
  app_event_loop();
  deinit();
  return 0;
}

// End ////////////////////////////////////////////////////////////////////////
