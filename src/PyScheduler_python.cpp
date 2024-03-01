#include "PyScheduler.h"

#include "PyTasklet.h"

static int
	Scheduler_init( PySchedulerObject* self, PyObject* args, PyObject* kwds )
{
	PyObject* temp;

	Py_IncRef( &self->ob_base );

	self->s_singleton = self;

    self->m_switch_trap_level = 0;

    self->m_previous_tasklet = nullptr;

    self->m_current_tasklet_changed_callback = nullptr;

	return 0;
}

static void
	Scheduler_dealloc( PySchedulerObject* self )
{
    Py_XDECREF( self->m_current_tasklet_changed_callback );

	Py_TYPE( self )->tp_free( (PyObject*)self );
}

/* Methods */
static PyObject*
	Scheduler_getcurrent( PySchedulerObject* self, PyObject* Py_UNUSED( ignored ) )
{
	Py_INCREF( self->m_current_tasklet );

	return reinterpret_cast<PyObject*>( self->m_current_tasklet );
}

static PyObject*
	Scheduler_getmain( PySchedulerObject* self, PyObject* Py_UNUSED( ignored ) )
{
	Py_INCREF( self->m_scheduler_tasklet );

	return reinterpret_cast<PyObject*>(self->m_scheduler_tasklet);
}

static PyObject*
	Scheduler_getruncount( PySchedulerObject* self, PyObject* Py_UNUSED( ignored ) )
{
	return PyLong_FromLong(self->get_tasklet_count()+1);  // +1 is the main tasklet
}

static PyObject*
	Scheduler_schedule( PySchedulerObject* self, PyObject* Py_UNUSED( ignored ) )
{
	PyErr_SetString( PyExc_RuntimeError, "Scheduler_schedule Not yet implemented" ); //TODO
	return NULL;
}

static PyObject*
	Scheduler_scheduleremove( PySchedulerObject* self, PyObject* Py_UNUSED( ignored ) )
{
	PyErr_SetString( PyExc_RuntimeError, "Scheduler_scheduleremove Not yet implemented" ); //TODO
	return NULL;
}

static PyObject*
	Scheduler_run( PySchedulerObject* self, PyObject* Py_UNUSED( ignored ) )
{
    return self->run();
}

static PyObject*
	Scheduler_set_schedule_callback( PySchedulerObject* self, PyObject* Py_UNUSED( ignored ) )
{
	PyErr_SetString( PyExc_RuntimeError, "Scheduler_set_schedule_callback Not yet implemented" ); //TODO
	return NULL;
}

static PyObject*
	Scheduler_get_schedule_callback( PySchedulerObject* self, PyObject* Py_UNUSED( ignored ) )
{
	PyErr_SetString( PyExc_RuntimeError, "Scheduler_get_schedule_callback Not yet implemented" ); //TODO
	return NULL;
}

static PyObject*
	Scheduler_get_thread_info( PySchedulerObject* self, PyObject* args, PyObject* kwds )
{
    //TODO: extend functionality to include thread id

	PyObject* thread_info_tuple = PyTuple_New( 3 );

    Py_INCREF( self->m_scheduler_tasklet );

    PyTuple_SetItem( thread_info_tuple, 0, reinterpret_cast<PyObject*>(self->m_scheduler_tasklet) );

    Py_INCREF( self->m_current_tasklet );

    PyTuple_SetItem( thread_info_tuple, 1, reinterpret_cast<PyObject*>( self->m_current_tasklet ) );

    PyTuple_SetItem( thread_info_tuple, 2, PyLong_FromLong(self->get_tasklet_count() + 1) );

	return thread_info_tuple;
}

static PyObject*
	Scheduler_set_scheduler_tasklet( PySchedulerObject* self, PyObject* args, PyObject* kwds )
{
	PyObject* temp;

	if( PyArg_ParseTuple( args, "O:set_scheduler_tasklet", &temp ) )
	{
		Py_INCREF( temp );

        PyTaskletObject* tasklet = (PyTaskletObject*)temp;

        tasklet->m_is_main = true;

		self->m_scheduler_tasklet = tasklet;

        self->m_current_tasklet = tasklet;

        self->m_previous_tasklet = tasklet;

        self->m_scheduler_tasklet->set_to_current_greenlet();
	}

	return Py_None;
}

static PyObject*
	Scheduler_set_current_tasklet_changed_callback( PySchedulerObject* self, PyObject* args, PyObject* kwds )
{
	PyObject* temp;

	if( PyArg_ParseTuple( args, "O:set_current_tasklet_changed_callback", &temp ) )
	{
		if( !PyCallable_Check( temp ) )
		{
			PyErr_SetString( PyExc_TypeError, "parameter must be callable" );
			return NULL;
		}

		Py_INCREF( temp );

        self->m_current_tasklet_changed_callback = temp;
	}

	return Py_None;
}

