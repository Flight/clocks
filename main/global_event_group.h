#ifndef GLOBAL_EVENT_GROUP_H
#define GLOBAL_EVENT_GROUP_H

#include "freertos/event_groups.h"

extern EventGroupHandle_t global_event_group;

#define IS_LIGHT_SENSOR_READING_DONE_BIT BIT0
#define IS_INSIDE_TEMPERATURE_READING_DONE_BIT BIT1
#define IS_OUTSIDE_WEATHER_READING_DONE_BIT BIT2
#define IS_WIFI_CONNECTED_BIT BIT3
#define IS_WIFI_FAILED_BIT BIT4
#define IS_TIME_SET_BIT BIT5
#define IS_TIME_FROM_NPT_UP_TO_DATE_BIT BIT6

#define TEMPERATURE_ERROR_CODE -1000

enum LightLevel
{
  LIGHT_LEVEL_LOW,
  LIGHT_LEVEL_MEDIUM,
  LIGHT_LEVEL_HIGH,
  LIGHT_LEVELS_AMOUNT
};
extern uint8_t global_light_level_index;
extern float global_inside_temperature;    // -1000 - error
extern float global_outside_temperature;   // -1000 - error
extern int8_t global_outside_will_it_rain; // -1 - error, 0 - no, 1 - yes

#endif // GLOBAL_EVENT_GROUP_H