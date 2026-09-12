#include "VideoCapture.h"

VideoCapture::VideoCapture()
{
}

void VideoCapture::addOverlay(Overlay* overlay)
{
	_overlays.store(overlay);
}

const Buffer* VideoCapture::getBuffer() const
{
	return _buffer;
}

void VideoCapture::createBuffer(int width, int height)
{
	_buffer = new Buffer(width, height);
}

uint32_t* VideoCapture::beginFrame()
{
	return _buffer->beginWriting();
}

template<typename... Args>
void VideoCapture::endFrame(Args... args)
{
	uint32_t* pixels = _buffer->getPixels();
	int width = _buffer->getWidth();
	int height = _buffer->getHeight();
	int stride = _buffer->getStride();
	for (Overlay* overlay : _overlays)
	{
		overlay->draw(pixels, width, height, stride);
	}
	_buffer->endWriting(args...);
	signalFrame();
}
