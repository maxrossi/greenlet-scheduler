/* 
	*************************************************************************

	PyScheduleManager.h

	Author:    James Hawk
	Created:   May. 2024
	Project:   Scheduler

	Description:   

	  PyScheduleManager python type definition

	(c) CCP 2024

	*************************************************************************
*/
#pragma once

class ScheduleManager;

typedef struct PyScheduleManagerObject
{
	PyObject_HEAD

	ScheduleManager* m_impl;

} _PyScheduleManagerObject;