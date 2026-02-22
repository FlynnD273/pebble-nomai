#include <pebble-fctx/fctx.h>
#include <pebble-fctx/ffont.h>
#include <pebble-fctx/fpath.h>
#include <pebble.h>
// #define DEBUG
#define ANIMATION_DEMO

static Window *s_window;
static Layer *s_time_layer;
static Layer *s_mask_layer;
static FContext *fcontext;

typedef enum GaugeType {
  GaugeTypeWatchBattery = 0,
  GaugeTypePhoneBattery,
  GaugeType24Hour,
  GaugeType24HourInverted,
  GaugeTypeTemp,
  GaugeTypePrecip,
  GaugeTypeCount,
} GaugeType;

#define SETTINGS_KEY 0
typedef struct settings {
  uint16_t version;
  uint32_t zoom_in_duration;
  uint32_t zoom_out_duration;
  uint32_t zoom_pause_duration;
  GaugeType upper_gauge;
  GaugeType lower_gauge;
  bool controlBacklight;
  bool default_mask;
} Settings;
static Settings settings;

static void default_settings() {
  settings.version = 2;
  settings.zoom_in_duration = 1000;
  settings.zoom_out_duration = 2000;
  settings.zoom_pause_duration = 5000;
  settings.upper_gauge = GaugeTypePhoneBattery;
  settings.lower_gauge = GaugeTypeWatchBattery;
  settings.controlBacklight = true;
  settings.default_mask = true;
}

static void load_settings() {
  default_settings();
  persist_read_data(SETTINGS_KEY, &settings, sizeof(settings));
  switch (settings.version) {
  case 1:
    settings.version = 2;
    settings.default_mask = true;
  }
}

#define PATH_COUNT 10
static uint8_t mask_path_resources[PATH_COUNT] = {
    RESOURCE_ID_MASK_Eye,
    RESOURCE_ID_MASK_OuterShadow,
    RESOURCE_ID_MASK_Shadow,
    RESOURCE_ID_MASK_OuterMid,
    RESOURCE_ID_MASK_Mid,

    RESOURCE_ID_MASK_OuterLightShadow,
    RESOURCE_ID_MASK_LightShadow,
    RESOURCE_ID_MASK_Center,
    RESOURCE_ID_MASK_Face,
    RESOURCE_ID_MASK_Highlight,
};
static uint16_t mask_path_clip[PATH_COUNT] = {
    290, 112, 850, 270, 256, 220, 292, 1950, 1175, 256,
};
static GColor mask_path_colors[PATH_COUNT];
void init_colors() {
  size_t i = 0;
  mask_path_colors[i++] = GColorBlack;
  mask_path_colors[i++] = GColorDarkGreen;
  mask_path_colors[i++] = GColorDarkGreen;
  mask_path_colors[i++] = GColorArmyGreen;
  mask_path_colors[i++] = GColorArmyGreen;
  mask_path_colors[i++] = GColorLimerick;
  mask_path_colors[i++] = GColorLimerick;
  mask_path_colors[i++] = GColorMediumAquamarine;
  mask_path_colors[i++] = GColorKellyGreen;
  mask_path_colors[i++] = GColorBrass;
}
static FFont *font;
static FPath *mask_paths[PATH_COUNT];
#define SMALL 0
#define BIG 2048
static uint16_t scale = SMALL;
static AppTimer *timer = NULL;

static struct tm *current_time;

static uint8_t gauge_values[GaugeTypeCount];

static void request_phone_data(char *action);

static bool is_using_gaugeType(GaugeType type) {
  return settings.lower_gauge == type || settings.upper_gauge == type;
}

