#include <pebble.h>

#define SETTINGS_KEY 1
#define STILL_MINUTES_TO_SLEEP 5
#define STILL_DELTA_MG 180
#define SECONDS_INTERVAL_ADAPTIVE 0
#define FLICK_LIVE_SECONDS 10
#define FACE_REF_PX 260
#define RING_PAD_PX 8
#define RING_THICKNESS_PX 12
#define RING_GAP_PX 7
#define RING_PAD_MIN 6
#define RING_THICKNESS_MIN 8
#define RING_GAP_MIN 5
#define CONTENT_INSET_EXTRA 4
#define TIME_DATE_GAP 2

typedef struct ClaySettings {
  GColor BackgroundColor;
  GColor TimeColor;
  GColor HoursColor;
  GColor MinutesColor;
  GColor SecondsColor;
  bool BatterySave;
  uint8_t SecondsInterval;
} ClaySettings;

static ClaySettings s_settings;
static Window *s_window;
static Layer *s_canvas;
static AppTimer *s_seconds_timer;
static bool s_sleeping;
static bool s_ticking;
static bool s_tap_subscribed;
static bool s_battery_subscribed;
static bool s_backlight_subscribed;
static bool s_backlight_on;
static time_t s_flick_until;
static uint8_t s_still_minutes;
static AccelData s_last_accel;
static bool s_have_accel;

static void prv_schedule_seconds_timer(void);
static void prv_enter_sleep(void);
static void prv_exit_sleep(void);
static void prv_apply_power_mode(void);
static void tick_handler(struct tm *tick_time, TimeUnits units_changed);
static int prv_tuple_int(const Tuple *tuple);

static bool prv_is_adaptive(void) {
  return s_settings.SecondsInterval == SECONDS_INTERVAL_ADAPTIVE;
}

static bool prv_adaptive_live(void) {
  if (s_backlight_on) {
    return true;
  }
  return time(NULL) < s_flick_until;
}

static uint8_t prv_sanitize_interval(int value) {
  if (value == SECONDS_INTERVAL_ADAPTIVE || value == 1 || value == 10 || value == 30) {
    return (uint8_t)value;
  }
  return 10;
}

static uint8_t prv_parse_interval_tuple(const Tuple *tuple) {
  if (!tuple) {
    return 10;
  }

  if (tuple->type == TUPLE_CSTRING) {
    if (tuple->length == 0) {
      return 10;
    }
    const uint8_t first = tuple->value->uint8;
    if (first == 'a' || first == 'A') {
      return SECONDS_INTERVAL_ADAPTIVE;
    }
    if (first < '0' || first > '9') {
      return 10;
    }
  }

  return prv_sanitize_interval(prv_tuple_int(tuple));
}

static GColor prv_tuple_color(const Tuple *tuple) {
  return GColorFromHEX(prv_tuple_int(tuple) & 0xFFFFFF);
}

static int32_t prv_progress_angle(int elapsed, int period) {
  if (elapsed <= 0 || period <= 0) {
    return 0;
  }
  return elapsed * TRIG_MAX_ANGLE / period;
}

static uint8_t prv_seconds_step(void) {
  if (prv_is_adaptive()) {
    return prv_adaptive_live() ? 1 : 10;
  }
  const uint8_t step = s_settings.SecondsInterval;
  if (step == 1 || step == 10 || step == 30) {
    return step;
  }
  return 10;
}

static void prv_default_settings(void) {
  s_settings.BackgroundColor = GColorBlack;
  s_settings.TimeColor = GColorWhite;
  s_settings.HoursColor = GColorPictonBlue;
  s_settings.MinutesColor = GColorVividCerulean;
  s_settings.SecondsColor = GColorBlueMoon;
  s_settings.BatterySave = false;
  s_settings.SecondsInterval = 10;
}

static void prv_save_settings(void) {
  persist_write_data(SETTINGS_KEY, &s_settings, sizeof(s_settings));
}

static void prv_load_settings(void) {
  prv_default_settings();
  persist_read_data(SETTINGS_KEY, &s_settings, sizeof(s_settings));
  s_settings.SecondsInterval = prv_sanitize_interval(s_settings.SecondsInterval);
}

