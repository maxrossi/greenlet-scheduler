#include "ScheduleManager.h"

#include "Tasklet.h"
#include "PyTasklet.h"
#include "PyScheduleManager.h"

ScheduleManager::ScheduleManager( PyObject* pythonObject ) :
	PythonCppType( pythonObject ),
	m_threadId( PyThread_get_thread_ident() ),
	m_schedulerTasklet( nullptr ), // Created in constructor
	m_currentTasklet( nullptr ),   // Set in constructor
	m_previousTasklet( nullptr ),  // Set in constructor
	m_switchTrapLevel(0),
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
	m_schedulerTasklet->Decref();

    Py_XDECREF( m_schedulerCallback );

    PyThread_tss_set( &s_threadLocalStorageKey, nullptr );

    s_numberOfActiveScheduleManagers--;
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

int ScheduleManager::NumberOfActiveScheduleManagers()
{
	return s_numberOfActiveScheduleManagers;
}

// Returns a new schedule manager reference
ScheduleManager* ScheduleManager::GetThreadScheduleManager()
{
    ScheduleManager* scheduleManager = reinterpret_cast<ScheduleManager*>( PyThread_tss_get( &s_threadLocalStorageKey ) );

    if( !scheduleManager )
	{
		// Create new scheduler for the thread
		PyObject* pyScheduleManager = PyObject_CallObject( reinterpret_cast<PyObject*>( s_scheduleManagerType ), nullptr );

		scheduleManager = reinterpret_cast<PyScheduleManagerObject*>( pyScheduleManager )->m_implementation;

        scheduleManager->m_schedulerTasklet->SetScheduleManager( scheduleManager );

        PyThread_tss_set( &s_threadLocalStorageKey, reinterpret_cast<void*>(scheduleManager) );

        s_numberOfActiveScheduleManagers++;
	}
    else
    {
		scheduleManager->Incref();
    }

    return scheduleManager;
}

void ScheduleManager::SetCurrentTasklet( Tasklet* tasklet )
{
    if (m_currentTasklet != tasklet)
    {
		RunSchedulerCallback( m_currentTasklet, tasklet );

		m_currentTasklet = tasklet;
    }
}

Tasklet* ScheduleManager::GetCurrentTasklet()
{
	return m_currentTasklet;
}

//TODO naming correct here?
void ScheduleManager::InsertTaskletAtBeginning( Tasklet* tasklet )
{
	tasklet->Incref();

    ScheduleManager* taskletScheduleManager = tasklet->GetScheduleManager();

    tasklet->SetPrevious( taskletScheduleManager->m_currentTasklet );

    tasklet->SetNext( taskletScheduleManager->m_currentTasklet->Next() );

    taskletScheduleManager->m_currentTasklet->SetNext( tasklet );

    tasklet->SetScheduled( true );

    m_numberOfTaskletsInQueue++;
}

void ScheduleManager::InsertTasklet( Tasklet* tasklet )
{
	ScheduleManager* taskletScheduleManager = tasklet->GetScheduleManager();

    if( !tasklet->IsScheduled() )
	{
		tasklet->Incref();

		taskletScheduleManager->m_previousTasklet->SetNext( tasklet );

		tasklet->SetPrevious( taskletScheduleManager->m_previousTasklet );

		// Clear out possible old next
		tasklet->SetNext( nullptr );

		taskletScheduleManager->m_previousTasklet = tasklet;

		tasklet->Unblock();	// TODO should probably not be here and replaced with error path

		tasklet->SetScheduled( true );

        m_numberOfTaskletsInQueue++;
    }
	else
	{
		tasklet->SetReschedule( true );
	}
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

	tasklet->SetNext( nullptr );
	tasklet->SetPrevious( nullptr );

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

	if( ScheduleManager::GetMainTasklet() == yieldingTasklet )
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

        // guard against re-entry to this tasklet, if it is still blocked
        while (yieldingTasklet->IsBlocked())
		{
			auto parentTasklet = yieldingTasklet->GetParent();
            while (parentTasklet->IsBlocked() && !parentTasklet->IsMain())
            {
				parentTasklet = parentTasklet->GetParent();
            }

			if( !parent_tasklet->SwitchTo() )
			{
				return false;
			}
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
                // Currently only checking this for main tasklets as the teardown buildup
                // Is not working correctly when used with nested tasklets with channels
                // TODO reinstate nested tasklet with channel support.
                if (GetCurrentTasklet()->IsMain())
                {
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

	m_schedulerCallback = callback;
}

void ScheduleManager::RunSchedulerCallback( Tasklet* previous, Tasklet* next )
{
    // Run Callback through python
	if(m_schedulerCallback)
	{
		PyObject* args = PyTuple_New( 2 );

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

int ScheduleManager::SwitchTrapLevel()
{
	return m_switchTrapLevel;
}

void ScheduleManager::SetSwitchTrapLevel( int level )
{
	m_switchTrapLevel = level;
}