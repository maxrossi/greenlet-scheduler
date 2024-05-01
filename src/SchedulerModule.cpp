#define PY_SSIZE_T_CLEAN
#include "stdafx.h"

#define SCHEDULER_MODULE
#include "Scheduler.h"
#include <greenlet.h>
#include <string>

#include "ScheduleManager.h"

//Types
#include "PyTasklet.cpp"
#include "PyChannel.cpp"



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

		Py_IncRef( temp );

		PyObject* previous_callback = Channel::channel_callback();

		Channel::set_channel_callback(temp);

		return previous_callback ? previous_callback : Py_None;
	}

	return nullptr;
}

static PyObject*
	get_channel_callback( PyObject* self, PyObject* args )
{
	PyObject* callable = Channel::channel_callback();

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

static PyObject*
	Scheduler_getcurrent( PyObject* self, PyObject* Py_UNUSED( ignored ) )
{
	ScheduleManager* current_scheduler = ScheduleManager::get_scheduler();

    PyObject* py_current_tasklet = current_scheduler->get_current_tasklet()->python_object();

	Py_IncRef( py_current_tasklet );

	return py_current_tasklet;
}

static PyObject*
	Scheduler_getmain( PyObject* self, PyObject* Py_UNUSED( ignored ) )
{
	ScheduleManager* current_scheduler = ScheduleManager::get_scheduler();

    PyObject* py_main_tasklet = current_scheduler->get_main_tasklet()->python_object();

	Py_IncRef( py_main_tasklet );

	return py_main_tasklet;
}

static PyObject*
	Scheduler_getruncount( PyObject* self, PyObject* Py_UNUSED( ignored ) )
{
	ScheduleManager* current_scheduler = ScheduleManager::get_scheduler();

	return PyLong_FromLong( current_scheduler->get_tasklet_count() ); 
}

static PyObject*
	Scheduler_schedule( PyObject* self, PyObject* Py_UNUSED( ignored ) )
{
	ScheduleManager* current_scheduler = ScheduleManager::get_scheduler();

	if( current_scheduler->schedule() )
	{
		return Py_None;
	}
	else
	{
		return nullptr;
    }
}

static PyObject*
	Scheduler_scheduleremove( PyObject* self, PyObject* Py_UNUSED( ignored ) )
{
	ScheduleManager* current_scheduler = ScheduleManager::get_scheduler();

	if( current_scheduler->schedule(true) )
	{
		return Py_None;
	}
	else
	{
		return nullptr;
	}
}

static PyObject*
	Scheduler_run( PyObject* self, PyObject* Py_UNUSED( ignored ) )
{
	ScheduleManager* current_scheduler = ScheduleManager::get_scheduler(); //TODO not needed it's a static function

    PyObject* ret = current_scheduler->run();

    // TODO the schedulers are never deleted, they will need to be deleted on thread joining back
    // Need to get a hook into this or something


	return ret;
}

static PyObject*
	Scheduler_run_n_tasklets( PyObject* self, PyObject* args )
{
	unsigned int number_of_tasklets = 0;

    if( PyArg_ParseTuple( args, "I:set_channel_callback", &number_of_tasklets ) )
	{
		ScheduleManager* current_scheduler = ScheduleManager::get_scheduler(); //TODO not needed it's a static function

		PyObject* ret = current_scheduler->run_n_tasklets( number_of_tasklets );

        Py_IncRef( ret );

		return ret;
	}
	else
	{
		return nullptr;
    }
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

        ScheduleManager* current_scheduler = ScheduleManager::get_scheduler();

		Py_IncRef( temp );

        PyObject* previous_callback = current_scheduler->scheduler_callback();

        ScheduleManager::set_scheduler_callback( temp );

		return previous_callback ? previous_callback : Py_None;
	}

	return nullptr;
}

static PyObject*
	Scheduler_get_schedule_callback( PyObject* self, PyObject* Py_UNUSED( ignored ) )
{
	ScheduleManager* current_scheduler = ScheduleManager::get_scheduler();

    PyObject* callable = current_scheduler->scheduler_callback();

	Py_IncRef( callable );

    return callable;
    
}

