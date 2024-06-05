#include "Channel.h"

#include "Tasklet.h"

#include "PyChannel.h"

#include <new>

static int
	Channel_init( PyChannelObject* self, PyObject* args, PyObject* kwds )
{
	self->m_weakref_list = nullptr;

	// Allocate the memory for the implementation member
	self->m_impl = (Channel*)PyObject_Malloc( sizeof( Channel ) );

	if( !self->m_impl )
	{
		PyErr_SetString( PyExc_RuntimeError, "Failed to allocate memory for implementation object." );

		return -1;
	}

    // Call constructor
	try
	{
		new( self->m_impl ) Channel( reinterpret_cast<PyObject*>( self ) );
	}
	catch( const std::exception& ex )
	{
		PyObject_Free( self->m_impl );

		PyErr_SetString( PyExc_RuntimeError, ex.what() );

		return -1;
	}
	catch( ... )
	{
		PyObject_Free( self->m_impl );

		PyErr_SetString( PyExc_RuntimeError, "Failed to construct implementation object." );

		return -1;
	}

	return 0;
}

static void
	Channel_dealloc( PyChannelObject* self )
{
    // Call destructor
	self->m_impl->~Channel();

    PyObject_Free( self->m_impl );

    // Handle weakrefs
	if( self->m_weakref_list != nullptr )
		PyObject_ClearWeakRefs( (PyObject*)self );

    Py_TYPE( self )->tp_free( (PyObject*)self );
}

static PyObject*
	Channel_preference_get( PyChannelObject* self, void* closure )
{
	return PyLong_FromLong( self->m_impl->preference() );
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

    long new_preference = PyLong_AsLong( value );

    // Only accept valid values
    // -1   - Prefer receive
    // 0    - Prefer neither
    // 1    - Prefer sender
    if( ( new_preference > -2 ) && ( new_preference < 2 ) )
	{
		self->m_impl->set_preference( new_preference );
    }

	return 0;
}

static PyObject*
	Channel_balance_get( PyChannelObject* self, void* closure )
{
	return PyLong_FromLong( self->m_impl->balance() );
}

static PyObject*
	Channel_queue_get( PyChannelObject* self, void* closure )
{
	Tasklet* front = self->m_impl->blocked_queue_front();

    if (!front)
    {
		Py_IncRef(Py_None);

		return Py_None;
    }
    else
    {
		PyObject* front_of_queue = front->python_object();

        Py_IncRef( front_of_queue );

		return front_of_queue;
    }
}

static PyGetSetDef Channel_getsetters[] = {
	{ "preference", (getter)Channel_preference_get, (setter)Channel_preference_set, "allows for customisation of how the channel actions", NULL },
	{ "balance", (getter)Channel_balance_get, NULL, "number of tasklets waiting to send (>0) or receive (<0)", NULL },
	{ "queue", (getter)Channel_queue_get, NULL, "the first tasklet in the chain of tasklets that are blocked on the channel", NULL },
	{ NULL } /* Sentinel */
};

static PyObject*
	Channel_send( PyChannelObject* self, PyObject* args, PyObject* kwds )
{
	PyObject* value;

	if( !PyArg_ParseTuple( args, "O:send_value", &value ) )
	{
		return nullptr;
	}

	if( !self->m_impl->send( value ) )
	{
		return nullptr;
	}

    Py_IncRef( Py_None );

	return Py_None;
}

static PyObject*
	Channel_receive( PyChannelObject* self, PyObject* Py_UNUSED( ignored ) )
{
	return self->m_impl->receive();
}

