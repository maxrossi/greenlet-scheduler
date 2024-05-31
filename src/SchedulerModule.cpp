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
#include "PyScheduleManager.cpp"

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

		if( previous_callback )
		{
			return previous_callback;
		}
		else
		{
			Py_IncRef( Py_None );

			return Py_None;
		}
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

    Tasklet* current_tasklet = current_scheduler->get_current_tasklet();

    current_tasklet->incref();

    current_scheduler->decref();

	return current_tasklet->python_object();
}

static PyObject*
	Scheduler_getmain( PyObject* self, PyObject* Py_UNUSED( ignored ) )
{
	ScheduleManager* current_scheduler = ScheduleManager::get_scheduler();

    PyObject* py_main_tasklet = current_scheduler->get_main_tasklet()->python_object();

	Py_IncRef( py_main_tasklet );

    current_scheduler->decref();

	return py_main_tasklet;
}

static PyObject*
	Scheduler_getruncount( PyObject* self, PyObject* Py_UNUSED( ignored ) )
{
	ScheduleManager* current_scheduler = ScheduleManager::get_scheduler();

    PyObject* ret = PyLong_FromLong( current_scheduler->get_tasklet_count() );

    current_scheduler->decref();

	return ret;
}

static PyObject*
	Scheduler_calculateruncount( PyObject* self, PyObject* Py_UNUSED( ignored ) )
{
	ScheduleManager* current_scheduler = ScheduleManager::get_scheduler();

	PyObject* ret = PyLong_FromLong( current_scheduler->calculate_tasklet_count() );

	current_scheduler->decref();

	return ret;
}

