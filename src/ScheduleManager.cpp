#include "ScheduleManager.h"

#include "Tasklet.h"
#include "PyTasklet.h"
#include "PyScheduleManager.h"

ScheduleManager::ScheduleManager( PyObject* pythonObject ) :
	m_pythonObject( pythonObject ),
	m_threadId( PyThread_get_thread_ident() ),
	m_schedulerTasklet( nullptr ), // Created in constructor
	m_currentTasklet( nullptr ),   // Set in constructor
	m_previousTasklet( nullptr ),  // Set in constructor
	m_switchTrapLevel(0),
    m_currentTaskletChangedCallback(nullptr),
    m_schedulerCallback(nullptr),
    m_schedulerFastCallback(nullptr),
    m_taskletLimit(-1),
	m_totalTaskletRunTimeLimit(-1),
    m_stopScheduler(false),
	m_numberOfTaskletsInQueue(0)
{
    // Create scheduler tasklet TODO pull out to separate
	CreateSchedulerTasklet();

    m_currentTasklet = m_schedulerTasklet;

	m_previousTasklet = m_schedulerTasklet;

}

ScheduleManager::~ScheduleManager()
{
	Py_DecRef( m_schedulerTasklet->PythonObject() );

    Py_XDECREF( m_schedulerCallback );

    // Remove from thread schedulers list
	s_schedulers.erase( m_threadId );
}

void ScheduleManager::CreateSchedulerTasklet()
{
	PyObject* taskletArgs = PyTuple_New( 2 );

	Py_IncRef( Py_None );

	PyTuple_SetItem( taskletArgs, 0, Py_None );

	PyTuple_SetItem( taskletArgs, 1, Py_True );

	PyObject* pySchedulerTasklet = PyObject_CallObject( reinterpret_cast<PyObject*>( s_taskletType ), taskletArgs );

	Py_DecRef( taskletArgs );

	m_schedulerTasklet = reinterpret_cast<PyTaskletObject*>( pySchedulerTasklet )->m_implementation;


	m_schedulerTasklet->SetToCurrentGreenlet();

	m_schedulerTasklet->SetScheduled( true );
}

PyObject* ScheduleManager::PythonObject()
{
	return m_pythonObject;
}

void ScheduleManager::Incref()
{
	Py_IncRef( m_pythonObject );
}

void ScheduleManager::Decref()
{
	Py_DecRef( m_pythonObject );
}

int ScheduleManager::NumberOfActiveScheduleManagers()
{
	return s_schedulers.size();
}

// TODO this and below are very similar and need joinging
// Doesn't create new reference
ScheduleManager* ScheduleManager::FindScheduler( long threadId )
{
	auto scheduler_find = s_schedulers.find( threadId );

    if( scheduler_find == s_schedulers.end() )
	{
		return nullptr;
	}
	else
	{
		// Return existing scheduler for the thread
		return scheduler_find->second;
	}
}

// Returns a new schedule manager reference
ScheduleManager* ScheduleManager::GetScheduler( long threadId /* = -1*/ )
{
	PyThread_acquire_lock( s_scheduleManagerLock, 1 );

	long schedulerThreadId = threadId;

    // If thread_id is less than 0 then use the current thread id
    if(threadId < 0)
	{
		schedulerThreadId = PyThread_get_thread_ident();
    }

    auto schedulerFind = s_schedulers.find( schedulerThreadId );

    ScheduleManager* ret = nullptr;

	if( schedulerFind == s_schedulers.end() )
	{
        // Create new scheduler for the thread
		PyObject* scheduleManager = PyObject_CallObject( reinterpret_cast<PyObject*>(s_scheduleManagerType), nullptr );

		ScheduleManager* threadScheduler = reinterpret_cast<PyScheduleManagerObject*>( scheduleManager )->m_implementation;
		
        // Store scheduler against thread id
		s_schedulers[schedulerThreadId] = threadScheduler;

        ret = threadScheduler;
    }
    else
    {
        // Incref and return existing scheduler for the thread
		schedulerFind->second->Incref();

		ret = schedulerFind->second;
    }

    PyThread_release_lock( s_scheduleManagerLock );

    return ret;
	
}

