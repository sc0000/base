#include "log.h"

#ifdef _MSC_VER
  #define _CRT_SECURE_NO_WARNINGS
#endif

#include <sys/stat.h>
#include <direct.h>
#include <stdio.h>
#include <time.h>

#include "base/fileio.h"

char log_file[256];
char time_str[256];

const char* current_time(void) {
  time_t t = time(NULL);
  struct tm tm = *localtime(&t);
  sprintf(time_str, "%02dh%02dm%02ds", 
    tm.tm_hour, tm.tm_min, tm.tm_sec);

  return time_str;
}

void store_startup_time(void) {
  time_t t = time(NULL);
  struct tm tm = *localtime(&t);
  sprintf(log_file, "base_logs/%d-%02d-%02d__%02d-%02d.txt", 
    tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, 
    tm.tm_hour, tm.tm_min);
}

void flog(log_type type, const char* format, ...) {
  va_list args;
  va_start(args, format);

  char msg[256];
  vsnprintf(msg, sizeof(msg), format, args);
  
  va_end(args);

  char typed_msg[1024];

  switch (type) {
    case LOG_INFO: 
      snprintf(typed_msg, sizeof(typed_msg), "INFO [%s] %s\n", current_time(), msg);
      break;

    case LOG_WARNING: 
      snprintf(typed_msg, sizeof(typed_msg), "WARN [%s] %s\n", current_time(), msg);
      break;

    case LOG_ERROR:
      snprintf(typed_msg, sizeof(typed_msg), "ERR  [%s] %s\n", current_time(), msg);
      break;

    case LOG_NUM_TYPES:
    default:
      sprintf(typed_msg, "NONE [%s]\t%s\n", current_time(), msg);
      break;
  }

  dir_ensure("base_logs");
  file_write(log_file, typed_msg);
}
