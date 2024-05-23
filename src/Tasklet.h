/* 
	*************************************************************************

	Tasklet.h

	Author:    James Hawk
	Created:   Feb. 2024
	Project:   Scheduler

	Description:   

	  Functions related to tasklets

	(c) CCP 2024

	*************************************************************************
*/
#pragma once

#include "stdafx.h"

class Channel;
class ScheduleManager;

class Tasklet
{
public:

	Tasklet( PyObject* python_object, PyObject* tasklet_exit_exception, bool is_main );

    ~Tasklet();

    PyObject* python_object();

    void incref();

	void decref();

    int refcount();

	void set_to_current_greenlet();

    bool remove();

    bool initialise();

    void uninitialise();

	bool insert();

    PyObject* switch_implementation();

    PyObject* switch_to();

    PyObject* run();

    bool kill( bool pending = false );

    PyObject* get_transfer_arguments();

	void clear_transfer_arguments();

    void set_transfer_arguments( PyObject* args, bool is_exception );

    void block( Channel* channel );

    void unblock();

    bool is_blocked() const;

    void set_alive( bool value );

    bool alive() const;

    bool scheduled() const;

    void set_scheduled( bool value );

    bool blocktrap() const;

    void set_blocktrap( bool value );

    bool is_main() const;

    void set_is_main( bool value );

    unsigned long thread_id() const;

	Tasklet* next() const;

	void set_next( Tasklet* next );

    Tasklet* previous() const;

	void set_previous( Tasklet* previous );

    Tasklet* next_blocked() const;

    void set_next_blocked(Tasklet* next);

    Tasklet* previous_blocked() const;

    void set_previous_blocked( Tasklet* previous );

    PyObject* arguments() const;

    void set_arguments(PyObject* arguments);

    PyObject* kw_arguments() const;

    void set_kw_arguments( PyObject* kwarguments );

    bool transfer_in_progress() const;

    void set_transfer_in_progress( bool value );

    bool transfer_is_exception() const;

    bool throw_exception( PyObject* exception, PyObject* value, PyObject* tb, bool pending );

    void raise_exception( );

    bool is_paused();

    Tasklet* get_tasklet_parent();

    int set_parent( Tasklet* parent );

    void clear_parent();

    bool tasklet_exception_raised();

    void clear_tasklet_exception();

    void set_reschedule( bool value );

    bool requires_reschedule();

    void set_tagged_for_removal( bool value );

    //void clear_callable();

    void set_callable( PyObject* callable );

    bool requires_removal();

    void check_cstate();

private:

    void set_exception_state( PyObject* exception, PyObject* arguments = Py_None );

	void set_python_exception_state_from_tasklet_exception_state();

    void clear_exception();

private:

    PyObject* m_python_object;

	PyGreenlet* m_greenlet;

	PyObject* m_callable;

	PyObject* m_arguments;

    PyObject* m_kwarguments;

    bool m_is_main;

    bool m_transfer_in_progress;;

    bool m_scheduled;

	bool m_alive;

    bool m_blocktrap;

    Tasklet* m_previous;

    Tasklet* m_next;

    Tasklet* m_next_blocked;

	Tasklet* m_previous_blocked;

    unsigned long m_thread_id;

    PyObject* m_transfer_arguments;

    bool m_transfer_is_exception;

    Channel* m_channel_blocked_on;

	bool m_blocked;

    bool m_has_started;

    bool m_paused;

    bool m_first_run;

    bool m_reschedule;

    bool m_remove;

    bool m_tagged_for_removal;  // This flag set will ensure that the tasklet doesn't get marked as not alive

    Tasklet* m_tasklet_parent; // Weak ref

    //Exception
    PyObject* m_exception_state;
	PyObject* m_exception_arguments;

    PyObject* m_tasklet_exit_exception; //Weak ref

    ScheduleManager* m_schedule_manager;

    bool m_kill_pending;
};
