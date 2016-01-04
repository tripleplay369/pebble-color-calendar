#include <pebble.h>
  
#define DAYS_PER_WEEK 7
#define TM_BEGIN_YEAR 1900
#define TITLE_MAX_LEN 15
#define SUNDAY_INDEX 0
#define SATURDAY_INDEX 6
#define MAX_DAY_LEN 3
#define MAX_ROW_INDEX 5
#define IS_EURO_KEY 0
  
static const char * MONTH_NAMES[] = {
  "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};

static const char * WEEK_LABELS[] = {
  "S", "M", "T", "W", "T", "F", "S"
};
  
static Window * main_window;
static Layer * main_layer;
static PropertyAnimation * property_animation;

static struct tm current_time;
static struct tm current_begin_month;
static struct tm next_begin_month;
static int next_direction = 0;
static int current_offset = 0;
static struct tm day_to_draw;

static const int TITLE_HEIGHT = 30;
static const int WEEK_LABELS_HEGHT = 10;
static const int DAY_WIDTH = 20;
static const int DAY_OFFSET_X = 3;
static const int CURRENT_DAY_RADIUS = 10;
static const int HEADER_CORNER_RADIUS = 5;

#if defined(PBL_RECT)
static const int START_X = 0;
static const int HEADER_HEIGHT = 45;
static const int WEEK_LABELS_OFFSET_Y = 26;
static const int TITLE_OFFSET = -2;
static const int DAY_OFFSET_Y = 43;
static const int DAY_HEIGHT = 20;
static const int DAY_CIRCLE_OFFSET = 2;
#elif defined(PBL_ROUND)
static const int START_X = 18;
static const int HEADER_HEIGHT = 45;
static const int WEEK_LABELS_OFFSET_Y = 26;
static const int TITLE_OFFSET = 0;
static const int DAY_OFFSET_Y = 43;
static const int DAY_HEIGHT = 18;
static const int DAY_CIRCLE_OFFSET = 3;
#endif

static const char * TITLE_FONT_KEY = FONT_KEY_GOTHIC_24_BOLD;
static const char * WEEK_LABEL_FONT_KEY = FONT_KEY_GOTHIC_14;
static const char * DAY_FONT_KEY = FONT_KEY_GOTHIC_18;
static uint8_t header_color = GColorLightGrayARGB8;
static uint8_t title_color = GColorRedARGB8;
static uint8_t sat_sun_color = GColorDarkGrayARGB8;
static uint8_t week_day_color = GColorBlackARGB8;
static uint8_t day_color = GColorBlackARGB8;
static uint8_t current_day_color = GColorVeryLightBlueARGB8;
static uint8_t current_day_text_color = GColorWhiteARGB8;
static uint8_t other_month_day_color = GColorLightGrayARGB8;

static uint8_t is_euro = 0;

static void start_drawing_month(struct tm * begin_month){
  memcpy(&day_to_draw, begin_month, sizeof(struct tm));
  
  while(day_to_draw.tm_wday != is_euro){
    day_to_draw.tm_mday--;
    mktime(&day_to_draw);
  }
}

static void set_next_month(int direction){
  current_offset += direction;
  next_direction = direction;
  memcpy(&next_begin_month, &current_begin_month, sizeof(struct tm));
  next_begin_month.tm_mon += direction;
  mktime(&next_begin_month);
}

static void next_day_of_month(){
  day_to_draw.tm_mday++;  
  mktime(&day_to_draw);
}

static void draw_month(Layer * layer, GContext * ctx, int offset_y, struct tm current_tm){
  GRect bounds = layer_get_bounds(layer);
    
  char title[TITLE_MAX_LEN] = {'\0'};
  snprintf(title, TITLE_MAX_LEN, "%s %d", MONTH_NAMES[current_tm.tm_mon], current_tm.tm_year + TM_BEGIN_YEAR);
  
  // draw header
  graphics_context_set_fill_color(ctx, (GColor)header_color);
  graphics_fill_rect(ctx, GRect(bounds.origin.x, offset_y + bounds.origin.y, bounds.size.w, HEADER_HEIGHT), HEADER_CORNER_RADIUS, GCornersAll);
  graphics_context_set_text_color(ctx, (GColor)title_color);
  graphics_draw_text(ctx, title, fonts_get_system_font(TITLE_FONT_KEY), GRect(bounds.origin.x, offset_y + bounds.origin.y + TITLE_OFFSET, bounds.size.w, TITLE_HEIGHT), GTextOverflowModeFill, GTextAlignmentCenter, NULL);
  
  for(int i = 0; i < DAYS_PER_WEEK; ++i){
    int ii = (i + is_euro) % DAYS_PER_WEEK;
    uint8_t color = (ii == SUNDAY_INDEX || ii == SATURDAY_INDEX ? sat_sun_color : week_day_color);
    graphics_context_set_text_color(ctx, (GColor)color);
    graphics_draw_text(ctx, WEEK_LABELS[ii], fonts_get_system_font(WEEK_LABEL_FONT_KEY), GRect(START_X + DAY_OFFSET_X + DAY_WIDTH * i, offset_y + WEEK_LABELS_OFFSET_Y, DAY_WIDTH, WEEK_LABELS_HEGHT), GTextOverflowModeFill, GTextAlignmentCenter, NULL);
  }
  
  // draw days
  int current_row = 0;
  start_drawing_month(&current_tm);
  do{
    int wday_index = day_to_draw.tm_wday - is_euro;
    if(wday_index < 0) wday_index = SATURDAY_INDEX;
    
    // draw current day circle
    if(day_to_draw.tm_mon == current_time.tm_mon && day_to_draw.tm_mday == current_time.tm_mday && day_to_draw.tm_year == current_time.tm_year){
      graphics_context_set_fill_color(ctx, (GColor)current_day_color);
      GPoint location = GPoint(START_X + DAY_OFFSET_X + wday_index * DAY_WIDTH + DAY_WIDTH / 2, offset_y + DAY_OFFSET_Y + current_row * DAY_HEIGHT + DAY_HEIGHT / 2 + DAY_CIRCLE_OFFSET);
      graphics_fill_circle(ctx, location, CURRENT_DAY_RADIUS);
      graphics_context_set_text_color(ctx, (GColor)current_day_text_color);
    }
    else if(day_to_draw.tm_mon != current_tm.tm_mon){
      graphics_context_set_text_color(ctx, (GColor)other_month_day_color);
    }
    else{
      graphics_context_set_text_color(ctx, (GColor)day_color);
    }
    
    char day[MAX_DAY_LEN] = {'\0'};
    snprintf(day, MAX_DAY_LEN, "%d", day_to_draw.tm_mday);
    GRect location = GRect(START_X + DAY_OFFSET_X + wday_index * DAY_WIDTH, offset_y + DAY_OFFSET_Y + current_row * DAY_HEIGHT, DAY_WIDTH, DAY_HEIGHT);
    graphics_draw_text(ctx, day, fonts_get_system_font(DAY_FONT_KEY), location, GTextOverflowModeFill, GTextAlignmentCenter, NULL);
    
    if(wday_index == SATURDAY_INDEX) ++current_row;
    next_day_of_month();
  }while(current_row <= MAX_ROW_INDEX);
}

static void update_proc(Layer * layer, GContext * ctx){
  Layer * window_layer = window_get_root_layer(main_window);
  GRect bounds = layer_get_bounds(window_layer);
  
  if(next_direction == 0){
    draw_month(layer, ctx, 0, current_begin_month);
  }
  else if(next_direction > 0){
    draw_month(layer, ctx, 0, current_begin_month);
    draw_month(layer, ctx, bounds.size.h, next_begin_month);
  }
  else if(next_direction < 0){
    draw_month(layer, ctx, 0, next_begin_month);
    draw_month(layer, ctx, bounds.size.h, current_begin_month);
  }
}
  
static void main_window_load(Window * window){
  Layer * window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
  main_layer = layer_create(bounds);
  layer_set_update_proc(main_layer, update_proc);
  layer_add_child(window_layer, main_layer);
}

static void main_window_unload(Window * window){
  layer_destroy(main_layer);
}

static void init_time(){
  if(persist_exists(IS_EURO_KEY)){
    is_euro = persist_read_int(IS_EURO_KEY);
  }
  else{
    persist_write_int(IS_EURO_KEY, is_euro);
  }
  
  struct tm * tmp_time;
  
  time_t now = time(NULL);
  tmp_time = localtime(&now);
  memcpy(&current_time, tmp_time, sizeof(struct tm));
  memcpy(&current_begin_month, &current_time, sizeof(struct tm));

  current_begin_month.tm_mday = 1;
  
  mktime(&current_begin_month);
}

void anim_stopped_handler(Animation *animation, bool finished, void *context){
  property_animation_destroy(property_animation);
  property_animation = NULL;
  
  next_direction = 0;
  memcpy(&current_begin_month, &next_begin_month, sizeof(struct tm));
  
  Layer * window_layer = window_get_root_layer(main_window);
  GRect frame = layer_get_frame(window_layer);
  layer_set_frame(main_layer, frame);
}

void animate(GRect start_frame, GRect end_frame){
  property_animation = property_animation_create_layer_frame(main_layer, &start_frame, &end_frame);
  animation_set_handlers((Animation *)property_animation, (AnimationHandlers){
    .stopped = anim_stopped_handler
  }, NULL);
  animation_schedule((Animation *)property_animation);
}

void down_single_click_handler(ClickRecognizerRef recognizer, void * context){
  if(animation_is_scheduled((Animation *)property_animation)) return;
  
  Layer * window_layer = window_get_root_layer(main_window);
  GRect bounds = layer_get_bounds(window_layer);
  
  GRect start_frame = GRect(0, 0, bounds.size.w, bounds.size.h * 2);
  GRect end_frame = GRect(0, -1 * bounds.size.h, bounds.size.w, bounds.size.h * 2);
  
  set_next_month(1);
  animate(start_frame, end_frame);
}

void up_single_click_handler(ClickRecognizerRef recognizer, void * context){
  if(animation_is_scheduled((Animation *)property_animation)) return;
  
  Layer * window_layer = window_get_root_layer(main_window);
  GRect bounds = layer_get_bounds(window_layer);
  
  GRect start_frame = GRect(0, -1 * bounds.size.h, bounds.size.w, bounds.size.h * 2);
  GRect end_frame = GRect(0, 0, bounds.size.w, bounds.size.h * 2);
  
  set_next_month(-1);
  animate(start_frame, end_frame);
}

void select_single_click_handler(ClickRecognizerRef recognizer, void * context){
  if(animation_is_scheduled((Animation *)property_animation)) return;
  if(current_offset == 0) return;
  
  Layer * window_layer = window_get_root_layer(main_window);
  GRect bounds = layer_get_bounds(window_layer);
  
  GRect start_frame, end_frame;
  if(current_offset > 0){
    start_frame = GRect(0, -1 * bounds.size.h, bounds.size.w, bounds.size.h * 2);
    end_frame = GRect(0, 0, bounds.size.w, bounds.size.h * 2);
  }
  else{
    start_frame = GRect(0, 0, bounds.size.w, bounds.size.h * 2);
    end_frame = GRect(0, -1 * bounds.size.h, bounds.size.w, bounds.size.h * 2);
  }
  
  set_next_month(-1 * current_offset);
  current_offset = 0;
  animate(start_frame, end_frame);
}

void select_long_click_handler(ClickRecognizerRef recognizer, void * context){
  is_euro = !is_euro;
  persist_write_int(IS_EURO_KEY, is_euro);
  layer_mark_dirty(main_layer);
}

void config_provider(Window * window){
  window_single_click_subscribe(BUTTON_ID_DOWN, down_single_click_handler);
  window_single_click_subscribe(BUTTON_ID_UP, up_single_click_handler);
  window_single_click_subscribe(BUTTON_ID_SELECT, select_single_click_handler);
  window_long_click_subscribe(BUTTON_ID_SELECT, 2000, select_long_click_handler, NULL);
}

static void init(){
  main_window = window_create();
  
  window_set_window_handlers(main_window, (WindowHandlers){
    .load = main_window_load,
    .unload = main_window_unload
  });

  window_stack_push(main_window, true);
  
  window_set_click_config_provider(main_window, (ClickConfigProvider)config_provider);
  
  init_time();
}

static void deinit(){
  window_destroy(main_window);
}
  
int main(void){
  init();
  app_event_loop();
  deinit();
}