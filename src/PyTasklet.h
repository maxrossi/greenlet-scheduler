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

	PyTaskletObject();

	void set_to_current_greenlet();

	bool insert();

    PyObject* switch_to();

    PyObject* run();

    void kill();

    PyObject* get_transfer_arguments();

    void set_transfer_arguments( PyObject* args, bool is_exception );

    void block( PyChannelObject* channel );

    void unblock();

    bool is_blocked();

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

private:

    PyObject* m_channel_blocked_on;

	bool m_blocked;
};
