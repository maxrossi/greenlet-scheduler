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
	PyObject_HEAD

public:
	bool insert();

public:
	int m_alive;
};
