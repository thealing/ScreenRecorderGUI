#pragma once

#include "hook.h"

enum {
	SOURCE_TYPE_ENTIRE_WINDOW,
	SOURCE_TYPE_VISIBLE_AREA,
	SOURCE_TYPE_CLIENT_AREA,
};

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

	virtual bool get(void* buffer, SRWLOCK* lock) = 0;

	virtual int get_source_type(int type) = 0;
};

class Capture_Base : public Capture_Interface {
public:
	int get_source_type(int type) override {
		return type;
	}
};

class Capture_GDI : public Capture_Base {
public:
	HRESULT start(Capture_Source source) override {
		m_source = source;
		memset(&m_source_info, 0, sizeof(m_source_info));
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
		log_info(L"Started GDI capture: %p %p", m_source.window, m_source_context);
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
		BitBlt(m_capture_context, 0, 0, m_source.width, m_source.height, m_source_context, m_source.rect.left, m_source.rect.top, flags);
	}
};

class Capture_BitBlt_GetDIBits : public Capture_BitBlt {
public:
	bool get(void* buffer, SRWLOCK* lock) override {
		blit(SRCCOPY);
		AcquireSRWLockExclusive(lock);
		bool result = GetDIBits(m_capture_context, m_capture_bitmap, 0, m_source.height, buffer, (LPBITMAPINFO)&m_source_info, DIB_RGB_COLORS);
		ReleaseSRWLockExclusive(lock);
		return result;
	}
};

class Capture_BitBlt_GetBitmapBits : public Capture_BitBlt {
public:
	bool get(void* buffer, SRWLOCK* lock) override {
		blit(SRCCOPY);
		AcquireSRWLockExclusive(lock);
		bool result = GetBitmapBits(m_capture_bitmap, m_source.stride * m_source.height * 4, buffer);
		ReleaseSRWLockExclusive(lock);
		return result;
	}
};

class Capture_CaptureBlt_GetDIBits : public Capture_BitBlt {
public:
	bool get(void* buffer, SRWLOCK* lock) override {
		blit(SRCCOPY | CAPTUREBLT);
		AcquireSRWLockExclusive(lock);
		bool result = GetDIBits(m_capture_context, m_capture_bitmap, 0, m_source.height, buffer, (LPBITMAPINFO)&m_source_info, DIB_RGB_COLORS);
		ReleaseSRWLockExclusive(lock);
		return result;
	}
};

class Capture_CaptureBlt_GetBitmapBits : public Capture_BitBlt {
public:
	bool get(void* buffer, SRWLOCK* lock) override {
		blit(SRCCOPY | CAPTUREBLT);
		AcquireSRWLockExclusive(lock);
		bool result = GetBitmapBits(m_capture_bitmap, m_source.stride * m_source.height * 4, buffer);
		ReleaseSRWLockExclusive(lock);
		return result;
	}
};

class Capture_PrintWindow : public Capture_GDI {
public:
	int get_source_type(int type) override {
		return type == SOURCE_TYPE_VISIBLE_AREA ? SOURCE_TYPE_ENTIRE_WINDOW : type;
	}
};

class Capture_PrintWindow_GetDIBits : public Capture_PrintWindow {
public:
	bool get(void* buffer, SRWLOCK* lock) override {
		PrintWindow(m_source.window, m_capture_context, m_source.client);
		AcquireSRWLockExclusive(lock);
		bool result = GetDIBits(m_capture_context, m_capture_bitmap, 0, m_source.height, buffer, (LPBITMAPINFO)&m_source_info, DIB_RGB_COLORS);
		ReleaseSRWLockExclusive(lock);
		return result;
	}
};

class Capture_PrintWindow_GetBitmapBits : public Capture_PrintWindow {
public:
	bool get(void* buffer, SRWLOCK* lock) override {
		PrintWindow(m_source.window, m_capture_context, m_source.client);
		AcquireSRWLockExclusive(lock);
		bool result = GetBitmapBits(m_capture_bitmap, m_source.stride * m_source.height * 4, buffer);
		ReleaseSRWLockExclusive(lock);
		return result;
	}
};