static void prv_time_draw(Layer *layer, GContext *ctx) {
  if (scale < 4) {
    return;
  }
  uint32_t y_move = 24 * 16;
  uint32_t text_y_offset =
      -36 * 16 / 2 + y_move - ((scale - SMALL) * y_move / (BIG - SMALL));
  uint32_t text_scale = (scale - SMALL) * 127 / (BIG - SMALL) + 129;
  GRect bounds = layer_get_bounds(layer);
  fctx_init_context(fcontext, ctx);
  char time_buf[6];
  if (clock_is_24h_style()) {
    strftime(time_buf, sizeof(time_buf), "%R", current_time);
  } else {
    if ((((current_time->tm_hour + 11) % 12) + 1) < 10) {
      time_buf[0] = '0' + (current_time->tm_hour % 12);
      time_buf[1] = ':';
      time_buf[2] = '0' + current_time->tm_min / 10;
      time_buf[3] = '0' + (current_time->tm_min % 10);
      time_buf[4] = '\0';
    } else {
      time_buf[0] = '0' + ((((current_time->tm_hour + 11) % 12) + 1) / 10);
      time_buf[1] = '0' + ((((current_time->tm_hour + 11) % 12) + 1) % 10);
      time_buf[2] = ':';
      time_buf[3] = '0' + current_time->tm_min / 10;
      time_buf[4] = '0' + (current_time->tm_min % 10);
      time_buf[5] = '\0';
    }
  }

  fctx_begin_fill(fcontext);
  uint16_t min = bounds.size.w < bounds.size.h ? bounds.size.w : bounds.size.h;
  fctx_set_text_em_height(fcontext, font, min * text_scale / 256 / 4);
  fctx_set_offset(fcontext, FPoint(bounds.size.w * 16 / 2,
                                   bounds.size.h * 16 / 2 + text_y_offset));
  fctx_set_fill_color(fcontext, GColorWhite);
  fctx_draw_string(fcontext, time_buf, font, GTextAlignmentCenter,
                   FTextAnchorMiddle);
  fctx_end_fill(fcontext);

  char date_buf[12];
  if (current_time->tm_mday < 10) {
    strftime(date_buf, sizeof(date_buf), "%a, %b ", current_time);
    date_buf[9] = '0' + current_time->tm_mday;
    date_buf[10] = '\0';
  } else {
    strftime(date_buf, sizeof(date_buf), "%a, %b %e", current_time);
  }

  fctx_begin_fill(fcontext);
  fctx_set_text_em_height(fcontext, font, min * text_scale / 256 / 8);
  fctx_set_offset(fcontext,
                  FPoint(bounds.size.w * 16 / 2,
                         bounds.size.h * 16 / 2 + min / 6 * 16 + text_y_offset -
                             36 * 16 * (256 - text_scale) / 256));
  fctx_set_fill_color(fcontext, GColorWhite);
  fctx_draw_string(fcontext, date_buf, font, GTextAlignmentCenter,
                   FTextAnchorMiddle);
  fctx_end_fill(fcontext);
  fctx_deinit_context(fcontext);

  uint16_t offset =
      1 + ((BIG - SMALL) - (scale - SMALL - 1)) * 8 / (BIG - SMALL);
#ifdef PBL_ROUND
  offset += 3;
#endif
  GRect rect = GRect(offset, offset, bounds.size.w - offset * 2,
                     bounds.size.h - offset * 2);
  if (scale < 670) {
    return;
  }

  graphics_context_set_stroke_color(ctx, GColorWhite);
  graphics_context_set_stroke_width(ctx, 1);
  for (uint16_t angle = 0; angle < 360; angle += 36 / 4) {
    graphics_draw_line(ctx,
                       gpoint_from_polar(rect, GOvalScaleModeFitCircle,
                                         DEG_TO_TRIGANGLE(angle)),
                       gpoint_from_polar(rect, GOvalScaleModeFitCircle,
                                         DEG_TO_TRIGANGLE(angle + 2)));
  }
  offset += 8;
  rect = GRect(offset, offset, bounds.size.w - offset * 2,
               bounds.size.h - offset * 2);
  graphics_context_set_stroke_width(ctx, 6);
  graphics_context_set_stroke_color(ctx, GColorPastelYellow);
  graphics_draw_arc(
      ctx, rect, GOvalScaleModeFitCircle,
      DEG_TO_TRIGANGLE(
          90 + 90 *
                   (100 - gauge_values[settings.lower_gauge] +
                    ((BIG - SMALL) - (scale - SMALL)) * 100 / (BIG - SMALL)) /
                   100),
      DEG_TO_TRIGANGLE(180));

  graphics_context_set_stroke_color(ctx, GColorCeleste);
  uint16_t angle_offset =
      90 *
      (gauge_values[settings.upper_gauge] -
       ((BIG - SMALL) - (scale - SMALL)) * 100 / (BIG - SMALL)) /
      100;
  if (angle_offset > 0) {
    graphics_draw_arc(ctx, rect, GOvalScaleModeFitCircle, DEG_TO_TRIGANGLE(270),
                      DEG_TO_TRIGANGLE(270 + angle_offset));
  }
}

static bool prv_should_skip_dither(GRect bounds, int16_t x, int16_t y) {
  int32_t radius = 12 * 16 * (scale - SMALL + 1) / (BIG - SMALL);
  return x < bounds.size.w / 2 + radius && x > bounds.size.w / 2 - radius &&
         y < bounds.size.h / 2 + radius && y > bounds.size.h / 2 - radius;
}

