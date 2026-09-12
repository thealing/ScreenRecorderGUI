#include "WindowSelector.h"

WindowSelector::WindowSelector()
{
	_selectedWindow = NULL;
}

HWND WindowSelector::getSelectedWindow() const
{
	return _selectedWindow;
}

HWND WindowSelector::getWindowUnderPoint(POINT point)
{
	HWND result = NULL;
	RECT rect = {};
	HWND window = GetDesktopWindow();
	window = GetWindow(window, GW_CHILD);
	while (window != NULL)
	{
		HWND overlayWindow = getHandle();
		if (window == overlayWindow)
		{
			goto Next;
		}
		if (!IsWindowVisible(window))
		{
			goto Next;
		}
		GetWindowRect(window, &rect);
		if (PtInRect(&rect, point))
		{
			result = window;
			window = GetWindow(window, GW_CHILD);
			continue;
		}
	Next:
		window = GetWindow(window, GW_HWNDNEXT);
	}
	return result;
}

void WindowSelector::onMouseMove(int mouseX, int mouseY)
{
	POINT mousePoint = { mouseX, mouseY };
	_selectedWindow = getWindowUnderPoint(mousePoint);
	RECT rect = WindowUtil::getAbsoluteClientRect(_selectedWindow);
	setRect(rect);
}

void WindowSelector::onMouseClick(int, int)
{
	postMessage(WM_CLOSE, 0, 0);
}

void WindowSelector::onKeyPress()
{
	_selectedWindow = NULL;
	postMessage(WM_CLOSE, 0, 0);
}
