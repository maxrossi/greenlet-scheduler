/* 
	*************************************************************************

	ScheduleManager.h

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
#include <chrono>

typedef int( schedule_hook_func )( struct PyTaskletObject* from, struct PyTaskletObject* to );  // TODO remove redef

class Tasklet;

class ScheduleManager
{
public:
	ScheduleManager();

	~ScheduleManager();

    static ScheduleManager* get_scheduler( long thread_id = -1 );

	static void set_current_tasklet( Tasklet* tasklet );

	static Tasklet* get_current_tasklet();

    static void remove_tasklet( Tasklet* tasklet );

    static void insert_tasklet_at_beginning( Tasklet* tasklet );

    static void insert_tasklet( Tasklet* tasklet );

    static int get_tasklet_count();

    static bool schedule(bool remove = false);

    static bool yield();

    static PyObject* run_n_tasklets( int number_of_tasklets );

    static PyObject* run_tasklets_for_time( long long timeout );

    static PyObject* run( Tasklet* start_tasklet = nullptr );

    static Tasklet* get_main_tasklet();

    static void set_scheduler_fast_callback( schedule_hook_func* func );

    static void set_scheduler_callback( PyObject* callback );

    void run_scheduler_callback( Tasklet* prev, Tasklet* next );

    PyObject* scheduler_callback();

    static bool is_switch_trapped();

    void set_current_tasklet_changed_callback( PyObject* callback );

    int switch_trap_level();

    void set_switch_trap_level( int level );

public:
	inline static PyObject* s_scheduler_module;                 // A reference to "current" attribute in base module. Must be updated when current tasklet is changed

	inline static PyObject* s_create_scheduler_tasklet_callable; // A reference to easily create scheduler tasklets TODO shouldn't be public

private:

    long m_thread_id;

    Tasklet* m_scheduler_tasklet;

    Tasklet* m_current_tasklet; //Weak ref

    Tasklet* m_previous_tasklet; //Weak ref

    long m_switch_trap_level;

    PyObject* m_current_tasklet_changed_callback;

	PyObject* m_scheduler_callback;

    schedule_hook_func* m_scheduler_fast_callback;

    int m_tasklet_limit;

    long long m_total_tasklet_run_time_limit;

    std::chrono::steady_clock::time_point m_start_time;

    bool m_stop_scheduler;

    inline static std::map<long, ScheduleManager*> s_schedulers;    //Each thread has its own scheduler

    
    
};
