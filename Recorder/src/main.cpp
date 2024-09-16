#include "main.h"

enum {
	VIDEO_SOURCE_FULLSCREEN,
	VIDEO_SOURCE_RECTANGLE,
	VIDEO_SOURCE_WINDOW,
};

enum {
	AUDIO_INPUT_SYSTEM = 0x1,
	AUDIO_INPUT_MICROPHONE = 0x2,
};

enum {
	CAPTURE_DEFAULT,
	CAPTURE_BITBLT_GETDIBITS,
	CAPTURE_BITBLT_GETBITMAPBITS,
	CAPTURE_CAPTUREBLT_GETDIBITS,
	CAPTURE_CAPTUREBLT_GETBITMAPBITS,
	CAPTURE_PRINTWINDOW_GETDIBITS,
	CAPTURE_PRINTWINDOW_GETBITMAPBITS,
	CAPTURE_DWM_PRINTWINDOW,
	CAPTURE_GETDIBITS,
	CAPTURE_GETBITMAPBITS,
	CAPTURE_DIRECT3D_9_GETFRONTBUFFERDATA,
	CAPTURE_DIRECT3D_DWMGETDXSHAREDSURFACE,
	CAPTURE_DIRECT3D_DWMDXGETWINDOWSHAREDSURFACE,
	CAPTURE_DXGI_OUTPUT_DUPLICATION,
	CAPTURE_COUNT,
};

enum {
	SOURCE_TYPE_ENTIRE_WINDOW,
	SOURCE_TYPE_VISIBLE_AREA,
	SOURCE_TYPE_CLIENT_AREA,
};

enum {
	RESIZER_RESIZER_DSP,
	RESIZER_VIDEO_PROCESSOR_MFT,
};

enum {
	RESIZE_MODE_STRETCH,
	RESIZE_MODE_LETTERBOX,
	RESIZE_MODE_CROP,
};

enum {
	COLOR_BGR,
	COLOR_RGB,
};

enum {
	FORMAT_IYUV,
	FORMAT_NV12,
};

struct Hotkey {
	wchar_t key;
	bool control;
	bool alt;
	bool shift;
};

struct Settings {
	bool stay_on_top;
	bool hide_at_start;
	bool hide_from_taskbar;
	bool ask_to_play;
	bool beep_at_start;
	bool beep_at_stop;
	Hotkey start_hotkey;
	Hotkey stop_hotkey;
	Hotkey pause_hotkey;
	Hotkey resume_hotkey;
	bool quality_preview;
	bool disable_preview;
	bool stop_on_close;
	Hotkey refresh_hotkey;
	int format;
};

struct Video_Options {
	int capture_method;
	int source_type;
	int resize_mode;
	int resizer_method;
	bool quality_resizer;
	bool gpu_resizer;
	bool draw_cursor;
	bool always_cursor;
	bool use_hook;
	int color;
	int format;
};

struct Audio_Options {
	int channel_count;
};

struct Audio_Player {
	bool playing;
	IMMDevice* device;
	IAudioClient* audio_client;
	IAudioRenderClient* render_client;
};

struct Audio_Input {
	bool active;
	IMMDevice* audio_device;
	IAudioClient* audio_client;
	IAudioCaptureClient* capture_client;
	HANDLE audio_event;
	WAVEFORMATEX* wave_format;
	DWORD stream_index;
	HANDLE thread;
	Audio_Player player;
	IMFTransform* resampler;
	DWORD bytes_per_sec;
	HRESULT error;
};

Audio_Input microphone_input;
Audio_Input system_input;
Audio_Options audio_options;
BITMAPINFOHEADER capture_info;
bool capture_resize;
bool capture_running;
bool fullscreen;
bool lock_controls;
bool dirty_capture;
bool recording_paused;
bool recording_running;
bool resample_audio;
Capture_Interface* current_capture;
const wchar_t* output_path;
double meas_capture_fps;
double meas_encode_fps;
double meas_interval;
double recording_pause_time;
double recording_start_time;
DWORD video_stream_index;
HANDLE capture_thread;
HANDLE encode_thread;
HANDLE frame_event;
HANDLE stop_event;
HBITMAP pause_image;
HBITMAP render_bitmap;
HBITMAP resume_image;
HBITMAP settings_image;
HBITMAP start_image;
HBITMAP stop_image;
HBRUSH background_brush;
HBRUSH black_brush;
HBRUSH white_brush;
HDC main_context;
HDC render_context;
HFONT display_font;
HFONT fps_font;
HFONT main_font;
HMODULE main_instance;
HPEN outline_pen;
HWND audio_button;
HWND audio_source_edit;
HWND audio_source_label;
HWND audio_source_list;
HWND audio_window;
HWND bitrate_edit;
HWND bitrate_label;
HWND cpu_label;
HWND framerate_edit;
HWND framerate_label;
HWND height_edit;
HWND height_label;
HWND keep_ratio_checkbox;
HWND main_window;
HWND mem_label;
HWND pause_button;
HWND resize_checkbox;
HWND settings_button;
HWND settings_window;
HWND size_label;
HWND source_window;
HWND start_button;
HWND stop_button;
HWND time_label;
HWND video_button;
HWND video_source_edit;
HWND video_source_label;
HWND video_source_list;
HWND video_window;
HWND width_edit;
HWND width_label;
IMFSinkWriter* sink_writer;
IMFTransform* resizer;
int aligned_height;
int aligned_width;
int audio_sources;
int capture_bitrate;
int capture_framerate;
int capture_height;
int capture_width;
int source_height;
int source_width;
int video_source;
int64_t recording_size;
int64_t recording_time;
RECT main_rect;
RECT source_rect;
Settings settings;
size_t capture_size_max;
SRWLOCK capture_lock;
Video_Options video_options;
void* capture_buffer;
bool hook_working;

Capture_Interface* get_capture(int index) {
	static Capture_Interface** captures = NULL;
	if (captures == NULL) {
		captures = new Capture_Interface*[CAPTURE_COUNT];
		captures[CAPTURE_BITBLT_GETDIBITS] = new Capture_BitBlt_GetDIBits;
		captures[CAPTURE_BITBLT_GETBITMAPBITS] = new Capture_BitBlt_GetBitmapBits;
		captures[CAPTURE_CAPTUREBLT_GETDIBITS] = new Capture_CaptureBlt_GetDIBits;
		captures[CAPTURE_CAPTUREBLT_GETBITMAPBITS] = new Capture_CaptureBlt_GetBitmapBits;
		captures[CAPTURE_PRINTWINDOW_GETDIBITS] = new Capture_PrintWindow_GetDIBits;
		captures[CAPTURE_PRINTWINDOW_GETBITMAPBITS] = new Capture_PrintWindow_GetBitmapBits;
		captures[CAPTURE_DWM_PRINTWINDOW] = new Capture_DWM_PrintWindow;
		captures[CAPTURE_GETDIBITS] = new Capture_GetDIBits;
		captures[CAPTURE_GETBITMAPBITS] = new Capture_GetBitmapBits;
		captures[CAPTURE_DIRECT3D_9_GETFRONTBUFFERDATA] = new Capture_Direct3D_9;
		captures[CAPTURE_DIRECT3D_DWMGETDXSHAREDSURFACE] = new Capture_DwmGetDxSharedSurface;
		captures[CAPTURE_DIRECT3D_DWMDXGETWINDOWSHAREDSURFACE] = new Capture_DwmDxGetWindowSharedSurface;
		captures[CAPTURE_DXGI_OUTPUT_DUPLICATION] = new Capture_DXGI_Output_Duplication;
	}
	if (index == CAPTURE_DEFAULT) {
		if (video_source == VIDEO_SOURCE_WINDOW) {
			if (video_options.source_type == SOURCE_TYPE_VISIBLE_AREA) {
				index = CAPTURE_BITBLT_GETDIBITS;
			}
			else {
				index = CAPTURE_DWM_PRINTWINDOW;
			}
		}
		else {
			index = CAPTURE_DXGI_OUTPUT_DUPLICATION; 
		}
	}
	return captures[index];
}

Capture_Interface* get_hook_capture() {
	static Capture_Interface* hook_capture = new Capture_Hook;
	return hook_capture;
}

void get_window_rect(HWND window, RECT* rect) {
	switch (video_options.source_type) {
		case SOURCE_TYPE_ENTIRE_WINDOW: {
			GetWindowRect(window, rect);
			break;
		}
		case SOURCE_TYPE_VISIBLE_AREA: {
			GetWindowRect(window, rect);
			if (is_actual_window(window)) {
				DwmGetWindowAttribute(window, DWMWA_EXTENDED_FRAME_BOUNDS, rect, sizeof(RECT));
			}
			break;
		}
		case SOURCE_TYPE_CLIENT_AREA: {
			GetClientRect(window, rect);
			POINT origin = { rect->left, rect->top };
			ClientToScreen(window, &origin);
			OffsetRect(rect, origin.x, origin.y);
			break;
		}
	}
}

void fit_window_rect(HWND window, RECT* rect) {
	switch (video_options.source_type) {
		case SOURCE_TYPE_ENTIRE_WINDOW: {
			OffsetRect(rect, -rect->left, -rect->top);
			break;
		}
		case SOURCE_TYPE_VISIBLE_AREA: {
			if (is_actual_window(window)) {
				RECT real_rect = *rect;
				DwmGetWindowAttribute(window, DWMWA_EXTENDED_FRAME_BOUNDS, &real_rect, sizeof(RECT));
				OffsetRect(&real_rect, -rect->left, -rect->top);
				*rect = real_rect;
			}
			else {
				OffsetRect(rect, -rect->left, -rect->top);
			}
			break;
		}
		case SOURCE_TYPE_CLIENT_AREA: {
			GetClientRect(window, rect);
			break;
		}
	}
}

void screen_to_window(HWND window, POINT* point) {
	if (video_options.use_hook) {
		ScreenToClient(window, point);
		RECT rect;
		GetClientRect(window, &rect);
		if (rect.right > 0 && rect.bottom > 0) {
			point->x = point->x * source_width / rect.right;
			point->y = point->y * source_height / rect.bottom;
		}
		return;
	}
	switch (video_options.source_type) {
		case SOURCE_TYPE_ENTIRE_WINDOW: {
			RECT rect;
			GetWindowRect(window, &rect);
			point->x -= rect.left;
			point->y -= rect.top;
			break;
		}
		case SOURCE_TYPE_VISIBLE_AREA: {
			RECT rect;
			get_window_rect(window, &rect);
			point->x -= rect.left;
			point->y -= rect.top;
			break;
		}
		case SOURCE_TYPE_CLIENT_AREA: {
			ScreenToClient(window, point);
			break;
		}
	}
}

HWND get_fullscreen_window() {
	return GetForegroundWindow();
}

