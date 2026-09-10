#pragma once

class WindowCapture : public VideoCapture
{
public:

	WindowCapture(HWND window);

	void start(int frameRate);

	void stop();

protected:

	HWND getWindow() const;

	virtual bool captureFrame() = 0;

private:

	void update();

private:

	HWND _window;
	UniquePointer<Timer> _timer;
	double _failTime;
};

