/* 
	*************************************************************************

	Utils.h

	Author:    Joseph Frangoudes
	Created:   Oct. 2024
	Project:   Scheduler

	Description:   

	  Helpful utility functions

	(c) CCP 2024

	*************************************************************************
*/
#pragma once
#ifndef UTILS_H
#define UTILS_H

#include <string>

#include "stdafx.h"

bool StdStringFromPyObject( PyObject* obj, std::string& str );

#endif //UTILS_H