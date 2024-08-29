#pragma once

#include "resource.h"

#include "convert.h"

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <windows.h>
#include <commctrl.h>
#include <psapi.h>
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <mftransform.h>
#include <codecapi.h>
#include <dwmapi.h>
#include <mmdeviceapi.h>
#include <audioclient.h>

#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "msimg32.lib")
#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "mf.lib")
#pragma comment(lib, "mfplat.lib")
#pragma comment(lib, "mfreadwrite.lib")
#pragma comment(lib, "mfuuid.lib")
#pragma comment(lib, "dwmapi.lib")

#define CHECK(x) check(x, #x)

void check(HRESULT result, const char* expr) {
	if (FAILED(result)) {
		printf("\"%s\" returned error %08x\n", expr, result);
	}
}

double get_time() {
	LARGE_INTEGER counter;
	QueryPerformanceCounter(&counter);
	LARGE_INTEGER frequency;
	QueryPerformanceFrequency(&frequency);
	return (double)counter.QuadPart / (double)frequency.QuadPart;
}

int get_rect_width(const RECT* rect) {
	return rect->right - rect->left;
}

int get_rect_height(const RECT* rect) {
	return rect->bottom - rect->top;
}

HWND get_window_under_point(POINT point, HWND skip) {
	HWND result = NULL;
	HWND window = GetWindow(GetDesktopWindow(), GW_CHILD);
	while (window != NULL) {
		if (window == skip) {
			goto next;
		}
		if (!IsWindowVisible(window)) {
			goto next;
		}
		RECT rect;
		GetWindowRect(window, &rect);
		if (PtInRect(&rect, point)) {
			result = window;
			window = GetWindow(window, GW_CHILD);
		}
		else {
		next:
			window = GetWindow(window, GW_HWNDNEXT);
		}
	}
	return result;
}

void save_data(const wchar_t* name, void* data, int size) {
	HKEY key;
	RegCreateKeyEx(HKEY_CURRENT_USER, L"SOFTWARE\\Scabture", 0, 0, 0, KEY_WRITE, 0, &key, 0);
	RegSetValueEx(key, name, 0, REG_BINARY, (LPBYTE)data, size);
	RegCloseKey(key);
}

int load_data(const wchar_t* name, void* data, int size) {
	HKEY key;
	RegOpenKeyEx(HKEY_CURRENT_USER, L"SOFTWARE\\Scabture", 0, KEY_READ, &key);
	RegQueryValueEx(key, name, 0, 0, (LPBYTE)data, (LPDWORD)&size);
	RegCloseKey(key);
	return size;
}

bool is_actual_window(HWND window) {
	return GetWindowLong(window, GWL_STYLE) & WS_OVERLAPPEDWINDOW;
}

void get_window_rect(HWND window, RECT* rect) {
	if (is_actual_window(window) && SUCCEEDED(DwmGetWindowAttribute(window, DWMWA_EXTENDED_FRAME_BOUNDS, rect, sizeof(RECT)))) {
	}
	else {
		GetWindowRect(window, rect);
	}
}

void fit_window_rect(HWND window, RECT* rect) {
	if (is_actual_window(window)) {
		RECT real_rect = *rect;
		DwmGetWindowAttribute(window, DWMWA_EXTENDED_FRAME_BOUNDS, &real_rect, sizeof(RECT));
		rect->right = real_rect.right - rect->left;
		rect->bottom = real_rect.bottom - rect->top;
		rect->left = real_rect.left - rect->left;
		rect->top = real_rect.top - rect->top;
	}
	else {
		rect->right = rect->right - rect->left;
		rect->bottom = rect->bottom - rect->top;
		rect->left = 0;
		rect->top = 0;
	}
}
