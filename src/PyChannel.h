#pragma once

#include "stdafx.h"

class PyChannel
{
	PyObject_HEAD

public:
	bool send();

public:
	int preference;
};