static void prv_mask_draw(Layer *layer, GContext *ctx) {
  if (scale == BIG) {
    return;
  }
  uint16_t y_offset = 296;
  GRect bounds = layer_get_bounds(layer);
  fctx_init_context(fcontext, ctx);
  uint16_t min = bounds.size.w < bounds.size.h ? bounds.size.w : bounds.size.h;
  fctx_set_scale(
      fcontext, FPointI(128, 128),
      FPoint(min * 16 * (scale + 128) / 128, min * 16 * (scale + 128) / 128));
  fctx_set_pivot(fcontext, FPointI(64, 64));
  fctx_set_offset(fcontext, FPoint(bounds.size.w * 16 / 2,
                                   bounds.size.h * 16 / 2 - y_offset));
  FPoint offset = FPoint(0, y_offset);
#ifdef PBL_COLOR
  for (uint8_t i = 0; i < PATH_COUNT; i++) {
    if (mask_path_clip[i] < scale) {
      continue;
    }
    fctx_begin_fill(fcontext);
    fctx_set_fill_color(fcontext, mask_path_colors[i]);
    fctx_draw_commands(fcontext, offset, mask_paths[i]->data,
                       mask_paths[i]->size);
    fctx_end_fill(fcontext);
  }
#else
  uint8_t i = 0;
  if (mask_path_clip[i] >= scale) {
    fctx_begin_fill(fcontext);
    fctx_set_fill_color(fcontext, GColorBlack);
    fctx_draw_commands(fcontext, offset, mask_paths[i]->data,
                       mask_paths[i]->size);
    fctx_end_fill(fcontext);
  }
  i++;
  GBitmap *fb;
  if (mask_path_clip[i] >= scale) {
    fctx_begin_fill(fcontext);
    fctx_set_fill_color(fcontext, GColorWhite);
    fctx_draw_commands(fcontext, offset, mask_paths[i]->data,
                       mask_paths[i]->size);
    fctx_end_fill(fcontext);
  }
  i++;
  if (mask_path_clip[i] >= scale) {
    fctx_begin_fill(fcontext);
    fctx_set_fill_color(fcontext, GColorWhite);
    fctx_draw_commands(fcontext, offset, mask_paths[i]->data,
                       mask_paths[i]->size);
    fctx_end_fill(fcontext);
    fb = graphics_capture_frame_buffer(ctx);
    for (int16_t y = 0; y < bounds.size.h; y++) {
      GBitmapDataRowInfo info = gbitmap_get_data_row_info(fb, y);
      for (int16_t x = info.min_x; x <= info.max_x; x += 8) {
        if (prv_should_skip_dither(bounds, x, y)) {
          continue;
        }
        char patt = 0;
        switch (y % 8) {
        case 0:
          patt = 0b00000000;
          break;
        case 1:
          patt = 0b00100010;
          break;
        case 2:
          patt = 0b00000000;
          break;
        case 3:
          patt = 0b10001010;
          break;
        case 4:
          patt = 0b00000000;
          break;
        case 5:
          patt = 0b00100010;
          break;
        case 6:
          patt = 0b00000000;
          break;
        case 7:
          patt = 0b10101000;
          break;
        }

        info.data[x / 8] &= patt;
      }
    }
    graphics_release_frame_buffer(ctx, fb);
  }
  i++;
  if (mask_path_clip[i] >= scale) {
    fctx_begin_fill(fcontext);
    fctx_set_fill_color(fcontext, GColorWhite);
    fctx_draw_commands(fcontext, offset, mask_paths[i]->data,
                       mask_paths[i]->size);
    fctx_end_fill(fcontext);
  }
  i++;
  if (mask_path_clip[i] >= scale) {
    fctx_begin_fill(fcontext);
    fctx_set_fill_color(fcontext, GColorWhite);
    fctx_draw_commands(fcontext, offset, mask_paths[i]->data,
                       mask_paths[i]->size);
    fctx_end_fill(fcontext);
    fb = graphics_capture_frame_buffer(ctx);
    for (int16_t y = 0; y < bounds.size.h; y++) {
      GBitmapDataRowInfo info = gbitmap_get_data_row_info(fb, y);
      for (int16_t x = info.min_x; x <= info.max_x; x += 8) {
        if (prv_should_skip_dither(bounds, x, y)) {
          continue;
        }
        char patt = 0;
        switch (y % 2) {
        case 0:
          patt = 0b01010101;
          break;
        case 1:
          patt = 0b10101010;
          break;
        }

        info.data[x / 8] &= patt;
      }
    }
    graphics_release_frame_buffer(ctx, fb);
  }
  i++;
  for (; i < PATH_COUNT; i++) {
    if (mask_path_clip[i] < scale) {
      continue;
    }
    fctx_begin_fill(fcontext);
    fctx_draw_commands(fcontext, offset, mask_paths[i]->data,
                       mask_paths[i]->size);
    fctx_end_fill(fcontext);
  }
#endif

  fctx_deinit_context(fcontext);
}

