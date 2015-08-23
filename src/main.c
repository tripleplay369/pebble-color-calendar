#include <pebble.h>
  
#define DAYS_PER_WEEK 7
#define TM_BEGIN_YEAR 1900
#define TITLE_MAX_LEN 15
#define SUNDAY_INDEX 0
#define SATUTDAY_INDEX 6
#define MAX_DAY_LEN 3
#define MAX_ROW_INDEX 5
  
static const char * MONTH_NAMES[] = {
  "January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December"
};

static const char * WEEK_LABELS[] = {
  "S", "M", "T", "W", "T", "F", "S"
};
  
static Window * main_window;
static Layer * main_layer;

static struct tm current_time;
static struct tm current_begin_month;
static struct tm day_to_draw;

static const int HEADER_HEIGHT = 45;
static const int TITLE_OFFSET = 0;
static const int TITLE_HEIGHT = 30;
static const int WEEK_LABELS_OFFSET_Y = 30;
static const int WEEK_LABELS_HEGHT = 10;
static const int DAY_WIDTH = 20;
static const int DAY_OFFSET_X = 3;
static const int DAY_OFFSET_Y = 45;
static const int DAY_HEIGHT = 20;
static const int CURRENT_DAY_RADIUS = 10;

static const char * TITLE_FONT_KEY = FONT_KEY_GOTHIC_24;
static const char * WEEK_LABEL_FONT_KEY = FONT_KEY_GOTHIC_09;
static const char * DAY_FONT_KEY = FONT_KEY_GOTHIC_14;
static uint8_t header_color = GColorLightGrayARGB8;
static uint8_t title_color = GColorSunsetOrangeARGB8;
static uint8_t sat_sun_color = GColorDarkGrayARGB8;
static uint8_t week_day_color = GColorBlackARGB8;
static uint8_t day_color = GColorDarkGrayARGB8;
static uint8_t current_day_color = GColorVeryLightBlueARGB8;
static uint8_t current_day_text_color = GColorWhiteARGB8;
static uint8_t other_month_day_color = GColorLightGrayARGB8;

static void start_drawing_month(struct tm * begin_month) {
  memcpy(&day_to_draw, begin_month, sizeof(struct tm));
  
  struct tm * tmp_time;
  while(day_to_draw.tm_wday != 0){
    day_to_draw.tm_mday--;
    time_t tmp_day = mktime(&day_to_draw);
    tmp_time = localtime(&tmp_day);
    memcpy(&day_to_draw, tmp_time, sizeof(struct tm));
  }
}

static void next_day_of_month() {
  day_to_draw.tm_mday++;
  
  struct tm * tmp_time;
  time_t tmp_day = mktime(&day_to_draw);
  tmp_time = localtime(&tmp_day);
    
  memcpy(&day_to_draw, tmp_time, sizeof(struct tm));
}

static void update_proc(Layer * layer, GContext * ctx) {
  GRect bounds = layer_get_bounds(layer);
  
  char title[TITLE_MAX_LEN] = {'\0'};
  snprintf(title, TITLE_MAX_LEN, "%s %d", MONTH_NAMES[current_begin_month.tm_mon], current_begin_month.tm_year + TM_BEGIN_YEAR);
  
  // draw header
  graphics_context_set_fill_color(ctx, (GColor)header_color);
  graphics_fill_rect(ctx, GRect(bounds.origin.x, bounds.origin.y, bounds.size.w, HEADER_HEIGHT), 5, GCornersAll);
  graphics_context_set_text_color(ctx, (GColor)title_color);
  graphics_draw_text(ctx, title, fonts_get_system_font(TITLE_FONT_KEY), GRect(bounds.origin.x, bounds.origin.y + TITLE_OFFSET, bounds.size.w, TITLE_HEIGHT), GTextOverflowModeFill, GTextAlignmentCenter, NULL);
  
  for(int i = 0; i < DAYS_PER_WEEK; ++i){
    uint8_t color = (i == SUNDAY_INDEX || i == SATUTDAY_INDEX ? sat_sun_color : week_day_color);
    graphics_context_set_text_color(ctx, (GColor)color);
    graphics_draw_text(ctx, WEEK_LABELS[i], fonts_get_system_font(WEEK_LABEL_FONT_KEY), GRect(DAY_OFFSET_X + DAY_WIDTH * i, WEEK_LABELS_OFFSET_Y, DAY_WIDTH, WEEK_LABELS_HEGHT), GTextOverflowModeFill, GTextAlignmentCenter, NULL);
  }
  
  // draw days
  int current_row = 0;
  start_drawing_month(&current_begin_month);
  do{
    // draw current day circle
    if(day_to_draw.tm_mon == current_time.tm_mon && day_to_draw.tm_mday == current_time.tm_mday){
      graphics_context_set_fill_color(ctx, (GColor)current_day_color);
      GPoint location = GPoint(DAY_OFFSET_X + current_time.tm_wday * DAY_WIDTH + DAY_WIDTH / 2, DAY_OFFSET_Y + current_row * DAY_HEIGHT + DAY_HEIGHT / 2);
      graphics_fill_circle(ctx, location, CURRENT_DAY_RADIUS);
      graphics_context_set_text_color(ctx, (GColor)current_day_text_color);
    }
    else if(day_to_draw.tm_mon != current_begin_month.tm_mon){
      graphics_context_set_text_color(ctx, (GColor)other_month_day_color);
    }
    else{
      graphics_context_set_text_color(ctx, (GColor)day_color);
    }
    
    char day[MAX_DAY_LEN] = {'\0'};
    snprintf(day, MAX_DAY_LEN, "%d", day_to_draw.tm_mday);
    GRect location = GRect(DAY_OFFSET_X + day_to_draw.tm_wday * DAY_WIDTH, DAY_OFFSET_Y + current_row * DAY_HEIGHT, DAY_WIDTH, DAY_HEIGHT);
    graphics_draw_text(ctx, day, fonts_get_system_font(DAY_FONT_KEY), location, GTextOverflowModeFill, GTextAlignmentCenter, NULL);
    
    if(day_to_draw.tm_wday == SATUTDAY_INDEX) ++current_row;
    next_day_of_month();
  }while(current_row <= MAX_ROW_INDEX);
}
  
static void main_window_load(Window * window) {
  Layer * window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
  main_layer = layer_create(bounds);
  layer_set_update_proc(main_layer, update_proc);
  layer_add_child(window_layer, main_layer);
}

static void main_window_unload(Window * window) {
  layer_destroy(main_layer);
}

static void init_time() {
  struct tm * tmp_time;
  
  time_t now = time(NULL);
  tmp_time = localtime(&now);
  memcpy(&current_time, tmp_time, sizeof(struct tm));
  memcpy(&current_begin_month, &current_time, sizeof(struct tm));
  
  current_begin_month.tm_sec = 0;
  current_begin_month.tm_min = 0;
  current_begin_month.tm_hour = 0;
  current_begin_month.tm_mday = 1;
  
  time_t tmp_begin_month = mktime(&current_begin_month);
  tmp_time = localtime(&tmp_begin_month);
  memcpy(&current_begin_month, tmp_time, sizeof(struct tm));
}

static void init() {
  main_window = window_create();
  
  window_set_window_handlers(main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload
  });

  window_stack_push(main_window, true);
  
  init_time();
}

static void deinit() {
  window_destroy(main_window);
}
  
int main(void) {
  init();
  app_event_loop();
  deinit();
}