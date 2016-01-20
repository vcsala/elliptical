#include <pebble.h>
#include "main.h"

static Window *s_main_window;
static Layer *s_background_layer;
static Layer *s_canvas_layer;

static char s_day_buffer[BUFFER_LENGTH + 1] = "";

static GPoint s_center;
static Time s_last_time;
static bool s_bluetooth_active = false;

// Curves for the ellipses
static GPoint s_external_curve[NUM_OF_POINTS + 1];
static GPoint s_internal_curve[NUM_OF_POINTS + 1];
static GPathInfo s_external;
static GPathInfo s_internal;

// Simple bluetooth icon
static const GPathInfo BLUETOOTH_PATH_INFO = {
  .num_points = 6,
  .points = (GPoint []) {{1, BLUETOOTH_STEP + 1}, 
                         {BLUETOOTH_STEP * 2, BLUETOOTH_STEP * 3}, 
                         {BLUETOOTH_STEP, BLUETOOTH_STEP * 4}, 
                         {BLUETOOTH_STEP, 0}, 
                         {BLUETOOTH_STEP * 2, BLUETOOTH_STEP},
                         {1, BLUETOOTH_STEP * 3 - 1}}
};

// Paths
static GPath *s_external_path = NULL;
static GPath *s_internal_path = NULL;
static GPath *s_bluetooth = NULL;

static void create_curves(GPoint center, GRect bounds) {
  float x_radius = (float)bounds.size.w / 2.0;
  float y_radius = (float)bounds.size.h / 2.0;
  float angle;
  
  for (int i = 0; i < NUM_OF_POINTS + 1; i++) {
    angle = (TRIG_MAX_ANGLE * (float)i) / (float)NUM_OF_POINTS;
    s_external_curve[i].x = (int16_t)(sin_lookup(angle) * (x_radius - EXTERNAL_MARGIN) / TRIG_MAX_RATIO) + center.x;
    s_external_curve[i].y = (int16_t)(-cos_lookup(angle) * (y_radius - EXTERNAL_MARGIN) / TRIG_MAX_RATIO) + center.y;
    s_internal_curve[i].x = (int16_t)(sin_lookup(angle) * (x_radius - INTERNAL_MARGIN) / TRIG_MAX_RATIO) + center.x;
    s_internal_curve[i].y = (int16_t)(-cos_lookup(angle) * (y_radius - INTERNAL_MARGIN) / TRIG_MAX_RATIO) + center.y;    
  }
      
  s_external.num_points = NUM_OF_POINTS + 1;
  s_external.points = s_external_curve;
  s_internal.num_points = NUM_OF_POINTS + 1;
  s_internal.points = s_internal_curve;
      
  s_external_path = gpath_create(&s_external);
  s_internal_path = gpath_create(&s_internal);
  s_bluetooth = gpath_create(&BLUETOOTH_PATH_INFO);
  
  gpath_move_to(s_bluetooth, GPoint(bounds.size.w * 3 / 4 - BLUETOOTH_WIDTH / 2, bounds.size.h / 2 - BLUETOOTH_HEIGHT / 2));
}
      
static void destroy_curves() {
  if (s_external_path != NULL) {
    gpath_destroy(s_external_path);
    s_external_path = NULL;
  }
  
  if (s_internal_path != NULL) {
    gpath_destroy(s_internal_path);
    s_internal_path = NULL;
  }
  
  if (s_bluetooth != NULL) {
    gpath_destroy(s_bluetooth);
    s_bluetooth = NULL;
  }
}

static void tick_handler(struct tm *tick_time, TimeUnits changed) {
  // Store time
  s_last_time.hours = tick_time->tm_hour;
  s_last_time.hours -= (s_last_time.hours > 12) ? 12 : 0;
  s_last_time.minutes = tick_time->tm_min;

  // Get day
  strftime(s_day_buffer, sizeof(s_day_buffer), "%a %e", tick_time);

  // Redraw
  if (s_canvas_layer) {
    layer_mark_dirty(s_canvas_layer);
  }
}

