#include "PyScheduler.h"


static int
	Scheduler_init( PyScheduler* self, PyObject* args, PyObject* kwds )
{
	self->current = reinterpret_cast<PyTasklet*>(Py_None);

	return 0;
}

static PyObject*
	Scheduler_current_get( PyScheduler* self, void* closure )
{
	return Py_NewRef(reinterpret_cast<PyObject*>(self->current));
}

static PyGetSetDef Scheduler_getsetters[] = {
	{ "current", (getter)Scheduler_current_get, NULL, "Get current", NULL },
	{ NULL } /* Sentinel */
};


/* Methods */
static PyObject*
	Scheduler_getcurrent( PyScheduler* self, PyObject* Py_UNUSED( ignored ) )
{
	return reinterpret_cast<PyObject*>(self->get_current());
}



/* Methods end */


static PyMethodDef Scheduler_methods[] = {
	{ "getcurrent", (PyCFunction)Scheduler_getcurrent, METH_NOARGS, "Return the currently executing tasklet of this thread" },
	{ NULL } /* Sentinel */
};


static PyTypeObject SchedulerType = {
	/* The ob_type field must be initialized in the module init function
     * to be portable to Windows without using C++. */
	PyVarObject_HEAD_INIT( NULL, 0 ) "scheduler.Scheduler", /*tp_name*/
	sizeof( PyScheduler ), /*tp_basicsize*/
	0, /*tp_itemsize*/
	/* methods */
	0, /*tp_dealloc*/
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

