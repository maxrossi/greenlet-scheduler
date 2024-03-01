#include "PyScheduler.h"

#include "PyTasklet.h"

void PySchedulerObject::set_current_tasklet( PyTaskletObject* tasklet )
{
	s_singleton->m_current_tasklet = tasklet;

    // Call callback to update value 'current' in module
    // TODO: would be best to remove this and rely on getcurrent method only

    PyObject* args = PyTuple_New( 1 );

    Py_INCREF( tasklet );
	
    PyTuple_SetItem( args, 0, reinterpret_cast<PyObject*>(tasklet) );

	PyObject_CallObject( s_singleton->m_current_tasklet_changed_callback, args );

    Py_DecRef( args );
}

PyObject* PySchedulerObject::get_current_tasklet()
{
	return reinterpret_cast<PyObject*>( s_singleton->m_current_tasklet );
}

void PySchedulerObject::insert_tasklet( PyTaskletObject* tasklet )
{

    Py_INCREF( tasklet );

    
	s_singleton->m_previous_tasklet->m_next = reinterpret_cast<PyObject*>(tasklet);

	tasklet->m_previous = reinterpret_cast<PyObject*>( s_singleton->m_previous_tasklet );

    //Clear any possible old value of next that may still be around from when the tasklet was previously scheduled
	tasklet->m_next = Py_None;
    

	s_singleton->m_previous_tasklet = tasklet;


    tasklet->m_scheduled = true;

}

int PySchedulerObject::get_tasklet_count()
{
    // TODO could be cached

	int count = 0;

    PyTaskletObject* current_tasklet = reinterpret_cast<PyTaskletObject*>( PySchedulerObject::get_main_tasklet() );

	while( current_tasklet->m_next != Py_None)
	{
		count++;
		current_tasklet = reinterpret_cast<PyTaskletObject*>(reinterpret_cast<PyTaskletObject*>( current_tasklet )->m_next);
    }

	return count;
}

void PySchedulerObject::schedule()
{

	s_singleton->m_scheduler_tasklet->switch_to();

}

PyObject* PySchedulerObject::run( PyTaskletObject* start_tasklet /* = nullptr */)
{
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
		PyTaskletObject* main_tasklet = reinterpret_cast<PyTaskletObject*>( PySchedulerObject::get_main_tasklet() );

		next_tasklet = main_tasklet->m_next;

        main_tasklet->m_next = Py_None;
    }

    while( next_tasklet != Py_None )
	{
		PyObject* current_tasklet = next_tasklet;

        next_tasklet = reinterpret_cast<PyTaskletObject*>( current_tasklet )->m_next;

		reinterpret_cast<PyTaskletObject*>( current_tasklet )->switch_to();

        PySchedulerObject::set_current_tasklet( s_singleton->m_scheduler_tasklet );

        if(!reinterpret_cast<PyTaskletObject*>( current_tasklet )->m_alive)
		{
			Py_XDECREF( current_tasklet );
        }
	}

    //Clear previous tasklet reference
	s_singleton->m_previous_tasklet = s_singleton->m_scheduler_tasklet;

    Py_IncRef( Py_None );

	return Py_None;
}

PyObject* PySchedulerObject::get_main_tasklet()
{
	return reinterpret_cast<PyObject*>(s_singleton->m_scheduler_tasklet);
}