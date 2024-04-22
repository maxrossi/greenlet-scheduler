#include "InterpreterWithSchedulerModule.h"

#include "StdAfx.h"

// Include build config specific paths
#define CONCATENATE_DIRECT( s1, s2, s3 ) s1##s2##s3
#define STRING_DIRECT( s ) #s
#define STRING( s ) STRING_DIRECT( s )
#define CONCATENATE_TO_STRING( s1, s2, s3 ) STRING( CONCATENATE_DIRECT( s1, s2, s3 ) )
#define MODULE_PATH_INCLUDE CONCATENATE_TO_STRING( PackagePaths, CCP_BUILD_FLAVOR, .h )
#include MODULE_PATH_INCLUDE

static SchedulerCAPI* s_scheduler_api = nullptr;
static int s_test_value = 0;

static PyObject*
	schedulertest_channel_send( PyObject* self, PyObject* args )
{
	PyObject* channel = Py_None;

	PyObject* value = Py_None;

    if( !PyArg_ParseTuple( args, "OO", &channel, &value ) )
	{
		return NULL;
    }

    if (!s_scheduler_api->PyChannel_Check(channel))
    {
		return NULL;
    }

    Py_IncRef( value );

    s_test_value = 1; // Increment test value

    int ret_val = s_scheduler_api->PyChannel_Send( reinterpret_cast<PyChannelObject*>(channel), value );

    s_test_value = ret_val; // Will be -1 if failed (eg tasklet killed)

    Py_DecRef( value );
		
    return ret_val == 0 ? Py_None : nullptr;
}

static PyObject*
	schedulertest_channel_receive( PyObject* self, PyObject* args )
{
	PyObject* channel = Py_None;

	if( !PyArg_ParseTuple( args, "O", &channel ) )
	{
		return NULL;
	}

	if( !s_scheduler_api->PyChannel_Check( channel ) )
	{
		return NULL;
	}

	s_test_value = 1; // Increment test value

	PyObject* ret_val = s_scheduler_api->PyChannel_Receive( reinterpret_cast<PyChannelObject*>( channel ) );

    s_test_value = ret_val ? 0 : -1;

	return ret_val;
}

static PyObject*
	schedulertest_schedule( PyObject* self, PyObject* args )
{
	int remove = -1;

	if( !PyArg_ParseTuple( args, "i", &remove ) )
	{
		return NULL;
	}

    return s_scheduler_api->PyScheduler_Schedule( nullptr, remove );
}

static PyObject*
	schedulertest_test_value( PyObject* self, PyObject* args )
{
	return PyLong_FromLong( s_test_value );
}

static PyObject*
	schedulertest_send_exception( PyObject* self, PyObject* args )
{
	PyObject* channel = Py_None;

	PyObject* klass = Py_None;

    PyObject* value = Py_None;

	if( !PyArg_ParseTuple( args, "O|OO", &channel, &klass, &value ) )
	{
		return NULL;
	}

    if (s_scheduler_api->PyChannel_SendException(reinterpret_cast<PyChannelObject*>(channel), klass, value) == 0)
    {
		return Py_None;
    }
    else
    {
		return NULL;
    }
}

static PyMethodDef SchedulerTestMethods[] = {
	{ "channel_send", schedulertest_channel_send, METH_VARARGS, "TODO" },
	{ "channel_receive", schedulertest_channel_receive, METH_VARARGS, "TODO" },
	{ "schedule", schedulertest_schedule, METH_VARARGS, "Schedules using c-api PyScheduler_Schedule" },
	{ "send_exception", schedulertest_send_exception, METH_VARARGS, "Sends and exception using c-api PyScheduler_Schedule" },
	{ "test_value", schedulertest_test_value, METH_VARARGS, "Returns the current state of the test value" },
	{ NULL, NULL, 0, NULL }
};

static PyModuleDef SchedulerTestModule = {
	PyModuleDef_HEAD_INIT,
	"schedulertest",
	NULL,
	-1,
	SchedulerTestMethods,
	NULL,
	NULL,
	NULL,
	NULL
};

