#pragma once

class VideoSourceManager : NonCopyable
{
public:

	VideoSourceManager();

	const Event* getChangeEvent();

	const Event* getSizeEvent();

	const Event* getDestroyEvent();

	void setFullscreenSource(HMONITOR monitor);

	void setRectangleSource(HWND window, RECT rect);

	void setWindowSource(HWND window);

	VideoSource getSource() const;

	VideoSource getSource(HWND* window, RECT* rect) const;

private:

	void setSource(VideoSource source, HWND window, HMONITOR monitor, RECT rect);

	void update();

private:

	mutable ReadWriteLock _lock;

	VideoSource _source;
	HWND _window;
	HMONITOR _monitor;
	RECT _rect;
	SIZE _size;
	UniquePointer<Timer> _timer;
	EventPool _changeEventPool;
	EventPool _sizeEventPool;
	EventPool _destroyEventPool;
};
