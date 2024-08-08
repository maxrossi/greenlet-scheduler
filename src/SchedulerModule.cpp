#define PY_SSIZE_T_CLEAN
#include "stdafx.h"

#define SCHEDULER_MODULE
#include "Scheduler.h"

#include <string>

#include <greenlet.h>

#include "ScheduleManager.h"

//Types
#include "PyTasklet.cpp"
#include "PyChannel.cpp"
#include "PyScheduleManager.cpp"

// End C API
static PyObject*
	SetChannelCallback( PyObject* self, PyObject* args )
{
	PyObject* temp;

	if( PyArg_ParseTuple( args, "O:set_channel_callback", &temp ) )
	{
		if( !PyCallable_Check( temp ) && temp != Py_None )
		{
			PyErr_SetString( PyExc_TypeError, "parameter must be callable or None." );

			return nullptr;
		}

		PyObject* previousCallback = Channel::ChannelCallback();

        if( PyCallable_Check( temp ) )
		{
			Py_IncRef( temp );

			Channel::SetChannelCallback( temp );
		}
        else
        {
			Channel::SetChannelCallback( nullptr );
        }

		if( previousCallback )
		{
			return previousCallback;
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
	GetChannelCallback( PyObject* self, PyObject* args )
{
	PyObject* callable = Channel::ChannelCallback();

	PyObject* ret = nullptr;

	if( callable )
	{

		Py_IncRef( callable );

		ret = callable;
	}
	else
	{
		Py_IncRef( Py_None );

		ret = Py_None;
	}

	return ret;
}

static PyObject*
	EnableSoftSwitch( PyObject* self, PyObject* args )
{
	PyObject* softSwitchValue;

    if( PyArg_ParseTuple( args, "O:set_callable", &softSwitchValue ) )
	{
		if( softSwitchValue != Py_None )
		{
			PyErr_SetString( PyExc_RuntimeError, "enable_soft_switch is only implemented for legacy reasons, the value cannot be changed." );

			return nullptr;
        }
	}

	return Py_False;
}

static PyObject*
	SchedulerGetCurrent( PyObject* self, PyObject* Py_UNUSED( ignored ) )
{
	ScheduleManager* currentScheduler = ScheduleManager::GetThreadScheduleManager();

    Tasklet* currentTasklet = currentScheduler->GetCurrentTasklet();

    currentTasklet->Incref();

    currentScheduler->Decref();

	return currentTasklet->PythonObject();
}

static PyObject*
	SchedulerGetMain( PyObject* self, PyObject* Py_UNUSED( ignored ) )
{
	ScheduleManager* currentScheduler = ScheduleManager::GetThreadScheduleManager();

    Tasklet* mainTasklet = currentScheduler->GetMainTasklet();

    mainTasklet->Incref();

    currentScheduler->Decref();

	return mainTasklet->PythonObject();
}

static PyObject*
	SchedulerGetRunCount( PyObject* self, PyObject* Py_UNUSED( ignored ) )
{
	ScheduleManager* currentScheduler = ScheduleManager::GetThreadScheduleManager();

    PyObject* ret = PyLong_FromLong( currentScheduler->GetCachedTaskletCount() );

    currentScheduler->Decref();

	return ret;
}

static PyObject*
	SchedulerCalculateRunCount( PyObject* self, PyObject* Py_UNUSED( ignored ) )
{
	ScheduleManager* currentScheduler = ScheduleManager::GetThreadScheduleManager();

	PyObject* ret = PyLong_FromLong( currentScheduler->GetCalculatedTaskletCount() );

	currentScheduler->Decref();

	return ret;
}

static PyObject*
	SchedulerSchedule( PyObject* self, PyObject* Py_UNUSED( ignored ) )
{
	ScheduleManager* currentScheduler = ScheduleManager::GetThreadScheduleManager();

    bool scheduleSuccessful = currentScheduler->Schedule();

    currentScheduler->Decref();

	if( scheduleSuccessful )
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
	SchedulerScheduleRemove( PyObject* self, PyObject* Py_UNUSED( ignored ) )
{
	ScheduleManager* currentScheduler = ScheduleManager::GetThreadScheduleManager();

    bool scheduleRemoveSuccessful = currentScheduler->Schedule( true );

    currentScheduler->Decref();

	if( scheduleRemoveSuccessful )
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
	SchedulerRun( PyObject* self, PyObject* Py_UNUSED( ignored ) )
{
	ScheduleManager* currentScheduler = ScheduleManager::GetThreadScheduleManager();

    bool ret = currentScheduler->Run();

    currentScheduler->Decref();

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
	SchedulerRunNTasklets( PyObject* self, PyObject* args )
{
	unsigned int numberOfTasklets = 0;

    if( PyArg_ParseTuple( args, "I:set_channel_callback", &numberOfTasklets ) )
	{
		ScheduleManager* currentScheduler = ScheduleManager::GetThreadScheduleManager();

		bool ret = currentScheduler->RunNTasklets( numberOfTasklets );

        currentScheduler->Decref();

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
	SchedulerSetScheduleCallback( PyObject* self, PyObject* args, PyObject* kwds )
{
    PyObject* temp;

	if( PyArg_ParseTuple( args, "O:set_schedule_callback", &temp ) )
	{
		if( !PyCallable_Check( temp ) && temp != Py_None )
		{
			PyErr_SetString( PyExc_TypeError, "parameter must be callable or None." );
			return nullptr;
		}

        ScheduleManager* currentScheduler = ScheduleManager::GetThreadScheduleManager();

        PyObject* previousCallback = currentScheduler->SchedulerCallback();

        if( PyCallable_Check( temp ) )
		{
			Py_IncRef( temp );

			currentScheduler->SetSchedulerCallback( temp );
		}
        else
        {
			currentScheduler->SetSchedulerCallback( nullptr );
        }

        currentScheduler->Decref();

		if( previousCallback )
		{
			return previousCallback;
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
	SchedulerGetScheduleCallback( PyObject* self, PyObject* Py_UNUSED( ignored ) )
{
	ScheduleManager* currentScheduler = ScheduleManager::GetThreadScheduleManager();

	PyObject* callable = currentScheduler->SchedulerCallback();

	PyObject* ret = nullptr;

	if( callable )
	{

		Py_IncRef( callable );

		ret = callable;
	}
	else
	{
		Py_IncRef( Py_None );

		ret = Py_None;
	}

	currentScheduler->Decref();

	return ret;
    
}

static PyObject*
	SchedulerGetThreadInfo( PyObject* self, PyObject* args, PyObject* kwds )
{
	ScheduleManager* currentScheduler = ScheduleManager::GetThreadScheduleManager();

	PyObject* threadInfoTuple = PyTuple_New( 3 );

    Tasklet* mainTasklet = currentScheduler->GetMainTasklet();

    mainTasklet->Incref();

	PyTuple_SetItem( threadInfoTuple, 0, mainTasklet->PythonObject() );

    Tasklet* currentTasklet = currentScheduler->GetCurrentTasklet();

    currentTasklet->Incref();

	PyTuple_SetItem( threadInfoTuple, 1, currentTasklet->PythonObject() );

	PyTuple_SetItem( threadInfoTuple, 2, PyLong_FromLong( currentScheduler->GetCachedTaskletCount() + 1 ) );

    currentScheduler->Decref();

	return threadInfoTuple;
}

static PyObject*
	SchedulerSwitchTrap( PyObject* self, PyObject* args, PyObject* kwds )
{
	ScheduleManager* currentScheduler = ScheduleManager::GetThreadScheduleManager();

	//TODO: channels need to track this and raise runtime error if appropriet
	int delta;

	if( !PyArg_ParseTuple( args, "i:delta", &delta ) )
	{
		PyErr_SetString( PyExc_RuntimeError, "Scheduler_switch_trap requires a delta argument." ); //TODO
	}

    long originalSwitchTrap = currentScheduler->SwitchTrapLevel();

	currentScheduler->SetSwitchTrapLevel( originalSwitchTrap + delta );

    currentScheduler->Decref();

	return PyLong_FromLong( originalSwitchTrap );
}

static PyObject*
	SchedulerGetScheduleManager( PyObject* self, PyObject* Py_UNUSED( ignored ) )
{
	ScheduleManager* scheduleManager = ScheduleManager::GetThreadScheduleManager();

    if (scheduleManager)
    {
        return scheduleManager->PythonObject();
    }
    else
    {
		Py_IncRef( Py_None );

		return Py_None;
    }
}

static PyObject*
	SchedulerGetNumberOfActiveScheduleManagers( PyObject* self, PyObject* Py_UNUSED( ignored ) )
{
	int numberOfScheduleManagers = ScheduleManager::NumberOfActiveScheduleManagers( );

    return PyLong_FromLong( numberOfScheduleManagers );
}


static PyObject*
	SchedulerGetNumberOfActiveChannels( PyObject* self, PyObject* Py_UNUSED( ignored ) )
{
	long numberOfChannels = Channel::NumberOfActiveChannels();

	return PyLong_FromLong( numberOfChannels );
}

static PyObject*
	SchedulerUnblockAllChannels( PyObject* self, PyObject* Py_UNUSED( ignored ) )
{
	int numberOfChannels = Channel::UnblockAllActiveChannels();

	return PyLong_FromLong( numberOfChannels );
}

void ModuleDestructor( void* )
{
    // Clear callbacks
	Channel::SetChannelCallback( nullptr );

	ScheduleManager::SetSchedulerCallback( nullptr );

    // Destroy thread local storage key
	PyThread_tss_delete( &ScheduleManager::s_threadLocalStorageKey );
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
		PyObject* schedulerTasklet = PyObject_CallObject( reinterpret_cast<PyObject*>( type ), args );

		return reinterpret_cast<PyTaskletObject*>( schedulerTasklet );
	}

    /// @brief Check if Python object is derived from TaskletType
	/// @param obj python object to check
	/// @return 1 if obj is a Tasklet type, otherwise return 0
	static int PyTasklet_Check( PyObject* obj )
	{
		return obj && PyObject_TypeCheck( obj, &TaskletType ) ? 1 : 0;
	}

    /// @brief Make tasklet ready to run by binding parameters to it and inserting into run queue. 
	/// @param tasklet python object type derived from PyTaskletType
	/// @param args tuple containing arguments
    /// @param args tuple containing kwords
	/// @return 0 on success -1 on failure
	static int PyTasklet_Setup( PyTaskletObject* tasklet, PyObject* args, PyObject* kwds )
	{
		if(TaskletSetup( reinterpret_cast<PyObject*>( tasklet ), args, kwds ))
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

		return self->m_implementation->Insert() ? 0 : -1;
	}

    /// @brief Get tasklet blocktrap status
	/// @param tasklet to be checked, python object type derived from PyTaskletType
	/// @return 1 if tasklet cannot be blocked otherwise return 0
	static int PyTasklet_GetBlockTrap( PyTaskletObject* self )
	{
		return self->m_implementation->IsBlocktrapped() ? 1 : 0;
	}

    /// @brief Set tasklet blocktrap status
	/// @param tasklet to be checked, python object type derived from PyTaskletType
    /// @param value new blocktrap value
	/// @return 1 if tasklet cannot be blocked otherwise return 0
	static void PyTasklet_SetBlockTrap( PyTaskletObject* task, int value )
	{
		task->m_implementation->SetBlocktrap( value );
	}

    /// @brief Check if tasklet is a main tasklet
	/// @param tasklet to be checked, python object type derived from PyTaskletType
	/// @return 1 if tasklet is a main tasklet, otherwise return 0
	static int PyTasklet_IsMain( PyTaskletObject* tasklet )
	{
		return tasklet->m_implementation->IsMain() ? 1 : 0;
	}

    /// @brief Check if tasklet is alive
	/// @param tasklet to be checked, python object type derived from PyTaskletType
	/// @return 1 if tasklet is alive, otherwise return 0
    static int PyTasklet_Alive( PyTaskletObject* tasklet )
	{
		return tasklet->m_implementation->IsAlive() ? 1 : 0;
	}

    /// @brief Kill a tasklet. Raises TaskletExit on tasklet.
	/// @param tasklet to be killed, python object type derived from PyTaskletType
	/// @return Always returns 0
    /// @todo Change return to reflect the result of the kill command
    static int PyTasklet_Kill( PyTaskletObject* tasklet )
	{
		bool ret = tasklet->m_implementation->Kill();

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
		PyTypeObject* channelType = type;

        if (!channelType)
        {
			channelType = &ChannelType;
        }

		PyObject* schedulerChannel = PyObject_CallObject( reinterpret_cast<PyObject*>( channelType ), nullptr );

		return reinterpret_cast < PyChannelObject*>(schedulerChannel);
	}

    /// @brief Send an argument over a channel
	/// @param self python object type derived from PyChannelType
	/// @param arg python object to send over channel
	/// @return 0 On success, -1 on failure
	static int PyChannel_Send( PyChannelObject* self, PyObject* arg )
	{
		return self->m_implementation->Send( arg ) ? 0 : -1;
	}

    /// @brief Receive an argument from a channel
	/// @param self python object type derived from PyChannelType
	/// @return received PyObject* on success, NULL on failure
	static PyObject* PyChannel_Receive( PyChannelObject* self )
	{
		return self->m_implementation->Receive();
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
			args = value;
        }
        else
        {
			Py_IncRef( Py_None );
			args = Py_None;
        }

		bool ret = self->m_implementation->Send( args, klass );

        Py_DecRef( args );

		return ret ? 0 : -1;
	}

    /// @brief Get first tasklet from channel blocked queue
	/// @param self python object type derived from PyChannelType
	/// @return first tasklet PyObject or NULL if blocked queue is empty
	static PyObject* PyChannel_GetQueue( PyChannelObject* self )
	{
		return ChannelQueueGet( self, nullptr );
	}

    /// @brief Set the channel's preference
	/// @param self python object type derived from PyChannelType
	static void PyChannel_SetPreference( PyChannelObject* self, int val )
	{
		int sanitisedPreferenceValue = val;

        if (val < -1)
        {
			sanitisedPreferenceValue = -1;
        }
		else if(val > 1)
        {
			sanitisedPreferenceValue = 1;
        }

		self->m_implementation->SetPreferenceFromInt( sanitisedPreferenceValue );
	}

    /// @brief Get the channel's preference
	/// @param self python object type derived from PyChannelType
	/// @return The channel's preference
    static int PyChannel_GetPreference( PyChannelObject* self )
	{
		return self->m_implementation->PreferenceAsInt();
	}

    /// @brief Set the channel's balance
	/// @param self python object type derived from PyChannelType
    /// @return The channel's balance
	static int PyChannel_GetBalance( PyChannelObject* self )
	{
		return self->m_implementation->Balance();
	}

    /// @brief Check if Python object is a main tasklet
	/// @param obj python object to check
	/// @return 1 if obj is a Tasklet type, otherwise return 0
    static int PyChannel_Check( PyObject* obj )
	{
		return obj && PyObject_TypeCheck( obj, &ChannelType ) ? 1 : 0;
	}

    /// @deprecated Please use PyChannel_SendException instead
    /// @brief Throw an exception on first tasklet waiting to receive on channel. This function increments the refcount of exc,val and tb.
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

		PyObject* args = PyTuple_New( 3 );

        Py_XINCREF( exc );
		Py_XINCREF( val );
		Py_XINCREF( tb );

        PyTuple_SetItem( args, 0, exc );

        if (val)
        {
			PyTuple_SetItem( args, 1, val );
        }
        else
        {
			Py_IncRef( Py_None );

			PyTuple_SetItem( args, 1, Py_None );
        }
		
        if (tb)
        {
			PyTuple_SetItem( args, 2, tb );
        }
        else
        {
			Py_IncRef( Py_None );
			
			PyTuple_SetItem( args, 2, Py_None );
        }

		bool ret = self->m_implementation->Send( Py_None, args, true );

        if (!ret)
        {
			Py_DecRef( args );
        }

		return ret ? 0 : -1;
	}

	// Scheduler functions

    /// @brief Get current scheduler
	/// @return Current scheduler as PyObject
    /// @note returns a new reference
	static PyObject* PyScheduler_GetScheduler( )
	{
		return ScheduleManager::GetThreadScheduleManager()->PythonObject();
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
			return SchedulerSchedule( nullptr, nullptr );
        }
		else
		{
			return SchedulerScheduleRemove( nullptr, nullptr );
        }
	}

    /// @brief Get number of tasklets in run queue
	/// @return Number of tasklets in run queue
	static int PyScheduler_GetRunCount()
	{
		ScheduleManager* scheduleManager = ScheduleManager::GetThreadScheduleManager();

        int ret = scheduleManager->GetCachedTaskletCount();

        scheduleManager->Decref();

		return ret;
	}

	/// @brief Returns the current tasklet.
	/// @return PyObject On success returns current tasklet, returns NULL on failure
	/// @note Returns a new reference
	static PyObject* PyScheduler_GetCurrent()
	{
		ScheduleManager* scheduleManager = ScheduleManager::GetThreadScheduleManager();

        Tasklet* currentTasklet = scheduleManager->GetCurrentTasklet();

        currentTasklet->Incref();

        scheduleManager->Decref();

		return currentTasklet->PythonObject();
	}

	/// @brief Run scheduler for specified number of nanoseconds
	/// @param timeout timeout value in nano seconds
	/// @param flags unused, deprecated
	/// @return Py_None on success, NULL on failure
	/// @todo rename and remove deprecated flags parameter
	static PyObject* PyScheduler_RunWatchdogEx( long long timeout, int flags )
	{
		ScheduleManager* scheduleManager = ScheduleManager::GetThreadScheduleManager();

		bool ret = scheduleManager->RunTaskletsForTime( timeout );
        
        scheduleManager->Decref();

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
		ScheduleManager* scheduleManager = ScheduleManager::GetThreadScheduleManager();

		bool ret = scheduleManager->RunNTasklets( number_of_tasklets_to_run );

        scheduleManager->Decref();

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

        Channel::SetChannelCallback( callable );

		return 0;
	}

    /// @brief Get callable set to be called on every channel send and receive
	/// @return Callable, Py_None if no callback is set
	static PyObject* PyScheduler_GetChannelCallback()
	{
		PyObject* channelCallback = Channel::ChannelCallback();

        return channelCallback;
        
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

		ScheduleManager* currentScheduler = ScheduleManager::GetThreadScheduleManager();

		currentScheduler->SetSchedulerCallback( callable );

        currentScheduler->Decref();

		return 0;
	}

    /// @brief Specify c++ function to be called on every schedule operation
	/// @param func c++ function
	static void PyScheduler_SetScheduleFastCallback( schedule_hook_func func )
	{
		ScheduleManager* currentScheduler = ScheduleManager::GetThreadScheduleManager();

		currentScheduler->SetSchedulerFastCallback( func );

        currentScheduler->Decref();
	}

} // extern C

static PyMethodDef SchedulerMethods[] = {

	{ "set_channel_callback",
        SetChannelCallback, 
        METH_VARARGS, 
        "Every send or receive action will result in callable being called. \n\n\
            :param callable: Callback to call on channel send/receive \n\
            :type callable: callable or None \n\
            :return: Previous channel callback. \n\
            :rtype: int" },

	{ "get_channel_callback",
        GetChannelCallback,
        METH_VARARGS,
        "Get the current global channel callback. \n\n\
            :return: Current channel callback or None. \n\
            :rtype: PyObject*" },

	{ "enable_softswitch",
        EnableSoftSwitch,
        METH_VARARGS,
        "Legacy API support. \n\n\
            :warning: Deprecated" },

    { "getcurrent",
        (PyCFunction)SchedulerGetCurrent,
        METH_NOARGS,
        "Get the current Tasklet. \n\n\
            :return: Current Tasklet executing Tasklet of this thread \n\
            :rtype: PyObject*" },

	{ "getmain",
        (PyCFunction)SchedulerGetMain,
        METH_NOARGS,
        "Get the main tasklet of this thread. \n\n\
            :return: Main Tasklet of this thread \n\
            :rtype: PyObject*" },

	{ "getruncount",
        (PyCFunction)SchedulerGetRunCount,
        METH_NOARGS,
        "Get number of currently runnable tasklets. \n\n\
            :return: Cached number of runnable tasklets \n\
            :rtype: Int." },

	{ "calculateruncount",
        (PyCFunction)SchedulerCalculateRunCount,
        METH_NOARGS, "Calculate number of currently runnable tasklets. \n\n\
            :return: Calculated number of runnable tasklets \n\
            :rtype: Int." },

	{ "schedule",
        (PyCFunction)SchedulerSchedule,
        METH_NOARGS,
        "Yield execution of current tasklet. Tasklet is added to end of run queue." },

	{ "schedule_remove",
        (PyCFunction)SchedulerScheduleRemove,
        METH_NOARGS,
        "Yield execution of current tasklet. Tasklet is don't add to end of run queue." },

	{ "run",
        (PyCFunction)SchedulerRun,
        METH_NOARGS,
        "Run scheduler to end of run queue." },

	{ "run_n_tasklets",
        (PyCFunction)SchedulerRunNTasklets,
        METH_VARARGS,
        "Run scheduler limited to n number of Tasklets. \n\n\
            :param int: Number of Tasklets to run before exiting" },

	{ "set_schedule_callback",
        (PyCFunction)SchedulerSetScheduleCallback,
        METH_VARARGS,
        "Specify callable to be called on every schedule operation. \n\n\
            :param callable: Callable method to be called, Passing NULL removes handler \n\
            :return: previous schedule callback or None if none previously set \n\
            :rtype: Callable" },

	{ "get_schedule_callback",
        (PyCFunction)SchedulerGetScheduleCallback,
        METH_NOARGS,
        "Get current schedule callback. \n\n\
            :return: Callable schedule callback method or None if none set \n\
            :rtype: Callable" },

	{ "get_thread_info",
        (PyCFunction)SchedulerGetThreadInfo,
        METH_VARARGS,
        "Get thread info. \n\n\
            :return: containing (main tasklet, current tasklet, run count) for the thread \n\
            :rtype: Tuple" },

	{ "switch_trap",
        (PyCFunction)SchedulerSwitchTrap,
        METH_VARARGS,
        "Alter swichtrap level. \n\n\
            :param Integer: Value to increase switchtrap level by. \n\
            :return: Previous switchtrap level \n\
            :rtype: Integer" },

	{ "get_schedule_manager",
        (PyCFunction)SchedulerGetScheduleManager,
        METH_NOARGS,
        "Get the schedule manager from the thread. \n\n\
            :return: Schedule Manager for the thread \n\
            :rtype: ScheduleManagerObject" },

	{ "get_number_of_active_schedule_managers",
        (PyCFunction)SchedulerGetNumberOfActiveScheduleManagers,
        METH_NOARGS,
        "Get the number of active schedule managers. \n\n\
            :return: Number of schedule managers \n\
            :rtype: Integer \n\
            :note: Value should match active number of threads." },

	{ "get_number_of_active_channels",
        (PyCFunction)SchedulerGetNumberOfActiveChannels,
        METH_NOARGS,
        "Get the number of active channels. \n\n\
            :return: Number of active channels \n\
            :rtype: Integer" },

	{ "unblock_all_channels",
        (PyCFunction)SchedulerUnblockAllChannels,
        METH_NOARGS,
        "Unblock all active channels." },
	
	{ nullptr, nullptr, 0, nullptr } /* Sentinel */
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
    nullptr, /* module documentation, may be NULL */
    -1,       /* size of per-interpreter state of the module,
                 or -1 if the module keeps state in global variables. */
    SchedulerMethods,
    nullptr,
    nullptr,
    nullptr,
	ModuleDestructor
};

PyMODINIT_FUNC
	CONCATENATE(PyInit__scheduler, CCP_BUILD_FLAVOR) (void)
{
    PyObject *m;
	static SchedulerCAPI api;
	PyObject* c_api_object;

    // Initialise thread local storage key
	if( PyThread_tss_create( &ScheduleManager::s_threadLocalStorageKey ) )
	{
		return nullptr;
	}

	//Add custom types
    if (PyType_Ready(&TaskletType) < 0)
    {
		return nullptr;
    }

    if (PyType_Ready(&ChannelType) < 0)
    {
		return nullptr;
    }
		
    if (PyType_Ready(&ScheduleManagerType) < 0)
    {
		return nullptr;
    }
		
    m = PyModule_Create( &schedulermodule );
    if (!m)
    {
		return nullptr;
    }
        
	Py_INCREF( &TaskletType );
	if( PyModule_AddObject( m, "tasklet", (PyObject*)&TaskletType ) < 0 )
	{
		Py_DECREF( &TaskletType );
		Py_DECREF( m );
		return nullptr;
	}

	Py_INCREF( &ChannelType );
	if( PyModule_AddObject( m, "channel", (PyObject*)&ChannelType ) < 0 )
	{
		Py_DECREF( &TaskletType );
		Py_DECREF( &ChannelType );
		Py_DECREF( m );
		return nullptr;
	}

    Py_INCREF( &ScheduleManagerType );
	if( PyModule_AddObject( m, "schedule_manager", (PyObject*)&ScheduleManagerType ) < 0 )
	{
		Py_DECREF( &TaskletType );
		Py_DECREF( &ChannelType );
		Py_DECREF( &ScheduleManagerType );
		Py_DECREF( m );
		return nullptr;
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
        return nullptr;
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
		return nullptr;
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
	c_api_object = PyCapsule_New( (void*)&api, "scheduler._C_API", nullptr );

	if( PyModule_AddObject( m, "_C_API", c_api_object ) < 0 )
	{
		Py_DECREF( &TaskletType );
		Py_DECREF( &ChannelType );
		Py_DECREF( &ScheduleManagerType );
		Py_XDECREF( TaskletExit );
		Py_CLEAR( TaskletExit );
		Py_XDECREF( c_api_object );
		Py_DECREF( m );
		return nullptr;
	}

	ScheduleManager::s_scheduleManagerType = &ScheduleManagerType;
	ScheduleManager::s_taskletType = &TaskletType;

    //Setup initial channel callback static
	Channel::SetChannelCallback(nullptr);
    
    return m;
}