LRESULT CALLBACK selection_proc(HWND window, UINT message, WPARAM wparam, LPARAM lparam) {
	static HBITMAP draw_bitmap;
	static HDC draw_context;
	static HDC window_context;
	static HWND selected_window;
	static int start_x;
	static int start_y;
	static int state;
	static RECT selected_rect;
	static RECT window_rect;
	bool paint = false;
	if (state == 0) {
		state = video_source;
		int screen_width = GetSystemMetrics(SM_CXSCREEN);
		int screen_height = GetSystemMetrics(SM_CYSCREEN);
		SetRect(&window_rect, 0, 0, screen_width, screen_height);
		SetRectEmpty(&selected_rect);
		window_context = GetDC(window);
		draw_context = CreateCompatibleDC(window_context);
		draw_bitmap = CreateCompatibleBitmap(window_context, screen_width, screen_height);
		SelectObject(draw_context, draw_bitmap);
		paint = true;
	}
	switch (message) {
		case WM_NCDESTROY: {
			state = 0;
			DeleteObject(draw_bitmap);
			DeleteDC(draw_context);
			ReleaseDC(window, window_context);
			break;
		}
		case WM_KEYDOWN: {
			video_source = VIDEO_SOURCE_FULLSCREEN;
			SendMessage(video_source_list, CB_SETCURSEL, 0, 0);
			DestroyWindow(window);
			break;
		}
		case WM_MOUSEMOVE: {
			if (state == 2) {
				int mouse_x = LOWORD(lparam);
				int mouse_y = HIWORD(lparam);
				if (video_source == VIDEO_SOURCE_RECTANGLE) {
					selected_rect.left = min(start_x, mouse_x);
					selected_rect.top = min(start_y, mouse_y);
					selected_rect.right = max(start_x, mouse_x);
					selected_rect.bottom = max(start_y, mouse_y);
				}
				if (video_source == VIDEO_SOURCE_WINDOW) {
					POINT position = { mouse_x, mouse_y };
					selected_window = get_window_under_point(position, window);
					get_window_rect(selected_window, &selected_rect);
				}
				FillRect(draw_context, &window_rect, black_brush);
				FillRect(draw_context, &selected_rect, white_brush);
				paint = TRUE;
			}
			break;
		}
		case WM_LBUTTONDOWN: {
			state++;
			int mouseX = LOWORD(lparam);
			int mouseY = HIWORD(lparam);
			if (state == 2) {
				start_x = mouseX;
				start_y = mouseY;
			}
			if (state == 3) {
				if (video_source == VIDEO_SOURCE_RECTANGLE) {
					source_window = GetDesktopWindow();
					source_rect = selected_rect;
				}
				if (video_source == VIDEO_SOURCE_WINDOW) {
					POINT position = { mouseX, mouseY };
					source_window = selected_window;
					source_rect = selected_rect;
					fit_window_rect(selected_window, &source_rect);
				}
				DestroyWindow(window);
			}
			break;
		}
	}
	if (paint) {
		POINT position = { 0, 0 };
		SIZE size = { window_rect.right, window_rect.bottom };
		BLENDFUNCTION blend_func = {};
		blend_func.SourceConstantAlpha = 100;
		UpdateLayeredWindow(window, NULL, &position, &size, draw_context, &position, 0, &blend_func, ULW_ALPHA);
	}
	return DefWindowProc(window, message, wparam, lparam);
}

LRESULT CALLBACK source_list_proc(HWND window, UINT message, WPARAM wparam, LPARAM lparam, UINT_PTR, DWORD_PTR) {
	if (message == WM_PAINT || message == WM_COMMAND) {
		PostMessage(video_source_edit, EM_SETSEL, (WPARAM)-1, 0);
		PostMessage(audio_source_edit, EM_SETSEL, (WPARAM)-1, 0);
	}
	return DefSubclassProc(window, message, wparam, lparam);
}

LRESULT CALLBACK edit_proc(HWND window, UINT message, WPARAM wparam, LPARAM lparam, UINT_PTR, DWORD_PTR) {
	if (message == WM_CHAR && wparam == VK_RETURN) {
		DWORD end;
		PostMessage(window, EM_GETSEL, NULL, (LPARAM)&end);
		SetFocus(main_window);
		SetFocus(window);
		PostMessage(window, EM_SETSEL, end, end);
		return 0;
	}
	return DefSubclassProc(window, message, wparam, lparam);
}

LRESULT CALLBACK button_proc(HWND window, UINT message, WPARAM wparam, LPARAM lparam, UINT_PTR, DWORD_PTR) {
	if (message == WM_ERASEBKGND) {
		return 1;
	}
	return DefSubclassProc(window, message, wparam, lparam);
}

LRESULT CALLBACK form_checkbox_proc(HWND window, UINT message, WPARAM wparam, LPARAM lparam, UINT_PTR, DWORD_PTR) {
	bool* data = (bool*)GetWindowLongPtr(window, GWLP_USERDATA);
	if (message == BM_SETCHECK) {
		*data = wparam;
	}
	if (message == WM_USER && wparam == 0) {
		SendMessage(window, BM_SETCHECK, *data, 0);
	}
	if (message == WM_USER) {
		return wparam;
	}
	return DefSubclassProc(window, message, wparam, lparam);
}

LRESULT CALLBACK form_hotkey_proc(HWND window, UINT message, WPARAM wparam, LPARAM lparam, UINT_PTR, DWORD_PTR) {
	Hotkey* data = (Hotkey*)GetWindowLongPtr(window, GWLP_USERDATA);
	if (message == WM_KEYDOWN) {
		wchar_t key = (wchar_t)wparam;
		if (iswalnum(key)) {
			data->key = key;
			data->control = GetKeyState(VK_CONTROL) & 0x8000;
			data->shift = GetKeyState(VK_SHIFT) & 0x8000;
			data->alt = GetKeyState(VK_MENU) & 0x8000;
		}
	}
	if (message == WM_KEYDOWN || message == WM_USER && wparam == 0) {
		wchar_t buffer[128] = L"";
		if (data->control) {
			wcscat(buffer, L"Ctrl+");
		}
		if (data->shift) {
			wcscat(buffer, L"Shift+");
		}
		if (data->alt) {
			wcscat(buffer, L"Alt+");
		}
		buffer[wcslen(buffer)] = data->key;
		if (data->key == 0) {
			wcscpy(buffer, L"None");
		}
		SetWindowText(window, buffer);
	}
	if (message == WM_CHAR) {
		return 0;
	}
	if (message == WM_USER) {
		return wparam;
	}
	return DefSubclassProc(window, message, wparam, lparam);
}

LRESULT CALLBACK form_list_proc(HWND window, UINT message, WPARAM wparam, LPARAM lparam, UINT_PTR, DWORD_PTR) {
	int* data = (int*)GetWindowLongPtr(window, GWLP_USERDATA);
	if (message == WM_USER && wparam == 1) {
		*data = (int)SendMessage(window, CB_GETCURSEL, 0, 0);
	}
	if (message == WM_USER && wparam == 0) {
		SendMessage(window, CB_SETCURSEL, *data, 0);
	}
	if (message == WM_USER) {
		return wparam;
	}
	return DefSubclassProc(window, message, wparam, lparam);
}

BOOL CALLBACK set_font_proc(HWND child, LPARAM lparam) {
	if (SendMessage(child, WM_GETFONT, 0, 0) == NULL) {
		SendMessage(child, WM_SETFONT, (WPARAM)lparam, 0);
	}
	return TRUE;
}

BOOL CALLBACK custom_draw_proc(HWND child, LPARAM) {
	if (GetWindowLongPtr(child, GWLP_USERDATA) == 0) {
		return TRUE;
	}
	wchar_t class_name[256];
	GetClassName(child, class_name, _countof(class_name));
	int style = GetWindowLong(child, GWL_STYLE);
	if (_wcsicmp(class_name, L"button") == 0 && (style & BS_OWNERDRAW) || _wcsicmp(class_name, L"static") == 0 && (style & SS_OWNERDRAW)) {
		RedrawWindow(child, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW);
	}
	return TRUE;
}

void update_dimensions() {
	source_width = next_multiple(2, max(1, get_rect_width(&source_rect)));
	source_height = next_multiple(2, max(1, get_rect_height(&source_rect)));
	aligned_width = next_multiple(32, source_width);
	aligned_height = next_multiple(2, source_height);
	if (capture_resize) {
		capture_width = next_multiple(2, capture_width);
		capture_height = next_multiple(2, capture_height);
	}
	else {
		capture_width = next_multiple(2, source_width);
		capture_height = next_multiple(2, source_height);
	}
}

void draw_cursor() {
	if (!IsWindow(source_window) || IsIconic(source_window)) {
		return;
	}
	if (!video_options.draw_cursor) {
		return;
	}
	static CURSORINFO cursor_info;
	cursor_info.cbSize = sizeof(cursor_info);
	if (!GetCursorInfo(&cursor_info)) {
		return;
	}
	if (cursor_info.flags != CURSOR_SHOWING && !video_options.always_cursor) {
		return;
	}
	static ICONINFO icon_info;
	ICONINFO new_icon_info;
	if (GetIconInfo(cursor_info.hCursor, &new_icon_info)) {
		DeleteObject(icon_info.hbmColor);
		DeleteObject(icon_info.hbmMask);
		icon_info = new_icon_info;
	}
	if (icon_info.hbmColor == NULL && icon_info.hbmMask == NULL) {
		return;
	}
	screen_to_window(source_window, &cursor_info.ptScreenPos);
	BITMAP icon_bitmap;
	GetObject(icon_info.hbmColor ? icon_info.hbmColor : icon_info.hbmMask, sizeof(BITMAP), &icon_bitmap);
	int width = icon_bitmap.bmWidth;
	int height = icon_info.hbmColor ? icon_bitmap.bmHeight : icon_bitmap.bmHeight / 2;
	BITMAPINFO bitmap_info = {};
	bitmap_info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bitmap_info.bmiHeader.biWidth = icon_bitmap.bmWidth;
	bitmap_info.bmiHeader.biHeight = -icon_bitmap.bmHeight;
	bitmap_info.bmiHeader.biSizeImage = icon_bitmap.bmWidth * icon_bitmap.bmHeight * 4;
	bitmap_info.bmiHeader.biPlanes = 1;
	bitmap_info.bmiHeader.biBitCount = 32;
	bitmap_info.bmiHeader.biCompression = BI_RGB;
	uint32_t* pixels = (uint32_t*)malloc(width * height * 4);
	uint32_t* masks = (uint32_t*)malloc(width * height * 4);
	static HDC context = CreateCompatibleDC(NULL);
	if (icon_info.hbmColor) {
		GetDIBits(context, icon_info.hbmColor, 0, height, pixels, &bitmap_info, DIB_RGB_COLORS);
		GetDIBits(context, icon_info.hbmMask, 0, height, masks, &bitmap_info, DIB_RGB_COLORS);
	}
	else {
		GetDIBits(context, icon_info.hbmMask, 0, height, pixels, &bitmap_info, DIB_RGB_COLORS);
		GetDIBits(context, icon_info.hbmMask, height, height, masks, &bitmap_info, DIB_RGB_COLORS);
	}
	for (int i = 0; i < height; i++) {
		for (int j = 0; j < width; j++) {
			uint8_t* pixel = (uint8_t*)((uint32_t*)pixels + i * width + j);
			uint8_t* mask = (uint8_t*)((uint32_t*)masks + i * width + j);
			if (*(uint32_t*)pixel == 0) {
				continue;
			}
			int y = cursor_info.ptScreenPos.y - icon_info.yHotspot + i;
			int x = cursor_info.ptScreenPos.x - icon_info.xHotspot + j;
			if (x < 0 || x >= source_width || y < 0 || y >= source_height) {
				continue;
			}
			uint8_t* dest = (uint8_t*)((uint32_t*)capture_buffer + y * aligned_width + x);
			if (icon_info.hbmColor) {
				for (int i = 0; i < 4; i++) {
					dest[i] = (dest[i] * (255 - pixel[3]) + pixel[i] * pixel[3]) / 255;
				}
			}
			else {
				for (int i = 0; i < 4; i++) {
					dest[i] ^= mask[i];
				}
			}
		}
	}
	free(pixels);
	free(masks);
}

