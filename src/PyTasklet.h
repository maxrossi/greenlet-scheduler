#pragma once

#include "stdafx.h"

class PyTasklet
{
	PyObject_HEAD

public:
	bool insert();

public:
	int alive;
};
