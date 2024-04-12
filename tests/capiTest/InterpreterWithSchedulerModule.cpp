#include "InterpreterWithSchedulerModule.h"

#include "StdAfx.h"

#include <StringConversions.h>
#include <Scheduler.h>

// Remove warning from getenv on MSVC
#pragma warning( disable : 4996 )

void InterpreterWithSchedulerModule::SetUp()
{
    //TODO break out logic into separate functions

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
	char* scheduler_module_path = std::getenv( "SCHEDULER_MODULE_PATH" );

	if( !scheduler_module_path )
	{
		std::cout << "SCHEDULER_MODULE_PATH environment variable not set" << std::endl; //TODO is there a better way to do this with gtest?
		exit( -1 );
	}
	else
	{
		PyWideStringList_Append( &config.module_search_paths, UTF8ToWide( scheduler_module_path ).c_str() );
	}

	char* scheduler_cextension_path = std::getenv( "SCHEDULER_CEXTENSION_MODULE_PATH" );

	if( !scheduler_cextension_path )
	{
		std::cout << "SCHEDULER_CEXTENSION_MODULE_PATH environment variable not set" << std::endl; //TODO is there a better way to do this with gtest?
		exit( -1 );
	}
	else
	{
		PyWideStringList_Append( &config.module_search_paths, UTF8ToWide( scheduler_cextension_path ).c_str() );
	}

	char* stdlib_path = std::getenv( "STDLIB_PATH" );

	if( !stdlib_path )
	{
		std::cout << "STDLIB_PATH environment variable not set" << std::endl; //TODO is there a better way to do this with gtest?
		exit( -1 );
	}
	else
	{
		PyWideStringList_Append( &config.module_search_paths, UTF8ToWide( stdlib_path ).c_str() );
	}

	char* greenlet_cextension_path = std::getenv( "GREENLET_CEXTENSION_MODULE_PATH" );

	if( !greenlet_cextension_path )
	{
		std::cout << "GREENLET_CEXTENSION_MODULE_PATH environment variable not set" << std::endl; //TODO is there a better way to do this with gtest?
		exit( -1 );
	}
	else
	{
		PyWideStringList_Append( &config.module_search_paths, UTF8ToWide( greenlet_cextension_path ).c_str() );
	}

	char* greenlet_module_path = std::getenv( "GREENLET_MODULE_PATH" );

	if( !greenlet_module_path )
	{
		std::cout << "GREENLET_MODULE_PATH environment variable not set" << std::endl; //TODO is there a better way to do this with gtest?
		exit( -1 );
	}
	else
	{
		PyWideStringList_Append( &config.module_search_paths, UTF8ToWide( greenlet_module_path ).c_str() );
	}


	// Add to search paths
	config.module_search_paths_set = 1;

	// Allow environment variables
	config.use_environment = 1;


	status = Py_InitializeFromConfig( &config );

	if( PyStatus_Exception( status ) )
	{
		PyErr_Print();
		exit( -1 );
	}
	PyConfig_Clear( &config );


	// Import scheduler
	PySys_WriteStdout( "Importing scheduler \n" );
	PyObject* scheduler_module = PyImport_ImportModule( "scheduler" );

	if( !scheduler_module )
	{
		PyErr_Print();
		PySys_WriteStdout( "Failed to import scheduler module\n" );
		exit( -1 );
	}

	PySys_WriteStdout( "Importing capsule \n" );

	auto api = SchedulerAPI();

	if( api == nullptr )
	{
		PySys_WriteStdout( "Failed to import scheduler capsule\n" );
		PyErr_Print();
		exit( -1 );
	}
}

void InterpreterWithSchedulerModule::TearDown()
{
	if( Py_FinalizeEx() < 0 )
	{
		PyErr_Print();
		exit( 120 );
	}
}