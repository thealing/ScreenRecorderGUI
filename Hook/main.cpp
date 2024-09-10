#include <stdio.h>
#include <stdint.h>
#include <windows.h>
#include <gl/gl.h>
#include <d3d9.h>
#include <d3d10_1.h>
#include <d3d11.h>

#pragma comment(lib, "opengl32.lib")
#pragma comment(lib, "d3d9.lib")
#pragma comment(lib, "d3d10.lib")
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")

#define PIPE_NAME L"\\\\.\\pipe\\scabture_hook_pipe_name"

struct OpenGL_Data {
	HANDLE init_event;
	HANDLE frame_event;
	uint8_t* swap_buffers;
	int viewport[4];
	void* pixels;
	bool ready;
};

struct D3D9_Data {
	HANDLE init_event;
	HANDLE frame_event;
	uint8_t* present;
	IDirect3DDevice9* device;
	IDirect3DSurface9* back_buffer;
	IDirect3DSurface9* middle_buffer;
	IDirect3DSurface9* capture_buffer;
	D3DLOCKED_RECT rect;
	bool ready;
};

struct D3D10_Data {
	HANDLE init_event;
	HANDLE frame_event;
	uintptr_t present;
	ID3D10Device* device;
	ID3D10Texture2D* back_buffer;
	ID3D10Texture2D* capture_buffer;
	D3D10_MAPPED_TEXTURE2D map;
	bool ready;
};

struct D3D11_Data {
	HANDLE init_event;
	HANDLE frame_event;
	uintptr_t present;
	ID3D11Device* device;
	ID3D11DeviceContext* context;
	ID3D11Texture2D* back_buffer;
	ID3D11Texture2D* capture_buffer;
	D3D11_MAPPED_SUBRESOURCE map;
	bool ready;
};

bool once() {
	static long a = 0;
	return InterlockedCompareExchange(&a, 1, 0) == 0;
}

BOOL opengl_do_swap_buffers(OpenGL_Data* data, HDC hdc) {
	while (once()) {
		glGetIntegerv(GL_VIEWPORT, data->viewport);
		data->pixels = malloc(data->viewport[2] * data->viewport[3] * 4);
		SetEvent(data->init_event);
	}
	BOOL result = ((BOOL (WINAPI *)(HDC))data->swap_buffers)(hdc);
	if (data->ready) {
		glReadPixels(data->viewport[0], data->viewport[1], data->viewport[2], data->viewport[3], GL_BGRA_EXT, GL_UNSIGNED_BYTE, data->pixels);
		data->ready = false;
		SetEvent(data->frame_event);
	}
	return result;
}

HRESULT d3d9_do_present(D3D9_Data* data, IDirect3DDevice9* device, const RECT* arg_1, const RECT* arg_2, HWND arg_3, const RGNDATA* arg_4) {
	while (once()) {
		data->device = device;
		if (FAILED(data->device->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &data->back_buffer))) {
			break;
		}
		D3DSURFACE_DESC sd;
		if (FAILED(data->back_buffer->GetDesc(&sd))) {
			break;
		}
		if (sd.MultiSampleType != D3DMULTISAMPLE_NONE) {
			if (FAILED(data->device->CreateRenderTarget(sd.Width, sd.Height, sd.Format, D3DMULTISAMPLE_NONE, 0, FALSE, &data->middle_buffer, NULL))) {
				break;
			}
		}
		if (FAILED(data->device->CreateOffscreenPlainSurface(sd.Width, sd.Height, sd.Format, D3DPOOL_SYSTEMMEM, &data->capture_buffer, NULL))) {
			break;
		}
		SetEvent(data->init_event);
	}
	HRESULT result = ((HRESULT (WINAPI *)(IDirect3DDevice9*, const RECT*, const RECT*, HWND, const RGNDATA*))data->present)(device, arg_1, arg_2, arg_3, arg_4);
	if (data->ready) {
		data->ready = false;
		SetEvent(data->frame_event);
	}
	return result;
}

