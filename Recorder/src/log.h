#pragma once

#include "platform.h"

enum {
  LOGGING_NONE,
  LOGGING_SINGLE_FILE,
  LOGGING_DATE_FILE,
};

int logging_mode;
FILE* log_file;

void log_format(const wchar_t* format, ...) {
  if (log_file == NULL) {
    wchar_t log_name[90];
    switch (logging_mode) {
      case LOGGING_SINGLE_FILE: {
        _swprintf(log_name, L"Scabture Log.txt");
        break;
      }
      case LOGGING_DATE_FILE: {
        SYSTEMTIME time;
        GetSystemTime(&time);
        _swprintf(log_name, L"Scabture Log [%04hi.%02hi.%02hi.] [%02hi.%02hi.%02hi.%03hi].txt", time.wYear, time.wMonth, time.wDay, time.wHour, time.wMinute, time.wSecond, time.wMilliseconds);
        break;
      }
    }
    log_file = _wfopen(log_name, L"w");
  }
  va_list args;
  va_start(args, format);
  vfwprintf(log_file, format, args);
  fflush(log_file);
  va_end(args);
}

void log_message(const wchar_t* type, const wchar_t* content) {
  SYSTEMTIME time;
  GetSystemTime(&time);
  log_format(L"[%02hi:%02hi:%02hi] [%s] %s\n", time.wHour, time.wMinute, time.wSecond, type, content);
}

void log_info(const wchar_t* message) {
  if (logging_mode != LOGGING_NONE) {
    log_message(L"INFO", message);
  }
}

void log_error(const wchar_t* message) {
  if (logging_mode != LOGGING_NONE) {
    log_message(L"ERROR", message);
  }
  MessageBeep(MB_ICONERROR);
}
