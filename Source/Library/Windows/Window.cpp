#include "Window.h"

Window::Window()
{
	_handle = NULL;
	_context = NULL;
	_renderContext = NULL;
	_renderBitmap = NULL;
	_foregroundColor = 0;
	_backgroundColor = 255;
}

Window::~Window()
{
	removeCallback(windowProc);
	destroyRenderObjects();
	ReleaseDC(_handle, _context);
	DestroyWindow(_handle);
}

void Window::show()
{
	showState(SW_SHOW);
}

void Window::hide()
{
	showState(SW_HIDE);
}

void Window::showState(int state)
{
	ShowWindow(_handle, state);
	UpdateWindow(_handle);
}

void Window::activate()
{
	BringWindowToTop(_handle);
}

void Window::close()
{
	postMessage(WM_QUIT, 0, 0);
}

void Window::setEnabled(bool enabled)
{
	EnableWindow(_handle, enabled);
}

void Window::setRedraw(bool redraw)
{
	sendMessage(WM_SETREDRAW, redraw, 0);
}

void Window::repaint()
{
	RedrawWindow(_handle, NULL, NULL, RDW_INVALIDATE | RDW_ALLCHILDREN | RDW_ERASE | RDW_FRAME | RDW_ERASENOW | RDW_UPDATENOW);
}

void Window::invalidate()
{
	RedrawWindow(_handle, NULL, NULL, RDW_INVALIDATE | RDW_ALLCHILDREN | RDW_ERASE | RDW_FRAME | RDW_ERASENOW);
}

void Window::setPosition(Vector position)
{
	if (getPosition() == position)
	{
		return;
	}
	SetWindowPos(_handle, NULL, position.x, position.y, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER);
}

void Window::setSize(Vector size)
{
	if (getSize() == size)
	{
		return;
	}
	SetWindowPos(_handle, NULL, 0, 0, size.x, size.y, SWP_NOMOVE | SWP_NOZORDER | SWP_NOOWNERZORDER);
}

void Window::setRect(const Rect& rect)
{
	RECT rc = { rect.lower.x, rect.lower.y, rect.upper.x, rect.upper.y };
	DWORD style = GetWindowLong(_handle, GWL_STYLE);
	AdjustWindowRect(&rc, style, false);
	SetWindowPos(_handle, NULL, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, SWP_NOZORDER);
}

void Window::setText(const wchar_t* position)
{
	setRedraw(false);
	sendMessage(WM_SETTEXT, NULL, (LPARAM)position);
	setRedraw(true);
	repaint();
}

void Window::setForegroundColor(Color color)
{
	_foregroundColor = color;
	repaint();
}

void Window::setBackgroundColor(Color color)
{
	_backgroundColor = color;
	repaint();
}

void Window::setFont(const Font* font)
{
	sendMessage(WM_SETFONT, font == NULL ? 0 : (WPARAM)font->_handle, 0);
	repaint();
}

void Window::setChildrenEnabled(bool enabled)
{
	broadcastMessage(Control::MessageSetEnabled, (WPARAM)enabled, 0);
}

void Window::setChildrenForegroundColor(Color color)
{
	broadcastMessage(Control::MessageSetForegroundColor, (WPARAM)&color, 0);
}

void Window::setChildrenBackgroundColor(Color color)
{
	broadcastMessage(Control::MessageSetBackgroundColor, (WPARAM)&color, 0);
}

void Window::setChildrenFont(const Font* font)
{
	broadcastMessage(Control::MessageSetFont, (WPARAM)font, 0);
}

void Window::setExcludedFromCapture(bool excluded)
{
	setExcludedFromCapture(_handle, excluded);
	DWORD windowThreadId = GetWindowThreadProcessId(_handle, NULL);
	EnumThreadWindows(windowThreadId, excludeFromCaptureProc, excluded);
}

