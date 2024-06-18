/* 
	*************************************************************************

	PyTasklet.h

	Author:    James Hawk
	Created:   April. 2024
	Project:   Scheduler

	Description:   

	  PyTaskletObject python type definition

	(c) CCP 2024

	*************************************************************************
*/
#pragma once

class Tasklet;

typedef struct PyTaskletObject
{
	PyObject_HEAD

	Tasklet* m_impl;

    PyObject* m_weakref_list;   // TODO: This is apparently the old style, new style crashes

} _PyTaskletObject;
