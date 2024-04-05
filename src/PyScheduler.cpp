#include "PyScheduler.h"

#include "PyTasklet.h"


Scheduler::Scheduler()
{
    // Create a tasklet for the scheduler
	m_scheduler_tasklet = reinterpret_cast<PyTaskletObject*>(PyObject_CallObject( s_create_scheduler_tasklet_callable, nullptr ));  //TODO never released, use initialisers

	m_current_tasklet = m_scheduler_tasklet;

	m_previous_tasklet = m_scheduler_tasklet;

    m_thread_id = PyThread_get_thread_ident(); //TODO not really needed, just used for inspection

	m_scheduler_tasklet->set_to_current_greenlet();

    m_scheduler_tasklet->set_scheduled( true );

    m_scheduler_callback = Py_None;

    m_switch_trap_level = 0;
}

Scheduler::~Scheduler()
{
	Py_DECREF( m_scheduler_tasklet );

    Py_XDECREF( m_scheduler_callback );
}

Scheduler* Scheduler::get_scheduler( long thread_id /* = -1*/ )
{
	long scheduler_thread_id = thread_id;

    // If thread_id is less than 0 then use the current thread id
    if(thread_id < 0)
	{
		scheduler_thread_id = PyThread_get_thread_ident();
    }

    if( s_schedulers.find( scheduler_thread_id ) == s_schedulers.end() )
	{
		s_schedulers[scheduler_thread_id] = new Scheduler(); //TODO not yet ever cleaned up
    }
	
    return s_schedulers[scheduler_thread_id];   //TODO double lookup done, address this
}

void Scheduler::set_current_tasklet( PyTaskletObject* tasklet )
{
	Scheduler* current_scheduler = get_scheduler();

	current_scheduler->m_current_tasklet = tasklet;

    /*
    * TODO
    * Need to update scheduler.current with this value
    * So far I don't know a nice way to get a reference
    * To this module (not the _scheduler module)
    */
}

PyObject* Scheduler::get_current_tasklet()
{
	Scheduler* current_scheduler = get_scheduler();

	return reinterpret_cast<PyObject*>( current_scheduler->m_current_tasklet );
}

void Scheduler::insert_tasklet_at_beginning( PyTaskletObject* tasklet )
{
	Py_INCREF( tasklet );

    Scheduler* current_scheduler = get_scheduler( tasklet->thread_id() );

    tasklet->set_previous( reinterpret_cast<PyObject*>(current_scheduler->m_current_tasklet) );

    tasklet->set_next( current_scheduler->m_current_tasklet->next() );

    current_scheduler->m_current_tasklet->set_next( reinterpret_cast<PyObject*>(tasklet) );

    tasklet->set_scheduled( true );
}

void Scheduler::insert_tasklet( PyTaskletObject* tasklet )
{
    Py_INCREF( tasklet );

    Scheduler* current_scheduler = get_scheduler( tasklet->thread_id() );

    if( current_scheduler->m_previous_tasklet != tasklet )
	{
		current_scheduler->m_previous_tasklet->set_next( reinterpret_cast<PyObject*>( tasklet ) );

		tasklet->set_previous( reinterpret_cast<PyObject*>( current_scheduler->m_previous_tasklet ) );

		// Clear out possible old next
        tasklet->set_next( Py_None );   

		current_scheduler->m_previous_tasklet = tasklet;

		tasklet->unblock();

		tasklet->set_scheduled( true );
    }
}

void Scheduler::remove_tasklet( PyTaskletObject* tasklet )
{
	PyObject* previous = tasklet->previous();

    PyObject* next = tasklet->next();

    if(previous != Py_None)
	{
		reinterpret_cast<PyTaskletObject*>(previous)->set_next( next );
	}
    
    if(next != Py_None)
	{
		reinterpret_cast<PyTaskletObject*>( next )->set_previous( previous );
    }
}

bool Scheduler::insert_and_schedule()
{
    // Add Current to the end of chain of runnable tasklets    
	PyObject* current_tasklet = Scheduler::get_current_tasklet();

    // Even if it is the main tasklet
	Scheduler::insert_tasklet( reinterpret_cast<PyTaskletObject*>(current_tasklet) ); // - TODO deal with mem leak caused by this
    
    return schedule();

}

int Scheduler::get_tasklet_count()
{
    // TODO could be cached

	int count = 0;

    PyTaskletObject* current_tasklet = reinterpret_cast<PyTaskletObject*>( Scheduler::get_main_tasklet() );

	while( current_tasklet->next() != Py_None)
	{
		count++;
		current_tasklet = reinterpret_cast<PyTaskletObject*>(reinterpret_cast<PyTaskletObject*>( current_tasklet )->next());
    }

	return count + 1;   // +1 is the main tasklet
}

