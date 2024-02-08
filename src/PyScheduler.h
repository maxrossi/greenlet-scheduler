/* 
	*************************************************************************

	PyScheduler.h

	Author:    James Hawk
	Created:   Feb. 2024
	Project:   Scheduler

	Description:   

	  Functionality related to scheduling tasklets

	(c) CCP 2024

	*************************************************************************
*/
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
