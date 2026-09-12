#include "DwmPrintWindowCapture.h"

DwmPrintWindowCapture::DwmPrintWindowCapture(HWND window) : WindowCapture(window)
{
	SIZE size = WindowUtil::getClientSize(window);
	createBuffer(size.cx, size.cy);
	const Buffer* buffer = getBuffer();
	int height = buffer->getHeight();
	int stride = buffer->getStride();
	HDC sourceContext = GetDC(window); 
	_captureContext = CreateCompatibleDC(sourceContext);
	BITMAPINFO bitmapInfo = {};
	bitmapInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bitmapInfo.bmiHeader.biWidth = stride;
	bitmapInfo.bmiHeader.biHeight = -height;
	bitmapInfo.bmiHeader.biPlanes = 1;
	bitmapInfo.bmiHeader.biBitCount = 32;
	bitmapInfo.bmiHeader.biCompression = BI_RGB;
	_captureBitmap = CreateDIBSection(sourceContext, &bitmapInfo, DIB_RGB_COLORS, &_capturePixels, NULL, 0);
	SelectObject(_captureContext, _captureBitmap);
	ReleaseDC(window, sourceContext);
}

DwmPrintWindowCapture::~DwmPrintWindowCapture()
{
	stop();
	DeleteDC(_captureContext);
	DeleteObject(_captureBitmap);
}

bool DwmPrintWindowCapture::captureFrame()
{
	const Buffer* buffer = getBuffer();
	int width = buffer->getWidth();
	int height = buffer->getHeight();
	int stride = buffer->getStride();
	bool result = true;
	if (result)
	{
		HWND window = getWindow();
		result = PrintWindow(window, _captureContext, 3);
	}
	if (result)
	{
		uint32_t* pixels = beginFrame();
		memcpy(pixels, _capturePixels, height * stride * sizeof(uint32_t));
		endFrame();
	}
	return result;
}
