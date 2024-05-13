
#include "StdAfx.h"
#include <Python.h>
#include <Scheduler.h>

#include "InterpreterWithSchedulerModule.h"

struct TaskletCapi : public InterpreterWithSchedulerModule{};


TEST_F( TaskletCapi, PyTasklet_New )
{
	// Create callable
	EXPECT_EQ( PyRun_SimpleString( "def foo():\n"
								   "   print(\"bar\")\n" ),
			   0 );
	PyObject* foo_callable = PyObject_GetAttrString( m_main_module, "foo" );
	EXPECT_NE( foo_callable, nullptr );
	EXPECT_TRUE( PyCallable_Check( foo_callable ) );

    // Create tasklet
    PyObject* tasklet_args = PyTuple_New( 1 );
	EXPECT_NE( tasklet_args, nullptr );
	EXPECT_EQ(PyTuple_SetItem( tasklet_args, 0, foo_callable ),0);
	PyTaskletObject* tasklet = m_api->PyTasklet_New( m_api->PyTaskletType, tasklet_args );
	EXPECT_NE( tasklet, nullptr );

    // Check type
    EXPECT_TRUE( m_api->PyTasklet_Check( reinterpret_cast<PyObject*>( tasklet ) ) );

    // Clean
	Py_XDECREF( tasklet );
    Py_XDECREF( tasklet_args );
	Py_XDECREF( foo_callable );

}

TEST_F( TaskletCapi, PyTasklet_Setup )
{
	long test_value = 101;

    // Create test value container
	EXPECT_EQ( PyRun_SimpleString( "testValue = [0]\n" ), 0 );

	// Create callable
	EXPECT_EQ( PyRun_SimpleString( "def foo(x):\n"
								   "   testValue[0] = x\n" ),
			   0 );
	PyObject* foo_callable = PyObject_GetAttrString( m_main_module, "foo" );
	EXPECT_NE( foo_callable, nullptr );
	EXPECT_TRUE( PyCallable_Check( foo_callable ) );

	// Create tasklet
	PyObject* tasklet_args = PyTuple_New( 1 );
	EXPECT_NE( tasklet_args, nullptr );
	EXPECT_EQ( PyTuple_SetItem( tasklet_args, 0, foo_callable ), 0 );
	PyTaskletObject* tasklet = m_api->PyTasklet_New( m_api->PyTaskletType, tasklet_args );
	EXPECT_NE( tasklet, nullptr );

	// Check type
	EXPECT_TRUE( m_api->PyTasklet_Check( reinterpret_cast<PyObject*>( tasklet ) ) );

    // Should not be added to queue yet
    EXPECT_EQ( m_api->PyScheduler_GetRunCount(), 1 );

    // Setup tasklet
	PyObject* callable_args = PyTuple_New( 1 );
	EXPECT_NE( callable_args, nullptr );
	EXPECT_EQ( PyTuple_SetItem( callable_args, 0, PyLong_FromLong( test_value ) ), 0 );
	EXPECT_EQ( m_api->PyTasklet_Setup( tasklet, callable_args, nullptr ), 0 );
	Py_XDECREF( callable_args );

    // Should be added to queue
    EXPECT_EQ( m_api->PyScheduler_GetRunCount(), 2 );

    // Run scheduler to run tasklet
	EXPECT_EQ( m_api->PyScheduler_RunNTasklets( 1 ), Py_None);

    // Should only be main tasklet remaining
    EXPECT_EQ( m_api->PyScheduler_GetRunCount(), 1 );

    // Retreive test value to check argument was setup correctly
	PyObject* python_test_value_list = PyObject_GetAttrString( m_main_module, "testValue" );
	EXPECT_NE( python_test_value_list, nullptr );
	EXPECT_TRUE( PyList_Check( python_test_value_list ) );
	PyObject* python_test_value = PyList_GetItem( python_test_value_list, 0 );
	EXPECT_NE( python_test_value, nullptr );
	EXPECT_TRUE( PyLong_Check( python_test_value ) );
	EXPECT_EQ( PyLong_AsLong( python_test_value ), test_value );
	Py_XDECREF( python_test_value_list );
	Py_XDECREF( python_test_value );
    
	// Clean
	Py_XDECREF( tasklet );
	Py_XDECREF( tasklet_args );
	Py_XDECREF( foo_callable );
}

