#include "Tasklet.h"

#include "ScheduleManager.h"

#include "PyTasklet.h"

#include <new>

static PyObject* TaskletExit;

static PyObject*
	Tasklet_bind( PyTaskletObject* self, PyObject* args, PyObject* kwds )
{

	PyObject* callable = nullptr;
	PyObject* bind_args = nullptr;
	PyObject* bind_kw_args = nullptr;

	bool args_supplied = false;
	bool kwargs_supplied = false;

	const char* keywords[] = { "callable", "args", "kwargs", NULL };

	if( PyArg_ParseTupleAndKeywords( args, kwds, "|OOO:bind", (char**)keywords, &callable, &bind_args, &bind_kw_args ) )
	{
		if( callable == Py_None && ( bind_args == nullptr && bind_kw_args == nullptr ) )
		{
			auto current = reinterpret_cast<PyTaskletObject*>( ScheduleManager::get_current_tasklet()->python_object() );
			if( self == current )
			{
				PyErr_SetString( PyExc_RuntimeError, "cannot unbind current tasklet" );
				return nullptr;
			}

			if( self->m_impl->scheduled() )
			{
				PyErr_SetString( PyExc_RuntimeError, "cannot unbind scheduled tasklet" );
				return nullptr;
			}

			self->m_impl->clear_callable();
			return Py_None;
		}

		if( callable != nullptr && callable != Py_None && !PyCallable_Check( callable ) )
		{
			PyErr_SetString( PyExc_TypeError, "parameter must be callable" );
			return nullptr;
		}

	    if( callable != nullptr && callable != Py_None )
	    {
		    Py_INCREF( callable );
		    // Call constructor
			self->m_impl->set_callable( callable );
	    }

		if( bind_args != nullptr )
		{
			if( bind_args == Py_None )
			{
				bind_args = Py_BuildValue( "()" );
				if( bind_args == nullptr )
				{
					PyErr_SetString( PyExc_TypeError, "internal error: Could not build empty tuple in place of PyNone for bind args" );
					return nullptr;
				}
			}
			else
			{
				Py_INCREF( bind_args );
			}

			Py_XDECREF( self->m_impl->arguments() );
			self->m_impl->set_arguments( bind_args );
			args_supplied = true;
		}

		if( bind_kw_args != nullptr )
		{
			Py_INCREF( bind_kw_args );
			Py_XDECREF( self->m_impl->kw_arguments() );
			self->m_impl->set_kw_arguments( bind_kw_args );
			kwargs_supplied = true;
		}


		if( args_supplied || kwargs_supplied )
		{
			self->m_impl->initialise();
			self->m_impl->set_alive( true );
		}


		return Py_None;
	}

	return nullptr;
}

