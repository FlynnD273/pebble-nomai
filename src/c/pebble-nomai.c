#include "fctx.h"
#include "fpath.h"

static Window *s_window;
static Layer *s_mask_layer;
static FContext *fcontext;

#define PATH_COUNT 6
static uint32_t mask_path_resources[PATH_COUNT] = {
    RESOURCE_ID_MASK_Shadow,      RESOURCE_ID_MASK_Mid,

    RESOURCE_ID_MASK_LightShadow, RESOURCE_ID_MASK_Center,
    RESOURCE_ID_MASK_Face,        RESOURCE_ID_MASK_Highlight,
};
static GColor mask_path_colors[PATH_COUNT];
static FPath *mask_paths[PATH_COUNT];
#define SMALL 0
#define BIG 2048
#define ANIM_ZOOM_IN_DUR 1000
#define ANIM_ZOOM_OUT_DUR 2000
static uint32_t scale = BIG;
static bool is_animating = false;

static void prv_mask_draw(Layer *layer, GContext *ctx) {
  uint32_t y_offset = 296;
  GRect bounds = layer_get_bounds(layer);
  fctx_init_context(fcontext, ctx);
  uint8_t min = bounds.size.w < bounds.size.h ? bounds.size.w : bounds.size.h;
  fctx_set_scale(
      fcontext, FPointI(128, 128),
      FPoint(min * 16 * (scale + 128) / 128, min * 16 * (scale + 128) / 128));
  // fctx_set_pivot(fcontext, FPointZero);
  fctx_set_pivot(fcontext, FPointI(64, 64));
  fctx_set_offset(fcontext, FPoint(bounds.size.w * 16 / 2,
                                   bounds.size.h * 16 / 2 + -y_offset));
  FPoint offset = FPoint(0, y_offset);
#ifdef PBL_COLOR
  for (uint8_t i = 0; i < PATH_COUNT; i++) {
    fctx_begin_fill(fcontext);
    fctx_set_fill_color(fcontext, mask_path_colors[i]);
    fctx_draw_commands(fcontext, offset, mask_paths[i]->data,
                       mask_paths[i]->size);
    fctx_end_fill(fcontext);
  }
#else
  fctx_begin_fill(fcontext);
  fctx_set_fill_color(fcontext, GColorWhite);
  fctx_draw_commands(fcontext, offset, mask_paths[0]->data,
                     mask_paths[0]->size);
  fctx_end_fill(fcontext);
  GBitmap *fb = graphics_capture_frame_buffer(ctx);
  for (int y = 0; y < bounds.size.h; y++) {
    GBitmapDataRowInfo info = gbitmap_get_data_row_info(fb, y);
    for (int x = info.min_x; x <= info.max_x; x += 8) {
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
  fctx_begin_fill(fcontext);
  fctx_set_fill_color(fcontext, GColorWhite);
  fctx_draw_commands(fcontext, offset, mask_paths[1]->data,
                     mask_paths[1]->size);
  fctx_end_fill(fcontext);
  fb = graphics_capture_frame_buffer(ctx);
  for (int y = 0; y < bounds.size.h; y++) {
    GBitmapDataRowInfo info = gbitmap_get_data_row_info(fb, y);
    for (int x = info.min_x; x <= info.max_x; x += 8) {
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
  fctx_begin_fill(fcontext);
  for (uint8_t i = 2; i < PATH_COUNT; i++) {
    fctx_draw_commands(fcontext, offset, mask_paths[i]->data,
                       mask_paths[i]->size);
  }
  fctx_end_fill(fcontext);
#endif

  fctx_deinit_context(fcontext);
}

static void zoom_in_setup(Animation *animation) {
  is_animating = true;
  scale = SMALL;
  layer_mark_dirty(s_mask_layer);
}

static void zoom_in_update(Animation *animation,
                           const AnimationProgress progress) {
  scale = (int)progress * (BIG - SMALL) / ANIMATION_NORMALIZED_MAX + SMALL;
  layer_mark_dirty(s_mask_layer);
}

static void zoom_out_setup(Animation *animation) {
  is_animating = true;
  scale = BIG;
  layer_mark_dirty(s_mask_layer);
}

static void zoom_out_update(Animation *animation,
                            const AnimationProgress progress) {
  scale = (int)progress * (SMALL - BIG) / ANIMATION_NORMALIZED_MAX + BIG;
  layer_mark_dirty(s_mask_layer);
}

static void animation_teardown(Animation *animation) { is_animating = false; }

static const AnimationImplementation zoom_in = {.setup = zoom_in_setup,
                                                .update = zoom_in_update,
                                                .teardown = animation_teardown};

static const AnimationImplementation zoom_out = {.setup = zoom_out_setup,
                                                 .update = zoom_out_update,
                                                 .teardown =
                                                     animation_teardown};

Animation *animation;

static void do_zoom_out() {
  animation = animation_create();
  animation_set_implementation(animation, &zoom_out);
  animation_set_curve(animation, AnimationCurveEaseOut);
  animation_set_duration(animation, ANIM_ZOOM_OUT_DUR);
  animation_schedule(animation);
}

static void do_zoom_in() {
  animation = animation_create();
  animation_set_implementation(animation, &zoom_in);
  animation_set_curve(animation, AnimationCurveEaseIn);
  animation_set_duration(animation, ANIM_ZOOM_IN_DUR);
  animation_schedule(animation);
}

static void accel_tap(AccelAxisType axis, int32_t direction) {
  if (!is_animating) {
    do_zoom_in();
  }
}

static void prv_window_load(Window *window) {
  mask_path_colors[0] = GColorDarkGreen;
  mask_path_colors[1] = GColorArmyGreen;
  mask_path_colors[2] = GColorLimerick;
  mask_path_colors[3] = GColorMediumAquamarine;
  mask_path_colors[4] = GColorKellyGreen;
  mask_path_colors[5] = GColorBrass;

  fcontext = calloc(1, sizeof(FContext));
#ifdef PBL_COLOR
  fctx_enable_aa(false);
#endif
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  for (uint8_t i = 0; i < PATH_COUNT; i++) {
    mask_paths[i] = fpath_create_from_resource(mask_path_resources[i]);
  }

  s_mask_layer = layer_create(bounds);
  layer_set_update_proc(s_mask_layer, prv_mask_draw);
  layer_add_child(window_layer, s_mask_layer);
  accel_tap_service_subscribe(accel_tap);

  do_zoom_out();
}

static void prv_window_unload(Window *window) {
  layer_destroy(s_mask_layer);
  for (uint8_t i = 0; i < PATH_COUNT; i++) {
    fpath_destroy(mask_paths[i]);
  }
}

static void prv_init(void) {
  s_window = window_create();
  window_set_window_handlers(s_window, (WindowHandlers){
                                           .load = prv_window_load,
                                           .unload = prv_window_unload,
                                       });
  window_set_background_color(s_window, GColorBlack);
  window_stack_push(s_window, false);
}

static void prv_deinit(void) { window_destroy(s_window); }

int main(void) {
  prv_init();
  app_event_loop();
  prv_deinit();
}
