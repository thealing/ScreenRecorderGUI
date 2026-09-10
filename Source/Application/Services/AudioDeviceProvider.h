#pragma once

class AudioDeviceProvider : IMMNotificationClient, Virtual
{
public:

	AudioDeviceProvider();

	~AudioDeviceProvider();

	const Event* getInputChangeEvent();

	const Event* getOutputChangeEvent();

	AudioDevice* getInputDevice();

	AudioDevice* getOutputDevice();

private:

	void checkDevices();

	bool getDeviceStatus(EDataFlow flow) const;

	HRESULT getDeviceName(LPCWSTR deviceId, PROPVARIANT* deviceName) const;

	virtual ULONG STDMETHODCALLTYPE AddRef() override;

	virtual ULONG STDMETHODCALLTYPE Release() override;

	virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject) override;

	virtual HRESULT STDMETHODCALLTYPE OnDeviceStateChanged(LPCWSTR pwstrDeviceId, DWORD dwNewState) override;

	virtual HRESULT STDMETHODCALLTYPE OnDeviceAdded(LPCWSTR pwstrDeviceId) override;

	virtual HRESULT STDMETHODCALLTYPE OnDeviceRemoved(LPCWSTR pwstrDeviceId) override;

	virtual HRESULT STDMETHODCALLTYPE OnDefaultDeviceChanged(EDataFlow flow, ERole role, LPCWSTR pwstrDefaultDeviceId) override;

	virtual HRESULT STDMETHODCALLTYPE OnPropertyValueChanged(LPCWSTR pwstrDeviceId, const PROPERTYKEY key) override;

private:

	unsigned int _refCount;
	Status _status;
	ComPointer<IMMDeviceEnumerator> _enumerator;
	UniquePointer<Timer> _checkTimer;
	HWND _foregroundWindow;
	bool _inputStatus;
	bool _outputStatus;
	EventPool _inputChangeEventPool;
	EventPool _outputChangeEventPool;
};
