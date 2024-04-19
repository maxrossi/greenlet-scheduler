#include "Tasklet.h"

#include "ScheduleManager.h"

#include "Channel.h"

#include <iostream>

Tasklet::Tasklet( PyObject* python_object, PyObject* callable, PyObject* tasklet_exit_exception ) :
	m_python_object( python_object ),
	m_greenlet( nullptr ),
	m_callable( callable ),
	m_arguments( nullptr ),
	m_kwarguments( nullptr ),
	m_is_main( false ),
	m_transfer_in_progress( false ),
	m_scheduled( false ),
	m_alive( false ),
	m_blocktrap( false ),
	m_previous( nullptr ),
	m_next( nullptr ),
	m_thread_id( PyThread_get_thread_ident() ),
	m_transfer_arguments( nullptr ),
	m_transfer_is_exception( false ),
	m_channel_blocked_on( nullptr ),
	m_blocked(false),
	m_exception_state(Py_None),
	m_exception_arguments( Py_None ),
	m_tasklet_exit_exception(tasklet_exit_exception),
	m_paused(false),
	m_tasklet_parent( nullptr ),
	m_first_run(true),
	m_reschedule(false)
{
	
}

Tasklet::~Tasklet()
{
	Py_XDECREF( m_callable );

	Py_XDECREF( m_arguments );

	Py_XDECREF( m_greenlet );

	Py_XDECREF( m_transfer_arguments );

}

PyObject* Tasklet::python_object()
{
	return m_python_object;
}

void Tasklet::clear_callable()
{
	Py_XDECREF( m_callable );
	Py_XDECREF( m_arguments );
	Py_XDECREF( m_greenlet );
	//m_callable = nullptr;
	//m_arguments = nullptr;
	//m_greenlet = nullptr;
}

void Tasklet::set_kw_arguments( PyObject* kwarguments )
{
	m_kwarguments = kwarguments;
}

PyObject* Tasklet::kw_arguments() const
{
	return m_kwarguments;
}

void Tasklet::set_to_current_greenlet()
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

bool Tasklet::remove()
{
	if(m_scheduled)
	{
		ScheduleManager::remove_tasklet( this );

		m_paused = true;

        return true;
    }
	else
	{
		return true;
    }
}

bool Tasklet::initialise()
{
	Py_XDECREF( m_greenlet );

	m_greenlet = PyGreenlet_New( m_callable, nullptr );

    return true;    //TODO handle failure
}

bool Tasklet::insert()
{
	if(!m_blocked)
	{
		ScheduleManager::insert_tasklet( this );

		m_paused = false;

        return true;
    }

	return false;   
}

PyObject* Tasklet::switch_implementation()
{
	// Remove the calling tasklet
	if( !m_alive )
	{
		PyErr_SetString( PyExc_RuntimeError, "You cannot run an unbound(dead) tasklet" );

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
		ScheduleManager::get_current_tasklet()->m_paused = true;

		if( ScheduleManager::run( this ) )
		{
			// Yeild the tasklets parent as to not continue execution of the rest of this tasklet
			ScheduleManager::yield();

			return Py_None;
        }
		else
		{
			ScheduleManager::get_current_tasklet()->m_paused = false;

			return nullptr;
        } 

	}
	else
	{
		//TODO - Needs remove implemented in order to be able to implement this
	}

	return Py_None;


}

PyObject* Tasklet::switch_to( )
{
	
    PyObject* ret = Py_None;

    bool args_suppled = false;
	bool kwargs_supplied = false;

	if( m_arguments )
	{
		if( !PyTuple_Check( m_arguments ) )
		{
			PyErr_SetString( PyExc_RuntimeError, "Arguments must be a tuple" );
			return nullptr;
		}

        args_suppled = true;

	}

    if( m_kwarguments )
	{
		if( !PyDict_Check( m_kwarguments ) )
		{
			PyErr_SetString( PyExc_RuntimeError, "kwargs must be a dict" );
			return nullptr;
		}

		kwargs_supplied = true;
	}

    auto main_tasklet = ScheduleManager::get_main_tasklet();
	if( main_tasklet != this && !( kwargs_supplied || args_suppled ) )
	{
		PyErr_SetString( PyExc_RuntimeError, "No arguments supplied to tasklet" );
		return nullptr;
	}

    if( PyThread_get_thread_ident() != m_thread_id)
	{

		ScheduleManager::insert_tasklet( this );

        ScheduleManager::yield();

    }
	else
	{
		

		if( ScheduleManager::is_switch_trapped() )
		{
			PyErr_SetString( PyExc_RuntimeError, "Cannot schedule when scheduler switch_trap level is greater than 0" );
			return nullptr;
        }


        // If tasklet has never been run exceptions are treated differently
        if(( m_first_run ) && (m_exception_state != Py_None))
		{
			// If tasklet exit has been raised then don't run tasklet and keep silent
			if( m_exception_state == m_tasklet_exit_exception )
			{
				m_alive = false;

				return Py_None;
            }
			else
			{
				set_python_exception_state_from_tasklet_exception_state();

                return nullptr;
            }
		
        }

        // Tasklet is on the same thread so can be switched to now
		ScheduleManager::set_current_tasklet( this );

        m_paused = false;

        m_first_run = false;

		ret = PyGreenlet_Switch( m_greenlet, m_arguments, m_kwarguments );

       

        // Check exception state of current tasklet
        // It is important to understand that the current tasklet may not be the same value as this object
        // This object will be the value of the tasklet that the other tasklet last switched to, commonly
        // This will be the scheduler. So when the tasklet is resumed it will resume from that context
        // We want to check the exception of the current tasklet so we need to get this value and check that

		Tasklet* current_tasklet =ScheduleManager::get_current_tasklet();

		if( current_tasklet->m_exception_state != Py_None )
		{

            current_tasklet->set_python_exception_state_from_tasklet_exception_state();

            return nullptr;
  
        }

        // Check state of tasklet
        if( !m_blocked && !m_transfer_in_progress && !m_is_main && !m_paused && !m_reschedule && !m_tagged_for_removal ) 
		{
			m_alive = false;
		}

        // Reset tagging used to preserve alive status after removal
        m_tagged_for_removal = false;
	
    }

	return ret;
}

