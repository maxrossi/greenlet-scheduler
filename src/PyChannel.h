/* 
	*************************************************************************

	PyChannel.h

	Author:    James Hawk
	Created:   Feb. 2024
	Project:   Scheduler

	Description:   

	  Functionality related to communication between tasklets

	(c) CCP 2024

	*************************************************************************
*/
#pragma once

#include "stdafx.h"

class PyChannelObject
{
public:
	PyObject_HEAD
	
	PyChannelObject();

    ~PyChannelObject();

	bool send( PyObject* args, bool exception=false );

    PyObject* receive();

    int balance() const;

    void remove_tasklet_from_blocked( PyObject* tasklet );

    static PyObject* channel_callback();

    static void set_channel_callback(PyObject* callback);

    int preference() const;

    void set_preference( int value );

private:

    void run_channel_callback( PyObject* channel, PyObject* tasklet, bool sending, bool will_block ) const;

    void add_tasklet_to_waiting_to_send(PyObject* tasklet);

    void add_tasklet_to_waiting_to_receive( PyObject* tasklet );

    PyObject* pop_next_tasklet_blocked_on_send();

    PyObject* pop_next_tasklet_blocked_on_receive();

    int m_balance;

	int m_preference;

    PyObject* m_first_tasklet_waiting_to_send;

    PyObject* m_first_tasklet_waiting_to_receive;

    PyObject* m_previous_blocked_send_tasklet;

    PyObject* m_previous_blocked_receive_tasklet;

    PyThread_type_lock m_lock;

    inline static PyObject* s_channel_callback; // This is global, not per channel

};