void ScheduleManager::SetCurrentTasklet( Tasklet* tasklet )
{
	m_currentTasklet = tasklet;

}

Tasklet* ScheduleManager::GetCurrentTasklet()
{
	return m_currentTasklet;
}

//TODO naming correct here?
void ScheduleManager::InsertTaskletAtBeginning( Tasklet* tasklet )
{
	Py_IncRef( tasklet->PythonObject() );

    ScheduleManager* currentScheduler = GetScheduler( tasklet->ThreadId() ); 

    tasklet->SetPrevious( currentScheduler->m_currentTasklet );

    tasklet->SetNext( currentScheduler->m_currentTasklet->Next() );

    currentScheduler->m_currentTasklet->SetNext( tasklet );

    currentScheduler->Decref();

    tasklet->SetScheduled( true );

    m_numberOfTaskletsInQueue++;
}

void ScheduleManager::InsertTasklet( Tasklet* tasklet )
{

    ScheduleManager* currentScheduler = GetScheduler( tasklet->ThreadId() );

    if( !tasklet->IsScheduled() )
	{
		Py_IncRef( tasklet->PythonObject() );
		currentScheduler->m_previousTasklet->SetNext( tasklet );

		tasklet->SetPrevious( currentScheduler->m_previousTasklet );

		// Clear out possible old next
		tasklet->SetNext( nullptr );

		currentScheduler->m_previousTasklet = tasklet;

		tasklet->Unblock();	// TODO should probably not be here and replaced with error path

		tasklet->SetScheduled( true );

        m_numberOfTaskletsInQueue++;
    }
	else
	{
		tasklet->SetReschedule( true );
	}

    currentScheduler->Decref();
}

bool ScheduleManager::RemoveTasklet( Tasklet* tasklet )
{
	Tasklet* previous = tasklet->Previous();

    Tasklet* next = tasklet->Next();

    if (previous == next)
    {
		return false;
    }

    if(previous != nullptr)
	{
		previous->SetNext( next );
	}
    
    if(next != nullptr)
	{
		next->SetPrevious( previous );
    }
    else
    {
		m_previousTasklet = previous;
    }

    m_numberOfTaskletsInQueue--;

    return true;
}

bool ScheduleManager::Schedule( bool remove /* = false */ )
{
    // Add Current to the end of chain of runnable tasklets    
	Tasklet* currentTasklet = ScheduleManager::GetCurrentTasklet();


    if(remove)
	{
		// Set tag for removal flag, this flag will ensure tasklet remains alive after removal
		currentTasklet->SetTaggedForRemoval( true );
    }
	else
	{
		// Set reschedule flag to inform scheduler that this tasklet must be re-inserted
		currentTasklet->SetReschedule( true );
    }
    

    return Yield();

}

int ScheduleManager::GetCachedTaskletCount()
{
	return m_numberOfTaskletsInQueue + 1;   // +1 is the main tasklet
}

int ScheduleManager::GetCalculatedTaskletCount()
{
	int count = 0;

	Tasklet* currentTasklet = ScheduleManager::GetMainTasklet();

	while( currentTasklet->Next() != nullptr )
	{
		count++;
		currentTasklet = currentTasklet->Next();
	}

	return count + 1; // +1 is the main tasklet
}

// Returns true if tasklet is in a clean state when resumed
// Returns false if exception has been raised on tasklet
bool ScheduleManager::Yield()
{
	Tasklet* yieldingTasklet = ScheduleManager::GetCurrentTasklet();

	if( ScheduleManager::GetMainTasklet() == ScheduleManager::GetCurrentTasklet() )
	{

		if( yieldingTasklet->IsBlocked() && yieldingTasklet->Next() == nullptr )
		{
			PyErr_SetString(PyExc_RuntimeError, "Deadlock: the last runnable tasklet cannot be blocked.");

			return false;
		}
		else if( yieldingTasklet->IsBlocked() )
        {
			bool success = ScheduleManager::Run();

            // if the run set an exception in python, we should fail due to that error now
            if( !success )
            {
				return false;
            }

            // if the main tasklet is still blocked, then this is a deadlock
			if( yieldingTasklet->IsBlocked() )
            {
				PyErr_SetString( PyExc_RuntimeError, "Deadlock: the last runnable tasklet cannot be blocked." );

				return false;
            }

            return success;
        }
        
		return ScheduleManager::Run();
	}
	else
	{
        //Switch to the parent tasklet - support for nested run and schedule calls
		Tasklet* parent_tasklet = yieldingTasklet->GetParent();

        if (!parent_tasklet->SwitchTo())
		{
			return false;
		}
	}

	return true;
}

