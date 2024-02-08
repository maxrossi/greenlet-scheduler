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

class PyChannel
{
	PyObject_HEAD

public:
	bool send();

public:
	int preference;
};