static PyObject*
	Channel_sendexception( PyChannelObject* self, PyObject* args, PyObject* kwds )
{
	
    if (PyTuple_Size(args) < 1)
    {
		PyErr_SetString( PyExc_RuntimeError, "Exception type required" );
		return nullptr;
    }

    PyObject* exception = PyTuple_GetItem(args,0);

    if( !PyExceptionClass_Check( exception ) && !PyObject_IsInstance( exception, PyExc_Exception ) )
    {
		PyErr_SetString( PyExc_RuntimeError, "Exception type or instance required" );
		return nullptr;
    }

    Py_IncRef( exception );

    PyObject* exception_arguments = nullptr;

    if (PyTuple_Size(args) > 1)
    {
		exception_arguments = PyTuple_GetSlice( args, 1, PyTuple_Size( args ) );
    }
    else
    {
		exception_arguments = PyTuple_New( 0 );
    }

	if( !self->m_impl->send( exception_arguments, exception ) )
	{
		Py_DecRef( exception_arguments );

		return NULL;
    }

    Py_DecRef( exception_arguments );

    Py_IncRef( Py_None );

	return Py_None;
}

static PyObject*
	Channel_sendThrow( PyChannelObject* self, PyObject* args, PyObject* kwds )
{
	const char* kwlist[] = { "exc", "val", "tb", NULL };

	PyObject* exception = nullptr;
	PyObject* value = Py_None;
	PyObject* tb = Py_None;

	if( !PyArg_ParseTupleAndKeywords( args, kwds, "O|OO", (char**)kwlist, &exception, &value, &tb ) )
	{
		PyErr_SetString( PyExc_RuntimeError, "Failed to parse arguments" );
		return nullptr;
	}

	if( !PyExceptionClass_Check( exception ) && !PyObject_IsInstance( exception, PyExc_Exception ) )
	{
		PyErr_SetString( PyExc_RuntimeError, "Exception type or instance required" );
		return nullptr;
	}

    Py_IncRef( exception );

	Py_IncRef( value );

	Py_IncRef( tb );

    PyObject* exception_arguments = PyTuple_New(2);

    PyTuple_SetItem( exception_arguments, 0, value );

	PyTuple_SetItem( exception_arguments, 1, tb );
	
    if( !self->m_impl->send( exception_arguments, exception ) )
	{
		Py_DecRef( exception_arguments );

		return NULL;
	}
    
    Py_DecRef( exception_arguments );

    Py_IncRef( Py_None );

	return Py_None;
}


static PyObject*
	Channel_iter( PyChannelObject* self )
{
	Py_INCREF( self ); 

	return reinterpret_cast<PyObject*>(self);
}

static PyObject*
	Channel_next( PyChannelObject* self )
{
    // Run receive until unblocked
    // Note: behaviour is slightly different to stackless but probably better
    // At end of iteration there will be an error due to DEADLOCK
    // This will return a nullptr
    // This null then returned here will turn this into a StopIteration error
    // Which makes more sense
	return Channel_receive( self, nullptr );
}

static PyObject*
	Channel_clearTasklets( PyChannelObject* self, PyObject* Py_UNUSED( ignored ) )
{
	self->m_impl->clear_blocked( false );

	Py_IncRef( Py_None );

    return Py_None;
}

static PyMethodDef Channel_methods[] = {
	{ "send", (PyCFunction)Channel_send, METH_VARARGS, "Send a value over the channel" },
	{ "receive", (PyCFunction)Channel_receive, METH_NOARGS, "Receive a value over the channel" },
	{ "send_exception", (PyCFunction)Channel_sendexception, METH_VARARGS, "Send an exception over the channel" },
	{ "send_throw", (PyCFunction)Channel_sendThrow, METH_VARARGS | METH_KEYWORDS, "(exc, val, tb) is raised on the first tasklet blocked on channel self." },
	{ "clear", (PyCFunction)Channel_clearTasklets, METH_NOARGS, "Clear channel, all blocked tasklets will be killed rasing TaskletExit exception" },
	{ NULL } /* Sentinel */
};

static PyTypeObject ChannelType = {
	/* The ob_type field must be initialized in the module init function
     * to be portable to Windows without using C++. */
	PyVarObject_HEAD_INIT( NULL, 0 ) "scheduler.Channel", /*tp_name*/
	sizeof( PyChannelObject ) + sizeof( Channel ), /*tp_basicsize*/
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
	offsetof( PyChannelObject, m_weakref_list ), /*tp_weaklistoffset*/
	(getiterfunc)Channel_iter, /*tp_iter*/
	(iternextfunc)Channel_next, /*tp_iternext*/
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