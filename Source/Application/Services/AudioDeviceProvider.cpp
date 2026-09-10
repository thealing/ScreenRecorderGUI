#include "AudioDeviceProvider.h"

AudioDeviceProvider::AudioDeviceProvider()
{
	_refCount = 0;
	_foregroundWindow = NULL;
	_inputStatus = false;
	_outputStatus = false;
	if (_status)
	{
		_status = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_ALL, IID_PPV_ARGS(&_enumerator));
	}
	if (_status)
	{
		_status = _enumerator->RegisterEndpointNotificationCallback(this);
	}
	if (_status)
	{
		_checkTimer = new Timer(0.0, 0.1, BIND(AudioDeviceProvider, checkDevices, this));
	}
	if (!_status)
	{
		LogUtil::logComError("AudioDeviceProvider", _status);
	}
}

AudioDeviceProvider::~AudioDeviceProvider()
{
	if (_status)
	{
		_enumerator->UnregisterEndpointNotificationCallback(this);
	}
}

const Event* AudioDeviceProvider::getInputChangeEvent()
{
	return _inputChangeEventPool.getEvent();
}

const Event* AudioDeviceProvider::getOutputChangeEvent()
{
	return _outputChangeEventPool.getEvent();
}

AudioDevice* AudioDeviceProvider::getInputDevice()
{
	AudioDevice* device = new AudioDevice(_enumerator, eCapture, eConsole);
	_inputStatus = device->getStatus();
	return device;
}

AudioDevice* AudioDeviceProvider::getOutputDevice()
{
	AudioDevice* device = new AudioDevice(_enumerator, eRender, eConsole);
	_outputStatus = device->getStatus();
	return device;
}

void AudioDeviceProvider::checkDevices()
{
	HWND foregroundWindow = GetForegroundWindow();
	if (foregroundWindow != _foregroundWindow)
	{
		_foregroundWindow = foregroundWindow;
		bool inputStatus = getDeviceStatus(eCapture);
		bool outputStatus = getDeviceStatus(eRender);
		if (inputStatus != _inputStatus)
		{
			_inputStatus = inputStatus;
			_inputChangeEventPool.setEvents();
		}
		if (outputStatus != _outputStatus)
		{
			_outputStatus = outputStatus;
			_outputChangeEventPool.setEvents();
		}
	}
}

bool AudioDeviceProvider::getDeviceStatus(EDataFlow flow) const
{
	Status result;
	WaveFormat waveFormat;
	ComPointer<IMMDevice> device;
	ComPointer<IAudioClient> audioClient;
	if (result)
	{
		result = _enumerator->GetDefaultAudioEndpoint(flow, eConsole, &device);
	}
	if (result)
	{
		result = device->Activate(__uuidof(IAudioClient), CLSCTX_ALL, NULL, (void**)&audioClient);
	}
	if (result)
	{
		result = audioClient->GetMixFormat(&waveFormat);
	}
	if (result)
	{
		DWORD flags = 0;
		if (flow == eRender)
		{
			flags = AUDCLNT_STREAMFLAGS_LOOPBACK;
		}
		result = audioClient->Initialize(AUDCLNT_SHAREMODE_SHARED, flags, 0, 0, waveFormat, NULL);
	}
	return result;
}

HRESULT AudioDeviceProvider::getDeviceName(LPCWSTR deviceId, PROPVARIANT* deviceName) const
{
	Status result;
	ComPointer<IMMDevice> device;
	ComPointer<IPropertyStore> propertyStore;
	if (result && deviceId == NULL)
	{
		result = E_INVALIDARG;
	}
	if (result && deviceName == NULL)
	{
		result = E_INVALIDARG;
	}
	if (result)
	{
		result = _enumerator->GetDevice(deviceId, &device);
	}
	if (result)
	{
		result = device->OpenPropertyStore(STGM_READ, &propertyStore);
	}
	if (result)
	{
		result = propertyStore->GetValue(PKEY_Device_FriendlyName, deviceName);
	}
	if (!result)
	{
		InitPropVariantFromString(L"none", deviceName);
		LogUtil::logComWarning(__FUNCTION__, result);
	}
	return result;
}

ULONG AudioDeviceProvider::AddRef()
{
	return InterlockedIncrement(&_refCount);
}

ULONG AudioDeviceProvider::Release()
{
	return InterlockedDecrement(&_refCount);
}

HRESULT AudioDeviceProvider::QueryInterface(REFIID riid, void** ppvObject)
{
	if (riid == __uuidof(IUnknown))
	{
		AddRef();
		*ppvObject = this;
		return S_OK;
	}
	if (riid == __uuidof(IMMNotificationClient))
	{
		AddRef();
		*ppvObject = this;
		return S_OK;
	}
	*ppvObject = NULL;
	return E_NOINTERFACE;
}

HRESULT AudioDeviceProvider::OnDeviceStateChanged(LPCWSTR pwstrDeviceId, DWORD dwNewState)
{
	const wchar_t* pszState = NULL;
	switch (dwNewState)
	{
		case DEVICE_STATE_ACTIVE:
		{
			pszState = L"ACTIVE";
			break;
		}
		case DEVICE_STATE_DISABLED:
		{
			pszState = L"DISABLED";
			break;
		}
		case DEVICE_STATE_NOTPRESENT:
		{
			pszState = L"NOTPRESENT";
			break;
		}
		case DEVICE_STATE_UNPLUGGED:
		{
			pszState = L"UNPLUGGED";
			break;
		}
		default:
		{
			pszState = L"UNKNOWN";
			break;
		}
	}
	PropVariant deviceName;
	getDeviceName(pwstrDeviceId, &deviceName);
	LogUtil::logDebug(L"AudioDeviceProvider: State of device \"%ls\" changed to %ls.", deviceName->pwszVal, pszState);
	return S_OK;
}

HRESULT AudioDeviceProvider::OnDeviceAdded(LPCWSTR pwstrDeviceId)
{
	PropVariant deviceName;
	getDeviceName(pwstrDeviceId, &deviceName);
	LogUtil::logDebug(L"AudioDeviceProvider: Added device \"%ls\".", deviceName->pwszVal);
	return S_OK;
}

HRESULT AudioDeviceProvider::OnDeviceRemoved(LPCWSTR pwstrDeviceId)
{
	PropVariant deviceName;
	getDeviceName(pwstrDeviceId, &deviceName);
	LogUtil::logDebug(L"AudioDeviceProvider: Removed device \"%ls\".", deviceName->pwszVal);
	return S_OK;
}

HRESULT AudioDeviceProvider::OnDefaultDeviceChanged(EDataFlow flow, ERole role, LPCWSTR pwstrDefaultDeviceId)
{
	PropVariant deviceName;
	getDeviceName(pwstrDefaultDeviceId, &deviceName);
	LogUtil::logDebug(L"AudioDeviceProvider: Default device of flow %i role %i changed to \"%ls\".", flow, role, deviceName->pwszVal);
	if (flow == eRender && role == eConsole)
	{
		LogUtil::logInfo(L"AudioDeviceProvider: Default output device changed to \"%ls\".", deviceName->pwszVal);
		_outputChangeEventPool.setEvents();
	}
	if (flow == eCapture && role == eConsole)
	{
		LogUtil::logInfo(L"AudioDeviceProvider: Default input device changed to \"%ls\".", deviceName->pwszVal);
		_inputChangeEventPool.setEvents();
	}
	return S_OK;
}

HRESULT AudioDeviceProvider::OnPropertyValueChanged(LPCWSTR deviceId, const PROPERTYKEY)
{
	return S_OK;
}
