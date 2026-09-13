#include "MainWindow.h"

#define CLASS_NAME L"Capture Bit Window Class"
#define WINDOW_NAME L"Capture Bit 4.7"

MainWindow::MainWindow() : CustomWindow(CLASS_NAME)
{
	_closeable = true;
	create(CLASS_NAME, WINDOW_NAME);
	setRedraw(false);
	_sourcePanel = new SourcePanel(this);
	_resizePanel = new ResizePanel(this);
	_qualityPanel = new QualityPanel(this);
	_fpsDisplay = new FpsDisplay(this);
	_recordingPanel = new RecordingPanel(this);
	_recordingDisplay = new RecordingDisplay(this);
	_settingsPanel = new SettingsPanel(this);
	_previewDisplay = new PreviewDisplay(this);
	_volumeDisplay = new VolumeDisplay(this);
	setChildrenForegroundColor(0);
	setChildrenBackgroundColor(255);
	setBackgroundColor(220);
	setRedraw(true);
	repaint();
}

void MainWindow::openOutputFile()
{
	openFile(_outputPath);
}

void MainWindow::setOutputPath(const wchar_t* path)
{
	_outputPath = path;
}

void MainWindow::setCloseable(bool closeable)
{
	_closeable = closeable;
}

void MainWindow::setTopMost(bool topMost)
{
	HWND handle = getHandle();
	SetWindowPos(handle, topMost ? HWND_TOPMOST : HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
}

void MainWindow::excludeFromCapture(bool exclude)
{
	setExcludedFromCapture(exclude ? WDA_EXCLUDEFROMCAPTURE : WDA_NONE);
}

SourcePanel* MainWindow::getSourcePanel() const
{
	return _sourcePanel;
}

ResizePanel* MainWindow::getResizePanel() const
{
	return _resizePanel;
}

QualityPanel* MainWindow::getQualityPanel() const
{
	return _qualityPanel;
}

FpsDisplay* MainWindow::getFpsDisplay() const
{
	return _fpsDisplay;
}

RecordingPanel* MainWindow::getRecordingPanel() const
{
	return _recordingPanel;
}

RecordingDisplay* MainWindow::getRecordingDisplay() const
{
	return _recordingDisplay;
}

SettingsPanel* MainWindow::getSettingsPanel() const
{
	return _settingsPanel;
}

PreviewDisplay* MainWindow::getPreviewDisplay() const
{
	return _previewDisplay;
}

VolumeDisplay* MainWindow::getVolumeDisplay() const
{
	return _volumeDisplay;
}

bool MainWindow::canClose()
{
	return _closeable;
}

Vector MainWindow::getMinimumSize()
{
	return Vector(888, 143);
}
