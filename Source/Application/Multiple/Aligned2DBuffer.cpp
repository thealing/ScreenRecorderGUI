#include "Aligned2DBuffer.h"

Aligned2DBuffer::Aligned2DBuffer(BYTE* data, LONG pitch, DWORD maxLength)
{
	_data = data;
	_pitch = pitch;
	_maxLength = maxLength;
	_currentLength = maxLength;
	_refCount = 1;
}

Aligned2DBuffer::~Aligned2DBuffer()
{
	BufferUtil::freeBuffer(_data);
}

HRESULT Aligned2DBuffer::QueryInterface(REFIID riid, void** object)
{
	if (riid == __uuidof(IMFMediaBuffer))
	{
		AddRef();
		*object = (IMFMediaBuffer*)this;
		return S_OK;
	}
	if (riid == __uuidof(IMF2DBuffer))
	{
		AddRef();
		*object = (IMF2DBuffer*)this;
		return S_OK;
	}
	*object = NULL;
	return E_NOINTERFACE;
}

ULONG Aligned2DBuffer::AddRef()
{
	return InterlockedIncrement(&_refCount);
}

ULONG Aligned2DBuffer::Release()
{
	LONG result = InterlockedDecrement(&_refCount);
	if (result == 0)
	{
		delete this;
	}
	return result;
}

HRESULT Aligned2DBuffer::Lock(BYTE** buffer, DWORD* maxLength, DWORD* currentLength)
{
	return E_NOTIMPL;
}

HRESULT Aligned2DBuffer::Unlock()
{
	return E_NOTIMPL;
}

HRESULT Aligned2DBuffer::GetCurrentLength(DWORD* currentLength)
{
	if (currentLength == NULL)
	{
		return E_POINTER;
	}
	*currentLength = _currentLength;
	return S_OK;
}

HRESULT Aligned2DBuffer::SetCurrentLength(DWORD currentLength)
{
	_currentLength = currentLength;
	return S_OK;
}

HRESULT Aligned2DBuffer::GetMaxLength(DWORD* maxLength)
{
	if (maxLength == NULL)
	{
		return E_POINTER;
	}
	*maxLength = _maxLength;
	return S_OK;
}

HRESULT Aligned2DBuffer::Lock2D(BYTE** scanline0, LONG* pitch)
{
	if (scanline0 == NULL || pitch == NULL)
	{
		return E_POINTER;
	}
	*scanline0 = _data;
	*pitch = _pitch;
	return S_OK;
}

HRESULT Aligned2DBuffer::Unlock2D()
{
	return S_OK;
}

HRESULT Aligned2DBuffer::GetScanline0AndPitch(BYTE** scanline0, LONG* pitch)
{
	return Lock2D(scanline0, pitch);
}

HRESULT Aligned2DBuffer::IsContiguousFormat(BOOL* isContiguous)
{
	if (isContiguous == NULL)
	{
		return E_POINTER;
	}
	*isContiguous = FALSE;
	return S_OK;
}

HRESULT Aligned2DBuffer::GetContiguousLength(DWORD* length)
{
	if (length == NULL)
	{
		return E_POINTER;
	}
	*length = _maxLength;
	return S_OK;
}

HRESULT Aligned2DBuffer::ContiguousCopyTo(BYTE* destBuffer, DWORD destBufferSize)
{
	return E_NOTIMPL;
}

HRESULT Aligned2DBuffer::ContiguousCopyFrom(const BYTE* srcBuffer, DWORD srcBufferSize)
{
	return E_NOTIMPL;
}
