#define PY_SSIZE_T_CLEAN
#include "stdafx.h"

#define SCHEDULER_MODULE
#include "Scheduler.h"

//Types
#include "PyScheduler_python.cpp"
#include "PyTasklet_python.cpp"
#include "PyChannel_python.cpp"

static PySchedulerObject* g_scheduler = nullptr;
/*
C Interface
*/
extern "C"
{
	// Tasklet functions
	static int PyTasklet_Setup( PyTaskletObject*, PyObject* args, PyObject* kwds )
	{
		PyErr_SetString( PyExc_RuntimeError, "PyTasklet_Setup Not yet implemented" ); //TODO
		return 0;
	}

    static int PyTasklet_Insert( PyTaskletObject* )
	{
		PyErr_SetString( PyExc_RuntimeError, "PyTasklet_Insert Not yet implemented" ); //TODO
		return 0;
	}

    static int PyTasklet_GetBlockTrap( PyTaskletObject* )
	{
		PyErr_SetString( PyExc_RuntimeError, "PyTasklet_GetBlockTrap Not yet implemented" ); //TODO
		return 0;
	}

    static void PyTasklet_SetBlockTrap( PyTaskletObject*, int )
	{
		PyErr_SetString( PyExc_RuntimeError, "PyTasklet_SetBlockTrap Not yet implemented" ); //TODO
	}

    static int PyTasklet_IsMain( PyTaskletObject* )
	{
		PyErr_SetString( PyExc_RuntimeError, "PyTasklet_IsMain Not yet implemented" ); //TODO
		return 0;
	}
        
    // Channel functions
	static PyChannelObject* PyChannel_New( PyTypeObject* )
	{
		PyErr_SetString( PyExc_RuntimeError, "PyChannel_New Not yet implemented" ); //TODO
		return NULL;
	}

    static int PyChannel_Send( PyChannelObject*, PyObject* )
	{
		PyErr_SetString( PyExc_RuntimeError, "PyChannel_Send Not yet implemented" ); //TODO
		return 0;
	}

    static PyObject* PyChannel_Receive( PyChannelObject* )
	{
		PyErr_SetString( PyExc_RuntimeError, "PyChannel_Receive Not yet implemented" ); //TODO
		return NULL;
	}

    static int PyChannel_SendException( PyChannelObject*, PyObject*, PyObject* )
	{
		PyErr_SetString( PyExc_RuntimeError, "PyChannel_SendException Not yet implemented" ); //TODO
		return 0;
	}

    static PyObject* PyChannel_GetQueue( PyChannelObject* )
	{
		PyErr_SetString( PyExc_RuntimeError, "PyChannel_GetQueue Not yet implemented" ); //TODO
		return NULL;
	}

    static void PyChannel_SetPreference( PyChannelObject*, int )
	{
		PyErr_SetString( PyExc_RuntimeError, "PyChannel_SetPreference Not yet implemented" ); //TODO
	}

    static int PyChannel_GetBalance( PyChannelObject* )
	{
		PyErr_SetString( PyExc_RuntimeError, "PyChannel_GetBalance Not yet implemented" ); //TODO
		return 0;
	}
	
	// Scheduler functions
	static PyObject* PyScheduler_Schedule( PyObject*, int )
	{
		PyErr_SetString( PyExc_RuntimeError, "PyScheduler_Schedule Not yet implemented" ); //TODO
		return NULL;
	}

    static int PyScheduler_GetRunCount()
	{
		PyErr_SetString( PyExc_RuntimeError, "PyScheduler_GetRunCount Not yet implemented" ); //TODO
		return 0;
	}
    
    static PyObject* PyScheduler_GetCurrent()
	{
		return reinterpret_cast<PyObject*>(g_scheduler->get_current());
	}

    // Note: flags used in game are PY_WATCHDOG_SOFT | PY_WATCHDOG_IGNORE_NESTING | PY_WATCHDOG_TOTALTIMEOUT
    // We can in theory remove the flags from proto and always assume these flags, we don't need to support
    // all combinations.
    // Initially left in just to keep api the same during the first stubbing out.
	static PyObject* PyScheduler_RunWatchdogEx( long, int )
	{
		PyErr_SetString( PyExc_RuntimeError, "PyScheduler_RunWatchdogEx Not yet implemented" ); //TODO
		return NULL;
	}

    static PyObject* PyScheduler_SetChannelCallback( PyObject*, PyObject* )
    {
		PyErr_SetString( PyExc_RuntimeError, "PyScheduler_SetChannelCallback Not yet implemented" ); //TODO
		return NULL;
    }

    static PyObject* PyScheduler_GetChannelCallback( PyObject*, PyObject* )
	{
		PyErr_SetString( PyExc_RuntimeError, "PyScheduler_GetChannelCallback Not yet implemented" ); //TODO
		return NULL;
	}

    static PyObject* PyScheduler_SetScheduleCallback( PyObject*, PyObject* )
    {
		PyErr_SetString( PyExc_RuntimeError, "PyScheduler_SetScheduleCallback Not yet implemented" ); //TODO
		return NULL;
    }

    static void PyScheduler_SetScheduleFastCallback( schedule_hook_func func )
	{
		PyErr_SetString( PyExc_RuntimeError, "PyScheduler_SetScheduleFastCallback Not yet implemented" ); //TODO
		return;
	}

    static PyObject* PyScheduler_CallMethod_Main( PyObject* o, char* name, char* format, ... )
    {
		PyErr_SetString( PyExc_RuntimeError, "PyScheduler_CallMethod_Main Not yet implemented" ); //TODO
		return NULL;
    }

}	// extern C

// End C API


static PyObject*
	get_scheduler( PyObject* self, PyObject* args )
{
	return reinterpret_cast<PyObject*>(g_scheduler);
}

