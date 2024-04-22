
#include "StdAfx.h"
#include <Python.h>
#include <Scheduler.h>

#include "InterpreterWithSchedulerModule.h"

struct SchedulerCapi : public InterpreterWithSchedulerModule{};

TEST_F( SchedulerCapi, PyScheduler_Schedule )
{
    // Create a test value
	EXPECT_EQ( PyRun_SimpleString( "testValue = [0]\n" ), 0 );

    // Create callable which schedules using c++
	EXPECT_EQ( PyRun_SimpleString( "def foo(remove):\n"
								   "   schedulertest.schedule(remove)\n"
								   "   testValue[0] = 1\n" ),
			   0 );

    // Create another callable for use in another tasklet
	EXPECT_EQ( PyRun_SimpleString( "def bar():\n"
								   "   testValue[0] = 2\n" ),
			   0 );

    // Create tasklets and run Specify no remove
	EXPECT_EQ( PyRun_SimpleString( "scheduler.tasklet(foo)(0)\n"
								   "scheduler.tasklet(bar)()\n"
								   "scheduler.run()\n" ),
			   0 );

    // Check tasklets are finished
	EXPECT_EQ( m_api->PyScheduler_GetRunCount(), 1 );

    // Check test value shows correct value
    PyObject* python_test_value_list = PyObject_GetAttrString( m_main_module, "testValue" );
	EXPECT_NE( python_test_value_list, nullptr );
    EXPECT_TRUE( PyList_Check( python_test_value_list ) );
    PyObject* python_test_value = PyList_GetItem( python_test_value_list, 0 );
	EXPECT_NE( python_test_value, nullptr );
    EXPECT_TRUE( PyLong_Check( python_test_value ) );
    EXPECT_EQ( PyLong_AsLong( python_test_value ), 1 );
    Py_XDECREF( python_test_value_list );
    Py_XDECREF( python_test_value );

    // Check remove functionality
	EXPECT_EQ( PyRun_SimpleString( "tasklet = scheduler.tasklet(foo)(1)\n"
								   "scheduler.run()\n" ),
			   0 );

    PyObject* tasklet = PyObject_GetAttrString( m_main_module, "tasklet" );
	EXPECT_NE( tasklet, nullptr );
    EXPECT_TRUE( m_api->PyTasklet_Check( tasklet ) );
    
    // Tasklet should still be alive
    EXPECT_EQ( m_api->PyTasklet_Alive( reinterpret_cast<PyTaskletObject*>( tasklet ) ), 1) ;

    // Clean
	Py_XDECREF( tasklet );
}

TEST_F( SchedulerCapi, PyScheduler_GetRunCount )
{
	int run_count = SchedulerAPI()->PyScheduler_GetRunCount();

	// Expected 1 is main
	EXPECT_EQ( run_count, 1 );

	// Create tasklet
	EXPECT_EQ( PyRun_SimpleString( "s = scheduler.tasklet(lambda: None)()\n" ), 0 );

	run_count = SchedulerAPI()->PyScheduler_GetRunCount();

	// Expected 2 (1 + new tasklet)
	EXPECT_EQ( run_count, 2 );

	// Run scheduler
	EXPECT_EQ( PyRun_SimpleString( "scheduler.run()\n" ), 0 );

	run_count = SchedulerAPI()->PyScheduler_GetRunCount();

	// 1 Expected all tasklets successfully run leaving main
	EXPECT_EQ( run_count, 1 );
}

TEST_F( SchedulerCapi, PyScheduler_GetCurrent )
{
	// Get main
	PyObject* current_tasklet = SchedulerAPI()->PyScheduler_GetCurrent();

    EXPECT_NE( current_tasklet, nullptr );

	EXPECT_TRUE( SchedulerAPI()->PyTasklet_IsMain( reinterpret_cast<PyTaskletObject*>( current_tasklet ) ) );
}