void Tasklet::clear_exception()
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

void Tasklet::set_exception_state( PyObject* exception, PyObject* arguments /* = Py_None */)
{
	clear_exception();

    Py_IncRef( exception );
    m_exception_state = exception;

    Py_IncRef( arguments );
	m_exception_arguments = arguments;
}

// Assumes valid exception state
// Exception state validity is sanitised in PyTasklet_python.cpp
void Tasklet::set_python_exception_state_from_tasklet_exception_state()
{
	//If it is an instance of an exception
	if( PyObject_IsInstance( m_exception_state, PyExc_Exception ) )
	{
        // PyErr_SetRaisedException steals reference to exception state
        // increffed to compensate so clear_exception will still work
		Py_IncRef( m_exception_state ); 
		PyErr_SetRaisedException( m_exception_state );
	}
	else
	{
		PyErr_SetObject( m_exception_state, m_exception_arguments );
	}	

	clear_exception();
}

PyObject* Tasklet::run()
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
		return ScheduleManager::run( this );
    }
	else
	{
		Tasklet* current_tasklet = ScheduleManager::get_current_tasklet();

		if( ScheduleManager::get_current_tasklet() == ScheduleManager::get_main_tasklet() )
		{
			// Run the scheduler starting at current_tasklet
			ScheduleManager::insert_tasklet_at_beginning( this );

			return ScheduleManager::run( this );
		}
		else
		{
			ScheduleManager::insert_tasklet_at_beginning( this );

            ScheduleManager::insert_tasklet_at_beginning( current_tasklet );

			ScheduleManager::yield(); // TODO handle the error case
		}
    }

    return Py_None;
}

bool Tasklet::kill( bool pending /*=false*/ )
{

    //Store so condition can be reinstated on failure
    bool blocked_store = m_blocked;
	Channel* block_channel_store = m_channel_blocked_on;

    if(m_blocked)
	{
        unblock();
    }
    
    // Raise TaskletExit error
	set_exception_state( m_tasklet_exit_exception );

    if( ScheduleManager::get_current_tasklet() == this )
	{
        // Continue on this tasklet and raise error immediately
		set_python_exception_state_from_tasklet_exception_state();

		return false;
    }
	else
	{
		if( pending )
		{
			ScheduleManager::insert_tasklet( this );

            return true;
		}
		else
		{
			
            // Must be alive
			if(!m_alive)
			{
                // If exception state is tasklet exit then handle silently
				if(m_exception_state == m_tasklet_exit_exception)
				{
					clear_exception();

					return true;
				}
				else
				{
					// Invalid code path - Should never enter
					clear_exception();

                    PyErr_SetString( PyExc_RuntimeError, "Invalid exception called on dead tasklet." );

                    return false;
                }
			    
            }
            

			PyObject* result = run();

			if( result == Py_None ) //TODO not keen mixing return types in this class
			{
				return true;
			}
			else
			{
				// Set tasklet back to original blocked state
				if( blocked_store )
				{

					block( block_channel_store );
				}

				return false;
			}
			
		}
	}
}

PyObject* Tasklet::get_transfer_arguments()
{
    //Ownership is relinquished
	PyObject* ret = m_transfer_arguments;

	return ret;
}

void Tasklet::clear_transfer_arguments()
{

	m_transfer_arguments = nullptr;

}

void Tasklet::set_transfer_arguments( PyObject* args, bool is_exception )
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

