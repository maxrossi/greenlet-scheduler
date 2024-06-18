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

} _PyTaskletObject;