static int prv_tuple_int(const Tuple *tuple) {
  if (!tuple) {
    return 0;
  }
  if (tuple->type == TUPLE_CSTRING) {
    int value = 0;
    for (const char *p = tuple->value->cstring; *p >= '0' && *p <= '9'; p++) {
      value = value * 10 + (*p - '0');
    }
    return value;
  }
  switch (tuple->length) {
    case 1:
      return (int)tuple->value->int8;
    case 2:
      return (int)tuple->value->int16;
    default:
      return (int)tuple->value->int32;
  }
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
  if (angle_end <= 0) {
    return;
  }

  graphics_context_set_fill_color(ctx, accent);
  graphics_fill_radial(ctx, rect, GOvalScaleModeFitCircle, thickness, 0, angle_end);
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

  if (s_sleeping) {
    graphics_context_set_fill_color(ctx, GColorBlack);
    graphics_fill_rect(ctx, full, 0, GCornerNone);
    return;
  }

  graphics_context_set_fill_color(ctx, s_settings.BackgroundColor);
  graphics_fill_rect(ctx, full, 0, GCornerNone);
  graphics_context_set_antialiased(ctx, true);

  const int side = bounds.size.w < bounds.size.h ? bounds.size.w : bounds.size.h;
  int pad = side * RING_PAD_PX / FACE_REF_PX;
  int thickness = side * RING_THICKNESS_PX / FACE_REF_PX;
  int gap = side * RING_GAP_PX / FACE_REF_PX;
  if (pad < RING_PAD_MIN) {
    pad = RING_PAD_MIN;
  }
  if (thickness < RING_THICKNESS_MIN) {
    thickness = RING_THICKNESS_MIN;
  }
  if (gap < RING_GAP_MIN) {
    gap = RING_GAP_MIN;
  }

  const GRect hours_rect = prv_square_inset(bounds, pad);
  const GRect minutes_rect = grect_inset(hours_rect, GEdgeInsets(thickness + gap));
  const GRect seconds_rect = grect_inset(minutes_rect, GEdgeInsets(thickness + gap));
  const GRect content = grect_inset(seconds_rect, GEdgeInsets(thickness + CONTENT_INSET_EXTRA));

  time_t now = time(NULL);
  struct tm *tick_time = localtime(&now);

  const int minute = tick_time->tm_min;
  const int step = prv_seconds_step();
  const int second_q = (tick_time->tm_sec / step) * step;
  const bool twenty_four = clock_is_24h_style();
  const int hour_period = twenty_four ? 24 : 12;
  const int hour_in_period = twenty_four ? tick_time->tm_hour : (tick_time->tm_hour % 12);

  const int32_t hour_angle = prv_progress_angle(hour_in_period * 60 + minute,
                                                hour_period * 60);
  const int32_t minute_angle = prv_progress_angle(minute, 60);
  const int32_t second_angle = prv_progress_angle(second_q, 60);

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

  const int stack_h = time_size.h + date_size.h + TIME_DATE_GAP;
  int time_y = content.origin.y + (content.size.h - stack_h) / 2;
  if (time_y < content.origin.y) {
    time_y = content.origin.y;
  }

  const GRect time_rect = GRect(content.origin.x, time_y, content.size.w, time_size.h);
  const GRect date_rect = GRect(content.origin.x, time_y + time_size.h + TIME_DATE_GAP,
                                content.size.w, date_size.h);

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

static void prv_cancel_seconds_timer(void) {
  if (s_seconds_timer) {
    app_timer_cancel(s_seconds_timer);
    s_seconds_timer = NULL;
  }
}

static void prv_set_ticking(bool on) {
  if (on && !s_ticking) {
    tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
    s_ticking = true;
  } else if (!on && s_ticking) {
    tick_timer_service_unsubscribe();
    s_ticking = false;
  }
}

static bool prv_is_plugged(void) {
  return battery_state_service_peek().is_plugged;
}

static int prv_abs_i(int value) {
  return value < 0 ? -value : value;
}

static bool prv_accel_still(const AccelData *now, const AccelData *prev) {
  return prv_abs_i(now->x - prev->x) < STILL_DELTA_MG &&
         prv_abs_i(now->y - prev->y) < STILL_DELTA_MG &&
         prv_abs_i(now->z - prev->z) < STILL_DELTA_MG;
}

static void prv_enter_sleep(void) {
  if (s_sleeping) {
    return;
  }
  s_sleeping = true;
  prv_cancel_seconds_timer();
  prv_set_ticking(false);
  if (s_window) {
    window_set_background_color(s_window, GColorBlack);
  }
  prv_mark_dirty();
}

static void prv_exit_sleep(void) {
  s_sleeping = false;
  s_still_minutes = 0;
  s_have_accel = false;
  if (s_window) {
    window_set_background_color(s_window, s_settings.BackgroundColor);
  }
  prv_set_ticking(true);
  prv_schedule_seconds_timer();
  prv_mark_dirty();
}

static void prv_check_idle(void) {
  if (!s_settings.BatterySave || s_sleeping) {
    return;
  }

  if (prv_is_plugged()) {
    prv_enter_sleep();
    return;
  }

  AccelData data;
  if (accel_service_peek(&data) != 0 || data.did_vibrate) {
    return;
  }

  if (s_have_accel && prv_accel_still(&data, &s_last_accel)) {
    if (s_still_minutes < 255) {
      s_still_minutes++;
    }
    if (s_still_minutes >= STILL_MINUTES_TO_SLEEP) {
      prv_enter_sleep();
      return;
    }
  } else {
    s_still_minutes = 0;
  }

  s_last_accel = data;
  s_have_accel = true;
}

static void prv_seconds_timer_callback(void *context);

static void prv_schedule_seconds_timer(void) {
  prv_cancel_seconds_timer();
  if (s_sleeping) {
    return;
  }

  const int step = prv_seconds_step();
  time_t now = 0;
  uint16_t ms = 0;
  time_ms(&now, &ms);
  struct tm *tick_time = localtime(&now);
  int remainder = tick_time->tm_sec % step;
  int wait_sec = step - remainder;
  if (wait_sec <= 0) {
    wait_sec = step;
  }

  uint32_t wait_ms = (uint32_t)wait_sec * 1000;
  if (wait_ms > ms) {
    wait_ms -= ms;
  }
  if (wait_ms == 0) {
    wait_ms = 1;
  }

  s_seconds_timer = app_timer_register(wait_ms, prv_seconds_timer_callback, NULL);
}

static void prv_seconds_timer_callback(void *context) {
  (void)context;
  s_seconds_timer = NULL;
  if (s_sleeping) {
    return;
  }
  prv_mark_dirty();
  prv_schedule_seconds_timer();
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  (void)tick_time;
  (void)units_changed;
  prv_mark_dirty();
  prv_check_idle();
  if (!s_sleeping) {
    prv_schedule_seconds_timer();
  }
}

static void prv_note_flick(void) {
  s_flick_until = time(NULL) + FLICK_LIVE_SECONDS;
}

static void prv_tap_handler(AccelAxisType axis, int32_t direction) {
  (void)axis;
  (void)direction;
  s_still_minutes = 0;
  s_have_accel = false;

  if (prv_is_adaptive()) {
    prv_note_flick();
  }

  if (s_settings.BatterySave && s_sleeping && !prv_is_plugged()) {
    prv_exit_sleep();
    return;
  }

  if (prv_is_adaptive() && !s_sleeping) {
    prv_schedule_seconds_timer();
    prv_mark_dirty();
  }
}

static void prv_backlight_handler(bool on) {
  s_backlight_on = on;
  if (!prv_is_adaptive() || s_sleeping) {
    return;
  }
  prv_schedule_seconds_timer();
  prv_mark_dirty();
}

static void prv_battery_handler(BatteryChargeState charge) {
  if (!s_settings.BatterySave) {
    return;
  }
  if (charge.is_plugged) {
    prv_enter_sleep();
  } else if (s_sleeping) {
    prv_exit_sleep();
  }
}

static void prv_sync_event_services(void) {
  const bool need_tap = s_settings.BatterySave || prv_is_adaptive();
  const bool need_battery = s_settings.BatterySave;
  const bool need_backlight = prv_is_adaptive();

  if (need_tap && !s_tap_subscribed) {
    accel_tap_service_subscribe(prv_tap_handler);
    s_tap_subscribed = true;
  } else if (!need_tap && s_tap_subscribed) {
    accel_tap_service_unsubscribe();
    s_tap_subscribed = false;
  }

  if (need_battery && !s_battery_subscribed) {
    battery_state_service_subscribe(prv_battery_handler);
    s_battery_subscribed = true;
  } else if (!need_battery && s_battery_subscribed) {
    battery_state_service_unsubscribe();
    s_battery_subscribed = false;
  }

  if (need_backlight && !s_backlight_subscribed) {
    backlight_service_subscribe(prv_backlight_handler);
    s_backlight_subscribed = true;
    s_backlight_on = light_is_on();
  } else if (!need_backlight && s_backlight_subscribed) {
    backlight_service_unsubscribe();
    s_backlight_subscribed = false;
    s_backlight_on = false;
    s_flick_until = 0;
  }
}

static void prv_apply_power_mode(void) {
  prv_sync_event_services();

  if (!s_settings.BatterySave) {
    s_still_minutes = 0;
    s_have_accel = false;
  }

  if (s_settings.BatterySave) {
    if (prv_is_plugged()) {
      prv_enter_sleep();
      return;
    }
    if (s_sleeping) {
      return;
    }
  } else if (s_sleeping) {
    prv_exit_sleep();
    return;
  }

  prv_set_ticking(true);
  prv_schedule_seconds_timer();
}

static void prv_inbox_received(DictionaryIterator *iter, void *context) {
  (void)context;
  bool changed = false;

  Tuple *bg = dict_find(iter, MESSAGE_KEY_BackgroundColor);
  if (bg) {
    s_settings.BackgroundColor = prv_tuple_color(bg);
    changed = true;
  }
  Tuple *time_color = dict_find(iter, MESSAGE_KEY_TimeColor);
  if (time_color) {
    s_settings.TimeColor = prv_tuple_color(time_color);
    changed = true;
  }
  Tuple *hours = dict_find(iter, MESSAGE_KEY_HoursColor);
  if (hours) {
    s_settings.HoursColor = prv_tuple_color(hours);
    changed = true;
  }
  Tuple *minutes = dict_find(iter, MESSAGE_KEY_MinutesColor);
  if (minutes) {
    s_settings.MinutesColor = prv_tuple_color(minutes);
    changed = true;
  }
  Tuple *seconds = dict_find(iter, MESSAGE_KEY_SecondsColor);
  if (seconds) {
    s_settings.SecondsColor = prv_tuple_color(seconds);
    changed = true;
  }
  Tuple *interval = dict_find(iter, MESSAGE_KEY_SecondsInterval);
  if (interval) {
    s_settings.SecondsInterval = prv_parse_interval_tuple(interval);
    changed = true;
  }
  Tuple *battery_save = dict_find(iter, MESSAGE_KEY_BatterySave);
  if (battery_save) {
    s_settings.BatterySave = prv_tuple_int(battery_save) != 0;
    changed = true;
  }

  if (changed) {
    prv_save_settings();
    if (s_window) {
      window_set_background_color(s_window, s_settings.BackgroundColor);
    }
    prv_apply_power_mode();
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

  prv_apply_power_mode();
}

static void deinit(void) {
  prv_cancel_seconds_timer();
  prv_set_ticking(false);
  if (s_tap_subscribed) {
    accel_tap_service_unsubscribe();
    s_tap_subscribed = false;
  }
  if (s_battery_subscribed) {
    battery_state_service_unsubscribe();
    s_battery_subscribed = false;
  }
  if (s_backlight_subscribed) {
    backlight_service_unsubscribe();
    s_backlight_subscribed = false;
  }
  app_message_deregister_callbacks();
  window_destroy(s_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
