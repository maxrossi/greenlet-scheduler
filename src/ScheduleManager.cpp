#include "ScheduleManager.h"

#include "Tasklet.h"

#include "PyTasklet.h"

ScheduleManager::ScheduleManager()
{
    // Create a tasklet for the scheduler
	m_scheduler_tasklet = reinterpret_cast<PyTaskletObject*>(PyObject_CallObject( s_create_scheduler_tasklet_callable, nullptr ))->m_impl;  //TODO never released, and need to use initialiserlist

	m_current_tasklet = m_scheduler_tasklet;

	m_previous_tasklet = m_scheduler_tasklet;

    m_thread_id = PyThread_get_thread_ident(); //TODO not really needed, just used for inspection

	m_scheduler_tasklet->set_to_current_greenlet();

    m_scheduler_tasklet->set_scheduled( true );

    m_scheduler_callback = Py_None;

    m_switch_trap_level = 0;

    m_tasklet_limit = -1;

    m_stop_scheduler = false;
}

ScheduleManager::~ScheduleManager()
{
	Py_DecRef( m_scheduler_tasklet->python_object() );

    Py_XDECREF( m_scheduler_callback );
}

ScheduleManager* ScheduleManager::get_scheduler( long thread_id /* = -1*/ )
{
	long scheduler_thread_id = thread_id;

    // If thread_id is less than 0 then use the current thread id
    if(thread_id < 0)
	{
		scheduler_thread_id = PyThread_get_thread_ident();
    }

    if( s_schedulers.find( scheduler_thread_id ) == s_schedulers.end() )
	{
		s_schedulers[scheduler_thread_id] = new ScheduleManager(); //TODO not yet ever cleaned up
    }
	
    return s_schedulers[scheduler_thread_id];   //TODO double lookup done, address this
}

void ScheduleManager::set_current_tasklet( Tasklet* tasklet )
{
	ScheduleManager* current_scheduler = get_scheduler();

	current_scheduler->m_current_tasklet = tasklet;

    /*
    * TODO
    * Need to update scheduler.current with this value
    * So far I don't know a nice way to get a reference
    * To this module (not the _scheduler module)
    */
}

Tasklet* ScheduleManager::get_current_tasklet()
{
	ScheduleManager* current_scheduler = get_scheduler();

	return current_scheduler->m_current_tasklet;
}

void ScheduleManager::insert_tasklet_at_beginning( Tasklet* tasklet )
{
	Py_IncRef( tasklet->python_object() );

    ScheduleManager* current_scheduler = get_scheduler( tasklet->thread_id() );

    tasklet->set_previous( current_scheduler->m_current_tasklet );

    tasklet->set_next( current_scheduler->m_current_tasklet->next() );

    current_scheduler->m_current_tasklet->set_next( tasklet );

    tasklet->set_scheduled( true );
}

void ScheduleManager::insert_tasklet( Tasklet* tasklet )
{
	Py_IncRef( tasklet->python_object() );

    ScheduleManager* current_scheduler = get_scheduler( tasklet->thread_id() );

    if( current_scheduler->m_previous_tasklet != tasklet )
	{
		current_scheduler->m_previous_tasklet->set_next( tasklet );

		tasklet->set_previous( current_scheduler->m_previous_tasklet );

		// Clear out possible old next
		tasklet->set_next( nullptr );

		current_scheduler->m_previous_tasklet = tasklet;

		tasklet->unblock();

		tasklet->set_scheduled( true );
    }
	else
	{
		tasklet->set_reschedule( true );
	}
}

void ScheduleManager::remove_tasklet( Tasklet* tasklet )
{
	Tasklet* previous = tasklet->previous();

    Tasklet* next = tasklet->next();

    if(previous != nullptr)
	{
		previous->set_next( next );
	}
    
    if(next != nullptr)
	{
		next->set_previous( previous );
    }
}

bool ScheduleManager::schedule( bool remove /* = false */ )
{
    // Add Current to the end of chain of runnable tasklets    
	Tasklet* current_tasklet = ScheduleManager::get_current_tasklet();


    if(remove)
	{
		// Set tag for removal flag, this flag will ensure tasklet remains alive after removal
		current_tasklet->set_tagged_for_removal( true );
    }
	else
	{
		// Set reschedule flag to inform scheduler that this tasklet must be re-inserted
		current_tasklet->set_reschedule( true );
    }
    

    return yield();

}

