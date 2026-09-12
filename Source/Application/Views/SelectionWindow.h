#pragma once

class SelectionWindow : public OverlayWindow
{
public:

	SelectionWindow();

protected:

	void setRect(const RECT& rect);

	virtual void onMouseMove(int mouseX, int mouseY) = 0;

	virtual void onMouseClick(int mouseX, int mouseY) = 0;

	virtual void onKeyPress() = 0;

private:

	void onPaint();

	POINT getScreenPoint(LPARAM lParam) const;

	virtual bool handleMessage(UINT message, WPARAM wParam, LPARAM lParam) override;

private:

	RECT _rect;
	bool _clicked;
};

