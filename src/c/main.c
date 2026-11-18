#include <pebble.h>

#define SETTINGS_KEY 1
#define SECONDS_STEP 10

typedef struct ClaySettings {
  GColor BackgroundColor;
  GColor TimeColor;
  GColor HoursColor;
  GColor MinutesColor;
  GColor SecondsColor;
} ClaySettings;

static ClaySettings s_settings;
static Window *s_window;
static Layer *s_canvas;
static AppTimer *s_seconds_timer;

static void prv_default_settings(void) {
  s_settings.BackgroundColor = GColorBlack;
  s_settings.TimeColor = GColorWhite;
  s_settings.HoursColor = GColorVividCerulean;
  s_settings.MinutesColor = GColorFashionMagenta;
  s_settings.SecondsColor = GColorBrightGreen;
}

static void prv_save_settings(void) {
  persist_write_data(SETTINGS_KEY, &s_settings, sizeof(s_settings));
}

static void prv_load_settings(void) {
  prv_default_settings();
  persist_read_data(SETTINGS_KEY, &s_settings, sizeof(s_settings));
}

static GColor prv_track_color(GColor accent) {
  const int r = accent.r * 48;
  const int g = accent.g * 48;
  const int b = accent.b * 48;
  if ((r + g + b) < 48) {
    return GColorDarkGray;
  }
  return GColorFromRGB(r, g, b);
}

static GRect prv_square_inset(GRect bounds, int pad) {
  const GPoint center = grect_center_point(&bounds);
  int side = bounds.size.w < bounds.size.h ? bounds.size.w : bounds.size.h;
  side -= pad * 2;
  if (side < 8) {
    side = 8;
  }
  return GRect(center.x - side / 2, center.y - side / 2, side, side);
}

static void prv_draw_ring(GContext *ctx, GRect rect, uint16_t thickness,
                          int32_t angle_end, GColor accent) {
  graphics_context_set_fill_color(ctx, prv_track_color(accent));
  graphics_fill_radial(ctx, rect, GOvalScaleModeFitCircle, thickness, 0, TRIG_MAX_ANGLE);

  if (angle_end <= 0) {
    return;
  }

  graphics_context_set_fill_color(ctx, accent);
  graphics_fill_radial(ctx, rect, GOvalScaleModeFitCircle, thickness, 0, angle_end);

  const int cap_r = thickness / 2;
  if (cap_r <= 0) {
    return;
  }

  const GRect mid = grect_inset(rect, GEdgeInsets(cap_r));
  graphics_fill_circle(ctx, gpoint_from_polar(mid, GOvalScaleModeFitCircle, 0), cap_r);
  graphics_fill_circle(ctx, gpoint_from_polar(mid, GOvalScaleModeFitCircle, angle_end), cap_r);
}

static void prv_format_time(struct tm *tick_time, char *buffer, size_t length) {
  if (clock_is_24h_style()) {
    strftime(buffer, length, "%H:%M", tick_time);
    return;
  }

  strftime(buffer, length, "%I:%M", tick_time);
  if (buffer[0] == '0') {
    memmove(buffer, buffer + 1, strlen(buffer));
  }
}

static void prv_format_date(struct tm *tick_time, char *buffer, size_t length) {
  strftime(buffer, length, "%a %d", tick_time);
  for (char *p = buffer; *p; p++) {
    if (*p >= 'a' && *p <= 'z') {
      *p = (char)(*p - ('a' - 'A'));
    }
  }
}