TEST_F( TaskletCapi, PyTasklet_Insert )
{
    // Create a test value
	EXPECT_EQ( PyRun_SimpleString( "testValue = [0]\n" ), 0 );

	// Create callable which schedule removes a tasklet
	EXPECT_EQ( PyRun_SimpleString( "def foo():\n"
								   "   scheduler.schedule_remove()\n"
								   "   testValue[0] = 1\n" ),
			   0 );

    // Create tasklets and run tasklet
	EXPECT_EQ( PyRun_SimpleString( "tasklet = scheduler.tasklet(foo)()\n"
								   "scheduler.run()\n" ),
			   0 );

    // test value should be unchanged
	PyObject* python_test_value_list = PyObject_GetAttrString( m_main_module, "testValue" );
	EXPECT_NE( python_test_value_list, nullptr );
	EXPECT_TRUE( PyList_Check( python_test_value_list ) );
	PyObject* python_test_value = PyList_GetItem( python_test_value_list, 0 );
	EXPECT_NE( python_test_value, nullptr );
	EXPECT_TRUE( PyLong_Check( python_test_value ) );
	EXPECT_EQ( PyLong_AsLong( python_test_value ), 0 );
	Py_XDECREF( python_test_value_list );
	Py_XDECREF( python_test_value );

    // Tasklet is now alive and removed from queue
	PyObject* tasklet = PyObject_GetAttrString( m_main_module, "tasklet" );
	EXPECT_NE( tasklet, nullptr );
	EXPECT_TRUE( m_api->PyTasklet_Check( tasklet ) );

    // Now insert the tasklet back into the queue
	EXPECT_EQ( m_api->PyTasklet_Insert( reinterpret_cast<PyTaskletObject*>( tasklet ) ), 0);

    // Queue should now show tasklet has been added
	EXPECT_EQ( m_api->PyScheduler_GetRunCount(), 2 );

    // Run the queue
	EXPECT_EQ( PyRun_SimpleString( "scheduler.run()\n" ), 0 );

    // All tasklets should have run
    EXPECT_EQ( m_api->PyScheduler_GetRunCount(), 1 );

    // Test value should now be set to 1
	python_test_value_list = PyObject_GetAttrString( m_main_module, "testValue" );
	EXPECT_NE( python_test_value_list, nullptr );
	EXPECT_TRUE( PyList_Check( python_test_value_list ) );
	python_test_value = PyList_GetItem( python_test_value_list, 0 );
	EXPECT_NE( python_test_value, nullptr );
	EXPECT_TRUE( PyLong_Check( python_test_value ) );
	EXPECT_EQ( PyLong_AsLong( python_test_value ), 1 );
	Py_XDECREF( python_test_value_list );
	Py_XDECREF( python_test_value );

    // Test adding in dead tasklet
	EXPECT_EQ( m_api->PyTasklet_Alive( reinterpret_cast<PyTaskletObject*>( tasklet ) ), 0 );

    EXPECT_EQ( m_api->PyTasklet_Insert( reinterpret_cast<PyTaskletObject*>( tasklet ) ), -1 );

    // Clean
	Py_XDECREF( tasklet );
}

TEST_F( TaskletCapi, PyTasklet_Check )
{
    // Create a callable
	EXPECT_EQ( PyRun_SimpleString( "def foo(x):\n"
								   "   testValue[0] = x\n" ),
			   0 );
	PyObject* foo_callable = PyObject_GetAttrString( m_main_module, "foo" );
	EXPECT_NE( foo_callable, nullptr );
	EXPECT_TRUE( PyCallable_Check( foo_callable ) );

    PyObject* tasklet_args = PyTuple_New( 1 );
	EXPECT_NE( tasklet_args, nullptr );
	EXPECT_EQ( PyTuple_SetItem( tasklet_args, 0, foo_callable ), 0 );
	PyTaskletObject* tasklet = m_api->PyTasklet_New( m_api->PyTaskletType, tasklet_args );
	EXPECT_NE( tasklet, nullptr );
	Py_XDECREF( tasklet_args );

	EXPECT_TRUE( m_api->PyTasklet_Check( reinterpret_cast<PyObject*>( tasklet ) ) );

	EXPECT_FALSE( m_api->PyTasklet_Check( nullptr ) );

	EXPECT_FALSE( m_api->PyTasklet_Check( m_scheduler_module ) );

    Py_XDECREF( tasklet );
	Py_XDECREF( foo_callable );
}

