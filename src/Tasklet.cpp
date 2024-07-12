#include "Tasklet.h"

#include "ScheduleManager.h"
#include "Channel.h"

Tasklet::Tasklet( PyObject* pythonObject, PyObject* taskletExitException, bool isMain ) :
	m_pythonObject( pythonObject ),
	m_greenlet( nullptr ),
	m_callable( nullptr ),
	m_arguments( nullptr ),
	m_kwArguments( nullptr ),
	m_isMain( isMain ),
	m_transferInProgress( false ),
	m_scheduled( false ),
	m_alive( isMain ),
	m_blocktrap( false ),
	m_previous( nullptr ),
	m_next( nullptr ),
	m_threadId( PyThread_get_thread_ident() ),
	m_transferArguments( nullptr ),
	m_transferException( nullptr ),
	m_channelBlockedOn( nullptr ),
	m_blocked( false ),
	m_exceptionState( Py_None ),
	m_exceptionArguments( Py_None ),
	m_taskletExitException( taskletExitException ),
	m_paused( false ),
	m_taskletParent( nullptr ),
	m_firstRun( true ),
	m_reschedule( false ),
	m_taggedForRemoval( false ),
	m_previousBlocked( nullptr ),
	m_nextBlocked( nullptr ),
	m_scheduleManager( nullptr ),
	m_remove( false ),
	m_killPending( false )
{

    // If tasklet is not a scheduler tasklet then register the tasklet with the scheduler
    // This will create a scheduler if required and while the tasklet is alive 
    // it will hold an incref to it
	if( !m_isMain )
	{
		m_scheduleManager = ScheduleManager::GetThreadScheduleManager( ); // This will return a new reference which is then decreffed in destructor
	}
}

Tasklet::~Tasklet()
{
	if( !m_isMain )
	{
		m_scheduleManager->Decref(); // Decref tasklets usage
	}

    // Clearing parent releases a strong reference to it held by the child
    // This is usually set to null by schedule manager but if it was last on a channel
    // then it needs clearing at this stage.
    // TODO when channel switching is done using the scheduler queue this can change.
    SetParent( nullptr );

	Py_XDECREF( m_callable );

	Py_XDECREF( m_arguments );

    Py_XDECREF( m_kwArguments );

	Py_XDECREF( m_greenlet );

	Py_XDECREF( m_transferArguments );

}

void Tasklet::SetNextBlocked(Tasklet* tasklet)
{
	m_nextBlocked = tasklet;
}

Tasklet* Tasklet::NextBlocked() const
{
	return m_nextBlocked;
}

void Tasklet::SetPreviousBlocked(Tasklet* tasklet)
{
	m_previousBlocked = tasklet;
}

Tasklet* Tasklet::PreviousBlocked() const
{
	return m_previousBlocked;
}

PyObject* Tasklet::PythonObject()
{
	return m_pythonObject;
}

void Tasklet::Incref()
{
	Py_IncRef( m_pythonObject );
}

void Tasklet::Decref()
{
	Py_DecRef( m_pythonObject );
}

int Tasklet::ReferenceCount()
{
	return m_pythonObject->ob_refcnt;
}

void Tasklet::SetKwArguments( PyObject* kwarguments )
{
	Py_XDECREF( m_kwArguments );
	m_kwArguments = kwarguments;
}

PyObject* Tasklet::KwArguments() const
{
	return m_kwArguments;
}

void Tasklet::SetToCurrentGreenlet()
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

bool Tasklet::Remove()
{
	if(m_scheduled)
	{
		m_scheduleManager->RemoveTasklet( this );

		m_paused = true;

        return true;
    }
	else
	{
		return true;
    }
}

bool Tasklet::Initialise()
{
	Py_XDECREF( m_greenlet );

	m_greenlet = PyGreenlet_New( m_callable, nullptr );

    m_paused = true;

    return true;    //TODO handle failure
}

void Tasklet::Uninitialise()
{
	Py_XDECREF( m_greenlet );
    
    m_greenlet = nullptr;
}

