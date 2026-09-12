#include "BitBltCapture.h"

BitBltCapture::BitBltCapture(HWND window, POINT position, SIZE size) : WindowCapture(window)
{
	_position = position;
	createBuffer(size.cx, size.cy);
	const Buffer* buffer = getBuffer();
	int height = buffer->getHeight();
	int stride = buffer->getStride();
	_sourceContext = GetDC(window); 
	_captureContext = CreateCompatibleDC(_sourceContext);
	BITMAPINFO bitmapInfo = {};
	bitmapInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bitmapInfo.bmiHeader.biWidth = stride;
	bitmapInfo.bmiHeader.biHeight = -height;
	bitmapInfo.bmiHeader.biPlanes = 1;
	bitmapInfo.bmiHeader.biBitCount = 32;
	bitmapInfo.bmiHeader.biCompression = BI_RGB;
	_captureBitmap = CreateDIBSection(_sourceContext, &bitmapInfo, DIB_RGB_COLORS, &_capturePixels, NULL, 0);
	SelectObject(_captureContext, _captureBitmap);
}

BitBltCapture::~BitBltCapture()
{
	stop();
	HWND window = getWindow();
	ReleaseDC(window, _sourceContext);
	DeleteDC(_captureContext);
	DeleteObject(_captureBitmap);
}

bool BitBltCapture::captureFrame()
{
	const Buffer* buffer = getBuffer();
	int width = buffer->getWidth();
	int height = buffer->getHeight();
	int stride = buffer->getStride();
	bool result = true;
	if (result)
	{
		result = BitBlt(_captureContext, 0, 0, width, height, _sourceContext, _position.x, _position.y, SRCCOPY);
	}
	if (result)
	{
		uint32_t* pixels = beginFrame();
		memcpy(pixels, _capturePixels, height * stride * sizeof(uint32_t));
		endFrame();
	}
	return result;
}
