#include "ScheduleManager.h"

#include "Tasklet.h"

#include "PyTasklet.h"

#include "PyScheduleManager.h"

ScheduleManager::ScheduleManager( PyObject* python_object ) :
	m_python_object( python_object ),
	m_thread_id( PyThread_get_thread_ident() ),
	m_scheduler_tasklet( nullptr ), // Created in constructor
	m_current_tasklet( nullptr ),   // Set in constructor
	m_previous_tasklet( nullptr ),  // Set in constructor
	m_switch_trap_level(0),
    m_current_tasklet_changed_callback(nullptr),
    m_scheduler_callback(nullptr),
    m_scheduler_fast_callback(nullptr),
    m_tasklet_limit(-1),
    m_stop_scheduler(false),
	m_number_of_tasklets_in_queue(0)
{
    // Create scheduler tasklet TODO pull out to separate
	create_scheduler_tasklet();

    m_current_tasklet = m_scheduler_tasklet;

	m_previous_tasklet = m_scheduler_tasklet;

}

ScheduleManager::~ScheduleManager()
{
	Py_DecRef( m_scheduler_tasklet->python_object() );

    Py_XDECREF( m_scheduler_callback );

    // Remove from thread schedulers list
	s_schedulers.erase( m_thread_id );
}

void ScheduleManager::create_scheduler_tasklet()
{
	PyObject* tasklet_args = PyTuple_New( 2 );

	PyTuple_SetItem( tasklet_args, 0, Py_None );

	PyTuple_SetItem( tasklet_args, 1, Py_True );

	PyObject* py_scheduler_tasklet = PyObject_CallObject( reinterpret_cast<PyObject*>( s_tasklet_type ), tasklet_args );

	Py_DecRef( tasklet_args );

	m_scheduler_tasklet = reinterpret_cast<PyTaskletObject*>( py_scheduler_tasklet )->m_impl;


	m_scheduler_tasklet->set_to_current_greenlet();

	m_scheduler_tasklet->set_scheduled( true );
}

PyObject* ScheduleManager::python_object()
{
	return m_python_object;
}

void ScheduleManager::incref()
{
	Py_IncRef( m_python_object );
}

void ScheduleManager::decref()
{
	Py_DecRef( m_python_object );
}

int ScheduleManager::num_active_schedule_managers()
{
	return s_schedulers.size();
}

