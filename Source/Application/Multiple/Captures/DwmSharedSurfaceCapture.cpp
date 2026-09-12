#include "DwmSharedSurfaceCapture.h"

DwmSharedSurfaceCapture::DwmSharedSurfaceCapture(HWND window, POINT position, SIZE size) : WindowCapture(window)
{
	RECT clientRect = WindowUtil::getRelativeClientRect(window);
	_position.x = position.x + clientRect.left;
	_position.y = position.y + clientRect.top;
	createBuffer(size.cx, size.cy);
	const Buffer* buffer = getBuffer();
	int height = buffer->getHeight();
	int stride = buffer->getStride();
	_textureDesc = {};
	HANDLE surface = NULL;
	if (_status)
	{
		_status = D3D11CreateDevice(0, D3D_DRIVER_TYPE_HARDWARE, NULL, D3D11_CREATE_DEVICE_BGRA_SUPPORT, NULL, 0, D3D11_SDK_VERSION, &_device, 0, &_context);
	}
	if (_status)
	{
		DynamicLibrary user32("user32");
		DwmGetDxSharedSurface* dwmGetDxSharedSurface = user32.getFunction<DwmGetDxSharedSurface>("DwmGetDxSharedSurface");
		if (dwmGetDxSharedSurface != NULL)
		{
			_status = dwmGetDxSharedSurface(window, &surface, NULL, NULL, NULL, NULL);
		}
		else
		{
			_status = E_NOTFOUND;
		}
	}
	if (_status)
	{
		_status = _device->OpenSharedResource(surface, IID_PPV_ARGS(&_sharedTexture));
	}
	if (_status)
	{
		_sharedTexture->GetDesc(&_textureDesc);
		_textureDesc.Usage = D3D11_USAGE_STAGING;
		_textureDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
		_textureDesc.BindFlags = 0;
		_textureDesc.MiscFlags = 0;
		_textureDesc.Width = stride;
		_textureDesc.Height = height;
		_status = _device->CreateTexture2D(&_textureDesc, NULL, &_captureTexture);
	}
	if (!_status)
	{
		LogUtil::logComError("DwmSharedSurfaceCapture", _status);
	}
}

DwmSharedSurfaceCapture::~DwmSharedSurfaceCapture()
{
	stop();
}

bool DwmSharedSurfaceCapture::captureFrame()
{
	if (!_status)
	{
		return false;
	}
	const Buffer* buffer = getBuffer();
	int width = buffer->getWidth();
	int height = buffer->getHeight();
	int stride = buffer->getStride();
	D3D11_MAPPED_SUBRESOURCE map = {};
	Status result;
	if (result)
	{
		D3D11_BOX box = {};
		box.left = _position.x;
		box.top = _position.y;
		box.right = _position.x + width;
		box.bottom = _position.y + height;
		box.back = 1;
		_context->CopySubresourceRegion(_captureTexture, 0, 0, 0, 0, _sharedTexture, 0, &box);
		result = _context->Map(_captureTexture, 0, D3D11_MAP_READ, 0, &map);
	}
	if (result)
	{
		uint32_t* pixels = beginFrame();
		memcpy(pixels, map.pData, height * stride * sizeof(uint32_t));
		endFrame();
		_context->Unmap(_captureTexture, 0);
	}
	return result;
}
