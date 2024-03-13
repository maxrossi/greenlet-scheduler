#include "PyChannel.h"

#include "PyTasklet.h"

#include "PyScheduler.h"

PyChannelObject::PyChannelObject():
	m_balance(0)
{

}

bool PyChannelObject::send( PyObject* args, bool exception /* = false */)
{
    PyThread_acquire_lock( m_lock, 1 );

    PyObject* current = Scheduler::get_current_tasklet();  //TODO naming clean up

    run_channel_callback( reinterpret_cast<PyObject*>(this), current, true, m_first_tasklet_waiting_to_receive == Py_None );  //TODO will_block logic here will change with addition of preference

    reinterpret_cast<PyTaskletObject*>( current )->m_transfer_in_progress = true;

	if( m_first_tasklet_waiting_to_receive == Py_None )
	{
		
		// Block as there is no tasklet sending
        if( !current )
		{
			PyErr_SetString( PyExc_RuntimeError, "No current tasklet set" );
			PyThread_release_lock( m_lock );
			return false;
		}

        //If current tasklet is main tasklet then throw runtime error
		if( current == Scheduler::get_main_tasklet() )
		{
			PyErr_SetString( PyExc_RuntimeError, "Channel cannot block on main tasklet" );
			PyThread_release_lock( m_lock );
			return false;
        }

        //If current tasklet has block_trap set to true then throw runtime error
		if( reinterpret_cast<PyTaskletObject*>( current )->m_blocktrap)
		{
			PyErr_SetString( PyExc_RuntimeError, "Channel cannot block on main tasklet with block_trap set true" );
			PyThread_release_lock( m_lock );
			return false;
		}

		// Block as there is no tasklet receiving
		Py_IncRef( current );

        add_tasklet_to_waiting_to_send( current );

		reinterpret_cast<PyTaskletObject*>( current )->block( this );

        PyThread_release_lock( m_lock );

         // Continue scheduler
		Scheduler::schedule();          

        PyThread_acquire_lock( m_lock, 1 );

	}

    PyTaskletObject* receiving_tasklet = reinterpret_cast<PyTaskletObject*>( pop_next_tasklet_blocked_on_receive() );

    receiving_tasklet->unblock();
	
    // Store for retrieval from receiving tasklet
	receiving_tasklet->set_transfer_arguments( args, exception );

    //Add this tasklet to the end of the scheduler
	Scheduler::insert_tasklet( reinterpret_cast<PyTaskletObject*>( Scheduler::get_current_tasklet() ) );

    PyThread_release_lock( m_lock );

    //Switch to the receiving tasklet
    receiving_tasklet->switch_to( );    

    Py_DECREF( receiving_tasklet );

	reinterpret_cast<PyTaskletObject*>( current )->m_transfer_in_progress = false;

	return true;

}

PyObject* PyChannelObject::receive()
{
	PyThread_acquire_lock( m_lock, 1 );

    reinterpret_cast<PyTaskletObject*>( Scheduler::get_current_tasklet() )->m_transfer_in_progress = true;

	// Block as there is no tasklet sending
	PyObject* current = Scheduler::get_current_tasklet();
    
    run_channel_callback( reinterpret_cast<PyObject*>( this ), current, false, m_first_tasklet_waiting_to_send == Py_None );    //TODO will_block logic here will change with addition of preference

    if( current == nullptr )
	{
		PyErr_SetString( PyExc_RuntimeError, "No current tasklet set" );
		return nullptr;
	}

	Py_IncRef( current );

    add_tasklet_to_waiting_to_receive( current );

    if( m_first_tasklet_waiting_to_send == Py_None )
	{
		//If current tasklet is main tasklet then throw runtime error
		if( current == Scheduler::get_main_tasklet() )
		{
			PyErr_SetString( PyExc_RuntimeError, "Channel cannot block on main tasklet" );
			PyThread_release_lock( m_lock );
			return nullptr;
		}

		//If current tasklet has block_trap set to true then throw runtime error
		if( reinterpret_cast<PyTaskletObject*>( current )->m_blocktrap )
		{
			PyErr_SetString( PyExc_RuntimeError, "Channel cannot block on main tasklet with block_trap set true" );
			PyThread_release_lock( m_lock );
			return nullptr;
		}

		reinterpret_cast<PyTaskletObject*>(current)->block(this);

        PyThread_release_lock( m_lock );

        // Continue scheduler
		Scheduler::schedule();
	
    }
	else
	{
		//Get first
		PyTaskletObject* sending_tasklet = reinterpret_cast<PyTaskletObject*>( pop_next_tasklet_blocked_on_send() );

		sending_tasklet->unblock();

        PyThread_release_lock( m_lock );

		sending_tasklet->switch_to();

        Py_DECREF( sending_tasklet );
	}

    //Process the exception
	if( reinterpret_cast<PyTaskletObject*>( current )->m_transfer_is_exception)
	{
		PyObject* arguments = reinterpret_cast<PyTaskletObject*>( current )->get_transfer_arguments();
	
        if(!PyTuple_Check(arguments))
		{
			PyErr_SetString( PyExc_RuntimeError, "This should be checked during send TODO remove this check when it is" ); //TODO
        }
		
        PyObject* exception_type = PyTuple_GetItem( arguments, 0 );

        PyObject* exception_values = PyTuple_GetSlice( arguments, 1, PyTuple_Size( arguments ) );

		PyErr_SetObject( exception_type, exception_values );

        Py_DecRef( arguments );

        reinterpret_cast<PyTaskletObject*>( current )->m_transfer_in_progress = false;  //TODO having two of these sucks

        return nullptr;

    }

    reinterpret_cast<PyTaskletObject*>( current )->m_transfer_in_progress = false;

    return reinterpret_cast<PyTaskletObject*>(current)->get_transfer_arguments();
}

