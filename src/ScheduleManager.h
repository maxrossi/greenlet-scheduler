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
	ScheduleManager( PyObject* python_object );

	~ScheduleManager();

    PyObject* python_object();

    void incref();

    void decref();

    static int num_active_schedule_managers();

    static ScheduleManager* find_scheduler( long thread_id );

    static ScheduleManager* get_scheduler( long thread_id = -1 );

	void set_current_tasklet( Tasklet* tasklet );

	Tasklet* get_current_tasklet();

    bool remove_tasklet( Tasklet* tasklet );

    void insert_tasklet_at_beginning( Tasklet* tasklet );

    void insert_tasklet( Tasklet* tasklet );

    int get_tasklet_count();

    int calculate_tasklet_count();

    bool schedule(bool remove = false);

    bool yield();

    bool run_n_tasklets( int number_of_tasklets );

    bool run_tasklets_for_time( long long timeout );

    bool run( Tasklet* start_tasklet = nullptr );

    Tasklet* get_main_tasklet();

    void set_scheduler_fast_callback( schedule_hook_func* func );

    void set_scheduler_callback( PyObject* callback );

    void run_scheduler_callback( Tasklet* prev, Tasklet* next );

    PyObject* scheduler_callback();

    bool is_switch_trapped();

    void set_current_tasklet_changed_callback( PyObject* callback );

    int switch_trap_level();

    void set_switch_trap_level( int level );

    void create_scheduler_tasklet();

public:

    inline static PyTypeObject* s_tasklet_type;

    inline static PyTypeObject* s_schedule_manager_type;

private:

    PyObject* m_python_object;

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

    int m_number_of_tasklets_in_queue;

    inline static std::map<long, ScheduleManager*> s_schedulers;    //Each thread has its own scheduler
    
};
