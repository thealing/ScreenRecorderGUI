#pragma once

#include "util.h"

#define PIPE_NAME L"\\\\.\\pipe\\scabture_hook_pipe_name"
#define INSTANCE_COUNT 32
#define CONNECTION_TIME 0.5

HANDLE pipes[INSTANCE_COUNT];
HANDLE events[INSTANCE_COUNT];
OVERLAPPED overlapped[INSTANCE_COUNT];
int connected_index = -1;

void injection_failed() {
	wprintf(L"Hook injection failed! Error: %d\n", GetLastError());
	MessageBeep(MB_ICONERROR);
}

bool inject_hook(HWND window, RECT* rect) {
	DWORD pid;
	DWORD tid = GetWindowThreadProcessId(window, &pid);
	if (tid == 0) {
		injection_failed();
		return false;
	}
	HANDLE process = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
	if (process == NULL) {
		injection_failed();
		return false;
	}
	HMODULE kernel_module = GetModuleHandle(L"kernel32");
	if (kernel_module == NULL) {
		injection_failed();
		return false;
	}
	FARPROC load_library = GetProcAddress(kernel_module, "LoadLibraryW");
	if (load_library == NULL) {
		injection_failed();
		return false;
	}
	wchar_t dll_path[MAX_PATH];
	GetModuleFileName(NULL, dll_path, _countof(dll_path));
	wchar_t* dll_name = wcsrchr(dll_path, '\\') + 1;
	wcscpy(dll_name, L"Hook.dll");
	if (GetFileAttributes(dll_name) == INVALID_FILE_ATTRIBUTES) {
#ifdef _WIN64
		wcscpy(dll_name, L"Hook64.dll");
#else
		wcscpy(dll_name, L"Hook32.dll");
#endif
	}
	void* virtual_address = VirtualAllocEx(process, NULL, sizeof(dll_path), MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
	if (virtual_address == NULL) {
		injection_failed();
		return false;
	}
	SIZE_T bytes_written = 0;
	WriteProcessMemory(process, virtual_address, dll_path, sizeof(dll_path), &bytes_written);
	if (bytes_written == 0) {
		injection_failed();
		return false;
	}
	HANDLE inject_thread = CreateRemoteThread(process, NULL, 0, (LPTHREAD_START_ROUTINE)load_library, virtual_address, 0, NULL);
	if (inject_thread == NULL) {
		injection_failed();
		return false;
	}
	WaitForSingleObject(inject_thread, INFINITE);
	CloseHandle(inject_thread);
	VirtualFreeEx(process, virtual_address, 0, MEM_RELEASE);
	CloseHandle(process);
	int state[INSTANCE_COUNT];
	if (connected_index != -1) {
		DisconnectNamedPipe(pipes[connected_index]);
		CloseHandle(pipes[connected_index]);
		CloseHandle(events[connected_index]);
	}
	connected_index = -1;
	for (int i = 0; i < INSTANCE_COUNT; i++) {
		pipes[i] = CreateNamedPipe(PIPE_NAME, PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED, PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT, PIPE_UNLIMITED_INSTANCES, 0, 0, 0, NULL);
		events[i] = CreateEvent(NULL, TRUE, FALSE, NULL);
		overlapped[i].Offset = 0;
		overlapped[i].OffsetHigh = 0;
		overlapped[i].Pointer = 0;
		overlapped[i].hEvent = events[i];
		ConnectNamedPipe(pipes[i], &overlapped[i]);
		state[i] = 0;
	}
	double time_out = get_time() + CONNECTION_TIME;
	while (get_time() < time_out) {
		DWORD index = WaitForMultipleObjects(INSTANCE_COUNT, events, FALSE, 0);
		if (index == WAIT_TIMEOUT) {
			continue;
		}
		ResetEvent(events[index]);
		DWORD bytes;
		if (state[index] == 0) {
			WriteFile(pipes[index], &pid, sizeof(pid), &bytes, &overlapped[index]);
			state[index] = 1;
			continue;
		}
		if (state[index] == 1) {
			ReadFile(pipes[index], rect, sizeof(*rect), &bytes, &overlapped[index]);
			state[index] = 2;
			continue;
		}
		if (state[index] == 2) {
			if (GetOverlappedResult(pipes[index], &overlapped[index], &bytes, FALSE) && bytes == 16) {
				connected_index = index;
				break;
			}
		}
	}
	for (int i = 0; i < INSTANCE_COUNT; i++) {
		if (i == connected_index) {
			continue;
		}
		DisconnectNamedPipe(pipes[i]);
		CloseHandle(pipes[i]);
		CloseHandle(events[i]);
	}
	if (connected_index == -1) {
		injection_failed();
		return false;
	}
	return true;
}