void Window::runMessageLoop()
{
	MSG msg = {};
	while (GetMessage(&msg, NULL, 0, 0) >= 0)
	{
		if (msg.message == WM_QUIT && msg.hwnd == _handle)
		{
			break;
		}
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
}

LRESULT Window::sendMessage(UINT message, WPARAM wParam, LPARAM lParam)
{
	return SendMessage(_handle, message, wParam, lParam);
}

LRESULT Window::postMessage(UINT message, WPARAM wParam, LPARAM lParam)
{
	return PostMessage(_handle, message, wParam, lParam);
}

void Window::postTask(const Callback& callback)
{
	postMessage(WM_USER, (WPARAM)callback.getFunction(), (LPARAM)callback.getArgument());
}

int Window::showMessageBox(const wchar_t* title, const wchar_t* content, UINT type) const
{
	return MessageBox(_handle, content, title, type);
}

void Window::openFile(const wchar_t* path)
{
	ShellExecute(_handle, L"open", path, NULL, NULL, SW_SHOWNORMAL);
}

void Window::getTitle(wchar_t* buffer, int size) const
{
	int length = GetWindowText(_handle, buffer, size);
	buffer[length] = '\0';
}

int Window::getTitleLength() const
{
	int length = GetWindowTextLength(_handle);
	return length + 1;
}

Window* Window::getParent() const
{
	HWND parentHandle = GetParent(_handle);
	return fromHandle(parentHandle);
}

Window* Window::getTopLevelParent() const
{
	HWND ancestorHandle = GetAncestor(_handle, GA_ROOT);
	return fromHandle(ancestorHandle);
}

Vector Window::getPosition() const
{
	RECT rect = {};
	GetWindowRect(_handle, &rect);
	HWND parentHandle = GetParent(_handle);
	ScreenToClient(parentHandle, (LPPOINT)&rect);
	return Vector(rect.left, rect.top);
}

Vector Window::getSize() const
{
	RECT rect = {};
	GetClientRect(_handle, &rect);
	return Vector(rect.right - rect.left, rect.bottom - rect.top);
}

bool Window::getExcludedFromCapture() const
{
	return getExcludedFromCapture(_handle);
}

HMONITOR Window::getMonitor() const
{
	return MonitorFromWindow(_handle, MONITOR_DEFAULTTONEAREST);
}

void Window::create(const wchar_t* className, const wchar_t* windowName)
{
	create(className, windowName, 0, NULL);
}

void Window::create(const wchar_t* className, const wchar_t* windowName, long style, Window* parent)
{
	create(className, windowName, style, 0, parent);
}

void Window::create(const wchar_t* className, const wchar_t* windowName, long style, long exStyle, Window* parent)
{
	HWND parentHandle;
	int addStyle;
	if (parent != NULL)
	{
		parentHandle = parent->_handle;
		addStyle = WS_CHILD;
	}
	else
	{
		parentHandle = NULL;
		addStyle = WS_OVERLAPPEDWINDOW;
	}
	if ((style & WS_CHILD) == 0 && (style & WS_POPUP) == 0)
	{
		style |= addStyle;
	}
	style |= WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_VISIBLE;
	_handle = CreateWindowEx(exStyle, className, windowName, style, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, parentHandle, NULL, NULL, NULL);
	_context = GetDC(_handle);
	SetWindowLongPtr(_handle, GWLP_USERDATA, (LONG_PTR)this);
	createRenderObjects();
	addCallback(windowProc);
	activate();
}

void Window::addCallback(SUBCLASSPROC proc)
{
	UINT_PTR subclassId = (UINT_PTR)this;
	SetWindowSubclass(_handle, proc, subclassId, 0);
}

void Window::removeCallback(SUBCLASSPROC proc)
{
	UINT_PTR subclassId = (UINT_PTR)this;
	RemoveWindowSubclass(_handle, proc, subclassId);
}

void Window::broadcastMessage(UINT message, WPARAM wParam, LPARAM lParam)
{
	HWND child = GetWindow(_handle, GW_CHILD);
	while (child != NULL)
	{
		SendMessage(child, message, wParam, lParam);
		child = GetWindow(child, GW_HWNDNEXT);
	}
}

HWND Window::getHandle() const
{
	return _handle;
}

HDC Window::getContext() const
{
	return _context;
}

bool Window::canClose()
{
	return true;
}

Vector Window::getMinimumSize()
{
	return Vector(0, 0);
}

void Window::doPaint(Graphics&)
{
}

void Window::createRenderObjects()
{
	Vector size = getSize();
	_renderContext = CreateCompatibleDC(_context);
	_renderBitmap = CreateCompatibleBitmap(_context, size.x, size.y);
	SelectObject(_renderContext, _renderBitmap);
}

void Window::destroyRenderObjects()
{
	DeleteDC(_renderContext);
	DeleteObject(_renderBitmap);
}

Window* Window::fromHandle(HWND handle)
{
	LONG_PTR data = GetWindowLongPtr(handle, GWLP_USERDATA);
	return (Window*)data;
}

LRESULT Window::windowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam, UINT_PTR subclassId, DWORD_PTR)
{
	Window* instance = (Window*)subclassId;
	switch (message)
	{
		case WM_WININICHANGE:
		{
			break;
		}
		case WM_SIZE:
		{
			instance->setRedraw(false);
			instance->destroyRenderObjects();
			instance->createRenderObjects();
			instance->broadcastMessage(Control::MessageResized, wParam, lParam);
			instance->setRedraw(true);
			instance->repaint();
			break;
		}
		case WM_LBUTTONDBLCLK:
		{
			SendMessage(window, WM_LBUTTONDOWN, wParam, lParam);
			break;
		}
		case WM_PAINT:
		{
			PAINTSTRUCT paint = {};
			HDC context = BeginPaint(window, &paint);
			RECT rect = {};
			GetClientRect(window, &rect);
			COLORREF textColor = instance->_foregroundColor;
			COLORREF backgroundColor = instance->_backgroundColor;
			SetTextColor(instance->_renderContext, textColor);
			SetBkColor(instance->_renderContext, backgroundColor);
			HBRUSH brush = CreateSolidBrush(backgroundColor);
			FillRect(instance->_renderContext, &rect, brush);
			DeleteObject(brush);
			DefSubclassProc(window, WM_PAINT, (WPARAM)instance->_renderContext, 0);
			Graphics graphics(instance->_renderContext);
			instance->doPaint(graphics);
			BitBlt(context, 0, 0, rect.right, rect.bottom, instance->_renderContext, 0, 0, SRCCOPY);
			EndPaint(window, &paint);
			instance->postMessage(WM_UPDATEUISTATE, MAKELONG(UIS_SET, UISF_HIDEFOCUS), 0);
			return 0;
		}
		case WM_CLOSE:
		{
			if (instance->canClose())
			{
				instance->close();
			}
			return 0;
		}
		case WM_GETMINMAXINFO:
		{
			MINMAXINFO* minMaxInfo = (MINMAXINFO*)lParam;
			DWORD style = (DWORD)GetWindowLong(instance->_handle, GWL_STYLE);
			Vector minSize = instance->getMinimumSize();
			RECT minRect = { 0, 0, minSize.x, minSize.y };
			AdjustWindowRect(&minRect, style, FALSE);
			minMaxInfo->ptMinTrackSize.x = minRect.right - minRect.left;
			minMaxInfo->ptMinTrackSize.y = minRect.bottom - minRect.top;
			return 0;
		}
		case WM_ERASEBKGND:
		{
			return 0;
		}
		case WM_CTLCOLORSTATIC:
		{
			return (LRESULT)GetStockObject(WHITE_BRUSH);
		}
		case WM_CTLCOLORLISTBOX:
		{
			return (LRESULT)GetStockObject(WHITE_BRUSH);
		}
		case WM_CTLCOLOREDIT:
		{
			HWND controlHandle = (HWND)lParam;
			Window* controlWindow = fromHandle(controlHandle);
			if (controlWindow != NULL)
			{
				Control* control = (Control*)controlWindow;
				COLORREF backgroundColor = control->_backgroundColor;
				HDC context = (HDC)wParam;
				SetBkColor(context, backgroundColor);
				return (LRESULT)GetStockObject(NULL_BRUSH);
			}
			return 0;
		}
		case WM_DRAWITEM:
		{
			LPDRAWITEMSTRUCT drawItem = (LPDRAWITEMSTRUCT)lParam;
			HWND controlHandle = drawItem->hwndItem;
			Window* controlWindow = fromHandle(controlHandle);
			if (controlWindow != NULL)
			{
				CustomControl* customControl = (CustomControl*)controlWindow;
				Graphics graphics(drawItem->hDC);
				bool disabled = drawItem->itemState & ODS_DISABLED;
				bool selected = drawItem->itemState & ODS_SELECTED;
				customControl->doCustomPaint(graphics, disabled, selected);
			}
			return 0;
		}
		case WM_COMMAND:
		{
			HWND childHandle = (HWND)lParam;
			Window* childWindow = fromHandle(childHandle);
			if (childWindow == NULL)
			{
				break;
			}
			switch (HIWORD(wParam))
			{
				case BN_CLICKED:
				{
					childWindow->postMessage(Control::MessageChanged, 0, 0);
					break;
				}
				case CBN_SELCHANGE:
				{
					childWindow->postMessage(Control::MessageChanged, 0, 0);
					break;
				}
			}
			return 1;
		}
		case WM_USER:
		{
			Callback callback((Callback::Function*)wParam, (Callback::Argument)lParam);
			callback.invoke();
			return 1;
		}
	}
	return DefSubclassProc(window, message, wParam, lParam);
}

BOOL Window::excludeFromCaptureProc(HWND window, LPARAM lParam)
{
	setExcludedFromCapture(window, (BOOL)lParam);
	return TRUE;
}

void Window::setExcludedFromCapture(HWND window, BOOL excluded)
{
	WINDOWCOMPOSITIONATTRIBDATA data = {};
	data.Attrib = WCA_EXCLUDED_FROM_DDA;
	data.pvData = &excluded;
	data.cbData = sizeof(excluded);
	WindowComposition::setAttribute(window, &data);
	RedrawWindow(window, NULL, NULL, RDW_INVALIDATE | RDW_ERASE | RDW_ERASENOW | RDW_UPDATENOW | RDW_ALLCHILDREN | RDW_FRAME);
}

BOOL Window::getExcludedFromCapture(HWND window)
{
	BOOL excluded = FALSE;
	WINDOWCOMPOSITIONATTRIBDATA data = {};
	data.Attrib = WCA_EXCLUDED_FROM_DDA;
	data.pvData = &excluded;
	data.cbData = sizeof(excluded);
	WindowComposition::getAttribute(window, &data);
	return excluded;
}
