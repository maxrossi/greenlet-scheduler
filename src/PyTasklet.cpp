#include "PyTasklet.h"

#include "PyScheduler.h"

void PyTaskletObject::set_to_current_greenlet()
{
	// Import Greenlet C-API
	PyGreenlet_Import();

	if( _PyGreenlet_API == NULL )
	{
		PySys_WriteStdout( "Failed to import greenlet capsule\n" );
		PyErr_Print();
	}

	Py_XDECREF( m_greenlet );

    m_greenlet = PyGreenlet_GetCurrent();
}

bool PyTaskletObject::insert()
{

    Py_XDECREF( m_greenlet );

    m_greenlet = PyGreenlet_New( m_callable, nullptr );
  
    Scheduler::insert_tasklet( this );

	return false;

}

PyObject* PyTaskletObject::switch_to( )
{
	if( _PyGreenlet_API == NULL )   //TODO remove
	{
		PySys_WriteStdout( "Failed to import greenlet capsule\n" );
		PyErr_Print();
	}

    PyObject* ret = Py_None;

	if( m_arguments )
	{
		if( !PyTuple_Check( m_arguments ) )
		{
			PyErr_SetString( PyExc_RuntimeError, "Arguments must be a tuple" );
			return nullptr;
		}

	}

    if( PyThread_get_thread_ident() != m_thread_id)
	{

		Scheduler::insert_tasklet( this );

        Scheduler::schedule();

    }
	else
	{
        // Tasklet is on the same thread so can be switched to now
		Scheduler::set_current_tasklet( this );

		m_next = Py_None;

        m_scheduled = false;    // TODO is a running tasklet scheduled in stackless?

		ret = PyGreenlet_Switch( m_greenlet, m_arguments, nullptr );

        if( !m_blocked && !m_transfer_in_progress ) //TODO this is a bit odd
		{
			m_alive = false;
		}
	
    }

	return ret;
}

PyObject* PyTaskletObject::run()
{
	if(!m_alive)
	{
		PyErr_SetString( PyExc_RuntimeError, "Cannot run tasklet that is not alive" );

		return nullptr;
    }

    if( m_blocked )
	{
		PyErr_SetString( PyExc_RuntimeError, "Cannot run tasklet that is blocked" );

		return nullptr;
	}

	// Run scheduler starting from this tasklet (If it is already in the scheduled)
	if(m_scheduled)
	{
		Scheduler::run( this );
    }
	else
	{
	    //TODO - Needs remove implemented in order to be able to implement this
    }

    return Py_None;
}

void PyTaskletObject::kill()
{
	// Remove reference from previous tasklet
	if(m_previous != Py_None)
	{
		reinterpret_cast<PyTaskletObject*>( m_previous )->m_next = Py_None;
    }

    if(m_blocked)
	{
	    // TODO - Remove from channel


        m_channel_blocked_on = Py_None;
    }

    // Unblock the rest of the chain
	if( m_next != Py_None )
	{
		reinterpret_cast<PyTaskletObject*>( m_next )->run();
	}

    m_alive = false;

	m_scheduled = false;

    m_blocked = false;

    // End reference
	Py_DECREF( this );
}

PyObject* PyTaskletObject::get_transfer_arguments()
{
    //Ownership is relinquished
	PyObject* ret = m_transfer_arguments;

	m_transfer_arguments = nullptr;

	return ret;
}

void PyTaskletObject::set_transfer_arguments( PyObject* args )
{
    //This needs to block until transfer arguments have definately been consumed so it doesn't clobber if used across threads
    //Want to keep the lock functionality inside channels, this requires thought.
	if(m_transfer_arguments != nullptr)
	{
        //TODO need to find a command to force switch thread context imediately
		PySys_WriteStdout( "TODO %d\n", PyThread_get_thread_ident() );
    }

	Py_IncRef( args );

	m_transfer_arguments = args;
}