void capture_proc() {
	HANDLE timer = CreateWaitableTimer(NULL, TRUE, NULL);
	LARGE_INTEGER due_time;
	GetSystemTimeAsFileTime((FILETIME*)&due_time);
	SetWaitableTimer(timer, &due_time, 0, NULL, NULL, FALSE);
	double meas_time = get_time();
	int meas_count = 0;
	HANDLE capture_events[] = { stop_event, timer };
	while (WaitForMultipleObjects(_countof(capture_events), capture_events, FALSE, INFINITE) != 0) {
		due_time.QuadPart = due_time.QuadPart + 10000000 / capture_framerate;
		SetWaitableTimer(timer, &due_time, 0, NULL, NULL, FALSE);
		double current_time = get_time();
		bool captured = current_capture->get(capture_buffer, &capture_lock);
		if (captured) {
			draw_cursor();
		}
		SetEvent(frame_event);
		if (captured) {
			static double last_time = get_time();
			double current_time = get_time();
			if (current_time > last_time + 1.0 / 60.0) {
				last_time = current_time;
				RedrawWindow(main_window, NULL, NULL, RDW_INVALIDATE);
			}
		}
		if (captured) {
			meas_count++;
		}
		if (current_time > meas_time + meas_interval) {
			meas_capture_fps = meas_count / (current_time - meas_time);
			meas_time = current_time;
			meas_count = 0;
		}
		LARGE_INTEGER time;
		GetSystemTimeAsFileTime((FILETIME*)&time);
		due_time.QuadPart = max(due_time.QuadPart, time.QuadPart);
	}
	CloseHandle(timer);
}

void start_capture() {
	if (capture_running || dirty_capture) {
		return;
	}
	dirty_capture = true;
	hook_working = false;
	Capture_Source source;
	source.window = fullscreen ? get_fullscreen_window() : source_window;
	if (fullscreen) {
		get_client_window_rect(source.window, &source_rect);
		update_dimensions();
	}
	while (video_options.use_hook && (fullscreen || video_source == VIDEO_SOURCE_WINDOW)) {
		UpdateWindow(main_window);
		RECT backup_rect = source_rect;
		if (!inject_hook(source.window, &source_rect)) {
			source_rect = backup_rect;
			break;
		}
		update_dimensions();
		current_capture = get_hook_capture();
		hook_working = true;
		break;
	}
	if (!hook_working) {
		current_capture = get_capture(video_options.capture_method);
	}
	capture_running = true;
	capture_info.biSize = sizeof(capture_info);
	capture_info.biWidth = aligned_width;
	capture_info.biHeight = -aligned_height;
	capture_info.biPlanes = 1;
	capture_info.biBitCount = 32;
	capture_info.biSizeImage = aligned_width * aligned_height * 4;
	capture_info.biCompression = BI_RGB;
	if (capture_info.biSizeImage > capture_size_max) {
		capture_running = false;
		meas_capture_fps = 0;
		goto end;
	}
	source.client = video_options.source_type == SOURCE_TYPE_CLIENT_AREA;
	source.rect = source_rect;
	source.width = source_width;
	source.height = source_height;
	source.stride = aligned_width;
	if (FAILED(current_capture->start(source))) {
		current_capture->stop();
		capture_running = false;
		meas_capture_fps = 0;
		goto end;
	}
	capture_buffer = _aligned_malloc(capture_info.biSizeImage, 16);
	frame_event = CreateEvent(NULL, FALSE, FALSE, NULL);
	stop_event = CreateEvent(NULL, FALSE, FALSE, NULL);
	capture_thread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)capture_proc, NULL, 0, NULL);
	recording_time = 0;
	recording_size = 0;
end:
	dirty_capture = false;
}

void stop_capture() {
	if (!capture_running || dirty_capture) {
		return;
	}
	dirty_capture = true;
	capture_running = false;
	if (capture_info.biSizeImage > capture_size_max) {
		goto end;
	}
	SetEvent(stop_event);
	WaitForSingleObject(capture_thread, INFINITE);
	CloseHandle(capture_thread);
	CloseHandle(frame_event);
	CloseHandle(stop_event);
	current_capture->stop();
	_aligned_free(capture_buffer);
end:
	dirty_capture = false;
}

void update_capture() {
	stop_capture();
	update_dimensions();
	start_capture();
}

HBITMAP load_image(int id) {
	return (HBITMAP)LoadImage(main_instance, MAKEINTRESOURCE(id), IMAGE_BITMAP, 0, 0, LR_LOADTRANSPARENT | LR_LOADMAP3DCOLORS);
}

void load_images() {
	start_image = load_image(ID_START);
	stop_image = load_image(ID_STOP);
	pause_image = load_image(ID_PAUSE);
	resume_image = load_image(ID_RESUME);
	settings_image = load_image(ID_SETTINGS);
}

HWND create_control(const wchar_t* name, const wchar_t* label, int styles) {
	HWND control = CreateWindow(name, label, WS_VISIBLE | WS_CHILD | WS_CLIPSIBLINGS | styles, 0, 0, 0, 0, main_window, NULL, NULL, NULL);
	if (wcscmp(name, L"EDIT") == 0) {
		SetWindowSubclass(control, edit_proc, 0, 0);
	}
	if (wcscmp(name, L"BUTTON") == 0) {
		SetWindowSubclass(control, button_proc, 0, 0);
	}
	return control;
}

void create_controls() {
	video_source_label = create_control(L"STATIC", L"Video Source:", SS_CENTERIMAGE);
	video_source_list = create_control(L"COMBOBOX", NULL, CBS_AUTOHSCROLL | CBS_DROPDOWN | CBS_HASSTRINGS);
	SendMessage(video_source_list, CB_ADDSTRING, 0, (LPARAM)L"Fullscreen");
	SendMessage(video_source_list, CB_ADDSTRING, 0, (LPARAM)L"Rectangle");
	SendMessage(video_source_list, CB_ADDSTRING, 0, (LPARAM)L"Window");
	SetWindowSubclass(video_source_list, source_list_proc, 0, 0);
	video_source_edit = FindWindowEx(video_source_list, NULL, L"EDIT", NULL);
	SendMessage(video_source_edit, EM_SETREADONLY, TRUE, 0);
	SendMessage(video_source_list, CB_SETCURSEL, 0, 0);
	audio_source_label = create_control(L"STATIC", L"Audio Source:", SS_CENTERIMAGE);
	audio_source_list = create_control(L"COMBOBOX", NULL, CBS_AUTOHSCROLL | CBS_DROPDOWN | CBS_HASSTRINGS);
	SendMessage(audio_source_list, CB_ADDSTRING, 0, (LPARAM)L"None");
	SendMessage(audio_source_list, CB_ADDSTRING, 0, (LPARAM)L"System");
	SendMessage(audio_source_list, CB_ADDSTRING, 0, (LPARAM)L"Microphone");
	SendMessage(audio_source_list, CB_ADDSTRING, 0, (LPARAM)L"System + Microphone");
	SetWindowSubclass(audio_source_list, source_list_proc, 0, 0);
	audio_source_edit = FindWindowEx(audio_source_list, NULL, L"EDIT", NULL);
	SendMessage(audio_source_edit, EM_SETREADONLY, TRUE, 0);
	SendMessage(audio_source_list, CB_SETCURSEL, 0, 0);
	video_button = create_control(L"BUTTON", L"Video Options", BS_PUSHBUTTON);
	audio_button = create_control(L"BUTTON", L"Audio Options", BS_PUSHBUTTON);
	width_label = create_control(L"STATIC", L"Width:", SS_CENTERIMAGE);
	width_edit = create_control(L"EDIT", L"", WS_BORDER | ES_RIGHT | ES_NUMBER);
	height_label = create_control(L"STATIC", L"Height:", SS_CENTERIMAGE);
	height_edit = create_control(L"EDIT", L"", WS_BORDER | ES_RIGHT | ES_NUMBER);
	resize_checkbox = create_control(L"BUTTON", L"Resize", BS_AUTOCHECKBOX | BS_CHECKBOX);
	keep_ratio_checkbox = create_control(L"BUTTON", L"Keep Ratio", BS_AUTOCHECKBOX | BS_CHECKBOX);
	framerate_label = create_control(L"STATIC", L"Frame Rate (FPS):", SS_CENTERIMAGE);
	framerate_edit = create_control(L"EDIT", L"", WS_BORDER | ES_RIGHT | ES_NUMBER);
	bitrate_label = create_control(L"STATIC", L"Bit Rate (KBPS):", SS_CENTERIMAGE);
	bitrate_edit = create_control(L"EDIT", L"", WS_BORDER | ES_RIGHT | ES_NUMBER);
	start_button = create_control(L"BUTTON", L"", WS_BORDER | BS_OWNERDRAW);
	stop_button = create_control(L"BUTTON", L"", WS_BORDER | BS_OWNERDRAW);
	pause_button = create_control(L"BUTTON", L"", WS_BORDER | BS_OWNERDRAW);
	settings_button = create_control(L"BUTTON", L"", WS_BORDER | BS_OWNERDRAW);
	EnumChildWindows(main_window, set_font_proc, (LPARAM)main_font);
	SendMessage(video_source_label, WM_SETFONT, (WPARAM)main_font, 0);
	SendMessage(video_source_list, WM_SETFONT, (WPARAM)main_font, 0);
	time_label = create_control(L"STATIC", L"", SS_OWNERDRAW);
	size_label = create_control(L"STATIC", L"", SS_OWNERDRAW);
	cpu_label = create_control(L"STATIC", L"", SS_OWNERDRAW);
	mem_label = create_control(L"STATIC", L"", SS_OWNERDRAW);
}

void update_controls() {
	lock_controls = true;
	HWND focused_control = GetFocus();
	wchar_t buffer[128];
	if (width_edit != focused_control) {
		swprintf(buffer, L"%i", capture_width);
		SendMessage(width_edit, WM_SETTEXT, 0, (LPARAM)buffer);
	}
	if (height_edit != focused_control) {
		swprintf(buffer, L"%i", capture_height);
		SendMessage(height_edit, WM_SETTEXT, 0, (LPARAM)buffer);
	}
	if (framerate_edit != focused_control) {
		swprintf(buffer, L"%i", capture_framerate);
		SendMessage(framerate_edit, WM_SETTEXT, 0, (LPARAM)buffer);
	}
	if (bitrate_edit != focused_control) {
		swprintf(buffer, L"%i", capture_bitrate);
		SendMessage(bitrate_edit, WM_SETTEXT, 0, (LPARAM)buffer);
	}
	if (recording_running && focused_control != NULL) {
		SetFocus(NULL);
	}
	EnableWindow(video_source_list, !recording_running);
	EnableWindow(audio_source_list, !recording_running);
	EnableWindow(video_button, !recording_running);
	EnableWindow(audio_button, !recording_running);
	capture_resize = SendMessage(resize_checkbox, BM_GETCHECK, 0, 0) != BST_UNCHECKED;
	EnableWindow(width_edit, !recording_running && capture_resize);
	EnableWindow(height_edit, !recording_running && capture_resize);
	SendMessage(width_edit, EM_SETREADONLY, !capture_resize, 0);
	SendMessage(height_edit, EM_SETREADONLY, !capture_resize, 0);
	EnableWindow(resize_checkbox, !recording_running);
	EnableWindow(keep_ratio_checkbox, !recording_running);
	EnableWindow(framerate_edit, !recording_running);
	EnableWindow(bitrate_edit, !recording_running);
	EnableWindow(start_button, !recording_running);
	EnableWindow(stop_button, recording_running);
	EnableWindow(pause_button, recording_running);
	EnableWindow(settings_button, settings_window == NULL);
	if (video_window != NULL || audio_window != NULL) {
		EnableWindow(start_button, FALSE);
	}
	lock_controls = false;
}