// Returns true if tasklet is in a clean state when resumed
// Returns false if exception has been raised on tasklet
bool Scheduler::schedule()
{
	if (Scheduler::get_main_tasklet() == Scheduler::get_current_tasklet())
	{
		auto current = reinterpret_cast<PyTaskletObject*>(Scheduler::get_current_tasklet());
		if (current->is_blocked() && current->next() == Py_None)
		{
			PyErr_SetString(PyExc_RuntimeError, "Deadlock: the last runnable tasklet cannot be blocked.");
			return false;
		}
		return Scheduler::run();
	}
	else
	{
        //Switch to the parent tasklet - support for nested run and schedule calls
		PyTaskletObject* current_tasklet = reinterpret_cast<PyTaskletObject*>( Scheduler::get_current_tasklet() );

		PyTaskletObject* parent_tasklet = reinterpret_cast<PyTaskletObject*>( current_tasklet->get_tasklet_parent() );

        if (!parent_tasklet->switch_to())
		{
			return false;
		}
	}

	return true;
}

PyObject* Scheduler::run( PyTaskletObject* start_tasklet /* = nullptr */ )
{
	Scheduler* current_scheduler = get_scheduler();

    PyTaskletObject* base_tasklet = nullptr;

    PyTaskletObject* end_tasklet = nullptr;

	if( start_tasklet )
	{
		base_tasklet = reinterpret_cast<PyTaskletObject*>(start_tasklet->previous());

        end_tasklet = current_scheduler->m_previous_tasklet;
    }
	else
	{
		base_tasklet = reinterpret_cast<PyTaskletObject*>( Scheduler::get_main_tasklet() ); 
    }

    bool run_complete = false;

    while( ( base_tasklet->next() != Py_None ) && ( !run_complete ) )
	{
		PyTaskletObject* current_tasklet = reinterpret_cast<PyTaskletObject*>( base_tasklet->next() );

        current_scheduler->run_scheduler_callback( current_tasklet->previous(), current_tasklet->next() );

        if( !is_switch_trapped() )
		{
            // Store the parent to the tasklet
            // Required for nested scheduling calls
		    reinterpret_cast<PyTaskletObject*>( current_tasklet )->set_parent( Scheduler::get_current_tasklet() );
        
			//If this is the last tasklet then update previous_tasklet to keep it at the end of the chain
			if( current_tasklet->next() == Py_None )
			{
				current_scheduler->m_previous_tasklet = reinterpret_cast<PyTaskletObject*>( current_tasklet->previous() );
			}

			// Remove tasklet from queue
			PyObject* previous_store = current_tasklet->previous();
			reinterpret_cast<PyTaskletObject*>( current_tasklet->previous() )->set_next( current_tasklet->next() );
			reinterpret_cast<PyTaskletObject*>( current_tasklet->next() )->set_previous( previous_store );
		}

        // If switch returns no error or if the error raised is a tasklet exception raised error
		if( current_tasklet->switch_to() || current_tasklet->tasklet_exception_raised() )
		{
            //Clear possible tasklet exception to capture
			current_tasklet->clear_tasklet_exception();

            // Update current tasklet
			Scheduler::set_current_tasklet( reinterpret_cast<PyTaskletObject*>( current_tasklet->get_tasklet_parent() ) );
        }
		else
		{
            // Switch was unsuccessful
			current_tasklet->clear_parent();

			return nullptr;
        }

        // Tasklets created during this run are not run in this loop
		if( current_tasklet == end_tasklet )
		{
			run_complete = true;
		}

        if( !current_tasklet->alive() )
		{
			Py_XDECREF( current_tasklet );
        }

	}

    Py_IncRef( Py_None );

	return Py_None;
}

PyObject* Scheduler::get_main_tasklet()
{
	Scheduler* current_scheduler = get_scheduler();

	return reinterpret_cast<PyObject*>( current_scheduler->m_scheduler_tasklet );
}

void Scheduler::set_scheduler_callback(PyObject* callback)
{
    //TODO so far this is specific to the thread it was called on
    //Check what stackless does, docs are not clear on this
    //Looks global but imo makes more logical sense as local to thread

	Scheduler* current_scheduler = get_scheduler();

    Py_IncRef( callback );

    current_scheduler->m_scheduler_callback = callback;
}

void Scheduler::run_scheduler_callback(PyObject* prev, PyObject* next)
{
	if(m_scheduler_callback != Py_None)
	{
		PyObject* args = PyTuple_New( 2 ); // TODO don't create this each time

        Py_IncRef( prev );

		Py_IncRef( next );

        PyTuple_SetItem( args, 0, prev );

        PyTuple_SetItem( args, 1, next );

		PyObject_Call( m_scheduler_callback, args, nullptr );

        Py_DecRef( args );
    }
}

bool Scheduler::is_switch_trapped()
{
	Scheduler* current_scheduler = get_scheduler();

    return current_scheduler->m_switch_trap_level != 0;
}