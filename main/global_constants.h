#ifndef GLOBAL_CONSTANTS_H
#define GLOBAL_CONSTANTS_H

#define LOG_BUFFER_SIZE 20480
#define TEMPERATURE_ERROR_CODE -1000
#define LOGS_FILE_PATH "/spiffs/logs.txt"

extern char *LIGHT_NVS_STORAGE_NAMESPACE;

extern uint16_t adc_low_threshold;
extern uint16_t adc_high_threshold;
extern uint16_t adc_hysteresis_margin;

#endif // GLOBAL_CONSTANTS_H