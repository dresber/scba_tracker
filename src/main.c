//**********************************************************************************//
//                          PEBBLE SCBA TRACKER                                     
//                                                                                  
//  Version:  1.0                                                           
//  Author:   Bernhard D.                                                   
//  Date:     2014-12-07                                                    
//                                                                          
//  DESCRIPTION: 
//
//  The programm is designed to track up to three SCBA teams on a fire      
//  scene. The tracker can choose from 4 different SCBA bottle types.       
//  Depending on the selected bottle the pebble will give alarm at each     
//  third of the overall working time (default_pressure - min_pressure).    
//  The pressure/air volume is reduced for 50 l/min which might simulate    
//  a team under mid to heavy work. The pressure/air volume can be adjusted 
//  from the user. The end time will be updated after an pressure update.   
//                                                                          
//  WARRANTY:
//
//  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
//  ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
//  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
//  DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
//  DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
//  INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
//  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
//  ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
//  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
//  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
//  PLEASE NOTE THAT I DO NOT GIVE ANY WARRANTY ON THE TRACKED TIME, AS     
//  WELL AS THE ALARMS AND THE PRESSURE INDICATION. THE APP IS JUST FOR     
//  ASSISTANCE. THE USER AND THE TEAMS ARE COMPLETELY SELF RESPONSIBLE.     
//                                                                          
//  SOURCES:                                                                
//
//  The Minimal snprintf() implementation from Michal Ludvig                
//                                                                          
//**********************************************************************************//

//  ----------- include paths ----------  //
//                                        //
//  ------------------------------------  //
#include "main.h"

//  -------- function prototypes -------  //
//                                        //
//  ------------------------------------  //
void window_load(Window *window);
void window_unload(Window *window);
void tick_handler(struct tm *tick_time, TimeUnits units_changed);
void start_scba_layer(Layer *layer, uint8_t team);
void click_down(void);
void click_up(void);
void click_select(void);
void click_handler(uint8_t key, uint8_t factor);
void switch_scba_selection(uint8_t key);
void change_scba_nr(uint8_t key);
void change_bottle_type(uint8_t key);
void change_bottle_pressure(uint8_t key, uint8_t factor);
void change_active_scba_icon(void);
void set_text_layer_font(TextLayer *layer, GColor background_color, GColor text_color, GTextAlignment text_alignment, const char* font);
void update_scba_team_info(uint8_t team_nr);
bool update_scba_team_info_screen(uint8_t team_nr);
void reduce_scba_team_air_volume(uint8_t team_nr);
void calc_scba_team_air_pressure(uint8_t team_nr);
void update_scba_team_end_time(uint8_t team_nr);
void calc_scba_team_air_volume(uint8_t team_nr);
void long_click_select(void);
void stop_scba_monitoring(uint8_t key);
void multi_click_up(void);
void multi_click_down(void);

//* -------- global variables ---------- *//
//                                        //
//* ------------------------------------ *//
Window *g_window;

TextLayer *g_header_layer;
TextLayer *g_clock_layer;

Layer *g_scba_one_layer;
Layer *g_scba_two_layer;
Layer *g_scba_three_layer;

GBitmap *icon_up;
GBitmap *icon_down;
GBitmap *icon_ok;
GBitmap *icon_active_scba;
GBitmap *icon_full_bottle;
GBitmap *icon_scba_firefighter;
GBitmap *icon_small_firefighter;
GBitmap *icon_small_full_bottle;
GBitmap *icon_small_third_full_bottle;
GBitmap *icon_small_half_full_bottle;
GBitmap *icon_small_third_empty_bottle;
GBitmap *icon_small_empty_bottle;
GBitmap *icon_small_stop_signe;
GBitmap *icon_small_exclamation_mark;

ActionBarLayer *g_action_bar;

scba_layer_t scba_one_layers;

scba_bottle_t  available_bottles[4] = {
  {90,   300,   80,   (char*)("9l")},
  {68,   300,   60,   (char*)("6,8l")}, 
  {80,   200,   80,   (char*)("2x4l")},
  {136,  300,   120,  (char*)("2x6,8l")} 
};

uint32_t scba_team_storage_keys[SCBA_TEAMS] = {
  SCBA_STORE_KEY_TEAM_ONE,
  SCBA_STORE_KEY_TEAM_TWO,
  SCBA_STORE_KEY_TEAM_THREE
};

//* ----------- functions -------------- *//
//                                        //
//* ------------------------------------ *//

