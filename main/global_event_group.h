#ifndef GLOBAL_EVENT_GROUP_H
#define GLOBAL_EVENT_GROUP_H

#include "freertos/event_groups.h"

extern EventGroupHandle_t global_event_group;

#define IS_LIGHT_SENSOR_READING_DONE_BIT BIT0
#define IS_INSIDE_TEMPERATURE_READING_DONE_BIT BIT1
#define IS_PRECISE_INSIDE_TEMPERATURE_READING_DONE_BIT BIT2
#define IS_OUTSIDE_TEMPERATURE_READING_DONE_BIT BIT3
#define IS_WIFI_CONNECTED_BIT BIT4
#define IS_TIME_SET_BIT BIT5
#define IS_TIME_FROM_PRECISION_CLOCK_FETCHED_BIT BIT6
#define IS_TIME_FROM_NPT_UP_TO_DATE_BIT BIT7

extern bool global_is_light_on;
extern float global_inside_temperature;
extern float global_outside_temperature;

#endif // GLOBAL_EVENT_GROUP_H