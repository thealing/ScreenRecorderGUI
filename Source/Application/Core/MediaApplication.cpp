#include "MediaApplication.h"

MediaApplication::MediaApplication(bool console) : Application(console)
{
	setAccurateTimer();
	setDpiAwareness();
	initPlatform();
}

MediaApplication::~MediaApplication()
{
	uninitPlatform();
}

void MediaApplication::initPlatform()
{
	if (_status)
	{
		_status = CoInitialize(NULL);
	}
	if (_status)
	{
		_status = MFStartup(MF_VERSION);
	}
	if (!_status)
	{
		LogUtil::logComError(__FUNCTION__, _status);
	}
}

void MediaApplication::uninitPlatform()
{
	if (_status)
	{
		_status = MFShutdown();
	}
	if (_status)
	{
		CoUninitialize();
	}
	if (!_status)
	{
		LogUtil::logComError(__FUNCTION__, _status);
	}
}

void MediaApplication::setAccurateTimer()
{
	double timerResolution = Timer::setResolution(0);
	LogUtil::logDebug(L"Changed timer resolution to %g.", timerResolution);
}

void MediaApplication::setDpiAwareness()
{
	DynamicLibrary shcore("shcore");
	SetProcessDpiAwareness* setProcessDpiAwareness = shcore.getFunction<SetProcessDpiAwareness>("SetProcessDpiAwareness");
	if (setProcessDpiAwareness && setProcessDpiAwareness(2) == S_OK)
	{
		LogUtil::logDebug(L"Set per-monitor DPI awareness.");
		return;
	}
	if (SetProcessDPIAware())
	{
		LogUtil::logDebug(L"Set system-wide DPI awareness.");
		return;
	}
	LogUtil::logWarning(L"Failed to set DPI awareness.");
}
