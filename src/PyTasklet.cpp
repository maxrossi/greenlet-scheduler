#include "Tasklet.h"

#include <new>

#include "ScheduleManager.h"
#include "PyTasklet.h"
#include "Utils.h"

static PyObject* TaskletExit;

static bool PyTaskletObjectIsValid( PyTaskletObject* tasklet )
{
	if( !tasklet->m_implementation )
	{
		PyErr_SetString( PyExc_RuntimeError, "Tasklet object is not valid. Most likely cause being __init__ not called on base type." );

		return false;
	}

	return true;
}

static PyObject*
	TaskletNew( PyTypeObject* type, PyObject* args, PyObject* kwds )
{
	PyTaskletObject* self;

	self = (PyTaskletObject*)type->tp_alloc( type, 0 );

	if( self != nullptr )
	{
		self->m_implementation = nullptr;

        self->m_weakrefList = nullptr;
	}

	return (PyObject*)self;
}

static int
	TaskletInit( PyTypeObject* self, PyObject* args, PyObject* kwds )
{
	PyTaskletObject* taskletObject = reinterpret_cast<PyTaskletObject*>( self );

	taskletObject->m_implementation = (Tasklet*)PyObject_Malloc( sizeof( Tasklet ) );

    Tasklet* tasklet = taskletObject->m_implementation;

	if( !tasklet )
	{
		PyErr_SetString( PyExc_RuntimeError, "Failed to allocate memory for implementation object." );

		return -1;
	}

    PyObject* callable = nullptr;

	bool isMain = false;

	if( !PyArg_ParseTuple( args, "|Op", &callable, &isMain ) )
	{
		PyErr_SetString( PyExc_RuntimeError, "Failed while parsing arguments" );

		return -1;
	}

    try
	{
		new( tasklet ) Tasklet( reinterpret_cast<PyObject*>( self ), TaskletExit, isMain );
	}
	catch( const std::exception& ex )
	{
		PyObject_Free( tasklet );

		PyErr_SetString( PyExc_RuntimeError, ex.what() );

		return -1;
	}
	catch( ... )
	{
		PyObject_Free( tasklet );

		PyErr_SetString( PyExc_RuntimeError, "Failed to construct implementation object." );

		return -1;
	}

    // Don't pass callable if this is main tasklet
    if (!isMain)
    {
		if( !tasklet->Bind( callable, nullptr, nullptr ) )
		{
			Py_DecRef( args );
			return -1;
		}
	}
    
    return 0;
}

static void
	TaskletDealloc( PyTaskletObject* self )
{
	// Untrack garbage
	PyObject_GC_UnTrack( self );

    // Call destructor
	Tasklet* tasklet = self->m_implementation;

	if( tasklet )
    {
		tasklet->~Tasklet();

		PyObject_Free( tasklet );
    }
	
    // Handle weakrefs
	if( self->m_weakrefList != nullptr )
	{
		PyObject_ClearWeakRefs( (PyObject*)self );
	}

	Py_TYPE( self )->tp_free( (PyObject*)self );
}

static PyObject*
	TaskletAliveGet( PyTaskletObject* self, void* closure )
{
    // Ensure PyTaskletObject is in a valid state
    if (!PyTaskletObjectIsValid(self))
    {
		return nullptr;
    }

	return self->m_implementation->IsAlive() ? Py_True : Py_False;
}

static PyObject*
	TaskletBlockedGet( PyTaskletObject* self, void* closure )
{
	// Ensure PyTaskletObject is in a valid state
	if( !PyTaskletObjectIsValid( self ) )
	{
		return nullptr;
	}

	return self->m_implementation->IsBlocked() ? Py_True : Py_False;
}

static PyObject*
	TaskletScheduledGet( PyTaskletObject* self, void* closure )
{
	// Ensure PyTaskletObject is in a valid state
	if( !PyTaskletObjectIsValid( self ) )
	{
		return nullptr;
	}

	return self->m_implementation->IsScheduled() ? Py_True : Py_False;
}

static PyObject*
	TaskletBlocktrapGet( PyTaskletObject* self, void* closure )
{
	// Ensure PyTaskletObject is in a valid state
	if( !PyTaskletObjectIsValid( self ) )
	{
		return nullptr;
	}

	return self->m_implementation->IsBlocktrapped() ? Py_True : Py_False;
}