void place_controls() {
	MoveWindow(video_source_label, 10, 10, 80, 20, TRUE);
	MoveWindow(video_source_list, 100, 10, 150, 100, TRUE);
	MoveWindow(video_button, 260, 10, 100, 20, TRUE);
	MoveWindow(audio_source_label, 10, 35, 80, 20, TRUE);
	MoveWindow(audio_source_list, 100, 35, 150, 100, TRUE);
	MoveWindow(audio_button, 260, 35, 100, 20, TRUE);
	MoveWindow(width_label, 380, 10, 50, 20, TRUE);
	MoveWindow(width_edit, 440, 10, 50, 20, TRUE);
	MoveWindow(height_label, 380, 35, 50, 20, TRUE);
	MoveWindow(height_edit, 440, 35, 50, 20, TRUE);
	MoveWindow(resize_checkbox, 520, 10, 75, 20, TRUE);
	MoveWindow(keep_ratio_checkbox, 520, 35, 75, 20, TRUE);
	MoveWindow(framerate_label, 620, 10, 100, 20, TRUE);
	MoveWindow(framerate_edit, 730, 10, 50, 20, TRUE);
	MoveWindow(bitrate_label, 620, 35, 100, 20, TRUE);
	MoveWindow(bitrate_edit, 730, 35, 50, 20, TRUE);
	MoveWindow(start_button, 15, main_rect.bottom - 65, 50, 50, TRUE);
	MoveWindow(pause_button, 80, main_rect.bottom - 65, 50, 50, TRUE);
	MoveWindow(stop_button, 145, main_rect.bottom - 65, 50, 50, TRUE);
	MoveWindow(time_label, 225, main_rect.bottom - 70, 155, 30, TRUE);
	MoveWindow(size_label, 230, main_rect.bottom - 40, 150, 30, TRUE);
	MoveWindow(cpu_label, main_rect.right - 230, main_rect.bottom - 70, 150, 30, TRUE);
	MoveWindow(mem_label, main_rect.right - 230, main_rect.bottom - 40, 150, 30, TRUE);
	MoveWindow(settings_button, main_rect.right - 65, main_rect.bottom - 65, 50, 50, TRUE);
}

void select_source() {
	video_source = (int)SendMessage(video_source_list, CB_GETCURSEL, 0, 0);
	if (video_source != VIDEO_SOURCE_FULLSCREEN) {
		ShowWindow(main_window, SW_HIDE);
		HWND selection_window = CreateWindowEx(WS_EX_LAYERED | WS_EX_TOPMOST, L"STATIC", L"", WS_POPUP | WS_VISIBLE, 0, 0, 0, 0, NULL, NULL, NULL, NULL);
		SetWindowLongPtr(selection_window, GWLP_WNDPROC, (LONG_PTR)selection_proc);
		MSG msg;
		while (GetMessage(&msg, selection_window, 0, 0) > 0) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		ShowWindow(main_window, SW_SHOW);
	}
	if (video_source == VIDEO_SOURCE_FULLSCREEN) {
		source_window = GetDesktopWindow();
		GetWindowRect(source_window, &source_rect);
	}
	update_capture();
}

void encode_proc() {
	double meas_time = recording_start_time;
	double meas_total = 0;
	int meas_count = 0;
	while (true) {
		WaitForSingleObject(frame_event, INFINITE);
		if (!recording_running) {
			break;
		}
		if (recording_paused) {
			continue;
		}
		double frame_start_time = get_time();
		IMFMediaBuffer* buffer;
		int pixel_count = aligned_width * aligned_height;
		int encode_size = pixel_count * 3 / 2;
		MFCreateAlignedMemoryBuffer(encode_size, MF_16_BYTE_ALIGNMENT, &buffer);
		buffer->SetCurrentLength(encode_size);
		BYTE* data;
		buffer->Lock(&data, NULL, NULL);
		AcquireSRWLockShared(&capture_lock);
		switch (video_options.format) {
			case FORMAT_IYUV: {
				if (video_options.color == COLOR_BGR) {
					convert_bgr32_to_iyuv((uint32_t*)capture_buffer, data, data + pixel_count, data + pixel_count * 5 / 4, aligned_width, aligned_height);
				}
				if (video_options.color == COLOR_RGB) {
					convert_rgb32_to_iyuv((uint32_t*)capture_buffer, data, data + pixel_count, data + pixel_count * 5 / 4, aligned_width, aligned_height);
				}
				break;
			}
			case FORMAT_NV12: {
				if (video_options.color == COLOR_BGR) {
					convert_bgr32_to_nv12((uint32_t*)capture_buffer, data, data + pixel_count, aligned_width, aligned_height);
				}
				if (video_options.color == COLOR_RGB) {
					convert_rgb32_to_nv12((uint32_t*)capture_buffer, data, data + pixel_count, aligned_width, aligned_height);
				}
				break;
			}
		}
		ReleaseSRWLockShared(&capture_lock);
		buffer->Unlock();
		IMFSample* sample;
		MFCreateSample(&sample);
		sample->AddBuffer(buffer);
		sample->SetSampleTime(llround((frame_start_time - recording_start_time) * 10000000));
		sample->SetSampleDuration(10000000 / capture_framerate);
		if (resizer != NULL) {
			CHECK(resizer->ProcessInput(0, sample, 0));
			sample->Release();
			buffer->Release();
			DWORD output_size = capture_width * capture_height * 3 / 2;
			MFCreateMemoryBuffer(output_size, &buffer);
			MFT_OUTPUT_DATA_BUFFER output_data = {};
			MFCreateSample(&output_data.pSample);
			output_data.pSample->AddBuffer(buffer);
			DWORD flags;
			CHECK(resizer->ProcessOutput(0, 1, &output_data, &flags));
			sample = output_data.pSample;
		}
		CHECK(sink_writer->WriteSample(video_stream_index, sample));
		sample->Release();
		buffer->Release();
		double frame_end_time = get_time();
		MF_SINK_WRITER_STATISTICS stats = { sizeof(stats) };
		sink_writer->GetStatistics(video_stream_index, &stats);
		recording_time = stats.llLastTimestampProcessed / 10000;
		recording_size = stats.qwByteCountProcessed;
		if (audio_sources & AUDIO_INPUT_SYSTEM) {
			sink_writer->GetStatistics(system_input.stream_index, &stats);
			recording_size += stats.qwByteCountProcessed;
		}
		if (audio_sources & AUDIO_INPUT_MICROPHONE) {
			sink_writer->GetStatistics(microphone_input.stream_index, &stats);
			recording_size += stats.qwByteCountProcessed;
		}
		meas_total += frame_end_time - frame_start_time;
		meas_count++;
		if (frame_start_time > meas_time + meas_interval) {
			meas_encode_fps = meas_count / meas_total;
			meas_time = frame_start_time;
			meas_total = 0;
			meas_count = 0;
		}
	}
}

void audio_proc(Audio_Input* input) {
	if (!input->active || FAILED(input->error)) {
		return;
	}
	input->audio_client->Start();
	while (true) {
		WaitForSingleObject(input->audio_event, INFINITE);
		if (!recording_running) {
			break;
		}
		double frame_time = get_time();
		BYTE* frame_buffer;
		UINT32 frame_count;
		DWORD flags;
		input->capture_client->GetBuffer(&frame_buffer, &frame_count, &flags, NULL, NULL);
		if (recording_paused || frame_count == 0) {
			input->capture_client->ReleaseBuffer(frame_count);
			continue;
		}
		UINT32 buffer_size = frame_count * input->wave_format->nBlockAlign;
		BYTE* data;
		IMFMediaBuffer* buffer;
		MFCreateMemoryBuffer(buffer_size, &buffer);
		buffer->SetCurrentLength(buffer_size);
		buffer->Lock(&data, NULL, NULL);
		memcpy(data, frame_buffer, buffer_size);
		buffer->Unlock();
		input->capture_client->ReleaseBuffer(frame_count);
		IMFSample* sample;
		MFCreateSample(&sample);
		sample->AddBuffer(buffer);
		{
			CHECK(input->resampler->ProcessInput(0, sample, 0));
			sample->Release();
			buffer->Release();
			DWORD output_size = next_multiple(4, input->bytes_per_sec * frame_count / input->wave_format->nSamplesPerSec + 1);
			MFCreateMemoryBuffer(output_size, &buffer);
			MFT_OUTPUT_DATA_BUFFER output_data = {};
			MFCreateSample(&output_data.pSample);
			output_data.pSample->AddBuffer(buffer);
			CHECK(input->resampler->ProcessOutput(0, 1, &output_data, &flags));
			sample = output_data.pSample;
		}
		int64_t duration = 10000000 * frame_count / input->wave_format->nSamplesPerSec;
		sample->SetSampleTime(llround((frame_time - recording_start_time) * 10000000) - duration);
		sample->SetSampleDuration(duration);
		CHECK(sink_writer->WriteSample(input->stream_index, sample));
		sample->Release();
		buffer->Release();
	}
	input->audio_client->Stop();
}

void start_player(Audio_Player* player, Audio_Input* input) {
	if (player->playing) {
		return;
	}
	player->playing = true;
	IMMDeviceEnumerator* device_enumerator;
	CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL, IID_PPV_ARGS(&device_enumerator));
	device_enumerator->GetDefaultAudioEndpoint(eRender, eConsole, &player->device);
	device_enumerator->Release();
	player->device->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr, (void**)&player->audio_client);
	WAVEFORMATEX* wave_format;
	player->audio_client->GetMixFormat(&wave_format);
	player->audio_client->Initialize(AUDCLNT_SHAREMODE_SHARED, AUDCLNT_STREAMFLAGS_EVENTCALLBACK, 0, 0, wave_format, nullptr);
	player->audio_client->GetService(__uuidof(IAudioRenderClient), (void**)&player->render_client);
	player->audio_client->SetEventHandle(input->audio_event);
	UINT32 buffer_size;
	player->audio_client->GetBufferSize(&buffer_size);
	BYTE* data;
	player->render_client->GetBuffer(buffer_size, &data);
	player->render_client->ReleaseBuffer(buffer_size, AUDCLNT_BUFFERFLAGS_SILENT);
	player->audio_client->Start();
}

void stop_player(Audio_Player* player) {
	if (!player->playing) {
		return;
	}
	player->playing = false;
	player->audio_client->Stop();
	player->render_client->Release();
	player->audio_client->Release();
	player->device->Release();
}

void start_audio(Audio_Input* input, EDataFlow source) {
	if (input->active) {
		return;
	}
	input->active = true;
	IMMDeviceEnumerator* device_enumerator;
	CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_ALL, IID_PPV_ARGS(&device_enumerator));
	device_enumerator->GetDefaultAudioEndpoint(source, eConsole, &input->audio_device);
	device_enumerator->Release();
	if (input->audio_device == NULL) {
		if (source == eRender) {
			MessageBox(main_window, L"No output devices!", L"Audio capture warning", MB_ICONWARNING);
		}
		if (source == eCapture) {
			MessageBox(main_window, L"No input devices!", L"Audio capture warning", MB_ICONWARNING);
		}
	reset:
		input->active = false;
		PostMessage(audio_source_list, CB_SETCURSEL, 0, 0);
		PostMessage(main_window, WM_COMMAND, MAKEWPARAM(0, CBN_SELCHANGE), (LPARAM)audio_source_list);
		return;
	}
	input->audio_device->Activate(__uuidof(IAudioClient), CLSCTX_ALL, NULL, (void**)&input->audio_client);
	input->audio_client->GetMixFormat(&input->wave_format);
	DWORD flags = AUDCLNT_STREAMFLAGS_EVENTCALLBACK;
	if (source == eRender) {
		flags |= AUDCLNT_STREAMFLAGS_LOOPBACK;
	}
	HRESULT result = input->audio_client->Initialize(AUDCLNT_SHAREMODE_SHARED, flags, 0, 0, input->wave_format, NULL);
	if (FAILED(result)) {
		MessageBox(main_window, L"Failed to initialize capture!", L"Audio capture warning", MB_ICONWARNING);
		CoTaskMemFree(input->wave_format);
		input->audio_client->Release();
		input->audio_device->Release();
		goto reset;
	}
	input->audio_client->GetService(__uuidof(IAudioCaptureClient), (void**)&input->capture_client);
	input->audio_event = CreateEvent(NULL, FALSE, FALSE, NULL);
	input->audio_client->SetEventHandle(input->audio_event);
	if (source == eRender) {
		start_player(&input->player, input);
	}
}

