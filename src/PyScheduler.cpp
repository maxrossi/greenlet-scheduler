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

    m_scheduler_callback = Py_None;
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

void Scheduler::insert_tasklet( PyTaskletObject* tasklet )
{

    Py_INCREF( tasklet );

    Scheduler* current_scheduler = get_scheduler( tasklet->m_thread_id );

	current_scheduler->m_previous_tasklet->m_next = reinterpret_cast<PyObject*>( tasklet );

	tasklet->m_previous = reinterpret_cast<PyObject*>( current_scheduler->m_previous_tasklet );
    
	current_scheduler->m_previous_tasklet = tasklet;

    tasklet->m_scheduled = true;

}

int Scheduler::get_tasklet_count()
{
    // TODO could be cached

	int count = 0;

    PyTaskletObject* current_tasklet = reinterpret_cast<PyTaskletObject*>( Scheduler::get_main_tasklet() );

	while( current_tasklet->m_next != Py_None)
	{
		count++;
		current_tasklet = reinterpret_cast<PyTaskletObject*>(reinterpret_cast<PyTaskletObject*>( current_tasklet )->m_next);
    }

	return count;
}

void Scheduler::schedule()
{
	Scheduler* current_scheduler = get_scheduler();

	current_scheduler->m_scheduler_tasklet->switch_to();
}

PyObject* Scheduler::run( PyTaskletObject* start_tasklet /* = nullptr */ )
{
	Scheduler* current_scheduler = get_scheduler();

	PyObject* next_tasklet = nullptr;

	if( start_tasklet )
	{
		next_tasklet = reinterpret_cast<PyObject*>( start_tasklet );
        
        if(start_tasklet->m_previous != Py_None)
		{
			reinterpret_cast<PyTaskletObject*>( start_tasklet->m_previous)->m_next = Py_None;
        }
        
    }
	else
	{
		PyTaskletObject* main_tasklet = reinterpret_cast<PyTaskletObject*>( Scheduler::get_main_tasklet() );

		next_tasklet = main_tasklet->m_next;

        main_tasklet->m_next = Py_None;
    }


    while( next_tasklet != Py_None )
	{
		PyObject* current_tasklet = next_tasklet;

        next_tasklet = reinterpret_cast<PyTaskletObject*>( current_tasklet )->m_next;

        current_scheduler->run_scheduler_callback( current_tasklet, next_tasklet );

		if(!reinterpret_cast<PyTaskletObject*>( current_tasklet )->switch_to())
		{
			return nullptr;
        }

        
        //TODO This feels hacky, this happens if the last tasklet tries to schedule itself during run
        if( reinterpret_cast<PyTaskletObject*>( current_tasklet )->m_next != Py_None)
		{
			next_tasklet = reinterpret_cast<PyTaskletObject*>( current_tasklet )->m_next;
			reinterpret_cast<PyTaskletObject*>( current_tasklet )->m_next = Py_None;
        }
        

        Scheduler::set_current_tasklet( current_scheduler->m_scheduler_tasklet );

        if(!reinterpret_cast<PyTaskletObject*>( current_tasklet )->m_alive)
		{
			Py_XDECREF( current_tasklet );
        }
	}

    // Clear previous tasklet reference
	current_scheduler->m_previous_tasklet = current_scheduler->m_scheduler_tasklet;

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