static PyObject*
	set_channel_callback( PyObject* self, PyObject* args )
{
	PyErr_SetString( PyExc_RuntimeError, "set_channel_callback Not yet implemented" ); //TODO
	return NULL;
}

static PyObject*
	get_channel_callback( PyObject* self, PyObject* args )
{
	PyErr_SetString( PyExc_RuntimeError, "get_channel_callback Not yet implemented" ); //TODO
	return NULL;
}

static PyObject*
	set_scheduler_callback( PyObject* self, PyObject* args )
{
	PyErr_SetString( PyExc_RuntimeError, "set_scheduler_callback Not yet implemented" ); //TODO
	return NULL;
}

static PyMethodDef SchedulerMethods[] = {
	{ "getscheduler", get_scheduler, METH_VARARGS, "Get the main scheduler object" },
	{ "set_channel_callback", set_channel_callback, METH_VARARGS, "Install a global channel callback" },
	{ "get_channel_callback", get_channel_callback, METH_VARARGS, "Get the current global channel callback" },
	{ "set_scheduler_callback", set_scheduler_callback, METH_VARARGS, "Get the current global channel callback" },
	{ NULL, NULL, 0, NULL } /* Sentinel */
};


static struct PyModuleDef schedulermodule = {
    PyModuleDef_HEAD_INIT,
    "carbon-scheduler",   /* name of module */
    NULL, /* module documentation, may be NULL */
    -1,       /* size of per-interpreter state of the module,
                 or -1 if the module keeps state in global variables. */
    SchedulerMethods
};

PyMODINIT_FUNC
PyInit__scheduler(void)
{
    PyObject *m;
	static SchedulerCAPI api;
	PyObject* c_api_object;

	//Add custom types
	if( PyType_Ready( &TaskletType ) < 0 )
		return NULL;

	if( PyType_Ready( &ChannelType ) < 0 )
		return NULL;

	if( PyType_Ready( &SchedulerType ) < 0 )
		return NULL;

    m = PyModule_Create( &schedulermodule );
    if (m == NULL)
        return NULL;



	Py_INCREF( &TaskletType );
	if( PyModule_AddObject( m, "Tasklet", (PyObject*)&TaskletType ) < 0 )
	{
		Py_DECREF( &TaskletType );
		Py_DECREF( m );
		return NULL;
	}

	Py_INCREF( &ChannelType );
	if( PyModule_AddObject( m, "Channel", (PyObject*)&ChannelType ) < 0 )
	{
		Py_DECREF( &ChannelType );
		Py_DECREF( m );
		return NULL;
	}

	Py_INCREF( &SchedulerType );
	if( PyModule_AddObject( m, "Scheduler", (PyObject*)&SchedulerType ) < 0 )
	{
		Py_DECREF( &SchedulerType );
		Py_DECREF( m );
		return NULL;
	}

	//Exceptions
    TaskletExit = PyErr_NewException( "carbon-scheduler.TaskletExit", NULL, NULL );
	Py_XINCREF( TaskletExit );
	if( PyModule_AddObject( m, "error", TaskletExit ) < 0 )
	{
		Py_XDECREF( TaskletExit );
		Py_CLEAR( TaskletExit );
        Py_DECREF(m);
        return NULL;
    }

	//Create the main scheduler object
	g_scheduler =  PyObject_New( PySchedulerObject, &SchedulerType );
	Scheduler_init( g_scheduler, nullptr, nullptr );

	//C_API
	/* Initialize the C API Object */
    // Types
	api.PyTaskletType = &TaskletType;
	api.PyChannelType = &ChannelType;

    // Exceptions
	api.TaskletExit = &TaskletExit;
    
    // Tasklet Functions
    api.PyTasklet_Setup = PyTasklet_Setup;
	api.PyTasklet_Insert = PyTasklet_Insert;
	api.PyTasklet_GetBlockTrap = PyTasklet_GetBlockTrap;
	api.PyTasklet_SetBlockTrap = PyTasklet_SetBlockTrap;
	api.PyTasklet_IsMain = PyTasklet_IsMain;

    // Channel Functions
	api.PyChannel_New = PyChannel_New;
	api.PyChannel_Send = PyChannel_Send;
	api.PyChannel_Receive = PyChannel_Receive;
	api.PyChannel_SendException = PyChannel_SendException;
	api.PyChannel_GetQueue = PyChannel_GetQueue;
	api.PyChannel_SetPreference = PyChannel_SetPreference;
	api.PyChannel_GetBalance = PyChannel_GetBalance;

    //Scheduler Functions
	api.PyScheduler_Schedule = PyScheduler_Schedule;
	api.PyScheduler_GetRunCount = PyScheduler_GetRunCount;
	api.PyScheduler_GetCurrent = PyScheduler_GetCurrent;
	api.PyScheduler_RunWatchdogEx = PyScheduler_RunWatchdogEx;
	api.PyScheduler_SetChannelCallback = PyScheduler_SetChannelCallback;
	api.PyScheduler_GetChannelCallback = PyScheduler_GetChannelCallback;
	api.PyScheduler_SetScheduleCallback = PyScheduler_SetScheduleCallback;
	api.PyScheduler_SetScheduleFastCallback = PyScheduler_SetScheduleFastCallback;
	api.PyScheduler_CallMethod_Main = PyScheduler_CallMethod_Main;

	/* Create a Capsule containing the API pointer array's address */
	c_api_object = PyCapsule_New( (void*)&api, "scheduler._C_API", NULL );

	if( PyModule_AddObject( m, "_C_API", c_api_object ) < 0 )
	{
		Py_XDECREF( c_api_object );
		Py_DECREF( m );
		return NULL;
	}

    return m;
}