static PyObject*
	Scheduler_switch_trap( PySchedulerObject* self, PyObject* args, PyObject* kwds )
{
	//TODO: channels need to track this and raise runtime error if appropriet
	int delta;

	if( !PyArg_ParseTuple( args, "i:delta", &delta ) )
	{
		PyErr_SetString( PyExc_RuntimeError, "Scheduler_switch_trap requires a delta argument." ); //TODO
	}

    self->m_switch_trap_level += delta;

    return PyLong_FromLong(self->m_switch_trap_level);
}

/* Methods end */

static PyMethodDef Scheduler_methods[] = {
	{ "getcurrent", (PyCFunction)Scheduler_getcurrent, METH_NOARGS, "Return the currently executing tasklet of this thread" },
	{ "getmain", (PyCFunction)Scheduler_getmain, METH_NOARGS, "Return the main tasklet of this thread" },
	{ "getruncount", (PyCFunction)Scheduler_getruncount, METH_NOARGS, "Return the number of currently runnable tasklets" },
	{ "schedule", (PyCFunction)Scheduler_schedule, METH_NOARGS, "Yield execution of the currently running tasklet" },
	{ "schedule_remove", (PyCFunction)Scheduler_scheduleremove, METH_NOARGS, "Yield execution of the currently running tasklet and remove" },
	{ "run", (PyCFunction)Scheduler_run, METH_NOARGS, "Run scheduler" },
	{ "set_schedule_callback", (PyCFunction)Scheduler_set_schedule_callback, METH_NOARGS, "Install a callback for scheduling" },
	{ "get_schedule_callback", (PyCFunction)Scheduler_get_schedule_callback, METH_NOARGS, "Get the current global schedule callback" },
	{ "get_thread_info", (PyCFunction)Scheduler_get_thread_info, METH_VARARGS, "Return a tuple containing the threads main tasklet, current tasklet and run-count" },
	{ "set_scheduler_tasklet", (PyCFunction)Scheduler_set_scheduler_tasklet, METH_VARARGS, "TODO" },
	{ "set_current_tasklet_changed_callback", (PyCFunction)Scheduler_set_current_tasklet_changed_callback, METH_VARARGS, "TODO" },
	{ "switch_trap", (PyCFunction)Scheduler_switch_trap, METH_VARARGS, "When the switch trap level is non-zero, any tasklet switching, e.g. due channel action or explicit, will result in a RuntimeError being raised." },
    { NULL } /* Sentinel */
};


static PyTypeObject SchedulerType = {
	/* The ob_type field must be initialized in the module init function
     * to be portable to Windows without using C++. */
	PyVarObject_HEAD_INIT( NULL, 0 ) "scheduler.Scheduler", /*tp_name*/
	sizeof( PySchedulerObject ), /*tp_basicsize*/
	0, /*tp_itemsize*/
	/* methods */
	(destructor)Scheduler_dealloc, /*tp_dealloc*/
	0, /*tp_vectorcall_offset*/
	0, /*tp_getattr*/
	0, /*tp_setattr*/
	0, /*tp_as_async*/
	0, /*tp_repr*/
	0, /*tp_as_number*/
	0, /*tp_as_sequence*/
	0, /*tp_as_mapping*/
	0, /*tp_hash*/
	0, /*tp_call*/
	0, /*tp_str*/
	0, /*tp_getattro*/
	0, /*tp_setattro*/
	0, /*tp_as_buffer*/
	Py_TPFLAGS_DEFAULT, /*tp_flags*/
	PyDoc_STR( "Scheduler objects" ), /*tp_doc*/
	0, /*tp_traverse*/
	0, /*tp_clear*/
	0, /*tp_richcompare*/
	0, /*tp_weaklistoffset*/
	0, /*tp_iter*/
	0, /*tp_iternext*/
	Scheduler_methods, /*tp_methods*/
	0, /*tp_members*/
	0, /*tp_getset*/
	0,
	/* see PyInit_xx */ /*tp_base*/
	0, /*tp_dict*/
	0, /*tp_descr_get*/
	0, /*tp_descr_set*/
	0, /*tp_dictoffset*/
	(initproc)Scheduler_init, /*tp_init*/
	0, /*tp_alloc*/
	PyType_GenericNew, /*tp_new*/
	0, /*tp_free*/
	0, /*tp_is_gc*/
};

