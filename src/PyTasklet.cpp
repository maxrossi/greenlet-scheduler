#include "PyTasklet.h"

#include "PyScheduler.h"

#include "PyChannel.h"

PyTaskletObject::PyTaskletObject( PyObject* callable, PyObject* tasklet_exit_exception ) :
	m_greenlet( nullptr ),
	m_callable( callable ),
	m_arguments( nullptr ),
	m_is_main( false ),
	m_transfer_in_progress( false ),
	m_scheduled( false ),
	m_alive( true ),
	m_blocktrap( false ),
	m_previous( Py_None ),
	m_next( Py_None ),
	m_thread_id( PyThread_get_thread_ident() ),
	m_transfer_arguments( nullptr ),
	m_transfer_is_exception( false ),
	m_channel_blocked_on(Py_None),
	m_blocked(false),
	m_exception_state(Py_None),
	m_exception_arguments( Py_None ),
	m_tasklet_exit_exception(tasklet_exit_exception),
	m_paused(false),
	m_tasklet_parent( Py_None )
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

bool PyTaskletObject::remove()
{
	if(m_scheduled)
	{
		Scheduler::remove_tasklet( this );

		m_paused = true;

        return true;
    }
	else
	{
		return true;
    }
}

bool PyTaskletObject::initialise()
{
	Py_XDECREF( m_greenlet );

	m_greenlet = PyGreenlet_New( m_callable, nullptr );

    return true;    //TODO handle failure
}

bool PyTaskletObject::insert()
{
	if(!m_blocked)
	{
		Scheduler::insert_tasklet( this );

		m_paused = false;

        return true;
    }

	return false;   
}

PyObject* PyTaskletObject::switch_implementation()
{
	// Remove the calling tasklet
	if( !m_alive )
	{
		PyErr_SetString( PyExc_RuntimeError, "Cannot switch to tasklet that is not alive (dead)" );

		return nullptr;
	}

	if( m_blocked )
	{
		PyErr_SetString( PyExc_RuntimeError, "Cannot switch to a tasklet that is blocked" );

		return nullptr;
	}

	// Run scheduler starting from this tasklet (If it is already in the scheduled)
	if( m_scheduled )
	{
        // Pause the parent tasklet
		reinterpret_cast<PyTaskletObject*>( Scheduler::get_current_tasklet() )->m_paused = true;

		Scheduler::run( this );

        // Schedule the tasklets parent as to not continue execution of the rest of this tasklet
		Scheduler::schedule();

	}
	else
	{
		//TODO - Needs remove implemented in order to be able to implement this
	}

	return Py_None;


}

PyObject* PyTaskletObject::switch_to( )
{
	
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

		if( Scheduler::is_switch_trapped() )
		{
			PyErr_SetString( PyExc_RuntimeError, "Cannot schedule when scheduler switch_trap level is greater than 0" );
			return nullptr;
        }

        // Tasklet is on the same thread so can be switched to now
		Scheduler::set_current_tasklet( this );

        m_scheduled = false;    // TODO is a running tasklet scheduled in stackless?

        m_paused = false;

		ret = PyGreenlet_Switch( m_greenlet, m_arguments, nullptr );

       

        // Check exception state of current tasklet
        // It is important to understand that the current tasklet may not be the same value as this object
        // This object will be the value of the tasklet that the other tasklet last switched to, commonly
        // This will be the scheduler. So when the tasklet is resumed it will resume from that context
        // We want to check the exception of the current tasklet so we need to get this value and check that

		PyTaskletObject* current_tasklet = reinterpret_cast<PyTaskletObject*>( Scheduler::get_current_tasklet() );

		if( current_tasklet->m_exception_state != Py_None )
		{

            current_tasklet->set_exception();

            return nullptr;
  
        }

        // Check state of tasklet
        if( !m_blocked && !m_transfer_in_progress && !m_is_main && !m_paused && !m_scheduled ) 
		{
			m_alive = false;
		}
	
    }

	return ret;
}

void PyTaskletObject::clear_exception()
{
	if( m_exception_state != Py_None)
	{
		Py_DecRef( m_exception_state );

        m_exception_state = Py_None;
    }

    if( m_exception_arguments != Py_None )
	{
		Py_DecRef( m_exception_arguments );

		m_exception_arguments = Py_None;
    }
}

bool PyTaskletObject::exception_state_is_valid()
{
	if( PyObject_IsInstance( m_exception_state, PyExc_Exception ) )
	{
		// If exception state is an implementation then expect no arguments
		return m_exception_arguments == Py_None;    
    }
	else
	{
		return PyExceptionClass_Check( m_exception_state );
    }
}

