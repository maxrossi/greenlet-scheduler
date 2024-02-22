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

class PyTaskletObject
{
public:
	PyObject_HEAD

	void set_to_current_greenlet();

	bool insert();

    PyObject* switch_to();

	PyGreenlet* m_greenlet;

	PyObject* m_callable;

	PyObject* m_arguments;

	bool m_alive;

    bool m_blocked;

    inline static PyTaskletObject* s_current = NULL;
};