bool ScheduleManager::RunTaskletsForTime( long long timeout )
{

	m_totalTaskletRunTimeLimit = timeout;

	bool ret = Run();

	m_stopScheduler = false;

	m_taskletLimit = -1;

    m_totalTaskletRunTimeLimit = -1;

    m_startTime = std::chrono::steady_clock::now();

	return ret;
}

bool ScheduleManager::RunNTasklets( int n )
{
    m_taskletLimit = n;

    bool ret = Run();

    m_stopScheduler = false;

    m_taskletLimit = -1;

    return ret;
}

bool ScheduleManager::Run( Tasklet* startTasklet /* = nullptr */ )
{
    Tasklet* baseTasklet = nullptr;

    Tasklet* endTasklet = nullptr;

	if( startTasklet )
	{
		baseTasklet = startTasklet->Previous();

        endTasklet = m_previousTasklet;
    }
	else
	{
		baseTasklet = GetCurrentTasklet();
    }

    bool runComplete = false;

    bool runUntilUnblocked = false;

    if (GetCurrentTasklet() == GetMainTasklet() && GetCurrentTasklet()->IsBlocked())
    {
		runUntilUnblocked = true;
    }

    while( ( baseTasklet->Next() != nullptr ) && ( !runComplete ) )
	{

        if( m_stopScheduler )
		{
			// Switch back to parent now
			Tasklet* activeTasklet = ScheduleManager::GetCurrentTasklet();

			if( activeTasklet == ScheduleManager::GetMainTasklet() )
			{
				break;
			}
			else
			{
				Tasklet* callParent = activeTasklet->GetParent();
				if( callParent->SwitchTo() )
				{
					// Update current tasklet
					ScheduleManager::SetCurrentTasklet( activeTasklet );
				}
				else
				{
					//TODO handle error
				}
			}
		}

		Tasklet* currentTasklet = baseTasklet->Next();

        RunSchedulerCallback( currentTasklet->Previous(), currentTasklet );

        // Store the parent to the tasklet
		// Required for nested scheduling calls

        bool currentTaskletParentBlocked = false;
        if (currentTasklet->GetParent())
        {
			currentTaskletParentBlocked = currentTasklet->GetParent()->IsBlocked();
        }

        if (currentTasklet->SetParent(ScheduleManager::GetCurrentTasklet()) == -1)
        {
			return false;
        }
		
        // If set to true then tasklet will be decreffed at the end of the loop
        bool cleanupCurrentTasklet = false;

        // If switch returns no error or if the error raised is a tasklet exception raised error
		if( currentTasklet->SwitchTo() || currentTasklet->TaskletExceptionRaised() )
		{
			//Clear possible tasklet exception to capture
			currentTasklet->ClearTaskletException();

            if (runUntilUnblocked && !GetMainTasklet()->IsBlocked())
            {
				runComplete = true;
            }

			// Update current tasklet
			ScheduleManager::SetCurrentTasklet( currentTasklet->GetParent() );
            

			//If this is the last tasklet then update previous_tasklet to keep it at the end of the chain
			if( currentTasklet->Next() == nullptr )
			{
				m_previousTasklet = currentTasklet->Previous();
			}

			// If running with a tasklet limit then if there are no tasklets left and
			// then don't move the scheduler forward to keep the stacks required to recreate the scheduler state
			if( !m_stopScheduler )
			{
				// Remove tasklet from queue
                if (RemoveTasklet(currentTasklet))
                {
					cleanupCurrentTasklet = true;
                }

                currentTasklet->SetScheduled( false );

				//Will this get skipped if it happens to be when it will schedule
				if( currentTasklet->RequiresReschedule() )
				{
					InsertTasklet( currentTasklet );
					currentTasklet->SetReschedule( false );
				}

                // Test Tasklet Run Limit
				if( m_taskletLimit > -1 )
				{
					if( m_taskletLimit > 0 )
					{
						m_taskletLimit--;
					}
					if( m_taskletLimit == 0 )
					{
						m_stopScheduler = true;
					}
				}

                // Test Total tasklet Run Limit
                if (m_totalTaskletRunTimeLimit > 0)
                {
					std::chrono::steady_clock::time_point current_time = std::chrono::steady_clock::now();

                    if( std::chrono::duration_cast<std::chrono::nanoseconds>( current_time - m_startTime ).count() >= m_totalTaskletRunTimeLimit )
					{
						m_stopScheduler = true;
					}
                }
               
			}
        }
		else
		{
		    // If exception state should lead to removal of tasklet
            if( currentTasklet->RequiresRemoval() )
			{
				// Update current tasklet
				ScheduleManager::SetCurrentTasklet( currentTasklet->GetParent() );

				if( RemoveTasklet( currentTasklet ) )
				{
					currentTasklet->SetParent( nullptr );

					currentTasklet->Decref();
				}
			}
            
            // Switch was unsuccessful
			currentTasklet->ClearParent();

			return false;
        }

        // Tasklets created during this run are not run in this loop
		if( currentTasklet == endTasklet )
		{
			runComplete = true;
		}

        // Same needs to happen in fail case
        if (!currentTasklet->IsAlive())
        {
			currentTasklet->SetParent( nullptr );
        }

        if( cleanupCurrentTasklet )
		{
			currentTasklet->Decref();
        }
		
	}

	return true;
}

