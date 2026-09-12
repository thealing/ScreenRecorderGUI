#pragma once

class Buffer : NonCopyable
{
public:

	Buffer(int width, int height);

	~Buffer();

	uint32_t* getPixels();

	uint32_t* beginWriting();

	void endWriting();

	void endWriting(LONGLONG timestamp);

	const uint32_t* beginReading() const;

	LONGLONG endReading() const;

	int getWidth() const;

	int getHeight() const;

	int getStride() const;

private:

	mutable ReadWriteLock _lock;

	int _width;
	int _height;
	int _stride;
	uint32_t* _pixels;
	LONGLONG _timestamp;
};
