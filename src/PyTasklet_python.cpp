#include "PyTasklet.h"

#include "PyScheduler.h"

#include <new>

static PyObject* TaskletExit;

static int
	Tasklet_init( PyTaskletObject* self, PyObject* args, PyObject* kwds )
{
    //Arguments passed in will be a callable function
    //TODO this needs to be in the bind function
	PyObject* temp;

	if( PyArg_ParseTuple( args, "O:set_callable", &temp ) )
	{
		if( !PyCallable_Check( temp ) )
		{
			PyErr_SetString( PyExc_TypeError, "parameter must be callable" );
			return -1;
		}

		Py_INCREF( temp );
		
        // Call constructor
		new( self ) PyTaskletObject( temp, TaskletExit );

        return 0;
	}

	return -1;
}

static void
	Tasklet_dealloc( PyTaskletObject* self )
{
    // Call destructor
	self->~PyTaskletObject();

	Py_TYPE( self )->tp_free( (PyObject*)self );
}

static PyObject*
	Tasklet_alive_get( PyTaskletObject* self, void* closure )
{
	return self->alive() ? Py_True : Py_False; 
}

static PyObject*
	Tasklet_blocked_get( PyTaskletObject* self, void* closure )
{
	return self->is_blocked() ? Py_True : Py_False;
}

static PyObject*
	Tasklet_scheduled_get( PyTaskletObject* self, void* closure )
{
	return self->scheduled() ? Py_True : Py_False;
}

static PyObject*
	Tasklet_blocktrap_get( PyTaskletObject* self, void* closure )
{
	return self->blocktrap() ? Py_True : Py_False;
}

static int
	Tasklet_blocktrap_set( PyTaskletObject* self, PyObject* value, void* closure )
{
	if(!PyBool_Check(value))
	{
		PyErr_SetString( PyExc_RuntimeError, "Blocktrap expects a boolean" );
		return -1;
    }

    self->set_blocktrap(PyObject_IsTrue( value ));

	return 0;
}

static PyObject*
	Tasklet_iscurrent_get( PyTaskletObject* self, void* closure )
{
	PyObject* current_tasklet = Scheduler::get_current_tasklet();

	return reinterpret_cast<PyObject*>( self ) == current_tasklet ? Py_True : Py_False;
}

static PyObject*
	Tasklet_ismain_get( PyTaskletObject* self, void* closure )
{
	return self->is_main() ? Py_True : Py_False;
}

static PyObject*
	Tasklet_threadid_get( PyTaskletObject* self, void* closure )
{
	return PyLong_FromLong( self->thread_id() );
}

static PyObject*
	Tasklet_next_get( PyTaskletObject* self, void* closure )
{
	PyObject* next = nullptr;

	next = self->next();

    if( !next )
	{
		PyErr_SetString( PyExc_RuntimeError, "Next tasklet in erroneous state." ); //TODO
    }

    Py_IncRef( next );
	
	return next;
}

static PyObject*
	Tasklet_previous_get( PyTaskletObject* self, void* closure )
{
	PyObject* previous = nullptr;

	previous = self->previous();

	if( !previous )
	{
		PyErr_SetString( PyExc_RuntimeError, "Previous tasklet in erroneous state." ); //TODO
	}

	Py_IncRef( previous );

	return previous;
}

static PyObject*
	Tasklet_paused_get( PyTaskletObject* self, void* closure )
{
	return self->is_paused() ? Py_True : Py_False;
}

static PyGetSetDef Tasklet_getsetters[] = {
	{ "alive", (getter)Tasklet_alive_get, NULL, "True while a tasklet is still running", NULL },
	{ "blocked", (getter)Tasklet_blocked_get, NULL, "True when a tasklet is blocked on a channel", NULL },
	{ "scheduled", (getter)Tasklet_scheduled_get, NULL, "True when the tasklet is either in the runnables list or blocked on a channel", NULL },
	{ "block_trap", (getter)Tasklet_blocktrap_get, (setter)Tasklet_blocktrap_set, "True while this tasklet is within a n atomic block", NULL },
	{ "is_current", (getter)Tasklet_iscurrent_get, NULL, "True if the tasklet is the current tasklet", NULL },
	{ "is_main", (getter)Tasklet_ismain_get, NULL, "True if the tasklet is the main tasklet", NULL },
	{ "thread_id", (getter)Tasklet_threadid_get, NULL, "Id of the thread the tasklet belongs to", NULL },
	{ "next", (getter)Tasklet_next_get, NULL, "Get next tasklet in scheduler", NULL },
	{ "prev", (getter)Tasklet_previous_get, NULL, "Get next tasklet in scheduler", NULL },
	{ "paused", (getter)Tasklet_paused_get, NULL, "This attribute is True when a tasklet is alive, but not scheduled or blocked on a channel", NULL },
	{ NULL } /* Sentinel */
};

static PyObject*
	Tasklet_insert( PyTaskletObject* self, PyObject* Py_UNUSED( ignored ) )
{
	return self->insert() ? Py_True : Py_False;
}

static PyObject*
	Tasklet_remove( PyTaskletObject* self, PyObject* Py_UNUSED( ignored ) )
{
	return self->remove() ? Py_True : Py_False;
}

static PyObject*
	Tasklet_run( PyTaskletObject* self, void* closure )
{
	return self->run();
}

