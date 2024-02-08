#define PY_SSIZE_T_CLEAN
#include "stdafx.h"

#define SCHEDULER_MODULE
#include "Scheduler.h"

//Types
#include "PyScheduler_python.cpp"
#include "PyTasklet_python.cpp"
#include "PyChannel_python.cpp"

static PyScheduler* g_scheduler = nullptr;
/*
C Interface


*/
extern "C"
{
	// Tasklet functions
	static PyTasklet_Check_RETURN PyTasklet_Check PyTasklet_Check_PROTO
	{
		PyErr_SetString( PyExc_RuntimeError, "PyTasklet_Check Not yet implemented" ); //TODO
		return 0;
	}

	static PyTasklet_Setup_RETURN PyTasklet_Setup PyTasklet_Setup_PROTO
	{
		PyErr_SetString( PyExc_RuntimeError, "PyTasklet_Setup Not yet implemented" ); //TODO
		return 0;
	}

    static PyTasklet_Insert_RETURN PyTasklet_Insert PyTasklet_Insert_PROTO
	{
		PyErr_SetString( PyExc_RuntimeError, "PyTasklet_Insert Not yet implemented" ); //TODO
		return 0;
	}

    static PyTasklet_GetBlockTrap_RETURN PyTasklet_GetBlockTrap PyTasklet_GetBlockTrap_PROTO
	{
		PyErr_SetString( PyExc_RuntimeError, "PyTasklet_GetBlockTrap Not yet implemented" ); //TODO
		return 0;
	}

    static PyTasklet_SetBlockTrap_RETURN PyTasklet_SetBlockTrap PyTasklet_SetBlockTrap_PROTO
	{
		PyErr_SetString( PyExc_RuntimeError, "PyTasklet_SetBlockTrap Not yet implemented" ); //TODO
	}

    static PyTasklet_IsMain_RETURN PyTasklet_IsMain PyTasklet_IsMain_PROTO
	{
		PyErr_SetString( PyExc_RuntimeError, "PyTasklet_IsMain Not yet implemented" ); //TODO
		return 0;
	}
        
    // Channel functions
	static PyChannel_New_RETURN PyChannel_New PyChannel_New_PROTO
	{
		PyErr_SetString( PyExc_RuntimeError, "PyChannel_New Not yet implemented" ); //TODO
		return NULL;
	}

    static PyChannel_Send_RETURN PyChannel_Send PyChannel_Send_PROTO
	{
		PyErr_SetString( PyExc_RuntimeError, "PyChannel_Send Not yet implemented" ); //TODO
		return 0;
	}

    static PyChannel_Receive_RETURN PyChannel_Receive PyChannel_Receive_PROTO
	{
		PyErr_SetString( PyExc_RuntimeError, "PyChannel_Receive Not yet implemented" ); //TODO
		return NULL;
	}

    static PyChannel_SendException_RETURN PyChannel_SendException PyChannel_SendException_PROTO
	{
		PyErr_SetString( PyExc_RuntimeError, "PyChannel_SendException Not yet implemented" ); //TODO
		return 0;
	}

    static PyChannel_GetQueue_RETURN PyChannel_GetQueue PyChannel_GetQueue_PROTO
	{
		PyErr_SetString( PyExc_RuntimeError, "PyChannel_GetQueue Not yet implemented" ); //TODO
		return NULL;
	}

    static PyChannel_SetPreference_RETURN PyChannel_SetPreference PyChannel_SetPreference_PROTO
	{
		PyErr_SetString( PyExc_RuntimeError, "PyChannel_SetPreference Not yet implemented" ); //TODO
	}

    static PyChannel_GetBalance_RETURN PyChannel_GetBalance PyChannel_GetBalance_PROTO
	{
		PyErr_SetString( PyExc_RuntimeError, "PyChannel_GetBalance Not yet implemented" ); //TODO
		return 0;
	}
	
	// Scheduler functions
	static PyScheduler_Schedule_RETURN PyScheduler_Schedule PyScheduler_Schedule_PROTO
	{
		PyErr_SetString( PyExc_RuntimeError, "PyScheduler_Schedule Not yet implemented" ); //TODO
		return NULL;
	}

    static PyScheduler_GetRunCount_RETURN PyScheduler_GetRunCount PyScheduler_GetRunCount_PROTO
	{
		PyErr_SetString( PyExc_RuntimeError, "PyScheduler_GetRunCount Not yet implemented" ); //TODO
		return 0;
	}
    
    static PyScheduler_GetCurrent_RETURN PyScheduler_GetCurrent PyScheduler_GetCurrent_PROTO
	{
		return reinterpret_cast<PyObject*>(g_scheduler->get_current());
	}

	static PyScheduler_RunWatchdogEx_RETURN PyScheduler_RunWatchdogEx PyScheduler_RunWatchdogEx_PROTO
	{
		PyErr_SetString( PyExc_RuntimeError, "PyScheduler_RunWatchdogEx Not yet implemented" ); //TODO
		return NULL;
	}

}	// extern C