static PyObject*
	Scheduler_schedule( PyObject* self, PyObject* Py_UNUSED( ignored ) )
{
	ScheduleManager* current_scheduler = ScheduleManager::get_scheduler();

    bool schedule_successful = current_scheduler->schedule();

    current_scheduler->decref();

	if( schedule_successful )
	{
		Py_IncRef( Py_None );

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

    bool schedule_remove_successful = current_scheduler->schedule( true );

    current_scheduler->decref();

	if( schedule_remove_successful )
	{
		Py_IncRef( Py_None );

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
	ScheduleManager* current_scheduler = ScheduleManager::get_scheduler();

    bool ret = current_scheduler->run();

    current_scheduler->decref();

    if (ret)
    {
		Py_IncRef( Py_None );

        return Py_None;
    }
    else
    {
		return nullptr;
    }
}

static PyObject*
	Scheduler_run_n_tasklets( PyObject* self, PyObject* args )
{
	unsigned int number_of_tasklets = 0;

    if( PyArg_ParseTuple( args, "I:set_channel_callback", &number_of_tasklets ) )
	{
		ScheduleManager* current_scheduler = ScheduleManager::get_scheduler();

		bool ret = current_scheduler->run_n_tasklets( number_of_tasklets );

        current_scheduler->decref();

        if (ret)
        {
			Py_IncRef( Py_None );

            return Py_None;
        }
        else
        {
			return nullptr;
        }
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

        current_scheduler->set_scheduler_callback( temp );

        current_scheduler->decref();

		if( previous_callback )
		{
			return previous_callback;
		}
		else
		{
			Py_IncRef( Py_None );

			return Py_None;
		}
	}

	return nullptr;
}

static PyObject*
	Scheduler_get_schedule_callback( PyObject* self, PyObject* Py_UNUSED( ignored ) )
{
	ScheduleManager* current_scheduler = ScheduleManager::get_scheduler();

    PyObject* callable = current_scheduler->scheduler_callback();

	Py_IncRef( callable );

    current_scheduler->decref();

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

    current_scheduler->decref();

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

    current_scheduler->decref();

	Py_IncRef( Py_None );

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

    current_scheduler->decref();

	return PyLong_FromLong( original_switch_trap );
}

static PyObject*
	Scheduler_get_schedule_manager( PyObject* self, PyObject* Py_UNUSED( ignored ) )
{
	ScheduleManager* schedule_manager = ScheduleManager::get_scheduler( );

    if (schedule_manager)
    {
        return schedule_manager->python_object();
    }
    else
    {
		Py_IncRef( Py_None );

		return Py_None;
    }
}

static PyObject*
	Scheduler_get_number_of_active_schedule_managers( PyObject* self, PyObject* Py_UNUSED( ignored ) )
{
	int num_schedule_managers = ScheduleManager::num_active_schedule_managers( );

    return PyLong_FromLong( num_schedule_managers );
}


static PyObject*
	Scheduler_get_number_of_active_channels( PyObject* self, PyObject* Py_UNUSED( ignored ) )
{
	
	int num_channels = Channel::num_active_channels();

	return PyLong_FromLong( num_channels );
}

static PyObject*
	Scheduler_unblock_all_channels( PyObject* self, PyObject* Py_UNUSED( ignored ) )
{
	int num_channels = Channel::unblock_all_channels();

	return PyLong_FromLong( num_channels );
}

void module_destructor( void* )
{
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
		if(Tasklet_setup( reinterpret_cast<PyObject*>( tasklet ), args, kwds ) == Py_None)
		{
			Py_DecRef( Py_None );

			return 0;
		}
		else
		{
			return -1;
		}
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
        if (klass == nullptr)
        {
			PyErr_SetString( PyExc_RuntimeError, "Exception type or instance required" );
			return -1;
        }

        if( !PyExceptionClass_Check( klass ) && !PyObject_IsInstance( klass, PyExc_Exception ) )
		{
			PyErr_SetString( PyExc_RuntimeError, "Exception type or instance required" );
			return -1;
		}

		PyObject* args = nullptr;
		
        if (value)
        {
			args = PyTuple_New( 1 );

			PyTuple_SetItem( args, 0, value );
        }
        else
        {
			args = PyTuple_New( 0 );
        }

		bool ret = self->m_impl->send( args, klass );

        Py_DecRef( args );

		return ret ? 0 : -1;
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

    static int PyChannel_SendThrow( PyChannelObject* self, PyObject* exc, PyObject* val, PyObject* tb )
	{
        if (!exc)
        {
			PyErr_SetString( PyExc_RuntimeError, "Exception type or instance required" );
			return -1;
        }

        if( !PyExceptionClass_Check( exc ) && !PyObject_IsInstance( exc, PyExc_Exception ) )
		{
			PyErr_SetString( PyExc_RuntimeError, "Exception type or instance required" );
			return -1;
		}

		PyObject* args = PyTuple_New( 2 );

        if (val)
        {
			PyTuple_SetItem( args, 0, val );
        }
        else
        {
			Py_IncRef( Py_None );

			PyTuple_SetItem( args, 0, Py_None );
        }
		
        if (tb)
        {
			PyTuple_SetItem( args, 1, tb );
        }
        else
        {
			Py_IncRef( Py_None );
			
			PyTuple_SetItem( args, 1, Py_None );
        }

        bool ret = self->m_impl->send( args, exc );

        Py_DecRef( args );

		return ret ? 0 : -1;
	}

	// Scheduler functions

    //Returns new reference
	static PyObject* PyScheduler_GetScheduler( )
	{
		return ScheduleManager::get_scheduler()->python_object();
	}

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
		ScheduleManager* schedule_manager = ScheduleManager::get_scheduler();

        int ret = schedule_manager->get_tasklet_count();

        schedule_manager->decref();

		return ret;
	}

	static PyObject* PyScheduler_GetCurrent()
	{
		ScheduleManager* schedule_manager = ScheduleManager::get_scheduler();

		PyObject* current = schedule_manager->get_current_tasklet()->python_object();

        schedule_manager->decref();

        Py_IncRef( current );

		return current;
	}

	// Note: flags used in game are PY_WATCHDOG_SOFT | PY_WATCHDOG_IGNORE_NESTING | PY_WATCHDOG_TOTALTIMEOUT
	// Implementation treats these flags as default behaviour
    // flags field is deprecated. Left in for stub compatibility
	static PyObject* PyScheduler_RunWatchdogEx( long long timeout, int flags )
	{
		ScheduleManager* schedule_manager = ScheduleManager::get_scheduler();

		bool ret = schedule_manager->run_tasklets_for_time( timeout );
        
        schedule_manager->decref();

        if ( ret )
        {
			Py_IncRef( Py_None );

            return Py_None;
        }
        else
        {
			return nullptr;
        }
	}

    static PyObject* PyScheduler_RunNTasklets( int number_of_tasklets_to_run )
	{
		ScheduleManager* schedule_manager = ScheduleManager::get_scheduler();

		bool ret = schedule_manager->run_n_tasklets( number_of_tasklets_to_run );

        schedule_manager->decref();

        if (ret)
        {
			Py_IncRef( Py_None );

            return Py_None;
        }
        else
        {
			return nullptr;
        }
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

        current_scheduler->decref();

		return 0;
	}

	static void PyScheduler_SetScheduleFastCallback( schedule_hook_func func )
	{
		ScheduleManager* current_scheduler = ScheduleManager::get_scheduler();

		current_scheduler->set_scheduler_fast_callback( func );

        current_scheduler->decref();
	}

} // extern C


static PyMethodDef SchedulerMethods[] = {
	{ "set_channel_callback", set_channel_callback, METH_VARARGS, "Install a global channel callback" },
	{ "get_channel_callback", get_channel_callback, METH_VARARGS, "Get the current global channel callback" },
	{ "enable_softswitch", enable_soft_switch, METH_VARARGS, "Legacy support" },
    { "getcurrent", (PyCFunction)Scheduler_getcurrent, METH_NOARGS, "Return the currently executing tasklet of this thread" },
	{ "getmain", (PyCFunction)Scheduler_getmain, METH_NOARGS, "Return the main tasklet of this thread" },
	{ "getruncount", (PyCFunction)Scheduler_getruncount, METH_NOARGS, "Return the number of currently runnable tasklets from a cached value" },
	{ "calculateruncount", (PyCFunction)Scheduler_calculateruncount, METH_NOARGS, "Calculates and return the number of currently runnable tasklets" },
	{ "schedule", (PyCFunction)Scheduler_schedule, METH_NOARGS, "Yield execution of the currently running tasklet" },
	{ "schedule_remove", (PyCFunction)Scheduler_scheduleremove, METH_NOARGS, "Yield execution of the currently running tasklet and remove" },
	{ "run", (PyCFunction)Scheduler_run, METH_NOARGS, "Run scheduler" },
	{ "run_n_tasklets", (PyCFunction)Scheduler_run_n_tasklets, METH_VARARGS, "Run scheduler stopping after n tasklets" },
	{ "set_schedule_callback", (PyCFunction)Scheduler_set_schedule_callback, METH_VARARGS, "Install a callback for scheduling" },
	{ "get_schedule_callback", (PyCFunction)Scheduler_get_schedule_callback, METH_NOARGS, "Get the current global schedule callback" },
	{ "get_thread_info", (PyCFunction)Scheduler_get_thread_info, METH_VARARGS, "Return a tuple containing the threads main tasklet, current tasklet and run-count" },
	{ "set_current_tasklet_changed_callback", (PyCFunction)Scheduler_set_current_tasklet_changed_callback, METH_VARARGS, "TODO" },
	{ "switch_trap", (PyCFunction)Scheduler_switch_trap, METH_VARARGS, "When the switch trap level is non-zero, any tasklet switching, e.g. due channel action or explicit, will result in a RuntimeError being raised." },
	{ "get_schedule_manager", (PyCFunction)Scheduler_get_schedule_manager, METH_NOARGS, "Return the schedule manager from the thread it is called" },
	{ "get_number_of_active_schedule_managers", (PyCFunction)Scheduler_get_number_of_active_schedule_managers, METH_NOARGS, "Return the number of active schedule managers" },
	{ "get_number_of_active_channels", (PyCFunction)Scheduler_get_number_of_active_channels, METH_NOARGS, "Return the number of active channels" },
	{ "unblock_all_channels", (PyCFunction)Scheduler_unblock_all_channels, METH_NOARGS, "Unblock all active channels " },
	
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

    if( PyType_Ready( &ScheduleManagerType ) < 0 )
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

    Py_INCREF( &ScheduleManagerType );
	if( PyModule_AddObject( m, "schedule_manager", (PyObject*)&ScheduleManagerType ) < 0 )
	{
		Py_DECREF( &TaskletType );
		Py_DECREF( &ChannelType );
		Py_DECREF( &ScheduleManagerType );
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
		Py_DECREF( &ScheduleManagerType );
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
		Py_DECREF( &ScheduleManagerType );
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
	api.PyChannel_SendThrow = PyChannel_SendThrow;

    //Scheduler Functions
	api.PyScheduler_GetScheduler = PyScheduler_GetScheduler;
	api.PyScheduler_Schedule = PyScheduler_Schedule;
	api.PyScheduler_GetRunCount = PyScheduler_GetRunCount;
	api.PyScheduler_GetCurrent = PyScheduler_GetCurrent;
	api.PyScheduler_RunWatchdogEx = PyScheduler_RunWatchdogEx;
	api.PyScheduler_RunNTasklets = PyScheduler_RunNTasklets;
	api.PyScheduler_SetChannelCallback = PyScheduler_SetChannelCallback;
	api.PyScheduler_GetChannelCallback = PyScheduler_GetChannelCallback;
	api.PyScheduler_SetScheduleCallback = PyScheduler_SetScheduleCallback;
	api.PyScheduler_SetScheduleFastCallback = PyScheduler_SetScheduleFastCallback;

	/* Create a Capsule containing the API pointer array's address */
	//c_api_object = PyCapsule_New( (void*)&api, "_scheduler_debug._C_API", NULL );
	c_api_object = PyCapsule_New( (void*)&api, "scheduler._C_API", NULL );

	if( PyModule_AddObject( m, "_C_API", c_api_object ) < 0 )
	{
		Py_DECREF( &TaskletType );
		Py_DECREF( &ChannelType );
		Py_DECREF( &ScheduleManagerType );
		Py_XDECREF( TaskletExit );
		Py_CLEAR( TaskletExit );
		Py_XDECREF( c_api_object );
		Py_DECREF( m );
		return NULL;
	}

	ScheduleManager::s_schedule_manager_type = &ScheduleManagerType;
	ScheduleManager::s_tasklet_type = &TaskletType;

    //Setup initial channel callback static
	Channel::set_channel_callback(nullptr);

    // Setup scheduler attributes
    // These are now going to be broken
    // TODO left here as reminder to discuss later
    //PyObject* main = Scheduler_getmain( m, nullptr );
	//PyObject_SetAttrString( m, "main", main );
	//PyObject_SetAttrString( m, "current", main );
    
    return m;
}
