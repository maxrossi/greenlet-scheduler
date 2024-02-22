#include "PyTasklet.h"

#include "PyScheduler.h"

void PyTaskletObject::set_to_current_greenlet()
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

bool PyTaskletObject::insert()
{
	
    Py_XDECREF( m_greenlet );

    m_greenlet = PyGreenlet_New( m_callable, nullptr );
  
    PySchedulerObject::insert_tasklet( this );

	return false;

}

PyObject* PyTaskletObject::switch_to( )
{
	if( m_arguments )
	{
		if( !PyTuple_Check( m_arguments ) )
		{
			PyErr_SetString( PyExc_RuntimeError, "Arguments must be a tuple" );
			return false;
		}

	}
	
    s_current = this;

	return PyGreenlet_Switch( m_greenlet, m_arguments, nullptr );
    
}