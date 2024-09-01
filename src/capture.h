#pragma once

#include "util.h"

struct Capture_Source {
	HWND window;
	bool client;
	RECT rect;
	int width;
	int height;
	int stride;
};

class Capture_Interface {
public:
	virtual HRESULT start(Capture_Source source) = 0;

	virtual void stop() = 0;

	virtual void get(void* buffer, SRWLOCK* lock) = 0;
};

class Capture_GDI : public Capture_Interface {
public:
	HRESULT start(Capture_Source source) override {
		m_source = source;
		m_source_info.biSize = sizeof(m_source_info);
		m_source_info.biWidth = m_source.stride;
		m_source_info.biHeight = -m_source.height;
		m_source_info.biPlanes = 1;
		m_source_info.biBitCount = 32;
		m_source_info.biSizeImage = m_source.stride * m_source.height * 4;
		m_source_info.biCompression = BI_RGB;
		m_source_context = m_source.client ? GetDC(m_source.window) : GetWindowDC(m_source.window);
		m_capture_context = CreateCompatibleDC(m_source_context);
		m_capture_bitmap = CreateCompatibleBitmap(m_source_context, m_source.stride, m_source.height);
		SelectObject(m_capture_context, m_capture_bitmap);
		return S_OK;
	}

	void stop() override {
		DeleteObject(m_capture_bitmap);
		DeleteDC(m_capture_context);
		ReleaseDC(m_source.window, m_source_context);
	}

protected:
	Capture_Source m_source;
	BITMAPINFOHEADER m_source_info;
	HDC m_source_context;
	HDC m_capture_context;
	HBITMAP m_capture_bitmap;
};

class Capture_BitBlt : public Capture_GDI {
protected:
	void blit(DWORD flags) {
		BitBlt(m_capture_context, 0, 0, m_source.width, m_source.height, m_source_context, m_source.rect.left,  m_source.rect.top, flags);
	}
};

class Capture_BitBlt_GetDIBits : public Capture_BitBlt {
public:
	void get(void* buffer, SRWLOCK* lock) override {
		blit(SRCCOPY);
		AcquireSRWLockExclusive(lock);
		GetDIBits(m_capture_context, m_capture_bitmap, 0, m_source.height, buffer, (LPBITMAPINFO)&m_source_info, DIB_RGB_COLORS);
		ReleaseSRWLockExclusive(lock);
	}
};

class Capture_BitBlt_GetBitmapBits : public Capture_BitBlt {
public:
	void get(void* buffer, SRWLOCK* lock) override {
		blit(SRCCOPY);
		AcquireSRWLockExclusive(lock);
		GetBitmapBits(m_capture_bitmap, m_source.stride * m_source.height * 4, buffer);
		ReleaseSRWLockExclusive(lock);
	}
};

class Capture_CaptureBlt_GetDIBits : public Capture_BitBlt {
public:
	void get(void* buffer, SRWLOCK* lock) override {
		blit(SRCCOPY | CAPTUREBLT);
		AcquireSRWLockExclusive(lock);
		GetDIBits(m_capture_context, m_capture_bitmap, 0, m_source.height, buffer, (LPBITMAPINFO)&m_source_info, DIB_RGB_COLORS);
		ReleaseSRWLockExclusive(lock);
	}
};

class Capture_CaptureBlt_GetBitmapBits : public Capture_BitBlt {
public:
	void get(void* buffer, SRWLOCK* lock) override {
		blit(SRCCOPY | CAPTUREBLT);
		AcquireSRWLockExclusive(lock);
		GetBitmapBits(m_capture_bitmap, m_source.stride * m_source.height * 4, buffer);
		ReleaseSRWLockExclusive(lock);
	}
};

class Capture_PrintWindow_GetDIBits : public Capture_GDI {
public:
	void get(void* buffer, SRWLOCK* lock) override {
		PrintWindow(m_source.window, m_capture_context, m_source.client);
		AcquireSRWLockExclusive(lock);
		GetDIBits(m_capture_context, m_capture_bitmap, 0, m_source.height, buffer, (LPBITMAPINFO)&m_source_info, DIB_RGB_COLORS);
		ReleaseSRWLockExclusive(lock);
	}
};