TEST_F( SchedulerCapi, PyScheduler_RunNTasklets )
{
    // Schedule 3 Tasklets and pump 1 at a time

    // Create a test value container
	EXPECT_EQ( PyRun_SimpleString( "testValue = [0]\n" ), 0 );

	// Create callable
	EXPECT_EQ( PyRun_SimpleString( "def foo(x):\n"
								   "   testValue[0] = testValue[0] + x\n" ),
			   0 );
	
    // Create three tasklets
	EXPECT_EQ( PyRun_SimpleString( "t1 = scheduler.tasklet(foo)(1)\n"
								   "t2 = scheduler.tasklet(foo)(2)\n"
								   "t3 = scheduler.tasklet(foo)(3)\n" ),
			   0 );

    // Check queue and run
	EXPECT_EQ( m_api->PyScheduler_GetRunCount(), 4 );

    // Run watchdog
	EXPECT_EQ( m_api->PyScheduler_RunNTasklets( 1 ), Py_None ); 

    EXPECT_EQ( m_api->PyScheduler_GetRunCount(), 3 );

    EXPECT_EQ( m_api->PyScheduler_RunNTasklets( 1 ), Py_None ); 

	EXPECT_EQ( m_api->PyScheduler_GetRunCount(), 2 );

    EXPECT_EQ( m_api->PyScheduler_RunNTasklets( 1 ), Py_None );

    // Check test value
	PyObject* python_test_value_list = PyObject_GetAttrString( m_main_module, "testValue" );
	EXPECT_NE( python_test_value_list, nullptr );
	EXPECT_TRUE( PyList_Check( python_test_value_list ) );
	PyObject* python_test_value = PyList_GetItem( python_test_value_list, 0 );
	EXPECT_NE( python_test_value, nullptr );
	EXPECT_TRUE( PyLong_Check( python_test_value ) );
	EXPECT_EQ( PyLong_AsLong( python_test_value ), 6 );
	Py_XDECREF( python_test_value_list );
	Py_XDECREF( python_test_value );

}

TEST_F( SchedulerCapi, PyScheduler_RunWatchdogEx )
{
	// Schedule 3 Tasklets and pump until complete

	// Create a test value container
	EXPECT_EQ( PyRun_SimpleString( "testValue = [0]\n" ), 0 );

	// Create callable
	EXPECT_EQ( PyRun_SimpleString( "def foo(x):\n"
								   "   testValue[0] = testValue[0] + x\n" ),
			   0 );

	// Create three tasklets
	EXPECT_EQ( PyRun_SimpleString( "t1 = scheduler.tasklet(foo)(1)\n"
								   "t2 = scheduler.tasklet(foo)(2)\n"
								   "t3 = scheduler.tasklet(foo)(3)\n" ),
			   0 );

	// Check queue and run
	EXPECT_EQ( m_api->PyScheduler_GetRunCount(), 4 );

	// Run watchdog until finished
    while (m_api->PyScheduler_GetRunCount() > 1)
    {
		EXPECT_EQ( m_api->PyScheduler_RunWatchdogEx( 10, 0 ), Py_None );
    }

	// Check test value
	PyObject* python_test_value_list = PyObject_GetAttrString( m_main_module, "testValue" );
	EXPECT_NE( python_test_value_list, nullptr );
	EXPECT_TRUE( PyList_Check( python_test_value_list ) );
	PyObject* python_test_value = PyList_GetItem( python_test_value_list, 0 );
	EXPECT_NE( python_test_value, nullptr );
	EXPECT_TRUE( PyLong_Check( python_test_value ) );
	EXPECT_EQ( PyLong_AsLong( python_test_value ), 6 );
	Py_XDECREF( python_test_value_list );
	Py_XDECREF( python_test_value );
}

