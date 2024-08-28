#include "main.h"

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
	bool better_preview;
	bool disable_preview;
};

BITMAPINFOHEADER capture_info;
bool capture_running;
bool capture_stretch;
bool lock_controls;
bool recording_paused;
bool recording_running;
const wchar_t* output_path;
double meas_capture_fps;
double meas_encode_fps;
double meas_interval;
double recording_pause_time;
double recording_start_time;
DWORD stream_index;
HANDLE capture_thread;
HANDLE encode_thread;
HANDLE frame_event;
HANDLE stop_event;
HBITMAP capture_bitmap;
HBITMAP pause_image;
HBITMAP render_bitmap;
HBITMAP resume_image;
HBITMAP settings_image;
HBITMAP start_image;
HBITMAP stop_image;
HBRUSH background_brush;
HBRUSH black_brush;
HBRUSH white_brush;
HDC capture_context;
HDC main_context;
HDC render_context;
HDC source_context;
HFONT display_font;
HFONT fps_font;
HFONT main_font;
HMODULE main_instance;
HPEN outline_pen;
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
HWND settings_button;
HWND settings_window;
HWND size_label;
HWND source_edit;
HWND source_label;
HWND source_list;
HWND source_window;
HWND start_button;
HWND stop_button;
HWND stretch_checkbox;
HWND time_label;
HWND width_edit;
HWND width_label;
IMFSinkWriter* sink_writer;
int capture_bitrate;
int capture_framerate;
int capture_height;
int capture_width;
int source_height;
int source_width;
int source_mode;
int64_t recording_size;
int64_t recording_time;
RECT main_rect;
RECT source_rect;
Settings settings;
size_t capture_size_max;
SRWLOCK capture_lock;
void* capture_buffer;

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
		state = source_mode;
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
			DestroyWindow(window);
			break;
		}
		case WM_MOUSEMOVE: {
			if (state == 2) {
				int mouse_x = LOWORD(lparam);
				int mouse_y = HIWORD(lparam);
				if (source_mode == 1) {
					selected_rect.left = min(start_x, mouse_x);
					selected_rect.top = min(start_y, mouse_y);
					selected_rect.right = max(start_x, mouse_x);
					selected_rect.bottom = max(start_y, mouse_y);
				}
				if (source_mode == 2) {
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
				if (source_mode == 1) {
					source_window = GetDesktopWindow();
					source_rect = selected_rect;
				}
				if (source_mode == 2) {
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

LRESULT CALLBACK source_list_proc(HWND window, UINT message, WPARAM wparam, LPARAM lparam) {
	if (message == WM_PAINT) {
		PostMessage(source_edit, EM_SETSEL, (WPARAM)-1, 0);
	}
	return DefSubclassProc(window, message, wparam, lparam);
}

LRESULT CALLBACK edit_proc(HWND window, UINT message, WPARAM wparam, LPARAM lparam) {
	if (message == WM_CHAR && wparam == VK_RETURN) {
		return 0;
	}
	return DefSubclassProc(window, message, wparam, lparam);
}

LRESULT CALLBACK button_proc(HWND window, UINT message, WPARAM wparam, LPARAM lparam) {
	if (message == WM_ERASEBKGND) {
		return 1;
	}
	return DefSubclassProc(window, message, wparam, lparam);
}

LRESULT CALLBACK form_checkbox_proc(HWND window, UINT message, WPARAM wparam, LPARAM lparam) {
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

LRESULT CALLBACK form_hotkey_proc(HWND window, UINT message, WPARAM wparam, LPARAM lparam) {
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

BOOL CALLBACK set_font_proc(HWND child, LPARAM lparam) {
	if (SendMessage(child, WM_GETFONT, 0, 0) == NULL) {
		SendMessage(child, WM_SETFONT, (WPARAM)lparam, 0);
	}
	return TRUE;
}

BOOL CALLBACK custom_draw_proc(HWND child, LPARAM lparam) {
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

void capture_proc() {
	HANDLE timer = CreateWaitableTimer(NULL, TRUE, NULL);
	LARGE_INTEGER due_time;
	GetSystemTimeAsFileTime((FILETIME*)&due_time);
	SetWaitableTimer(timer, &due_time, 0, NULL, NULL, FALSE);
	double meas_time = get_time();
	int meas_count = 0;
	HANDLE events[] = { stop_event, timer };
	while (WaitForMultipleObjects(_countof(events), events, FALSE, INFINITE) != 0) {
		due_time.QuadPart = due_time.QuadPart + 10000000 / capture_framerate;
		SetWaitableTimer(timer, &due_time, 0, NULL, NULL, FALSE);
		double current_time = get_time();
		BitBlt(capture_context, 0, 0, source_width, source_height, source_context, source_rect.left, source_rect.top, SRCCOPY);
		AcquireSRWLockExclusive(&capture_lock);
		GetDIBits(capture_context, capture_bitmap, 0, source_height, capture_buffer, (LPBITMAPINFO)&capture_info, DIB_RGB_COLORS);
		ReleaseSRWLockExclusive(&capture_lock);
		SetEvent(frame_event);
		RedrawWindow(main_window, NULL, NULL, RDW_INVALIDATE);
		meas_count++;
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
	if (capture_running) {
		return;
	}
	capture_running = true;
	capture_info.biSize = sizeof(capture_info);
	capture_info.biWidth = source_width;
	capture_info.biHeight = -source_height;
	capture_info.biPlanes = 1;
	capture_info.biBitCount = 32;
	capture_info.biSizeImage = source_width * source_height * 4;
	capture_info.biCompression = BI_RGB;
	if (capture_info.biSizeImage > capture_size_max) {
		meas_capture_fps = 0;
		return;
	}
	capture_buffer = _aligned_malloc(capture_info.biSizeImage, 16);
	source_context = GetWindowDC(source_window);
	capture_context = CreateCompatibleDC(source_context);
	capture_bitmap = CreateCompatibleBitmap(source_context, source_width, source_height);
	SelectObject(capture_context, capture_bitmap);
	frame_event = CreateEvent(NULL, FALSE, FALSE, NULL);
	stop_event = CreateEvent(NULL, FALSE, FALSE, NULL);
	capture_thread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)capture_proc, NULL, 0, NULL);
	recording_time = 0;
	recording_size = 0;
}

void stop_capture() {
	if (!capture_running) {
		return;
	}
	capture_running = false;
	if (capture_info.biSizeImage > capture_size_max) {
		return;
	}
	SetEvent(stop_event);
	WaitForSingleObject(capture_thread, INFINITE);
	CloseHandle(capture_thread);
	CloseHandle(frame_event);
	CloseHandle(stop_event);
	DeleteObject(capture_bitmap);
	DeleteDC(capture_context);
	ReleaseDC(source_window, source_context);
	_aligned_free(capture_buffer);
}

void update_capture() {
	stop_capture();
	start_capture();
}

void reset_capture() {
	stop_capture();
	source_width = (get_rect_width(&source_rect) + 31) / 32 * 32;
	source_height = (get_rect_height(&source_rect) + 1) / 2 * 2;
	capture_width = source_width;
	capture_height = source_height;
	start_capture();
}

void update_source() {
	source_mode = (int)SendMessage(source_list, CB_GETCURSEL, 0, 0);
	if (source_mode == 0) {
		source_window = GetDesktopWindow();
		GetWindowRect(source_window, &source_rect);
	}
	else {
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
	reset_capture();
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
		SetWindowSubclass(control, (SUBCLASSPROC)edit_proc, 0, 0);
	}
	if (wcscmp(name, L"BUTTON") == 0) {
		SetWindowSubclass(control, (SUBCLASSPROC)button_proc, 0, 0);
	}
	return control;
}

void create_controls() {
	source_label = create_control(L"STATIC", L"Source:", SS_CENTERIMAGE);
	source_list = create_control(L"COMBOBOX", NULL, CBS_AUTOHSCROLL | CBS_DROPDOWN | CBS_HASSTRINGS);
	SendMessage(source_list, CB_ADDSTRING, 0, (LPARAM)L"Fullscreen");
	SendMessage(source_list, CB_ADDSTRING, 0, (LPARAM)L"Rectangle");
	SendMessage(source_list, CB_ADDSTRING, 0, (LPARAM)L"Window");
	SendMessage(source_list, CB_SETCURSEL, 0, 0);
	SetWindowSubclass(source_list, (SUBCLASSPROC)source_list_proc, 0, 0);
	source_edit = FindWindowEx(source_list, NULL, L"EDIT", NULL);
	SendMessage(source_edit, EM_SETREADONLY, TRUE, 0);
	width_label = create_control(L"STATIC", L"Width:", SS_CENTERIMAGE);
	width_edit = create_control(L"EDIT", L"", WS_BORDER | ES_RIGHT | ES_NUMBER);
	height_label = create_control(L"STATIC", L"Height:", SS_CENTERIMAGE);
	height_edit = create_control(L"EDIT", L"", WS_BORDER | ES_RIGHT | ES_NUMBER);
	stretch_checkbox = create_control(L"BUTTON", L"Stretch", BS_AUTOCHECKBOX | BS_CHECKBOX);
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
	EnableWindow(source_list, !recording_running);
	capture_stretch = SendMessage(stretch_checkbox, BM_GETCHECK, 0, 0) != BST_UNCHECKED;
	EnableWindow(width_edit, !recording_running && capture_stretch);
	EnableWindow(height_edit, !recording_running && capture_stretch);
	SendMessage(width_edit, EM_SETREADONLY, !capture_stretch, 0);
	SendMessage(height_edit, EM_SETREADONLY, !capture_stretch, 0);
	EnableWindow(stretch_checkbox, !recording_running);
	EnableWindow(keep_ratio_checkbox, !recording_running);
	EnableWindow(framerate_edit, !recording_running);
	EnableWindow(bitrate_edit, !recording_running);
	EnableWindow(start_button, !recording_running);
	EnableWindow(stop_button, recording_running);
	EnableWindow(pause_button, recording_running);
	EnableWindow(settings_button, settings_window == NULL);
	lock_controls = false;
}

void place_controls() {
	MoveWindow(source_label, 10, 10, 80, 20, TRUE);
	MoveWindow(source_list, 100, 10, 150, 100, TRUE);
	MoveWindow(width_label, 270, 10, 50, 20, TRUE);
	MoveWindow(width_edit, 330, 10, 50, 20, TRUE);
	MoveWindow(height_label, 270, 35, 50, 20, TRUE);
	MoveWindow(height_edit, 330, 35, 50, 20, TRUE);
	MoveWindow(stretch_checkbox, 410, 10, 75, 20, TRUE);
	MoveWindow(keep_ratio_checkbox, 410, 35, 75, 20, TRUE);
	MoveWindow(framerate_label, 510, 10, 100, 20, TRUE);
	MoveWindow(framerate_edit, 620, 10, 50, 20, TRUE);
	MoveWindow(bitrate_label, 510, 35, 100, 20, TRUE);
	MoveWindow(bitrate_edit, 620, 35, 50, 20, TRUE);
	MoveWindow(start_button, 15, main_rect.bottom - 65, 50, 50, TRUE);
	MoveWindow(pause_button, 80, main_rect.bottom - 65, 50, 50, TRUE);
	MoveWindow(stop_button, 145, main_rect.bottom - 65, 50, 50, TRUE);
	MoveWindow(time_label, 225, main_rect.bottom - 70, 155, 30, TRUE);
	MoveWindow(size_label, 230, main_rect.bottom - 40, 150, 30, TRUE);
	MoveWindow(cpu_label, main_rect.right - 230, main_rect.bottom - 70, 150, 30, TRUE);
	MoveWindow(mem_label, main_rect.right - 230, main_rect.bottom - 40, 150, 30, TRUE);
	MoveWindow(settings_button, main_rect.right - 65, main_rect.bottom - 65, 50, 50, TRUE);
}

void encode_proc() {
	recording_start_time = get_time();
	sink_writer->BeginWriting();
	double meas_time = recording_start_time;
	double meas_total = 0;
	int meas_count = 0;
	while (recording_running) {
		WaitForSingleObject(frame_event, INFINITE);
		if (!recording_running) {
			break;
		}
		if (recording_paused) {
			continue;
		}
		double frame_start_time = get_time();
		IMFMediaBuffer* buffer;
		int encode_size = source_width * source_height * 3 / 2;
		MFCreateAlignedMemoryBuffer(encode_size, MF_16_BYTE_ALIGNMENT, &buffer);
		buffer->SetCurrentLength(encode_size);
		BYTE* data;
		buffer->Lock(&data, NULL, NULL);
		AcquireSRWLockShared(&capture_lock);
		convert_bgr32_to_nv12((uint32_t*)capture_buffer, data, data + source_width * source_height, source_width, source_height);
		ReleaseSRWLockShared(&capture_lock);
		buffer->Unlock();
		IMFSample* sample;
		CHECK(MFCreateSample(&sample));
		CHECK(sample->AddBuffer(buffer));
		sample->SetSampleTime(llround((frame_start_time - recording_start_time) * 10000000));
		sample->SetSampleDuration(10000000 / capture_framerate);
		CHECK(sink_writer->WriteSample(stream_index, sample));
		sample->Release();
		buffer->Release();
		double frame_end_time = get_time();
		MF_SINK_WRITER_STATISTICS stats = { sizeof(stats) };
		CHECK(sink_writer->GetStatistics(stream_index, &stats));
		recording_time = stats.llLastTimestampProcessed / 10000;
		recording_size = stats.qwByteCountProcessed;
		meas_total += frame_end_time - frame_start_time;
		meas_count++;
		if (frame_start_time > meas_time + meas_interval) {
			meas_encode_fps = meas_count / meas_total;
			meas_time = frame_start_time;
			meas_total = 0;
			meas_count = 0;
		}
	}
	sink_writer->Finalize();
}

void start_recording() {
	if (recording_running) {
		return;
	}
	recording_running = true;
	recording_paused = false;
	recording_time = 0;
	recording_size = 0;
	output_path = L"out.mp4";
	update_controls();
	if (settings.hide_at_start) {
		ShowWindow(main_window, SW_MINIMIZE);
	}
	if (settings.hide_at_start && settings.hide_from_taskbar) {
		ShowWindow(main_window, SW_HIDE);
	}
	UpdateWindow(main_window);
	WaitForSingleObject(frame_event, INFINITE);
	WaitForSingleObject(frame_event, INFINITE);
	IMFMediaType* output_type;
	MFCreateMediaType(&output_type);
	output_type->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
	output_type->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_H264);
	output_type->SetUINT32(MF_MT_INTERLACE_MODE, MFVideoInterlace_Progressive);
	output_type->SetUINT32(MF_MT_AVG_BITRATE, capture_bitrate * 1000);
	MFSetAttributeSize(output_type, MF_MT_FRAME_SIZE, capture_width, capture_height);
	MFSetAttributeRatio(output_type, MF_MT_FRAME_RATE, capture_framerate, 1);
	MFCreateSinkWriterFromURL(output_path, NULL, NULL, &sink_writer);
	CHECK(sink_writer->AddStream(output_type, &stream_index));
	output_type->Release();
	IMFMediaType* input_type;
	MFCreateMediaType(&input_type);
	input_type->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
	input_type->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_NV12);
	input_type->SetUINT32(MF_MT_INTERLACE_MODE, MFVideoInterlace_Progressive);
	MFSetAttributeSize(input_type, MF_MT_FRAME_SIZE, source_width, source_height);
	MFSetAttributeRatio(input_type, MF_MT_FRAME_RATE, capture_framerate, 1);
	IMFAttributes* encoder_attributes;
	MFCreateAttributes(&encoder_attributes, 0);
	// customize codec
	CHECK(sink_writer->SetInputMediaType(stream_index, input_type, encoder_attributes));
	if (settings.beep_at_start) {
		Beep(5000, 200);
	}
	encode_thread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)encode_proc, NULL, 0, NULL);
}

void stop_recording() {
	if (!recording_running) {
		return;
	}
	recording_running = false;
	recording_paused = false;
	WaitForSingleObject(encode_thread, INFINITE);
	CloseHandle(encode_thread);
	sink_writer->Release();
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
	if (settings.beep_at_start) {
		Beep(5000, 200);
	}
	recording_start_time += get_time() - recording_pause_time;
	recording_paused = false;
}

HWND create_dialog(int width, const wchar_t* title, WNDPROC proc) {
	RECT rect = { 0, 0, width, 10 };
	LONG style = WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_VISIBLE;
	AdjustWindowRect(&rect, style, FALSE);
	int adjusted_width = get_rect_width(&rect);
	int adjusted_height = get_rect_height(&rect);
	POINT position = { main_rect.right, main_rect.bottom };
	ClientToScreen(main_window, &position);
	HWND window = CreateWindow(L"STATIC", title, style, (position.x - adjusted_width) / 2, (position.y - adjusted_height) / 2, adjusted_width, adjusted_height, main_window, NULL, NULL, NULL);
	SetWindowLongPtr(window, GWLP_WNDPROC, (LONG_PTR)proc);
	int height = (int)SendMessage(window, WM_USER, 0, 0);
	height += 50;
	SetRect(&rect, 0, 0, width, height);
	CreateWindow(L"BUTTON", L"OK", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON, width - 90, height - 30, 80, 20, window, (HMENU)IDOK, NULL, NULL);
	CreateWindow(L"BUTTON", L"Cancel", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON, width - 180, height - 30, 80, 20, window, (HMENU)IDCANCEL, NULL, NULL);
	AdjustWindowRect(&rect, style, FALSE);
	width = get_rect_width(&rect);
	height = get_rect_height(&rect);
	SetWindowPos(window, NULL, 0, 0, width, height, SWP_NOMOVE | SWP_NOZORDER);
	SetRect(&rect, 0, 0, width, height);
	HDC context = GetDC(window);
	FillRect(context, &rect, white_brush);
	ReleaseDC(window, context);
	EnumChildWindows(window, set_font_proc, (LPARAM)main_font);
	return window;
} 

HWND add_form(HWND window, int* row, const wchar_t* type, const wchar_t* label, void* value) {
	int margin = 10;
	HWND control = NULL;
	RECT rect;
	GetClientRect(window, &rect);
	if (type == NULL) {
		*row += 4;
		control = CreateWindow(L"STATIC", NULL, WS_VISIBLE | WS_CHILD | SS_ETCHEDHORZ, margin, *row, rect.right - margin * 2, 1, window, NULL, NULL, NULL);
		*row += 5;
	}
	else {
		int height = 25;
		int spacing = 3;
		int label_width = (rect.right - margin * 3) * 2 / 3;
		int control_width = (rect.right - margin * 3) / 3;
		CreateWindow(L"STATIC", label, WS_VISIBLE | WS_CHILD | SS_CENTERIMAGE, margin, *row, label_width, height, window, NULL, NULL, NULL);
		const wchar_t* control_class = NULL;
		LONG control_style = 0;
		WNDPROC control_proc = NULL;
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
		control = CreateWindow(control_class, L"", WS_VISIBLE | WS_CHILD | control_style, rect.left + label_width + margin * 2, *row + spacing, control_width, height - spacing, window, NULL, NULL, NULL);
		SetWindowLongPtr(control, GWLP_USERDATA, (LONG_PTR)value);
		SetWindowSubclass(control, (SUBCLASSPROC)control_proc, 0, 0);
		SendMessage(control, WM_USER, 0, 0);
		*row += height;
	}
	SendMessage(control, WM_SETFONT, (WPARAM)main_font, 0);
	return control;
}

void update_settings() {
	SetWindowPos(main_window, settings.stay_on_top ? HWND_TOPMOST : HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
	SetStretchBltMode(render_context, settings.better_preview ? HALFTONE : COLORONCOLOR);
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
			add_form(window, &row, L"checkbox", L"Ask to play recording when finished", &settings.ask_to_play);
			add_form(window, &row, NULL, NULL, NULL);
			add_form(window, &row, L"checkbox", L"High quality preview", &settings.better_preview);
			add_form(window, &row, L"checkbox", L"Disable preview while recording", &settings.disable_preview);
			add_form(window, &row, NULL, NULL, NULL);
			add_form(window, &row, L"checkbox", L"Beep before recording starts", &settings.beep_at_start);
			add_form(window, &row, L"checkbox", L"Beep after recording stops", &settings.beep_at_stop);
			add_form(window, &row, NULL, NULL, NULL);
			add_form(window, &row, L"hotkey", L"Start recording", &settings.start_hotkey);
			add_form(window, &row, L"hotkey", L"Stop recording", &settings.stop_hotkey);
			add_form(window, &row, L"hotkey", L"Pause recording", &settings.pause_hotkey);
			add_form(window, &row, L"hotkey", L"Resume recording", &settings.resume_hotkey);
			return row;
		}
		case WM_DESTROY: {
			settings_window = NULL;
			update_settings();
			update_controls();
			break;
		}
		case WM_CTLCOLORSTATIC: {
			return (INT_PTR)white_brush;
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

void draw_preview() {
	RECT rect = { 3, 66, main_rect.right - 3, main_rect.bottom - 81 };
	if (get_rect_width(&rect) < 10 || get_rect_height(&rect) < 10) {
		return;
	}
	int width = get_rect_width(&rect);
	int height = get_rect_height(&rect);
	width = min(width, height * capture_width / capture_height);
	height = min(height, width * capture_height / capture_width);
	if (capture_stretch) {
		width = min(capture_width, width);
		height = min(capture_height, height);
	}
	int start_x = (rect.left + rect.right) / 2 - width / 2;
	int start_y = (rect.top + rect.bottom) / 2 - (height + 1) / 2;
	if (recording_running && settings.disable_preview) {
		rect.left = start_x;
		rect.top = start_y;
		rect.right = rect.left + width;
		rect.bottom = rect.top + height;
		FillRect(render_context, &rect, black_brush);
		SetTextColor(render_context, RGB(255, 255, 255));
		SelectObject(render_context, display_font);
		DrawText(render_context, L"Preview Disabled", -1, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
	}
	else {
		AcquireSRWLockShared(&capture_lock);
		StretchDIBits(render_context, start_x, start_y, width, height, 0, 0, source_width, source_height, capture_buffer, (const BITMAPINFO*)&capture_info, DIB_RGB_COLORS, SRCCOPY);
		ReleaseSRWLockShared(&capture_lock);
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
			load_images();
			create_controls();
			place_controls();
			update_source();
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
			PostQuitMessage(0);
			break;
		}
		case WM_GETMINMAXINFO: {
			RECT min_rect = { 0, 0, 780, 145 };
			AdjustWindowRect(&min_rect, GetWindowLong(window, GWL_STYLE), FALSE);
			MINMAXINFO* min_max_info = (MINMAXINFO*)lparam;
			min_max_info->ptMinTrackSize.x = get_rect_width(&min_rect);
			min_max_info->ptMinTrackSize.y = get_rect_height(&min_rect);
			return 0;
		}
		case WM_PAINT: {
			if (source_mode == 2 && IsWindow(source_window) && !IsIconic(source_window) && !capture_stretch && !recording_running) {
				RECT new_rect;
				GetWindowRect(source_window, &new_rect);
				fit_window_rect(source_window, &new_rect);
				if (memcmp(&source_rect, &new_rect, sizeof(RECT)) != 0) {
					source_rect = new_rect;
					reset_capture();
					update_controls();
				}
				wchar_t source_title[128];
				GetWindowText(source_edit, source_title, _countof(source_title));
				wchar_t new_title[128];
				GetWindowText(source_window, new_title, _countof(new_title));
				if (wcslen(new_title) == 0) {
					wcscpy(new_title, L"Unnamed Window");
				}
				if (wcscmp(source_title, new_title) != 0) {
					SetWindowText(source_edit, new_title);
				}
			}
			EnumChildWindows(main_window, custom_draw_proc, NULL);
			SelectObject(render_context, outline_pen);
			SelectObject(render_context, background_brush);
			Rectangle(render_context, 1, 1, main_rect.right, main_rect.bottom);
			SelectObject(render_context, white_brush);
			Rectangle(render_context, 1, 1, 680, 65);
			Rectangle(render_context, 1, 1, 500, 65);
			Rectangle(render_context, 1, 1, 260, 65);
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
				static size_t meas_cpu = 0;
				static size_t meas_mem = 0;
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
					swprintf(text, L"CPU: %2zi %%", meas_cpu);
				}
				if (draw_struct->hwndItem == mem_label) {
					swprintf(text, L"MEM: %3zi MB", meas_mem);
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
					if (child == source_list) {
						update_source();
					}
					break;
				}
				case BN_CLICKED: {
					if (child == stretch_checkbox) {
						reset_capture();
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
						create_dialog(360, L"Settings", settings_proc);
					}
					break;
				}
				case EN_UPDATE: {
					wchar_t buffer[12];
					GetWindowText(child, buffer, _countof(buffer));
					if (child == width_edit) {
						capture_width = _wtoi(buffer);
					}
					if (child == height_edit) {
						capture_height = _wtoi(buffer);
					}
					if (child == framerate_edit) {
						capture_framerate = _wtoi(buffer);
					}
					if (child == bitrate_edit) {
						capture_bitrate = _wtoi(buffer);
					}
					if (SendMessage(keep_ratio_checkbox, BM_GETCHECK, 0, 0) != BST_UNCHECKED) {
						if (child == width_edit) {
							capture_height = capture_width * source_height / source_width;
						}
						if (child == height_edit) {
							capture_width = capture_height * source_width / source_height;
						}
					}
					capture_width = max(1, capture_width);
					capture_height = max(1, capture_height);
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

int main() {
	main_font = CreateFont(16, 0, 0, 0, FW_REGULAR, FALSE, FALSE, FALSE, 0, 0, 0, 0, 0, L"Segoe UI");
	fps_font = CreateFont(60, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, 0, 0, 0, 0, 0, L"Consolas");
	display_font = CreateFont(25, 0, 0, 0, FW_REGULAR, FALSE, FALSE, FALSE, 0, 0, 0, 0, 0, L"Verdana");
	white_brush = CreateSolidBrush(RGB(255, 255, 255));
	black_brush = CreateSolidBrush(RGB(0, 0, 0));
	background_brush = CreateSolidBrush(RGB(220, 220, 220));
	outline_pen = CreatePen(PS_SOLID, 2, RGB(0, 0, 0));
	capture_framerate = 60;
	capture_bitrate = 16000;
	meas_interval = 0.5;
	capture_size_max = 100000000;
	InitializeSRWLock(&capture_lock);
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
