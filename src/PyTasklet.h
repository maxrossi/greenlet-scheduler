/* 
	*************************************************************************

	PyTasklet.h

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

class PyChannelObject;

class PyTaskletObject
{
public:
	PyObject_HEAD

	PyTaskletObject( PyObject* callable, PyObject* tasklet_exit_exception );

    ~PyTaskletObject();

	void set_to_current_greenlet();

    bool remove();

    bool initialise();

	bool insert();

    PyObject* switch_implementation();

    PyObject* switch_to();

    PyObject* run();

    bool kill( bool pending = false );

    PyObject* get_transfer_arguments();

	void clear_transfer_arguments();

    void set_transfer_arguments( PyObject* args, bool is_exception );

    void block( PyChannelObject* channel );

    void unblock();

    bool is_blocked() const;

    bool alive() const;

    bool scheduled() const;

    void set_scheduled( bool value );

    bool blocktrap() const;

    void set_blocktrap( bool value );

    bool is_main() const;

    void set_is_main( bool value );

    unsigned long thread_id() const;

    PyObject* next() const;

    void set_next( PyObject* next );

    PyObject* previous() const;

    void set_previous( PyObject* previous );

    PyObject* arguments() const;

    void set_arguments(PyObject* arguments);

    bool transfer_in_progress() const;

    void set_transfer_in_progress( bool value );

    bool transfer_is_exception() const;

    bool throw_impl( PyObject* exception, PyObject* value, PyObject* tb, bool pending );

    void raise_exception( );

    bool is_paused();

    PyObject* get_tasklet_parent();

    void set_parent( PyObject* parent );

    void clear_parent();

    bool tasklet_exception_raised();

    void clear_tasklet_exception();

private:

	bool set_exception();

    bool exception_state_is_valid();

    void clear_exception();

    

private:

	PyGreenlet* m_greenlet;

	PyObject* m_callable;

	PyObject* m_arguments;

    bool m_is_main;

    bool m_transfer_in_progress;;

    bool m_scheduled;

	bool m_alive;

    bool m_blocktrap;

    PyObject* m_previous;

    PyObject* m_next;

    unsigned long m_thread_id;

    PyObject* m_transfer_arguments;

    bool m_transfer_is_exception;

    PyObject* m_channel_blocked_on;

	bool m_blocked;

    bool m_has_started;

    bool m_paused;

    PyObject* m_tasklet_parent; // Weak ref

    //Exception
    PyObject* m_exception_state;
	PyObject* m_exception_arguments;

    PyObject* m_tasklet_exit_exception; //Weak ref

};
