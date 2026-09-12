#include "Buffer.h"

Buffer::Buffer(int width, int height)
{
	_width = BufferUtil::alignWidth(width);
	_height = BufferUtil::alignHeight(height);
	_stride = BufferUtil::alignStride(width);
	_pixels = BufferUtil::allocateBuffer<uint32_t>(_stride * _height);
	_timestamp = 0;
}

Buffer::~Buffer()
{
	BufferUtil::freeBuffer(_pixels);
}

uint32_t* Buffer::getPixels()
{
	return _pixels;
}

uint32_t* Buffer::beginWriting()
{
	_lock.beginWriting();
	return _pixels;
}

void Buffer::endWriting()
{
	LONGLONG timestamp = MFGetSystemTime();
	endWriting(timestamp);
}

void Buffer::endWriting(LONGLONG timestamp)
{
	_timestamp = timestamp;
	_lock.endWriting();
}

const uint32_t* Buffer::beginReading() const
{
	_lock.beginReading();
	return _pixels;
}

LONGLONG Buffer::endReading() const
{
	LONGLONG timestamp = _timestamp;
	_lock.endReading();
	return timestamp;
}

int Buffer::getWidth() const
{
	return _width;
}

int Buffer::getHeight() const
{
	return _height;
}

int Buffer::getStride() const
{
	return _stride;
}
