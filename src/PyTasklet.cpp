#include "Tasklet.h"

#include "ScheduleManager.h"

#include "PyTasklet.h"

#include <new>

static PyObject* TaskletExit;


static PyObject*
	Tasklet_new( PyTypeObject* type, PyObject* args, PyObject* kwds )
{
	PyTaskletObject* self;

	self = (PyTaskletObject*)type->tp_alloc( type, 0 );

	if( self != nullptr )
	{
		self->m_impl = nullptr;

        self->m_weakref_list = nullptr;
	}

	return (PyObject*)self;
}

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
			ScheduleManager* schedule_manager = ScheduleManager::get_scheduler();

			auto current = reinterpret_cast<PyTaskletObject*>( schedule_manager->get_current_tasklet()->python_object() );

            schedule_manager->decref();

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

            // Clear callable
			self->m_impl->set_callable( nullptr );

            // Clear arguments
            self->m_impl->set_arguments( nullptr );

            // Clear greenlet
            self->m_impl->uninitialise();

            self->m_impl->set_alive( false );

			Py_IncRef( Py_None );

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

			self->m_impl->set_arguments( bind_args );
			args_supplied = true;
		}

		if( bind_kw_args != nullptr )
		{
			Py_INCREF( bind_kw_args );
			self->m_impl->set_kw_arguments( bind_kw_args );
			kwargs_supplied = true;
		}


		if( args_supplied || kwargs_supplied )
		{
			self->m_impl->initialise();
			self->m_impl->set_alive( true );
		}

		Py_IncRef(Py_None);

		return Py_None;
	}

	return nullptr;
}

static int
	Tasklet_init( PyTypeObject* self, PyObject* args, PyObject* kwds )
{
	PyTaskletObject* tasklet_object = reinterpret_cast<PyTaskletObject*>( self );

	tasklet_object->m_impl = (Tasklet*)PyObject_Malloc( sizeof( Tasklet ) );

	if( !tasklet_object->m_impl )
	{
		PyErr_SetString( PyExc_RuntimeError, "Failed to allocate memory for implementation object." );

		return -1;
	}

    PyObject* callable = nullptr;
	bool is_main = false;

    if( !PyArg_ParseTuple( args, "|Op", &callable, &is_main ) )
	{
		PyErr_SetString( PyExc_RuntimeError, "Failed while parsing arguments" );

		return -1;
	}

    try
	{
		new( tasklet_object->m_impl ) Tasklet( reinterpret_cast<PyObject*>( self ), TaskletExit, is_main );
	}
	catch( const std::exception& ex )
	{
		PyObject_Free( tasklet_object->m_impl );

		PyErr_SetString( PyExc_RuntimeError, ex.what() );

		return -1;
	}
	catch( ... )
	{
		PyObject_Free( tasklet_object->m_impl );

		PyErr_SetString( PyExc_RuntimeError, "Failed to construct implementation object." );

		return -1;
	}

    // Don't pass args if this is main tasklet

    if (!is_main)
    {
        if( Tasklet_bind( tasklet_object, args, kwds ) == nullptr )
        {
		    Py_DecRef( args );
		    return -1;
        }
	}
    
    return 0;
}

static void
	Tasklet_dealloc( PyTaskletObject* self )
{
    // Call destructor
    if (self->m_impl)
    {
		self->m_impl->~Tasklet();

		PyObject_Free( self->m_impl );
    }
	
    // Handle weakrefs
	if( self->m_weakref_list != nullptr )
	{
		PyObject_ClearWeakRefs( (PyObject*)self );
	}

	Py_TYPE( self )->tp_free( (PyObject*)self );
}

static bool PyTaskletObject_is_valid( PyTaskletObject* tasklet )
{
    if (!tasklet->m_impl)
    {
		PyErr_SetString( PyExc_RuntimeError, "Tasklet object is not valid. Most likely cause being __init__ not called on base type." );

		return false;
    }

    return true;
}

static PyObject*
	Tasklet_alive_get( PyTaskletObject* self, void* closure )
{
    // Ensure PyTaskletObject is in a valid state
    if (!PyTaskletObject_is_valid(self))
    {
		return nullptr;
    }

	return self->m_impl->alive() ? Py_True : Py_False; 
}

