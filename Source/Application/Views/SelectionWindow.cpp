#include "SelectionWindow.h"

SelectionWindow::SelectionWindow() : OverlayWindow(L"Selection Window")
{
	_rect = {};
	_clicked = false;
	setAlpha(100);
	postMessage(WM_PAINT, 0, 0);
}

void SelectionWindow::setRect(const RECT& rect)
{
	HWND handle = getHandle();
	POINT lowerBound = { rect.left, rect.top };
	POINT upperBound = { rect.right, rect.bottom };
	ScreenToClient(handle, &lowerBound);
	ScreenToClient(handle, &upperBound);
	_rect = { lowerBound.x, lowerBound.y, upperBound.x, upperBound.y };
	postMessage(WM_PAINT, 0, 0);
}

void SelectionWindow::onPaint()
{
	HBRUSH blackBrush = CreateSolidBrush(RGB(0, 0, 0));
	HBRUSH whiteBrush = CreateSolidBrush(RGB(0, 255, 255));
	HDC context = getContext();
	RECT rect = { 0, 0, getWidth(), getHeight() };
	FillRect(context, &rect, blackBrush);
	FillRect(context, &_rect, whiteBrush);
	display();
}

POINT SelectionWindow::getScreenPoint(LPARAM lParam) const
{
	HWND handle = getHandle();
	POINT point = { LOWORD(lParam), HIWORD(lParam) };
	ClientToScreen(handle, &point);
	return point;
}

bool SelectionWindow::handleMessage(UINT message, WPARAM, LPARAM lParam)
{
	switch (message)
	{
		case WM_KEYDOWN:
		{
			onKeyPress();
			return true;
		}
		case WM_MOUSEMOVE:
		{
			POINT point = getScreenPoint(lParam);
			onMouseMove(point.x, point.y);
			return true;
		}
		case WM_LBUTTONDOWN:
		{
			POINT point = getScreenPoint(lParam);
			onMouseClick(point.x, point.y);
			return true;
		}
		case WM_PAINT:
		{
			onPaint();
			return true;
		}
		default:
		{
			return false;
		}
	}
}
