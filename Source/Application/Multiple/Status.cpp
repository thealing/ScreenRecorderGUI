#include "Status.h"

Status::Status()
{
	_result = S_OK;
}

Status::Status(HRESULT result)
{
	_result = result;
}

bool Status::operator==(HRESULT result) const
{
	return _result == result;
}

Status::operator bool() const
{
	return _result == S_OK;
}

Status::operator HRESULT() const
{
	return _result;
}