static int
	TaskletBlocktrapSet( PyTaskletObject* self, PyObject* value, void* closure )
{
	// Ensure PyTaskletObject is in a valid state
	if( !PyTaskletObjectIsValid( self ) )
	{
		return -1;
	}

	if(!PyBool_Check(value))
	{
		PyErr_SetString( PyExc_RuntimeError, "Blocktrap expects a boolean" );
		return -1;
    }

    self->m_implementation->SetBlocktrap( PyObject_IsTrue( value ) );

	return 0;
}

static PyObject*
	TaskletIsCurrentGet( PyTaskletObject* self, void* closure )
{
	// Ensure PyTaskletObject is in a valid state
	if( !PyTaskletObjectIsValid( self ) )
	{
		return nullptr;
	}

	ScheduleManager* scheduleManager = ScheduleManager::GetThreadScheduleManager();

	Tasklet* currentTasklet = scheduleManager->GetCurrentTasklet();

    scheduleManager->Decref();

	return reinterpret_cast<PyObject*>( self ) == currentTasklet->PythonObject() ? Py_True : Py_False;
}

static PyObject*
	TaskletIsMainGet( PyTaskletObject* self, void* closure )
{
	// Ensure PyTaskletObject is in a valid state
	if( !PyTaskletObjectIsValid( self ) )
	{
		return nullptr;
	}

	return self->m_implementation->IsMain() ? Py_True : Py_False;
}

static PyObject*
	TaskletThreadIdGet( PyTaskletObject* self, void* closure )
{
	// Ensure PyTaskletObject is in a valid state
	if( !PyTaskletObjectIsValid( self ) )
	{
		return nullptr;
	}

	return PyLong_FromLong( self->m_implementation->ThreadId() );
}

static PyObject*
	TaskletNextGet( PyTaskletObject* self, void* closure )
{
	// Ensure PyTaskletObject is in a valid state
	if( !PyTaskletObjectIsValid( self ) )
	{
		return nullptr;
	}

	Tasklet* next = self->m_implementation->Next();

    if( !next )
	{
		Py_IncRef( Py_None );

		return Py_None;
    }
	else
	{
		PyObject* pyNext = next->PythonObject();

        Py_IncRef( pyNext );

		return pyNext;
    }
    
}

static PyObject*
	TaskletPreviousGet( PyTaskletObject* self, void* closure )
{
	// Ensure PyTaskletObject is in a valid state
	if( !PyTaskletObjectIsValid( self ) )
	{
		return nullptr;
	}

	Tasklet* previous = self->m_implementation->Previous();

	if( !previous )
	{
		Py_IncRef( Py_None );

		return Py_None;
	}
	else
	{
		previous->Incref();

		return previous->PythonObject();
    }

}

static PyObject*
	TaskletPausedGet( PyTaskletObject* self, void* closure )
{
	// Ensure PyTaskletObject is in a valid state
	if( !PyTaskletObjectIsValid( self ) )
	{
		return nullptr;
	}

	return self->m_implementation->IsPaused() ? Py_True : Py_False;
}

static PyObject*
    TaskletFrameGet(PyTaskletObject* self, void* closure)
{
    PyErr_SetString( PyExc_RuntimeError, "frame Not implemented" );

	return nullptr;
}

static PyObject*
    TaskletMethodNameGet(PyTaskletObject* self, void* closure)
{
	// Ensure PyTaskletObject is in a valid state
	if( !PyTaskletObjectIsValid( self ) )
	{
		return nullptr;
	}

    std::string str = self->m_implementation->GetMethodName();

    return PyUnicode_FromStringAndSize( str.c_str(), str.size() );
}

static int
    TaskletMethodNameSet(PyTaskletObject* self, PyObject* value, void* closure)
{
    // Ensure PyTaskletObject is in a valid state
	if( !PyTaskletObjectIsValid( self ) )
	{
		return -1;
	}

	
	std::string cstr;

    if( !StdStringFromPyObject( value, cstr ) )
    {
		return -1;
    }

    self->m_implementation->SetMethodName( cstr );

    return 0;
}

