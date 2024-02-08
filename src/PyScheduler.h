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

class PyTaskletObject;

class PySchedulerObject
{
	PyObject_HEAD

public:
	PyTaskletObject* get_current();

public:
	PyTaskletObject* m_current;
};