/**
*
*/
void click_config_provider(void *context) 
{
  window_single_click_subscribe(BUTTON_ID_DOWN, (ClickHandler) click_down);
  window_single_click_subscribe(BUTTON_ID_UP, (ClickHandler) click_up);
  window_single_click_subscribe(BUTTON_ID_SELECT, (ClickHandler) click_select);
  window_long_click_subscribe(BUTTON_ID_SELECT, 1000, (ClickHandler)long_click_select, (ClickHandler)long_click_select);
  window_multi_click_subscribe(BUTTON_ID_UP, 2, 20, 300, false, (ClickHandler)multi_click_up);
  window_multi_click_subscribe(BUTTON_ID_DOWN, 2, 20, 300, false, (ClickHandler)multi_click_down);
}

/**
*
*/
void handle_init(void) 
{
  g_window = window_create();
  
  window_set_window_handlers(g_window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload
  });
  
  window_set_fullscreen(g_window, true);
  tick_timer_service_subscribe(SECOND_UNIT, (TickHandler)tick_handler);
  window_stack_push(g_window, true);
}

/**
*
*/
void window_load(Window *window)
{  
  screen_status = SCBA_START_SCREEN;
  active_scba = 0;
  
  g_header_layer = text_layer_create(GRect(0,0,60,30));
  text_layer_set_background_color(g_header_layer, GColorClear);
  text_layer_set_text_color(g_header_layer, GColorBlack);
  text_layer_set_text_alignment(g_header_layer, GTextAlignmentCenter);
  text_layer_set_text(g_header_layer, "SCBA Tracker");

  g_clock_layer = text_layer_create(GRect(60,0,60,30));
  set_text_layer_font(g_clock_layer, GColorClear, GColorBlack, GTextAlignmentCenter, FONT_KEY_GOTHIC_18_BOLD);
  
  g_scba_one_layer = layer_create(GRect(0,30,120,43));
  g_scba_two_layer = layer_create(GRect(0,74,120,43));
  g_scba_three_layer = layer_create(GRect(0,117,120,43));

  icon_active_scba = gbitmap_create_with_resource(RESOURCE_ID_ACTIVE_SCBA);
  icon_full_bottle = gbitmap_create_with_resource(RESOURCE_ID_FULL_BOTTLE);
  icon_scba_firefighter = gbitmap_create_with_resource(RESOURCE_ID_SCBA_FIREFIGHTER);
  icon_small_firefighter = gbitmap_create_with_resource(RESOURCE_ID_SMALL_SCBA_FIREFIGHTER);
  icon_small_full_bottle = gbitmap_create_with_resource(RESOURCE_ID_SMALL_FULL_BOTTLE);
  icon_small_third_full_bottle = gbitmap_create_with_resource(RESOURCE_ID_SMALL_THIRD_FULL_BOTTLE);
  icon_small_half_full_bottle = gbitmap_create_with_resource(RESOURCE_ID_SMALL_HALF_FULL_BOTTLE);
  icon_small_third_empty_bottle = gbitmap_create_with_resource(RESOURCE_ID_SMALL_THIRD_EMPTY_BOTTLE);
  icon_small_empty_bottle = gbitmap_create_with_resource(RESOURCE_ID_SMALL_EMPTY_BOTTLE);
  icon_small_stop_signe = gbitmap_create_with_resource(RESOURCE_ID_SMALL_STOP_SIGNE);
  icon_small_exclamation_mark = gbitmap_create_with_resource(RESOURCE_ID_SMALL_EXCLAMATION_MARK);
  
  start_scba_layer(g_scba_one_layer, 0);
  start_scba_layer(g_scba_two_layer, 1);
  start_scba_layer(g_scba_three_layer, 2);
  
  g_action_bar = action_bar_layer_create();
  action_bar_layer_set_background_color(g_action_bar, GColorClear);
  action_bar_layer_add_to_window(g_action_bar, window);
  action_bar_layer_set_click_config_provider(g_action_bar, click_config_provider);

  icon_up = gbitmap_create_with_resource(RESOURCE_ID_ARROW_UP);
  icon_down = gbitmap_create_with_resource(RESOURCE_ID_ARROW_DOWN);
  icon_ok = gbitmap_create_with_resource(RESOURCE_ID_OK_BTN);

  action_bar_layer_set_icon(g_action_bar, BUTTON_ID_UP, icon_up);
  action_bar_layer_set_icon(g_action_bar, BUTTON_ID_DOWN, icon_down);
  action_bar_layer_set_icon(g_action_bar, BUTTON_ID_SELECT, icon_ok);
  
  layer_add_child(window_get_root_layer(window), g_scba_one_layer);
  layer_add_child(window_get_root_layer(window), g_scba_two_layer);
  layer_add_child(window_get_root_layer(window), g_scba_three_layer);
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(g_header_layer));
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(g_clock_layer));
  
  change_active_scba_icon();
}

