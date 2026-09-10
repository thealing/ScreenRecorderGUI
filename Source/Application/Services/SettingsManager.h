#pragma once

template<class Settings>
class SettingsManager : Virtual
{
public:

	SettingsManager(const wchar_t* name);

	const Event* getChangeEvent();

	bool setSettings(const Settings& settings);

	Settings getSettings() const;

protected:

	void init();

	virtual Settings getDefault() const = 0;

	virtual void validate(Settings& settings) const = 0;

private:

	mutable ReadWriteLock _lock;

	const wchar_t* _name;
	Settings _settings;
	LatchEvent _initEvent;
	EventPool _changeEventPool;
};