TEST_F( TaskletCapi, PyTasklet_GetBlockTrap )
{
    // Create a tasklet
	EXPECT_EQ( PyRun_SimpleString( "tasklet = scheduler.tasklet(lambda: None)\n" ), 0 );
	PyObject* tasklet = PyObject_GetAttrString( m_main_module, "tasklet" );
	EXPECT_NE( tasklet, nullptr );
	EXPECT_TRUE( m_api->PyTasklet_Check( tasklet ) );
	
    // Check default blocktrap
	EXPECT_EQ( m_api->PyTasklet_GetBlockTrap( reinterpret_cast<PyTaskletObject*>(tasklet) ), 0 );

    EXPECT_EQ( PyRun_SimpleString( "tasklet.block_trap = True\n" ), 0 );

    // Check default blocktrap
	EXPECT_EQ( m_api->PyTasklet_GetBlockTrap( reinterpret_cast<PyTaskletObject*>( tasklet ) ), 1 );

    Py_XDECREF( tasklet );

}

TEST_F( TaskletCapi, PyTasklet_IsMain )
{
	PyObject* main_tasklet = m_api->PyScheduler_GetCurrent();

    EXPECT_NE( main_tasklet, nullptr );

    EXPECT_TRUE( m_api->PyTasklet_Check( main_tasklet ) );

    EXPECT_TRUE( m_api->PyTasklet_IsMain( reinterpret_cast<PyTaskletObject*>(main_tasklet) ) );

    Py_DecRef( main_tasklet );
	
    // Create tasklet
	EXPECT_EQ( PyRun_SimpleString( "tasklet = scheduler.tasklet(lambda: None)\n" ), 0 );
	PyObject* tasklet = PyObject_GetAttrString( m_main_module, "tasklet" );
	EXPECT_NE( tasklet, nullptr );
	EXPECT_TRUE( m_api->PyTasklet_Check( tasklet ) );
    EXPECT_FALSE( m_api->PyTasklet_IsMain( reinterpret_cast<PyTaskletObject*>( tasklet ) ) );
    
    EXPECT_EQ( PyRun_SimpleString( "tasklet = None\n" ), 0 );
	Py_XDECREF( tasklet );
    
}

TEST_F( TaskletCapi, PyTasklet_Alive )
{

	EXPECT_EQ( PyRun_SimpleString( "tasklet = scheduler.tasklet(lambda: None)()\n" ), 0 );
	PyObject* tasklet = PyObject_GetAttrString( m_main_module, "tasklet" );
	EXPECT_NE( tasklet, nullptr );
	EXPECT_TRUE( m_api->PyTasklet_Check( tasklet ) );

    // Tasklet should be alive as it hasn't been run yet
    EXPECT_EQ( m_api->PyTasklet_Alive( reinterpret_cast<PyTaskletObject*>( tasklet ) ), 1 );

    // Run Queue
	EXPECT_EQ( PyRun_SimpleString( "scheduler.run()\n" ), 0 );

    // Tasklet should be dead
    EXPECT_EQ( m_api->PyTasklet_Alive( reinterpret_cast<PyTaskletObject*>( tasklet ) ), 0 );

    // Clean
	Py_XDECREF( tasklet );

}

TEST_F( TaskletCapi, PyTasklet_Kill )
{
	EXPECT_EQ( PyRun_SimpleString( "tasklet = scheduler.tasklet(lambda: None)()\n" ), 0 );
	PyObject* tasklet = PyObject_GetAttrString( m_main_module, "tasklet" );
	EXPECT_NE( tasklet, nullptr );
	EXPECT_TRUE( m_api->PyTasklet_Check( tasklet ) );

    // Check tasklet was added to run queue
	EXPECT_EQ( m_api->PyScheduler_GetRunCount(), 2 );

    // Kill Tasklet
    EXPECT_EQ( m_api->PyTasklet_Kill( reinterpret_cast<PyTaskletObject*>( tasklet ) ), 0);

    // Check tasklet was removed from queue
	EXPECT_EQ( m_api->PyScheduler_GetRunCount(), 1 );

    // Clean
	Py_XDECREF( tasklet );
}