static PyObject*
	Scheduler_get_thread_info( PyObject* self, PyObject* args, PyObject* kwds )
{
	ScheduleManager* current_scheduler = ScheduleManager::get_scheduler();

	PyObject* thread_info_tuple = PyTuple_New( 3 );

	Py_IncRef( current_scheduler->get_main_tasklet()->python_object() );

	PyTuple_SetItem( thread_info_tuple, 0, current_scheduler->get_main_tasklet()->python_object());

	Py_IncRef( current_scheduler->get_current_tasklet()->python_object() );

	PyTuple_SetItem( thread_info_tuple, 1, current_scheduler->get_current_tasklet()->python_object());

	PyTuple_SetItem( thread_info_tuple, 2, PyLong_FromLong( current_scheduler->get_tasklet_count() + 1 ) );

	return thread_info_tuple;
}

//TODO below doesn't work anymore, was rubbish anyway, needs to work per thread
static PyObject*
	Scheduler_set_current_tasklet_changed_callback( PyObject* self, PyObject* args, PyObject* kwds )
{
	ScheduleManager* current_scheduler = ScheduleManager::get_scheduler();

	PyObject* temp;

	if( PyArg_ParseTuple( args, "O:set_current_tasklet_changed_callback", &temp ) )
	{
		if( !PyCallable_Check( temp ) )
		{
			PyErr_SetString( PyExc_TypeError, "parameter must be callable" );
			return NULL;
		}

		Py_IncRef( temp );

		current_scheduler->set_current_tasklet_changed_callback( temp );
	}

	return Py_None;
}

static PyObject*
	Scheduler_switch_trap( PyObject* self, PyObject* args, PyObject* kwds )
{
	ScheduleManager* current_scheduler = ScheduleManager::get_scheduler();

	//TODO: channels need to track this and raise runtime error if appropriet
	int delta;

	if( !PyArg_ParseTuple( args, "i:delta", &delta ) )
	{
		PyErr_SetString( PyExc_RuntimeError, "Scheduler_switch_trap requires a delta argument." ); //TODO
	}

    long original_switch_trap = current_scheduler->switch_trap_level();

	current_scheduler->set_switch_trap_level( original_switch_trap + delta );

	return PyLong_FromLong( original_switch_trap );
}

static PyObject*
	Scheduler_new_scheduler_tasklet( PyObject* self, PyObject* Py_UNUSED( ignored ) )
{
	//Make a tasklet for scheduler

	PyObject* dict = PyModule_GetDict( self );

	PyObject* tasklet_type = PyDict_GetItemString( dict, "tasklet" ); //Weak Linkage TODO

	PyObject* scheduler_callable = PyDict_GetItemString( dict, "run" ); //Weak Linkage TODO

	PyObject* args = PyTuple_New( 1 );

	PyTuple_SetItem( args, 0, scheduler_callable );

	PyObject* scheduler_tasklet = PyObject_CallObject( tasklet_type, args );

	Py_DecRef( args );

	// Setup the schedulers tasklet
	PyTaskletObject* tasklet = (PyTaskletObject*)scheduler_tasklet;

	tasklet->m_impl->set_is_main( true );

    return scheduler_tasklet;

}

void module_destructor( void* )
{
    // Cleanup
	Py_XDECREF( ScheduleManager::s_create_scheduler_tasklet_callable );
}



