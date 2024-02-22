#include "PyScheduler.h"

#include "PyTasklet.h"

static int
	Scheduler_init( PySchedulerObject* self, PyObject* args, PyObject* kwds )
{
	PyObject* temp;

    self->m_tasklets = new std::queue<PyObject*>();

	Py_IncRef( &self->ob_base );

	self->s_singleton = self;

	return 0;
}

static void
	Scheduler_dealloc( PySchedulerObject* self )
{
	delete self->m_tasklets;

	Py_TYPE( self )->tp_free( (PyObject*)self );
}

static PyObject*
	Scheduler_current_get( PySchedulerObject* self, void* closure ) //Old remove
{
	return nullptr;
}

static PyObject*
	Scheduler_main_get( PySchedulerObject* self, void* closure )
{
	PySys_WriteStdout( "Scheduler_main_get Not yet implemented \n" );  //TODO
	return Py_None;
}

static PyGetSetDef Scheduler_getsetters[] = {
	{ "current", (getter)Scheduler_current_get, NULL, "The currently executing tasklet of this thread", NULL },
	{ "main", (getter)Scheduler_main_get, NULL, "The main tasklet of this thread", NULL },
	{ NULL } /* Sentinel */
};


/* Methods */
static PyObject*
	Scheduler_getcurrent( PySchedulerObject* self, PyObject* Py_UNUSED( ignored ) )
{
	return reinterpret_cast<PyObject*>(self->get_current());
}

static PyObject*
	Scheduler_getmain( PySchedulerObject* self, PyObject* Py_UNUSED( ignored ) )
{
	PyErr_SetString( PyExc_RuntimeError, "Scheduler_getmain Not yet implemented" ); //TODO
	return NULL;
}

static PyObject*
	Scheduler_getruncount( PySchedulerObject* self, PyObject* Py_UNUSED( ignored ) )
{
	return PyLong_FromLong(self->get_tasklet_count());
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
	Scheduler_get_thread_info( PySchedulerObject* self, PyObject* Py_UNUSED( ignored ) )
{
	PyErr_SetString( PyExc_RuntimeError, "Scheduler_get_thread_info Not yet implemented" ); //TODO
	return NULL;
}

static PyObject*
	Scheduler_set_scheduler_tasklet( PySchedulerObject* self, PyObject* args, PyObject* kwds )
{
	PyObject* temp;

	if( PyArg_ParseTuple( args, "O:set_scheduler_tasklet", &temp ) )
	{
		Py_INCREF( temp );

		self->m_scheduler_tasklet = (PyTaskletObject*)temp;

        self->m_scheduler_tasklet->set_to_current_greenlet();
	}

	return Py_None;
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
	{ "get_thread_info", (PyCFunction)Scheduler_get_thread_info, METH_NOARGS, "Return a tuple containing the threads main tasklet, current tasklet and run-count" },
	{ "set_scheduler_tasklet", (PyCFunction)Scheduler_set_scheduler_tasklet, METH_VARARGS, "TODO" },
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
	Scheduler_getsetters, /*tp_getset*/
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

