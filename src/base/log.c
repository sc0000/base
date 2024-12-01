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
  sprintf(log_file, "logs/%d-%02d-%02d__%02d-%02d.txt", 
    tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, 
    tm.tm_hour, tm.tm_min);
}

void flog(log_type type, const char* msg) {
  char typed_msg[512];
  
  switch (type) {
    case LOG_INFO: 
      sprintf(typed_msg, "INFO [%s]\t%s\n", current_time(), msg);
      break;

    case LOG_WARNING: 
      sprintf(typed_msg, "WARN [%s]\t%s\n", current_time(), msg);
      break;

    case LOG_ERROR:
      sprintf(typed_msg, "ERR  [%s]\t%s\n", current_time(), msg);
      break;

    case LOG_NUM_TYPES:
      break;
  }

  dir_ensure("logs");
  file_write(log_file, typed_msg);
}
