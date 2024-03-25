#include "PyTasklet.h"

#include "PyScheduler.h"

#include "PyChannel.h"

PyTaskletObject::PyTaskletObject(PyObject* callable):
	m_greenlet( nullptr ),
	m_callable( callable ),
	m_arguments( nullptr ),
	m_is_main( false ),
	m_transfer_in_progress( true ),//Why is this true?
	m_scheduled( false ),
	m_alive( true ),
	m_blocktrap( false ),
	m_previous( Py_None ),
	m_next( Py_None ),
	m_thread_id( PyThread_get_thread_ident() ),
	m_transfer_arguments( nullptr ),
	m_transfer_is_exception( false ),
	m_channel_blocked_on(Py_None),
	m_blocked(false)
{

}

PyTaskletObject ::~PyTaskletObject()
{
	Py_XDECREF( m_callable );

	Py_XDECREF( m_arguments );

	Py_XDECREF( m_greenlet );

	Py_XDECREF( m_transfer_arguments );
}

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

		if( this !=  reinterpret_cast<PyTaskletObject*>( Scheduler::get_main_tasklet()) )
		{
			m_next = Py_None; // Don't like where this resides so far
		}

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
		reinterpret_cast<PyChannelObject*>( m_channel_blocked_on )->remove_tasklet_from_blocked( reinterpret_cast<PyObject*>(this) );
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

	return ret;
}

void PyTaskletObject::clear_transfer_arguments()
{

	m_transfer_arguments = nullptr;

}

void PyTaskletObject::set_transfer_arguments( PyObject* args, bool is_exception )
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

    m_transfer_is_exception = is_exception;
}

bool PyTaskletObject::is_blocked() const
{
	return m_blocked;
}

void PyTaskletObject::block( PyChannelObject* channel )
{
	m_blocked = true;

    m_channel_blocked_on = reinterpret_cast<PyObject*>(channel);
}

void PyTaskletObject::unblock()
{
	m_blocked = false;

	m_channel_blocked_on = Py_None;
}

bool PyTaskletObject::alive() const
{
	return m_alive;
}

bool PyTaskletObject::scheduled() const
{
	return m_scheduled;
}

void PyTaskletObject::set_scheduled( bool value )
{
	m_scheduled = value;
}

bool PyTaskletObject::blocktrap() const
{
	return m_blocktrap;
}

void PyTaskletObject::set_blocktrap( bool value )
{
	m_blocktrap = value;
}

bool PyTaskletObject::is_main() const
{
	return m_is_main;
}

void PyTaskletObject::set_is_main( bool value )
{
	m_is_main = value;
}

unsigned long PyTaskletObject::thread_id() const
{
	return m_thread_id;
}

PyObject* PyTaskletObject::next() const
{
	return m_next;
}

void PyTaskletObject::set_next( PyObject* next )
{
	m_next = next;
}

PyObject* PyTaskletObject::previous() const
{
	return m_previous;
}

void PyTaskletObject::set_previous( PyObject* previous )
{
	m_previous = previous;
}

PyObject* PyTaskletObject::arguments() const
{
	return m_arguments;
}

void PyTaskletObject::set_arguments( PyObject* arguments )
{
	m_arguments = arguments;
}

bool PyTaskletObject::transfer_in_progress() const
{
	return m_transfer_in_progress;
}

void PyTaskletObject::set_transfer_in_progress( bool value )
{
	m_transfer_in_progress = value;
}

bool PyTaskletObject::transfer_is_exception() const
{
	return m_transfer_is_exception;
}