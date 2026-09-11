#include "Profiler.h"

static volatile int _level;
static Thread* _thread;

static void threadProc(void*)
{
	AllocConsole();
	MessageBeep(MB_OK);
	FILE* file;
	freopen_s(&file, "CONOUT$", "w", stdout);
	freopen_s(&file, "CONIN$", "r", stdin);
	while (true)
	{
		int totalCount = 0;
		int activeCount = 0;
		double startTime = getTime();
		double updateTime = startTime;
		double enterTime = 0;
		double durationSum = 0;
		int durationCount = 0;
		while (true)
		{
			double currentTime = getTime();
			if (currentTime - updateTime >= 0.00011)
			{
				updateTime = currentTime;
				totalCount++;
				if (_level > 0)
				{
					activeCount++;
				}
			}
			if (currentTime - startTime >= 1.5)
			{
				break;
			}
			if (_level > 0)
			{
				if (enterTime == 0 && activeCount < totalCount)
				{
					enterTime = currentTime;
				}
			}
			else if (enterTime != 0)
			{
				durationSum += currentTime - enterTime;
				durationCount++;
				enterTime = 0;
			}
		}
		wprintf(L"Profiler: Usage: %i %% Duration: %f ms\n", activeCount * 100 / totalCount, durationCount < 1 ? 0 : durationSum * 1000 / durationCount);
	}
}

void startProfiler()
{
	if (_thread == NULL)
	{
		_thread = new Thread(Callback(threadProc, NULL));
	}
}

void enterBlock()
{
	_level++;
}

void leaveBlock()
{
	_level--;
}