bool Tasklet::Insert()
{
    if ( m_blocked )
    {
		PyErr_SetString( PyExc_RuntimeError, "Failed to insert tasklet: Cannot insert blocked tasklet" );

		return false;
    }

    if ( !m_alive )
    {
		PyErr_SetString( PyExc_RuntimeError, "Failed to insert tasklet: Cannot insert dead tasklet" );

		return false;
    }

    m_scheduleManager->InsertTasklet( this );

	m_paused = false;

    return true;
 
}

bool Tasklet::SwitchImplementation()
{
	// Remove the calling tasklet
	if( !m_alive )
	{
		PyErr_SetString( PyExc_RuntimeError, "You cannot run an unbound(dead) tasklet" );

		return false;
	}

	if( m_blocked )
	{
		PyErr_SetString( PyExc_RuntimeError, "Cannot switch to a tasklet that is blocked" );

		return false;
	}

	// Run scheduler starting from this tasklet (If it is already in the scheduled)
	if( m_scheduled )
	{
        // Pause the parent tasklet
		m_scheduleManager->GetCurrentTasklet()->m_paused = true;

		if( m_scheduleManager->Run( this ) )
		{
			// Yeild the tasklets parent as to not continue execution of the rest of this tasklet
            if ( !m_scheduleManager->Yield() )
            {
				return false;
            }

			return true;
        }
		else
		{
			m_scheduleManager->GetCurrentTasklet()->m_paused = false;

			return false;
        } 

	}
	else
	{
		m_scheduleManager->GetCurrentTasklet()->m_paused = true;

        m_scheduleManager->InsertTasklet( this );

        if (!m_scheduleManager->Run(this))
        {
			m_scheduleManager->GetCurrentTasklet()->m_paused = false;

			return false;
        }

        m_scheduleManager->GetCurrentTasklet()->m_paused = false;

	}

	return true;


}

bool Tasklet::SwitchTo( )
{
	
    bool ret = true;

    bool argsSuppled = false;
	bool kwArgsSupplied = false;

	if( m_arguments )
	{
		if( !PyTuple_Check( m_arguments ) )
		{
			PyErr_SetString( PyExc_RuntimeError, "Arguments must be a tuple" );

			return false;
		}

        argsSuppled = true;

	}

    if( m_kwArguments )
	{
		if( !PyDict_Check( m_kwArguments ) )
		{
			PyErr_SetString( PyExc_RuntimeError, "kwargs must be a dict" );

			return false;
		}

		kwArgsSupplied = true;
	}

    ScheduleManager* scheduleManager = ScheduleManager::GetThreadScheduleManager();

    auto main_tasklet = scheduleManager->GetMainTasklet();
	// Check required arguments have been supplied if this is the first time the tasklet
	// Has been switch to
	if( m_firstRun && (main_tasklet != this) && !( kwArgsSupplied || argsSuppled ) )
	{
		PyErr_SetString( PyExc_RuntimeError, "No arguments supplied to tasklet" );

        scheduleManager->Decref();

		return false;
	}

	if( scheduleManager != m_scheduleManager)
	{
        // Tasklet being switched to is on a different thread than the current scheduleManager
        scheduleManager->InsertTasklet( this );

        if ( !scheduleManager->Yield() )
        {
			scheduleManager->Decref();

            return false;
        }
    }
	else
	{

		if( scheduleManager->IsSwitchTrapped() )
		{
			PyErr_SetString( PyExc_RuntimeError, "Cannot schedule when scheduler switch_trap level is non-zero" );

            scheduleManager->Decref();

			return false;
        }


        // If tasklet has never been run exceptions are treated differently
        if(( m_firstRun ) && (m_exceptionState != Py_None))
		{
			// If tasklet exit has been raised then don't run tasklet and keep silent
			if( m_exceptionState == m_taskletExitException )
			{
				m_alive = false;

                scheduleManager->Decref();

				return true;
            }
			else
			{
				SetPythonExceptionStateFromTaskletExceptionState();

                scheduleManager->Decref();

                // Inform scheduler to remove this tasklet
                m_remove = true;

                return false;
            }
		
        }

        // Tasklet is on the same thread so can be switched to now
		scheduleManager->SetCurrentTasklet( this );

        m_paused = false;

        PyObject* args = nullptr;
		PyObject* kwargs = nullptr;

        if (m_firstRun)
        {
			args = Arguments();
			kwargs = KwArguments();
        }

        m_firstRun = false;

		ret = PyGreenlet_Switch( m_greenlet, args, kwargs );

        // Clear arguments
		SetArguments( nullptr );
		SetKwArguments( nullptr );
        
        // Check exception state of current tasklet
        // It is important to understand that the current tasklet may not be the same value as this object
        // This object will be the value of the tasklet that the other tasklet last switched to, commonly
        // This will be the scheduler. So when the tasklet is resumed it will resume from that context
        // We want to check the exception of the current tasklet so we need to get this value and check that

		Tasklet* currentTasklet = scheduleManager->GetCurrentTasklet();

		if( currentTasklet->m_exceptionState != Py_None )
		{

            currentTasklet->SetPythonExceptionStateFromTaskletExceptionState();

            scheduleManager->Decref();

            return false;
  
        }

        // Check state of tasklet
        if( !m_blocked && !m_transferInProgress && !m_isMain && !m_paused && !m_reschedule && !m_taggedForRemoval ) 
		{
			m_alive = false;
		}

		// Removed tasklet is paused
        if (m_taggedForRemoval)
        {
			m_paused = true;
        }

		// Reset tagging used to preserve alive status after removal
        m_taggedForRemoval = false;

        
		if( !ret )
		{
			// Inform scheduler to remove this tasklet
			m_remove = true;
		}

    }

    scheduleManager->Decref();

	return ret;
}