static PyObject*
    TaskletModuleNameGet(PyTaskletObject* self, void* closure)
{
	// Ensure PyTaskletObject is in a valid state
	if( !PyTaskletObjectIsValid( self ) )
	{
		return nullptr;
	}

	std::string str = self->m_implementation->GetModuleName();

	return PyUnicode_FromStringAndSize( str.c_str(), str.size() );
}

static int
	TaskletModuleNameSet( PyTaskletObject* self, PyObject* value, void* closure )
{
	// Ensure PyTaskletObject is in a valid state
	if( !PyTaskletObjectIsValid( self ) )
	{
		return -1;
	}
	std::string cstr;

	if( !StdStringFromPyObject( value, cstr ) )
	{
		return -1;
	}

	self->m_implementation->SetModuleName( cstr );

	return 0;
}

static PyObject*
	TaskletContextGet( PyTaskletObject* self, void* closure )
{
	// Ensure PyTaskletObject is in a valid state
	if( !PyTaskletObjectIsValid( self ) )
	{
		return nullptr;
	}

	std::string str = self->m_implementation->GetContext();

	return PyUnicode_FromStringAndSize( str.c_str(), str.size() );
}

static int
    TaskletContextSet(PyTaskletObject* self, PyObject* value, void* closure)
{
	// Ensure PyTaskletObject is in a valid state
	if( !PyTaskletObjectIsValid( self ) )
	{
		return -1;
	}

	std::string cstr;

	if( !StdStringFromPyObject( value, cstr ) )
	{
		return -1;
	}

	self->m_implementation->SetContext( cstr );

	return 0;
}

static PyObject*
	TaskletFileNameGet( PyTaskletObject* self, void* closure )
{
	// Ensure PyTaskletObject is in a valid state
	if( !PyTaskletObjectIsValid( self ) )
	{
		return nullptr;
	}

	std::string str = self->m_implementation->GetFilename();

	return PyUnicode_FromStringAndSize( str.c_str(), str.size() );
}

static int
	TaskletFileNameSet( PyTaskletObject* self, PyObject* value, void* closure )
{
	// Ensure PyTaskletObject is in a valid state
	if( !PyTaskletObjectIsValid( self ) )
	{
		return -1;
	}

	std::string cstr;

	if( !StdStringFromPyObject( value, cstr ) )
	{
		return -1;
	}

	self->m_implementation->SetFilename( cstr );

	return 0;
}

static PyObject*
    TaskletLineNumberGet(PyTaskletObject* self, void* closure)
{
	// Ensure PyTaskletObject is in a valid state
	if( !PyTaskletObjectIsValid( self ) )
	{
		return nullptr;
	}

	long lineNumber = self->m_implementation->GetLineNumber();

	return PyLong_FromLong( lineNumber );
}

static int
    TaskletLineNumberSet(PyTaskletObject* self, PyObject* value, void* closure)
{
	// Ensure PyTaskletObject is in a valid state
	if( !PyTaskletObjectIsValid( self ) )
	{
		return -1;
	}

    long lineNumber = PyLong_AsLong( value );

    if (PyErr_Occurred())
    {
		return -1;
    }

    self->m_implementation->SetLineNumber( lineNumber );
    
    return 0;
}

static PyObject*
    TaskletParentCallsiteGet(PyTaskletObject* self, void* closure)
{
	// Ensure PyTaskletObject is in a valid state
	if( !PyTaskletObjectIsValid( self ) )
	{
		return nullptr;
	}

	std::string str = self->m_implementation->GetParentCallsite();

	return PyUnicode_FromStringAndSize( str.c_str(), str.size() );
}

static int
    TaskletParentCallsiteSet(PyTaskletObject* self, PyObject* value, void* closure)
{
	// Ensure PyTaskletObject is in a valid state
	if( !PyTaskletObjectIsValid( self ) )
	{
		return -1;
	}

	std::string cstr;

	if( !StdStringFromPyObject( value, cstr ) )
	{
		return -1;
	}

	self->m_implementation->SetParentCallsite( cstr );

	return 0;
}

