#include "VideoSourceManager.h"

VideoSourceManager::VideoSourceManager()
{
	_timer = new Timer(0, 0.1, BIND(VideoSourceManager, update, this));
}

const Event* VideoSourceManager::getChangeEvent()
{
	return _changeEventPool.getEvent();
}

const Event* VideoSourceManager::getSizeEvent()
{
	return _sizeEventPool.getEvent();
}

const Event* VideoSourceManager::getDestroyEvent()
{
	return _destroyEventPool.getEvent();
}

void VideoSourceManager::setFullscreenSource(HMONITOR monitor)
{
	HWND window = GetDesktopWindow();
	RECT rect = {};
	setSource(VideoSourceFullscreen, window, monitor, rect);
}

void VideoSourceManager::setRectangleSource(HWND window, RECT rect)
{
	setSource(VideoSourceRectangle, window, NULL, rect);
}

void VideoSourceManager::setWindowSource(HWND window)
{
	RECT rect = {};
	setSource(VideoSourceWindow, window, NULL, rect);
}

VideoSource VideoSourceManager::getSource() const
{
	ReadLockHolder holder(&_lock);
	return _source;
}

VideoSource VideoSourceManager::getSource(HWND* window, RECT* rect) const
{
	ReadLockHolder holder(&_lock);
	*window = _window;
	if (_source == VideoSourceFullscreen)
	{
		MONITORINFO monitorInfo = {};
		monitorInfo.cbSize = sizeof(MONITORINFO);
		GetMonitorInfo(_monitor, &monitorInfo);
		*rect = monitorInfo.rcMonitor;
	}
	if (_source == VideoSourceRectangle)
	{
		*rect = _rect;
	}
	if (_source == VideoSourceWindow)
	{
		GetClientRect(_window, rect);
	}
	return _source;
}

void VideoSourceManager::setSource(VideoSource source, HWND window, HMONITOR monitor, RECT rect)
{
	WriteLockHolder holder(&_lock);
	_source = source;
	_window = window;
	_monitor = monitor;
	_rect = rect;
	_size = WindowUtil::getClientSize(window);
	_changeEventPool.setEvents();
}

void VideoSourceManager::update()
{
	WriteLockHolder holder(&_lock);
	if (!IsWindow(_window))
	{
		_window = NULL;
		_destroyEventPool.setEvents();
		return;
	}
	SIZE size = WindowUtil::getClientSize(_window);
	if (!MemoryUtil::areEqual(size, _size))
	{
		_size = size;
		_sizeEventPool.setEvents();
		return;
	}
}