void stop_audio(Audio_Input* input) {
	if (!input->active) {
		return;
	}
	input->active = false;
	input->capture_client->Release();
	input->audio_client->Release();
	input->audio_device->Release();
	CloseHandle(input->audio_event);
	CoTaskMemFree(input->wave_format);
	stop_player(&input->player);
}

void update_audio() {
	if (audio_sources & AUDIO_INPUT_SYSTEM) {
		stop_audio(&system_input);
	}
	if (audio_sources & AUDIO_INPUT_MICROPHONE) {
		stop_audio(&microphone_input);
	}
	audio_sources = (int)SendMessage(audio_source_list, CB_GETCURSEL, 0, 0);
	if (audio_sources & AUDIO_INPUT_SYSTEM) {
		start_audio(&system_input, eRender);
	}
	if (audio_sources & AUDIO_INPUT_MICROPHONE) {
		start_audio(&microphone_input, eCapture);
	}
}

void add_audio_stream(Audio_Input* input) {
	if (!input->active) {
		return;
	}
	input->error = 0;
	IMFMediaType* input_type;
	MFCreateMediaType(&input_type);
	MFInitMediaTypeFromWaveFormatEx(input_type, input->wave_format, sizeof(*input->wave_format) + input->wave_format->cbSize);
	CoCreateInstance(CLSID_CResamplerMediaObject, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&input->resampler));
	input->error = input->resampler->SetInputType(0, input_type, 0);
	input_type->Release();
	if (FAILED(input->error)) {
		MessageBox(main_window, L"Input format not supported by the resampler!", L"Audio error", MB_ICONERROR);
		return;
	}
	WAVEFORMATEX output_format = {};
	output_format.wFormatTag = WAVE_FORMAT_PCM;
	output_format.nSamplesPerSec = 48000;
	output_format.wBitsPerSample = 16;
	output_format.nChannels = audio_options.channel_count > 0 ? (WORD)audio_options.channel_count : input->wave_format->nChannels;
	output_format.nBlockAlign = output_format.nChannels * output_format.wBitsPerSample / 8;
	output_format.nAvgBytesPerSec = output_format.nSamplesPerSec * output_format.nBlockAlign;
	input->bytes_per_sec = output_format.nAvgBytesPerSec;
	MFCreateMediaType(&input_type);
	MFInitMediaTypeFromWaveFormatEx(input_type, &output_format, sizeof(output_format));
	input->error = input->resampler->SetOutputType(0, input_type, 0);
	if (FAILED(input->error)) {
		MessageBox(main_window, L"Output format not supported by the resampler!", L"Audio error", MB_ICONERROR);
		input_type->Release();
		return;
	}
	IMFMediaType* output_type;
	MFCreateMediaType(&output_type);
	output_type->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio);
	output_type->SetGUID(MF_MT_SUBTYPE, MFAudioFormat_AAC);
	output_type->SetUINT32(MF_MT_AUDIO_BITS_PER_SAMPLE, 16);
	output_type->SetUINT32(MF_MT_AUDIO_AVG_BYTES_PER_SECOND, 24000);
	output_type->SetUINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, output_format.nSamplesPerSec);
	output_type->SetUINT32(MF_MT_AUDIO_NUM_CHANNELS, output_format.nChannels);
	input->error = sink_writer->AddStream(output_type, &input->stream_index);
	if (FAILED(input->error)) {
		MessageBox(main_window, L"Output format not supported by the encoder!", L"Audio error", MB_ICONERROR);
		return;
	}
	input->error = sink_writer->SetInputMediaType(input->stream_index, input_type, NULL);
	if (FAILED(input->error)) {
		MessageBox(main_window, L"Input format not supported by the encoder!", L"Audio error", MB_ICONERROR);
		return;
	}
	output_type->Release();
	input_type->Release();
}

void stop_audio_stream(Audio_Input* input) {
	SetEvent(input->audio_event);
	WaitForSingleObject(input->thread, INFINITE);
	CloseHandle(input->thread);
	input->resampler->Release();
}

void start_recording() {
	if (recording_running) {
		return;
	}
	if (!capture_running) {
		return;
	}
	recording_running = true;
	recording_paused = false;
	recording_time = 0;
	recording_size = 0;
	output_path = L"out.mp4";
	if (settings.hide_at_start) {
		ShowWindow(main_window, SW_MINIMIZE);
	}
	if (settings.hide_at_start && settings.hide_from_taskbar) {
		ShowWindow(main_window, SW_HIDE);
	}
	update_controls();
	InvalidateRect(main_window, NULL, FALSE);
	UpdateWindow(main_window);
	WaitForSingleObject(frame_event, INFINITE);
	WaitForSingleObject(frame_event, INFINITE);
	IMFMediaType* output_type;
	MFCreateMediaType(&output_type);
	output_type->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
	output_type->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_H264);
	output_type->SetUINT32(MF_MT_INTERLACE_MODE, MFVideoInterlace_Progressive);
	output_type->SetUINT32(MF_MT_AVG_BITRATE, capture_bitrate * 1000);
	output_type->SetUINT32(MF_MT_DEFAULT_STRIDE, capture_width);
	MFSetAttributeSize(output_type, MF_MT_FRAME_SIZE, capture_width, capture_height);
	MFSetAttributeRatio(output_type, MF_MT_FRAME_RATE, capture_framerate, 1);
	MFCreateSinkWriterFromURL(output_path, NULL, NULL, &sink_writer);
	GUID format_guid = MFVideoFormat_Base;
	switch (video_options.format) {
		case FORMAT_IYUV: {
			format_guid = MFVideoFormat_IYUV;
			break;
		}
		case FORMAT_NV12: {
			format_guid = MFVideoFormat_NV12;
			break;
		}
	}
	IMFMediaType* input_type;
	MFCreateMediaType(&input_type);
	input_type->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
	input_type->SetGUID(MF_MT_SUBTYPE, format_guid);
	input_type->SetUINT32(MF_MT_INTERLACE_MODE, MFVideoInterlace_Progressive);
	input_type->SetUINT32(MF_MT_DEFAULT_STRIDE, aligned_width);
	MFSetAttributeSize(input_type, MF_MT_FRAME_SIZE, capture_width, capture_height);
	MFSetAttributeRatio(input_type, MF_MT_FRAME_RATE, capture_framerate, 1);
	if (capture_resize) {
		if (video_options.resizer_method == RESIZER_RESIZER_DSP) {
			CoCreateInstance(CLSID_CResizerDMO, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&resizer));
		}
		if (video_options.resizer_method == RESIZER_VIDEO_PROCESSOR_MFT) {
			CoCreateInstance(CLSID_VideoProcessorMFT, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&resizer));
		}
		if (resizer == NULL) {
		resizer_error:
			MessageBox(main_window, L"Failed to initialize resizer!", L"Video error", MB_ICONERROR);
		}
	}
	if (resizer != NULL) {
		IMFMediaType* media_type;
		MFCreateMediaType(&media_type);
		media_type->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
		media_type->SetGUID(MF_MT_SUBTYPE, format_guid);
		media_type->SetUINT32(MF_MT_DEFAULT_STRIDE, aligned_width);
		MFSetAttributeSize(media_type, MF_MT_FRAME_SIZE, aligned_width, aligned_height);
		HRESULT input_error = resizer->SetInputType(0, media_type, 0);
		media_type->SetUINT32(MF_MT_DEFAULT_STRIDE, capture_width);
		MFSetAttributeSize(media_type, MF_MT_FRAME_SIZE, capture_width, capture_height);
		HRESULT output_error = resizer->SetOutputType(0, media_type, 0);
		media_type->Release();
		if (FAILED(input_error) || FAILED(output_error)) {
			resizer->Release();
			resizer = NULL;
			goto resizer_error;
		}
		int src_x = 0;
		int src_y = 0;
		int dst_x = 0;
		int dst_y = 0;
		int src_width = source_width;
		int src_height = source_height;
		int dst_width = capture_width;
		int dst_height = capture_height;
		if (video_options.resize_mode == RESIZE_MODE_LETTERBOX) {
			dst_width = min(dst_width, dst_height * source_width / source_height);
			dst_height = min(dst_height, dst_width * source_height / source_width);
		}
		if (video_options.resize_mode == RESIZE_MODE_CROP) {
			src_width = min(src_width, src_height * capture_width / capture_height);
			src_height = min(src_height, src_width * capture_height / capture_width);
		}
		src_x = (source_width - src_width) / 2;
		src_y = (source_height - src_height) / 2;
		dst_x = (capture_width - dst_width) / 2;
		dst_y = (capture_height - dst_height) / 2;
		if (video_options.resizer_method == RESIZER_RESIZER_DSP) {
			IWMResizerProps* properties;
			resizer->QueryInterface(IID_PPV_ARGS(&properties));
			properties->SetFullCropRegion(src_x, src_y, src_width, src_height, dst_x, dst_y, dst_width, dst_height);
			properties->SetResizerQuality(video_options.quality_resizer);
			properties->Release();
		}
		if (video_options.resizer_method == RESIZER_VIDEO_PROCESSOR_MFT) {
			RECT src_rect = { src_x, src_y, src_x + src_width, src_y + src_height };
			RECT dst_rect = { dst_x, dst_y, dst_x + dst_width, dst_y + dst_height };
			IMFVideoProcessorControl* control;
			resizer->QueryInterface(IID_PPV_ARGS(&control));
			control->SetSourceRectangle(&src_rect);
			control->SetDestinationRectangle(&dst_rect);
			control->Release();
		}
		if (video_options.gpu_resizer) {
			UINT32 gpu_supported = 0;
			IMFAttributes* attributes = NULL;
			resizer->GetAttributes(&attributes);
			if (attributes != NULL) {
				attributes->GetUINT32(MF_SA_D3D_AWARE, &gpu_supported);
				attributes->Release();
			}
			if (gpu_supported == 0) {
				MessageBox(main_window, L"Resizer does not support hardware acceleration!", L"Video warning", MB_ICONWARNING);
			}
			else {
				resizer->ProcessMessage(MFT_MESSAGE_SET_D3D_MANAGER, 1);
			}
		}
		input_type->SetUINT32(MF_MT_DEFAULT_STRIDE, capture_width);
	}
	HRESULT input_error = sink_writer->AddStream(output_type, &video_stream_index);
	HRESULT output_error = sink_writer->SetInputMediaType(video_stream_index, input_type, NULL);
	output_type->Release();
	input_type->Release();
	if (FAILED(input_error) || FAILED(output_error)) {
		MessageBox(main_window, L"Bad format!", L"Video error", MB_ICONERROR);
		sink_writer->Release();
		if (resizer != NULL) {
			resizer->Release();
			resizer = NULL;
		}
		recording_running = false;
		return;
	}
	if (audio_sources & AUDIO_INPUT_SYSTEM) {
		add_audio_stream(&system_input);
	}
	if (audio_sources & AUDIO_INPUT_MICROPHONE) {
		add_audio_stream(&microphone_input);
	}
	if (settings.beep_at_start) {
		Beep(5000, 200);
	}
	sink_writer->BeginWriting();
	recording_start_time = get_time();
	encode_thread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)encode_proc, NULL, 0, NULL);
	if (audio_sources & AUDIO_INPUT_SYSTEM) {
		system_input.thread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)audio_proc, &system_input, 0, NULL);
	}
	if (audio_sources & AUDIO_INPUT_MICROPHONE) {
		microphone_input.thread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)audio_proc, &microphone_input, 0, NULL);
	}
}

