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
	PyObject_HEAD

public:
	bool send();

public:
	int m_preference;
};