static void bluetooth_handler(bool state) {
  s_bluetooth_active = state;
  
  // Redraw
  if (s_canvas_layer) {
    layer_mark_dirty(s_canvas_layer);
  }  
}

static void update_background_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  int16_t x_radius = (int16_t)(bounds.size.w / 2);
  int16_t y_radius = (int16_t)(bounds.size.h / 2);
  float tick_angle;
  GPoint tick1;
  GPoint tick2;
  float x_tick_length = TICK_LENGTH;
  float y_tick_length = x_tick_length * ((float)bounds.size.h / (float)bounds.size.w);

  // Clock face dial
  graphics_context_set_antialiased(ctx, ANTIALIASING);
  graphics_context_set_fill_color(ctx, CLOCKFACE_COLOR);
  graphics_context_set_stroke_width(ctx, THICK_LINE);
  gpath_draw_filled(ctx, s_external_path);
  graphics_context_set_stroke_color(ctx, CLOCKFACE_COLOR);
  gpath_draw_outline(ctx, s_external_path);
  graphics_context_set_stroke_color(ctx, DECOR_COLOR);
  gpath_draw_outline(ctx, s_internal_path);
  
  for (int i = 0; i < 360; i += 6) {
    tick_angle = TRIG_MAX_ANGLE * (float)i / 360;
    tick1.x = (int16_t)(sin_lookup(tick_angle) * (float)(x_radius - INTERNAL_MARGIN) / TRIG_MAX_RATIO) + s_center.x;
    tick1.y = (int16_t)(-cos_lookup(tick_angle) * (float)(y_radius - INTERNAL_MARGIN) / TRIG_MAX_RATIO) + s_center.y;
    tick2.x = (int16_t)(sin_lookup(tick_angle) * ((float)(x_radius - INTERNAL_MARGIN) - x_tick_length) / TRIG_MAX_RATIO) + s_center.x;
    tick2.y = (int16_t)(-cos_lookup(tick_angle) * ((float)(y_radius - INTERNAL_MARGIN) - y_tick_length) / TRIG_MAX_RATIO) + s_center.y;

    if (i % 30 == 0) {
      graphics_context_set_stroke_width(ctx, THICK_LINE);
    } else {
      graphics_context_set_stroke_width(ctx, THIN_LINE);      
    }
    
    graphics_context_set_stroke_color(ctx, DECOR_COLOR);
    graphics_draw_line(ctx, tick1, tick2);
  }
}