static void handle_battery(BatteryChargeState charge_state) {
  gauge_values[GaugeTypeWatchBattery] = charge_state.charge_percent;
  layer_mark_dirty(s_mask_layer);
}

static void zoom_in_setup(Animation *animation) {
  scale = SMALL;
  layer_mark_dirty(s_mask_layer);
#ifndef ANIMATION_DEMO
  if (settings.controlBacklight) {
    light_enable(true);
  }
#endif
}

static void zoom_in_update(Animation *animation,
                           const AnimationProgress progress) {
  scale = (int)progress * (BIG - SMALL) / ANIMATION_NORMALIZED_MAX + SMALL;
  layer_mark_dirty(s_mask_layer);
}

static void zoom_out_setup(Animation *animation) {
  scale = BIG;
  layer_mark_dirty(s_mask_layer);
#ifndef ANIMATION_DEMO
  if (settings.controlBacklight) {
    light_enable(true);
  }
#endif
}

static void zoom_out_update(Animation *animation,
                            const AnimationProgress progress) {
  scale = (int)progress * (SMALL - BIG) / ANIMATION_NORMALIZED_MAX + BIG;
  layer_mark_dirty(s_mask_layer);
}

static void zoom_in_teardown(Animation *animation) {
#ifndef ANIMATION_DEMO
  if (!settings.default_mask && settings.controlBacklight) {
    light_enable(false);
  }
#endif
}
static void zoom_out_teardown(Animation *animation) {
#ifndef ANIMATION_DEMO
  if (settings.default_mask && settings.controlBacklight) {
    light_enable(false);
  }
#endif
}

static const AnimationImplementation zoom_in = {.setup = zoom_in_setup,
                                                .update = zoom_in_update,
                                                .teardown = zoom_in_teardown};

static const AnimationImplementation zoom_out = {.setup = zoom_out_setup,
                                                 .update = zoom_out_update,
                                                 .teardown = zoom_out_teardown};

Animation *animation;

static void do_zoom_out() {
  animation = animation_create();
  animation_set_implementation(animation, &zoom_out);
  animation_set_curve(animation, AnimationCurveEaseInOut);
  animation_set_duration(animation, settings.zoom_out_duration);
  animation_schedule(animation);
}

static void do_zoom_in() {
  animation = animation_create();
  animation_set_implementation(animation, &zoom_in);
  animation_set_curve(animation, AnimationCurveEaseInOut);
  animation_set_duration(animation, settings.zoom_in_duration);
  animation_schedule(animation);
}

static void zoom_to_default() {
  timer = NULL;
  if (settings.default_mask) {
    do_zoom_out();
  } else {
    do_zoom_in();
  }
}

static void zoom_to_nondefault() {
  if (settings.default_mask) {
    do_zoom_in();
  } else {
    do_zoom_out();
  }
}

static void accel_tap(AccelAxisType axis, int32_t direction) {
  if (!animation_is_scheduled(animation)) {
    if (timer) {
      app_timer_reschedule(timer, settings.zoom_pause_duration);
    } else {
      zoom_to_nondefault();
      timer = app_timer_register(settings.zoom_pause_duration +
                                     settings.zoom_in_duration,
                                 zoom_to_default, NULL);
    }
  }
}

static void handle_minute_tick(struct tm *tick_time, TimeUnits units_changed) {
  if (units_changed & MINUTE_UNIT) {
    memcpy(current_time, tick_time, sizeof(struct tm));
    gauge_values[GaugeType24Hour] = current_time->tm_hour * 100 / 24;
    gauge_values[GaugeType24HourInverted] =
        (24 - current_time->tm_hour) * 100 / 24;
    if (current_time->tm_min % 5 == 0) {
      request_phone_data("Battery");
    }
    if (current_time->tm_min % 15 == 0) {
      if (is_using_gaugeType(GaugeTypeTemp) ||
          is_using_gaugeType(GaugeTypePrecip)) {
        request_phone_data("Weather");
      }
    }
    if (scale == BIG) {
      layer_mark_dirty(s_mask_layer);
    }
  }
}

static void pebblekit_connected(bool connected) {
  if (!connected) {
    gauge_values[GaugeTypePhoneBattery] = 0;
  }
}