static PyObject*
    TaskletParentMethodNameGet(PyTaskletObject* self, void* closure)
{
	// Ensure PyTaskletObject is in a valid state
	if( !PyTaskletObjectIsValid( self ) )
	{
		return nullptr;
	}

	std::string str = self->m_implementation->GetParentMethodName();

	return PyUnicode_FromStringAndSize( str.c_str(), str.size() );
}

static int
TaskletParentMethodNameSet(PyTaskletObject* self, PyObject* value, void* closure)
{
	// Ensure PyTaskletObject is in a valid state
	if( !PyTaskletObjectIsValid( self ) )
	{
		return -1;
	}

	std::string cstr;

	if( !StdStringFromPyObject( value, cstr ) )
	{
		return -1;
	}

	self->m_implementation->SetParentMethodName( cstr );

	return 0;
}

static PyObject*
TaskletStartTimeGet(PyTaskletObject* self, void* closure)
{
	// Ensure PyTaskletObject is in a valid state
	if( !PyTaskletObjectIsValid( self ) )
	{
		return nullptr;
	}

	long long startTime = self->m_implementation->GetStartTime();

	return PyLong_FromLongLong( startTime );
}

static int
	TaskletStartTimeSet( PyTaskletObject* self, PyObject* value, void* closure )
{
	// Ensure PyTaskletObject is in a valid state
	if( !PyTaskletObjectIsValid( self ) )
	{
		return -1;
	}

	long long startTime = PyLong_AsLongLong( value );

	if( PyErr_Occurred() )
	{
		return -1;
	}

	self->m_implementation->SetStartTime( startTime );

	return 0;
}

static PyObject*
	TaskletEndTimeGet( PyTaskletObject* self, void* closure )
{
	// Ensure PyTaskletObject is in a valid state
	if( !PyTaskletObjectIsValid( self ) )
	{
		return nullptr;
	}

	long long endTime = self->m_implementation->GetEndTime();

	return PyLong_FromLongLong( endTime );
}

static int
	TaskletEndTimeSet( PyTaskletObject* self, PyObject* value, void* closure )
{
	// Ensure PyTaskletObject is in a valid state
	if( !PyTaskletObjectIsValid( self ) )
	{
		return -1;
	}

	long long endTime = PyLong_AsLongLong( value );

	if( PyErr_Occurred() )
	{
		return -1;
	}

	self->m_implementation->SetStartTime( endTime );

	return 0;
}

static PyObject*
	TaskletRunTimeGet( PyTaskletObject* self, void* closure )
{
	// Ensure PyTaskletObject is in a valid state
	if( !PyTaskletObjectIsValid( self ) )
	{
		return nullptr;
	}

	double runTime = self->m_implementation->GetRunTime();
	return PyFloat_FromDouble( runTime );
}

static int
	TaskletRunTimeSet( PyTaskletObject* self, PyObject* value, void* closure )
{
	// Ensure PyTaskletObject is in a valid state
	if( !PyTaskletObjectIsValid( self ) )
	{
		return -1;
	}

	double runTime = PyFloat_AsDouble( value );

	if( PyErr_Occurred() )
	{
		return -1;
	}

	self->m_implementation->SetRunTime( runTime );

	return 0;
}

static PyObject*
    TaskletHighlightedGet( PyTaskletObject* self, void* closure )
{
	// Ensure PyTaskletObject is in a valid state
	if( !PyTaskletObjectIsValid( self ) )
	{
		return nullptr;
	}

    bool highlighted = self->m_implementation->GetHighlighted();

    return highlighted ? Py_True : Py_False;
}

static int
	TaskletHighlightedSet( PyTaskletObject* self, PyObject* value, void* closure )
{
	// Ensure PyTaskletObject is in a valid state
	if( !PyTaskletObjectIsValid( self ) )
	{
		return -1;
	}

    if ( !PyBool_Check(value) )
    {
		PyErr_SetString( PyExc_TypeError, "highlighted must be either a True or False value" );
		return -1;
    }

    bool highlighted = PyObject_IsTrue( value );

    self->m_implementation->SetHighlighted( highlighted );

    return 0;
}