class Capture_DWM_PrintWindow : public Capture_PrintWindow {
public:
	bool get(void* buffer, SRWLOCK* lock) override {
		PrintWindow(m_source.window, m_capture_context, m_source.client + 2);
		AcquireSRWLockExclusive(lock);
		bool result = GetBitmapBits(m_capture_bitmap, m_source.stride * m_source.height * 4, buffer);
		ReleaseSRWLockExclusive(lock);
		return result;
	}
};

class Capture_Source_Bitmap : public Capture_GDI {
public:
	HRESULT start(Capture_Source source) override {
		HRESULT result = Capture_GDI::start(source);
		m_source_bitmap = (HBITMAP)GetCurrentObject(m_source_context, OBJ_BITMAP);
		log_info(L"Started source bitmap capture: %p", m_source_context);
		return result;
	}

	void stop() override {
		DeleteObject(m_source_bitmap);
		Capture_GDI::stop();
	}

	int get_source_type(int type) override {
		return SOURCE_TYPE_ENTIRE_WINDOW;
	}

protected:
	HBITMAP m_source_bitmap;
};

class Capture_GetDIBits : public Capture_Source_Bitmap {
public:
	bool get(void* buffer, SRWLOCK* lock) override {
		AcquireSRWLockExclusive(lock);
		bool result = GetDIBits(m_source_context, m_source_bitmap, 0, m_source.height, buffer, (LPBITMAPINFO)&m_source_info, DIB_RGB_COLORS);
		ReleaseSRWLockExclusive(lock);
		return result;
	}
};

class Capture_GetBitmapBits : public Capture_Source_Bitmap {
public:
	bool get(void* buffer, SRWLOCK* lock) override {
		AcquireSRWLockExclusive(lock);
		bool result = GetBitmapBits(m_source_bitmap, m_source.stride * m_source.height * 4, buffer);
		ReleaseSRWLockExclusive(lock);
		return result;
	}
};