bool PyTaskletObject::set_exception()
{
    if(m_alive)
	{
	    //If it is an instance of an exception use this
	    if( PyObject_IsInstance( m_exception_state, PyExc_Exception ) )
	    {
		    if( m_exception_arguments == Py_None)
		    {
				Py_IncRef( m_exception_state ); // TODO check but I think below steals this reference

			    PyErr_SetRaisedException( m_exception_state );
            }
		    else
		    {
		        // Exception instance sent but other arguments also provided
			    PyErr_SetString( PyExc_TypeError, "Aditional arguments sent when providing exception instance to throw, did you mean to send an exception class?" );
            }
	    }
	    else
	    {
		    if( exception_state_is_valid() )
			{
				PyErr_SetObject( m_exception_state, m_exception_arguments );

		    }
		    else
		    {
		        // Invalid
			    PyErr_SetString( PyExc_TypeError, "Non-valid exception provided." );
            }
	    }	
	}
	else
	{
		//If it is a TaskletError then don't raise and error
		if (m_exception_state == m_tasklet_exit_exception )
		{
			return false;
		}
        
		PyErr_SetString( PyExc_RuntimeError, "Exception called on dead tasklet." );

    }

	clear_exception();

    return true;
}

PyObject* PyTaskletObject::run()
{
	if(!m_alive)
	{
		PyErr_SetString( PyExc_RuntimeError, "Cannot run tasklet that is not alive (dead)" );

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
		return Scheduler::run( this );
    }
	else
	{
		PyTaskletObject* current_tasklet = reinterpret_cast<PyTaskletObject*>( Scheduler::get_current_tasklet() );

		if( Scheduler::get_current_tasklet() == Scheduler::get_main_tasklet() )
		{
			// Run the scheduler starting at current_tasklet
			Scheduler::insert_tasklet_at_beginning( this );

			Scheduler::run( this );
		}
		else
		{
			Scheduler::insert_tasklet_at_beginning( this );

            Scheduler::insert_tasklet_at_beginning( current_tasklet );

			Scheduler::schedule(); // TODO handle the error case
		}
    }

    return Py_None;
}

bool PyTaskletObject::kill( bool pending /*=false*/ )
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

    // Raise TaskletExit error
	Py_IncRef( m_tasklet_exit_exception );
	m_exception_state = m_tasklet_exit_exception;

    if( reinterpret_cast<PyTaskletObject*>(Scheduler::get_current_tasklet()) == this )
	{
        // Continue on this tasklet and raise error immediately
		return !set_exception();    //Confusing syntax TODO
    }
	else
	{
		if( pending )
		{
			Scheduler::insert_tasklet( this );
		}
		else
		{
			run();
		}

        return true;
	}
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
    //This should all change with the channel preference change
	if(m_transfer_arguments != nullptr)
	{
        //TODO need to find a command to force switch thread context imediately
		PySys_WriteStdout( "TRANSFER ARGS BROKEN %d\n", PyThread_get_thread_ident() );
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

bool PyTaskletObject::throw_impl( PyObject* exception, PyObject* value, PyObject* tb, bool pending )
{
	Py_IncRef( exception );
	Py_IncRef( value );

    m_exception_state = exception;
	m_exception_arguments = value;

    if( reinterpret_cast<PyTaskletObject*>( Scheduler::get_current_tasklet() ) == this )
	{
		// Continue on this tasklet and raise error immediately
        return !set_exception();    //TODO Confusing syntax
	}
	else
	{
		if( pending )
		{
			Scheduler::insert_tasklet( this );
		}
		else
		{
			if(m_blocked)
			{
				m_blocked = false;

				run();
			}
			else
			{
                // If it tasklet is not blocked then raise error immediately
                // See test test_tasklet.TestTaskletThrowImmediate.test_new
				return !set_exception(); //TODO Confusing syntax
            }
			
		}

		return true;
    }

}

void PyTaskletObject::raise_exception()
{
    
}

bool PyTaskletObject::is_paused()
{
	return m_paused;
}

PyObject* PyTaskletObject::get_tasklet_parent()
{
	return m_tasklet_parent;
}

void PyTaskletObject::set_parent( PyObject* parent )
{
	m_tasklet_parent = parent;
}

void PyTaskletObject::clear_parent()
{
	m_tasklet_parent = Py_None;
}

bool PyTaskletObject::tasklet_exception_raised()
{
	return PyErr_Occurred() == m_tasklet_exit_exception;
}

void PyTaskletObject::clear_tasklet_exception()
{
	if( PyErr_Occurred() == m_tasklet_exit_exception )
	{
		PyErr_Clear();
	}
}

