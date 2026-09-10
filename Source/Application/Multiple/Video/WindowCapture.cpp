#include "WindowCapture.h"

WindowCapture::WindowCapture(HWND window)
{
	_window = window;
}

void WindowCapture::start(int frameRate)
{
	_failTime = 0;
	_timer = new Timer(0, 1.0 / frameRate, BIND(WindowCapture, update, this));
}

void WindowCapture::stop()
{
	_timer = NULL;
}

HWND WindowCapture::getWindow() const
{
	return _window;
}

void WindowCapture::update()
{
	if (_window == NULL)
	{
		return;
	}
	if (!IsWindow(_window))
	{
		_window = NULL;
		signalError();
		return;
	}
	if (captureFrame())
	{
		_failTime = 0;
		return;
	}
	if (_failTime == 0)
	{
		_failTime = getTime();
		return;
	}
	double currentTime = getTime();
	if (currentTime > _failTime + 1)
	{
		_window = NULL;
		signalError();
		return;
	}
}
