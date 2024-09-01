#pragma once

#include "resource.h"

#include "convert.h"

#include "capture.h"

int next_multiple(int x, int y) {
	return (x + y - 1) / x * x;
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

bool is_actual_window(HWND window) {
	return GetWindowLong(window, GWL_STYLE) & WS_OVERLAPPEDWINDOW;
}
