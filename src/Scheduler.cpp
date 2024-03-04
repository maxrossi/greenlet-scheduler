#define PY_SSIZE_T_CLEAN
#include "stdafx.h"

#define SCHEDULER_MODULE
#include "Scheduler.h"
#include <greenlet.h>

#include "PyScheduler.h"

//Types
#include "PyTasklet_python.cpp"
#include "PyChannel_python.cpp"

/*
C Interface


*/
extern "C"
{
	// Tasklet functions
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
		return Scheduler::get_current_tasklet();
	}

    // Note: flags used in game are PY_WATCHDOG_SOFT | PY_WATCHDOG_IGNORE_NESTING | PY_WATCHDOG_TOTALTIMEOUT
    // We can in theory remove the flags from proto and always assume these flags, we don't need to support
    // all combinations.
    // Initially left in just to keep api the same during the first stubbing out.
	static PyScheduler_RunWatchdogEx_RETURN PyScheduler_RunWatchdogEx PyScheduler_RunWatchdogEx_PROTO
	{
		PyErr_SetString( PyExc_RuntimeError, "PyScheduler_RunWatchdogEx Not yet implemented" ); //TODO
		return NULL;
	}

    static PyScheduler_SetChannelCallback_RETURN PyScheduler_SetChannelCallback PyScheduler_SetChannelCallback_PROTO
    {
		PyErr_SetString( PyExc_RuntimeError, "PyScheduler_SetChannelCallback Not yet implemented" ); //TODO
		return NULL;
    }

    static PyScheduler_GetChannelCallback_RETURN PyScheduler_GetChannelCallback PyScheduler_GetChannelCallback_PROTO
	{
		PyErr_SetString( PyExc_RuntimeError, "PyScheduler_GetChannelCallback Not yet implemented" ); //TODO
		return NULL;
	}

    static PyScheduler_SetScheduleCallback_RETURN PyScheduler_SetScheduleCallback PyScheduler_SetScheduleCallback_PROTO
    {
		PyErr_SetString( PyExc_RuntimeError, "PyScheduler_SetScheduleCallback Not yet implemented" ); //TODO
		return NULL;
    }

    static PyScheduler_SetScheduleFastCallback_RETURN PyScheduler_SetScheduleFastCallback PyScheduler_SetScheduleFastCallback_PROTO
	{
		PyErr_SetString( PyExc_RuntimeError, "PyScheduler_SetScheduleFastCallback Not yet implemented" ); //TODO
		return;
	}

    static PyScheduler_CallMethod_Main_RETURN PyScheduler_CallMethod_Main PyScheduler_CallMethod_Main_PROTO
    {
		PyErr_SetString( PyExc_RuntimeError, "PyScheduler_CallMethod_Main Not yet implemented" ); //TODO
		return NULL;
    }

}	// extern C

// End C API
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

static PyObject*
	enable_soft_switch( PyObject* self, PyObject* args )
{
	PyObject* soft_switch_value;

    if( PyArg_ParseTuple( args, "O:set_callable", &soft_switch_value ) )
	{
		if( soft_switch_value != Py_None )
		{
			PyErr_SetString( PyExc_RuntimeError, "enable_soft_switch is only implemented for legacy reasons, the value cannot be changed." ); //TODO
			return NULL;
        }
	}

	return Py_False;
}

// TEMP
static PyObject*
	Scheduler_getcurrent( PyObject* self, PyObject* Py_UNUSED( ignored ) )
{
	Scheduler* current_scheduler = Scheduler::get_scheduler();

	Py_INCREF( current_scheduler->m_current_tasklet );

	return reinterpret_cast<PyObject*>( current_scheduler->m_current_tasklet );
}

static PyObject*
	Scheduler_getmain( PyObject* self, PyObject* Py_UNUSED( ignored ) )
{
	Scheduler* current_scheduler = Scheduler::get_scheduler();

	Py_INCREF( current_scheduler->m_scheduler_tasklet );

	return reinterpret_cast<PyObject*>( current_scheduler->m_scheduler_tasklet );
}

static PyObject*
	Scheduler_getruncount( PyObject* self, PyObject* Py_UNUSED( ignored ) )
{
	Scheduler* current_scheduler = Scheduler::get_scheduler();

	return PyLong_FromLong( current_scheduler->get_tasklet_count() + 1 ); // +1 is the main tasklet
}

static PyObject*
	Scheduler_schedule( PyObject* self, PyObject* Py_UNUSED( ignored ) )
{
	Scheduler* current_scheduler = Scheduler::get_scheduler();

	PyErr_SetString( PyExc_RuntimeError, "Scheduler_schedule Not yet implemented" ); //TODO
	return NULL;
}

