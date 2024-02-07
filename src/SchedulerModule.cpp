#define PY_SSIZE_T_CLEAN
#include "stdafx.h"

#define SCHEDULER_MODULE
#include "SchedulerModule.h"

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

	// Channel functions
	
	// Scheduler functions
	static PyScheduler_GetCurrent_RETURN PyScheduler_GetCurrent PyScheduler_GetCurrent_PROTO
	{
		return reinterpret_cast<PyObject*>(g_scheduler->get_current());
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
	PyScheduler_API[PyScheduler_GetCurrent_NUM] = (void*)PyScheduler_GetCurrent;

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

