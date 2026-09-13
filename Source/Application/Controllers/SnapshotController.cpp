#include "SnapshotController.h"

SnapshotController::SnapshotController(VideoCaptureManager* videoCaptureManager, KeyboardListener* keyboardListener)
{
	_videoCaptureManager = videoCaptureManager;
	_keyboardListener = keyboardListener;
	_eventDispatcher.addEntry(keyboardListener->getSnapshotEvent(), BIND(SnapshotController, onSnapshotHotkeyPressed, this));
	_eventDispatcher.start();
	LogUtil::logDebug(L"SnapshotController: Started on thread %i.", _eventDispatcher.getThreadId());
}

SnapshotController::~SnapshotController()
{
	_eventDispatcher.stop();
	LogUtil::logDebug(L"SnapshotController: Stopped.");
}

void SnapshotController::onSnapshotHotkeyPressed()
{
	LogUtil::logInfo(L"SnapshotController: Snapshot hotkey pressed.");
	takeSnapshot();
}

void SnapshotController::takeSnapshot()
{
	VideoCapture* capture = _videoCaptureManager->lockCapture();
	Status result;
	ComPointer<IWICImagingFactory> factory;
	ComPointer<IWICBitmap> bitmap;
	ComPointer<IWICStream> stream;
	ComPointer<IWICBitmapEncoder> encoder;
	ComPointer<IWICBitmapFrameEncode> frame;
	if (result && capture == NULL)
	{
		result = E_POINTER;
	}
	if (result)
	{
		result = CoCreateInstance(CLSID_WICImagingFactory1, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&factory));
	}
	if (result)
	{
		const Buffer* buffer = capture->getBuffer();
		int width = buffer->getWidth();
		int height = buffer->getHeight();
		int stride = buffer->getStride();
		const uint32_t* pixels = buffer->beginReading();
		result = factory->CreateBitmapFromMemory(width, height, GUID_WICPixelFormat32bppBGRA, stride * 4, height * stride * 4, (BYTE*)pixels, &bitmap);
		buffer->endReading();
	}
	if (result)
	{
		result = factory->CreateStream(&stream);
	}
	if (result)
	{
		UniquePointer<const wchar_t> path = FileUtil::generateSnapshotSavePath();
		result = stream->InitializeFromFilename(path, GENERIC_WRITE);
	}
	if (result)
	{
		result = factory->CreateEncoder(GUID_ContainerFormatPng, NULL, &encoder);
	}
	if (result)
	{
		result = encoder->Initialize(stream, WICBitmapEncoderNoCache);
	}
	if (result)
	{
		result = encoder->CreateNewFrame(&frame, NULL);
	}
	if (result)
	{
		result = frame->Initialize(NULL);
	}
	if (result)
	{
		const Buffer* buffer = capture->getBuffer();
		int width = buffer->getWidth();
		int height = buffer->getHeight();
		result = frame->SetSize(width, height);
	}
	_videoCaptureManager->unlockCapture();
	if (result)
	{
		WICPixelFormatGUID format = GUID_WICPixelFormat32bppBGRA;
		result = frame->SetPixelFormat(&format);
	}
	if (result)
	{
		result = frame->WriteSource(bitmap, NULL);
	}
	if (result)
	{
		result = frame->Commit();
	}
	if (result)
	{
		result = encoder->Commit();
	}
	if (result)
	{
		MessageBeep(MB_OK);
	}
	else
	{
		MessageBeep(MB_ICONERROR);
	}
	if (!result)
	{
		LogUtil::logComError(__FUNCTION__, result);
	}
}