static PyObject*
	Scheduler_scheduleremove( PyObject* self, PyObject* Py_UNUSED( ignored ) )
{
	Scheduler* current_scheduler = Scheduler::get_scheduler();

	PyErr_SetString( PyExc_RuntimeError, "Scheduler_scheduleremove Not yet implemented" ); //TODO
	return NULL;
}

static PyObject*
	Scheduler_run( PyObject* self, PyObject* Py_UNUSED( ignored ) )
{
	Scheduler* current_scheduler = Scheduler::get_scheduler();  //TODO not needed it's a static function

    PyObject* ret = current_scheduler->run();

    // TODO the schedulers are never deleted, they will need to be deleted on thread joining back
    // Need to get a hook into this or something


	return ret;
}

static PyObject*
	Scheduler_set_schedule_callback( PyObject* self, PyObject* Py_UNUSED( ignored ) )
{
	Scheduler* current_scheduler = Scheduler::get_scheduler();

	PyErr_SetString( PyExc_RuntimeError, "Scheduler_set_schedule_callback Not yet implemented" ); //TODO
	return NULL;
}

static PyObject*
	Scheduler_get_schedule_callback( PyObject* self, PyObject* Py_UNUSED( ignored ) )
{
	Scheduler* current_scheduler = Scheduler::get_scheduler();

	PyErr_SetString( PyExc_RuntimeError, "Scheduler_get_schedule_callback Not yet implemented" ); //TODO
	return NULL;
}

static PyObject*
	Scheduler_get_thread_info( PyObject* self, PyObject* args, PyObject* kwds )
{
	Scheduler* current_scheduler = Scheduler::get_scheduler();

	PyObject* thread_info_tuple = PyTuple_New( 3 );

	Py_INCREF( current_scheduler->m_scheduler_tasklet );

	PyTuple_SetItem( thread_info_tuple, 0, reinterpret_cast<PyObject*>( current_scheduler->m_scheduler_tasklet ) );

	Py_INCREF( current_scheduler->m_current_tasklet );

	PyTuple_SetItem( thread_info_tuple, 1, reinterpret_cast<PyObject*>( current_scheduler->m_current_tasklet ) );

	PyTuple_SetItem( thread_info_tuple, 2, PyLong_FromLong( current_scheduler->get_tasklet_count() + 1 ) );

	return thread_info_tuple;
}


//TODO below doesn't work anymore, was rubbish anyway, needs to work per thread
static PyObject*
	Scheduler_set_current_tasklet_changed_callback( PyObject* self, PyObject* args, PyObject* kwds )
{
	Scheduler* current_scheduler = Scheduler::get_scheduler();

	PyObject* temp;

	if( PyArg_ParseTuple( args, "O:set_current_tasklet_changed_callback", &temp ) )
	{
		if( !PyCallable_Check( temp ) )
		{
			PyErr_SetString( PyExc_TypeError, "parameter must be callable" );
			return NULL;
		}

		Py_INCREF( temp );

		current_scheduler->m_current_tasklet_changed_callback = temp;
	}

	return Py_None;
}

static PyObject*
	Scheduler_switch_trap( PyObject* self, PyObject* args, PyObject* kwds )
{
	Scheduler* current_scheduler = Scheduler::get_scheduler();

	//TODO: channels need to track this and raise runtime error if appropriet
	int delta;

	if( !PyArg_ParseTuple( args, "i:delta", &delta ) )
	{
		PyErr_SetString( PyExc_RuntimeError, "Scheduler_switch_trap requires a delta argument." ); //TODO
	}

	current_scheduler->m_switch_trap_level += delta;

	return PyLong_FromLong( current_scheduler->m_switch_trap_level );
}

static PyObject*
	Scheduler_new_scheduler_tasklet( PyObject* self, PyObject* Py_UNUSED( ignored ) )
{
	//Make a tasklet for scheduler

	PyObject* dict = PyModule_GetDict( self );

	PyObject* tasklet_type = PyDict_GetItemString( dict, "Tasklet" ); //Weak Linkage TODO

	PyObject* scheduler_callable = PyDict_GetItemString( dict, "run" ); //Weak Linkage TODO

	PyObject* args = PyTuple_New( 1 );

	PyTuple_SetItem( args, 0, scheduler_callable );

	PyObject* scheduler_tasklet = PyObject_CallObject( tasklet_type, args );

	Py_DecRef( args );

	// Setup the schedulers tasklet
	PyTaskletObject* tasklet = (PyTaskletObject*)scheduler_tasklet;

	tasklet->m_is_main = true;

    return scheduler_tasklet;

}