class Capture_Dimensions_Only {
protected:
	void init_dimensions(Capture_Source source) {
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

class Capture_Window_Only: public Capture_Dimensions_Only {
protected:
	void init_dimensions(Capture_Source source) {
		if (source.client) {
			get_client_window_rect(source.window, &source.rect);
		}
		else {
			get_clamped_window_rect(source.window, &source.rect);
		}
		Capture_Dimensions_Only::init_dimensions(source);
	}
};

class Capture_Fullscreen_Only : public Capture_Dimensions_Only {
protected:
	void init_dimensions(Capture_Source source) {
		Capture_Dimensions_Only::init_dimensions(source);
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

class Capture_SharedSurface : public Capture_Base, private Capture_Window_Only {
public:
	HRESULT start(Capture_Source source) override {
		HANDLE surface;
		HRESULT result = D3D11CreateDevice(0, D3D_DRIVER_TYPE_HARDWARE, NULL, D3D11_CREATE_DEVICE_BGRA_SUPPORT, NULL, 0, D3D11_SDK_VERSION, &m_device, 0, &m_context);
		if (FAILED(result)) {
			goto end;
		}
		result = get_shared_surface(source.window, &surface);
		if (FAILED(result)) {
			goto end;
		}
		result = m_device->OpenSharedResource(surface, __uuidof(ID3D11Texture2D), (void**)&m_shared_texture);
		if (FAILED(result)) {
			goto end;
		}
		m_shared_texture->GetDesc(&texture_desc);
		texture_desc.Usage = D3D11_USAGE_STAGING;
		texture_desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
		texture_desc.BindFlags = 0;
		texture_desc.MiscFlags = 0;
		result = m_device->CreateTexture2D(&texture_desc, NULL, &m_offscreen_texture);
		if (FAILED(result)) {
			goto end;
		}
	end:
		if (FAILED(result)) {
			log_error(L"Failed to get shared surface! Error: 0x%08x", result);
			return result;
		}
		init_dimensions(source);
		log_info(L"Started shared surface capture: %p", surface);
		return result;
	}

	void stop() override {
		safe_release(&m_offscreen_texture);
		safe_release(&m_shared_texture);
		safe_release(&m_context);
		safe_release(&m_device);
	}

	bool get(void* buffer, SRWLOCK* lock) override {
		m_context->CopyResource(m_offscreen_texture, m_shared_texture);
		D3D11_MAPPED_SUBRESOURCE map;
		if (FAILED(m_context->Map(m_offscreen_texture, 0, D3D11_MAP_READ, 0, &map))) {
			return false;
		}
		AcquireSRWLockExclusive(lock);
		for (int i = 0; i < min(m_rect.bottom, (int)texture_desc.Height) - m_rect.top; i++) {
			memcpy((uint32_t*)buffer + i * m_stride, (uint8_t*)map.pData + (m_rect.top + i) * map.RowPitch + m_rect.left * 4, (min(m_rect.right, (int)texture_desc.Width) - m_rect.left) * 4);
		}
		ReleaseSRWLockExclusive(lock);
		m_context->Unmap(m_offscreen_texture, 0);
		return true;
	}

protected:
	virtual HRESULT get_shared_surface(HWND window, HANDLE* surface) = 0;

protected:
	ID3D11Device* m_device = NULL;
	ID3D11DeviceContext* m_context = NULL;
	ID3D11Texture2D* m_shared_texture = NULL;
	ID3D11Texture2D* m_offscreen_texture = NULL;
	D3D11_TEXTURE2D_DESC texture_desc;
};

class Capture_DwmGetDxSharedSurface : public Capture_SharedSurface {
public:
	HRESULT get_shared_surface(HWND window, HANDLE* surface) override {
		return GetWindowSharedSurface(window, surface, NULL, NULL, NULL, NULL);
	}

private:
	typedef HRESULT (__stdcall* DwmGetDxSharedSurface)(HWND, HANDLE*, LUID*, ULONG*, ULONG*, ULONGLONG*);

	DwmGetDxSharedSurface GetWindowSharedSurface = (DwmGetDxSharedSurface)GetProcAddress(GetModuleHandle(L"user32"), "DwmGetDxSharedSurface");
};

class Capture_DwmDxGetWindowSharedSurface : public Capture_SharedSurface {
public:
	HRESULT get_shared_surface(HWND window, HANDLE* surface) override {
		IDXGIDevice* dxgi = NULL;
		IDXGIAdapter* adapter = NULL;
		IDXGIOutput* output = NULL;
		DXGI_ADAPTER_DESC adapter_desc;
		DXGI_OUTPUT_DESC output_desc;
		DWORD flags = 0x10;
		DXGI_FORMAT format = DXGI_FORMAT_B8G8R8A8_UNORM;
		HRESULT result = m_device->QueryInterface(__uuidof(IDXGIDevice), (void**)&dxgi);
		if (FAILED(result)) {
			goto end;
		}
		result = dxgi->GetAdapter(&adapter);
		if (FAILED(result)) {
			goto end;
		}
		result = adapter->GetDesc(&adapter_desc);
		if (FAILED(result)) {
			goto end;
		}
		result = adapter->EnumOutputs(0, &output);
		if (FAILED(result)) {
			goto end;
		}
		result = output->GetDesc(&output_desc);
		if (FAILED(result)) {
			goto end;
		}
		result = GetWindowSharedSurface(window, adapter_desc.AdapterLuid, output_desc.Monitor, &flags, &format, surface, NULL);
		if (result == 0x00263005) {
			result = 0;
		}
	end:
		safe_release(&output);
		safe_release(&adapter);
		safe_release(&dxgi);
		return result;
	}

private:
	typedef HRESULT (__stdcall* DwmDxGetWindowSharedSurface)(HWND, LUID, HMONITOR, DWORD*, DXGI_FORMAT*, HANDLE*, UINT64*);

	DwmDxGetWindowSharedSurface GetWindowSharedSurface = (DwmDxGetWindowSharedSurface)GetProcAddress(GetModuleHandle(L"dwmapi"), "DwmpDxGetWindowSharedSurface");
};

class Capture_DXGI_Output_Duplication : public Capture_Base, private Capture_Fullscreen_Only {
public:
	HRESULT start(Capture_Source source) override {
		IDXGIDevice* dxgi = NULL;
		IDXGIAdapter* adapter = NULL;
		IDXGIOutput* output = NULL;
		IDXGIOutput1* output_1 = NULL;
		HRESULT result = D3D11CreateDevice(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, 0, NULL, 0, D3D11_SDK_VERSION, &m_device, NULL, &m_context);
		if (FAILED(result)) {
			goto end;
		}
		result = m_device->QueryInterface(__uuidof(IDXGIDevice), (void**)&dxgi);
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
		result = output->QueryInterface(__uuidof(IDXGIOutput1), (void**)&output_1);
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
			log_error(L"Failed to initialize output duplication! Error: 0x%08x", result);
			return result;
		}
		init_dimensions(source);
		log_info(L"Started output duplication capture");
		return result;
	}

	void stop() override {
		safe_release(&m_duplication);
		safe_release(&m_context);
		safe_release(&m_device);
	}

	bool get(void* buffer, SRWLOCK* lock) override {
		IDXGIResource* desktop_resource = NULL;
		ID3D11Texture2D* desktop_image = NULL;
		ID3D11Texture2D* capture_texture = NULL;
		DXGI_OUTDUPL_FRAME_INFO desktop_info;
		CHECK(m_duplication->AcquireNextFrame(INFINITE, &desktop_info, &desktop_resource));
		if (desktop_resource == NULL) {
			return false;
		}
		CHECK(desktop_resource->QueryInterface(__uuidof(ID3D11Texture2D), (void**)&desktop_image));
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
		memcpy(buffer, map.pData, m_height * m_stride * 4);
		ReleaseSRWLockExclusive(lock);
		m_context->Unmap(capture_texture, 0);
		CHECK(m_duplication->ReleaseFrame());
		safe_release(&capture_texture);
		safe_release(&desktop_image);
		safe_release(&desktop_resource);
		return true;
	}

private:
	ID3D11Device* m_device = NULL;
	ID3D11DeviceContext* m_context = NULL;
	IDXGIOutputDuplication* m_duplication = NULL;
};

class Capture_Direct3D_9 : public Capture_Base, private Capture_Fullscreen_Only {
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
			log_error(L"Failed to initialize Direct3D 9! Error: 0x%08x", result);
			return result;
		}
		init_dimensions(source);
		log_info(L"Started Direct3D 9 capture");
		return result;
	}

	void stop() override {
		safe_release(&m_surface);
		safe_release(&m_device);
	}

	bool get(void* buffer, SRWLOCK* lock) override {
		CHECK(m_device->GetFrontBufferData(0, m_surface));
		D3DLOCKED_RECT locked_rect;
		CHECK(m_surface->LockRect(&locked_rect, &m_rect, D3DLOCK_READONLY));
		AcquireSRWLockExclusive(lock);
		for (int i = 0; i < m_height; i++) {
			memcpy((uint32_t*)buffer + i * m_stride, (uint8_t*)locked_rect.pBits + i * locked_rect.Pitch, m_width * 4);
		}
		ReleaseSRWLockExclusive(lock);
		CHECK(m_surface->UnlockRect());
		return true;
	}

protected:
	IDirect3DDevice9* m_device = NULL;
	IDirect3DSurface9* m_surface = NULL;
};

class Capture_Hook : public Capture_Base {
public:
	HRESULT start(Capture_Source source) override {
		m_width = source.rect.right;
		m_height = source.rect.bottom;
		m_stride = source.stride;
		return S_OK;
	}

	void stop() override {
	}

	bool get(void* buffer, SRWLOCK* lock) override {
		if (connected_index == -1) {
			return false;
		}
		DWORD bytes_avail;
		if (!PeekNamedPipe(pipes[connected_index], NULL, 0, NULL, &bytes_avail, NULL)) {
			return false;
		}
		if ((int)bytes_avail != m_width * m_height * 4) {
			return false;
		}
		AcquireSRWLockExclusive(lock);
		DWORD bytes_read;
		for (int i = 0; i < m_height; i++) {
			ReadFile(pipes[connected_index], (uint32_t*)buffer + i * m_stride, m_width * 4, &bytes_read, NULL);
		}
		ReleaseSRWLockExclusive(lock);
		return true;
	}

private:
	int m_width;
	int m_height;
	int m_stride;
};