void Tasklet::ClearException()
{
	if( m_exceptionState != Py_None)
	{
		Py_DecRef( m_exceptionState );

        m_exceptionState = Py_None;
    }

    if( m_exceptionArguments != Py_None )
	{
		Py_DecRef( m_exceptionArguments );

		m_exceptionArguments = Py_None;
    }
}

void Tasklet::SetExceptionState( PyObject* exception, PyObject* arguments /* = Py_None */)
{
	ClearException();

    Py_IncRef( exception );
    m_exceptionState = exception;

    Py_IncRef( arguments );
	m_exceptionArguments = arguments;
}

// Assumes valid exception state
// Exception state validity is sanitised in PyTasklet_python.cpp
void Tasklet::SetPythonExceptionStateFromTaskletExceptionState()
{
	//If it is an instance of an exception
	if( PyObject_IsInstance( m_exceptionState, PyExc_Exception ) )
	{
        // PyErr_SetRaisedException steals reference to exception state
        // increffed to compensate so clear_exception will still work
		Py_IncRef( m_exceptionState ); 
		PyErr_SetRaisedException( m_exceptionState );
	}
	else
	{
		PyErr_SetObject( m_exceptionState, m_exceptionArguments );
	}	

	ClearException();
}

bool Tasklet::Run()
{
	if(!m_alive)
	{
		PyErr_SetString( PyExc_RuntimeError, "Cannot run tasklet that is not alive (dead)" );

		return false;
    }

    if( m_blocked )
	{
		PyErr_SetString( PyExc_RuntimeError, "Cannot run tasklet that is blocked" );

		return false;
	}

	// Run scheduler starting from this tasklet (If it is already in the scheduled)
	if(m_scheduled)
	{
		bool ret = m_scheduleManager->Run( this );

		return ret;
    }
	else
	{
		Tasklet* current_tasklet = m_scheduleManager->GetCurrentTasklet();

		if( m_scheduleManager->GetCurrentTasklet() == m_scheduleManager->GetMainTasklet() )
		{
			// Run the scheduler starting at current_tasklet
			m_scheduleManager->InsertTaskletAtBeginning( this );

            bool ret = m_scheduleManager->Run( this );

			return ret;
		}
		else
		{
            m_scheduleManager->InsertTasklet( this );

            bool ret = m_scheduleManager->Run( this );

            return ret;
		}
    }

    return true;
}

