//* ----------- include paths ---------- *//
//                                        //
//* ------------------------------------ *//
#include <pebble.h>
#include "mini-printf.h"

//* -------- general definitions ------- *//
//                                        //
//* ------------------------------------ *//
#define SCBA_START_SCREEN  0x00
#define SCBA_CNFG_SCREEN_NR  0x01
#define SCBA_CNFG_SCREEN_BOTTLE_TYPE  0x02
#define SCBA_CNFG_SCREEN_BOTTLE_PRESSURE  0x03
#define SCBA_INFO_SCREEN  0x04
#define SCBA_ALARM  0x05
#define SCBA_UPDATE_PRESSURE  0x06
#define SCBA_STOP_MONITORING  0x07
  
#define SCBA_NOT_STARTED 0x00
#define SCBA_FULL_BOTTLE_NO_ALARM 0x01
#define SCBA_THIRD_FULL_BOTTLE_ALARM 0x02
#define SCBA_THIRD_FULL_BOTTLE_ALARM_CONFIRMED 0x03
#define SCBA_HALF_FULL_BOTTLE_ALARM 0x04
#define SCBA_HALF_FULL_BOTTLE_ALARM_CONFIRMED 0x05
#define SCBA_THIRD_EMPTY_BOTTLE_ALARM 0x06
#define SCBA_THIRD_EMPTY_BOTTLE_ALARM_CONFIRMED 0x07
#define SCBA_MIN_BOTTLE_PRESSURE_ALARM 0x08
#define SCBA_MIN_BOTTLE_PRESSURE_ALARM_CONFIRMED 0x09
#define SCBA_EMPTY_BOTTLE_ALARM 0x0A
#define SCBA_EMPTY_BOTTLE_ALARM_CONFIRMED 0x0B
  
#define SCBA_STORE_KEY_TEAM_ONE   0x0001
#define SCBA_STORE_KEY_TEAM_TWO   0x0010
#define SCBA_STORE_KEY_TEAM_THREE 0x0100
#define SCBA_STORE_KEY_BREATHING_RATE    0x0002
#define SCBA_STORE_KEY_BOTTLE_ONE_AVAILABLE 0x0003
#define SCBA_STORE_KEY_BOTTLE_TWO_AVAILABLE 0x0004
#define SCBA_STORE_KEY_BOTTLE_THREE_AVAILABLE 0x0005
#define SCBA_STORE_KEY_BOTTLE_FOUR_AVAILABLE 0x0006
#define SCBA_STORE_KEY_BOTTLE_FIVE_AVAILABLE 0x0008
#define SCBA_STORE_KEY_BOTTLE_SIX_AVAILABLE 0x0009
#define SCBA_STORE_KEY_DEFAULT_BOTTLE 0x0007
#define SCBA_STORE_KEY_IMPERIAL_UNITS 0x000A
  
#define SCBA_TEAMS 3
  
#define SCBA_DATA_DEFAULT_PRESSURE 300
#define SCBA_DATA_DEFAULT_BOTTLE_TYPE  0
#define SCBA_DEFAULT_AIR_CONSUMPTION 500  // in dliter per minute
#define SCBA_BOTTLE_MIN_PRESSURE 50 // in bar
#define SCBA_AVAILABLE_BOTTLE_TYPES 6
#define SCBA_TEAM_HIGHEST_NR  10

#define NUM_ACTION_BAR_ITEMS   3
  
#define CLICK_UP 0x02
#define CLICK_DOWN 0x01
#define CLICK_SELECT 0x00
#define ORDINARY_CLICK 0x01
#define MULTI_CLICK 0x0A
#define LONG_CLICK_CNT_DELAY 50 // in ms
#define BAR_TO_PSI_FACTOR 14.503773773
#define NOT_AVAILABLE 0
#define AVAILABLE 1
#define DEBUG
  
//* ------- structure definitions ------ *//
//                                        //
//* ------------------------------------ *//
typedef  struct
{
  Layer *scba_info_layer;
  Layer *scba_cnfg_layer;
  TextLayer  *start_layer;
  TextLayer  *cnfg_scba_nr;
  TextLayer  *cnfg_text_layer;
  TextLayer  *cnfg_text_layer_input;
  TextLayer  *cnfg_bottle_type;
  TextLayer  *cnfg_bottle_pressure;
  TextLayer  *scba_start_time;
  TextLayer  *scba_start_time_text;
  TextLayer  *scba_end_time;
  TextLayer  *scba_end_time_text;
  TextLayer  *scba_passed_time;
  TextLayer  *scba_passed_time_text;
  TextLayer  *scba_team_nr;
  TextLayer  *scba_bottle_pressure;  
  BitmapLayer  *scba_info_team_layer;
  BitmapLayer  *scba_bottle_layer;
  BitmapLayer  *cnfg_bitmap_layer;
  BitmapLayer  *active_layer;
  char text_start_time[6];
  char text_stop_time[6];
  char text_passed_time[4];
  char text_team_nr[2];
  char text_pressure[5];
}scba_layer_t;

typedef struct
{
  uint8_t  scba_team_nr;
  time_t   scba_team_start_time;
  uint16_t scba_team_bottle_pressure;
  uint16_t scba_team_bottle_air_volume;
  uint8_t  scba_team_bottle_type;
  uint8_t  scba_team_status;
  uint8_t  scba_team_pressure_psi;
}__attribute__((__packed__)) scba_team_t;

typedef struct
{
  uint8_t  bottle_volume_in_dliter;
  uint16_t bottle_default_pressure;
  uint16_t bottle_default_pressure_in_psi;
  uint16_t air_volume_in_dliter_per_bar;
  char*    bottle_name;
}scba_bottle_t;

//* ---------- global variables -------- *//
//                                        //
//* ------------------------------------ *//
scba_layer_t   scba_layer[SCBA_TEAMS];
scba_team_t    scba_team_data[SCBA_TEAMS];

uint8_t screen_status;
uint8_t active_scba;

char text_buffer[20];
