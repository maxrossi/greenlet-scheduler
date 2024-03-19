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
		return Scheduler::get_current_tasklet();
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
	set_channel_callback( PyObject* self, PyObject* args )
{
    //TODO what if a callback is supplied which requires the wrong number of arguments?
	PyObject* temp;

	if( PyArg_ParseTuple( args, "O:set_channel_callback", &temp ) )
	{
		if( !PyCallable_Check( temp ) )
		{
			PyErr_SetString( PyExc_TypeError, "parameter must be callable" );
			return nullptr;
		}

		Py_INCREF( temp );

		PyObject* previous_callback = PyChannelObject::channel_callback();

		PyChannelObject::set_channel_callback(temp);

		return previous_callback;
	}

	return nullptr;
}

static PyObject*
	get_channel_callback( PyObject* self, PyObject* args )
{
	PyObject* callable = PyChannelObject::channel_callback();

	Py_IncRef( callable );

	return callable;
}

static PyObject*
	enable_soft_switch( PyObject* self, PyObject* args )
{
	PyObject* soft_switch_value;

    if( PyArg_ParseTuple( args, "O:set_callable", &soft_switch_value ) )
	{
		if( soft_switch_value != Py_None )
		{
			PyErr_SetString( PyExc_RuntimeError, "enable_soft_switch is only implemented for legacy reasons, the value cannot be changed." );

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
	Scheduler_set_schedule_callback( PyObject* self, PyObject* args, PyObject* kwds )
{
	//TODO what if a callback is supplied which requires the wrong number of arguments?
    PyObject* temp;

	if( PyArg_ParseTuple( args, "O:set_schedule_callback", &temp ) )
	{
		if( !PyCallable_Check( temp ) )
		{
			PyErr_SetString( PyExc_TypeError, "parameter must be callable" );
			return NULL;    //TODO convert all to nullptr - left so I remember
		}

        Scheduler* current_scheduler = Scheduler::get_scheduler();

		Py_INCREF( temp );

        PyObject* previous_callback = current_scheduler->m_scheduler_callback;

        current_scheduler->m_scheduler_callback = temp;

		return previous_callback;
	}

	return nullptr;
}

static PyObject*
	Scheduler_get_schedule_callback( PyObject* self, PyObject* Py_UNUSED( ignored ) )
{
	Scheduler* current_scheduler = Scheduler::get_scheduler();

    PyObject* callable = current_scheduler->m_scheduler_callback;

	Py_IncRef( callable );

    return callable;
    
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

	tasklet->set_is_main(true);

    return scheduler_tasklet;

}

void module_destructor( void* )
{
    // Cleanup

	Py_XDECREF( Scheduler::s_create_scheduler_tasklet_callable );
}

static PyMethodDef SchedulerMethods[] = {
	{ "set_channel_callback", set_channel_callback, METH_VARARGS, "Install a global channel callback" },
	{ "get_channel_callback", get_channel_callback, METH_VARARGS, "Get the current global channel callback" },
	{ "enable_softswitch", enable_soft_switch, METH_VARARGS, "Legacy support" },
    { "getcurrent", (PyCFunction)Scheduler_getcurrent, METH_NOARGS, "Return the currently executing tasklet of this thread" },
	{ "getmain", (PyCFunction)Scheduler_getmain, METH_NOARGS, "Return the main tasklet of this thread" },
	{ "getruncount", (PyCFunction)Scheduler_getruncount, METH_NOARGS, "Return the number of currently runnable tasklets" },
	{ "schedule", (PyCFunction)Scheduler_schedule, METH_NOARGS, "Yield execution of the currently running tasklet" },
	{ "schedule_remove", (PyCFunction)Scheduler_scheduleremove, METH_NOARGS, "Yield execution of the currently running tasklet and remove" },
	{ "run", (PyCFunction)Scheduler_run, METH_NOARGS, "Run scheduler" },
	{ "set_schedule_callback", (PyCFunction)Scheduler_set_schedule_callback, METH_VARARGS, "Install a callback for scheduling" },
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
    SchedulerMethods,
    NULL,
    NULL,
    NULL,
	module_destructor
};

#define CONCATENATE_DIRECT(s1, s2) s1##s2
#define CONCATENATE(s1, s2) CONCATENATE_DIRECT(s1, s2)

PyMODINIT_FUNC
	CONCATENATE(PyInit__scheduler, CCP_BUILD_FLAVOR) (void)
{
    PyObject *m;
	static SchedulerCAPI api;
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
		Py_DECREF( &TaskletType );
		Py_DECREF( &ChannelType );
		Py_XDECREF( TaskletExit );
		Py_CLEAR( TaskletExit );
		Py_XDECREF( c_api_object );
		Py_DECREF( m );
		return NULL;
	}

    //Create a main scheduler
	PyObject* dict = PyModule_GetDict( m );
	PyObject* create_scheduler_tasklet_callable = PyDict_GetItemString( dict, "newSchedulerTasklet" ); //Weak Linkage TODO
    Scheduler::s_create_scheduler_tasklet_callable = create_scheduler_tasklet_callable;
	
    //Setup initial channel callback static
	PyChannelObject::set_channel_callback(Py_None);

    return m;
}