/*
C Interface
*/
extern "C"
{
	// Tasklet functions
	static PyTaskletObject* PyTasklet_New( PyTypeObject* type, PyObject* args )
	{
		PyObject* scheduler_tasklet = PyObject_CallObject( reinterpret_cast<PyObject*>( type ), args );

		return reinterpret_cast<PyTaskletObject*>( scheduler_tasklet );
	}

	static int PyTasklet_Setup( PyTaskletObject* tasklet, PyObject* args, PyObject* kwds )
	{
		return Tasklet_setup( reinterpret_cast<PyObject*>( tasklet ), args, kwds ) == Py_None ? 0 : -1;
	}

	static int PyTasklet_Insert( PyTaskletObject* self )
	{
		return self->m_impl->insert() ? 0 : -1;
	}

	static int PyTasklet_GetBlockTrap( PyTaskletObject* self )
	{
		return self->m_impl->blocktrap() ? 1 : 0;
	}

	static void PyTasklet_SetBlockTrap( PyTaskletObject* task, int value )
	{
		task->m_impl->set_blocktrap( value );
	}

	static int PyTasklet_IsMain( PyTaskletObject* tasklet )
	{
		return tasklet->m_impl->is_main() ? 1 : 0;
	}

    static int PyTasklet_Check( PyObject* obj )
	{
		return obj && PyObject_TypeCheck( obj, &TaskletType );
	}

    static int PyTasklet_Alive( PyTaskletObject* tasklet )
	{
		return tasklet->m_impl->alive() ? 1 : 0;
	}

    static int PyTasklet_Kill( PyTaskletObject* tasklet )
	{
		return tasklet->m_impl->kill() ? 0 : 1;
	}

	// Channel functions
	static PyChannelObject* PyChannel_New( PyTypeObject* type )
	{
		PyTypeObject* channel_type = type;

        if (!channel_type)
        {
			channel_type = &ChannelType;
        }

		PyObject* scheduler_channel = PyObject_CallObject( reinterpret_cast<PyObject*>( channel_type ), nullptr );

		return reinterpret_cast < PyChannelObject*>(scheduler_channel);
	}

	static int PyChannel_Send( PyChannelObject* self, PyObject* arg )
	{
		return self->m_impl->send( arg ) ? 0 : -1;
	}

	static PyObject* PyChannel_Receive( PyChannelObject* self )
	{
		return self->m_impl->receive();
	}

	static int PyChannel_SendException( PyChannelObject* self, PyObject* klass, PyObject* value )
	{
		PyObject* args = PyTuple_New( 2 );

		PyTuple_SetItem( args, 0, klass );

		PyTuple_SetItem( args, 1, value );

		bool retval = self->m_impl->send( args, true );

        Py_DecRef( args );

		return retval;
	}

	static PyObject* PyChannel_GetQueue( PyChannelObject* self )
	{
		return Channel_queue_get( self, nullptr );
	}

	static void PyChannel_SetPreference( PyChannelObject* self, int val )
	{
		int sanitised_value = val;

        if (val < -1)
        {
			sanitised_value = -1;
        }
		else if(val > 1)
        {
			sanitised_value = 1;
        }

		self->m_impl->set_preference( sanitised_value );
	}

    static int PyChannel_GetPreference( PyChannelObject* self )
	{
		return self->m_impl->preference( );
	}

	static int PyChannel_GetBalance( PyChannelObject* self )
	{
		return self->m_impl->balance();
	}

    static int PyChannel_Check( PyObject* obj )
	{
		return obj && PyObject_TypeCheck( obj, &ChannelType );
	}

	// Scheduler functions
	static PyObject* PyScheduler_Schedule( PyObject* retval, int remove )
	{
		if(remove == 0)
		{
			return Scheduler_schedule( nullptr, nullptr );
        }
		else
		{
			return Scheduler_scheduleremove( nullptr, nullptr );
        }
	}

	static int PyScheduler_GetRunCount()
	{
		return ScheduleManager::get_tasklet_count();
	}

	static PyObject* PyScheduler_GetCurrent()
	{
		PyObject* current = ScheduleManager::get_current_tasklet()->python_object();

        Py_IncRef( current );

		return current;
	}

	// Note: flags used in game are PY_WATCHDOG_SOFT | PY_WATCHDOG_IGNORE_NESTING | PY_WATCHDOG_TOTALTIMEOUT
	// Implementation treats these flags as default behaviour
    // flags field is deprecated. Left in for stub compatibility
	static PyObject* PyScheduler_RunWatchdogEx( long long timeout, int flags )
	{
		return ScheduleManager::run_tasklets_for_time( timeout );
	}

    static PyObject* PyScheduler_RunNTasklets( int number_of_tasklets_to_run )
	{
		return ScheduleManager::run_n_tasklets( number_of_tasklets_to_run );
	}

	static int PyScheduler_SetChannelCallback( PyObject* callable )
	{
		if( callable && !PyCallable_Check( callable ) )
		{
			return -1;
		}

        Channel::set_channel_callback( callable );

		return 0;
	}

	static PyObject* PyScheduler_GetChannelCallback()
	{
		PyObject* channel_callback = Channel::channel_callback();

        return channel_callback;
        
	}

	static int PyScheduler_SetScheduleCallback( PyObject* callable )
	{
        if (callable && !PyCallable_Check(callable))
        {
			return -1;
        }

		ScheduleManager* current_scheduler = ScheduleManager::get_scheduler();

		current_scheduler->set_scheduler_callback( callable );

		return 0;
	}

	static void PyScheduler_SetScheduleFastCallback( schedule_hook_func func )
	{
		ScheduleManager* current_scheduler = ScheduleManager::get_scheduler();

		current_scheduler->set_scheduler_fast_callback( func );
	}

	static PyObject* PyScheduler_CallMethod_Main( PyObject* o, char* name, char* format, ... )
    {
		PyErr_SetString( PyExc_RuntimeError, "PyScheduler_CallMethod_Main Not yet implemented" ); //TODO
		return NULL;
    }

} // extern C


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
	{ "run_n_tasklets", (PyCFunction)Scheduler_run_n_tasklets, METH_VARARGS, "Run scheduler stopping after n tasklets" },
	{ "set_schedule_callback", (PyCFunction)Scheduler_set_schedule_callback, METH_VARARGS, "Install a callback for scheduling" },
	{ "get_schedule_callback", (PyCFunction)Scheduler_get_schedule_callback, METH_NOARGS, "Get the current global schedule callback" },
	{ "get_thread_info", (PyCFunction)Scheduler_get_thread_info, METH_VARARGS, "Return a tuple containing the threads main tasklet, current tasklet and run-count" },
	{ "set_current_tasklet_changed_callback", (PyCFunction)Scheduler_set_current_tasklet_changed_callback, METH_VARARGS, "TODO" },
	{ "switch_trap", (PyCFunction)Scheduler_switch_trap, METH_VARARGS, "When the switch trap level is non-zero, any tasklet switching, e.g. due channel action or explicit, will result in a RuntimeError being raised." },
	{ "newSchedulerTasklet", (PyCFunction)Scheduler_new_scheduler_tasklet, METH_NOARGS, "Create a tasklet that is setup as a main tasklet for use as a schedulers tasklet" },

	{ NULL, NULL, 0, NULL } /* Sentinel */
};

