#include "Encoder.h"

Encoder::Encoder(FrameSource* source, SinkWriter* sinkWriter) : FrameSink(source)
{
	_sinkWriter = sinkWriter;
	_streamIndex = -1;
	_startTime = 0;
	_pauseTime = 0;
}

const Event* Encoder::getEncodeEvent()
{
	return _encodeEventPool.getEvent();
}

const Event* Encoder::getErrorEvent()
{
	return _errorEventPool.getEvent();
}

void Encoder::start()
{
	_startTime = MFGetSystemTime();
	FrameSink::start();
}

void Encoder::stop()
{
	FrameSink::stop();
}

void Encoder::pause()
{
	_pauseTime = MFGetSystemTime();
}

void Encoder::resume()
{
	_startTime += MFGetSystemTime() - _pauseTime;
	_pauseTime = 0;
}

HRESULT Encoder::getStatistics(MF_SINK_WRITER_STATISTICS* statistics) const
{
	Status result;
	if (result && _sinkWriter == NULL)
	{
		result = E_POINTER;
	}
	if (result && _streamIndex == -1)
	{
		result = E_ILLEGAL_METHOD_CALL;
	}
	if (result)
	{
		result = _sinkWriter->getStatistics(_streamIndex, statistics);
	}
	if (!result)
	{
		LogUtil::logComWarning(__FUNCTION__, result);
	}
	return result;
}

Status Encoder::getStatus() const
{
	return _status;
}

HRESULT Encoder::addStream(IMFMediaType* inputType, IMFMediaType* outputType)
{
	Status result;
	if (result && _sinkWriter == NULL)
	{
		result = E_POINTER;
	}
	if (result && _streamIndex != -1)
	{
		result = E_ILLEGAL_METHOD_CALL;
	}
	if (result)
	{
		result = _sinkWriter->addStream(inputType, outputType, &_streamIndex);
	}
	if (!result)
	{
		LogUtil::logComWarning(__FUNCTION__, result);
	}
	return result;
}

void Encoder::onFrame()
{
	if (isPaused())
	{
		return;
	}
	Status result;
	ComPointer<IMFSample> sample;
	LONGLONG time = 0;
	if (result)
	{
		result = getSample(&sample);
	}
	if (result)
	{
		result = sample->GetSampleTime(&time);
	}
	if (result)
	{
		time -= _startTime;
		if (time < 0)
		{
			result = MF_E_INVALID_TIMESTAMP;
		}
	}
	if (result)
	{
		result = sample->SetSampleTime(time);
	}
	if (result)
	{
		result = writeSample(sample);
	}
	if (result)
	{
		_encodeEventPool.setEvents();
	}
	if (!result)
	{
		LogUtil::logComWarning(__FUNCTION__, result);
	}
}

void Encoder::onError()
{
	Status result;
	if (result)
	{
		LONGLONG timestamp = MFGetSystemTime() - _startTime;
		result = sendStreamTick(timestamp);
	}
	if (result)
	{
		_errorEventPool.setEvents();
	}
	if (!result)
	{
		LogUtil::logComWarning(__FUNCTION__, result);
	}
}

bool Encoder::isPaused()
{
	return _pauseTime != 0;
}

HRESULT Encoder::writeSample(IMFSample* sample)
{
	Status result;
	if (result && _sinkWriter == NULL)
	{
		result = E_POINTER;
	}
	if (result && _streamIndex == -1)
	{
		result = E_ILLEGAL_METHOD_CALL;
	}
	if (result)
	{
		result = _sinkWriter->writeSample(_streamIndex, sample);
	}
	if (!result)
	{
		LogUtil::logComWarning(__FUNCTION__, result);
	}
	return result;
}

HRESULT Encoder::sendStreamTick(LONGLONG timestamp)
{
	Status result;
	if (result && _sinkWriter == NULL)
	{
		result = E_POINTER;
	}
	if (result && _streamIndex == -1)
	{
		result = E_ILLEGAL_METHOD_CALL;
	}
	if (result)
	{
		result = _sinkWriter->sendStreamTick(_streamIndex, timestamp);
	}
	if (!result)
	{
		LogUtil::logComWarning(__FUNCTION__, result);
	}
	return result;
}
