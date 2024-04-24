/* 
	*************************************************************************

	PyChannel.h

	Author:    James Hawk
	Created:   April. 2024
	Project:   Scheduler

	Description:   

	  PyChannelObject python type definition

	(c) CCP 2024

	*************************************************************************
*/
#pragma once

class Channel;

typedef struct PyChannelObject
{
	PyObject_HEAD

	Channel* m_impl;

    PyObject* m_weakref_list; // TODO: This is apparently the old style, new style crashes

} _PyChannelObject;