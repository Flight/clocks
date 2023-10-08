#include "freertos/event_groups.h"

extern EventGroupHandle_t global_event_group;

#define IS_LIGHT_SENSOR_READING_DONE BIT0
#define IS_INSIDE_TEMPERATURE_READING_DONE BIT1
#define IS_WIFI_CONNECTED_BIT BIT2
#define IS_TIME_FROM_PRECISION_CLOCK_FETCHED BIT3
#define IS_NTP_SET_BIT BIT4
#define IS_OUTSIDE_TEMPERATURE_READING_DONE BIT5

extern bool global_is_light_on;
extern float global_inside_temperature_sensor_value;
extern float global_outside_temperature_sensor_value;