HRESULT d3d10_do_present(D3D10_Data* data, IDXGISwapChain* swap_chain, UINT arg_1, UINT arg_2) {
	while (once()) {
		DXGI_SWAP_CHAIN_DESC sd;
		if (FAILED(swap_chain->GetDesc(&sd))) {
			break;
		}
		if (FAILED(swap_chain->GetDevice(IID_PPV_ARGS(&data->device)))) {
			break;
		}
		if (FAILED(swap_chain->GetBuffer(0, IID_PPV_ARGS(&data->back_buffer)))) {
			break;
		}
		D3D10_TEXTURE2D_DESC desc = {};
		desc.Width = sd.BufferDesc.Width;
		desc.Height = sd.BufferDesc.Height;
		desc.Format = sd.BufferDesc.Format;
		desc.SampleDesc = sd.SampleDesc;
		desc.MipLevels = 1;
		desc.ArraySize = 1;
		desc.Usage = D3D10_USAGE_STAGING;
		desc.CPUAccessFlags = D3D10_CPU_ACCESS_READ;
		if (FAILED(data->device->CreateTexture2D(&desc, NULL, &data->capture_buffer))) {
			break;
		}
		SetEvent(data->init_event);
	}
	HRESULT result = ((HRESULT (WINAPI *)(IDXGISwapChain*, UINT, UINT))data->present)(swap_chain, arg_1, arg_2);
	if (data->ready) {
		data->ready = false;
		SetEvent(data->frame_event);
	}
	return result;
}

HRESULT d3d11_do_present(D3D11_Data* data, IDXGISwapChain* swap_chain, UINT arg_1, UINT arg_2) {
	while (once()) {
		DXGI_SWAP_CHAIN_DESC sd;
		if (FAILED(swap_chain->GetDesc(&sd))) {
			break;
		}
		if (FAILED(swap_chain->GetDevice(IID_PPV_ARGS(&data->device)))) {
			break;
		}
		data->device->GetImmediateContext(&data->context);
		if (FAILED(swap_chain->GetBuffer(0, IID_PPV_ARGS(&data->back_buffer)))) {
			break;
		}
		D3D11_TEXTURE2D_DESC dd;
		data->back_buffer->GetDesc(&dd);
		D3D11_TEXTURE2D_DESC desc = {};
		desc.Width = sd.BufferDesc.Width;
		desc.Height = sd.BufferDesc.Height;
		desc.Format = sd.BufferDesc.Format;
		desc.SampleDesc = sd.SampleDesc;
		desc.MipLevels = 1;
		desc.ArraySize = 1;
		desc.Usage = D3D11_USAGE_STAGING;
		desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
		if (FAILED(data->device->CreateTexture2D(&desc, NULL, &data->capture_buffer))) {
			break;
		}
		SetEvent(data->init_event);
	}
	HRESULT result = ((HRESULT (WINAPI *)(IDXGISwapChain*, UINT, UINT))data->present)(swap_chain, arg_1, arg_2);
	if (data->ready) {
		data->context->CopyResource(data->capture_buffer, data->back_buffer);
		if (FAILED(data->context->Map(data->capture_buffer, 0, D3D11_MAP_READ, 0, &data->map))) {
			return result;
		}
		data->ready = false;
		SetEvent(data->frame_event);
	}
	return result;
}

OpenGL_Data opengl_data;
D3D9_Data d3d9_data;
D3D10_Data d3d10_data;
D3D11_Data d3d11_data;

BOOL WINAPI opengl_fake_swap_buffers(HDC hdc) {
	return opengl_do_swap_buffers(&opengl_data, hdc);
}

HRESULT WINAPI d3d9_fake_present(IDirect3DDevice9* device, const RECT* arg_1, const RECT* arg_2, HWND arg_3, const RGNDATA* arg_4) {
	return d3d9_do_present(&d3d9_data, device, arg_1, arg_2, arg_3, arg_4);
}

HRESULT WINAPI d3d10_fake_present(IDXGISwapChain* swap_chain, UINT arg_1, UINT arg_2) {
	return d3d10_do_present(&d3d10_data, swap_chain, arg_1, arg_2);
}