static void update_canvas_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  int16_t x_radius = (int16_t)(bounds.size.w / 2);
  int16_t y_radius = (int16_t)(bounds.size.h / 2);

  // Calculate angles and adjust for minutes through the hour
  float minute_angle = TRIG_MAX_ANGLE * s_last_time.minutes / 60;
  float hour_angle = TRIG_MAX_ANGLE * s_last_time.hours / 12;
  hour_angle += (minute_angle / TRIG_MAX_ANGLE) * (TRIG_MAX_ANGLE / 12);
  
  // Plot hands
  GPoint minute_hand = (GPoint) {
    .x = (int16_t)(sin_lookup(TRIG_MAX_ANGLE * s_last_time.minutes / 60) * (x_radius - MINUTE_HAND_MARGIN) / TRIG_MAX_RATIO) + s_center.x,
    .y = (int16_t)(-cos_lookup(TRIG_MAX_ANGLE * s_last_time.minutes / 60) * (y_radius - MINUTE_HAND_MARGIN) / TRIG_MAX_RATIO) + s_center.y,
  };
  GPoint hour_hand = (GPoint) {
    .x = (int16_t)(sin_lookup(hour_angle) * (x_radius - HOUR_HAND_MARGIN) / TRIG_MAX_RATIO) + s_center.x,
    .y = (int16_t)(-cos_lookup(hour_angle) * (y_radius - HOUR_HAND_MARGIN) / TRIG_MAX_RATIO) + s_center.y,
  };
  
  GSize txtbox = graphics_text_layout_get_content_size(s_day_buffer, 
                                                       fonts_get_system_font(FONT_KEY_GOTHIC_14), 
                                                       GRect((bounds.size.w - TEXT_BOX_WIDTH) / 2, bounds.size.h * 3 / 4 - TEXT_BOX_HEIGHT / 2,
                                                            TEXT_BOX_WIDTH, TEXT_BOX_HEIGHT), 
                                                       GTextOverflowModeTrailingEllipsis , 
                                                       GTextAlignmentCenter);
  txtbox.h += 5;
  txtbox.w += 8;
                                  
  // --- DRAWING ---
  graphics_context_set_antialiased(ctx, ANTIALIASING);

  // Day
  graphics_context_set_text_color(ctx, CLOCKFACE_COLOR);
  graphics_context_set_fill_color(ctx, DAY_BOX_COLOR);
  graphics_fill_rect(ctx, 
                     GRect((bounds.size.w - txtbox.w) / 2, bounds.size.h * 3 / 4 - txtbox.h / 2,
                           txtbox.w, txtbox.h),
                     TEXT_BOX_CORNER,
                     GCornersAll);
  graphics_draw_text(ctx, 
                     s_day_buffer, 
                     fonts_get_system_font(FONT_KEY_GOTHIC_14), 
                     GRect((bounds.size.w - txtbox.w) / 2, bounds.size.h * 3 / 4 - txtbox.h / 2,
                           txtbox.w, txtbox.h),
                     GTextOverflowModeTrailingEllipsis, 
                     GTextAlignmentCenter, 
                     NULL);
  
  // Bluetooth
  if (s_bluetooth_active) {
    graphics_context_set_stroke_width(ctx, 1);
    graphics_context_set_fill_color(ctx, BLUETOOTH_COLOR);
    gpath_draw_outline_open(ctx, s_bluetooth);
  }
  
  // Hands
  graphics_context_set_stroke_color(ctx, MINUTE_HAND_COLOR);
  graphics_context_set_stroke_width(ctx, THICK_LINE);
  graphics_draw_line(ctx, s_center, minute_hand);
  graphics_draw_circle(ctx, s_center, CENTER_RADIUS + 1);
    
  graphics_context_set_stroke_color(ctx, HOUR_HAND_COLOR);
  graphics_context_set_stroke_width(ctx, THICK_LINE);
  graphics_draw_line(ctx, s_center, hour_hand);
  
#ifdef PBL_PLATFORM_APLITE
  graphics_context_set_stroke_color(ctx, CLOCKFACE_COLOR);
  graphics_context_set_stroke_width(ctx, THIN_LINE);
  graphics_draw_line(ctx, s_center, hour_hand);
#endif

  graphics_context_set_fill_color(ctx, CLOCKFACE_COLOR);
  graphics_context_set_stroke_color(ctx, HOUR_HAND_COLOR);
  graphics_context_set_stroke_width(ctx, THICK_LINE);
  graphics_fill_circle(ctx, s_center, CENTER_RADIUS);
  graphics_draw_circle(ctx, s_center, CENTER_RADIUS);
}

static void window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  s_center = grect_center_point(&bounds);

  s_background_layer = layer_create(bounds);
  layer_set_update_proc(s_background_layer, update_background_proc);
  layer_add_child(window_layer, s_background_layer);
  
  s_canvas_layer = layer_create(bounds);
  layer_set_update_proc(s_canvas_layer, update_canvas_proc);
  layer_add_child(window_layer, s_canvas_layer);
  
  create_curves(s_center, bounds);
  
  bluetooth_connection_service_subscribe(bluetooth_handler);
}

static void window_unload(Window *window) {
  bluetooth_connection_service_unsubscribe();
  
  layer_destroy(s_canvas_layer);
  layer_destroy(s_background_layer);
  
  destroy_curves();
}

static void init() {
  time_t t = time(NULL);
  struct tm *time_now = localtime(&t);
  s_bluetooth_active = bluetooth_connection_service_peek();
  tick_handler(time_now, MINUTE_UNIT);

  s_main_window = window_create();
  window_set_background_color(s_main_window, GColorBlack);
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload,
  });
  window_stack_push(s_main_window, true);

  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
}

static void deinit() {
  window_destroy(s_main_window);
}

int main() {
  init();
  app_event_loop();
  deinit();
}
