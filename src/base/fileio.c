#include "base/fileio.h"

#ifdef _MSC_VER
  #define _CRT_SECURE_NO_WARNINGS
#endif

#ifdef _WIN32
  #include <direct.h>
  #define MKDIR(path) _mkdir(path)
#else
  #include <unistd.h>
  #define MKDIR(path) mkdir(path, 0755)
#endif

#include <stdio.h>
#include <sys/stat.h>

void dir_ensure(const char* path) {
  struct stat st;

  if (stat(path, &st) == 0)
      return;

  MKDIR(path);
}

void file_write(const char* path, const char* text) {
  FILE* file = fopen(path, "a");
  if (!file) return;
  fprintf(file, text);
  fclose(file);
}
  