Tasklet* ScheduleManager::GetMainTasklet()
{
	return m_schedulerTasklet;
}

void ScheduleManager::SetSchedulerFastCallback( schedule_hook_func* func )
{
	m_schedulerFastCallback = func;
}

void ScheduleManager::SetSchedulerCallback( PyObject* callback )
{
    //TODO so far this is specific to the thread it was called on
    //Check what stackless does, docs are not clear on this
    //Looks global but imo makes more logical sense as local to thread

    Py_XDECREF( m_schedulerCallback );

    Py_IncRef( callback );

    m_schedulerCallback = callback;
}

void ScheduleManager::RunSchedulerCallback( Tasklet* previous, Tasklet* next )
{
    // Run Callback through python
	if(m_schedulerCallback)
	{
		PyObject* args = PyTuple_New( 2 ); // TODO don't create this each time

        PyObject* pyPrevious = Py_None;

        if (previous)
        {
			pyPrevious = previous->PythonObject();
        }

        PyObject* pyNext = Py_None;

        if (next)
        {
			pyNext = next->PythonObject();
        }

        Py_IncRef( pyPrevious );

		Py_IncRef( pyNext );

        PyTuple_SetItem( args, 0, pyPrevious );

        PyTuple_SetItem( args, 1, pyNext );

		PyObject_Call( m_schedulerCallback, args, nullptr );

        Py_DecRef( args );
    }

    // Run fast callback bypassing python
    if (m_schedulerFastCallback)
    {
		m_schedulerFastCallback( reinterpret_cast<PyTaskletObject*>(previous->PythonObject()), reinterpret_cast<PyTaskletObject*>(next->PythonObject()) );
    }
}

bool ScheduleManager::IsSwitchTrapped()
{
    return m_switchTrapLevel != 0;
}

PyObject* ScheduleManager::SchedulerCallback()
{
	return m_schedulerCallback;
}

void ScheduleManager::SetCurrentTaskletChangedCallback( PyObject* callback )
{
    //TODO xdecref?
	m_currentTaskletChangedCallback = callback;
}

int ScheduleManager::SwitchTrapLevel()
{
	return m_switchTrapLevel;
}

void ScheduleManager::SetSwitchTrapLevel( int level )
{
	m_switchTrapLevel = level;
}