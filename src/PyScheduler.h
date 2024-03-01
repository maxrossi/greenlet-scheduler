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

	static void set_current_tasklet( PyTaskletObject* tasklet );

	static PyObject* get_current_tasklet();

    static void insert_tasklet( PyTaskletObject* tasklet );

    static int get_tasklet_count();

    static void schedule();

    static PyObject* run(PyTaskletObject* start_tasklet = nullptr);

    static PyObject* get_main_tasklet();

    PyTaskletObject* m_scheduler_tasklet;

    PyTaskletObject* m_current_tasklet; //Weak ref

    int m_switch_trap_level;

    PyTaskletObject* m_previous_tasklet;   //Weak ref

    PyObject* m_current_tasklet_changed_callback;

	inline static PySchedulerObject* s_singleton;
};