class Capture_PrintWindow_GetBitmapBits : public Capture_GDI {
public:
	void get(void* buffer, SRWLOCK* lock) override {
		PrintWindow(m_source.window, m_capture_context, m_source.client);
		AcquireSRWLockExclusive(lock);
		GetBitmapBits(m_capture_bitmap, m_source.stride * m_source.height * 4, buffer);
		ReleaseSRWLockExclusive(lock);
	}
};

class Capture_Source_Bitmap : public Capture_GDI {
public:
	HRESULT start(Capture_Source source) override {
		HRESULT result = Capture_GDI::start(source);
		m_source_bitmap = (HBITMAP)GetCurrentObject(m_source_context, OBJ_BITMAP);
		return result;
	}

	void stop() override {
		DeleteObject(m_source_bitmap);
		Capture_GDI::stop();
	}

protected:
	HBITMAP m_source_bitmap;
};

class Capture_GetDIBits : public Capture_Source_Bitmap {
public:
	void get(void* buffer, SRWLOCK* lock) override {
		AcquireSRWLockExclusive(lock);
		GetDIBits(m_source_context, m_source_bitmap, 0, m_source.height, buffer, (LPBITMAPINFO)&m_source_info, DIB_RGB_COLORS);
		ReleaseSRWLockExclusive(lock);
	}
};

class Capture_GetBitmapBits : public Capture_Source_Bitmap {
public:
	void get(void* buffer, SRWLOCK* lock) override {
		AcquireSRWLockExclusive(lock);
		GetBitmapBits(m_source_bitmap, m_source.stride * m_source.height * 4, buffer);
		ReleaseSRWLockExclusive(lock);
	}
};

class Capture_Dimensions_Only {
protected:
	void init(Capture_Source source) {
		m_rect = source.rect;
		m_width = source.width;
		m_height = source.height;
		m_stride = source.stride;
	}

protected:
	RECT m_rect;
	int m_width;
	int m_height;
	int m_stride;
};

class Capture_Fullscreen_Only : protected Capture_Dimensions_Only {
protected:
	void init_dimensions(Capture_Source source) {
		Capture_Dimensions_Only::init(source);
		if (source.window != GetDesktopWindow()) {
			if (source.client) {
				POINT origin = { 0, 0 };
				ClientToScreen(source.window, &origin);
				OffsetRect(&m_rect, origin.x, origin.y);
			}
			else {
				RECT origin_rect;
				GetWindowRect(source.window, &origin_rect);
				OffsetRect(&m_rect, origin_rect.left, origin_rect.top);
			}
			clamp_window_rect(&m_rect);
		}
	}
};

class Capture_DXGI_Output_Duplication : public Capture_Interface, protected Capture_Fullscreen_Only {
public:
	HRESULT start(Capture_Source source) override {
		IDXGIDevice* dxgi = NULL;
		IDXGIAdapter* adapter = NULL;
		IDXGIOutput* output = NULL;
		IDXGIOutput1* output_1 = NULL;
		HRESULT result = D3D11CreateDevice(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, 0, NULL, 0, D3D11_SDK_VERSION, &m_device, NULL, &m_context);
		if (FAILED(result)) {
			return result;
		}
		result = m_device->QueryInterface(__uuidof(IDXGIDevice), reinterpret_cast<void**>(&dxgi));
		if (FAILED(result)) {
			goto end;
		}
		result = dxgi->GetAdapter(&adapter);
		if (FAILED(result)) {
			goto end;
		}
		result = adapter->EnumOutputs(0, &output);
		if (FAILED(result)) {
			goto end;
		}
		result = output->QueryInterface(__uuidof(IDXGIOutput1), reinterpret_cast<void**>(&output_1));
		if (FAILED(result)) {
			goto end;
		}
		result = output_1->DuplicateOutput(m_device, &m_duplication);
		if (FAILED(result)) {
			goto end;
		}
	end:
		safe_release(&output_1);
		safe_release(&output);
		safe_release(&adapter);
		safe_release(&dxgi);
		if (FAILED(result)) {
			MessageBox(NULL, L"Failed to initialize output duplication!", L"DXGI error", MB_ICONERROR);
			return result;
		}
		init_dimensions(source);
		return result;
	}

	void stop() override {
		safe_release(&m_duplication);
		safe_release(&m_context);
		safe_release(&m_device);
	}