// TODO this and below are very similar and need joinging
// Doesn't create new reference
ScheduleManager* ScheduleManager::find_scheduler( long thread_id )
{
	auto scheduler_find = s_schedulers.find( thread_id );

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
ScheduleManager* ScheduleManager::get_scheduler( long thread_id /* = -1*/ )
{
	long scheduler_thread_id = thread_id;

    // If thread_id is less than 0 then use the current thread id
    if(thread_id < 0)
	{
		scheduler_thread_id = PyThread_get_thread_ident();
    }

    auto scheduler_find = s_schedulers.find( scheduler_thread_id );

	if( scheduler_find == s_schedulers.end() )
	{
        // Create new scheduler for the thread
		PyObject* scheduler_tasklet = PyObject_CallObject( reinterpret_cast<PyObject*>(s_schedule_manager_type), nullptr );

		ScheduleManager* thread_scheduler = reinterpret_cast<PyScheduleManagerObject*>( scheduler_tasklet )->m_impl;
		
        // Store scheduler against thread id
		s_schedulers[scheduler_thread_id] = thread_scheduler;

        return thread_scheduler;
    }
    else
    {
        // Incref and return existing scheduler for the thread
		scheduler_find->second->incref();

		return scheduler_find->second;
    }
	
}

void ScheduleManager::set_current_tasklet( Tasklet* tasklet )
{
	m_current_tasklet = tasklet;

}

Tasklet* ScheduleManager::get_current_tasklet()
{
	return m_current_tasklet;
}

void ScheduleManager::insert_tasklet_at_beginning( Tasklet* tasklet )
{
	Py_IncRef( tasklet->python_object() );

    ScheduleManager* current_scheduler = get_scheduler( tasklet->thread_id() ); 

    tasklet->set_previous( current_scheduler->m_current_tasklet );

    tasklet->set_next( current_scheduler->m_current_tasklet->next() );

    current_scheduler->m_current_tasklet->set_next( tasklet );

    current_scheduler->decref();

    tasklet->set_scheduled( true );

    m_number_of_tasklets_in_queue++;
}

void ScheduleManager::insert_tasklet( Tasklet* tasklet )
{

    ScheduleManager* current_scheduler = get_scheduler( tasklet->thread_id() );

    if( !tasklet->scheduled() )
	{
		Py_IncRef( tasklet->python_object() );
		current_scheduler->m_previous_tasklet->set_next( tasklet );

		tasklet->set_previous( current_scheduler->m_previous_tasklet );

		// Clear out possible old next
		tasklet->set_next( nullptr );

		current_scheduler->m_previous_tasklet = tasklet;

		tasklet->unblock();

		tasklet->set_scheduled( true );

        m_number_of_tasklets_in_queue++;
    }
	else
	{
		tasklet->set_reschedule( true );
	}

    current_scheduler->decref();
}

bool ScheduleManager::remove_tasklet( Tasklet* tasklet )
{
	Tasklet* previous = tasklet->previous();

    Tasklet* next = tasklet->next();

    if (previous == next)
    {
		return false;
    }

    if(previous != nullptr)
	{
		previous->set_next( next );
	}
    
    if(next != nullptr)
	{
		next->set_previous( previous );
    }
    else
    {
		m_previous_tasklet = previous;
    }

    m_number_of_tasklets_in_queue--;

    return true;
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
	return m_number_of_tasklets_in_queue + 1;   // +1 is the main tasklet
}

int ScheduleManager::calculate_tasklet_count()
{
	int count = 0;

	Tasklet* current_tasklet = ScheduleManager::get_main_tasklet();

	while( current_tasklet->next() != nullptr )
	{
		count++;
		current_tasklet = current_tasklet->next();
	}

	return count + 1; // +1 is the main tasklet
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

PyObject* ScheduleManager::run_tasklets_for_time( long long timeout )
{

	m_total_tasklet_run_time_limit = timeout;

	PyObject* ret = run();

	m_stop_scheduler = false;

	m_tasklet_limit = -1;

    m_total_tasklet_run_time_limit = -1;

    m_start_time = std::chrono::steady_clock::now();

	return ret;
}

PyObject* ScheduleManager::run_n_tasklets( int number_of_tasklets )
{
    m_tasklet_limit = number_of_tasklets;

    PyObject* ret = run();

    m_stop_scheduler = false;

    m_tasklet_limit = -1;

    return ret;
}

PyObject* ScheduleManager::run( Tasklet* start_tasklet /* = nullptr */ )
{
    Tasklet* base_tasklet = nullptr;

    Tasklet* end_tasklet = nullptr;

	if( start_tasklet )
	{
		base_tasklet = start_tasklet->previous();

        end_tasklet = m_previous_tasklet;
    }
	else
	{
		base_tasklet = get_current_tasklet();
    }

    bool run_complete = false;

    bool run_until_unblocked = false;

    if (get_current_tasklet() == get_main_tasklet() && get_current_tasklet()->is_blocked())
    {
		run_until_unblocked = true;
    }

    while( ( base_tasklet->next() != nullptr ) && ( !run_complete ) )
	{

        if( m_stop_scheduler )
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

		Tasklet* current_tasklet = base_tasklet->next();
		bool valid_next_tasklet_clobbered_by_reschedule = false;

        run_scheduler_callback( current_tasklet->previous(), current_tasklet );

        // Store the parent to the tasklet
		// Required for nested scheduling calls

        bool currentTaskletParentBlocked = false;
        if (current_tasklet->get_tasklet_parent())
        {
			currentTaskletParentBlocked = current_tasklet->get_tasklet_parent()->is_blocked();
        }

        if (current_tasklet->set_parent(ScheduleManager::get_current_tasklet()) == -1)
        {
			return nullptr;
        }
		
        // If set to true then tasklet will be decreffed at the end of the loop
        bool cleanup_current_tasklet = false;


        // If switch returns no error or if the error raised is a tasklet exception raised error
		if( current_tasklet->switch_to() || current_tasklet->tasklet_exception_raised() )
		{
			//Clear possible tasklet exception to capture
			current_tasklet->clear_tasklet_exception();

            if (run_until_unblocked && !get_main_tasklet()->is_blocked())
            {
				run_complete = true;
            }

			// Update current tasklet
			ScheduleManager::set_current_tasklet( current_tasklet->get_tasklet_parent() );
            

			//If this is the last tasklet then update previous_tasklet to keep it at the end of the chain
			if( current_tasklet->next() == nullptr )
			{
				m_previous_tasklet = current_tasklet->previous();
			}

			// If running with a tasklet limit then if there are no tasklets left and
			// then don't move the scheduler forward to keep the stacks required to recreate the scheduler state
			if( !m_stop_scheduler )
			{
				// Remove tasklet from queue
                if (remove_tasklet(current_tasklet))
                {
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

                // Test Tasklet Run Limit
				if( m_tasklet_limit > -1 )
				{
					if( m_tasklet_limit > 0 )
					{
						m_tasklet_limit--;
					}
					if( m_tasklet_limit == 0 )
					{
						m_stop_scheduler = true;
					}
				}

                // Test Total tasklet Run Limit
                if (m_total_tasklet_run_time_limit > 0)
                {
					std::chrono::steady_clock::time_point current_time = std::chrono::steady_clock::now();

                    if( std::chrono::duration_cast<std::chrono::nanoseconds>( current_time - m_start_time ).count() >= m_total_tasklet_run_time_limit )
					{
						m_stop_scheduler = true;
					}
                }
               
			}
        }
		else
		{
		    // If exception state should lead to removal of tasklet
            if( current_tasklet->requires_removal() )
			{
				if( remove_tasklet( current_tasklet ) )
				{
					current_tasklet->decref();
				}
			}
            
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
	return m_scheduler_tasklet;
}

void ScheduleManager::set_scheduler_fast_callback( schedule_hook_func* func )
{
	m_scheduler_fast_callback = func;
}

void ScheduleManager::set_scheduler_callback( PyObject* callback )
{
    //TODO so far this is specific to the thread it was called on
    //Check what stackless does, docs are not clear on this
    //Looks global but imo makes more logical sense as local to thread

    Py_XDECREF( m_scheduler_callback );

    Py_IncRef( callback );

    m_scheduler_callback = callback;
}

void ScheduleManager::run_scheduler_callback( Tasklet* prev, Tasklet* next )
{
    // Run Callback through python
	if(m_scheduler_callback)
	{
		PyObject* args = PyTuple_New( 2 ); // TODO don't create this each time

        PyObject* py_prev = Py_None;

        if (prev)
        {
			py_prev = prev->python_object();
        }

        PyObject* py_next = Py_None;

        if (next)
        {
			py_next = next->python_object();
        }

        Py_IncRef( py_prev );

		Py_IncRef( py_next );

        PyTuple_SetItem( args, 0, py_prev );

        PyTuple_SetItem( args, 1, py_next );

		PyObject_Call( m_scheduler_callback, args, nullptr );

        Py_DecRef( args );
    }

    // Run fast callback bypassing python
    if (m_scheduler_fast_callback)
    {
		m_scheduler_fast_callback( reinterpret_cast<PyTaskletObject*>(prev->python_object()), reinterpret_cast<PyTaskletObject*>(next->python_object()) );
    }
}

bool ScheduleManager::is_switch_trapped()
{
    return m_switch_trap_level != 0;
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