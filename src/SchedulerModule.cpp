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
	Py_DECREF( ScheduleManager::s_schedule_manager_lock );
}

/*
C Interface
*/
extern "C"
{
	// Tasklet functions

	/// @brief Creates new tasklet.
	/// @param type python object type derived from PyTaskletType
	/// @param args tuple containing callable python object
	/// @return PyTaskletObject On success returns a new tasklet object, returns NULL on failure
	/// @note Returns a new reference
	static PyTaskletObject* PyTasklet_New( PyTypeObject* type, PyObject* args )
	{
		PyObject* scheduler_tasklet = PyObject_CallObject( reinterpret_cast<PyObject*>( type ), args );

		return reinterpret_cast<PyTaskletObject*>( scheduler_tasklet );
	}

    /// @brief Check if Python object is derived from TaskletType
	/// @param obj python object to check
	/// @return 1 if obj is a Tasklet type, otherwise return 0
	static int PyTasklet_Check( PyObject* obj )
	{
		return obj && PyObject_TypeCheck( obj, &TaskletType );
	}

    /// @brief Make tasklet ready to run by binding parameters to it and inserting into run queue. 
	/// @param tasklet python object type derived from PyTaskletType
	/// @param args tuple containing arguments
    /// @param args tuple containing kwords
	/// @return 0 on success -1 on failure
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

    /// @brief Insert tasklet into run queue if not already present.
	/// @param tasklet to be inserted, python object type derived from PyTaskletType
	/// @return 0 on success -1 on failure
    /// @note Raises RuntimeError on failure
	static int PyTasklet_Insert( PyTaskletObject* self )
	{
		/*
        if (!PyTasklet_Check(reinterpret_cast<PyObject*>(self)))
        {
			PyErr_SetString( PyExc_TypeError, "'self' must be derived from type PyTaskletType" );
			return -1;
        }
        */

		return self->m_impl->insert() ? 0 : -1;
	}

    /// @brief Get tasklet blocktrap status
	/// @param tasklet to be checked, python object type derived from PyTaskletType
	/// @return 1 if tasklet cannot be blocked otherwise return 0
	static int PyTasklet_GetBlockTrap( PyTaskletObject* self )
	{
		return self->m_impl->blocktrap() ? 1 : 0;
	}

    /// @brief Set tasklet blocktrap status
	/// @param tasklet to be checked, python object type derived from PyTaskletType
    /// @param value new blocktrap value
	/// @return 1 if tasklet cannot be blocked otherwise return 0
	static void PyTasklet_SetBlockTrap( PyTaskletObject* task, int value )
	{
		task->m_impl->set_blocktrap( value );
	}

    /// @brief Check if tasklet is a main tasklet
	/// @param tasklet to be checked, python object type derived from PyTaskletType
	/// @return 1 if tasklet is a main tasklet, otherwise return 0
	static int PyTasklet_IsMain( PyTaskletObject* tasklet )
	{
		return tasklet->m_impl->is_main() ? 1 : 0;
	}

    /// @brief Check if tasklet is alive
	/// @param tasklet to be checked, python object type derived from PyTaskletType
	/// @return 1 if tasklet is alive, otherwise return 0
    static int PyTasklet_Alive( PyTaskletObject* tasklet )
	{
		return tasklet->m_impl->alive() ? 1 : 0;
	}

    /// @brief Kill a tasklet. Raises TaskletExit on tasklet.
	/// @param tasklet to be killed, python object type derived from PyTaskletType
	/// @return Always returns 0
    /// @todo Change return to reflect the result of the kill command
    static int PyTasklet_Kill( PyTaskletObject* tasklet )
	{
		bool ret = tasklet->m_impl->kill();

        // This should return result of ret, however to keep with Stackless behaviour
        // this will return 0 which indicates it was hard switched
		return 0;
	}

	// Channel functions

    /// @brief Creates new channel.
	/// @param type python object type derived from PyChannelType
    /// @exception Raises TypeError if type is incorrect
	/// @return PyChannelObject On success, NULL on failure
	/// @note Returns a new reference
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

    /// @brief Send an argument over a channel
	/// @param self python object type derived from PyChannelType
	/// @param arg python object to send over channel
	/// @return 0 On success, -1 on failure
	static int PyChannel_Send( PyChannelObject* self, PyObject* arg )
	{
		return self->m_impl->send( arg ) ? 0 : -1;
	}

    /// @brief Receive an argument from a channel
	/// @param self python object type derived from PyChannelType
	/// @return received PyObject* on success, NULL on failure
	static PyObject* PyChannel_Receive( PyChannelObject* self )
	{
		return self->m_impl->receive();
	}

    /// @brief Raise an exception on first tasklet waiting to receive on channel
	/// @param self python object type derived from PyChannelType
    /// @param klass python exception
    /// @param value python exception value
	/// @return 0 on success, -1 on failure
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

    /// @brief Get first tasklet from channel blocked queue
	/// @param self python object type derived from PyChannelType
	/// @return first tasklet PyObject or NULL if blocked queue is empty
	static PyObject* PyChannel_GetQueue( PyChannelObject* self )
	{
		return Channel_queue_get( self, nullptr );
	}

    /// @brief Set the channel's preference
	/// @param self python object type derived from PyChannelType
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

    /// @brief Get the channel's preference
	/// @param self python object type derived from PyChannelType
	/// @return The channel's preference
    static int PyChannel_GetPreference( PyChannelObject* self )
	{
		return self->m_impl->preference( );
	}

    /// @brief Set the channel's balance
	/// @param self python object type derived from PyChannelType
    /// @return The channel's balance
	static int PyChannel_GetBalance( PyChannelObject* self )
	{
		return self->m_impl->balance();
	}

    /// @brief Check if Python object is a main tasklet
	/// @param obj python object to check
	/// @return 1 if obj is a Tasklet type, otherwise return 0
    static int PyChannel_Check( PyObject* obj )
	{
		return obj && PyObject_TypeCheck( obj, &ChannelType );
	}

    /// @deprecated Please use PyChannel_SendException instead
    /// @brief Throw an exception on first tasklet waiting to receive on channel
	/// @param self python object type derived from PyChannelType
	/// @param exc python exception
	/// @param val python exception value
    /// @param tb traceback
	/// @return 0 on success, -1 on failure
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

    /// @brief Get current scheduler
	/// @return Current scheduler as PyObject
    /// @note returns a new reference
	static PyObject* PyScheduler_GetScheduler( )
	{
		return ScheduleManager::get_scheduler()->python_object();
	}

    /// @brief Yield execution of current tasklet. Tasklet is added to end of run queue
	/// @param retval Py_None if successfull, NULL on failure
	/// @param remove if set tasklet will not be added to end of run queue
	/// @return Py_None on success, NULL on failure
    /// @todo remove retval and just use return type
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

    /// @brief Get number of tasklets in run queue
	/// @return Number of tasklets in run queue
	static int PyScheduler_GetRunCount()
	{
		ScheduleManager* schedule_manager = ScheduleManager::get_scheduler();

        int ret = schedule_manager->get_tasklet_count();

        schedule_manager->decref();

		return ret;
	}

	/// @brief Returns the current tasklet.
	/// @return PyObject On success returns current tasklet, returns NULL on failure
	/// @note Returns a new reference
	static PyObject* PyScheduler_GetCurrent()
	{
		ScheduleManager* schedule_manager = ScheduleManager::get_scheduler();

		PyObject* current = schedule_manager->get_current_tasklet()->python_object();

        Py_IncRef( current );

        schedule_manager->decref();

		return current;
	}

	/// @brief Run scheduler for specified number of nanoseconds
	/// @param timeout timeout value in nano seconds
	/// @param flags unused, deprecated
	/// @return Py_None on success, NULL on failure
	/// @todo rename and remove deprecated flags parameter
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

    /// @brief Run scheduler limited to n number of Tasklets
	/// @param number_of_tasklets_to_run Number of Tasklets to run before exiting
	/// @return Py_None on success, NULL on failure
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

    /// @brief Specify callable to be called on every channel send and receive
	/// @param callable method to be called, Passing NULL removes handler
	/// @return 0 on success, -1 on failure
	static int PyScheduler_SetChannelCallback( PyObject* callable )
	{
		if( callable && !PyCallable_Check( callable ) )
		{
			return -1;
		}

        Channel::set_channel_callback( callable );

		return 0;
	}

    /// @brief Get callable set to be called on every channel send and receive
	/// @return Callable, Py_None if no callback is set
	static PyObject* PyScheduler_GetChannelCallback()
	{
		PyObject* channel_callback = Channel::channel_callback();

        return channel_callback;
        
	}

    /// @brief Specify callable to be called on every schedule operation
	/// @param callable method to be called, Passing NULL removes handler
	/// @return 0 on success, -1 on failure
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

    /// @brief Specify c++ function to be called on every schedule operation
	/// @param func c++ function
	static void PyScheduler_SetScheduleFastCallback( schedule_hook_func func )
	{
		ScheduleManager* current_scheduler = ScheduleManager::get_scheduler();

		current_scheduler->set_scheduler_fast_callback( func );

        current_scheduler->decref();
	}

} // extern C

