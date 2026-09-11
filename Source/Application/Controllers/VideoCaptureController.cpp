#include "VideoCaptureController.h"

VideoCaptureController::VideoCaptureController(MainWindow* mainWindow, VideoCaptureManager* videoCaptureManager, WindowCaptureFactory* windowCaptureFactory, ScreenCaptureFactory* screenCaptureFactory, VideoResizerFactory* videoResizerFactory, VideoSourceManager* videoSourceManager, VideoSettingsManager* videoSettingsManager, KeyboardListener* keyboardListener)
{
	_mainWindow = mainWindow;
	_videoCaptureManager = videoCaptureManager;
	_windowCaptureFactory = windowCaptureFactory;
	_screenCaptureFactory = screenCaptureFactory;
	_videoResizerFactory = videoResizerFactory;
	_videoSourceManager = videoSourceManager;
	_videoSettingsManager = videoSettingsManager;
	_keyboardListener = keyboardListener;
	_eventDispatcher.addEntry(videoCaptureManager->getErrorEvent(), BIND(VideoCaptureController, onCaptureFailed, this));
	_eventDispatcher.addEntry(windowCaptureFactory->getChangeEvent(), BIND(VideoCaptureController, onWindowCaptureChanged, this));
	_eventDispatcher.addEntry(screenCaptureFactory->getChangeEvent(), BIND(VideoCaptureController, onScreenCaptureChanged, this));
	_eventDispatcher.addEntry(videoResizerFactory->getChangeEvent(), BIND(VideoCaptureController, onResizerChanged, this));
	_eventDispatcher.addEntry(videoSourceManager->getChangeEvent(), BIND(VideoCaptureController, onSourceChanged, this));
	_eventDispatcher.addEntry(videoSourceManager->getSizeEvent(), BIND(VideoCaptureController, onSourceSizeChanged, this));
	_eventDispatcher.addEntry(videoSourceManager->getDestroyEvent(), BIND(VideoCaptureController, onSourceDestroyed, this));
	_eventDispatcher.addEntry(keyboardListener->getRefreshEvent(), BIND(VideoCaptureController, onRefreshHotkeyPressed, this));
	_eventDispatcher.start();
	LogUtil::logDebug(L"VideoCaptureController: Started on thread %i.", _eventDispatcher.getThreadId());
}

VideoCaptureController::~VideoCaptureController()
{
	_eventDispatcher.stop();
	LogUtil::logDebug(L"VideoCaptureController: Stopped.");
}

void VideoCaptureController::onCaptureFailed()
{
	LogUtil::logInfo(L"VideoCaptureController: Video capture failed.");
	updateCapture();
}

void VideoCaptureController::onWindowCaptureChanged()
{
	LogUtil::logInfo(L"VideoCaptureController: Window capture changed.");
	VideoSource source = _videoSourceManager->getSource();
	if (source == VideoSourceWindow)
	{
		updateCapture();
	}
}

void VideoCaptureController::onScreenCaptureChanged()
{
	LogUtil::logInfo(L"VideoCaptureController: Screen capture changed.");
	VideoSource source = _videoSourceManager->getSource();
	if (source != VideoSourceWindow)
	{
		updateCapture();
	}
}

void VideoCaptureController::onResizerChanged()
{
	LogUtil::logInfo(L"VideoCaptureController: Video resizer changed.");
	updateCapture();
}

void VideoCaptureController::onSourceChanged()
{
	LogUtil::logInfo(L"VideoCaptureController: Video source changed.");
	updateCapture();
}

void VideoCaptureController::onSourceSizeChanged()
{
	LogUtil::logInfo(L"VideoCaptureController: Video source size changed.");
	updateCapture();
}

void VideoCaptureController::onSourceDestroyed()
{
	LogUtil::logInfo(L"VideoCaptureController: Video source destroyed.");
	HMONITOR monitor = _mainWindow->getMonitor();
	_videoSourceManager->setFullscreenSource(monitor);
}

void VideoCaptureController::onRefreshHotkeyPressed()
{
	LogUtil::logInfo(L"VideoCaptureController: Refresh hotkey pressed.");
	updateCapture();
}

void VideoCaptureController::updateCapture()
{
	LogUtil::logInfo(L"VideoCaptureController: Updating capture.");
	_videoCaptureManager->reset();
	HWND window = NULL;
	RECT rect = {};
	VideoSource source = _videoSourceManager->getSource(&window, &rect);
	VideoCapture* capture;
	if (source == VideoSourceWindow)
	{
		capture = _windowCaptureFactory->createCapture(window, rect);
	}
	else
	{
		capture = _screenCaptureFactory->createCapture(window, rect);
	}
	capture = _videoResizerFactory->createResizer(capture);
	_videoCaptureManager->setCapture(capture);
	LogUtil::logInfo(L"VideoCaptureController: Updated capture.");
}