/**
*
*/
void start_scba_layer(Layer *layer, uint8_t team)
{  
  bool data_loaded = false;
  // Set default values for the SCBA team
  if(persist_exists(scba_team_storage_keys[team]))
  {
    persist_read_data(scba_team_storage_keys[team], &scba_team_data[team], sizeof(scba_team_data[team]));
    data_loaded = true;
  }
  else
  {
    scba_team_data[team].scba_team_nr = team+1;
    scba_team_data[team].scba_team_bottle_pressure = SCBA_DATA_DEFAULT_PRESSURE;  
    scba_team_data[team].scba_team_bottle_type = SCBA_DATA_DEFAULT_BOTTLE_TYPE;
    scba_team_data[team].scba_team_status = SCBA_NOT_STARTED;
  }
  // Add objects for SCBA start screen
  scba_layer[team].start_layer = text_layer_create(GRect(10,0,110,40));
  set_text_layer_font(scba_layer[team].start_layer, GColorClear, GColorBlack, GTextAlignmentCenter, FONT_KEY_GOTHIC_14);
  text_layer_set_text(scba_layer[team].start_layer, "Start SCBA");
  // Add object for active SCBA indicator
  scba_layer[team].active_layer = bitmap_layer_create(GRect(0,0,10,40));
  bitmap_layer_set_bitmap(scba_layer[team].active_layer, icon_active_scba);
  // Add general objects for the configuration input
  scba_layer[team].scba_cnfg_layer = layer_create(GRect(10,0,110,40));  
  scba_layer[team].cnfg_text_layer = text_layer_create(GRect(30,0,40,40));
  scba_layer[team].cnfg_text_layer_input = text_layer_create(GRect(70,0,40,40));
  scba_layer[team].cnfg_bitmap_layer = bitmap_layer_create(GRect(0,0,30,40));
  set_text_layer_font(scba_layer[team].cnfg_text_layer, GColorClear, GColorBlack, GTextAlignmentLeft, FONT_KEY_GOTHIC_14);
  set_text_layer_font(scba_layer[team].cnfg_text_layer_input, GColorClear, GColorBlack, GTextAlignmentLeft, FONT_KEY_GOTHIC_18_BOLD);
  text_layer_set_text(scba_layer[team].cnfg_text_layer, "TEAM\nNr.:");
  bitmap_layer_set_bitmap(scba_layer[team].cnfg_bitmap_layer, icon_scba_firefighter);
  layer_add_child(scba_layer[team].scba_cnfg_layer, bitmap_layer_get_layer(scba_layer[team].cnfg_bitmap_layer));
  layer_add_child(scba_layer[team].scba_cnfg_layer, text_layer_get_layer(scba_layer[team].cnfg_text_layer_input));
  layer_add_child(scba_layer[team].scba_cnfg_layer, text_layer_get_layer(scba_layer[team].cnfg_text_layer));
  layer_set_hidden((Layer *)scba_layer[team].scba_cnfg_layer, true);
  // Add info layer for showing the actual SCBA team information
  scba_layer[team].scba_info_layer = layer_create(GRect(10,0,110,43));
  scba_layer[team].scba_info_team_layer = bitmap_layer_create(GRect(0,0,20,19));  
  scba_layer[team].scba_team_nr = text_layer_create(GRect(0,19,20,24));
  scba_layer[team].scba_start_time_text = text_layer_create(GRect(25,0,15,14));
  scba_layer[team].scba_end_time_text = text_layer_create(GRect(25,14,15,14));
  scba_layer[team].scba_passed_time_text = text_layer_create(GRect(25,28,15,14));
  scba_layer[team].scba_start_time = text_layer_create(GRect(40,0,50,14));
  scba_layer[team].scba_end_time = text_layer_create(GRect(40,14,50,14));
  scba_layer[team].scba_passed_time = text_layer_create(GRect(40,28,50,14));
  scba_layer[team].scba_bottle_pressure = text_layer_create(GRect(90,0,20,14));
  scba_layer[team].scba_bottle_layer = bitmap_layer_create(GRect(90,14,20,30));

  set_text_layer_font(scba_layer[team].scba_team_nr, GColorClear, GColorBlack, GTextAlignmentCenter, FONT_KEY_GOTHIC_24_BOLD);
  set_text_layer_font(scba_layer[team].scba_start_time_text, GColorClear, GColorBlack, GTextAlignmentLeft, FONT_KEY_GOTHIC_14);
  set_text_layer_font(scba_layer[team].scba_end_time_text, GColorClear, GColorBlack, GTextAlignmentLeft, FONT_KEY_GOTHIC_14);
  set_text_layer_font(scba_layer[team].scba_passed_time_text, GColorClear, GColorBlack, GTextAlignmentLeft, FONT_KEY_GOTHIC_14);
  set_text_layer_font(scba_layer[team].scba_start_time, GColorClear, GColorBlack, GTextAlignmentCenter, FONT_KEY_GOTHIC_14);
  set_text_layer_font(scba_layer[team].scba_end_time, GColorClear, GColorBlack, GTextAlignmentCenter, FONT_KEY_GOTHIC_14);
  set_text_layer_font(scba_layer[team].scba_passed_time, GColorClear, GColorBlack, GTextAlignmentCenter, FONT_KEY_GOTHIC_14);
  set_text_layer_font(scba_layer[team].scba_bottle_pressure, GColorClear, GColorBlack, GTextAlignmentCenter, FONT_KEY_GOTHIC_14_BOLD);

  text_layer_set_text(scba_layer[team].scba_start_time_text, "S:");
  text_layer_set_text(scba_layer[team].scba_end_time_text, "E:");
  text_layer_set_text(scba_layer[team].scba_passed_time_text, "T:");
  text_layer_set_text(scba_layer[team].scba_bottle_pressure , scba_layer[team].text_pressure);
  text_layer_set_text(scba_layer[team].scba_team_nr , scba_layer[team].text_team_nr);
  text_layer_set_text(scba_layer[team].scba_start_time , scba_layer[team].text_start_time);
  text_layer_set_text(scba_layer[team].scba_end_time , scba_layer[team].text_stop_time);
  text_layer_set_text(scba_layer[team].scba_passed_time , scba_layer[team].text_passed_time);

  bitmap_layer_set_bitmap(scba_layer[team].scba_info_team_layer, icon_small_firefighter);
  bitmap_layer_set_bitmap(scba_layer[team].scba_bottle_layer, icon_small_full_bottle);
  
  layer_add_child(scba_layer[team].scba_info_layer, bitmap_layer_get_layer(scba_layer[team].scba_info_team_layer));
  layer_add_child(scba_layer[team].scba_info_layer, text_layer_get_layer(scba_layer[team].scba_team_nr));
  layer_add_child(scba_layer[team].scba_info_layer, text_layer_get_layer(scba_layer[team].scba_start_time_text));
  layer_add_child(scba_layer[team].scba_info_layer, text_layer_get_layer(scba_layer[team].scba_end_time_text));
  layer_add_child(scba_layer[team].scba_info_layer, text_layer_get_layer(scba_layer[team].scba_passed_time_text));
  layer_add_child(scba_layer[team].scba_info_layer, text_layer_get_layer(scba_layer[team].scba_start_time));
  layer_add_child(scba_layer[team].scba_info_layer, text_layer_get_layer(scba_layer[team].scba_end_time));
  layer_add_child(scba_layer[team].scba_info_layer, text_layer_get_layer(scba_layer[team].scba_passed_time));
  layer_add_child(scba_layer[team].scba_info_layer, text_layer_get_layer(scba_layer[team].scba_bottle_pressure));
  layer_add_child(scba_layer[team].scba_info_layer, bitmap_layer_get_layer(scba_layer[team].scba_bottle_layer));

  layer_set_hidden((Layer *)scba_layer[team].scba_info_layer, true);  
  // Add all childs to the main layer
  layer_add_child(layer, text_layer_get_layer(scba_layer[team].start_layer));
  layer_add_child(layer, bitmap_layer_get_layer(scba_layer[team].active_layer));
  layer_add_child(layer, scba_layer[team].scba_info_layer);
  layer_add_child(layer, scba_layer[team].scba_cnfg_layer);
  
  if(data_loaded == true)
  {
      layer_set_hidden((Layer *)scba_layer[team].start_layer, true);
      layer_set_hidden((Layer *)scba_layer[team].scba_info_layer, false);  
      update_scba_team_end_time(team);
      screen_status = SCBA_INFO_SCREEN;
  } 
}