static PyMethodDef SchedulerMethods[] = {

	{ "set_channel_callback",
        set_channel_callback, 
        METH_VARARGS, 
        "Every send or receive action will result in callable being called. \n\n\
            :param callable: Callback to call on channel send/receive \n\
            :type callable: callable or None \n\
            :return: Previous channel callback. \n\
            :rtype: int" },

	{ "get_channel_callback",
        get_channel_callback,
        METH_VARARGS,
        "Get the current global channel callback. \n\n\
            :return: Current channel callback or None. \n\
            :rtype: PyObject*" },

	{ "enable_softswitch",
        enable_soft_switch,
        METH_VARARGS,
        "Legacy API support. \n\n\
            :warning: Deprecated" },

    { "getcurrent",
        (PyCFunction)Scheduler_getcurrent,
        METH_NOARGS,
        "Get the current Tasklet. \n\n\
            :return: Current Tasklet executing Tasklet of this thread \n\
            :rtype: PyObject*" },

	{ "getmain",
        (PyCFunction)Scheduler_getmain,
        METH_NOARGS,
        "Get the main tasklet of this thread. \n\n\
            :return: Main Tasklet of this thread \n\
            :rtype: PyObject*" },

	{ "getruncount",
        (PyCFunction)Scheduler_getruncount,
        METH_NOARGS,
        "Get number of currently runnable tasklets. \n\n\
            :return: Cached number of runnable tasklets \n\
            :rtype: Int." },

	{ "calculateruncount",
        (PyCFunction)Scheduler_calculateruncount,
        METH_NOARGS, "Calculate number of currently runnable tasklets. \n\n\
            :return: Calculated number of runnable tasklets \n\
            :rtype: Int." },

	{ "schedule",
        (PyCFunction)Scheduler_schedule,
        METH_NOARGS,
        "Yield execution of current tasklet. Tasklet is added to end of run queue." },

	{ "schedule_remove",
        (PyCFunction)Scheduler_scheduleremove,
        METH_NOARGS,
        "Yield execution of current tasklet. Tasklet is don't add to end of run queue." },

	{ "run",
        (PyCFunction)Scheduler_run,
        METH_NOARGS,
        "Run scheduler to end of run queue." },

	{ "run_n_tasklets",
        (PyCFunction)Scheduler_run_n_tasklets,
        METH_VARARGS,
        "Run scheduler limited to n number of Tasklets. \n\n\
            :param int: Number of Tasklets to run before exiting" },

	{ "set_schedule_callback",
        (PyCFunction)Scheduler_set_schedule_callback,
        METH_VARARGS,
        "Specify callable to be called on every schedule operation. \n\n\
            :param callable: Callable method to be called, Passing NULL removes handler \n\
            :return: previous schedule callback or None if none previously set \n\
            :rtype: Callable" },

	{ "get_schedule_callback",
        (PyCFunction)Scheduler_get_schedule_callback,
        METH_NOARGS,
        "Get current schedule callback. \n\n\
            :return: Callable schedule callback method or None if none set \n\
            :rtype: Callable" },

	{ "get_thread_info",
        (PyCFunction)Scheduler_get_thread_info,
        METH_VARARGS,
        "Get thread info. \n\n\
            :return: containing (main tasklet, current tasklet, run count) for the thread \n\
            :rtype: Tuple" },

	{ "switch_trap",
        (PyCFunction)Scheduler_switch_trap,
        METH_VARARGS,
        "Alter swichtrap level. \n\n\
            :param Integer: Value to increase switchtrap level by. \n\
            :return: Previous switchtrap level \n\
            :rtype: Integer" },

	{ "get_schedule_manager",
        (PyCFunction)Scheduler_get_schedule_manager,
        METH_NOARGS,
        "Get the schedule manager from the thread. \n\n\
            :return: Schedule Manager for the thread \n\
            :rtype: ScheduleManagerObject" },

	{ "get_number_of_active_schedule_managers",
        (PyCFunction)Scheduler_get_number_of_active_schedule_managers,
        METH_NOARGS,
        "Get the number of active schedule managers. \n\n\
            :return: Number of schedule managers \n\
            :rtype: Integer \n\
            :note: Value should match active number of threads." },

	{ "get_number_of_active_channels",
        (PyCFunction)Scheduler_get_number_of_active_channels,
        METH_NOARGS,
        "Get the number of active channels. \n\n\
            :return: Number of active channels \n\
            :rtype: Integer" },

	{ "unblock_all_channels",
        (PyCFunction)Scheduler_unblock_all_channels,
        METH_NOARGS,
        "Unblock all active channels." },
	
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
	TaskletExit = PyErr_NewException( exit_exception_string.c_str(), PyExc_SystemExit, nullptr );
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
	ScheduleManager::s_schedule_manager_lock = PyThread_allocate_lock();

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