	void get(void* buffer, SRWLOCK* lock) override {
		IDXGIResource* desktop_resource = NULL;
		ID3D11Texture2D* desktop_image = NULL;
		ID3D11Texture2D* capture_texture = NULL;
		DXGI_OUTDUPL_FRAME_INFO desktop_info;
		CHECK(m_duplication->AcquireNextFrame(INFINITE, &desktop_info, &desktop_resource));
		if (desktop_resource == NULL) {
			return;
		}
		CHECK(desktop_resource->QueryInterface(__uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&desktop_image)));
		D3D11_TEXTURE2D_DESC desc;
		desktop_image->GetDesc(&desc);
		desc.Usage = D3D11_USAGE_STAGING;
		desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
		desc.BindFlags = 0;
		desc.MiscFlags = 0;
		desc.Width = m_stride;
		desc.Height = m_height;
		CHECK(m_device->CreateTexture2D(&desc, NULL, &capture_texture));
		D3D11_BOX box = {};
		box.left = m_rect.left;
		box.top = m_rect.top;
		box.right = m_rect.right;
		box.bottom = m_rect.bottom;
		m_context->CopySubresourceRegion(capture_texture, 0, 0, 0, 0, desktop_image, 0, &box);
		D3D11_MAPPED_SUBRESOURCE map;
		CHECK(m_context->Map(capture_texture, 0, D3D11_MAP_READ, 0, &map));
		AcquireSRWLockExclusive(lock);
		memcpy(buffer, map.pData, desc.Height * map.RowPitch);
		ReleaseSRWLockExclusive(lock);
		m_context->Unmap(capture_texture, 0);
		CHECK(m_duplication->ReleaseFrame());
		safe_release(&capture_texture);
		safe_release(&desktop_image);
		safe_release(&desktop_resource);
	}

private:
	ID3D11Device* m_device = NULL;
	ID3D11DeviceContext* m_context = NULL;
	IDXGIOutputDuplication* m_duplication = NULL;
};

class Capture_Direct3D_9 : public Capture_Interface, protected Capture_Fullscreen_Only {
public:
	HRESULT start(Capture_Source source) override {
		HRESULT result = S_OK;
		D3DPRESENT_PARAMETERS params = {};
		D3DDISPLAYMODE display_mode = {};
		IDirect3D9* d3d = Direct3DCreate9(D3D_SDK_VERSION);
		if (FAILED(result)) {
			result = E_FAIL;
			goto end;
		}
		result = d3d->GetAdapterDisplayMode(D3DADAPTER_DEFAULT, &display_mode);
		if (FAILED(result)) {
			goto end;
		}
		params.Windowed = TRUE;
		params.SwapEffect = D3DSWAPEFFECT_DISCARD;
		params.Flags = D3DPRESENTFLAG_LOCKABLE_BACKBUFFER;
		params.BackBufferFormat = display_mode.Format;
		params.BackBufferWidth = display_mode.Width;
		params.BackBufferHeight = display_mode.Height;
		result = d3d->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, GetDesktopWindow(), D3DCREATE_HARDWARE_VERTEXPROCESSING, &params, &m_device);
		if (FAILED(result)) {
			goto end;
		}
		result = m_device->CreateOffscreenPlainSurface(display_mode.Width, display_mode.Height, D3DFMT_A8R8G8B8, D3DPOOL_SYSTEMMEM, &m_surface, NULL);
		if (FAILED(result)) {
			goto end;
		}
	end:
		safe_release(&d3d);
		if (FAILED(result)) {
			MessageBox(NULL, L"Failed to initialize Direct3D 9!", L"Direct3D error", MB_ICONERROR);
			return result;
		}
		init_dimensions(source);
		return result;
	}

	void stop() override {
		safe_release(&m_surface);
		safe_release(&m_device);
	}

	void get(void* buffer, SRWLOCK* lock) override {
		CHECK(m_device->GetFrontBufferData(0, m_surface));
		D3DLOCKED_RECT locked_rect;
		CHECK(m_surface->LockRect(&locked_rect, &m_rect, D3DLOCK_READONLY));
		AcquireSRWLockExclusive(lock);
		for (int i = 0; i < m_height; i++) {
			memcpy((uint32_t*)buffer + i * m_stride, (uint8_t*)locked_rect.pBits + i * locked_rect.Pitch, m_width * 4);
		}
		ReleaseSRWLockExclusive(lock);
		CHECK(m_surface->UnlockRect());
	}

protected:
	IDirect3DDevice9* m_device = NULL;
	IDirect3DSurface9* m_surface = NULL;
};
