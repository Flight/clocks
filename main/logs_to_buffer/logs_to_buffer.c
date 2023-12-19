#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "logs_to_buffer.h"

#include "global_constants.h"

char global_log_buffer[LOG_BUFFER_SIZE];
static size_t log_buffer_len = 0;

int logs_to_buffer(const char *format, va_list args)
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

  return vprintf(format, args);
}