static PyObject*
	Tasklet_blocked_get( PyTaskletObject* self, void* closure )
{
	// Ensure PyTaskletObject is in a valid state
	if( !PyTaskletObject_is_valid( self ) )
	{
		return nullptr;
	}

	return self->m_impl->is_blocked() ? Py_True : Py_False;
}

static PyObject*
	Tasklet_scheduled_get( PyTaskletObject* self, void* closure )
{
	// Ensure PyTaskletObject is in a valid state
	if( !PyTaskletObject_is_valid( self ) )
	{
		return nullptr;
	}

	return self->m_impl->scheduled() ? Py_True : Py_False;
}

static PyObject*
	Tasklet_blocktrap_get( PyTaskletObject* self, void* closure )
{
	// Ensure PyTaskletObject is in a valid state
	if( !PyTaskletObject_is_valid( self ) )
	{
		return nullptr;
	}

	return self->m_impl->blocktrap() ? Py_True : Py_False;
}

static int
	Tasklet_blocktrap_set( PyTaskletObject* self, PyObject* value, void* closure )
{
	// Ensure PyTaskletObject is in a valid state
	if( !PyTaskletObject_is_valid( self ) )
	{
		return -1;
	}

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
	// Ensure PyTaskletObject is in a valid state
	if( !PyTaskletObject_is_valid( self ) )
	{
		return nullptr;
	}

	ScheduleManager* schedule_manager = ScheduleManager::get_scheduler();

	Tasklet* current_tasklet = schedule_manager->get_current_tasklet();

    schedule_manager->decref();

	return reinterpret_cast<PyObject*>( self ) == current_tasklet->python_object() ? Py_True : Py_False;
}

static PyObject*
	Tasklet_ismain_get( PyTaskletObject* self, void* closure )
{
	// Ensure PyTaskletObject is in a valid state
	if( !PyTaskletObject_is_valid( self ) )
	{
		return nullptr;
	}

	return self->m_impl->is_main() ? Py_True : Py_False;
}

static PyObject*
	Tasklet_threadid_get( PyTaskletObject* self, void* closure )
{
	// Ensure PyTaskletObject is in a valid state
	if( !PyTaskletObject_is_valid( self ) )
	{
		return nullptr;
	}

	return PyLong_FromLong( self->m_impl->thread_id() );
}

