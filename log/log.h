#pragma once

typedef enum {
  LOG_INFO,
  LOG_WARNING,
  LOG_ERROR
} log_type;

const char* current_time(void);
void store_startup_time(void);
void flog(log_type type, const char* msg);