/**
*
*/
void window_unload(Window *window)
{
  text_layer_destroy(g_header_layer);
  text_layer_destroy(g_clock_layer);
}

/**
*
*/
void handle_deinit(void) 
{
  window_destroy(g_window);
}

/**
*
*/
void tick_handler(struct tm *tick_time, TimeUnits units_changed)
{
  static char buffer[] = "00:00";
  static uint8_t cnt[3] = {0,0,0};
  bool least_one_alarm_active = false;
  bool temp_alarm = false;
  uint8_t i=0;
  
  strftime(buffer, sizeof("00:00"), "%H:%M", tick_time);
  
  text_layer_set_text(g_clock_layer, buffer);
  
  for(i=0; i< SCBA_TEAMS; i++)
  {
    if (scba_team_data[i].scba_team_status != SCBA_NOT_STARTED)  
    {
      if ((cnt[i] >= 30) && (screen_status != SCBA_UPDATE_PRESSURE))
      {
        reduce_scba_team_air_volume(i);
        update_scba_team_end_time(i);
        calc_scba_team_air_pressure(i);
        persist_write_data(scba_team_storage_keys[i], &scba_team_data[i], sizeof(scba_team_data[i]));
        cnt[i] = 0;
      }
      else
      {
        cnt[i]++;
      }
      
      temp_alarm = update_scba_team_info_screen(i);
      
      if (temp_alarm == true)
      {
        least_one_alarm_active = true;  
      }
    }
  }
  
  if (least_one_alarm_active == true)
  {
    light_enable_interaction();  
  }
}

