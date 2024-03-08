/* 
	*************************************************************************

	Scheduler.h

	Author:    James Hawk
	Created:   Feb. 2024
	Project:   Scheduler

	Description:   

	 Provides channels and a scheduler for Greenlet coroutines.

	(c) CCP 2024

	*************************************************************************
*/

#ifndef Py_SCHEDULER_H
#define Py_SCHEDULER_H

#include <type_traits>

/* Header file for scheduler */

/* C API functions */
class PyTaskletObject;
class PyChannelObject;

#define Py_WATCHDOG_THREADBLOCK 1
#define PY_WATCHDOG_SOFT 2
#define PY_WATCHDOG_IGNORE_NESTING 4
#define PY_WATCHDOG_TOTALTIMEOUT 8


#define PyTasklet_Check( op ) ( op && PyObject_TypeCheck( op, &PyTasklet_Type ) )

typedef int( schedule_hook_func )( PyTaskletObject* from, PyTaskletObject* to );

class SchedulerCAPI
{
    public:
    // =============== function pointer types ===============
    //tasklet functions
    using PyTasklet_Setup_Routine                     = std::add_pointer_t<int(PyTaskletObject*, PyObject * args, PyObject * kwds)>;
    using PyTasklet_Insert_Routine                    = std::add_pointer_t<int(PyTaskletObject*)>;
    using PyTasklet_GetBlockTrap_Routine              = std::add_pointer_t<int(PyTaskletObject*)>;
    using PyTasklet_SetBlockTrap_Routine              = std::add_pointer_t<void(PyTaskletObject*, int)>;
    using PyTasklet_IsMain_Routine                    = std::add_pointer_t<int(PyTaskletObject*)>;

    //channel functions
	using PyChannel_New_Routine           		      = std::add_pointer_t<PyChannelObject*(PyTypeObject*)>;
	using PyChannel_Send_Routine          		      = std::add_pointer_t<int(PyChannelObject*, PyObject*)>;
	using PyChannel_Receive_Routine       		      = std::add_pointer_t<PyObject*(PyChannelObject*)>;
    using PyChannel_SendException_Routine 		      = std::add_pointer_t<int(PyChannelObject*, PyObject*, PyObject*)>;
    using PyChannel_GetQueue_Routine      		      = std::add_pointer_t<PyObject*(PyChannelObject*)>;
    using PyChannel_SetPreference_Routine 		      = std::add_pointer_t<void(PyChannelObject*, int)>;
    using PyChannel_GetBalance_Routine    		      = std::add_pointer_t<int(PyChannelObject*)>;

    //scheduler functions
    using PyScheduler_Schedule_Routine                = std::add_pointer_t<PyObject*(PyObject*, int)>;
    using PyScheduler_GetRunCount_Routine             = std::add_pointer_t<int(void)>;
    using PyScheduler_GetCurrent_Routine              = std::add_pointer_t<PyObject*(void)>;
    using PyScheduler_RunWatchdogEx_Routine           = std::add_pointer_t<PyObject*(long, int)>;
    using PyScheduler_SetChannelCallback_Routine      = std::add_pointer_t<PyObject*(PyObject*, PyObject*)>;
    using PyScheduler_GetChannelCallback_Routine      = std::add_pointer_t<PyObject*(PyObject*, PyObject*)>;
    using PyScheduler_SetScheduleCallback_Routine     = std::add_pointer_t<PyObject*(PyObject*, PyObject*)>;
    using PyScheduler_SetScheduleFastCallback_Routine = std::add_pointer_t<void(schedule_hook_func func)>;
    using PyScheduler_CallMethod_Main_Routine         = std::add_pointer_t<PyObject*(PyObject *o, char *name, char *format, ...)>;

    // =============== member function pointers ===============

    //tasklet functions
	PyTasklet_Setup_Routine PyTasklet_Setup;
	PyTasklet_Insert_Routine PyTasklet_Insert;
	PyTasklet_GetBlockTrap_Routine PyTasklet_GetBlockTrap;
	PyTasklet_SetBlockTrap_Routine PyTasklet_SetBlockTrap;
	PyTasklet_IsMain_Routine PyTasklet_IsMain;

    //channel functions
	PyChannel_New_Routine PyChannel_New;
	PyChannel_Send_Routine PyChannel_Send;
	PyChannel_Receive_Routine PyChannel_Receive;
	PyChannel_SendException_Routine PyChannel_SendException;
	PyChannel_GetQueue_Routine PyChannel_GetQueue;
	PyChannel_SetPreference_Routine PyChannel_SetPreference;
	PyChannel_GetBalance_Routine PyChannel_GetBalance;

    //scheduler functions
	PyScheduler_Schedule_Routine PyScheduler_Schedule;
	PyScheduler_GetRunCount_Routine PyScheduler_GetRunCount;
	PyScheduler_GetCurrent_Routine PyScheduler_GetCurrent;
	PyScheduler_RunWatchdogEx_Routine PyScheduler_RunWatchdogEx;
	PyScheduler_SetChannelCallback_Routine PyScheduler_SetChannelCallback;
	PyScheduler_GetChannelCallback_Routine PyScheduler_GetChannelCallback;
	PyScheduler_SetScheduleCallback_Routine PyScheduler_SetScheduleCallback;
	PyScheduler_SetScheduleFastCallback_Routine PyScheduler_SetScheduleFastCallback;
	PyScheduler_CallMethod_Main_Routine PyScheduler_CallMethod_Main;

    // types
    PyTypeObject* PyTaskletType;
	PyTypeObject* PyChannelType;

    // exceptions
	PyObject** TaskletExit;
};

#endif /* !defined(Py_SCHEDULERMODULE_H) */