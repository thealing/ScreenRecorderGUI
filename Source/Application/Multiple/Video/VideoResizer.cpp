#include "VideoResizer.h"

VideoResizer::VideoResizer(VideoCapture* source, Vector outputSize, Resizer* resizer)
{
	createBuffer(outputSize.x, outputSize.y);
	_source = source;
	_resizer = resizer;
	_eventDispatcher.addEntry(source->getFrameEvent(), BIND(VideoResizer, onFrame, this));
	_eventDispatcher.addEntry(source->getErrorEvent(), BIND(VideoResizer, onError, this));
	_eventDispatcher.start();
}

VideoResizer::~VideoResizer()
{
	_eventDispatcher.stop();
}

void VideoResizer::onFrame()
{
	const Buffer* inputBuffer = _source->getBuffer();
	const uint32_t* inputPixels = inputBuffer->beginReading();
	uint32_t* outputPixels = beginFrame();
	_resizer->resize(inputPixels, outputPixels);
	LONGLONG timestamp = inputBuffer->endReading();
	endFrame(timestamp);
}

void VideoResizer::onError()
{
	signalError();
}
