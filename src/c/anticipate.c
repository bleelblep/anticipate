#include <pebble.h>
#include "time_digits.h"

// Preserve the original centred 144 x 168 composition and its screen margins.
// Only reclaim the former complications: left column and bottom timeline.
enum {
  FACE_WIDTH = 144, FACE_HEIGHT = 168,
  TIME_LEFT = 4, TIME_TOP = 4, DIGIT_WIDTH = 66, DIGIT_HEIGHT = 78,
  DIGIT_GAP = 4, ROW_GAP = 4, SOURCE_WIDTH = 46, SOURCE_HEIGHT = 71
};

static Window *s_window;
static Layer *s_time_layer;
static int s_hour;
static int s_minute;

static bool digit_pixel(int digit, int x, int y) {
  return (DIGIT_ROWS[digit][y][x / 8] & (1 << (x % 8))) != 0;
}

// Scale runs from the original artwork with crisp edges and no bitmap buffers.
static void draw_digit(GContext *ctx, int digit, int x, int y) {
  const int width = DIGIT_WIDTHS[digit];
  for (int sy = 0; sy < SOURCE_HEIGHT; ++sy) {
    const int top = y + sy * DIGIT_HEIGHT / SOURCE_HEIGHT;
    const int bottom = y + (sy + 1) * DIGIT_HEIGHT / SOURCE_HEIGHT;
    int sx = 0;
    while (sx < width) {
      if (!digit_pixel(digit, sx, sy)) {
        ++sx;
        continue;
      }
      const int start = sx;
      while (sx < width && digit_pixel(digit, sx, sy)) {
        ++sx;
      }
      const int left = x + start * DIGIT_WIDTH / SOURCE_WIDTH;
      const int right = x + sx * DIGIT_WIDTH / SOURCE_WIDTH;
      graphics_fill_rect(ctx, GRect(left, top, right - left, bottom - top),
                         0, GCornerNone);
    }
  }
}

static void draw_pair(GContext *ctx, int value, int y) {
  const int ones = value % 10;
  const int tens = value / 10;
  const int right = TIME_LEFT + 2 * DIGIT_WIDTH + DIGIT_GAP;
  const int ones_width = DIGIT_WIDTHS[ones] * DIGIT_WIDTH / SOURCE_WIDTH;
  const int tens_width = DIGIT_WIDTHS[tens] * DIGIT_WIDTH / SOURCE_WIDTH;
  // Preserve the original right alignment and tight spacing around narrow 1s.
  const int ones_x = right - ones_width;
  draw_digit(ctx, ones, ones_x, y);
  draw_digit(ctx, tens, ones_x - DIGIT_GAP - tens_width, y);
}

static void time_update_proc(Layer *layer, GContext *ctx) {
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_rect(ctx, layer_get_bounds(layer), 0, GCornerNone);
  graphics_context_set_fill_color(ctx, GColorWhite);
  draw_pair(ctx, s_hour, TIME_TOP);
  draw_pair(ctx, s_minute, TIME_TOP + DIGIT_HEIGHT + ROW_GAP);
}

static void update_time(struct tm *tick_time, TimeUnits changed) {
  (void)changed;
  s_hour = tick_time->tm_hour;
  if (!clock_is_24h_style()) {
    s_hour %= 12;
    if (s_hour == 0) s_hour = 12;
  }
  s_minute = tick_time->tm_min;
  if (s_time_layer) layer_mark_dirty(s_time_layer);
}

static void window_load(Window *window) {
  Layer *root = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(root);
  s_time_layer = layer_create(GRect((bounds.size.w - FACE_WIDTH) / 2,
                                   (bounds.size.h - FACE_HEIGHT) / 2,
                                   FACE_WIDTH, FACE_HEIGHT));
  if (!s_time_layer) return;
  layer_set_update_proc(s_time_layer, time_update_proc);
  layer_add_child(root, s_time_layer);
  time_t now = time(NULL);
  update_time(localtime(&now), MINUTE_UNIT);
}

static void window_unload(Window *window) {
  (void)window;
  if (s_time_layer) layer_destroy(s_time_layer);
  s_time_layer = NULL;
}

int main(void) {
  s_window = window_create();
  if (!s_window) return 1;
  window_set_background_color(s_window, GColorBlack);
  window_set_window_handlers(s_window, (WindowHandlers) {
    .load = window_load, .unload = window_unload
  });
  window_stack_push(s_window, true);
  tick_timer_service_subscribe(MINUTE_UNIT, update_time);
  app_event_loop();
  tick_timer_service_unsubscribe();
  window_destroy(s_window);
  return 0;
}