int PyChannelObject::balance()
{
	return m_balance;
}

void PyChannelObject::remove_tasklet_from_blocked( PyObject* tasklet )
{
	PyObject* previous = reinterpret_cast<PyTaskletObject*>( tasklet )->m_previous;

    PyObject* next = reinterpret_cast<PyTaskletObject*>( tasklet )->m_next;

    if(previous == Py_None)
	{
		if(m_balance > 0)
		{
		    // Send
			m_first_tasklet_waiting_to_send = Py_None;
        }
		else
		{
		    //Receive
			m_first_tasklet_waiting_to_receive = Py_None;
        }
    }
	else
	{
		reinterpret_cast<PyTaskletObject*>( previous )->m_next = next;
	}

    if(next != Py_None)
	{
		reinterpret_cast<PyTaskletObject*>( next )->m_previous = previous;
    }
    
}

void PyChannelObject::run_channel_callback( PyObject* channel, PyObject* tasklet, bool sending, bool will_block )
{
	if( s_channel_callback != Py_None )
	{
		PyObject* args = PyTuple_New( 4 ); // TODO don't create this each time

		Py_IncRef( channel );

		Py_IncRef( tasklet );

		PyTuple_SetItem( args, 0, channel );

		PyTuple_SetItem( args, 1, tasklet );

        PyTuple_SetItem( args, 2, sending ? Py_True : Py_False );

        PyTuple_SetItem( args, 3, will_block ? Py_True : Py_False );

		PyObject_Call( s_channel_callback, args, nullptr );

		Py_DecRef( args );
	}
}

void PyChannelObject::add_tasklet_to_waiting_to_send( PyObject* tasklet )
{
	if(m_first_tasklet_waiting_to_send == Py_None)
	{
		m_first_tasklet_waiting_to_send = tasklet;
    }
	else
	{
		reinterpret_cast<PyTaskletObject*>( m_previous_blocked_send_tasklet )->m_next = tasklet;

		reinterpret_cast<PyTaskletObject*>( tasklet )->m_previous = m_previous_blocked_send_tasklet;
    }

    m_previous_blocked_send_tasklet = tasklet;

    m_balance++;
}

void PyChannelObject::add_tasklet_to_waiting_to_receive( PyObject* tasklet )
{
	if( m_first_tasklet_waiting_to_receive == Py_None )
	{
		m_first_tasklet_waiting_to_receive = tasklet;
	}
	else
	{
		reinterpret_cast<PyTaskletObject*>( m_previous_blocked_receive_tasklet )->m_next = tasklet;

		reinterpret_cast<PyTaskletObject*>( tasklet )->m_previous = m_previous_blocked_receive_tasklet;
	}

	m_previous_blocked_receive_tasklet = tasklet;

	m_balance--;
}

PyObject* PyChannelObject::pop_next_tasklet_blocked_on_send()
{
	PyObject* next = m_first_tasklet_waiting_to_send;

    m_first_tasklet_waiting_to_send = reinterpret_cast<PyTaskletObject*>( next )->m_next;

    m_balance--;

    return next;
}

PyObject* PyChannelObject::pop_next_tasklet_blocked_on_receive()
{
	PyObject* next = m_first_tasklet_waiting_to_receive;

	m_first_tasklet_waiting_to_receive = reinterpret_cast<PyTaskletObject*>( next )->m_next;

    m_balance++;

	return next;
}