/**
*
*/
void click_down(void)
{
  click_handler(CLICK_DOWN, ORDINARY_CLICK);
}

/**
*
*/
void click_up(void)
{
  click_handler(CLICK_UP, ORDINARY_CLICK);
}

/**
*
*/
void click_select(void)
{
  click_handler(CLICK_SELECT, ORDINARY_CLICK);  
}

/**
*
*/
void long_click_select(void)
{
  layer_set_hidden((Layer *)scba_layer[active_scba].start_layer, false);
  layer_set_hidden((Layer *)scba_layer[active_scba].scba_info_layer, true);  
  text_layer_set_text(scba_layer[active_scba].start_layer, "Stop SCBA monitoring?");
  screen_status = SCBA_STOP_MONITORING;
}

/**
*
*/
void multi_click_up(void)
{
  click_handler(CLICK_UP, MULTI_CLICK);
}

/**
*
*/
void multi_click_down(void)
{
  click_handler(CLICK_DOWN, MULTI_CLICK);
}

/**
*
*/
void click_handler(uint8_t key, uint8_t factor)
{
  switch(screen_status)
  {    
    case SCBA_CNFG_SCREEN_NR:
      change_scba_nr(key);
      break;

    case SCBA_CNFG_SCREEN_BOTTLE_TYPE:
      change_bottle_type(key);
      break;

    case SCBA_CNFG_SCREEN_BOTTLE_PRESSURE:
      change_bottle_pressure(key, factor);
      break;

    case SCBA_START_SCREEN:
    case SCBA_INFO_SCREEN:
      switch_scba_selection(key);
      break;
    
    case SCBA_UPDATE_PRESSURE:
      change_bottle_pressure(key, factor);
      break;
    
    case SCBA_STOP_MONITORING:
      stop_scba_monitoring(key);
      break;
    
    default:
      break;
  }
}

/**
*
*/
void switch_scba_selection(uint8_t key)
{
  switch(key)
  {
    case CLICK_UP:
      if(active_scba == 0)
      {
        active_scba = 2;
      }
      else
      {
        active_scba -= 1;
      }
      break;

    case CLICK_DOWN:
      if(active_scba == 2)
      {
        active_scba = 0;
      }
      else
      {
        active_scba += 1;
      }
      break;
    
    case CLICK_SELECT:
      if ((scba_team_data[active_scba].scba_team_status == SCBA_THIRD_FULL_BOTTLE_ALARM) || (scba_team_data[active_scba].scba_team_status == SCBA_HALF_FULL_BOTTLE_ALARM) ||
         (scba_team_data[active_scba].scba_team_status == SCBA_THIRD_EMPTY_BOTTLE_ALARM) || (scba_team_data[active_scba].scba_team_status == SCBA_EMPTY_BOTTLE_ALARM))
      {
          text_layer_set_text_color(scba_layer[active_scba].scba_team_nr, GColorBlack);
          text_layer_set_background_color(scba_layer[active_scba].scba_team_nr, GColorClear);
          scba_team_data[active_scba].scba_team_status ++;
      }
      else if (scba_team_data[active_scba].scba_team_status == SCBA_NOT_STARTED)
      {
        layer_set_hidden((Layer *)scba_layer[active_scba].start_layer, true);
        layer_set_hidden((Layer *)scba_layer[active_scba].scba_info_layer, true);  
        layer_set_hidden((Layer *)scba_layer[active_scba].scba_cnfg_layer, false);
        mini_snprintf(text_buffer, sizeof(text_buffer), "%d", scba_team_data[active_scba].scba_team_nr);
        text_layer_set_text(scba_layer[active_scba].cnfg_text_layer_input, text_buffer);
        screen_status = SCBA_CNFG_SCREEN_NR;
      }
      else
      {
        text_layer_set_text_color(scba_layer[active_scba].scba_bottle_pressure, GColorClear);
        text_layer_set_background_color(scba_layer[active_scba].scba_bottle_pressure, GColorBlack);      
        screen_status = SCBA_UPDATE_PRESSURE;  
      }
      break;
    
    default:
      break;
  }
  change_active_scba_icon();
}