void stop_recording() {
	if (!recording_running) {
		return;
	}
	recording_running = false;
	recording_paused = false;
	SetEvent(frame_event);
	WaitForSingleObject(encode_thread, INFINITE);
	CloseHandle(encode_thread);
	if (audio_sources & AUDIO_INPUT_SYSTEM) {
		stop_audio_stream(&system_input);
	}
	if (audio_sources & AUDIO_INPUT_MICROPHONE) {
		stop_audio_stream(&microphone_input);
	}
	sink_writer->Finalize();
	sink_writer->Release();
	if (resizer != NULL) {
		resizer->Release();
		resizer = NULL;
	}
	if (settings.hide_at_start) {
		ShowWindow(main_window, SW_SHOWNORMAL);
	}
	update_controls();
	RedrawWindow(main_window, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW);
	if (settings.beep_at_stop) {
		Beep(2000, 200);
	}
	if (settings.ask_to_play) {
		if (MessageBox(main_window, L"Play the recording now?", L"Recording Finished", MB_YESNO | MB_ICONQUESTION) == IDYES) {
			ShellExecute(NULL, L"open", output_path, NULL, NULL, SW_SHOWNORMAL);
		}
	}
}

void pause_recording() {
	if (!recording_running || recording_paused) {
		return;
	}
	recording_paused = true;
	recording_pause_time = get_time();
	if (settings.beep_at_stop) {
		Beep(2000, 200);
	}
}

void resume_recording() {
	if (!recording_running || !recording_paused) {
		return;
	}
	UpdateWindow(main_window);
	WaitForSingleObject(frame_event, INFINITE);
	if (settings.beep_at_start) {
		Beep(5000, 200);
	}
	recording_start_time += get_time() - recording_pause_time;
	recording_paused = false;
}

void update_source() {
	if (dirty_capture) {
		return;
	}
	if ((bool)D3DKMTCheckExclusiveOwnership() != fullscreen) {
		fullscreen = !fullscreen;
		stop_recording();
		update_capture();
	}
	if (fullscreen) {
		return;
	}
	if (settings.stop_on_close && !IsWindow(source_window) && recording_running) {
		stop_recording();
	}
	if (video_source == VIDEO_SOURCE_WINDOW && !hook_working && IsWindow(source_window) && !IsIconic(source_window) && !recording_running) {
		RECT new_rect;
		GetWindowRect(source_window, &new_rect);
		fit_window_rect(source_window, &new_rect);
		if (memcmp(&source_rect, &new_rect, sizeof(RECT)) != 0) {
			source_rect = new_rect;
			if (capture_resize) {
				update_capture();
			}
			else {
				update_capture();
			}
			update_controls();
		}
		wchar_t source_title[128];
		GetWindowText(video_source_edit, source_title, _countof(source_title));
		wchar_t new_title[128];
		GetWindowText(source_window, new_title, _countof(new_title));
		if (wcslen(new_title) == 0) {
			wcscpy(new_title, L"Unnamed Window");
		}
		if (wcscmp(source_title, new_title) != 0) {
			SetWindowText(video_source_edit, new_title);
		}
	}
	if (GetAsyncKeyState('R') && !recording_running) {
		update_capture();
	}
}

LRESULT CALLBACK dialog_proc(HWND window, UINT message, WPARAM wparam, LPARAM lparam, UINT_PTR, DWORD_PTR) {
	switch (message) {
		case WM_ERASEBKGND: {
			HDC context = (HDC)wparam;
			RECT rect;
			GetClientRect(window, &rect);
			FillRect(context, &rect, white_brush);
			return 1;
		}
		case WM_CTLCOLORSTATIC: {
			return (INT_PTR)white_brush;
		}
		case WM_COMMAND: {
			if (HIWORD(wparam) == CBN_SELENDOK || HIWORD(wparam) == CBN_SELENDCANCEL) {
				SetFocus(window);
			}
			break;
		}
	}
	return DefSubclassProc(window, message, wparam, lparam);
}

HWND create_dialog(int label_width, int control_width, const wchar_t* title, WNDPROC proc) {
	int width = label_width + control_width;
	RECT rect = { 0, 0, width, 10 };
	LONG style = WS_POPUP | WS_CAPTION | WS_SYSMENU;
	AdjustWindowRect(&rect, style, FALSE);
	int adjusted_width = get_rect_width(&rect);
	int adjusted_height = get_rect_height(&rect);
	POINT position = { (main_rect.left + main_rect.right) / 2, (main_rect.top + main_rect.bottom) / 2 };
	ClientToScreen(main_window, &position);
	HWND window = CreateWindow(L"STATIC", title, style, 0, 0, adjusted_width, adjusted_height, main_window, NULL, NULL, NULL);
	SetWindowLongPtr(window, GWLP_USERDATA, MAKELPARAM(label_width, control_width));
	SetWindowLongPtr(window, GWLP_WNDPROC, (LONG_PTR)proc);
	SetWindowSubclass(window, dialog_proc, 0, 0);
	int height = (int)SendMessage(window, WM_USER, 0, 0);
	height += 50;
	SetRect(&rect, 0, 0, width, height);
	CreateWindow(L"BUTTON", L"OK", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON, width - 90, height - 30, 80, 20, window, (HMENU)IDOK, NULL, NULL);
	CreateWindow(L"BUTTON", L"Cancel", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON, width - 180, height - 30, 80, 20, window, (HMENU)IDCANCEL, NULL, NULL);
	AdjustWindowRect(&rect, style, FALSE);
	width = get_rect_width(&rect);
	height = get_rect_height(&rect);
	SetWindowPos(window, NULL, position.x - width / 2, position.y - height / 2, width, height, SWP_NOZORDER);
	EnumChildWindows(window, set_font_proc, (LPARAM)main_font);
	ShowWindow(window, SW_SHOWNORMAL);
	RedrawWindow(window, NULL, NULL, RDW_INVALIDATE | RDW_ERASENOW | RDW_UPDATENOW);
	return window;
} 

HWND add_form(HWND window, int* row, const wchar_t* type, const wchar_t* label, void* value, ...) {
	int margin = 10;
	HWND control = NULL;
	RECT rect;
	GetClientRect(window, &rect);
	if (*row == 0) {
		*row = 5;
	}
	if (type == NULL) {
		*row += 6;
		control = CreateWindow(L"STATIC", NULL, WS_VISIBLE | WS_CHILD | SS_ETCHEDHORZ, margin, *row, rect.right - margin * 2, 1, window, NULL, NULL, NULL);
		*row += 7;
	}
	else {
		int height = 27;
		int spacing = 3;
		int item_height = height - spacing * 2;
		LPARAM widths = GetWindowLongPtr(window, GWLP_USERDATA);
		int label_width = LOWORD(widths) - margin * 2;
		int control_width = HIWORD(widths)- margin * 2;
		CreateWindow(L"STATIC", label, WS_VISIBLE | WS_CHILD | SS_CENTERIMAGE, margin, *row, label_width, height, window, NULL, NULL, NULL);
		const wchar_t* control_class = NULL;
		LONG control_style = 0;
		SUBCLASSPROC control_proc = NULL;
		if (_wcsicmp(type, L"checkbox") == 0) {
			control_class = L"BUTTON";
			control_style =  BS_AUTOCHECKBOX | BS_LEFTTEXT | BS_VCENTER;
			control_proc = form_checkbox_proc;
		}
		if (_wcsicmp(type, L"hotkey") == 0) {
			control_class = L"EDIT";
			control_style =  WS_BORDER | ES_RIGHT;
			control_proc = form_hotkey_proc;
		}
		if (_wcsicmp(type, L"list") == 0) {
			control_class = L"COMBOBOX";
			control_style =  CBS_DROPDOWNLIST | CBS_HASSTRINGS;
			control_proc = form_list_proc;
			item_height = 500;
		}
		control = CreateWindow(control_class, L"", WS_VISIBLE | WS_CHILD | control_style, rect.left + label_width + margin * 3, *row + spacing, control_width, item_height, window, NULL, NULL, NULL);
		if (_wcsicmp(type, L"list") == 0) {
			va_list args;
			va_start(args, value);
			while (true) {
				wchar_t* item = va_arg(args, wchar_t*);
				if (item == NULL) {
					break;
				}
				SendMessage(control, CB_ADDSTRING, 0, (LPARAM)item);
			}
			va_end(args);
		}
		SetWindowLongPtr(control, GWLP_USERDATA, (LONG_PTR)value);
		SetWindowSubclass(control, control_proc, 0, 0);
		SendMessage(control, WM_USER, 0, 0);
		*row += height;
	}
	SendMessage(control, WM_SETFONT, (WPARAM)main_font, 0);
	return control;
}

