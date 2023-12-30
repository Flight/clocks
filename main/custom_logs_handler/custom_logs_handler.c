#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <esp_log.h>
#include <esp_system.h>

#include "custom_logs_handler.h"

#include "global_constants.h"

static const char *TAG = "Custom Logs Handler";

static FILE *logs_file = NULL;

char global_log_buffer[LOG_BUFFER_SIZE];
static size_t log_buffer_len = 0;

static const uint32_t LOG_FILE_MAX_SIZE = 1024 * 50; // 50 KB

void open_logs_file()
{
  logs_file = fopen(LOGS_FILE_PATH, "a");
  if (!logs_file)
  {
    ESP_LOGE(TAG, "Failed to open log file");
  }
  else
  {
    ESP_LOGI(TAG, "Log file opened successfully");
  }
}

static void save_logs_to_buffer(const char *format, va_list args)
{
  int written = vsnprintf(global_log_buffer + log_buffer_len, LOG_BUFFER_SIZE - log_buffer_len, format, args);
  if (written > 0)
  {
    log_buffer_len += written;
    if (log_buffer_len >= LOG_BUFFER_SIZE)
    {
      log_buffer_len = 0;
      memset(global_log_buffer, 0, LOG_BUFFER_SIZE);
    }
  }
}

// Check the file size and remove it's content if it's too big
static void check_logs_file_size()
{
  if (logs_file != NULL)
  {
    fseek(logs_file, 0, SEEK_END);
    uint32_t file_size = ftell(logs_file);
    if (file_size > LOG_FILE_MAX_SIZE)
    {
      remove(LOGS_FILE_PATH);
      open_logs_file();
    }
  }
}

static void add_logs_to_file(const char *format, va_list args)
{
  check_logs_file_size();

  if (logs_file != NULL)
  {
    int written = vfprintf(logs_file, format, args);

    if (written >= 0)
    {
      fflush(logs_file);
    }
  }
}

int custom_logs_handler(const char *format, va_list args)
{
  save_logs_to_buffer(format, args);
  add_logs_to_file(format, args);

  return vprintf(format, args);
}
