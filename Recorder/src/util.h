#pragma once

#include "log.h"

double get_time() {
	LARGE_INTEGER counter;
	QueryPerformanceCounter(&counter);
	LARGE_INTEGER frequency;
	QueryPerformanceFrequency(&frequency);
	return (double)counter.QuadPart / (double)frequency.QuadPart;
}

#define CHECK(x) check(x, #x)

void check(HRESULT result, const char* expr) {
	if (FAILED(result)) {
		static int error_codes[256];
		static double error_times[256];
		static int error_count = 0;
		int index = -1;
		for (int i = 0; i < error_count; i++) {
			if (error_codes[i] == result) {
				index = i;
			}
		}
		if (index == -1) {
			index = error_count++;
			error_codes[index] = result;
		}
		double current_time = get_time();
		if (error_times[index] + 1 < current_time) {
			error_times[index] = current_time;
			log_error(L"\"%hs\" returned error %08x", expr, result);
		}
	}
}

void safe_release(void* object) {
	IUnknown** unknown = (IUnknown**)object;
	if (*unknown != NULL) {
		(*unknown)->Release();
		*unknown = NULL;
	}
}

void save_data(const wchar_t* name, void* data, int size) {
	HKEY key;
	RegCreateKeyEx(HKEY_CURRENT_USER, L"SOFTWARE\\Scabture", 0, 0, 0, KEY_WRITE, 0, &key, 0);
	RegSetValueEx(key, name, 0, REG_BINARY, (LPBYTE)data, size);
	RegCloseKey(key);
	log_info(L"Saved data: %s", name);
}

int load_data(const wchar_t* name, void* data, int size) {
	HKEY key;
	RegOpenKeyEx(HKEY_CURRENT_USER, L"SOFTWARE\\Scabture", 0, KEY_READ, &key);
	RegQueryValueEx(key, name, 0, 0, (LPBYTE)data, (LPDWORD)&size);
	RegCloseKey(key);
	log_info(L"Loaded data: %s", name);
	return size;
}

int get_rect_width(const RECT* rect) {
	return rect->right - rect->left;
}

int get_rect_height(const RECT* rect) {
	return rect->bottom - rect->top;
}

void get_client_window_rect(HWND window, RECT* rect) {
	POINT origin = { 0, 0 };
	ClientToScreen(window, &origin);
	RECT window_rect;
	GetWindowRect(window, &window_rect);
	GetClientRect(window, rect);
	OffsetRect(rect, origin.x - window_rect.left, origin.y - window_rect.top);
}

void get_screen_rect(RECT* rect) {
	GetWindowRect(GetDesktopWindow(), rect);
}

void get_clamped_window_rect(HWND window, RECT* rect) {
	RECT window_rect;
	GetWindowRect(window, &window_rect);
	POINT origin = { window_rect.left, window_rect.top };
	RECT screen_rect;
	get_screen_rect(&screen_rect);
	OffsetRect(&screen_rect, -origin.x, -origin.y);
	IntersectRect(rect, rect, &screen_rect);
}

void clamp_window_rect(RECT* rect) {
	RECT screen_rect;
	get_screen_rect(&screen_rect);
	IntersectRect(rect, rect, &screen_rect);
}

