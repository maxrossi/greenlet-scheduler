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

	bool send( PyObject* args );

    int balance();

    PyObject* receive();

	int m_preference;

    std::queue<PyObject*>* m_waiting_to_send;

    std::queue<PyObject*>* m_waiting_to_receive;

    PyObject* m_transfer_arguments;
};
