#pragma once

#include "platform.h"

enum {
  LOGGING_NONE,
  LOGGING_SINGLE_FILE,
  LOGGING_DATE_FILE,
};

int logging_mode;
bool supressed_errors;
HWND main_window;

void log_format(const wchar_t* format, ...) {
  static FILE* file = NULL;
  if (file == NULL) {
    wchar_t log_name[MAX_PATH];
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
    file = _wfopen(log_name, L"w");
  }
  va_list args;
  va_start(args, format);
  vfwprintf(file, format, args);
  fflush(file);
  va_end(args);
}

void log_message(const wchar_t* type, const wchar_t* content) {
  static CRITICAL_SECTION section = {};
  if (section.SpinCount == 0) {
    InitializeCriticalSection(&section);
  }
  EnterCriticalSection(&section);
  SYSTEMTIME time;
  GetSystemTime(&time);
  log_format(L"[%02hi:%02hi:%02hi] [%5i] [%ls] %ls\n", time.wHour, time.wMinute, time.wSecond, GetCurrentThreadId(), type, content);
  LeaveCriticalSection(&section);
}

void log_info(const wchar_t* format, ...) {
  if (logging_mode != LOGGING_NONE) {
    wchar_t content[128];
    va_list args;
    va_start(args, format);
    vswprintf(content, format, args);
    va_end(args);
    log_message(L"INFO", content);
  }
}

void log_error(const wchar_t* format, ...) {
  if (logging_mode != LOGGING_NONE) {
    wchar_t content[128];
    va_list args;
    va_start(args, format);
    vswprintf(content, format, args);
    va_end(args);
    log_message(L"ERROR", content);
  }
  if (!supressed_errors) {
    int option = MessageBox(main_window, L"One or more errors have occurred. For more information, check the log file.\n\nTo quit the application, press 'Abort'.\n\nTo continue execution, press 'Retry'.\n\nTo supress further error messages, press 'Ignore'.\n", L"Scabture error", MB_ABORTRETRYIGNORE | MB_ICONERROR);
    if (option == IDIGNORE) {
      log_info(L"Errors supressed");
      supressed_errors = true;
    }
    if (option == IDABORT) {
      log_info(L"Aborting");
      PostMessage(main_window, WM_DESTROY, 0, 0);
      supressed_errors = true;
    }
  }
}
