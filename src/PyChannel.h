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

#include <queue>

class PyChannelObject
{
public:
	PyObject_HEAD

	bool send( PyObject* args, bool exception=false );

    int balance();

    PyObject* receive();

    void remove_tasklet_from_blocked( PyObject* tasklet );

    void run_channel_callback( PyObject* channel, PyObject* tasklet, bool sending, bool will_block );

	int m_preference;

    std::queue<PyObject*>* m_waiting_to_send;

    std::queue<PyObject*>* m_waiting_to_receive;

    PyThread_type_lock m_lock;

    inline static PyObject* s_channel_callback; // This is global, not per channel

};
