#include "OverlayWindow.h"

OverlayWindow::OverlayWindow(const wchar_t* title)
{
	int x = GetSystemMetrics(SM_XVIRTUALSCREEN);
	int y = GetSystemMetrics(SM_YVIRTUALSCREEN);
	_width = GetSystemMetrics(SM_CXVIRTUALSCREEN);
	_height = GetSystemMetrics(SM_CYVIRTUALSCREEN);
	_handle = CreateWindowEx(WS_EX_LAYERED | WS_EX_TOPMOST, L"STATIC", title, WS_POPUP | WS_VISIBLE, x, y, _width, _height, NULL, NULL, NULL, NULL);
	SetWindowLongPtr(_handle, GWLP_USERDATA, (LONG_PTR)this);
	SetWindowLongPtr(_handle, GWLP_WNDPROC, (LONG_PTR)windowProc);
	HDC hdc = GetDC(_handle);
	_context = CreateCompatibleDC(hdc);
	_bitmap = CreateCompatibleBitmap(hdc, _width, _height);
	ReleaseDC(_handle, hdc);
	SelectObject(_context, _bitmap);
	_alpha = 0;
}

OverlayWindow::~OverlayWindow()
{
	DestroyWindow(_handle);
	DeleteDC(_context);
	DeleteObject(_bitmap);
}

void OverlayWindow::run()
{
	MSG msg = {};
	while (GetMessage(&msg, _handle, 0, 0) > 0)
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
}

void OverlayWindow::setAlpha(BYTE alpha)
{
	_alpha = alpha;
}

void OverlayWindow::display()
{
	POINT position = { 0, 0 };
	SIZE size = { _width, _height };
	BLENDFUNCTION blendFunc = {};
	blendFunc.SourceConstantAlpha = _alpha;
	UpdateLayeredWindow(_handle, NULL, NULL, &size, _context, &position, 0, &blendFunc, ULW_ALPHA);
	DwmFlush();
}

void OverlayWindow::postMessage(UINT message, WPARAM wParam, LPARAM lParam)
{
	PostMessage(_handle, message, wParam, lParam);
}

int OverlayWindow::getWidth() const
{
	return _width;
}

int OverlayWindow::getHeight() const
{
	return _height;
}

HWND OverlayWindow::getHandle() const
{
	return _handle;
}

HDC OverlayWindow::getContext() const
{
	return _context;
}

bool OverlayWindow::handleMessage(UINT, WPARAM, LPARAM)
{
	return false;
}

LRESULT OverlayWindow::windowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
	if (message != WM_CLOSE)
	{
		OverlayWindow* instance = (OverlayWindow*)GetWindowLongPtr(window, GWLP_USERDATA);
		if (instance->handleMessage(message, wParam, lParam))
		{
			return 0;
		}
	}
	return DefWindowProc(window, message, wParam, lParam);
}