/**
 * <Name> and <Name>_DIRECT Pattern is used here because:
 * s1##s2 will not expand s1 & s2 symbols, however a parent macro will
 * So this:
 * CONCATENATE(s1, s2) s1##s2 will not expand either symbol s1 or s2
 * An Extra step is needed to expand s1 and s2, hence:
 *
 * CONCATENATE_DIRECT(s1, s2) s1##s2 // directly concatinate s1 and s2 without resolving them
 * CONCATENATE(s1, s2) CONCATENATE_DIRECT(s1, s2) // resolve s1 and s2 before "passing them" to CONCATENATE_DIRECT
 */
#define CONCATENATE_DIRECT(s1, s2) s1##s2
#define CONCATENATE(s1, s2) CONCATENATE_DIRECT(s1, s2)

#define STRING_DIRECT(s) #s
#define STRING(s) STRING_DIRECT(s)
#define CONCATENATE_TO_STRING(s1, s2) STRING(CONCATENATE_DIRECT(s1, s2))

static struct PyModuleDef schedulermodule = {
    PyModuleDef_HEAD_INIT,
    CONCATENATE_TO_STRING(_scheduler, CCP_BUILD_FLAVOR),   /* name of module */
    NULL, /* module documentation, may be NULL */
    -1,       /* size of per-interpreter state of the module,
                 or -1 if the module keeps state in global variables. */
    SchedulerMethods,
    NULL,
    NULL,
    NULL,
	module_destructor
};

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
	if( PyModule_AddObject( m, "tasklet", (PyObject*)&TaskletType ) < 0 )
	{
		Py_DECREF( &TaskletType );
		Py_DECREF( m );
		return NULL;
	}

	Py_INCREF( &ChannelType );
	if( PyModule_AddObject( m, "channel", (PyObject*)&ChannelType ) < 0 )
	{
		Py_DECREF( &TaskletType );
		Py_DECREF( &ChannelType );
		Py_DECREF( m );
		return NULL;
	}

	//Exceptions
	auto exit_exception_string = CONCATENATE_TO_STRING(_scheduler, CCP_BUILD_FLAVOR) + std::string(".TaskletExit");
	TaskletExit = PyErr_NewException( exit_exception_string.c_str(), NULL, NULL );
	Py_XINCREF( TaskletExit );
	if( PyModule_AddObject( m, "TaskletExit", TaskletExit ) < 0 )
	{
		Py_DECREF( &TaskletType );
		Py_DECREF( &ChannelType );
		Py_XDECREF( TaskletExit );
		Py_CLEAR( TaskletExit );
        Py_DECREF(m);
        return NULL;
    }

    // Import Greenlet
	PyObject* greenlet_module = PyImport_ImportModule( "greenlet" );    //TODO cleanup

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
	api.PyTasklet_New = PyTasklet_New;
    api.PyTasklet_Setup = PyTasklet_Setup;
	api.PyTasklet_Insert = PyTasklet_Insert;
	api.PyTasklet_GetBlockTrap = PyTasklet_GetBlockTrap;
	api.PyTasklet_SetBlockTrap = PyTasklet_SetBlockTrap;
	api.PyTasklet_IsMain = PyTasklet_IsMain;
	api.PyTasklet_Check = PyTasklet_Check;
	api.PyTasklet_Alive = PyTasklet_Alive;
	api.PyTasklet_Kill = PyTasklet_Kill;

    // Channel Functions
	api.PyChannel_New = PyChannel_New;
	api.PyChannel_Send = PyChannel_Send;
	api.PyChannel_Receive = PyChannel_Receive;
	api.PyChannel_SendException = PyChannel_SendException;
	api.PyChannel_GetQueue = PyChannel_GetQueue;
	api.PyChannel_GetPreference = PyChannel_GetPreference;
	api.PyChannel_SetPreference = PyChannel_SetPreference;
	api.PyChannel_GetBalance = PyChannel_GetBalance;
	api.PyChannel_Check = PyChannel_Check;

    //Scheduler Functions
	api.PyScheduler_Schedule = PyScheduler_Schedule;
	api.PyScheduler_GetRunCount = PyScheduler_GetRunCount;
	api.PyScheduler_GetCurrent = PyScheduler_GetCurrent;
	api.PyScheduler_RunWatchdogEx = PyScheduler_RunWatchdogEx;
	api.PyScheduler_RunNTasklets = PyScheduler_RunNTasklets;
	api.PyScheduler_SetChannelCallback = PyScheduler_SetChannelCallback;
	api.PyScheduler_GetChannelCallback = PyScheduler_GetChannelCallback;
	api.PyScheduler_SetScheduleCallback = PyScheduler_SetScheduleCallback;
	api.PyScheduler_SetScheduleFastCallback = PyScheduler_SetScheduleFastCallback;
	api.PyScheduler_CallMethod_Main = PyScheduler_CallMethod_Main;

	/* Create a Capsule containing the API pointer array's address */
	//c_api_object = PyCapsule_New( (void*)&api, "_scheduler_debug._C_API", NULL );
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
	ScheduleManager::s_create_scheduler_tasklet_callable = create_scheduler_tasklet_callable;
	
    //Setup initial channel callback static
	Channel::set_channel_callback(nullptr);

    //Setup scheduler attributes
	PyObject* main = Scheduler_getmain( m, nullptr );
	PyObject_SetAttrString( m, "main", main );
	PyObject_SetAttrString( m, "current", main );
	ScheduleManager::s_scheduler_module = m;
  
    return m;
}
