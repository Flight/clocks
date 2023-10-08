#include "freertos/event_groups.h"

extern EventGroupHandle_t global_event_group;

#define IS_WIFI_CONNECTED_BIT BIT0
#define IS_TIME_FROM_PRECISION_CLOCK_FETCHED BIT1
#define IS_NTP_SET_BIT BIT2
#define IS_INSIDE_TEMPERATURE_READING_DONE BIT3
#define IS_OUTSIDE_TEMPERATURE_READING_DONE BIT4

extern float global_light_sensor_value;
extern float global_inside_temperature_sensor_value;
extern float global_outside_temperature_sensor_value;