TEST_F( SchedulerCapi, PyScheduler_SetChannelCallback )
{
	// Create a test value container
	EXPECT_EQ( PyRun_SimpleString( "testValue = [None,None,None,None]\n" ), 0 );

    // Create a channel callback
	EXPECT_EQ( PyRun_SimpleString( "def channel_callback(channel, tasklet, sending, willblock):\n"
								   "   testValue[0] = channel\n"
								   "   testValue[1] = tasklet\n"
								   "   testValue[2] = sending\n"
								   "   testValue[3] = willblock\n" ),
			   0 );

    // Get scheduler callback callable
	PyObject* callback_callable = PyObject_GetAttrString( m_main_module, "channel_callback" );
	EXPECT_NE( callback_callable, nullptr );
	EXPECT_TRUE( PyCallable_Check( callback_callable ) );

    // Set a channel callback to c-api
	EXPECT_EQ( m_api->PyScheduler_SetChannelCallback( callback_callable ), 0);

    // Create callable to send over channel
	EXPECT_EQ( PyRun_SimpleString( "def send_test():\n"
								   "   channel.send(5)\n" ),
			   0 );

    // Create and call a send action on a channel to invoke callback
	EXPECT_EQ( PyRun_SimpleString( "channel = scheduler.channel()\n"
								   "tasklet = scheduler.tasklet(send_test)()\n"
								   "scheduler.run()" ),
			   0 );

    // Retreive test values
	PyObject* python_test_value_list = PyObject_GetAttrString( m_main_module, "testValue" );
	EXPECT_NE( python_test_value_list, nullptr );
	EXPECT_TRUE( PyList_Check( python_test_value_list ) );
	PyObject* callback_channel = PyList_GetItem( python_test_value_list, 0 );
	EXPECT_NE( callback_channel, nullptr );
	PyObject* callback_tasklet = PyList_GetItem( python_test_value_list, 1 );
	EXPECT_NE( callback_tasklet, nullptr );
	PyObject* callback_sending = PyList_GetItem( python_test_value_list, 2 );
	EXPECT_NE( callback_sending, nullptr );
	PyObject* callback_willblock = PyList_GetItem( python_test_value_list, 3 );
	EXPECT_NE( callback_willblock, nullptr );

    // Check test values against expected
	EXPECT_TRUE( m_api->PyChannel_Check( callback_channel ) );

    PyObject* original_channel = PyObject_GetAttrString( m_main_module, "channel" );

    EXPECT_NE( original_channel, nullptr );

    EXPECT_EQ( callback_channel, original_channel );

    EXPECT_TRUE( m_api->PyTasklet_Check( callback_tasklet ) );

    PyObject* original_tasklet = PyObject_GetAttrString( m_main_module, "tasklet" );

    EXPECT_NE( original_tasklet, nullptr );

    EXPECT_EQ( callback_tasklet, original_tasklet );

    EXPECT_TRUE( PyBool_Check( callback_sending ) );

    EXPECT_TRUE( PyObject_IsTrue( callback_sending ) );

    EXPECT_TRUE( PyBool_Check( callback_willblock ) );

	EXPECT_TRUE( PyObject_IsTrue( callback_willblock ) );

    // Cleanup
	Py_XDECREF( python_test_value_list );
	Py_XDECREF( original_channel );
	Py_XDECREF( original_tasklet );
	Py_XDECREF( callback_channel );
	Py_XDECREF( callback_tasklet );
	Py_XDECREF( callback_sending );
	Py_XDECREF( callback_willblock );
    Py_XDECREF( callback_callable );
    
}

TEST_F( SchedulerCapi, PyScheduler_GetChannelCallback )
{
    // Create and channel callback
	EXPECT_EQ( PyRun_SimpleString( "def channel_callback(channel, tasklet, sending, willblock):\n"
								   "   pass\n" ),
			   0 );

    // Set channel callback
	EXPECT_EQ( PyRun_SimpleString( "scheduler.set_channel_callback(channel_callback)\n" ), 0 );

    // Get channel callback via c-api
	PyObject* channel_callback_capi = m_api->PyScheduler_GetChannelCallback();
	EXPECT_NE( channel_callback_capi, nullptr );

    // Get callable from python
	PyObject* channel_callback_python = PyObject_GetAttrString( m_main_module, "channel_callback" );
	EXPECT_NE( channel_callback_python, nullptr );

    // Check they match
    EXPECT_EQ( channel_callback_capi, channel_callback_python );

    // Clean
    Py_XDECREF( channel_callback_python );

}

