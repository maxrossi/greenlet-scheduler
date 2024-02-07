#include "PyChannel.h"


static int
	Channel_init( PyChannel* self, PyObject* args, PyObject* kwds )
{
	self->preference = 0;

	return 0;
}

static PyObject*
	Channel_getpreference( PyChannel* self, void* closure )
{
	return PyLong_FromLong( self->preference );
}

static int
	Channel_setpreference( PyChannel* self, PyObject* value, void* closure ) //TODO just test
{
	if( value == NULL )
	{
		PyErr_SetString( PyExc_TypeError, "Cannot delete the first attribute" );
		return -1;
	}
	if( !PyLong_Check( value ) )
	{
		PyErr_SetString( PyExc_TypeError,
						 "The first attribute value must be a number" );
		return -1;
	}

	self->preference = PyLong_AsLong( value );
	return 0;
}

static PyGetSetDef Channel_getsetters[] = {
	{ "preference", (getter)Channel_getpreference, (setter)Channel_setpreference, "Channel Preference", NULL },
	{ NULL } /* Sentinel */
};


static PyObject*
	Channel_send( PyChannel* self, PyObject* Py_UNUSED( ignored ) )
{
	self->send();

	return NULL;
}


static PyMethodDef Channel_methods[] = {
	{ "send", (PyCFunction)Channel_send, METH_NOARGS, "TODO description" },
	{ NULL } /* Sentinel */
};


static PyTypeObject ChannelType = {
	/* The ob_type field must be initialized in the module init function
     * to be portable to Windows without using C++. */
	PyVarObject_HEAD_INIT( NULL, 0 ) "scheduler.Channel", /*tp_name*/
	sizeof( PyChannel ), /*tp_basicsize*/
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
	PyDoc_STR( "Channel objects" ), /*tp_doc*/
	0, /*tp_traverse*/
	0, /*tp_clear*/
	0, /*tp_richcompare*/
	0, /*tp_weaklistoffset*/
	0, /*tp_iter*/
	0, /*tp_iternext*/
	Channel_methods, /*tp_methods*/
	0, /*tp_members*/
	Channel_getsetters, /*tp_getset*/
	0,
	/* see PyInit_xx */ /*tp_base*/
	0, /*tp_dict*/
	0, /*tp_descr_get*/
	0, /*tp_descr_set*/
	0, /*tp_dictoffset*/
	(initproc)Channel_init, /*tp_init*/
	0, /*tp_alloc*/
	PyType_GenericNew, /*tp_new*/
	0, /*tp_free*/
	0, /*tp_is_gc*/
};