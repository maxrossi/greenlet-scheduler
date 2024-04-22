
#include "StdAfx.h"
#include <Python.h>
#include <Scheduler.h>

#include "InterpreterWithSchedulerModule.h"



struct ChannelCapi : public InterpreterWithSchedulerModule{};

    TEST_F( ChannelCapi, PyChannel_New )
    {

	    PyChannelObject* channel = m_api->PyChannel_New( m_api->PyChannelType );

        ASSERT_NE( channel, nullptr );

	    EXPECT_TRUE( m_api->PyChannel_Check( reinterpret_cast<PyObject*>( channel ) ) );
    }

    TEST_F( ChannelCapi, PyChannel_Send )
    {
	    // Test Value
	    long test_value = 101;

	    // Create a channel for use in test
	    PyChannelObject* channel = m_api->PyChannel_New( m_api->PyChannelType );

        EXPECT_NE( channel, nullptr );

	    // Create a callable which calls PyChannel_Send in c++
		EXPECT_EQ( PyRun_SimpleString( "def send_function(channel,value):\n"
									   "   schedulertest.channel_send(channel,value)\n" ),
				   0 );

	    PyObject* send_callable = PyObject_GetAttrString( m_main_module, "send_function" );
		EXPECT_NE( send_callable, nullptr );
		EXPECT_TRUE( PyCallable_Check( send_callable ) );

	    // Create tasklet with foo callable
	    PyObject* tasklet_args = PyTuple_New( 1 );
		EXPECT_NE( tasklet_args, nullptr );
		EXPECT_EQ( PyTuple_SetItem( tasklet_args, 0, send_callable ), 0 );
	    PyTaskletObject* tasklet = m_api->PyTasklet_New( m_api->PyTaskletType, tasklet_args );
		EXPECT_NE( tasklet, nullptr );
	    Py_XDECREF( tasklet_args );
		Py_XDECREF( send_callable );

	    // Setup tasklet to bind arguments and add the queue
	    PyObject* callable_args = PyTuple_New( 2 );
		EXPECT_NE( callable_args, nullptr );
		EXPECT_EQ( PyTuple_SetItem( callable_args, 0, reinterpret_cast<PyObject*>( channel ) ), 0 );
		EXPECT_EQ( PyTuple_SetItem( callable_args, 1, PyLong_FromLong( test_value ) ), 0 );
	    EXPECT_EQ(m_api->PyTasklet_Setup( tasklet, callable_args, nullptr ), 0);
	    Py_XDECREF( callable_args );

	    // Check that tasklet was scheduled
	    EXPECT_EQ( m_api->PyScheduler_GetRunCount(), 2 );

	    // Run scheduler (once)
		EXPECT_EQ( m_api->PyScheduler_RunNTasklets( 1 ), Py_None );

	    // Check balance has one waiting to send (1)
	    EXPECT_EQ( m_api->PyChannel_GetBalance( channel ), 1 );

	    // Receive on channel to unblock
	    PyObject* received = m_api->PyChannel_Receive( channel );

        EXPECT_NE( received, nullptr );

	    // Should be a long
	    EXPECT_TRUE( PyLong_Check( received ) );

	    // Should be the same value as the one passed in
	    EXPECT_EQ( PyLong_AsLong( received ), test_value );

	    // Channel balance should reset
	    EXPECT_EQ( m_api->PyChannel_GetBalance( channel ), 0 );

	    // There should be the remaining of send left on the queue
	    EXPECT_EQ( m_api->PyScheduler_GetRunCount(), 2 );

	    // Run scheduler
		EXPECT_EQ( m_api->PyScheduler_RunNTasklets( 1 ), Py_None );

	    // There should be nothing left on the queue but main
	    EXPECT_EQ( m_api->PyScheduler_GetRunCount(), 1 );

	    // Cleanup
	    Py_XDECREF( tasklet );

	    Py_XDECREF( channel );
    }

    TEST_F( ChannelCapi, PyChannel_Send_With_Killed_Tasklet )
	{
		// Test Value
		long test_value = 101;

		// Create a channel for use in test
		PyChannelObject* channel = m_api->PyChannel_New( m_api->PyChannelType );

        EXPECT_NE( channel, nullptr );

		// Create a callable which calls PyChannel_Send in c++
		EXPECT_EQ( PyRun_SimpleString( "def send_function(channel,value):\n"
									   "   schedulertest.channel_send(channel,value)\n" ),
				   0 );

		PyObject* send_callable = PyObject_GetAttrString( m_main_module, "send_function" );
		EXPECT_NE( send_callable, nullptr );
		EXPECT_TRUE( PyCallable_Check( send_callable ) );

		// Create tasklet with foo callable
		PyObject* tasklet_args = PyTuple_New( 1 );
		EXPECT_NE( tasklet_args, nullptr );
		EXPECT_EQ( PyTuple_SetItem( tasklet_args, 0, send_callable ), 0 );
		PyTaskletObject* tasklet = m_api->PyTasklet_New( m_api->PyTaskletType, tasklet_args );
		EXPECT_NE( tasklet, nullptr );
		EXPECT_TRUE( m_api->PyTasklet_Check( reinterpret_cast<PyObject*>(tasklet) ) );
		Py_XDECREF( tasklet_args );
		Py_XDECREF( send_callable );

		// Setup tasklet to bind arguments and add the queue
		PyObject* callable_args = PyTuple_New( 2 );
		EXPECT_NE( callable_args, nullptr );
		EXPECT_EQ( PyTuple_SetItem( callable_args, 0, reinterpret_cast<PyObject*>( channel ) ), 0 );
		EXPECT_EQ( PyTuple_SetItem( callable_args, 1, PyLong_FromLong( test_value ) ), 0 );
		EXPECT_EQ( m_api->PyTasklet_Setup( tasklet, callable_args, nullptr ), 0 );
		Py_XDECREF( callable_args );

		// Check that tasklet was scheduled
		EXPECT_EQ( m_api->PyScheduler_GetRunCount(), 2 );

		// Run scheduler (once)
		EXPECT_EQ( m_api->PyScheduler_RunNTasklets( 1 ), Py_None );

        // Check the test value 
        EXPECT_EQ( PyRun_SimpleString( "channel_state_test_value = schedulertest.test_value()\n" ), 0 );

		PyObject* channel_state_test_value = PyObject_GetAttrString( m_main_module, "channel_state_test_value" );
		EXPECT_NE( channel_state_test_value, nullptr );
		EXPECT_TRUE( PyLong_Check( channel_state_test_value ) );
		EXPECT_EQ( PyLong_AsLong( channel_state_test_value ), 1 ); 
        Py_XDECREF( channel_state_test_value );

        // Kill the tasklet
		EXPECT_EQ( m_api->PyTasklet_Kill( reinterpret_cast<PyTaskletObject*>( tasklet ) ), 0);

        // Check that cleanup occurred
		EXPECT_EQ( PyRun_SimpleString( "channel_state_test_value = schedulertest.test_value()\n" ), 0 );

		channel_state_test_value = PyObject_GetAttrString( m_main_module, "channel_state_test_value" );
		EXPECT_NE( channel_state_test_value, nullptr );
		EXPECT_TRUE( PyLong_Check( channel_state_test_value ) );
		EXPECT_EQ( PyLong_AsLong( channel_state_test_value ), -1 );
		Py_XDECREF( channel_state_test_value );

		// Cleanup
		Py_DECREF( tasklet );
		Py_XDECREF( channel );
	}

    TEST_F( ChannelCapi, PyChannel_Receive )
    {
	    // Test Value
	    long test_value = 101;

	    // Create channel
	    PyChannelObject* channel = m_api->PyChannel_New( m_api->PyChannelType );

        EXPECT_NE( channel, nullptr );

	    // Create callable
		EXPECT_EQ( PyRun_SimpleString( "def foo(channel, value):\n"
									   "   channel.send(value)\n" ),
				   0 );

	    PyObject* foo_callable = PyObject_GetAttrString( m_main_module, "foo" );
		EXPECT_NE( foo_callable, nullptr );
	    EXPECT_TRUE( PyCallable_Check( foo_callable ) );

	    // Create tasklet with foo callable
	    PyObject* tasklet_args = PyTuple_New( 1 );
		EXPECT_NE( tasklet_args, nullptr );
		EXPECT_EQ( PyTuple_SetItem( tasklet_args, 0, foo_callable ), 0 );
	    PyTaskletObject* tasklet = m_api->PyTasklet_New( m_api->PyTaskletType, tasklet_args );
		EXPECT_NE( tasklet, nullptr );
		EXPECT_TRUE( m_api->PyTasklet_Check( reinterpret_cast<PyObject*>( tasklet ) ) );
	    Py_XDECREF( tasklet_args );
	    Py_XDECREF( foo_callable );

	    // Setup tasklet to bind arguments and add the queue
	    PyObject* callable_args = PyTuple_New( 2 );
		EXPECT_NE( callable_args, nullptr );
		EXPECT_EQ( PyTuple_SetItem( callable_args, 0, reinterpret_cast<PyObject*>( channel ) ), 0 );
		EXPECT_EQ( PyTuple_SetItem( callable_args, 1, PyLong_FromLong( test_value ) ), 0 );
	    EXPECT_EQ( m_api->PyTasklet_Setup( tasklet, callable_args, nullptr ), 0);
	    Py_XDECREF( callable_args );

	    // Check that tasklet was scheduled
	    EXPECT_EQ( m_api->PyScheduler_GetRunCount(), 2 );

	    // Run scheduler (once)
		EXPECT_EQ( m_api->PyScheduler_RunNTasklets( 1 ), Py_None );

	    // Check balance has one waiting to send (1)
	    EXPECT_EQ( m_api->PyChannel_GetBalance( channel ), 1 );

	    // Receive on channel to unblock
	    PyObject* received = m_api->PyChannel_Receive( channel );

        EXPECT_NE( received, nullptr );

	    // Should be a long
	    EXPECT_TRUE( PyLong_Check( received ) );

	    // Should be the same value as the one passed in
	    EXPECT_EQ( PyLong_AsLong( received ), test_value );

	    // Channel balance should reset
	    EXPECT_EQ( m_api->PyChannel_GetBalance( channel ), 0 );

	    // There should be the remaining of send left on the queue
	    EXPECT_EQ( m_api->PyScheduler_GetRunCount(), 2 );

	    // Run scheduler
		EXPECT_EQ( m_api->PyScheduler_RunNTasklets( 1 ), Py_None );

	    // There should be nothing left on the queue but main
	    EXPECT_EQ( m_api->PyScheduler_GetRunCount(), 1 );

	    // Cleanup
		Py_XDECREF( received );

	    Py_XDECREF( tasklet );

	    Py_XDECREF( channel );

    }


    TEST_F( ChannelCapi, PyChannel_Receive_With_Killed_Tasklet )
	{

		// Create channel
		PyChannelObject* channel = m_api->PyChannel_New( m_api->PyChannelType );

        EXPECT_NE( channel, nullptr );

        // Set channel preference to prefer sender
		m_api->PyChannel_SetPreference( channel, 1 );

		// Create callable
		EXPECT_EQ( PyRun_SimpleString( "def foo(channel):\n"
									   "   value = schedulertest.channel_receive(channel)\n" ),
				   0 );

		PyObject* foo_callable = PyObject_GetAttrString( m_main_module, "foo" );
		EXPECT_NE( foo_callable, nullptr );
		EXPECT_TRUE( PyCallable_Check( foo_callable ) );

		// Create tasklet with foo callable
		PyObject* tasklet_args = PyTuple_New( 1 );
		EXPECT_NE( tasklet_args, nullptr );
		EXPECT_EQ( PyTuple_SetItem( tasklet_args, 0, foo_callable ), 0 );
		PyTaskletObject* tasklet = m_api->PyTasklet_New( m_api->PyTaskletType, tasklet_args );
		EXPECT_NE( tasklet, nullptr );
		EXPECT_TRUE(m_api->PyTasklet_Check(reinterpret_cast<PyObject*>(tasklet)));
		Py_XDECREF( tasklet_args );
		Py_XDECREF( foo_callable );

		// Setup tasklet to bind arguments and add the queue
		PyObject* callable_args = PyTuple_New( 1 );
		EXPECT_NE( callable_args, nullptr );
		EXPECT_EQ( PyTuple_SetItem( callable_args, 0, reinterpret_cast<PyObject*>( channel ) ), 0 );
		EXPECT_EQ(m_api->PyTasklet_Setup( tasklet, callable_args, nullptr ),0);
		Py_XDECREF( callable_args );

		// Check that tasklet was scheduled
		EXPECT_EQ( m_api->PyScheduler_GetRunCount(), 2 );

		// Run scheduler (once)
		EXPECT_EQ( m_api->PyScheduler_RunNTasklets( 1 ), Py_None );

		// Check balance has one waiting to send (1)
		EXPECT_EQ( m_api->PyChannel_GetBalance( channel ), -1 );

        // Check the test value
		EXPECT_EQ( PyRun_SimpleString( "channel_state_test_value = schedulertest.test_value()\n" ), 0 );

		PyObject* channel_state_test_value = PyObject_GetAttrString( m_main_module, "channel_state_test_value" );
		EXPECT_NE( channel_state_test_value, nullptr );
		EXPECT_TRUE( PyLong_Check( channel_state_test_value ) );
		EXPECT_EQ( PyLong_AsLong( channel_state_test_value ), 1 );
		Py_XDECREF( channel_state_test_value );

		// Kill the tasklet
		EXPECT_EQ( m_api->PyTasklet_Kill( reinterpret_cast<PyTaskletObject*>( tasklet ) ), 0);

		// Check that cleanup occurred
		EXPECT_EQ( PyRun_SimpleString( "channel_state_test_value = schedulertest.test_value()\n" ), 0 );

		channel_state_test_value = PyObject_GetAttrString( m_main_module, "channel_state_test_value" );
		EXPECT_NE( channel_state_test_value, nullptr );
		EXPECT_TRUE( PyLong_Check( channel_state_test_value ) );
		EXPECT_EQ( PyLong_AsLong( channel_state_test_value ), -1 );
		Py_XDECREF( channel_state_test_value );

		// Cleanup
		Py_XDECREF( tasklet );

		Py_XDECREF( channel );
	}


    TEST_F( ChannelCapi, PyChannel_SendException )
    {
	    // Create a channel for use in test
		EXPECT_EQ( PyRun_SimpleString( "channel = scheduler.channel()\n" ), 0 );

	    // Create callable which sends an exception over a channel
		EXPECT_EQ( PyRun_SimpleString( "def foo():\n"
									   "   schedulertest.send_exception(channel, ValueError)\n" ),
				   0 );

	    // Create a tasklet and run
		EXPECT_EQ( PyRun_SimpleString( "scheduler.tasklet(foo)()\n"
									   "scheduler.run()\n" ),
				   0 );

	    // Get channel
	    PyObject* channel = PyObject_GetAttrString( m_main_module, "channel" );
		EXPECT_NE( channel, nullptr );
	    EXPECT_TRUE( m_api->PyChannel_Check( channel ) );

	    // Attempt to receive on channel
		EXPECT_EQ( m_api->PyChannel_Receive( reinterpret_cast<PyChannelObject*>( channel ) ), nullptr );

	    // Check error has been raised
	    PyObject* error_type = PyErr_Occurred();

	    // Should be of type value error
	    EXPECT_EQ( error_type, PyExc_ValueError );

	    // Clear the error
	    PyErr_Clear();

	    Py_XDECREF( channel );
    }

    //NOTE: failure due to channel queue not yet implemented
    TEST_F( ChannelCapi, PyChannel_GetQueue )
    {

	    // Create channel in python
		EXPECT_EQ( PyRun_SimpleString( "send_channel = scheduler.channel()\n" ), 0 );

	    PyObject* send_channel = PyObject_GetAttrString( m_main_module, "send_channel" );
		EXPECT_NE( send_channel, nullptr );
	    EXPECT_TRUE( m_api->PyChannel_Check( send_channel ) );

	    // Create blocking function
		EXPECT_EQ( PyRun_SimpleString( "def send_block():\n"
									   "   send_channel.send(1)\n" ),
				   0 );

	    // Create and run tasklet to block on send on channel
		EXPECT_EQ( PyRun_SimpleString( "send_tasklet = scheduler.tasklet(send_block)()\n"
									   "scheduler.run()\n" ),
				   0 );

	    PyObject* blocked_tasklet = m_api->PyChannel_GetQueue( reinterpret_cast<PyChannelObject*>( send_channel ) );
		EXPECT_NE( blocked_tasklet, nullptr );
	    EXPECT_TRUE( m_api->PyTasklet_Check( blocked_tasklet ) );

	    // Check the tasklet matches expected
	    PyObject* send_tasklet = PyObject_GetAttrString( m_main_module, "send_tasklet" );
		EXPECT_NE( send_tasklet, nullptr );
	    EXPECT_EQ( blocked_tasklet, send_tasklet );

	    Py_XDECREF( send_channel );
	    Py_XDECREF( send_tasklet );
    }

    TEST_F( ChannelCapi, PyChannel_SetPreference )
    {
	    PyChannelObject* channel = m_api->PyChannel_New( m_api->PyChannelType );

        EXPECT_NE( channel, nullptr );

	    // Check default - prefer receiver -1
	    EXPECT_EQ( m_api->PyChannel_GetPreference( channel ), -1 );

	    // Set prefer neither
	    m_api->PyChannel_SetPreference( channel, 0 );

	    EXPECT_EQ( m_api->PyChannel_GetPreference( channel ), 0 );

	    // Set prefer sender
	    m_api->PyChannel_SetPreference( channel, 1 );

	    EXPECT_EQ( m_api->PyChannel_GetPreference( channel ), 1 );

	    // Check out of bounds values
	    m_api->PyChannel_SetPreference( channel, -2 );

	    EXPECT_EQ( m_api->PyChannel_GetPreference( channel ), -1 );

	    m_api->PyChannel_SetPreference( channel, 2 );

	    EXPECT_EQ( m_api->PyChannel_GetPreference( channel ), 1 );

	    Py_XDECREF( channel );
    }

    TEST_F( ChannelCapi, PyChannel_GetBalance )
    {
	    // Create channel in python
		EXPECT_EQ( PyRun_SimpleString( "send_channel = scheduler.channel()\n" ), 0 );

	    PyObject* send_channel = PyObject_GetAttrString( m_main_module, "send_channel" );
		EXPECT_NE( send_channel, nullptr );

	    EXPECT_TRUE( m_api->PyChannel_Check( send_channel ) );

	    // Check default
	    EXPECT_EQ( m_api->PyChannel_GetBalance( reinterpret_cast<PyChannelObject*>( send_channel ) ), 0 );

	    // Create blocking function
		EXPECT_EQ( PyRun_SimpleString( "def send_block():\n"
									   "   send_channel.send(1)\n" ),
				   0 );

	    // Create and run tasklet to block on send on channel
		EXPECT_EQ( PyRun_SimpleString( "scheduler.tasklet(send_block)()\n"
									   "scheduler.run()\n" ),
				   0 );

	    // Balance with one blocking send
	    EXPECT_EQ( m_api->PyChannel_GetBalance( reinterpret_cast<PyChannelObject*>( send_channel ) ), 1 );

	    Py_XDECREF( send_channel );

	    // Check blocking receive
		EXPECT_EQ( PyRun_SimpleString( "receive_channel = scheduler.channel()\n"
									   "receive_channel.preference = 1\n" ),
				   0 );

	    PyObject* receive_channel = PyObject_GetAttrString( m_main_module, "receive_channel" );
		EXPECT_NE( receive_channel, nullptr );

	    EXPECT_TRUE( m_api->PyChannel_Check( receive_channel ) );

	    // Check default
	    EXPECT_EQ( m_api->PyChannel_GetBalance( reinterpret_cast<PyChannelObject*>( receive_channel ) ), 0 );

	    // Create blocking function
		EXPECT_EQ( PyRun_SimpleString( "def receive_block():\n"
									   "   receive_channel.receive()\n" ),
				   0 );

	    // Create and run tasklet to block on send on channel
		EXPECT_EQ( PyRun_SimpleString( "scheduler.tasklet(receive_block)()\n"
									   "scheduler.run()\n" ),
				   0 );

	    // Balance with one blocking send
	    EXPECT_EQ( m_api->PyChannel_GetBalance( reinterpret_cast<PyChannelObject*>( receive_channel ) ), -1 );

	    Py_XDECREF( receive_channel );
    }

    TEST_F( ChannelCapi, PyChannel_Check )
    {
	    PyChannelObject* channel = m_api->PyChannel_New( m_api->PyChannelType );

        EXPECT_NE( channel, nullptr );

	    EXPECT_TRUE( m_api->PyChannel_Check( reinterpret_cast<PyObject*>( channel ) ) );

	    EXPECT_FALSE( m_api->PyChannel_Check( nullptr ) );

	    EXPECT_FALSE( m_api->PyChannel_Check( m_scheduler_module ) );

	    Py_XDECREF( channel );
    }

