#pragma once

class Aligned2DBuffer : public IMFMediaBuffer, public IMF2DBuffer
{
public:

	Aligned2DBuffer(BYTE* data, LONG pitch, DWORD totalSize);

	~Aligned2DBuffer();

	virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** object) override;

	virtual ULONG STDMETHODCALLTYPE AddRef() override;

	virtual ULONG STDMETHODCALLTYPE Release() override;

	virtual HRESULT STDMETHODCALLTYPE Lock(BYTE** buffer, DWORD* maxLength, DWORD* currentLength) override;

	virtual HRESULT STDMETHODCALLTYPE Unlock() override;

	virtual HRESULT STDMETHODCALLTYPE GetCurrentLength(DWORD* currentLength) override;

	virtual HRESULT STDMETHODCALLTYPE SetCurrentLength(DWORD currentLength) override;

	virtual HRESULT STDMETHODCALLTYPE GetMaxLength(DWORD* maxLength) override;

	virtual HRESULT STDMETHODCALLTYPE Lock2D(BYTE** scanline0, LONG* pitch) override;

	virtual HRESULT STDMETHODCALLTYPE Unlock2D() override;

	virtual HRESULT STDMETHODCALLTYPE GetScanline0AndPitch(BYTE** scanline0, LONG* pitch) override;

	virtual HRESULT STDMETHODCALLTYPE IsContiguousFormat(BOOL* isContiguous) override;

	virtual HRESULT STDMETHODCALLTYPE GetContiguousLength(DWORD* length) override;

	virtual HRESULT STDMETHODCALLTYPE ContiguousCopyTo(BYTE* destBuffer, DWORD destBufferSize) override;

	virtual HRESULT STDMETHODCALLTYPE ContiguousCopyFrom(const BYTE* srcBuffer, DWORD srcBufferSize) override;

private:

	BYTE* _data;
	LONG _pitch;
	DWORD _maxLength;
	DWORD _currentLength;
	LONG _refCount;
};
