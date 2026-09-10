#include "AudioDevice.h"

AudioDevice::AudioDevice(IMMDeviceEnumerator* enumerator, EDataFlow flow, ERole role)
{
	if (enumerator == NULL)
	{
		_status = E_INVALIDARG;
	}
	if (_status)
	{
		_status = enumerator->GetDefaultAudioEndpoint(flow, role, &_device);
	}
	if (_status)
	{
		_status = _device->Activate(__uuidof(IAudioClient), CLSCTX_ALL, NULL, (void**)&_audioClient);
	}
	if (_status)
	{
		_status = _audioClient->GetMixFormat(&_waveFormat);
	}
	if (_status)
	{
		if (flow == eRender)
		{
			UniquePointer<const wchar_t> format = _waveFormat.toString();
			LogUtil::logInfo(L"Output device format: %ls.", (const wchar_t*)format);
		}
		if (flow == eCapture)
		{
			UniquePointer<const wchar_t> format = _waveFormat.toString();
			LogUtil::logInfo(L"Input device format: %ls.", (const wchar_t*)format);
		}
		DWORD flags = 0;
		if (flow == eRender)
		{
			flags = AUDCLNT_STREAMFLAGS_LOOPBACK;
		}
		_status = _audioClient->Initialize(AUDCLNT_SHAREMODE_SHARED, flags, 0, 0, _waveFormat, NULL);
	}
	if (_status)
	{
		_status = _audioClient->GetService(IID_PPV_ARGS(&_captureClient));
	}
	if (_status)
	{
		_status = _audioClient->Start();
	}
	if (_status)
	{
		REFERENCE_TIME defaultPeriod = 0;
		_status = _audioClient->GetDevicePeriod(&defaultPeriod, NULL);
		if (_status)
		{
			REFERENCE_TIME halfPeriod = defaultPeriod / 2;
			_timer = new Timer(0, halfPeriod / 10000000.0, BIND(AudioDevice, onTimer, this));
			if (flow == eRender)
			{
				_player = new SilencePlayer(enumerator, flow, role);
			}
		}
	}
	if (!_status)
	{
		LogUtil::logComError("AudioDevice", _status);
	}
}

AudioDevice::~AudioDevice()
{
	if (_status)
	{
		_status = _audioClient->Stop();
	}
}

HRESULT AudioDevice::getFormat(IMFMediaType** format)
{
	Status result;
	if (result && _waveFormat == NULL)
	{
		result = E_POINTER;
	}
	if (result)
	{
		result = MFCreateMediaType(format);
	}
	if (result)
	{
		result = MFInitMediaTypeFromWaveFormatEx(*format, _waveFormat, _waveFormat.getSize());
	}
	if (!result)
	{
		LogUtil::logComWarning(__FUNCTION__, result);
	}
	return result;
}

HRESULT AudioDevice::getSample(IMFSample** sample)
{
	Status result;
	ComPointer<IMFMediaBuffer> buffer;
	BYTE* frameBuffer = NULL;
	UINT32 frameCount = 0;
	DWORD flags = 0;
	UINT64 position = 0;
	UINT32 bufferSize = 0;
	BYTE* bufferData = NULL;
	if (result && _captureClient == NULL)
	{
		result = E_POINTER;
	}
	if (result && _waveFormat == NULL)
	{
		result = E_POINTER;
	}
	if (result)
	{
		result = _captureClient->GetBuffer(&frameBuffer, &frameCount, &flags, NULL, &position);
	}
	if (result)
	{
		bufferSize = frameCount * _waveFormat->nBlockAlign;
		result = MFCreateMemoryBuffer(bufferSize, &buffer);
	}
	if (result)
	{
		result = buffer->SetCurrentLength(bufferSize);
	}
	if (result)
	{
		result = buffer->Lock(&bufferData, NULL, NULL);
	}
	if (result)
	{
		memcpy(bufferData, frameBuffer, bufferSize);
		result = buffer->Unlock();
	}
	if (frameBuffer != NULL)
	{
		_captureClient->ReleaseBuffer(frameCount);
	}
	if (result)
	{
		result = MFCreateSample(sample);
	}
	if (result)
	{
		result = (*sample)->AddBuffer(buffer);
	}
	if (result)
	{
		LONGLONG time = position; 
		result = (*sample)->SetSampleTime(time);
	}
	if (result)
	{
		LONGLONG duration = 10000000ll * frameCount / max(_waveFormat->nSamplesPerSec, 1ul); 
		result = (*sample)->SetSampleDuration(duration);
	}
	if (result)
	{
		if (flags & AUDCLNT_BUFFERFLAGS_DATA_DISCONTINUITY)
		{
			result = (*sample)->SetUINT32(MFSampleExtension_Discontinuity, TRUE);
		}
	}
	if (!result)
	{
		LogUtil::logComWarning(__FUNCTION__, result);
	}
	return result;
}

void AudioDevice::onTimer()
{
	Status result;
	UINT32 packetSize = 0;
	if (result && _captureClient == NULL)
	{
		result = E_POINTER;
	}
	if (result && _waveFormat == NULL)
	{
		result = E_POINTER;
	}
	if (result)
	{
		result = _captureClient->GetNextPacketSize(&packetSize);
	}
	if (result)
	{
		if (packetSize > 0)
		{
			signalFrame();
		}
	}
	else if (_status)
	{
		_status = result;
		signalError();
	}
}
