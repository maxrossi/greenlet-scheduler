/* 
	*************************************************************************

	Channel.h

	Author:    James Hawk
	Created:   Feb. 2024
	Project:   Scheduler

	Description:   

	  Functionality related to communication between tasklets

	(c) CCP 2024

	*************************************************************************
*/
#pragma once

#include <deque>

#include "stdafx.h"

const int SENDER = 1;
const int RECEIVER = -1;
const int PREFER_NEITHER = 0;

class Tasklet;

class Channel
{
public:
	
	Channel( PyObject* python_object );

    ~Channel();

    PyObject* python_object();

	bool send( PyObject* args, bool exception=false );

    PyObject* receive();

    int balance() const;

    void remove_tasklet_from_blocked( Tasklet* tasklet );

    static PyObject* channel_callback();

    static void set_channel_callback(PyObject* callback);

    int preference() const;

    void set_preference( int value );

    Tasklet* blocked_queue_front();

private:

    void run_channel_callback( Channel* channel, Tasklet* tasklet, bool sending, bool will_block ) const;

    void add_tasklet_to_waiting_to_send( Tasklet* tasklet );

    void add_tasklet_to_waiting_to_receive( Tasklet* tasklet );

    Tasklet* pop_next_tasklet_blocked_on_send();

    Tasklet* pop_next_tasklet_blocked_on_receive();

    bool channel_switch( Tasklet* caller, Tasklet* other, int dir, int caller_dir );

private:

    PyObject* m_python_object;

    int m_balance;

	int m_preference;

    PyThread_type_lock m_lock;

    inline static PyObject* s_channel_callback; // This is global, not per channel

    Tasklet* m_first_blocked_on_receive;

	Tasklet* m_last_blocked_on_receive;

	Tasklet* m_first_blocked_on_send;

    Tasklet* m_last_blocked_on_send;
    
};