/**
*
*/
void change_scba_nr(uint8_t key)
{
  switch(key)
  {
    case CLICK_UP:
      if(scba_team_data[active_scba].scba_team_nr > 9)
      {
        scba_team_data[active_scba].scba_team_nr = 1;
      }
      else
      {
        scba_team_data[active_scba].scba_team_nr += 1; 
      }
      break;
    
    case CLICK_DOWN:
      if(scba_team_data[active_scba].scba_team_nr < 2)
      {
        scba_team_data[active_scba].scba_team_nr = 10;        
      }  
      else
      {
        scba_team_data[active_scba].scba_team_nr -= 1;
      }
      break;
    
    case CLICK_SELECT:
      text_layer_set_text(scba_layer[active_scba].cnfg_text_layer, "Size:");
      text_layer_set_text(scba_layer[active_scba].cnfg_text_layer_input, available_bottles[scba_team_data[active_scba].scba_team_bottle_type].bottle_name);
      bitmap_layer_set_bitmap(scba_layer[active_scba].cnfg_bitmap_layer, icon_full_bottle);
      screen_status = SCBA_CNFG_SCREEN_BOTTLE_TYPE;
      break;
  }
  
  if((key == CLICK_UP) || (key == CLICK_DOWN))
  {
    mini_snprintf(text_buffer, sizeof(text_buffer), "%d", scba_team_data[active_scba].scba_team_nr);
    text_layer_set_text(scba_layer[active_scba].cnfg_text_layer_input, text_buffer); 
  }
}

/**
*
*/
void change_bottle_type(uint8_t key)
{
  switch(key)
  {
    case CLICK_UP:
      if(scba_team_data[active_scba].scba_team_bottle_type > 2)
      {
        scba_team_data[active_scba].scba_team_bottle_type = 0;
      }
      else
      {
        scba_team_data[active_scba].scba_team_bottle_type += 1; 
      }
      break;
    
    case CLICK_DOWN:
      if(scba_team_data[active_scba].scba_team_bottle_type < 1)
      {
        scba_team_data[active_scba].scba_team_bottle_type = 3;        
      }  
      else
      {
        scba_team_data[active_scba].scba_team_bottle_type -= 1;
     }
      break;
    
    case CLICK_SELECT:
      text_layer_set_text(scba_layer[active_scba].cnfg_text_layer, "Pressure:");
      scba_team_data[active_scba].scba_team_bottle_pressure = available_bottles[scba_team_data[active_scba].scba_team_bottle_type].bottle_default_pressure;
      mini_snprintf(text_buffer, sizeof(text_buffer), "%d", scba_team_data[active_scba].scba_team_bottle_pressure);
      text_layer_set_text(scba_layer[active_scba].cnfg_text_layer_input, text_buffer);
      screen_status = SCBA_CNFG_SCREEN_BOTTLE_PRESSURE;
      break;
  }
  
  if((key == CLICK_UP) || (key == CLICK_DOWN))
  {
    text_layer_set_text(scba_layer[active_scba].cnfg_text_layer_input, available_bottles[scba_team_data[active_scba].scba_team_bottle_type].bottle_name);  
  }
}

/**
*
*/
void change_bottle_pressure(uint8_t key, uint8_t factor)
{
  uint16_t max_pressure = (available_bottles[scba_team_data[active_scba].scba_team_bottle_type].bottle_default_pressure * 110 ) / 100;
  
  switch(key)
  {
    case CLICK_UP:
      if(scba_team_data[active_scba].scba_team_bottle_pressure >= max_pressure)
      {
        scba_team_data[active_scba].scba_team_bottle_pressure = 0;
      }
      else
      {
        scba_team_data[active_scba].scba_team_bottle_pressure += (1 * factor); 
      }
      break;
    
    case CLICK_DOWN:
      if(scba_team_data[active_scba].scba_team_bottle_pressure < 1)
      {
        scba_team_data[active_scba].scba_team_bottle_pressure = max_pressure;        
      }  
      else
      {
        scba_team_data[active_scba].scba_team_bottle_pressure -= (1 * factor);
      }
      break;
    
    case CLICK_SELECT:
      if (screen_status != SCBA_UPDATE_PRESSURE)
      {
        layer_set_hidden((Layer *)scba_layer[active_scba].scba_cnfg_layer, true);
        layer_set_hidden((Layer *)scba_layer[active_scba].scba_info_layer, false);
        time(&scba_team_data[active_scba].scba_team_start_time);
        scba_team_data[active_scba].scba_team_status = SCBA_FULL_BOTTLE_NO_ALARM;
      }
      calc_scba_team_air_volume(active_scba); 
      update_scba_team_end_time(active_scba);
      update_scba_team_info_screen(active_scba);
      
      text_layer_set_text_color(scba_layer[active_scba].scba_bottle_pressure, GColorBlack);
      text_layer_set_background_color(scba_layer[active_scba].scba_bottle_pressure, GColorClear);      
      
      persist_write_data(scba_team_storage_keys[active_scba], &scba_team_data[active_scba], sizeof(scba_team_data[active_scba]));   
      screen_status = SCBA_INFO_SCREEN;
      break;
  }
  
  if((key == CLICK_UP) || (key == CLICK_DOWN))
  {
    mini_snprintf(text_buffer, sizeof(text_buffer), "%d", scba_team_data[active_scba].scba_team_bottle_pressure);
    text_layer_set_text(scba_layer[active_scba].cnfg_text_layer_input, text_buffer);
    update_scba_team_info_screen(active_scba);
  }
}