HRESULT WINAPI d3d11_fake_present(IDXGISwapChain* swap_chain, UINT arg_1, UINT arg_2) {
	return d3d11_do_present(&d3d11_data, swap_chain, arg_1, arg_2);
}

HANDLE connect(RECT rect) {
	if (!WaitNamedPipe(PIPE_NAME, 0)) {
		return NULL;
	}
	HANDLE pipe = CreateFile(PIPE_NAME, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
	if (pipe == INVALID_HANDLE_VALUE) {
		return NULL;
	}
	DWORD pid;
	DWORD bytes_read;
	if (!ReadFile(pipe, &pid, sizeof(pid), &bytes_read, NULL) || bytes_read != sizeof(pid)) {
		CloseHandle(pipe);
		return NULL;
	}
	if (pid != GetCurrentProcessId()) {
		CloseHandle(pipe);
		return NULL;
	}
	DWORD bytes_written;
	if (!WriteFile(pipe, &rect, sizeof(rect), &bytes_written, NULL) || bytes_written != sizeof(rect)) {
		CloseHandle(pipe);
		return NULL;
	}
	return pipe;
}

void opengl_proc(OpenGL_Data* data) {
	WaitForSingleObject(data->init_event, INFINITE);
	RECT rect = { 0, 0, data->viewport[2], data->viewport[3] };
	DWORD row_size = rect.right * 4;
	DWORD size = row_size * rect.bottom;
	uint8_t* flipped_pixels = (uint8_t*)malloc(size);
	HANDLE pipe = NULL;
	while (true) {
		while (pipe == NULL) {
			Sleep(50);
			pipe = connect(rect);
		}
		data->ready = true;
		WaitForSingleObject(data->frame_event, INFINITE);
		for (int i = 0; i < rect.bottom; i++) {
			memcpy(flipped_pixels + (rect.bottom - 1 - i) * row_size, (uint8_t*)data->pixels + i * row_size, row_size);
		}
		DWORD bytes_to_write = size;
		DWORD bytes_written;
		if (!WriteFile(pipe, flipped_pixels, bytes_to_write, &bytes_written, NULL) || bytes_written != bytes_to_write) {
			CloseHandle(pipe);
			pipe = NULL;
		}
	}
}

void d3d9_proc(D3D9_Data* data) {
	WaitForSingleObject(data->init_event, INFINITE);
	D3DSURFACE_DESC desc;
	data->capture_buffer->GetDesc(&desc);
	RECT rect = { 0, 0, (int)desc.Width, (int)desc.Height };
	HANDLE pipe = NULL;
	while (true) {
		while (pipe == NULL) {
			Sleep(50);
			pipe = connect(rect);
		}
		data->ready = true;
		WaitForSingleObject(data->frame_event, INFINITE);
		if (data->middle_buffer != NULL) {
			if (FAILED(data->device->StretchRect(data->back_buffer, NULL, data->middle_buffer, NULL, D3DTEXF_NONE))) {
				continue;
			}
			if (FAILED(data->device->GetRenderTargetData(data->middle_buffer, data->capture_buffer))) {
				continue;
			}
		}
		else {
			if (FAILED(data->device->GetRenderTargetData(data->back_buffer, data->capture_buffer))) {
				continue;
			}
		}
		if (FAILED(data->capture_buffer->LockRect(&data->rect, NULL, MAXDWORD))) {
			continue;
		}
		DWORD bytes_to_write = desc.Width * desc.Height * 4;
		DWORD bytes_written;
		if (!WriteFile(pipe, data->rect.pBits, bytes_to_write, &bytes_written, NULL) || bytes_written != bytes_to_write) {
			CloseHandle(pipe);
			pipe = NULL;
		}
		data->capture_buffer->UnlockRect();
	}
}

void d3d10_proc(D3D10_Data* data) {
	WaitForSingleObject(data->init_event, INFINITE);
	D3D10_TEXTURE2D_DESC desc;
	data->capture_buffer->GetDesc(&desc);
	RECT rect = { 0, 0, (int)desc.Width, (int)desc.Height };
	HANDLE pipe = NULL;
	while (true) {
		while (pipe == NULL) {
			Sleep(50);
			pipe = connect(rect);
		}
		data->ready = true;
		WaitForSingleObject(data->frame_event, INFINITE);
		data->device->CopyResource(data->capture_buffer, data->back_buffer);
		if (FAILED(data->capture_buffer->Map(0, D3D10_MAP_READ, 0, &data->map))) {
			continue;
		}
		DWORD bytes_to_write = desc.Width * desc.Height * 4;
		DWORD bytes_written;
		if (!WriteFile(pipe, data->map.pData, bytes_to_write, &bytes_written, NULL) || bytes_written != bytes_to_write) {
			CloseHandle(pipe);
			pipe = NULL;
		}
		data->capture_buffer->Unmap(0);
	}
}

void d3d11_proc(D3D11_Data* data) {
	WaitForSingleObject(data->init_event, INFINITE);
	D3D11_TEXTURE2D_DESC desc;
	data->capture_buffer->GetDesc(&desc);
	RECT rect = { 0, 0, (int)desc.Width, (int)desc.Height };
	HANDLE pipe = NULL;
	while (true) {
		while (pipe == NULL) {
			Sleep(50);
			pipe = connect(rect);
		}
		data->ready = true;
		WaitForSingleObject(data->frame_event, INFINITE);
		DWORD bytes_to_write = desc.Width * desc.Height * 4;
		DWORD bytes_written;
		if (!WriteFile(pipe, data->map.pData, bytes_to_write, &bytes_written, NULL) || bytes_written != bytes_to_write) {
			CloseHandle(pipe);
			pipe = NULL;
		}
		data->context->Unmap(data->capture_buffer, 0);
	}
}

uint8_t* write_hook(uint8_t* real_function, uint8_t* fake_function) {
#ifdef _WIN64
	uint8_t code[15] = { 0x48, 0xB8, 0, 0, 0, 0, 0, 0, 0, 0, 0xFF, 0xE0 };
	int offset = 2;
#else
	uint8_t code[8] = { 0xB8, 0, 0, 0, 0, 0xFF, 0xE0 };
	int offset = 1;
#endif
	uint8_t* real_start = real_function + sizeof(code);
	DWORD old_protect;
	if (!VirtualProtect(real_function, sizeof(code), PAGE_EXECUTE_READWRITE, &old_protect)) {
		return NULL;
	}
	uint8_t* temp = (uint8_t*)VirtualAlloc(0, sizeof(code) * 2, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
	memcpy(temp, real_function, sizeof(code));
	memcpy(temp + sizeof(code), code, sizeof(code));
	memcpy(temp + sizeof(code) + offset, &real_start, sizeof(uint8_t*));
	memcpy(real_function, code, sizeof(code));
	memcpy(real_function + offset, &fake_function, sizeof(uint8_t*));
	VirtualProtect(real_function, sizeof(uintptr_t), old_protect, &old_protect);
	return temp;
}

void opengl_init() {
	opengl_data.init_event = CreateEvent(NULL, FALSE, FALSE, NULL);
	opengl_data.frame_event = CreateEvent(NULL, FALSE, FALSE, NULL);
	HMODULE opengl_module = LoadLibrary(L"opengl32");
	FARPROC wgl_swap_buffers = GetProcAddress(opengl_module, "wglSwapBuffers");
	if (wgl_swap_buffers == NULL) {
		return;
	}
	opengl_data.swap_buffers = write_hook((uint8_t*)wgl_swap_buffers, (uint8_t*)opengl_fake_swap_buffers);
	CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)opengl_proc, &opengl_data, 0, NULL);
}