void update_settings() {
	SetWindowPos(main_window, settings.stay_on_top ? HWND_TOPMOST : HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
	SetStretchBltMode(render_context, settings.quality_preview ? HALFTONE : COLORONCOLOR);
	save_data(L"Settings", &settings, sizeof(settings));
}

void init_settings() {
	settings.stay_on_top = false;
	settings.hide_at_start = true;
	settings.hide_from_taskbar = false;
	settings.beep_at_start = false;
	settings.beep_at_stop = false;
	settings.ask_to_play = true;
	load_data(L"Settings", &settings, sizeof(settings));
	update_settings();
}

void init_config() {
	load_data(L"framerate", &capture_framerate, sizeof(capture_framerate));
	load_data(L"bitrate", &capture_bitrate, sizeof(capture_bitrate));
	load_data(L"audio_mode", &audio_sources, sizeof(audio_sources));
	PostMessage(audio_source_list, CB_SETCURSEL, (WPARAM)audio_sources, 0);
	PostMessage(main_window, WM_COMMAND, MAKEWPARAM(0, CBN_SELCHANGE), (LPARAM)audio_source_list);
}

void save_config() {
	save_data(L"framerate", &capture_framerate, sizeof(capture_framerate));
	save_data(L"bitrate", &capture_bitrate, sizeof(capture_bitrate));
	save_data(L"audio_mode", &audio_sources, sizeof(audio_sources));
}

LRESULT CALLBACK settings_proc(HWND window, UINT message, WPARAM wparam, LPARAM lparam) {
	static Settings backup;
	switch (message) {
		case WM_USER: {
			settings_window = window;
			backup = settings;
			int row = 0;
			add_form(window, &row, L"checkbox", L"Stay on top", &settings.stay_on_top);
			add_form(window, &row, L"checkbox", L"Minimize before starting recording", &settings.hide_at_start);
			add_form(window, &row, L"checkbox", L"Hide from taskbar while recording", &settings.hide_from_taskbar);
			add_form(window, &row, NULL, NULL, NULL);
			add_form(window, &row, L"checkbox", L"Stop recording when the window closes", &settings.stop_on_close);
			add_form(window, &row, NULL, NULL, NULL);
			add_form(window, &row, L"checkbox", L"Ask to play recording when finished", &settings.ask_to_play);
			add_form(window, &row, NULL, NULL, NULL);
			add_form(window, &row, L"checkbox", L"High quality preview", &settings.quality_preview);
			add_form(window, &row, L"checkbox", L"Disable preview while recording", &settings.disable_preview);
			add_form(window, &row, NULL, NULL, NULL);
			add_form(window, &row, L"checkbox", L"Beep before recording starts", &settings.beep_at_start);
			add_form(window, &row, L"checkbox", L"Beep after recording stops", &settings.beep_at_stop);
			add_form(window, &row, NULL, NULL, NULL);
			add_form(window, &row, L"hotkey", L"Start recording", &settings.start_hotkey);
			add_form(window, &row, L"hotkey", L"Stop recording", &settings.stop_hotkey);
			add_form(window, &row, L"hotkey", L"Pause recording", &settings.pause_hotkey);
			add_form(window, &row, L"hotkey", L"Resume recording", &settings.resume_hotkey);
			add_form(window, &row, L"hotkey", L"Refresh", &settings.refresh_hotkey);
			return row;
		}
		case WM_DESTROY: {
			settings_window = NULL;
			update_settings();
			update_controls();
			break;
		}
		case WM_COMMAND: {
			if (LOWORD(wparam) == IDCANCEL) {
				settings = backup;
			}
			if (LOWORD(wparam) != 0) {
				DestroyWindow(window);
			}
			else if (SendMessage((HWND)lparam, WM_USER, 1, 0)) {
				update_settings();
			}
			break;
		}
	}
	return DefWindowProc(window, message, wparam, lparam);
}

LRESULT CALLBACK video_options_proc(HWND window, UINT message, WPARAM wparam, LPARAM lparam) {
	static Video_Options backup;
	static Video_Options previous;
	switch (message) {
		case WM_USER: {
			video_window = window;
			backup = video_options;
			previous = video_options;
			int row = 0;
			const wchar_t* capture_names[CAPTURE_COUNT];
			capture_names[CAPTURE_DEFAULT] = L"Auto";
			capture_names[CAPTURE_BITBLT_GETDIBITS] = L"BitBlt + GetDIBits";
			capture_names[CAPTURE_BITBLT_GETBITMAPBITS] = L"BitBlt + GetBitmapBits";
			capture_names[CAPTURE_CAPTUREBLT_GETDIBITS] = L"CaptureBlt + GetDIBits";
			capture_names[CAPTURE_CAPTUREBLT_GETBITMAPBITS] = L"CaptureBlt + GetBitmapBits";
			capture_names[CAPTURE_PRINTWINDOW_GETDIBITS] = L"PrintWindow + GetDIBits";
			capture_names[CAPTURE_PRINTWINDOW_GETBITMAPBITS] = L"PrintWindow + GetBitmapBits";
			capture_names[CAPTURE_DWM_PRINTWINDOW] = L"DWM PrintWindow";
			capture_names[CAPTURE_GETDIBITS] = L"GetDIBits";
			capture_names[CAPTURE_GETBITMAPBITS] = L"GetBitmapBits";
			capture_names[CAPTURE_DIRECT3D_9_GETFRONTBUFFERDATA] = L"GetFrontBufferData";
			capture_names[CAPTURE_DIRECT3D_DWMGETDXSHAREDSURFACE] = L"DwmGetDxSharedSurface";
			capture_names[CAPTURE_DIRECT3D_DWMDXGETWINDOWSHAREDSURFACE] = L"DwmDxGetWindowSharedSurface";
			capture_names[CAPTURE_DXGI_OUTPUT_DUPLICATION] = L"DXGI Output Duplication";
			add_form(window, &row, L"list", L"Capture method", &video_options.capture_method, capture_names[0], capture_names[1], capture_names[2], capture_names[3], capture_names[4], capture_names[5], capture_names[6], capture_names[7], capture_names[8], capture_names[9], capture_names[10], capture_names[11], capture_names[12], capture_names[13], NULL);
			add_form(window, &row, L"list", L"Window area", &video_options.source_type, L"Entire Window", L"Visible Area", L"Client Area", NULL);
			add_form(window, &row, L"checkbox", L"Use capture hook", &video_options.use_hook);
			add_form(window, &row, NULL, NULL, NULL);
			add_form(window, &row, L"checkbox", L"Show mouse cursor", &video_options.draw_cursor);
			add_form(window, &row, L"checkbox", L"Draw cursor when it is invisible", &video_options.always_cursor);
			add_form(window, &row, NULL, NULL, NULL);
			add_form(window, &row, L"list", L"Resize mode", &video_options.resize_mode, L"Stretch", L"Letterbox", L"Crop", NULL);
			add_form(window, &row, L"list", L"Resizer method", &video_options.resizer_method, L"Video Resizer DSP", L"Video Processor MFT", NULL);
			add_form(window, &row, L"checkbox", L"High quality resize", &video_options.quality_resizer);
			add_form(window, &row, L"checkbox", L"Hardware accelerated resize", &video_options.gpu_resizer);
			add_form(window, &row, NULL, NULL, NULL);
			add_form(window, &row, L"list", L"Color format", &video_options.color, L"Default - BRG", L"Inverted - RGB", NULL);
			add_form(window, &row, L"list", L"Encode format", &video_options.format, L"IYUV", L"NV12", NULL);
			return row;
		}
		case WM_DESTROY: {
			video_window = NULL;
			update_controls();
			break;
		}
		case WM_COMMAND: {
			if (LOWORD(wparam) == IDCANCEL) {
				video_options = backup;
			}
			if (LOWORD(wparam) != 0) {
				DestroyWindow(window);
			}
			else {
				SendMessage((HWND)lparam, WM_USER, 1, 0);
			}
			if (memcmp(&video_options, &previous, sizeof(Video_Options)) != 0) {
				previous = video_options;
				save_data(L"video_options", &video_options, sizeof(video_options));
				update_capture();
			}
			break;
		}
	}
	return DefWindowProc(window, message, wparam, lparam);
}

LRESULT CALLBACK audio_options_proc(HWND window, UINT message, WPARAM wparam, LPARAM lparam) {
	static Audio_Options backup;
	switch (message) {
		case WM_USER: {
			audio_window = window;
			backup = audio_options;
			int row = 0;
			add_form(window, &row, L"list", L"Number of channels", &audio_options.channel_count, L"Same as input", L"1 (mono)", L"2 (stereo)", NULL);
			return row;
		}
		case WM_DESTROY: {
			audio_window = NULL;
			if (memcmp(&audio_options, &backup, sizeof(Audio_Options)) != 0) {
				save_data(L"audio_options", &audio_options, sizeof(audio_options));
				update_capture();
			}
			update_controls();
			break;
		}
		case WM_COMMAND: {
			if (LOWORD(wparam) == IDCANCEL) {
				audio_options = backup;
			}
			if (LOWORD(wparam) != 0) {
				DestroyWindow(window);
			}
			else {
				SendMessage((HWND)lparam, WM_USER, 1, 0);
			}
			break;
		}
	}
	return DefWindowProc(window, message, wparam, lparam);
}

void draw_preview() {
	if (dirty_capture || !capture_running) {
		return;
	}
	RECT rect = { 3, 66, main_rect.right - 3, main_rect.bottom - 81 };
	if (get_rect_width(&rect) < 10 || get_rect_height(&rect) < 10) {
		return;
	}
	int dst_width = get_rect_width(&rect);
	int dst_height = get_rect_height(&rect);
	dst_width = min(dst_width, dst_height * capture_width / capture_height);
	dst_height = min(dst_height, dst_width * capture_height / capture_width);
	if (capture_resize) {
		dst_width = min(capture_width, dst_width);
		dst_height = min(capture_height, dst_height);
	}
	int middle_x = (rect.left + rect.right) / 2;
	int middle_y = (rect.top + rect.bottom) / 2;
	rect.left = middle_x - dst_width / 2;
	rect.top = middle_y - (dst_height + 1) / 2;
	rect.right = rect.left + dst_width;
	rect.bottom = rect.top + dst_height;
	if (recording_running && settings.disable_preview) {
		FillRect(render_context, &rect, black_brush);
		SetTextColor(render_context, RGB(255, 255, 255));
		SelectObject(render_context, display_font);
		DrawText(render_context, L"Preview Disabled", -1, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
	}
	else {
		int src_width = source_width;
		int src_height = source_height;
		if (capture_resize) {
			if (video_options.resize_mode == RESIZE_MODE_LETTERBOX) {
				FillRect(render_context, &rect, black_brush);
				dst_width = min(dst_width, dst_height * source_width / source_height);
				dst_height = min(dst_height, dst_width * source_height / source_width);
			}
			if (video_options.resize_mode == RESIZE_MODE_CROP) {
				src_width = min(src_width, src_height * dst_width / dst_height);
				src_height = min(src_height, src_width * dst_height / dst_width);
			}
		}
		int src_x = middle_x - dst_width / 2;
		int src_y = middle_y - (dst_height + 1) / 2;
		int dst_x = (source_width - src_width) / 2;
		int dst_y = (source_height - src_height) / 2;
		if (video_options.color == COLOR_BGR) {
			AcquireSRWLockShared(&capture_lock);
			StretchDIBits(render_context, src_x, src_y, dst_width, dst_height, dst_x, dst_y, src_width, src_height, capture_buffer, (const BITMAPINFO*)&capture_info, DIB_RGB_COLORS, SRCCOPY);
			ReleaseSRWLockShared(&capture_lock);
		}
		if (video_options.color == COLOR_RGB) {
			static void* rgb_buffer;
			static int rgb_width;
			static int rgb_height;
			if (rgb_width != aligned_width || rgb_height != aligned_height) {
				if (rgb_buffer != NULL) {
					_aligned_free(rgb_buffer);
				}
				rgb_buffer = _aligned_malloc(capture_info.biSizeImage, 16);
				rgb_width = aligned_width;
				rgb_height = aligned_height;
			}
			AcquireSRWLockShared(&capture_lock);
			convert_bgr_to_rgb((uint32_t*)capture_buffer, (uint32_t*)rgb_buffer, aligned_width, aligned_height);
			ReleaseSRWLockShared(&capture_lock);
			StretchDIBits(render_context, src_x, src_y, dst_width, dst_height, dst_x, dst_y, src_width, src_height, rgb_buffer, (const BITMAPINFO*)&capture_info, DIB_RGB_COLORS, SRCCOPY);
		}
	}
}

void draw_fps() {
	RECT rect = { main_rect.right - 100, 1, main_rect.right, 65 };
	Rectangle(render_context, rect.left, rect.top, rect.right, rect.bottom);
	double encode_fps = recording_running && !recording_paused && meas_encode_fps > 0 ? meas_encode_fps : INFINITY;
	wchar_t buffer[12];
	swprintf(buffer, L"%3i", lround(fmin(meas_capture_fps, encode_fps)));
	SelectObject(render_context, fps_font);
	SetTextColor(render_context, meas_capture_fps <= encode_fps ? RGB(0, 200, 0) : RGB(200, 0, 0));
	DrawText(render_context, buffer, -1, &rect, DT_NOCLIP | DT_SINGLELINE | DT_CENTER | DT_VCENTER);
}

LRESULT CALLBACK keyboard_hook_proc(int code, WPARAM wparam, LPARAM lparam) {
	if (code == HC_ACTION) {
		PKBDLLHOOKSTRUCT keyboard = (PKBDLLHOOKSTRUCT)lparam;
		if (wparam == WM_KEYDOWN && settings_window == NULL) {
			wchar_t key = (wchar_t)MapVirtualKey(keyboard->vkCode, MAPVK_VK_TO_CHAR);
			if (iswalnum(key)) {
				Hotkey hotkey = {};
				hotkey.key = key;
				hotkey.control = GetKeyState(VK_CONTROL) & 0x8000;
				hotkey.shift = GetKeyState(VK_SHIFT) & 0x8000;
				hotkey.alt = GetKeyState(VK_MENU) & 0x8000;
				if (memcmp(&hotkey, &settings.start_hotkey, sizeof(Hotkey)) == 0) {
					start_recording();
					return 1;
				}
				if (memcmp(&hotkey, &settings.stop_hotkey, sizeof(Hotkey)) == 0) {
					stop_recording();
					return 1;
				}
				if (memcmp(&hotkey, &settings.pause_hotkey, sizeof(Hotkey)) == 0) {
					pause_recording();
					return 1;
				}
				if (memcmp(&hotkey, &settings.resume_hotkey, sizeof(Hotkey)) == 0) {
					resume_recording();
					return 1;
				}
				if (memcmp(&hotkey, &settings.refresh_hotkey, sizeof(Hotkey)) == 0) {
					update_capture();
					return 1;
				}
			}
		}
	}
	return CallNextHookEx(NULL, code, wparam, lparam);
}

LRESULT CALLBACK window_proc(HWND window, UINT message, WPARAM wparam, LPARAM lparam) {
	switch (message) {
		case WM_CREATE: {
			main_instance = GetModuleHandle(NULL);
			main_window = window;
			main_context = GetDC(window);
			render_context = CreateCompatibleDC(main_context);
			render_bitmap = CreateCompatibleBitmap(main_context, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN));
			SelectObject(render_context, render_bitmap);
			SetBkMode(render_context, TRANSPARENT);
			load_data(L"video_options", &video_options, sizeof(video_options));
			load_data(L"audio_options", &audio_options, sizeof(audio_options));
			load_images();
			create_controls();
			place_controls();
			select_source();
			update_audio();
			update_controls();
			SetWindowsHookEx(WH_KEYBOARD_LL, keyboard_hook_proc, main_instance, 0);
			break;
		}
		case WM_SIZE: {
			GetClientRect(window, &main_rect);
			place_controls();
			break;
		}
		case WM_DESTROY: {
			save_config();
			PostQuitMessage(0);
			break;
		}
		case WM_GETMINMAXINFO: {
			RECT min_rect = { 0, 0, 890, 145 };
			AdjustWindowRect(&min_rect, GetWindowLong(window, GWL_STYLE), FALSE);
			MINMAXINFO* min_max_info = (MINMAXINFO*)lparam;
			min_max_info->ptMinTrackSize.x = get_rect_width(&min_rect);
			min_max_info->ptMinTrackSize.y = get_rect_height(&min_rect);
			return 0;
		}
		case WM_TIMER: {
			update_source();
			break;
		}
		case WM_PAINT: {
			EnumChildWindows(main_window, custom_draw_proc, NULL);
			SelectObject(render_context, outline_pen);
			SelectObject(render_context, background_brush);
			Rectangle(render_context, 1, 1, main_rect.right, main_rect.bottom);
			SelectObject(render_context, white_brush);
			Rectangle(render_context, 1, 1, 790, 65);
			Rectangle(render_context, 1, 1, 610, 65);
			Rectangle(render_context, 1, 1, 370, 65);
			Rectangle(render_context, 1, main_rect.bottom - 80, 395, main_rect.bottom);
			Rectangle(render_context, 1, main_rect.bottom - 80, 210, main_rect.bottom);
			Rectangle(render_context, main_rect.right - 245, main_rect.bottom - 80, main_rect.right, main_rect.bottom);
			draw_fps();
			draw_preview();
			BitBlt(main_context, 0, 0, main_rect.right, main_rect.bottom, render_context, 0, 0, SRCCOPY);
			break;
		}
		case WM_DRAWITEM: {
			LPDRAWITEMSTRUCT draw_struct = (LPDRAWITEMSTRUCT)lparam;
			HDC draw_context = (HDC)GetWindowLongPtr(draw_struct->hwndItem, GWLP_USERDATA);
			if (draw_context == NULL) {
				draw_context = CreateCompatibleDC(draw_struct->hDC);
				SetWindowLongPtr(draw_struct->hwndItem, GWLP_USERDATA, (LONG_PTR)draw_context);
				HBITMAP draw_bitmap = CreateCompatibleBitmap(draw_struct->hDC, draw_struct->rcItem.right, draw_struct->rcItem.bottom);
				SelectObject(draw_context, draw_bitmap);
			}
			if (draw_struct->CtlType == ODT_BUTTON) {
				HBITMAP button_image = NULL;
				if (draw_struct->hwndItem == start_button) {
					button_image = start_image;
				}
				if (draw_struct->hwndItem == stop_button) {
					button_image = stop_image;
				}
				if (draw_struct->hwndItem == pause_button) {
					button_image = recording_paused ? resume_image : pause_image;
				}
				if (draw_struct->hwndItem == settings_button) {
					button_image = settings_image;
				}
				BITMAP image_bitmap;
				GetObject(button_image, sizeof(image_bitmap), &image_bitmap);
				HDC image_context = CreateCompatibleDC(draw_context);
				SelectObject(image_context, button_image);
				BLENDFUNCTION blend_func = {};
				blend_func.SourceConstantAlpha = 255;
				if (draw_struct->itemState & ODS_SELECTED) {
					blend_func.SourceConstantAlpha = 200;
				}
				if (draw_struct->itemState & ODS_DISABLED) {
					blend_func.SourceConstantAlpha = 100;
				}
				FillRect(draw_context, &draw_struct->rcItem, black_brush);
				AlphaBlend(draw_context, 0, 0, draw_struct->rcItem.right, draw_struct->rcItem.bottom, image_context, 0, 0, image_bitmap.bmWidth, image_bitmap.bmHeight, blend_func);
				DeleteDC(image_context);
			}
			if (draw_struct->CtlType == ODT_STATIC) {
				wchar_t text[128] = L"";
				int format = DT_NOCLIP | DT_VCENTER | DT_SINGLELINE;
				if (draw_struct->hwndItem == time_label) {
					swprintf(text, L"%lli:%02lli:%02lli.%03lli", recording_time / 3600000, recording_time / 60000 % 60, recording_time / 1000 % 60, recording_time % 1000);
					format |= DT_RIGHT;
				}
				if (draw_struct->hwndItem == size_label) {
					if (recording_size < 1e6) {
						swprintf(text, L"%.2f KB", recording_size / 1e3);
					}
					else if (recording_size < 1e9) {
						swprintf(text, L"%.2f MB", recording_size / 1e6);
					}
					else if (recording_size < 1e12) {
						swprintf(text, L"%.2f GB", recording_size / 1e9);
					}
					else {
						swprintf(text, L"%.2f TB", recording_size / 1e12);
					}
					format |= DT_RIGHT;
				}
				static HANDLE process = GetCurrentProcess();
				static uint64_t total_time;
				static uint64_t process_time;
				static double meas_cpu_time = 0;
				static double meas_mem_time = 0;
				static uint64_t meas_cpu = 0;
				static uint64_t meas_mem = 0;
				double current_time = get_time();
				if (current_time > meas_cpu_time + 0.7) {
					uint64_t total_times[3];
					GetSystemTimes((FILETIME*)&total_times[0], (FILETIME*)&total_times[1], (FILETIME*)&total_times[2]);
					uint64_t process_times[4];
					GetProcessTimes(process, (FILETIME*)&process_times[0], (FILETIME*)&process_times[1], (FILETIME*)&process_times[2], (FILETIME*)&process_times[3]);
					uint64_t current_total_time = total_times[1] + total_times[2];
					uint64_t current_process_time = process_times[2] + process_times[3];
					meas_cpu = 100 * (current_process_time - process_time) / (current_total_time - total_time);
					total_time = current_total_time;
					process_time = current_process_time;
					meas_cpu_time = current_time;
				}
				if (current_time > meas_mem_time + 0.25) {
					PROCESS_MEMORY_COUNTERS process_memory;
					GetProcessMemoryInfo(process, &process_memory, sizeof(process_memory));
					meas_mem = process_memory.WorkingSetSize / 1000000;
					meas_mem_time = current_time;
				}
				if (draw_struct->hwndItem == cpu_label) {
					swprintf(text, L"CPU: %2llu %%", meas_cpu);
				}
				if (draw_struct->hwndItem == mem_label) {
					swprintf(text, L"MEM: %3llu MB", meas_mem);
				}
				SelectObject(draw_context, display_font);
				SetBkColor(draw_context, RGB(255, 255, 255));
				SetTextColor(draw_context, RGB(0, 0, 0));
				FillRect(draw_context, &draw_struct->rcItem, white_brush);
				DrawText(draw_context, text, -1, &draw_struct->rcItem, format);
			}
			BitBlt(draw_struct->hDC, 0, 0, draw_struct->rcItem.right, draw_struct->rcItem.bottom, draw_context, 0, 0, SRCCOPY);
			return 1;
		}
		case WM_ERASEBKGND: {
			return 1;
		}
		case WM_CTLCOLORSTATIC: {
			return (LRESULT)white_brush;
		}
		case WM_COMMAND: {
			if (lock_controls) {
				break;
			}
			HWND child = (HWND)lparam;
			switch (HIWORD(wparam)) {
				case CBN_SELCHANGE: {
					if (child == video_source_list) {
						select_source();
					}
					if (child == audio_source_list) {
						update_audio();
					}
					break;
				}
				case CBN_SELENDOK: {
					SetFocus(window);
					break;
				}
				case CBN_SELENDCANCEL: {
					SetFocus(window);
					break;
				}
				case BN_CLICKED: {
					if (child == resize_checkbox) {
						update_capture();
					}
					if (child == start_button) {
						start_recording();
					}
					if (child == stop_button) {
						stop_recording();
					}
					if (child == pause_button) {
						if (recording_paused) {
							resume_recording();
						}
						else {
							pause_recording();
						}
					}
					if (child == settings_button) {
						create_dialog(240, 120, L"Settings", settings_proc);
					}
					if (child == video_button) {
						if (video_window == NULL) {
							create_dialog(200, 200, L"Video Options", video_options_proc);
						}
						else {
							SetForegroundWindow(video_window);
						}
					}
					if (child == audio_button) {
						if (audio_window == NULL) {
							create_dialog(150, 150, L"Audio Options", audio_options_proc);
						}
						else {
							SetForegroundWindow(audio_window);
						}
					}
					break;
				}
				case EN_UPDATE: {
					wchar_t buffer[12];
					GetWindowText(child, buffer, _countof(buffer));
					if (child == width_edit) {
						capture_width = max(1, _wtoi(buffer));
					}
					if (child == height_edit) {
						capture_height = max(1, _wtoi(buffer));
					}
					if (child == framerate_edit) {
						capture_framerate = max(1, _wtoi(buffer));
					}
					if (child == bitrate_edit) {
						capture_bitrate = max(1,  _wtoi(buffer));
					}
					if (SendMessage(keep_ratio_checkbox, BM_GETCHECK, 0, 0) != BST_UNCHECKED) {
						if (child == width_edit) {
							capture_height = max(1, capture_width * source_height / source_width);
						}
						if (child == height_edit) {
							capture_width = max(1, capture_height * source_width / source_height);
						}
					}
					if (child == width_edit || child == height_edit || child == framerate_edit) {
						update_capture();
					}
					break;
				}
			}
			update_controls();
			break;
		}
	}
	return DefWindowProc(window, message, wparam, lparam);
}

int real_main() {
	main_font = CreateFont(16, 0, 0, 0, FW_REGULAR, FALSE, FALSE, FALSE, 0, 0, 0, 0, 0, L"Segoe UI");
	fps_font = CreateFont(60, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, 0, 0, 0, 0, 0, L"Consolas");
	display_font = CreateFont(25, 0, 0, 0, FW_REGULAR, FALSE, FALSE, FALSE, 0, 0, 0, 0, 0, L"Verdana");
	white_brush = CreateSolidBrush(RGB(255, 255, 255));
	black_brush = CreateSolidBrush(RGB(0, 0, 0));
	background_brush = CreateSolidBrush(RGB(220, 220, 220));
	outline_pen = CreatePen(PS_SOLID, 2, RGB(0, 0, 0));
	InitializeSRWLock(&capture_lock);
	capture_framerate = 60;
	capture_bitrate = 16000;
	meas_interval = 0.5;
	capture_size_max = 100000000;
	resample_audio = true;
	int timer_resolution = 1;
	timeBeginPeriod(timer_resolution);
	CoInitialize(NULL);
	MFStartup(MF_VERSION);
	WNDCLASS wc = {};
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.lpfnWndProc = window_proc;
	wc.lpszClassName = L"Scabture Window Class";
	wc.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
	RegisterClass(&wc);
	CreateWindow(wc.lpszClassName, L"Scabinic", WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, NULL, NULL);
	init_settings();
	init_config();
	SetTimer(main_window, 1, 100, NULL);
	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0) > 0) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	MFShutdown();
	CoUninitialize();
	timeEndPeriod(timer_resolution);
	return 0;
}

int main() {
	real_main();
	return 0;
}

/*

D3DKMTOutputDupl, D3DKMTCreateDCFromMemory (replace window's dc, bypass GetDIBits, get subrect quickly...)
dwmapi captures (win10): DwmpBeginDisplayCapture, DwmpBeginWindowCapture, ...

*/