/**
*
*/
void change_active_scba_icon(void)
{
  uint8_t i;
  
  for(i=0; i<SCBA_TEAMS; i++)
  {
    if(active_scba == i)
    {
        layer_set_hidden((Layer *)scba_layer[i].active_layer, false);
    }
    else
    {
        layer_set_hidden((Layer *)scba_layer[i].active_layer, true);
    }
  }
}

/**
*
*/
void stop_scba_monitoring(uint8_t key)
{
  switch(key)
  {
    case CLICK_SELECT:
      text_layer_set_text(scba_layer[active_scba].start_layer, "Start SCBA");
      text_layer_set_text(scba_layer[active_scba].cnfg_text_layer, "TEAM\nNr.:");
      bitmap_layer_set_bitmap(scba_layer[active_scba].cnfg_bitmap_layer, icon_scba_firefighter);
      scba_team_data[active_scba].scba_team_nr = active_scba+1;
      scba_team_data[active_scba].scba_team_bottle_pressure = SCBA_DATA_DEFAULT_PRESSURE;  
      scba_team_data[active_scba].scba_team_bottle_type = SCBA_DATA_DEFAULT_BOTTLE_TYPE;
      scba_team_data[active_scba].scba_team_status = SCBA_NOT_STARTED;
      persist_delete(scba_team_storage_keys[active_scba]);
      screen_status = SCBA_INFO_SCREEN;
      break;
    
    case CLICK_UP:
    case CLICK_DOWN:
      layer_set_hidden((Layer *)scba_layer[active_scba].start_layer, true);
      layer_set_hidden((Layer *)scba_layer[active_scba].scba_info_layer, false);  
      screen_status = SCBA_INFO_SCREEN;
      break;
    
    default:
      break;
  }
}

/**
*
*/
void set_text_layer_font(TextLayer *layer, GColor background_color, GColor text_color, GTextAlignment text_alignment, const char* font)
{
  text_layer_set_background_color(layer, background_color);
  text_layer_set_text_color(layer, text_color);
  text_layer_set_text_alignment(layer, text_alignment);
  text_layer_set_font(layer, fonts_get_system_font(font));  
}