// End C API


static PyObject*
	get_scheduler( PyObject* self, PyObject* args )
{
	return reinterpret_cast<PyObject*>(g_scheduler);
}

static PyMethodDef SchedulerMethods[] = {
	{ "getscheduler", get_scheduler, METH_VARARGS, "Get the main scheduler object" },
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
	static void* PyScheduler_API[PyScheduler_API_pointers];
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
	g_scheduler =  PyObject_New( PyScheduler, &SchedulerType );
	Scheduler_init( g_scheduler, nullptr, nullptr );

	//C_API
	/* Initialize the C API pointer array */
    // Tasklet Functions
	PyScheduler_API[PyTasklet_Check_NUM] = (void*)PyTasklet_Check;
	PyScheduler_API[PyTasklet_Setup_NUM] = (void*)PyTasklet_Setup;
	PyScheduler_API[PyTasklet_Insert_NUM] = (void*)PyTasklet_Insert;
	PyScheduler_API[PyTasklet_GetBlockTrap_NUM] = (void*)PyTasklet_GetBlockTrap;
	PyScheduler_API[PyTasklet_SetBlockTrap_NUM] = (void*)PyTasklet_SetBlockTrap;
	PyScheduler_API[PyTasklet_IsMain_NUM] = (void*)PyTasklet_IsMain;

    // Channel Functions
	PyScheduler_API[PyChannel_New_NUM] = (void*)PyChannel_New;
	PyScheduler_API[PyChannel_Send_NUM] = (void*)PyChannel_Send;
	PyScheduler_API[PyChannel_Receive_NUM] = (void*)PyChannel_Receive;
	PyScheduler_API[PyChannel_SendException_NUM] = (void*)PyChannel_SendException;
	PyScheduler_API[PyChannel_GetQueue_NUM] = (void*)PyChannel_GetQueue;
	PyScheduler_API[PyChannel_SetPreference_NUM] = (void*)PyChannel_SetPreference;
	PyScheduler_API[PyChannel_GetBalance_NUM] = (void*)PyChannel_GetBalance;

    // Scheduler Functions
	PyScheduler_API[PyScheduler_Schedule_NUM] = (void*)PyScheduler_Schedule;
	PyScheduler_API[PyScheduler_GetRunCount_NUM] = (void*)PyScheduler_GetRunCount;
	PyScheduler_API[PyScheduler_GetCurrent_NUM] = (void*)PyScheduler_GetCurrent;
	PyScheduler_API[PyScheduler_RunWatchdogEx_NUM] = (void*)PyScheduler_RunWatchdogEx;

	/* Create a Capsule containing the API pointer array's address */
	c_api_object = PyCapsule_New( (void*)PyScheduler_API, "scheduler._C_API", NULL );

	if( PyModule_AddObject( m, "_C_API", c_api_object ) < 0 )
	{
		Py_XDECREF( c_api_object );
		Py_DECREF( m );
		return NULL;
	}

    return m;
}

