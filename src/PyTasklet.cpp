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
  
    PySchedulerObject::insert_tasklet( this );

	return false;

}

PyObject* PyTaskletObject::switch_to( )
{

	if( m_arguments )
	{
		if( !PyTuple_Check( m_arguments ) )
		{
			PyErr_SetString( PyExc_RuntimeError, "Arguments must be a tuple" );
			return nullptr;
		}

	}
	
    PySchedulerObject::set_current_tasklet( this );

    PyObject* ret = PyGreenlet_Switch( m_greenlet, m_arguments, nullptr );

    if( !m_blocked )    //TODO This is unexpected to me
	{
		m_alive = false;
	}

    m_scheduled = false;

    m_next = Py_None;

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
		PySchedulerObject::run( this );
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