bool Tasklet::is_blocked() const
{
	return m_blocked;
}

void Tasklet::block( Channel* channel )
{
	m_blocked = true;

    m_channel_blocked_on = channel;
}

void Tasklet::unblock()
{
	m_blocked = false;

	m_channel_blocked_on = nullptr;
}

void Tasklet::set_alive( bool value )
{
	m_alive = value;
}

bool Tasklet::alive() const
{
	return m_alive;
}

bool Tasklet::scheduled() const
{
	return m_scheduled;
}

void Tasklet::set_scheduled( bool value )
{
	m_scheduled = value;
}

bool Tasklet::blocktrap() const
{
	return m_blocktrap;
}

void Tasklet::set_blocktrap( bool value )
{
	m_blocktrap = value;
}

bool Tasklet::is_main() const
{
	return m_is_main;
}

void Tasklet::set_is_main( bool value )
{
	m_is_main = value;
}

unsigned long Tasklet::thread_id() const
{
	return m_thread_id;
}

Tasklet* Tasklet::next() const
{
	return m_next;
}

void Tasklet::set_next( Tasklet* next )
{
	m_next = next;
}

Tasklet* Tasklet::previous() const
{
	return m_previous;
}

void Tasklet::set_previous( Tasklet* previous )
{
	m_previous = previous;
}

PyObject* Tasklet::arguments() const
{
	return m_arguments;
}

void Tasklet::set_arguments( PyObject* arguments )
{
	m_arguments = arguments;
}

bool Tasklet::transfer_in_progress() const
{
	return m_transfer_in_progress;
}

void Tasklet::set_transfer_in_progress( bool value )
{
	m_transfer_in_progress = value;
}

bool Tasklet::transfer_is_exception() const
{
	return m_transfer_is_exception;
}

bool Tasklet::throw_impl( PyObject* exception, PyObject* value, PyObject* tb, bool pending )
{

    set_exception_state( exception, value );

    if( ScheduleManager::get_current_tasklet() == this )
	{
		// Continue on this tasklet and raise error immediately
		set_python_exception_state_from_tasklet_exception_state();

        return false;
	}
	else
	{
		if( pending )
		{
			if(m_alive)
			{
				ScheduleManager::insert_tasklet( this );
            }
			else
			{
				// If exception state is tasklet exit then handle silently
				if( m_exception_state == m_tasklet_exit_exception )
				{
					clear_exception();

					return true;
				}
				else
				{
					clear_exception();

					PyErr_SetString( PyExc_RuntimeError, "You cannot throw to a dead tasklet." );

					return false;
				}
            }
			
		}
		else
		{
			if(m_blocked)
			{
				Channel* block_channel_store = m_channel_blocked_on;

				unblock();

				if(run())
				{
					return true;
                }
				else
				{
                    // On failure return to original state
					block( block_channel_store );

					return false;
                }
			}
			else
			{
                // Must be alive
                if(!m_alive)
				{
					// If exception state is tasklet exit then handle silently
					if( m_exception_state == m_tasklet_exit_exception )
					{
						clear_exception();

						return true;
					}
					else
					{
						clear_exception();

						PyErr_SetString( PyExc_RuntimeError, "You cannot throw to a dead tasklet" );

						return false;
					}

                }
				else if( m_exception_state == m_tasklet_exit_exception )
				{
					return run();
                }
				else
				{
					// If it tasklet is not blocked then raise error immediately
					// See test test_tasklet.TestTaskletThrowImmediate.test_new
					set_python_exception_state_from_tasklet_exception_state();

                    return false;
                }
            }
			
		}

		return true;
    }

}

void Tasklet::raise_exception()
{
    
}

bool Tasklet::is_paused()
{
	return m_paused;
}

Tasklet* Tasklet::get_tasklet_parent()
{
	return m_tasklet_parent;
}

void Tasklet::set_parent( Tasklet* parent )
{
	m_tasklet_parent = parent;
	PyGreenlet_SetParent( this->m_greenlet, parent->m_greenlet );
}

void Tasklet::clear_parent()
{
	m_tasklet_parent = nullptr;
}

bool Tasklet::tasklet_exception_raised()
{
	return PyErr_Occurred() == m_tasklet_exit_exception;
}

void Tasklet::clear_tasklet_exception()
{
	if( PyErr_Occurred() == m_tasklet_exit_exception )
	{
		clear_exception();

		PyErr_Clear();
	}
}

void Tasklet::set_reschedule( bool value )
{
	m_reschedule = value;
}

bool Tasklet::requires_reschedule()
{
	return m_reschedule;
}

void Tasklet::set_tagged_for_removal( bool value )
{
	m_tagged_for_removal = value;
}

void Tasklet::set_callable(PyObject* callable)
{
	m_callable = callable;
}
