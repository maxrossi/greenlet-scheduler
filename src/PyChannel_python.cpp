#include "PyChannel.h"


static int
	Channel_init( PyChannelObject* self, PyObject* args, PyObject* kwds )
{
	self->m_transfer_arguments = nullptr;

	self->m_preference = 0;

    self->m_waiting_to_send = new std::queue<PyObject*>();

    self->m_waiting_to_receive = new std::queue<PyObject*>();

	return 0;
}

static void
	Channel_dealloc( PyChannelObject* self )
{
    //TODO need to clean up tasklets that are stored in waiting_to_send and waiting_to_receive lists

	delete self->m_waiting_to_send;

	delete self->m_waiting_to_receive;

    Py_XDECREF( self->m_transfer_arguments );

	Py_TYPE( self )->tp_free( (PyObject*)self );
}

static PyObject*
	Channel_preference_get( PyChannelObject* self, void* closure )
{
	return PyLong_FromLong( self->m_preference );
}

static int
	Channel_preference_set( PyChannelObject* self, PyObject* value, void* closure ) //TODO just test
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

	self->m_preference = PyLong_AsLong( value );
	return 0;
}

static PyObject*
	Channel_balance_get( PyChannelObject* self, void* closure )
{
	return PyLong_FromLong( self->balance() );
}

static int
	Channel_balance_set( PyChannelObject* self, PyObject* value, void* closure ) //TODO just test
{
	PyErr_SetString( PyExc_RuntimeError, "Channel_balance_set Not yet implemented" ); //TODO
	return -1;
}

static PyObject*
	Channel_queue_get( PyChannelObject* self, void* closure )
{
	PyErr_SetString( PyExc_RuntimeError, "Channel_queue_get Not yet implemented" ); //TODO
	return NULL;
}

static PyGetSetDef Channel_getsetters[] = {
	{ "preference", (getter)Channel_preference_get, (setter)Channel_preference_set, "allows for customisation of how the channel actions", NULL },
	{ "balance", (getter)Channel_balance_get, (setter)Channel_balance_set, "number of tasklets waiting to send (>0) or receive (<0)", NULL },
	{ "queue", (getter)Channel_queue_get, NULL, "the first tasklet in the chain of tasklets that are blocked on the channel", NULL },
	{ NULL } /* Sentinel */
};


static PyObject*
	Channel_send( PyChannelObject* self, PyObject* args, PyObject* kwds )
{
	PyObject* value;

	if( PyArg_ParseTuple( args, "O:send_value", &value ) )
	{
		if( !self->send( value ) )
		{
			return NULL;
		}
	}

    Py_IncRef( Py_None );

	return Py_None;
}

static PyObject*
	Channel_receive( PyChannelObject* self, PyObject* Py_UNUSED( ignored ) )
{
	return self->receive();
}

static PyObject*
	Channel_sendexception( PyChannelObject* self, PyObject* args, PyObject* kwds )
{
	PyErr_SetString( PyExc_RuntimeError, "Channel_sendexception Not yet implemented" ); //TODO
	return NULL;
}


static PyMethodDef Channel_methods[] = {
	{ "send", (PyCFunction)Channel_send, METH_VARARGS, "Send a value over the channel" },
	{ "receive", (PyCFunction)Channel_receive, METH_NOARGS, "Receive a value over the channel" },
	{ "send_exception", (PyCFunction)Channel_sendexception, METH_VARARGS, "Send an exception over the channel" },
	{ NULL } /* Sentinel */
};


static PyTypeObject ChannelType = {
	/* The ob_type field must be initialized in the module init function
     * to be portable to Windows without using C++. */
	PyVarObject_HEAD_INIT( NULL, 0 ) "scheduler.Channel", /*tp_name*/
	sizeof( PyChannelObject ), /*tp_basicsize*/
	0, /*tp_itemsize*/
	/* methods */
	(destructor)Channel_dealloc, /*tp_dealloc*/
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
	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /*tp_flags*/
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