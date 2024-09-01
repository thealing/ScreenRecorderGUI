#pragma once

#include "platform.h"

#define CHECK(x) check(x, #x)

void check(HRESULT result, const char* expr) {
	if (FAILED(result)) {
		printf("\"%s\" returned error %08x\n", expr, result);
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
}

int load_data(const wchar_t* name, void* data, int size) {
	HKEY key;
	RegOpenKeyEx(HKEY_CURRENT_USER, L"SOFTWARE\\Scabture", 0, KEY_READ, &key);
	RegQueryValueEx(key, name, 0, 0, (LPBYTE)data, (LPDWORD)&size);
	RegCloseKey(key);
	return size;
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

void clamp_window_rect(RECT* rect) {
	RECT screen_rect;
	GetWindowRect(GetDesktopWindow(), &screen_rect);
	IntersectRect(rect, rect, &screen_rect);
}
