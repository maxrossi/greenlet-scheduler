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
#include <map>

struct PyTaskletObject;

class Scheduler
{
public:
	Scheduler();

	~Scheduler();

    static Scheduler* get_scheduler(long thread_id=-1);

	static void set_current_tasklet( PyTaskletObject* tasklet );

	static PyObject* get_current_tasklet();

    static void insert_tasklet( PyTaskletObject* tasklet );

    static int get_tasklet_count();

    static void schedule();

    static PyObject* run(PyTaskletObject* start_tasklet = nullptr);

    static PyObject* get_main_tasklet();

    static void set_scheduler_callback( PyObject* callback );

    void run_scheduler_callback( PyObject* prev, PyObject* next );

    long m_thread_id;

    PyTaskletObject* m_scheduler_tasklet;

    PyTaskletObject* m_current_tasklet; //Weak ref

    int m_switch_trap_level;

    PyTaskletObject* m_previous_tasklet;   //Weak ref

    PyObject* m_current_tasklet_changed_callback;

	PyObject* m_scheduler_callback;

    inline static std::map<long, Scheduler*> s_schedulers;    //Each thread has its own scheduler

    inline static PyObject* s_create_scheduler_tasklet_callable;    // A reference to easily create scheduler tasklets

    
};