int ScheduleManager::get_tasklet_count()
{
    // TODO could be cached

	int count = 0;

    Tasklet* current_tasklet = ScheduleManager::get_main_tasklet();

	while( current_tasklet->next() != nullptr)
	{
		count++;
		current_tasklet = current_tasklet->next();
    }

	return count + 1;   // +1 is the main tasklet
}

// Returns true if tasklet is in a clean state when resumed
// Returns false if exception has been raised on tasklet
bool ScheduleManager::yield()
{
	if( ScheduleManager::get_main_tasklet() == ScheduleManager::get_current_tasklet() )
	{
		auto current = ScheduleManager::get_current_tasklet();

		if (current->is_blocked() && current->next() == nullptr)
		{
			PyErr_SetString(PyExc_RuntimeError, "Deadlock: the last runnable tasklet cannot be blocked.");

			return false;
		}

		return ScheduleManager::run();
	}
	else
	{
        //Switch to the parent tasklet - support for nested run and schedule calls
		Tasklet* current_tasklet = ScheduleManager::get_current_tasklet();

		Tasklet* parent_tasklet = current_tasklet->get_tasklet_parent();

        if (!parent_tasklet->switch_to())
		{
			return false;
		}
	}

	return true;
}


PyObject* ScheduleManager::run_n_tasklets( int number_of_tasklets )
{
	ScheduleManager* current_scheduler = get_scheduler();

    current_scheduler->m_tasklet_limit = number_of_tasklets;

    PyObject* ret = run();

    current_scheduler->m_stop_scheduler = false;

    current_scheduler->m_tasklet_limit = -1;

    return ret;
}