void d3d9_init() {
	d3d9_data.init_event = CreateEvent(NULL, FALSE, FALSE, NULL);
	d3d9_data.frame_event = CreateEvent(NULL, FALSE, FALSE, NULL);
	IDirect3D9* d3d = Direct3DCreate9(D3D_SDK_VERSION);
	if (!d3d) {
		return;
	}
	D3DPRESENT_PARAMETERS params = {};
	params.BackBufferWidth = 128;
	params.BackBufferHeight = 128;
	params.BackBufferFormat = D3DFMT_A8R8G8B8;
	params.BackBufferCount = 1;
	params.SwapEffect = D3DSWAPEFFECT_DISCARD;
	params.Windowed = TRUE;
	IDirect3DDevice9* device;
	HRESULT result = d3d->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_NULLREF, NULL, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &params, &device);
	if (FAILED(result)) {
		return;
	}
	uintptr_t* vtable = *(uintptr_t**)device;
	uint8_t* present = (uint8_t*)vtable[17];
	d3d9_data.present = write_hook(present, (uint8_t*)d3d9_fake_present);
	device->Release();
	d3d->Release();
	CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)d3d9_proc, &d3d9_data, 0, NULL);
}

void d3d10_init() {
	d3d10_data.init_event = CreateEvent(NULL, FALSE, FALSE, NULL);
	d3d10_data.frame_event = CreateEvent(NULL, FALSE, FALSE, NULL);
	ID3D10Device* device;
	IDXGISwapChain* swap_chain;
	DXGI_SWAP_CHAIN_DESC desc = {};
	desc.BufferCount = 1;
	desc.BufferDesc.Width = 128;
	desc.BufferDesc.Height = 128;
	desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	desc.SampleDesc.Count = 1;
	desc.OutputWindow = CreateWindow(L"STATIC", L"", 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL);
	desc.Windowed = TRUE;
	HRESULT result = D3D10CreateDeviceAndSwapChain(NULL, D3D10_DRIVER_TYPE_HARDWARE, NULL, 0, D3D10_SDK_VERSION, &desc, &swap_chain, &device);
	if (FAILED(result)) {
		return;
	}
	uintptr_t* vtable = *(uintptr_t**)swap_chain;
	uintptr_t* present = &vtable[8];
	DWORD old_protect;
	if (!VirtualProtect(present, sizeof(uintptr_t), PAGE_EXECUTE_READWRITE, &old_protect)) {
		return;
	}
	d3d10_data.present = *present;
	*present = (uintptr_t)d3d10_fake_present;
	VirtualProtect(present, sizeof(uintptr_t), old_protect, &old_protect);
	swap_chain->Release();
	device->Release();
	CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)d3d10_proc, &d3d10_data, 0, NULL);
}

