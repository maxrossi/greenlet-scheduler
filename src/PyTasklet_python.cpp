#include "PyTasklet.h"

static PyObject* TaskletExit;

static int
	Tasklet_init( PyTaskletObject* self, PyObject* args, PyObject* kwds )
{
	self->m_alive = 0;

	return 0;
}

static PyObject*
	Tasklet_alive_get( PyTaskletObject* self, void* closure )
{
	return PyBool_FromLong( self->m_alive ); //TODO remove
}

static PyObject*
	Tasklet_blocked_get( PyTaskletObject* self, void* closure )
{
	PyErr_SetString( PyExc_RuntimeError, "Tasklet_blocked_get Not yet implemented" ); //TODO
	return NULL;
}

static PyObject*
	Tasklet_scheduled_get( PyTaskletObject* self, void* closure )
{
	PyErr_SetString( PyExc_RuntimeError, "Tasklet_scheduled_get Not yet implemented" ); //TODO
	return NULL;
}

static PyObject*
	Tasklet_blocktrap_get( PyTaskletObject* self, void* closure )
{
	PyErr_SetString( PyExc_RuntimeError, "Tasklet_blocktrap_get Not yet implemented" ); //TODO
	return NULL;
}

static PyObject*
	Tasklet_iscurrent_get( PyTaskletObject* self, void* closure )
{
	PyErr_SetString( PyExc_RuntimeError, "Tasklet_iscurrent_get Not yet implemented" ); //TODO
	return NULL;
}

static PyObject*
	Tasklet_ismain_get( PyTaskletObject* self, void* closure )
{
	PyErr_SetString( PyExc_RuntimeError, "Tasklet_ismain_get Not yet implemented" ); //TODO
	return NULL;
}

static PyObject*
	Tasklet_threadid_get( PyTaskletObject* self, void* closure )
{
	PyErr_SetString( PyExc_RuntimeError, "Tasklet_threadid_get Not yet implemented" ); //TODO
	return NULL;
}

static PyGetSetDef Tasklet_getsetters[] = {
	{ "alive", (getter)Tasklet_alive_get, NULL, "True while a tasklet is still running", NULL },
	{ "blocked", (getter)Tasklet_blocked_get, NULL, "True when a tasklet is blocked on a channel", NULL },
	{ "scheduled", (getter)Tasklet_scheduled_get, NULL, "True when the tasklet is either in the runnables list or blocked on a channel", NULL },
	{ "block_trap", (getter)Tasklet_blocktrap_get, NULL, "True while this tasklet is within a n atomic block", NULL },
	{ "is_current", (getter)Tasklet_iscurrent_get, NULL, "True if the tasklet is the current tasklet", NULL },
	{ "is_main", (getter)Tasklet_ismain_get, NULL, "True if the tasklet is the main tasklet", NULL },
	{ "thread_id", (getter)Tasklet_threadid_get, NULL, "Id of the thread the tasklet belongs to", NULL },
	{ NULL } /* Sentinel */
};


static PyObject*
	Tasklet_insert( PyTaskletObject* self, PyObject* Py_UNUSED( ignored ) )
{
	self->insert();

	return NULL;
}

static PyObject*
	Tasklet_run( PyTaskletObject* self, void* closure )
{
	PyErr_SetString( PyExc_RuntimeError, "Tasklet_run Not yet implemented" ); //TODO
	return NULL;
}

static PyObject*
	Tasklet_switch( PyTaskletObject* self, void* closure )
{
	PyErr_SetString( PyExc_RuntimeError, "Tasklet_switch Not yet implemented" ); //TODO
	return NULL;
}

static PyObject*
	Tasklet_raiseexception( PyTaskletObject* self, void* closure )
{
	PyErr_SetString( PyExc_RuntimeError, "Tasklet_raiseexception Not yet implemented" ); //TODO
	return NULL;
}

static PyObject*
	Tasklet_kill( PyTaskletObject* self, void* closure )
{
	PyErr_SetString( PyExc_RuntimeError, "Tasklet_kill Not yet implemented" ); //TODO
	return NULL;
}

static PyObject*
	Tasklet_setcontext( PyTaskletObject* self, void* closure )
{
	PyErr_SetString( PyExc_RuntimeError, "Tasklet_setcontext Not yet implemented" ); //TODO
	return NULL;
}

static PyMethodDef Tasklet_methods[] = {
	{ "insert", (PyCFunction)Tasklet_insert, METH_NOARGS, "Insert a tasklet at the end of the scheduler runnables queue" },
	{ "run", (PyCFunction)Tasklet_run, METH_NOARGS, "run immediately*" },
	{ "switch", (PyCFunction)Tasklet_switch, METH_NOARGS, "run immediately, pause caller" },
	{ "raise_exception", (PyCFunction)Tasklet_raiseexception, METH_NOARGS, "Raise an exception on the given tasklet" },
	{ "kill", (PyCFunction)Tasklet_kill, METH_NOARGS, "Terminates the tasklet and unblocks it" },
	{ "set_context", (PyCFunction)Tasklet_setcontext, METH_NOARGS, "Set the Context object to be used while this tasklet runs" },

	{ NULL } /* Sentinel */
};


static PyTypeObject TaskletType = {
	/* The ob_type field must be initialized in the module init function
     * to be portable to Windows without using C++. */
	PyVarObject_HEAD_INIT( NULL, 0 ) "scheduler.Tasklet", /*tp_name*/
	sizeof( PyTaskletObject ), /*tp_basicsize*/
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
	PyDoc_STR( "Tasklet objects" ), /*tp_doc*/
	0, /*tp_traverse*/
	0, /*tp_clear*/
	0, /*tp_richcompare*/
	0, /*tp_weaklistoffset*/
	0, /*tp_iter*/
	0, /*tp_iternext*/
	Tasklet_methods, /*tp_methods*/
	0, /*tp_members*/
	Tasklet_getsetters, /*tp_getset*/
	0,
	/* see PyInit_xx */ /*tp_base*/
	0, /*tp_dict*/
	0, /*tp_descr_get*/
	0, /*tp_descr_set*/
	0, /*tp_dictoffset*/
	(initproc)Tasklet_init, /*tp_init*/
	0, /*tp_alloc*/
	PyType_GenericNew, /*tp_new*/
	0, /*tp_free*/
	0, /*tp_is_gc*/
};