bool Tasklet::Kill( bool pending /*=false*/ )
{
    // Quick out if kill is already pending
    if (m_killPending)
    {
		return true;
    }

    //Store so condition can be reinstated on failure
    bool blockedStore = m_blocked;
	Channel* blockChannelStore = m_channelBlockedOn;

    if(m_blocked)
	{
        Unblock();
    }
    
    // Raise TaskletExit error
	SetExceptionState( m_taskletExitException );

    if( m_scheduleManager->GetCurrentTasklet() == this )
	{
        // Continue on this tasklet and raise error immediately
		SetPythonExceptionStateFromTaskletExceptionState();

		return false;
    }
	else
	{
		if( pending )
		{
            m_scheduleManager->InsertTasklet( this );

            m_killPending = true;

            if( blockedStore )
			{
				blockChannelStore->UnblockTaskletFromChannel( this );

				SetBlockedDirection( 0 );
			}

            return true;
		}
		else
		{
			
            // Must be alive
			if(!m_alive)
			{
                // If exception state is tasklet exit then handle silently
				if(m_exceptionState == m_taskletExitException)
				{
					ClearException();

					return true;
				}
				else
				{
					// Invalid code path - Should never enter
					ClearException();

                    PyErr_SetString( PyExc_RuntimeError, "Invalid exception called on dead tasklet." );

                    return false;
                }
			    
            }

	
			bool result = Run();

			if( result )
			{
				return true;
			}
			else
			{
				// Set tasklet back to original blocked state
				if( blockedStore )
				{

					Block( blockChannelStore );
				}

				return false;
			}
			
		}
	}

    return false;
}

PyObject* Tasklet::GetTransferArguments()
{
    //Ownership is relinquished
	PyObject* ret = m_transferArguments;

	return ret;
}

void Tasklet::ClearTransferArguments()
{

	m_transferArguments = nullptr;

}

void Tasklet::SetTransferArguments( PyObject* args, PyObject* exception )
{
    //This should all change with the channel preference change
	if(m_transferArguments != nullptr)
	{
        //TODO this needs to be converted to an assert
		PySys_WriteStdout( "TRANSFER ARGS BROKEN %d\n", PyThread_get_thread_ident() );
    }

	Py_IncRef( args );

	m_transferArguments = args;

    m_transferException = exception;
}

bool Tasklet::IsBlocked() const
{
	return m_blocked;
}

bool Tasklet::IsOnChannelBlockList() const
{
	return m_channelBlockedOn != nullptr || m_nextBlocked != nullptr || m_previousBlocked != nullptr;
}

void Tasklet::Block( Channel* channel )
{
	m_blocked = true;

    m_channelBlockedOn = channel;
}

void Tasklet::Unblock()
{
	m_blocked = false;

	m_channelBlockedOn = nullptr;
}

void Tasklet::SetAlive( bool value )
{
	m_alive = value;
}

bool Tasklet::IsAlive() const
{
	return m_alive;
}

bool Tasklet::IsScheduled() const
{
	return m_scheduled;
}

void Tasklet::SetScheduled( bool value )
{
	m_scheduled = value;
}

bool Tasklet::IsBlocktrapped() const
{
	return m_blocktrap;
}

void Tasklet::SetBlocktrap( bool value )
{
	m_blocktrap = value;
}

bool Tasklet::IsMain() const
{
	return m_isMain;
}

void Tasklet::MarkAsMain( bool value )
{
	m_isMain = value;
}

unsigned long Tasklet::ThreadId() const
{
	return m_threadId;
}

Tasklet* Tasklet::Next() const
{
	return m_next;
}

void Tasklet::SetNext( Tasklet* next )
{
	m_next = next;
}

Tasklet* Tasklet::Previous() const
{
	return m_previous;
}

void Tasklet::SetPrevious( Tasklet* previous )
{
	m_previous = previous;
}