static void canvas_update_proc(Layer *layer, GContext *ctx) {
  const GRect full = layer_get_bounds(layer);
  const GRect bounds = layer_get_unobstructed_bounds(layer);

  graphics_context_set_fill_color(ctx, s_settings.BackgroundColor);
  graphics_fill_rect(ctx, full, 0, GCornerNone);
  graphics_context_set_antialiased(ctx, true);

  const int side = bounds.size.w < bounds.size.h ? bounds.size.w : bounds.size.h;
  int pad = side * 8 / 260;
  int thickness = side * 12 / 260;
  int gap = side * 7 / 260;
  if (pad < 6) {
    pad = 6;
  }
  if (thickness < 8) {
    thickness = 8;
  }
  if (gap < 5) {
    gap = 5;
  }

  const GRect hours_rect = prv_square_inset(bounds, pad);
  const GRect minutes_rect = grect_inset(hours_rect, GEdgeInsets(thickness + gap));
  const GRect seconds_rect = grect_inset(minutes_rect, GEdgeInsets(thickness + gap));
  const GRect content = grect_inset(seconds_rect, GEdgeInsets(thickness + 4));

  time_t now = time(NULL);
  struct tm *tick_time = localtime(&now);

  const int hour12 = tick_time->tm_hour % 12;
  const int minute = tick_time->tm_min;
  const int second_q = (tick_time->tm_sec / SECONDS_STEP) * SECONDS_STEP;

  const int32_t hour_angle = DEG_TO_TRIGANGLE((hour12 * 60 + minute) * 360 / (12 * 60));
  const int32_t minute_angle = DEG_TO_TRIGANGLE(minute * 360 / 60);
  const int32_t second_angle = DEG_TO_TRIGANGLE(second_q * 360 / 60);

  prv_draw_ring(ctx, hours_rect, (uint16_t)thickness, hour_angle, s_settings.HoursColor);
  prv_draw_ring(ctx, minutes_rect, (uint16_t)thickness, minute_angle, s_settings.MinutesColor);
  prv_draw_ring(ctx, seconds_rect, (uint16_t)thickness, second_angle, s_settings.SecondsColor);

  static char time_buffer[8];
  static char date_buffer[16];
  prv_format_time(tick_time, time_buffer, sizeof(time_buffer));
  prv_format_date(tick_time, date_buffer, sizeof(date_buffer));

  const GFont time_font = fonts_get_system_font(FONT_KEY_LECO_42_NUMBERS);
  const GFont date_font = fonts_get_system_font(FONT_KEY_GOTHIC_18);

  const GSize time_size = graphics_text_layout_get_content_size(
      time_buffer, time_font, content, GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter);
  const GSize date_size = graphics_text_layout_get_content_size(
      date_buffer, date_font, content, GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter);

  const int stack_h = time_size.h + date_size.h + 2;
  int time_y = content.origin.y + (content.size.h - stack_h) / 2;
  if (time_y < content.origin.y) {
    time_y = content.origin.y;
  }

  const GRect time_rect = GRect(content.origin.x, time_y, content.size.w, time_size.h);
  const GRect date_rect = GRect(content.origin.x, time_y + time_size.h + 2, content.size.w, date_size.h);

  graphics_context_set_text_color(ctx, s_settings.TimeColor);
  graphics_draw_text(ctx, time_buffer, time_font, time_rect,
                     GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
  graphics_draw_text(ctx, date_buffer, date_font, date_rect,
                     GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
}

static void prv_mark_dirty(void) {
  if (s_canvas) {
    layer_mark_dirty(s_canvas);
  }
}

static void prv_seconds_timer_callback(void *context);

static void prv_schedule_seconds_timer(void) {
  if (s_seconds_timer) {
    app_timer_cancel(s_seconds_timer);
    s_seconds_timer = NULL;
  }

  time_t now = time(NULL);
  struct tm *tick_time = localtime(&now);
  int remainder = tick_time->tm_sec % SECONDS_STEP;
  int wait_sec = SECONDS_STEP - remainder;
  if (wait_sec <= 0) {
    wait_sec = SECONDS_STEP;
  }
  s_seconds_timer = app_timer_register((uint32_t)wait_sec * 1000, prv_seconds_timer_callback, NULL);
}

static void prv_seconds_timer_callback(void *context) {
  (void)context;
  s_seconds_timer = NULL;
  prv_mark_dirty();
  prv_schedule_seconds_timer();
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  (void)tick_time;
  (void)units_changed;
  prv_mark_dirty();
  prv_schedule_seconds_timer();
}

static void prv_inbox_received(DictionaryIterator *iter, void *context) {
  (void)context;
  bool changed = false;

  Tuple *bg = dict_find(iter, MESSAGE_KEY_BackgroundColor);
  if (bg) {
    s_settings.BackgroundColor = GColorFromHEX(bg->value->int32);
    changed = true;
  }
  Tuple *time_color = dict_find(iter, MESSAGE_KEY_TimeColor);
  if (time_color) {
    s_settings.TimeColor = GColorFromHEX(time_color->value->int32);
    changed = true;
  }
  Tuple *hours = dict_find(iter, MESSAGE_KEY_HoursColor);
  if (hours) {
    s_settings.HoursColor = GColorFromHEX(hours->value->int32);
    changed = true;
  }
  Tuple *minutes = dict_find(iter, MESSAGE_KEY_MinutesColor);
  if (minutes) {
    s_settings.MinutesColor = GColorFromHEX(minutes->value->int32);
    changed = true;
  }
  Tuple *seconds = dict_find(iter, MESSAGE_KEY_SecondsColor);
  if (seconds) {
    s_settings.SecondsColor = GColorFromHEX(seconds->value->int32);
    changed = true;
  }

  if (changed) {
    prv_save_settings();
    if (s_window) {
      window_set_background_color(s_window, s_settings.BackgroundColor);
    }
    prv_mark_dirty();
  }
}

static void prv_unobstructed_change(AnimationProgress progress, void *context) {
  (void)progress;
  (void)context;
  prv_mark_dirty();
}

static void prv_unobstructed_did_change(void *context) {
  (void)context;
  prv_mark_dirty();
}

static void window_load(Window *window) {
  Layer *root = window_get_root_layer(window);
  const GRect bounds = layer_get_bounds(root);

  window_set_background_color(window, s_settings.BackgroundColor);

  s_canvas = layer_create(bounds);
  layer_set_update_proc(s_canvas, canvas_update_proc);
  layer_add_child(root, s_canvas);

  UnobstructedAreaHandlers handlers = {
    .change = prv_unobstructed_change,
    .did_change = prv_unobstructed_did_change
  };
  unobstructed_area_service_subscribe(handlers, NULL);
}

static void window_unload(Window *window) {
  (void)window;
  unobstructed_area_service_unsubscribe();
  layer_destroy(s_canvas);
  s_canvas = NULL;
}

static void init(void) {
  prv_load_settings();

  s_window = window_create();
  window_set_window_handlers(s_window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload
  });
  window_stack_push(s_window, true);

  app_message_register_inbox_received(prv_inbox_received);
  app_message_open(256, 256);

  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
  prv_schedule_seconds_timer();
}

static void deinit(void) {
  if (s_seconds_timer) {
    app_timer_cancel(s_seconds_timer);
    s_seconds_timer = NULL;
  }
  tick_timer_service_unsubscribe();
  app_message_deregister_callbacks();
  window_destroy(s_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