static PyMethodDef SchedulerMethods[] = {
	{ "set_channel_callback", set_channel_callback, METH_VARARGS, "Install a global channel callback" },
	{ "get_channel_callback", get_channel_callback, METH_VARARGS, "Get the current global channel callback" },
	{ "set_scheduler_callback", set_scheduler_callback, METH_VARARGS, "Get the current global channel callback" },
	{ "enable_softswitch", enable_soft_switch, METH_VARARGS, "Legacy support" },

    { "getcurrent", (PyCFunction)Scheduler_getcurrent, METH_NOARGS, "Return the currently executing tasklet of this thread" },
	{ "getmain", (PyCFunction)Scheduler_getmain, METH_NOARGS, "Return the main tasklet of this thread" },
	{ "getruncount", (PyCFunction)Scheduler_getruncount, METH_NOARGS, "Return the number of currently runnable tasklets" },
	{ "schedule", (PyCFunction)Scheduler_schedule, METH_NOARGS, "Yield execution of the currently running tasklet" },
	{ "schedule_remove", (PyCFunction)Scheduler_scheduleremove, METH_NOARGS, "Yield execution of the currently running tasklet and remove" },
	{ "run", (PyCFunction)Scheduler_run, METH_NOARGS, "Run scheduler" },
	{ "set_schedule_callback", (PyCFunction)Scheduler_set_schedule_callback, METH_NOARGS, "Install a callback for scheduling" },
	{ "get_schedule_callback", (PyCFunction)Scheduler_get_schedule_callback, METH_NOARGS, "Get the current global schedule callback" },
	{ "get_thread_info", (PyCFunction)Scheduler_get_thread_info, METH_VARARGS, "Return a tuple containing the threads main tasklet, current tasklet and run-count" },
	{ "set_current_tasklet_changed_callback", (PyCFunction)Scheduler_set_current_tasklet_changed_callback, METH_VARARGS, "TODO" },
	{ "switch_trap", (PyCFunction)Scheduler_switch_trap, METH_VARARGS, "When the switch trap level is non-zero, any tasklet switching, e.g. due channel action or explicit, will result in a RuntimeError being raised." },
	{ "newSchedulerTasklet", (PyCFunction)Scheduler_new_scheduler_tasklet, METH_NOARGS, "Create a tasklet that is setup as a main tasklet for use as a schedulers tasklet" },

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
		Py_DECREF( &TaskletType );
		Py_DECREF( &ChannelType );
		Py_DECREF( m );
		return NULL;
	}

	//Exceptions
    TaskletExit = PyErr_NewException( "carbon-scheduler.TaskletExit", NULL, NULL );
	Py_XINCREF( TaskletExit );
	if( PyModule_AddObject( m, "error", TaskletExit ) < 0 )
	{
		Py_DECREF( &TaskletType );
		Py_DECREF( &ChannelType );
		Py_XDECREF( TaskletExit );
		Py_CLEAR( TaskletExit );
        Py_DECREF(m);
        return NULL;
    }

    // Import Greenlet
	PyObject* greenlet_module = PyImport_ImportModule( "greenlet" );

    if( !greenlet_module )
	{
		PySys_WriteStdout( "Failed to import greenlet module\n" );
		Py_DECREF( &TaskletType );
		Py_DECREF( &ChannelType );
		Py_XDECREF( TaskletExit );
		Py_CLEAR( TaskletExit );
		Py_DECREF( m );
		return NULL;
	}

	//C_API
	/* Initialize the C API pointer array */
    // Types
	PyScheduler_API[PyTasklet_Type_NUM] = (void*)&TaskletType;
	PyScheduler_API[PyChannel_Type_NUM] = (void*)&ChannelType;

    // Exceptions
	PyScheduler_API[PyExc_TaskletExit_NUM] = (void*)&TaskletExit;

    // Tasklet Functions
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
	PyScheduler_API[PyScheduler_SetChannelCallback_NUM] = (void*)PyScheduler_SetChannelCallback;
	PyScheduler_API[PyScheduler_SetScheduleCallback_NUM] = (void*)PyScheduler_SetScheduleCallback;

	/* Create a Capsule containing the API pointer array's address */
	c_api_object = PyCapsule_New( (void*)PyScheduler_API, "scheduler._C_API", NULL );

	if( PyModule_AddObject( m, "_C_API", c_api_object ) < 0 )
	{
		Py_DECREF( &TaskletType );
		Py_DECREF( &ChannelType );
		Py_XDECREF( TaskletExit );
		Py_CLEAR( TaskletExit );
		Py_XDECREF( c_api_object );
		Py_DECREF( m );
		return NULL;
	}

    //Create a main scheduler TODO So far never dies, need to kill it in module free function or deal with this differently
    //TODO Needs thought
	PyObject* dict = PyModule_GetDict( m );
	PyObject* create_scheduler_tasklet_callable = PyDict_GetItemString( dict, "newSchedulerTasklet" ); //Weak Linkage TODO
    Scheduler::s_create_scheduler_tasklet_callable = create_scheduler_tasklet_callable; //TODO never cleaned up
	
    //Create a main scheduler (I think this will go soon)
    //Scheduler::s_singleton = new Scheduler();

    return m;
}