static PyGetSetDef Tasklet_getsetters[] = {
	{ "alive", 
        (getter)TaskletAliveGet,
        NULL,
        "True while a tasklet is still running.",
        NULL },

	{ "blocked",
        (getter)TaskletBlockedGet,
        NULL,
        "True when a tasklet is blocked on a channel.",
        NULL },

	{ "scheduled",
        (getter)TaskletScheduledGet,
        NULL,
        "True when the tasklet is either in the runnables list or blocked on a channel.",
        NULL },

	{ "block_trap",
        (getter)TaskletBlocktrapGet,
        (setter)TaskletBlocktrapSet,
        "True while this tasklet is within a n atomic block.",
        NULL },

	{ "is_current",
        (getter)TaskletIsCurrentGet,
        NULL,
        "True if the tasklet is the current tasklet.",
        NULL },

	{ "is_main",
        (getter)TaskletIsMainGet,
        NULL,
        "True if the tasklet is the main tasklet.",
        NULL },

	{ "thread_id",
        (getter)TaskletThreadIdGet,
        NULL,
        "Id of the thread the tasklet belongs to.",
        NULL },

	{ "next",
        (getter)TaskletNextGet,
        NULL,
        "Get next tasklet in schedule queue.",
        NULL },

	{ "prev",
        (getter)TaskletPreviousGet,
        NULL,
        "Get next tasklet in schedule queue.",
        NULL },

	{ "paused",
        (getter)TaskletPausedGet,
        NULL,
        "This attribute is True when a tasklet is alive, but not scheduled or blocked on a channel.",
        NULL },

	{ "frame",
	  (getter)TaskletFrameGet,
        NULL,
	    "Get the current frame of a tasklet. Frame has been dissabled due to instability in greenlet. This attribute will always be None",
        NULL },
	{ "method_name",
	  (getter)TaskletMethodNameGet,
	  (setter)TaskletMethodNameSet,
		"(in the near future, this field will be read-only) the name of the callable bound to the tasklet.",
        NULL },
	{ "module_name",
	  (getter)TaskletModuleNameGet,
	  (setter)TaskletModuleNameSet,
	    "(in the near future, this field will be read-only) the name of the callable bound to the tasklet.",
	    NULL },
	{ "context",
	  (getter)TaskletContextGet,
	  (setter)TaskletContextSet,
	    "context of the tasklet.",
	    NULL },
	{ "file_name",
	  (getter)TaskletFileNameGet,
	  (setter)TaskletFileNameSet,
	  "(in the near future, this field will be read-only) name of the file containing callable bound to the tasklet tasklet.",
	  NULL },
	{ "line_number",
	  (getter)TaskletLineNumberGet,
	  (setter)TaskletLineNumberSet,
	  "(in the near future, this field will be read-only) line number of the callable boud to the tasklet.",
	  NULL },
	{ "parent_callsite",
	  (getter)TaskletParentCallsiteGet,
	  (setter)TaskletParentCallsiteSet,
	  "(in the near future, this field will be read-only) callsite of the parent tasklet when this tasklet was created.",
	  NULL },
	{ "parent_method_name",
	  (getter)TaskletParentMethodNameGet,
	  (setter)TaskletParentMethodNameSet,
	  "(in the near future, this field will be read-only) name of the callable bound to the parent tasklet when this tasklet was created.",
	  NULL },
	{ "startTime",
	  (getter)TaskletStartTimeGet,
	  (setter)TaskletStartTimeSet,
	  "(in the near future, this field will be read-only) Time the tasklet was started.",
	  NULL },
	{ "endTime",
	  (getter)TaskletEndTimeGet,
	  (setter)TaskletEndTimeSet,
	  "(in the near future, this field will be read-only) Time the tasklet finished.",
	  NULL },
	{ "runTime",
	  (getter)TaskletRunTimeGet,
	  (setter)TaskletRunTimeSet,
	  "(in the near future, this field will be read-only) Total time the tasklet was running for.",
	  NULL },
	{ "highlighted",
	  (getter)TaskletHighlightedGet,
	  (setter)TaskletHighlightedSet,
	  "A generic boolean marker used for tracing purposes.",
	  NULL },

	{ NULL } /* Sentinel */
};