PyObject* ScheduleManager::run( Tasklet* start_tasklet /* = nullptr */ )
{
	ScheduleManager* current_scheduler = get_scheduler();

    Tasklet* base_tasklet = nullptr;

    Tasklet* end_tasklet = nullptr;

	if( start_tasklet )
	{
		base_tasklet = start_tasklet->previous();

        end_tasklet = current_scheduler->m_previous_tasklet;
    }
	else
	{
		base_tasklet = ScheduleManager::get_main_tasklet(); 
    }

    bool run_complete = false;

    while( ( base_tasklet->next() != nullptr ) && ( !run_complete ) )
	{
		Tasklet* current_tasklet = base_tasklet->next();
		bool valid_next_tasklet_clobbered_by_reschedule = false;

        current_scheduler->run_scheduler_callback( current_tasklet->previous(), current_tasklet->next() );

        // Store the parent to the tasklet
		// Required for nested scheduling calls

        bool currentTaskletParentBlocked = false;
        if (current_tasklet->get_tasklet_parent())
        {
			currentTaskletParentBlocked = current_tasklet->get_tasklet_parent()->is_blocked();
        }

		current_tasklet->set_parent( ScheduleManager::get_current_tasklet() );
		
        // If set to true then tasklet will be decreffed at the end of the loop
        bool cleanup_current_tasklet = false;


        // If switch returns no error or if the error raised is a tasklet exception raised error
		if( current_tasklet->switch_to() || current_tasklet->tasklet_exception_raised() )
		{
			//Clear possible tasklet exception to capture
			current_tasklet->clear_tasklet_exception();

			// Update current tasklet
			ScheduleManager::set_current_tasklet( current_tasklet->get_tasklet_parent() );
            

			//If this is the last tasklet then update previous_tasklet to keep it at the end of the chain
			if( current_tasklet->next() == nullptr )
			{
				current_scheduler->m_previous_tasklet = current_tasklet->previous();
			}

			//Decriment run count
			if( current_scheduler->m_tasklet_limit > -1 )
			{
				if( current_scheduler->m_tasklet_limit > 0 )
				{
					current_scheduler->m_tasklet_limit--;
				}
			}

			// If running with a tasklet limit then if there are no tasklets left and
			// then don't move the scheduler forward to keep the stacks required to recreate the scheduler state
			if( !current_scheduler->m_stop_scheduler )
			{
				// Remove tasklet from queue
				Tasklet* previous_store = current_tasklet->previous();

				if( current_tasklet->previous() != current_tasklet->next() )
				{
					current_tasklet->previous()->set_next( current_tasklet->next() );

                    if(current_tasklet->next())
					{
						current_tasklet->next()->set_previous( previous_store );
					}

                    cleanup_current_tasklet = true;
                }

                current_tasklet->set_scheduled( false );

				//Will this get skipped if it happens to be when it will schedule
				if( current_tasklet->requires_reschedule() )
				{
					//Special case, we are here because tasklet scheduled itself
                    if (current_tasklet->next() != nullptr)
                    {
						valid_next_tasklet_clobbered_by_reschedule = true;
                    }
					insert_tasklet( current_tasklet );
					current_tasklet->set_reschedule( false );
				}

				if( current_scheduler->m_tasklet_limit > -1 )
				{
					if( current_scheduler->m_tasklet_limit == 0 )
					{
						current_scheduler->m_stop_scheduler = true;
					}
				}
			}

			if( current_scheduler->m_stop_scheduler )
			{
			    // Switch back to parent now
				Tasklet* active_tasklet = ScheduleManager::get_current_tasklet();

                if( active_tasklet == ScheduleManager::get_main_tasklet() )
                {
                    break;
                }
                else
                {
					Tasklet* call_parent = active_tasklet->get_tasklet_parent();

					if( call_parent->switch_to() )
					{
						// Update current tasklet
						ScheduleManager::set_current_tasklet( active_tasklet );
					}
					else
					{
						//TODO handle error
					}
                }

                
            }

        }
		else
		{
            // Switch was unsuccessful
			current_tasklet->clear_parent();

			return nullptr;
        }

        // Tasklets created during this run are not run in this loop
		if( current_tasklet == end_tasklet || ( current_tasklet->next() == nullptr && end_tasklet == nullptr && !valid_next_tasklet_clobbered_by_reschedule ) )
		{
			run_complete = true;
		}

        if( cleanup_current_tasklet )
		{
			Py_DecRef( current_tasklet->python_object() );
        }
		
	}

    Py_IncRef( Py_None );

	return Py_None;
}

Tasklet* ScheduleManager::get_main_tasklet()
{
	ScheduleManager* current_scheduler = get_scheduler();

	return current_scheduler->m_scheduler_tasklet;
}

void ScheduleManager::set_scheduler_callback( PyObject* callback )
{
    //TODO so far this is specific to the thread it was called on
    //Check what stackless does, docs are not clear on this
    //Looks global but imo makes more logical sense as local to thread

	ScheduleManager* current_scheduler = get_scheduler();

    Py_IncRef( callback );

    current_scheduler->m_scheduler_callback = callback;
}

void ScheduleManager::run_scheduler_callback( Tasklet* prev, Tasklet* next )
{
	if(m_scheduler_callback != Py_None)
	{
		PyObject* args = PyTuple_New( 2 ); // TODO don't create this each time

        PyObject* py_prev = prev->python_object();

        PyObject* py_next = next->python_object();

        Py_IncRef( py_prev );

		Py_IncRef( py_next );

        PyTuple_SetItem( args, 0, py_prev );

        PyTuple_SetItem( args, 1, py_next );

		PyObject_Call( m_scheduler_callback, args, nullptr );

        Py_DecRef( args );
    }
}

bool ScheduleManager::is_switch_trapped()
{
	ScheduleManager* current_scheduler = get_scheduler();

    return current_scheduler->m_switch_trap_level != 0;
}


PyObject* ScheduleManager::scheduler_callback()
{
	return m_scheduler_callback;
}

void ScheduleManager::set_current_tasklet_changed_callback( PyObject* callback )
{
	m_current_tasklet_changed_callback = callback;
}

int ScheduleManager::switch_trap_level()
{
	return m_switch_trap_level;
}

void ScheduleManager::set_switch_trap_level( int level )
{
	m_switch_trap_level = level;
}