static PyObject*
	Tasklet_next_get( PyTaskletObject* self, void* closure )
{
	// Ensure PyTaskletObject is in a valid state
	if( !PyTaskletObject_is_valid( self ) )
	{
		return nullptr;
	}

	Tasklet* next = self->m_impl->next();

    if( !next )
	{
		Py_IncRef( Py_None );

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
	// Ensure PyTaskletObject is in a valid state
	if( !PyTaskletObject_is_valid( self ) )
	{
		return nullptr;
	}

	Tasklet* previous = self->m_impl->previous();

	if( !previous )
	{
		Py_IncRef( Py_None );

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
	// Ensure PyTaskletObject is in a valid state
	if( !PyTaskletObject_is_valid( self ) )
	{
		return nullptr;
	}

	return self->m_impl->is_paused() ? Py_True : Py_False;
}

static PyObject*
    Tasklet_frame_get(PyTaskletObject* self, void* closure)
{
	PyObject* greenlet = reinterpret_cast<PyObject*>(self->m_impl->get_greenlet());

    if (greenlet == nullptr)
    {
		PyErr_SetString( PyExc_RuntimeError, "Attempting to access a frame from a tasklet with no greenlet." );
		return nullptr;
    }

    return PyObject_GetAttrString( greenlet, "gr_frame" );
}

static PyGetSetDef Tasklet_getsetters[] = {
	{ "alive", 
        (getter)Tasklet_alive_get,
        NULL,
        "True while a tasklet is still running.",
        NULL },

	{ "blocked",
        (getter)Tasklet_blocked_get,
        NULL,
        "True when a tasklet is blocked on a channel.",
        NULL },

	{ "scheduled",
        (getter)Tasklet_scheduled_get,
        NULL,
        "True when the tasklet is either in the runnables list or blocked on a channel.",
        NULL },

	{ "block_trap",
        (getter)Tasklet_blocktrap_get,
        (setter)Tasklet_blocktrap_set,
        "True while this tasklet is within a n atomic block.",
        NULL },

	{ "is_current",
        (getter)Tasklet_iscurrent_get,
        NULL,
        "True if the tasklet is the current tasklet.",
        NULL },

	{ "is_main",
        (getter)Tasklet_ismain_get,
        NULL,
        "True if the tasklet is the main tasklet.",
        NULL },

	{ "thread_id",
        (getter)Tasklet_threadid_get,
        NULL,
        "Id of the thread the tasklet belongs to.",
        NULL },

	{ "next",
        (getter)Tasklet_next_get,
        NULL,
        "Get next tasklet in schedule queue.",
        NULL },

	{ "prev",
        (getter)Tasklet_previous_get,
        NULL,
        "Get next tasklet in schedule queue.",
        NULL },

	{ "paused",
        (getter)Tasklet_paused_get,
        NULL,
        "This attribute is True when a tasklet is alive, but not scheduled or blocked on a channel.",
        NULL },
	{ "frame",

        (getter)Tasklet_frame_get,
        NULL,
	    "Get the current frame of a tasklet",
        NULL },

	{ NULL } /* Sentinel */
};

static PyObject*
	Tasklet_insert( PyTaskletObject* self, PyObject* Py_UNUSED( ignored ) )
{
	// Ensure PyTaskletObject is in a valid state
	if( !PyTaskletObject_is_valid( self ) )
	{
		return nullptr;
	}

	return self->m_impl->insert() ? Py_True : Py_False;
}

static PyObject*
	Tasklet_remove( PyTaskletObject* self, PyObject* Py_UNUSED( ignored ) )
{
	// Ensure PyTaskletObject is in a valid state
	if( !PyTaskletObject_is_valid( self ) )
	{
		return nullptr;
	}

	return self->m_impl->remove() ? Py_True : Py_False;
}

static PyObject*
	Tasklet_run( PyTaskletObject* self, void* closure )
{
	// Ensure PyTaskletObject is in a valid state
	if( !PyTaskletObject_is_valid( self ) )
	{
		return nullptr;
	}

    if (self->m_impl->run())
    {
		Py_IncRef( Py_None );

		return Py_None;
    }
    else
    {
		return nullptr;
    }
}

static PyObject*
	Tasklet_switch( PyTaskletObject* self, void* closure )
{
	// Ensure PyTaskletObject is in a valid state
	if( !PyTaskletObject_is_valid( self ) )
	{
		return nullptr;
	}

    if (self->m_impl->switch_implementation())
    {
		Py_IncRef( Py_None );

        return Py_None;
    }
    else
    {
		return nullptr;
    }
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
	// Ensure PyTaskletObject is in a valid state
	if( !PyTaskletObject_is_valid( self ) )
	{
		return nullptr;
	}

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

    if (self->m_impl->throw_exception(exception, value, tb, pending))
    {
		Py_IncRef( Py_None );

        return Py_None;
    }
    else
    {
		return nullptr;
    }

}

static PyObject*
	Tasklet_raiseexception( PyTaskletObject* self, PyObject* args, PyObject* kwds )
{
	// Ensure PyTaskletObject is in a valid state
	if( !PyTaskletObject_is_valid( self ) )
	{
		return nullptr;
	}

	PyObject* exception = nullptr;
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

    if (self->m_impl->throw_exception(exception, arguments, Py_None, false))
    {
		Py_IncRef( Py_None );

        return Py_None;
    }
    else
    {
		return nullptr;
    }
}

static PyObject*
	Tasklet_kill( PyTaskletObject* self, PyObject* args, PyObject* kwds )
{
	// Ensure PyTaskletObject is in a valid state
	if( !PyTaskletObject_is_valid( self ) )
	{
		return nullptr;
	}

	const char* kwlist[] = { "pending", NULL };

	bool pending = false;

	if( !PyArg_ParseTupleAndKeywords( args, kwds, "|p", (char**)kwlist, &pending ) )
	{
		PyErr_SetString( PyExc_RuntimeError, "Failed to parse arguments" );
		return nullptr;
    }

	if(self->m_impl->kill( pending ))
	{
		Py_IncRef( Py_None );

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

    // Ensure PyTaskletObject is in a valid state
	if( !PyTaskletObject_is_valid( tasklet ) )
	{
		return nullptr;
	}

	PyObject* result = NULL;

    Py_XINCREF( args );

	tasklet->m_impl->set_arguments( args );

    Py_XINCREF( kwargs );

    tasklet->m_impl->set_kw_arguments( kwargs );

    //Initialize the tasklet
    if (!tasklet->m_impl->initialise())
    {
		tasklet->m_impl->set_arguments( nullptr );

        tasklet->m_impl->set_kw_arguments( nullptr );

		return nullptr;
    }

    //Mark alive
    tasklet->m_impl->set_alive( true );

    //Add to scheduler
    if (!tasklet->m_impl->insert())
    {
		tasklet->m_impl->set_alive( false );

        tasklet->m_impl->uninitialise();

		tasklet->m_impl->set_arguments( nullptr );

        tasklet->m_impl->set_kw_arguments( nullptr );

		return nullptr;
    }

	Py_IncRef( Py_None );

	return Py_None;
}

static PyObject*
	Tasklet_call( PyObject* callable, PyObject* args, PyObject* kwargs )
{
	if(!Tasklet_setup( callable, args, kwargs ))
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
	{ "insert",
        (PyCFunction)Tasklet_insert,
        METH_NOARGS,
        "Insert a tasklet at the end of the scheduler runnables queue." },

	{ "remove",
        (PyCFunction)Tasklet_remove,
        METH_NOARGS,
        "Remove a tasklet from the runnables queue." },

	{ "run",
        (PyCFunction)Tasklet_run,
        METH_NOARGS,
        "run immediately if in valid state." },

	{ "switch",
        (PyCFunction)Tasklet_switch,
        METH_NOARGS,
        "Pause calling tasklet and run immediately." },

	{ "throw",
        (PyCFunction)Tasklet_throw,
        METH_VARARGS | METH_KEYWORDS,
        "Raise an exception on the given tasklet. \n\n\
            :param exc: Exception to raise \n\
            :type exc: Python Exception: \n\
            :param tb: Traceback \n\
            :type tb: Python Exception traceback \n\
            :param pending: If True then throw will schedule and the caller will continue execution \n\
            :type pending: Bool \n\
            :throws: RuntimeError If attempt is made to raise exception on dead Tasklet aside from TaskletExit exceptions." },

	{ "raise_exception",
        (PyCFunction)Tasklet_raiseexception,
        METH_VARARGS,
        "Raise an exception on the given tasklet. \n\n\
            :param exc_class: Exception to raise \n\
            :type exc_class: Object sub-class of Exception \n\
            :param args: Exception arguments \n\
            :throws: RuntimeError If attempt is made to raise exception on dead Tasklet aside from TaskletExit exceptions." },

	{ "kill",
        (PyCFunction)Tasklet_kill,
        METH_VARARGS | METH_KEYWORDS,
        "Terminate the current tasklet and unblock. \n\n\
            :param pending: If True then kill will schedule the kill and continue execution\n\
            :type pending: Bool \n\
            :throws: TaskletExit on the calling Tasklet" },

	{ "set_context",
        (PyCFunction)Tasklet_setcontext,
        METH_NOARGS,
        "Set the Context object to be used while this tasklet runs." },

	{ "bind",
        (PyCFunction)Tasklet_bind,
        METH_VARARGS | METH_KEYWORDS,
        "binds a callable to a tasklet. \n\n\
            :param func: Callable to bind to Tasklet \n\
            :type func: Callable \n\
            :param args: Arguments that will be passed to callable. \n\
            :type args: Tuple \n\
            :param kwargs: Keyword arguments that will be passed ot callable \n\
            :type kwargs: Tuple" },

	{ "setup",
        (PyCFunction)Tasklet_setup,
        METH_VARARGS | METH_KEYWORDS,
        "Provide Tasklet with arguments to pass to a bound callable. \n\n\
            :param args: Arguments that will be passed to callable. \n\
            :type args: Tuple \n\
            :param kwargs: Keyword arguments that will be passed ot callable \n\
            :type kwargs: Tuple" },

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
	offsetof( PyTaskletObject, m_weakref_list ), /*tp_weaklistoffset*/
	0, /*tp_iter*/
	0, /*tp_iternext*/
	Tasklet_methods, /*tp_methods*/
	0, /*tp_members*/
	Tasklet_getsetters, /*tp_getset*/
	0, /* see PyInit_xx */ /*tp_base*/
	0, /*tp_dict*/
	0, /*tp_descr_get*/
	0, /*tp_descr_set*/
	0, /*tp_dictoffset*/
	(initproc)Tasklet_init, /*tp_init*/
	0, /*tp_alloc*/
	Tasklet_new, /*tp_new*/
	0, /*tp_free*/
	0, /*tp_is_gc*/
};

