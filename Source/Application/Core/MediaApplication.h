#pragma once

class MediaApplication : public Application
{
public:

	MediaApplication(bool console);

	~MediaApplication();

private:

	void initPlatform();

	void uninitPlatform();

	void setAccurateTimer();

	void setDpiAwareness();

private:

	Status _status;

private:

	typedef HRESULT WINAPI SetProcessDpiAwareness(int);
};