static PyObject*
	PyInit_schedulertest( void )
{
	return PyModule_Create( &SchedulerTestModule );
}

void InterpreterWithSchedulerModule::SetUp()
{

	PyConfig config;

	PyConfig_InitPythonConfig( &config );

	PyStatus status;

	const char* program_name = "SchedulerCapiTest";

	status = PyConfig_SetBytesString( &config, &config.program_name, program_name );
	if( PyStatus_Exception( status ) )
	{
		PyErr_Print();
		exit( -1 );
	}

	/* Read all configuration at once */
	status = PyConfig_Read( &config );
	if( PyStatus_Exception( status ) )
	{
		PyErr_Print();
		exit( -1 );
	}

	// Setup search paths
	status = PyWideStringList_Append( &config.module_search_paths, SCHEDULER_MODULE_PATH.c_str() );
	if( PyStatus_Exception( status ) )
	{
		PyErr_Print();
		exit( -1 );
	}

	status = PyWideStringList_Append( &config.module_search_paths, SCHEDULER_CEXTENSION_MODULE_PATH.c_str() );
	if( PyStatus_Exception( status ) )
	{
		PyErr_Print();
		exit( -1 );
	}

	status = PyWideStringList_Append( &config.module_search_paths, STDLIB_PATH.c_str() );
	if( PyStatus_Exception( status ) )
	{
		PyErr_Print();
		exit( -1 );
	}

	status = PyWideStringList_Append( &config.module_search_paths, GREENLET_CEXTENSION_MODULE_PATH.c_str() );
	if( PyStatus_Exception( status ) )
	{
		PyErr_Print();
		exit( -1 );
	}

	status = PyWideStringList_Append( &config.module_search_paths, GREENLET_MODULE_PATH.c_str() );
	if( PyStatus_Exception( status ) )
	{
		PyErr_Print();
		exit( -1 );
	}

    //Set environment variable for build flavor
	if(putenv( BUILDFLAVOR.c_str() ) == -1)
	{
		std::cout << "Failed to set build environment variable" << std::endl;
		exit( -1 );
    }

	// Add to search paths
	config.module_search_paths_set = 1;

	// Allow environment variables
	config.use_environment = 0;

    // Reset test value
    s_test_value = 0;

    // Import extension used in tests
	PyImport_AppendInittab( "schedulertest", &PyInit_schedulertest );

	status = Py_InitializeFromConfig( &config );

	if( PyStatus_Exception( status ) )
	{
		PyErr_Print();
		exit( -1 );
	}
	PyConfig_Clear( &config );


	// Import scheduler
	m_scheduler_module = PyImport_ImportModule( "scheduler" );

	if( !m_scheduler_module )
	{
		PyErr_Print();
		PySys_WriteStdout( "Failed to import scheduler module\n" );
		exit( -1 );
	}

    // Import capsule
	m_api = SchedulerAPI();

	if( m_api == nullptr )
	{
		PySys_WriteStdout( "Failed to import scheduler capsule\n" );
		PyErr_Print();
		exit( -1 );
	}

    // Store api for use in tests
    s_scheduler_api = m_api;

    // Import scheduler ready for use in tests
    if (PyRun_SimpleString("import scheduler\n") != 0)
    {
		PyErr_Print();
		exit( -1 );
    }

    // Import extension module ready for use in tests
	if( PyRun_SimpleString( "import schedulertest\n" ) != 0 )
	{
		PyErr_Print();
		exit( -1 );
	}

    // Import for use in tests
    m_main_module = PyImport_AddModule( "__main__" );
	if( m_main_module == nullptr )
	{
		PyErr_Print();
		exit( -1 );
    }
}

void InterpreterWithSchedulerModule::TearDown()
{
	
	Py_DecRef( m_scheduler_module );
	
	if( Py_FinalizeEx() < 0 )
	{
		PyErr_Print();
		exit( -1 );
	}
}