static PyObject*
	Tasklet_switch( PyTaskletObject* self, void* closure )
{
	return self->switch_implementation();
}

static bool
	check_exception_setup( PyObject* exception, PyObject* arguments )
{
	if( PyObject_IsInstance( exception, PyExc_Exception ) )
	{
		// If exception state is an implementation then expect no arguments
		if( arguments != Py_None )
		{
			PyErr_SetString( PyExc_TypeError, "missing required argument 'exc' (pos 1)" );

			return false;
		}
		else
		{
			return true;
		}
	}
	else
	{
		if( exception == Py_None )
		{
			PyErr_SetString( PyExc_TypeError, "missing required argument 'exc' (pos 1)" );

			return false;
		}
		if( !PyExceptionClass_Check( exception ) )
		{
			PyErr_SetString( PyExc_TypeError, "exceptions must be classes, or instances" );

			return false;
		}
		else
		{
			return true;
		}
	}
}

static PyObject*
	Tasklet_throw( PyTaskletObject* self, PyObject* args, PyObject* kwds )
{
	const char* kwlist[] = { "exc", "val", "tb", "pending", NULL };

    PyObject* exception = Py_None;
	PyObject* value = Py_None;
	PyObject* tb = Py_None; // TODO not yet used but the test passes which isn't great? 
	bool pending = false;

    if( !PyArg_ParseTupleAndKeywords( args, kwds, "|OOOp", (char**)kwlist, &exception, &value, &tb, &pending ) )
	{
		PyErr_SetString( PyExc_RuntimeError, "Failed to parse arguments" );
		return nullptr;
    }

    // Test state validity
	if( !check_exception_setup( exception, value ) )
	{
		return nullptr;
    }

    return self->throw_impl( exception, value, tb, pending ) ? Py_None : nullptr;

}

static PyObject*
	Tasklet_raiseexception( PyTaskletObject* self, PyObject* args, PyObject* kwds )
{
	PyObject* exception = Py_None;
	PyObject* arguments = Py_None;

    if( !PyArg_ParseTuple( args, "O:exception_class|O", &exception, &arguments ) )
	{
		PyErr_SetString( PyExc_RuntimeError, "Failed to parse arguments" );
		return nullptr;
	}

    // Test state validity
	if( !check_exception_setup( exception, arguments ) )
	{
		return nullptr;
	}

	return self->throw_impl( exception, arguments, Py_None, false ) ? Py_None : nullptr;
}

static PyObject*
	Tasklet_kill( PyTaskletObject* self, PyObject* args, PyObject* kwds )
{
	const char* kwlist[] = { "pending", NULL };

	bool pending = false;

	if( !PyArg_ParseTupleAndKeywords( args, kwds, "|p", (char**)kwlist, &pending ) )
	{
		PyErr_SetString( PyExc_RuntimeError, "Failed to parse arguments" );
		return nullptr;
    }

	if(self->kill( pending ))
	{
		return Py_None;
    }
	else
	{
		return nullptr;
    }

}

static PyObject*
	Tasklet_setcontext( PyTaskletObject* self, void* closure )
{
	PyErr_SetString( PyExc_RuntimeError, "Tasklet_setcontext Not yet implemented" ); //TODO
	return NULL;
}


static PyObject*
    Tasklet_setup( PyObject* callable, PyObject* args, PyObject* kwargs )
{
	PyTaskletObject* tasklet = reinterpret_cast<PyTaskletObject*>( callable );
	PyObject* result = NULL;

    Py_XINCREF( args );
	Py_XDECREF( tasklet->arguments() );
	tasklet->set_arguments(args);

    //Initialize the tasklet
	tasklet->initialise();

    //Add to scheduler
    tasklet->insert();

    tasklet->set_alive(true);

    Py_IncRef( callable );

	return callable;
}


static PyMethodDef Tasklet_methods[] = {
	{ "insert", (PyCFunction)Tasklet_insert, METH_NOARGS, "Insert a tasklet at the end of the scheduler runnables queue" },
	{ "remove", (PyCFunction)Tasklet_remove, METH_NOARGS, "Remove a tasklet from the runnables queue" },
	{ "run", (PyCFunction)Tasklet_run, METH_NOARGS, "run immediately*" },
	{ "switch", (PyCFunction)Tasklet_switch, METH_NOARGS, "run immediately, pause caller" },
	{ "throw", (PyCFunction)Tasklet_throw, METH_VARARGS | METH_KEYWORDS, "Raise an exception on the given tasklet" },
	{ "raise_exception", (PyCFunction)Tasklet_raiseexception, METH_VARARGS, "Raise an exception on the given tasklet" },
	{ "kill", (PyCFunction)Tasklet_kill, METH_VARARGS | METH_KEYWORDS, "Terminates the tasklet and unblocks it" },
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
	(destructor)Tasklet_dealloc, /*tp_dealloc*/
	0, /*tp_vectorcall_offset*/
	0, /*tp_getattr*/
	0, /*tp_setattr*/
	0, /*tp_as_async*/
	0, /*tp_repr*/
	0, /*tp_as_number*/
	0, /*tp_as_sequence*/
	0, /*tp_as_mapping*/
	0, /*tp_hash*/
	Tasklet_setup, /*tp_call*/
	0, /*tp_str*/
	0, /*tp_getattro*/
	0, /*tp_setattro*/
	0, /*tp_as_buffer*/
	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /*tp_flags*/
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

