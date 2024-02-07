#pragma once

#include "stdafx.h"

class PyTasklet;

class PyScheduler
{
	PyObject_HEAD

public:
	PyTasklet* get_current();

public:
	PyTasklet* current;
};