static PyObject*
	TaskletBind( PyTaskletObject* self, PyObject* args, PyObject* kwds )
{
	// Ensure PyTaskletObject is in a valid state
	if( !PyTaskletObjectIsValid( self ) )
	{
		return nullptr;
	}

	PyObject* callable = nullptr;

	PyObject* bindArgs = nullptr;

	PyObject* bindKwArgs = nullptr;

	const char* keywords[] = { "callable", "args", "kwargs", NULL };

	if( !PyArg_ParseTupleAndKeywords( args, kwds, "|OOO:bind", (char**)keywords, &callable, &bindArgs, &bindKwArgs ) )
	{
		return nullptr;
	}

	Tasklet* tasklet = self->m_implementation;

    // Check if this is an unbind
	if( ( callable == Py_None ) && ( !bindArgs ) && ( !bindKwArgs ) )
	{
		// Unbind Tasklet
		if( !tasklet->UnBind() )
		{
			return nullptr;
		}
	}
	else
	{
		// Bind Tasklet
		if( !tasklet->Bind( callable, bindArgs, bindKwArgs ) )
		{
			return nullptr;
		}
	}

	tasklet->Incref();

	return tasklet->PythonObject();
}

static PyObject*
	TaskletInsert( PyTaskletObject* self, PyObject* Py_UNUSED( ignored ) )
{
	// Ensure PyTaskletObject is in a valid state
	if( !PyTaskletObjectIsValid( self ) )
	{
		return nullptr;
	}

    Tasklet* tasklet = self->m_implementation;

    if( !tasklet->Insert() )
    {
		return nullptr;
    }

    tasklet->Incref();

    return tasklet->PythonObject();

}

static PyObject*
	TaskletRemove( PyTaskletObject* self, PyObject* Py_UNUSED( ignored ) )
{
	// Ensure PyTaskletObject is in a valid state
	if( !PyTaskletObjectIsValid( self ) )
	{
		return nullptr;
	}

    Tasklet* tasklet = self->m_implementation;

    if( !tasklet->Remove() )
    {
		return nullptr;
    }

    // Note: tasklet is not increffed before returning as to remove the reference
    // that the schedule manager previously held.

    return tasklet->PythonObject();
}