static void prv_window_load(Window *window) {
  init_colors();

  fcontext = malloc(sizeof(FContext));
  font = ffont_create_from_resource(RESOURCE_ID_WildsFont);
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  for (uint8_t i = 0; i < PATH_COUNT; i++) {
    mask_paths[i] = fpath_create_from_resource(mask_path_resources[i]);
  }

  s_time_layer = layer_create(bounds);
  layer_set_update_proc(s_time_layer, prv_time_draw);
  layer_add_child(window_layer, s_time_layer);

  s_mask_layer = layer_create(bounds);
  layer_set_update_proc(s_mask_layer, prv_mask_draw);
  layer_add_child(window_layer, s_mask_layer);

  bluetooth_connection_service_subscribe(pebblekit_connected);
  accel_tap_service_subscribe(accel_tap);
  battery_state_service_subscribe(handle_battery);
  tick_timer_service_subscribe(MINUTE_UNIT, handle_minute_tick);

  handle_battery(battery_state_service_peek());
  time_t now = time(NULL);
  current_time = localtime(&now);
  handle_minute_tick(current_time, MINUTE_UNIT);

#if !defined(DEBUG) && !defined(ANIMATION_DEMO)
  zoom_to_default();
#endif
#ifdef ANIMATION_DEMO
  light_enable(true);
#endif
}

static void prv_window_unload(Window *window) {
  layer_destroy(s_mask_layer);
  for (uint8_t i = 0; i < PATH_COUNT; i++) {
    fpath_destroy(mask_paths[i]);
  }
  ffont_destroy(font);
  if (animation_is_scheduled(animation)) {
    animation_destroy(animation);
  }
  free(fcontext);
}

static void save_settings() {
  persist_write_data(SETTINGS_KEY, &settings, sizeof(settings));
}

static void request_phone_data(char *action) {
  DictionaryIterator *iter;
  AppMessageResult result = app_message_outbox_begin(&iter);

  if (result == APP_MSG_OK) {
    dict_write_cstring(iter, MESSAGE_KEY_Action, action);
    result = app_message_outbox_send();
  }
}

static void inbox_received_handler(DictionaryIterator *iter, void *context) {
  Tuple *tuple;
  tuple = dict_find(iter, MESSAGE_KEY_PhoneBattLevel);
  if (tuple) {
    gauge_values[GaugeTypePhoneBattery] = tuple->value->int32;
  }
  tuple = dict_find(iter, MESSAGE_KEY_WTemp);
  if (tuple) {
    gauge_values[GaugeTypeTemp] = tuple->value->int32;
  }
  tuple = dict_find(iter, MESSAGE_KEY_WPrecip);
  if (tuple) {
    gauge_values[GaugeTypePrecip] = tuple->value->int32;
  }
  tuple = dict_find(iter, MESSAGE_KEY_UpperGauge);
  if (tuple) {
    settings.upper_gauge = atoi(tuple->value->cstring);
  }
  tuple = dict_find(iter, MESSAGE_KEY_LowerGauge);
  if (tuple) {
    settings.lower_gauge = atoi(tuple->value->cstring);
  }
  tuple = dict_find(iter, MESSAGE_KEY_ZoomInDur);
  if (tuple) {
    settings.zoom_in_duration = tuple->value->int32;
  }
  tuple = dict_find(iter, MESSAGE_KEY_ZoomOutDur);
  if (tuple) {
    settings.zoom_out_duration = tuple->value->int32;
  }
  tuple = dict_find(iter, MESSAGE_KEY_ZoomPauseDur);
  if (tuple) {
    settings.zoom_pause_duration = tuple->value->int32;
  }
  tuple = dict_find(iter, MESSAGE_KEY_ControlBacklight);
  if (tuple) {
    settings.controlBacklight = tuple->value->int8;
  }
  tuple = dict_find(iter, MESSAGE_KEY_DefaultMask);
  if (tuple) {
    settings.default_mask = tuple->value->int8;
    zoom_to_default();
  }
  save_settings();
}

static void prv_init(void) {
  load_settings();
  s_window = window_create();
  window_set_window_handlers(s_window, (WindowHandlers){
                                           .load = prv_window_load,
                                           .unload = prv_window_unload,
                                       });
  window_set_background_color(s_window, GColorBlack);
  window_stack_push(s_window, false);

  app_message_register_inbox_received(inbox_received_handler);
  app_message_open(128, 128);
}

static void prv_deinit(void) { window_destroy(s_window); }

int main(void) {
  prv_init();
  app_event_loop();
  prv_deinit();
}
