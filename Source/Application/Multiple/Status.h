#pragma once

class Status
{
public:

	Status();

	Status(HRESULT result);

	bool operator==(HRESULT result) const;

	operator bool() const;

	operator HRESULT() const;

private:

	HRESULT _result;
};