/**
*
*/
bool update_scba_team_info_screen(uint8_t team_nr)
{
  time_t temp_time = 0;
  time_t absolute_delta_time = 0;
  static uint8_t cnt[SCBA_TEAMS] = {19, 19, 19};
  uint16_t team_default_pressure = available_bottles[scba_team_data[team_nr].scba_team_bottle_type].bottle_default_pressure;
  uint16_t team_pressure = scba_team_data[team_nr].scba_team_bottle_pressure;
  uint16_t team_pressure_third = (team_default_pressure - SCBA_BOTTLE_MIN_PRESSURE) / 4;
  bool alarm = false;
  
  
  time(&temp_time);
  absolute_delta_time = temp_time - scba_team_data[team_nr].scba_team_start_time;
  
  mini_snprintf(scba_layer[team_nr].text_pressure, sizeof(scba_layer[team_nr].text_pressure), "%d", scba_team_data[team_nr].scba_team_bottle_pressure);
  mini_snprintf(scba_layer[team_nr].text_team_nr, sizeof(scba_layer[team_nr].text_team_nr), "%d", scba_team_data[team_nr].scba_team_nr);
  strftime(scba_layer[team_nr].text_start_time, sizeof("00:00"), "%H:%M", localtime(&scba_team_data[team_nr].scba_team_start_time));
  strftime(scba_layer[team_nr].text_passed_time, sizeof("00"), "%M", localtime(&absolute_delta_time));
  
  // set actions according to the bottle pressure
  // mayday alarm
  if(team_pressure < 40)
  {
    bitmap_layer_set_bitmap(scba_layer[team_nr].scba_info_team_layer, icon_small_stop_signe);
    cnt[team_nr] ++;
    
    if (cnt[team_nr] >= 20)
    {
      vibes_double_pulse(); 
      cnt[team_nr] = 0; 
    }
  }
  // return team pressure alarm 
  else if(team_pressure < 50)
  {
    if (scba_team_data[team_nr].scba_team_status < SCBA_EMPTY_BOTTLE_ALARM_CONFIRMED)
    {
      scba_team_data[team_nr].scba_team_status = SCBA_EMPTY_BOTTLE_ALARM;
      alarm = true;
    }
    else
    {
      bitmap_layer_set_bitmap(scba_layer[team_nr].scba_bottle_layer, icon_small_empty_bottle);
    }
  }
  // third third alarm
  else if (team_pressure < (team_default_pressure - (3 * team_pressure_third)))
  {
    if (scba_team_data[team_nr].scba_team_status < SCBA_THIRD_EMPTY_BOTTLE_ALARM_CONFIRMED)
    {
      scba_team_data[team_nr].scba_team_status = SCBA_THIRD_EMPTY_BOTTLE_ALARM;
      alarm = true;
    }
    else
    {
      bitmap_layer_set_bitmap(scba_layer[team_nr].scba_bottle_layer, icon_small_third_empty_bottle);
    }
  }
  // second third alarm
  else if (team_pressure < (team_default_pressure - (2 * team_pressure_third)))
  {
    if (scba_team_data[team_nr].scba_team_status < SCBA_HALF_FULL_BOTTLE_ALARM_CONFIRMED)
    {
      scba_team_data[team_nr].scba_team_status = SCBA_HALF_FULL_BOTTLE_ALARM;
      alarm = true;
    }
    else
    {
      bitmap_layer_set_bitmap(scba_layer[team_nr].scba_bottle_layer, icon_small_half_full_bottle);
    }
  }
  // first third alarm
  else if (team_pressure < (team_default_pressure - team_pressure_third))
  {
    if (scba_team_data[team_nr].scba_team_status < SCBA_THIRD_FULL_BOTTLE_ALARM_CONFIRMED)
    {
      scba_team_data[team_nr].scba_team_status = SCBA_THIRD_FULL_BOTTLE_ALARM;
      alarm = true;
    }
    else
    {
      bitmap_layer_set_bitmap(scba_layer[team_nr].scba_bottle_layer, icon_small_third_full_bottle);
    }
  }
  else
  {
    bitmap_layer_set_bitmap(scba_layer[team_nr].scba_bottle_layer, icon_small_full_bottle);
  }
  
  if (alarm == true)
  {
    vibes_short_pulse();
    bitmap_layer_set_bitmap(scba_layer[team_nr].scba_bottle_layer, icon_small_exclamation_mark);
  }
  else if (team_pressure >= 40)
  {
    bitmap_layer_set_bitmap(scba_layer[team_nr].scba_info_team_layer, icon_small_firefighter);
  }
  
  return (alarm);
}

/**
*
*/
void reduce_scba_team_air_volume(uint8_t team_nr)
{
  if(scba_team_data[team_nr].scba_team_bottle_air_volume >= (SCBA_DEFAULT_AIR_CONSUMPTION / 2))
  {
    scba_team_data[team_nr].scba_team_bottle_air_volume -= (SCBA_DEFAULT_AIR_CONSUMPTION / 2);  
  }
  else
  {
    scba_team_data[team_nr].scba_team_bottle_air_volume = 0;
  }
}

/**
*
*/
void calc_scba_team_air_pressure(uint8_t team_nr)
{
  scba_team_data[team_nr].scba_team_bottle_pressure = (scba_team_data[team_nr].scba_team_bottle_air_volume ) / available_bottles[scba_team_data[team_nr].scba_team_bottle_type].air_volume_in_dliter_per_bar;   
}

/**
*
*/
void calc_scba_team_air_volume(uint8_t team_nr)
{
  scba_team_data[team_nr].scba_team_bottle_air_volume = scba_team_data[team_nr].scba_team_bottle_pressure * available_bottles[scba_team_data[team_nr].scba_team_bottle_type].air_volume_in_dliter_per_bar;
}

/**
*
*/
void update_scba_team_end_time(uint8_t team_nr)
{
  time_t temp_time = 0;
  time_t expected_end_time = 0;
  uint16_t safety_volume = available_bottles[team_nr].air_volume_in_dliter_per_bar * SCBA_BOTTLE_MIN_PRESSURE;
  
  time(&temp_time);
  
  if (scba_team_data[team_nr].scba_team_bottle_pressure >= 50)
  {
    expected_end_time = (temp_time + ( 60 * ((scba_team_data[team_nr].scba_team_bottle_air_volume - safety_volume) / SCBA_DEFAULT_AIR_CONSUMPTION )));
    strftime(scba_layer[team_nr].text_stop_time, sizeof("00:00"), "%H:%M", localtime(&expected_end_time));    
  }
}

//* ----------- main call -------------- *//
//                                        //
//* ------------------------------------ *//
int main(void) 
{
  handle_init();
  app_event_loop();
  handle_deinit();
}
