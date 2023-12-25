// Custom logging function that writes the log messages to a buffer together with the console. This is useful for debugging when the device is not connected to a computer.

void open_logs_file();

int custom_logs_handler(const char *format, va_list args);