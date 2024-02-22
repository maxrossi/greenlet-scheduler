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
#include <queue>

struct PyTaskletObject;

class PySchedulerObject
{
public:
	PyObject_HEAD

	static PyObject* get_current();

    static void insert_tasklet( PyTaskletObject* tasklet );

    static int get_tasklet_count();

    static void schedule();

    static PyObject* run();

    std::queue<PyObject*>* m_tasklets;

    PyTaskletObject* m_scheduler_tasklet;

	inline static PySchedulerObject* s_singleton;
};
