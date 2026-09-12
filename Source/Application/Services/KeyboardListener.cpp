#include "KeyboardListener.h"

static KeyboardListener* _keyboardListenerInstance;

KeyboardListener::KeyboardListener()
{
	assert(_keyboardListenerInstance == NULL);
	_keyboardListenerInstance = this;
	HMODULE module = GetModuleHandle(NULL);
	_settings = {};
	_hook = SetWindowsHookEx(WH_KEYBOARD_LL, hookProc, module, 0);
}

KeyboardListener::~KeyboardListener()
{
	assert(_keyboardListenerInstance == this);
	UnhookWindowsHookEx(_hook);
	_keyboardListenerInstance = NULL;
}

bool KeyboardListener::setSettings(const KeyboardSettings& settings)
{
	if (MemoryUtil::areEqual(_settings, settings))
	{
		return false;
	}
	_settings = settings;
	return true;
}

const Event* KeyboardListener::getStartEvent() const
{
	return &_startEvent;
}

const Event* KeyboardListener::getStopEvent() const
{
	return &_stopEvent;
}

const Event* KeyboardListener::getPauseEvent() const
{
	return &_pauseEvent;
}

const Event* KeyboardListener::getResumeEvent() const
{
	return &_resumeEvent;
}

const Event* KeyboardListener::getSnapshotEvent() const
{
	return &_snapshotEvent;
}

const Event* KeyboardListener::getRefreshEvent() const
{
	return &_refreshEvent;
}

void KeyboardListener::handleInput(const Hotkey& hotkey)
{
	if (MemoryUtil::areEqual(_settings.startHotkey, hotkey))
	{
		_startEvent.set();
	}
	if (MemoryUtil::areEqual(_settings.stopHotkey, hotkey))
	{
		_stopEvent.set();
	}
	if (MemoryUtil::areEqual(_settings.pauseHotkey, hotkey))
	{
		_pauseEvent.set();
	}
	if (MemoryUtil::areEqual(_settings.resumeHotkey, hotkey))
	{
		_resumeEvent.set();
	}
	if (MemoryUtil::areEqual(_settings.snapshotHotkey, hotkey))
	{
		_snapshotEvent.set();
	}
	if (MemoryUtil::areEqual(_settings.refreshHotkey, hotkey))
	{
		_refreshEvent.set();
	}
}

LRESULT KeyboardListener::hookProc(int code, WPARAM wParam, LPARAM lParam)
{
	if (code == HC_ACTION && wParam == WM_KEYDOWN)
	{
		PKBDLLHOOKSTRUCT keyData = (PKBDLLHOOKSTRUCT)lParam;
		wchar_t key = (wchar_t)MapVirtualKey(keyData->vkCode, MAPVK_VK_TO_CHAR);
		if (iswalnum(key))
		{
			Hotkey hotkey = {};
			hotkey.key = key;
			hotkey.shift = GetKeyState(VK_SHIFT) & 0x8000;
			hotkey.control = GetKeyState(VK_CONTROL) & 0x8000;
			hotkey.alt = GetKeyState(VK_MENU) & 0x8000;
			_keyboardListenerInstance->handleInput(hotkey);
		}
	}
	return CallNextHookEx(NULL, code, wParam, lParam);
}
