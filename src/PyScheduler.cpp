#include "PyScheduler.h"

#include "PyTasklet.h"


PyObject* PySchedulerObject::get_current()
{

	PyErr_SetString( PyExc_RuntimeError, "Not yet implemented" );

	return nullptr;

}

void PySchedulerObject::insert_tasklet( PyTaskletObject* tasklet )
{

    Py_IncRef( &tasklet->ob_base );

    s_singleton->m_tasklets->push( (PyObject*)tasklet );

}

int PySchedulerObject::get_tasklet_count()
{
	return s_singleton->m_tasklets->size();
}

void PySchedulerObject::schedule()
{

	s_singleton->m_scheduler_tasklet->switch_to();
	
}

PyObject* PySchedulerObject::run()
{
	while( get_tasklet_count() > 0 )
	{
		PyTaskletObject* tasklet = (PyTaskletObject*)s_singleton->m_tasklets->front();

        tasklet->switch_to();

        Py_DECREF( tasklet );

		s_singleton->m_tasklets->pop();
        
    }

    Py_IncRef( Py_None );

	return Py_None;
}