TEST_F( SchedulerCapi, PyScheduler_SetScheduleCallback )
{

    // Create a test value container
	EXPECT_EQ( PyRun_SimpleString( "testValue = [None]\n" ), 0 );

    // Create a scheduler callback
	EXPECT_EQ( PyRun_SimpleString( "def schedule_callback(prev,next):\n"
								   "   testValue[0] = next\n" ),
			   0 );

	// Get scheduler callback callable
	PyObject* callback_callable = PyObject_GetAttrString( m_main_module, "schedule_callback" );
	EXPECT_NE( callback_callable, nullptr );
	EXPECT_TRUE( PyCallable_Check( callback_callable ) );

	// Set a scheduler callback to c-api
	EXPECT_EQ( m_api->PyScheduler_SetScheduleCallback( callback_callable ), 0 );

	// Create and run tasklet
	EXPECT_EQ( PyRun_SimpleString( "tasklet = scheduler.tasklet(lambda: None)()\n"
								   "scheduler.run()\n" ),
			   0 );

	// Get reference to the tasklet
	PyObject* tasklet = PyObject_GetAttrString( m_main_module, "tasklet" );
	EXPECT_NE( tasklet, nullptr );
	EXPECT_TRUE( m_api->PyTasklet_Check( tasklet ) );

	// Get test value
	PyObject* python_test_value_list = PyObject_GetAttrString( m_main_module, "testValue" );
	EXPECT_NE( python_test_value_list, nullptr );
	EXPECT_TRUE( PyList_Check( python_test_value_list ) );
	PyObject* python_test_value = PyList_GetItem( python_test_value_list, 0 );
	EXPECT_NE( python_test_value, nullptr );

	// Test object should be the same as the tasklet
	EXPECT_TRUE( m_api->PyTasklet_Check( python_test_value ) );

	// Should match the tasklet
	EXPECT_EQ( python_test_value, tasklet );

	// Cleanup
	Py_XDECREF( python_test_value_list );
	Py_XDECREF( python_test_value );
	Py_XDECREF( callback_callable );
	Py_XDECREF( tasklet );

	// Reset the scheduler callback
	EXPECT_EQ( m_api->PyScheduler_SetScheduleCallback( nullptr ), 0 );

}

static PyTaskletObject* s_test_from = nullptr;

static PyTaskletObject* s_test_to = nullptr;

static int fast_callback( struct PyTaskletObject* from, struct PyTaskletObject* to )
{
	s_test_from = from;

    s_test_to = to;

	return 0;
}

TEST_F( SchedulerCapi, PyScheduler_SetScheduleFastcallback )
{
	m_api->PyScheduler_SetScheduleFastCallback( fast_callback );

    // Create and run tasklet
	EXPECT_EQ( PyRun_SimpleString( "tasklet = scheduler.tasklet(lambda: None)()\n"
								   "scheduler.run()\n" ),
			   0 );

    // Get reference to the tasklet
	PyObject* tasklet = PyObject_GetAttrString( m_main_module, "tasklet" );
	EXPECT_NE( tasklet, nullptr );
	EXPECT_TRUE( m_api->PyTasklet_Check( tasklet ) );

    // Get main tasklet
	PyObject* main_tasklet = m_api->PyScheduler_GetCurrent();

    // Check values
    EXPECT_EQ( s_test_from, reinterpret_cast<PyTaskletObject*>(main_tasklet) );
	EXPECT_EQ( s_test_to, reinterpret_cast<PyTaskletObject*>( tasklet ) );

    // Clean
	Py_XDECREF( tasklet );
}