static int
	Tasklet_init( PyTaskletObject* self, PyObject* args, PyObject* kwds )
{
	self->m_impl = (Tasklet*)PyObject_Malloc( sizeof( Tasklet ) );
	

    if( !self->m_impl )
	{
		PyErr_SetString( PyExc_RuntimeError, "Failed to allocate memory for implementation object." );

		return -1;
	}

    try
	{
		new( self->m_impl ) Tasklet( reinterpret_cast<PyObject*>( self ), nullptr, TaskletExit );
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

    if (Tasklet_bind(self, args, kwds) == nullptr)
    {
		return -1;
    }

    return 0;
}

static void
	Tasklet_dealloc( PyTaskletObject* self )
{
    // Call destructor
	self->m_impl->~Tasklet();

    PyObject_Free( self->m_impl );

	Py_TYPE( self )->tp_free( (PyObject*)self );
}

static PyObject*
	Tasklet_alive_get( PyTaskletObject* self, void* closure )
{
	return self->m_impl->alive() ? Py_True : Py_False; 
}

static PyObject*
	Tasklet_blocked_get( PyTaskletObject* self, void* closure )
{
	return self->m_impl->is_blocked() ? Py_True : Py_False;
}

static PyObject*
	Tasklet_scheduled_get( PyTaskletObject* self, void* closure )
{
	return self->m_impl->scheduled() ? Py_True : Py_False;
}

static PyObject*
	Tasklet_blocktrap_get( PyTaskletObject* self, void* closure )
{
	return self->m_impl->blocktrap() ? Py_True : Py_False;
}

static int
	Tasklet_blocktrap_set( PyTaskletObject* self, PyObject* value, void* closure )
{
	if(!PyBool_Check(value))
	{
		PyErr_SetString( PyExc_RuntimeError, "Blocktrap expects a boolean" );
		return -1;
    }

    self->m_impl->set_blocktrap( PyObject_IsTrue( value ) );

	return 0;
}

static PyObject*
	Tasklet_iscurrent_get( PyTaskletObject* self, void* closure )
{
	Tasklet* current_tasklet = ScheduleManager::get_current_tasklet();

	return reinterpret_cast<PyObject*>( self ) == current_tasklet->python_object() ? Py_True : Py_False;
}

static PyObject*
	Tasklet_ismain_get( PyTaskletObject* self, void* closure )
{
	return self->m_impl->is_main() ? Py_True : Py_False;
}

static PyObject*
	Tasklet_threadid_get( PyTaskletObject* self, void* closure )
{
	return PyLong_FromLong( self->m_impl->thread_id() );
}

static PyObject*
	Tasklet_next_get( PyTaskletObject* self, void* closure )
{
	Tasklet* next = self->m_impl->next();

    if( !next )
	{
		return Py_None;
    }
	else
	{
		PyObject* py_next = next->python_object();

        Py_IncRef( py_next );

		return py_next;
    }
    
}

static PyObject*
	Tasklet_previous_get( PyTaskletObject* self, void* closure )
{
	Tasklet* previous = self->m_impl->previous();

	if( !previous )
	{
		return Py_None;
	}
	else
	{
		PyObject* py_previous = previous->python_object();

        Py_IncRef( py_previous );

		return py_previous;
    }

}

static PyObject*
	Tasklet_paused_get( PyTaskletObject* self, void* closure )
{
	return self->m_impl->is_paused() ? Py_True : Py_False;
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
	return self->m_impl->insert() ? Py_True : Py_False;
}

static PyObject*
	Tasklet_remove( PyTaskletObject* self, PyObject* Py_UNUSED( ignored ) )
{
	return self->m_impl->remove() ? Py_True : Py_False;
}

static PyObject*
	Tasklet_run( PyTaskletObject* self, void* closure )
{
	return self->m_impl->run();
}

static PyObject*
	Tasklet_switch( PyTaskletObject* self, void* closure )
{
	return self->m_impl->switch_implementation();
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

    return self->m_impl->throw_impl( exception, value, tb, pending ) ? Py_None : nullptr;

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

	return self->m_impl->throw_impl( exception, arguments, Py_None, false ) ? Py_None : nullptr;
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

	if(self->m_impl->kill( pending ))
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


static int
    Tasklet_setup( PyObject* callable, PyObject* args, PyObject* kwargs )
{
	PyTaskletObject* tasklet = reinterpret_cast<PyTaskletObject*>( callable );
	PyObject* result = NULL;

    Py_XINCREF( args );

	Py_XDECREF( tasklet->m_impl->arguments() );

	tasklet->m_impl->set_arguments( args );

    //Initialize the tasklet
	tasklet->m_impl->initialise();

    //Mark alive
    tasklet->m_impl->set_alive( true );

    //Add to scheduler
    tasklet->m_impl->insert();

	return 0;
}

static PyObject*
	Tasklet_call( PyObject* callable, PyObject* args, PyObject* kwargs )
{
	if(Tasklet_setup( callable, args, kwargs ) == -1)
	{
		return nullptr;
    }
	else
	{
		Py_IncRef( callable );

		return callable;
	}
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
	{ "bind", (PyCFunction)Tasklet_bind, METH_VARARGS | METH_KEYWORDS, "binds a callable to a tasklet" },
	{ "setup", (PyCFunction)Tasklet_setup, METH_VARARGS | METH_KEYWORDS, "inserts a tasklet into the scheduler" },
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
	Tasklet_call, /*tp_call*/
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