static PyObject*
	TaskletRun( PyTaskletObject* self, void* closure )
{
	// Ensure PyTaskletObject is in a valid state
	if( !PyTaskletObjectIsValid( self ) )
	{
		return nullptr;
	}

    if( self->m_implementation->Run() )
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
	TaskletSwitch( PyTaskletObject* self, void* closure )
{
	// Ensure PyTaskletObject is in a valid state
	if( !PyTaskletObjectIsValid( self ) )
	{
		return nullptr;
	}

    if( self->m_implementation->SwitchImplementation() )
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
	CheckExceptionSetup( PyObject* exception, PyObject* arguments )
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
	TaskletThrow( PyTaskletObject* self, PyObject* args, PyObject* kwds )
{
	// Ensure PyTaskletObject is in a valid state
	if( !PyTaskletObjectIsValid( self ) )
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
	if( !CheckExceptionSetup( exception, value ) )
	{
		return nullptr;
    }

    if( self->m_implementation->ThrowException( exception, value, tb, pending ) )
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
	TaskletRaiseException( PyTaskletObject* self, PyObject* args, PyObject* kwds )
{
	// Ensure PyTaskletObject is in a valid state
	if( !PyTaskletObjectIsValid( self ) )
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
	if( !CheckExceptionSetup( exception, arguments ) )
	{
		return nullptr;
	}

    if( self->m_implementation->ThrowException( exception, arguments, Py_None, false ) )
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
	TaskletKill( PyTaskletObject* self, PyObject* args, PyObject* kwds )
{
	// Ensure PyTaskletObject is in a valid state
	if( !PyTaskletObjectIsValid( self ) )
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

	if( self->m_implementation->Kill( pending ) )
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
	TaskletSetContext( PyTaskletObject* self, void* closure )
{
	PyErr_SetString( PyExc_RuntimeError, "Tasklet_setcontext Not yet implemented" ); //TODO
	return nullptr;
}


static int
	TaskletTraverse( PyTaskletObject* self, visitproc visit, void* arg )
{
	Tasklet* tasklet = self->m_implementation;

	if( !tasklet )
	{
		return 0;
	}

	PyObject* callable = tasklet->GetCallable();

	if( callable )
	{
		Py_VISIT( callable );
	}

	PyObject* bind_args = tasklet->Arguments();

	if( bind_args )
	{
		Py_VISIT( bind_args );
	}

	return 0;
}

static int
	TaskletClear( PyTaskletObject* self )
{
	Tasklet* tasklet = self->m_implementation;

	if( !tasklet )
	{
		return 0;
	}

    tasklet->Clear();

	return 0;
}

static PyObject*
    TaskletSetup( PyObject* callable, PyObject* args, PyObject* kwargs )
{
	PyTaskletObject* Pytasklet = reinterpret_cast<PyTaskletObject*>( callable );

    // Ensure PyTaskletObject is in a valid state
	if( !PyTaskletObjectIsValid( Pytasklet ) )
	{
		return nullptr;
	}

    Tasklet* tasklet = Pytasklet->m_implementation;

    if( !tasklet->Setup( args, kwargs ) )
    {
		return nullptr;
    }
    else
    {
		tasklet->Incref();

		return tasklet->PythonObject();
    }
}

static PyMethodDef Tasklet_methods[] = {
	{ "insert",
        (PyCFunction)TaskletInsert,
        METH_NOARGS,
        "Insert a tasklet at the end of the scheduler runnables queue." },

	{ "remove",
        (PyCFunction)TaskletRemove,
        METH_NOARGS,
        "Remove a tasklet from the runnables queue." },

	{ "run",
        (PyCFunction)TaskletRun,
        METH_NOARGS,
        "run immediately if in valid state." },

	{ "switch",
        (PyCFunction)TaskletSwitch,
        METH_NOARGS,
        "Pause calling tasklet and run immediately." },

	{ "throw",
        (PyCFunction)TaskletThrow,
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
        (PyCFunction)TaskletRaiseException,
        METH_VARARGS,
        "Raise an exception on the given tasklet. \n\n\
            :param exc_class: Exception to raise \n\
            :type exc_class: Object sub-class of Exception \n\
            :param args: Exception arguments \n\
            :throws: RuntimeError If attempt is made to raise exception on dead Tasklet aside from TaskletExit exceptions." },

	{ "kill",
        (PyCFunction)TaskletKill,
        METH_VARARGS | METH_KEYWORDS,
        "Terminate the current tasklet and unblock. \n\n\
            :param pending: If True then kill will schedule the kill and continue execution\n\
            :type pending: Bool \n\
            :throws: TaskletExit on the calling Tasklet" },

	{ "set_context",
        (PyCFunction)TaskletSetContext,
        METH_NOARGS,
        "Set the Context object to be used while this tasklet runs." },

	{ "bind",
        (PyCFunction)TaskletBind,
        METH_VARARGS | METH_KEYWORDS,
        "binds a callable to a tasklet. \n\n\
            :param func: Callable to bind to Tasklet \n\
            :type func: Callable \n\
            :param args: Arguments that will be passed to callable. \n\
            :type args: Tuple \n\
            :param kwargs: Keyword arguments that will be passed ot callable \n\
            :type kwargs: Tuple" },

	{ "setup",
        (PyCFunction)TaskletSetup,
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
	(destructor)TaskletDealloc, /*tp_dealloc*/
	0, /*tp_vectorcall_offset*/
	0, /*tp_getattr*/
	0, /*tp_setattr*/
	0, /*tp_as_async*/
	0, /*tp_repr*/
	0, /*tp_as_number*/
	0, /*tp_as_sequence*/
	0, /*tp_as_mapping*/
	0, /*tp_hash*/
	TaskletSetup, /*tp_call*/
	0, /*tp_str*/
	0, /*tp_getattro*/
	0, /*tp_setattro*/
	0, /*tp_as_buffer*/
	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC, /*tp_flags*/
	PyDoc_STR( "Tasklet objects" ), /*tp_doc*/
	(traverseproc)TaskletTraverse, /*tp_traverse*/
	(inquiry)TaskletClear, /*tp_clear*/
	0, /*tp_richcompare*/
	offsetof( PyTaskletObject, m_weakrefList ), /*tp_weaklistoffset*/
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
	(initproc)TaskletInit, /*tp_init*/
	0, /*tp_alloc*/
	TaskletNew, /*tp_new*/
	0, /*tp_free*/
	0, /*tp_is_gc*/
};