void d3d11_init() {
	d3d11_data.init_event = CreateEvent(NULL, FALSE, FALSE, NULL);
	d3d11_data.frame_event = CreateEvent(NULL, FALSE, FALSE, NULL);
	ID3D11Device* device;
	ID3D11DeviceContext* context;
	IDXGISwapChain* swap_chain;
	DXGI_SWAP_CHAIN_DESC desc = {};
	desc.BufferCount = 1;
	desc.BufferDesc.Width = 128;
	desc.BufferDesc.Height = 128;
	desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	desc.SampleDesc.Count = 1;
	desc.OutputWindow = CreateWindow(L"STATIC", L"", 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL);
	desc.Windowed = TRUE;
	HRESULT result = D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, 0, NULL, 0, D3D11_SDK_VERSION, &desc, &swap_chain, &device, NULL, &context);
	if (FAILED(result)) {
		return;
	}
	uintptr_t* vtable = *(uintptr_t**)swap_chain;
	uintptr_t* present = &vtable[8];
	DWORD old_protect;
	if (!VirtualProtect(present, sizeof(uintptr_t), PAGE_EXECUTE_READWRITE, &old_protect)) {
		return;
	}
	d3d11_data.present = *present;
	*present = (uintptr_t)d3d11_fake_present;
	VirtualProtect(present, sizeof(uintptr_t), old_protect, &old_protect);
	swap_chain->Release();
	context->Release();
	device->Release();
	CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)d3d11_proc, &d3d11_data, 0, NULL);
}

void hook_proc() {
	opengl_init();
	d3d9_init();
	d3d10_init();
	d3d11_init();
}

BOOL WINAPI DllMain(HINSTANCE, DWORD action, LPVOID) {
	if (action == DLL_PROCESS_ATTACH) {
		CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)hook_proc, NULL, 0, NULL);
	}
	return TRUE;
}