PyObject* Tasklet::Arguments() const
{
	return m_arguments;
}

void Tasklet::SetArguments( PyObject* arguments )
{
	Py_XDECREF( m_arguments );

	m_arguments = arguments;
}

bool Tasklet::TransferInProgress() const
{
	return m_transferInProgress;
}

void Tasklet::SetTransferInProgress( bool value )
{
	m_transferInProgress = value;
}

PyObject* Tasklet::TransferException() const
{
	return m_transferException;
}

bool Tasklet::ThrowException( PyObject* exception, PyObject* value, PyObject* tb, bool pending )
{

    SetExceptionState( exception, value );

    if( m_scheduleManager->GetCurrentTasklet() == this )
	{
		// Continue on this tasklet and raise error immediately
		SetPythonExceptionStateFromTaskletExceptionState();

        return false;
	}
	else
	{
		if( pending )
		{
			if(m_alive)
			{
                m_scheduleManager->InsertTasklet( this );
            }
			else
			{
				// If exception state is tasklet exit then handle silently
				if( m_exceptionState == m_taskletExitException )
				{
					ClearException();

					return true;
				}
				else
				{
					ClearException();

					PyErr_SetString( PyExc_RuntimeError, "You cannot throw to a dead tasklet." );

					return false;
				}
            }
			
		}
		else
		{
			if(m_blocked)
			{
				Channel* block_channel_store = m_channelBlockedOn;
				int blocked_direction_store = m_blockedDirection;

				Unblock();

				if(Run())
				{
					return true;
                }
				else
				{
                    // On failure return to original state
					Block( block_channel_store );

					return false;
                }
			}
			else
			{
                // Must be alive
                if(!m_alive)
				{
					// If exception state is tasklet exit then handle silently
					if( m_exceptionState == m_taskletExitException )
					{
						ClearException();

						return true;
					}
					else
					{
						ClearException();

						PyErr_SetString( PyExc_RuntimeError, "You cannot throw to a dead tasklet" );

						return false;
					}

                }
				else
				{
					bool ret = Run();

					return ret;
                }
            }
			
		}

		return true;
    }

}

bool Tasklet::IsPaused()
{
	return m_paused;
}

Tasklet* Tasklet::GetParent()
{
	return m_taskletParent;
}

int Tasklet::SetParent( Tasklet* parent )
{
	int ret = 0;

	if( parent )
	{
		parent->Incref();

	    ret = PyGreenlet_SetParent( this->m_greenlet, parent->m_greenlet );


	    if( ret == -1 )
	    {
		    return ret;
	    }

    }

    if (m_taskletParent)
    {
		m_taskletParent->Decref();
    }

	m_taskletParent = parent;

    return ret;
}

void Tasklet::ClearParent()
{
	m_taskletParent = nullptr;
}

bool Tasklet::TaskletExceptionRaised()
{
	return PyErr_Occurred() == m_taskletExitException;
}

void Tasklet::ClearTaskletException()
{
	if( PyErr_Occurred() == m_taskletExitException )
	{
		ClearException();

		PyErr_Clear();
	}
}

void Tasklet::SetReschedule( bool value )
{
	m_reschedule = value;
}

bool Tasklet::RequiresReschedule()
{
	return m_reschedule;
}

void Tasklet::SetTaggedForRemoval( bool value )
{
	m_taggedForRemoval = value;
}

void Tasklet::SetCallable(PyObject* callable)
{
	Py_XDECREF( m_callable );

	m_callable = callable;
}

bool Tasklet::RequiresRemoval()
{
	return m_remove;
}

int Tasklet::GetBlockedDirection()
{
	return m_blockedDirection;
}

void Tasklet::SetBlockedDirection(int direction)
{
	m_blockedDirection = direction;
}

void Tasklet::SetScheduleManager( ScheduleManager* scheduleManager )
{
    // This is something only used for main Tasklets
    // The resulting will store only a weak reference
	m_scheduleManager = scheduleManager;
}

ScheduleManager* Tasklet::GetScheduleManager()
{
	return m_scheduleManager;
}