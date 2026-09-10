#include "SilencePlayer.h"

SilencePlayer::SilencePlayer(IMMDeviceEnumerator* enumerator, EDataFlow flow, ERole role)
{
	WaveFormat waveFormat;
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
		_status = _audioClient->GetMixFormat(&waveFormat);
	}
	if (_status)
	{
		DWORD flags = 0;
		_status = _audioClient->Initialize(AUDCLNT_SHAREMODE_SHARED, flags, 0, 0, waveFormat, NULL);
	}
	if (_status)
	{
		_status = _audioClient->GetService(IID_PPV_ARGS(&_renderClient));
	}
	if (_status)
	{
		UINT32 frameCount = 0;
		BYTE* buffer = NULL;
		_status = _audioClient->GetBufferSize(&frameCount);
		if (_status)
		{
			_status = _renderClient->GetBuffer(frameCount, &buffer);
		}
		if (_status)
		{
			_status = _renderClient->ReleaseBuffer(frameCount, AUDCLNT_BUFFERFLAGS_SILENT);
		}
	}
	if (_status)
	{
		_status = _audioClient->Start();
	}
	if (!_status)
	{
		LogUtil::logComError("SilencePlayer", _status);
	}
}

SilencePlayer::~SilencePlayer()
{
	if (_status)
	{
		_status = _audioClient->Stop();
	}
}
