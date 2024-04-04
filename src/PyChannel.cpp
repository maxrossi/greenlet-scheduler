#include "PyChannel.h"

#include "PyTasklet.h"

#include "PyScheduler.h"

PyChannelObject::PyChannelObject():
	m_balance(0),
	m_preference(0),
	m_first_tasklet_waiting_to_send(Py_None),
	m_first_tasklet_waiting_to_receive(Py_None),
	m_previous_blocked_send_tasklet(Py_None),
	m_previous_blocked_receive_tasklet(Py_None),
	m_lock( PyThread_allocate_lock() )
{

}

PyChannelObject::~PyChannelObject()
{
	//TODO need to clean up tasklets that are stored in waiting_to_send and waiting_to_receive lists


	Py_DECREF( m_lock );
}

bool PyChannelObject::send( PyObject* args, bool exception /* = false */)
{
    PyThread_acquire_lock( m_lock, 1 );

    PyObject* current = Scheduler::get_current_tasklet();  //TODO naming clean up

    run_channel_callback( reinterpret_cast<PyObject*>(this), current, true, m_first_tasklet_waiting_to_receive == Py_None );  //TODO will_block logic here will change with addition of preference

    reinterpret_cast<PyTaskletObject*>( current )->set_transfer_in_progress(true);

	if( m_first_tasklet_waiting_to_receive == Py_None )
	{
		
		// Block as there is no tasklet sending
        if( !current )
		{
			PyErr_SetString( PyExc_RuntimeError, "No current tasklet set" );
			PyThread_release_lock( m_lock );
			return false;
		}

        //If current tasklet has block_trap set to true then throw runtime error
		if( reinterpret_cast<PyTaskletObject*>( current )->blocktrap())
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
		if(!Scheduler::schedule())
		{
			reinterpret_cast<PyTaskletObject*>( current )->unblock();

			reinterpret_cast<PyTaskletObject*>( current )->set_transfer_in_progress( false );

			PyObject* tasklet = pop_next_tasklet_blocked_on_send();

            Py_DecRef( tasklet );

			return false;
        }

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
    if(!receiving_tasklet->switch_to( ))
	{
		return false;
    }

    Py_DECREF( receiving_tasklet );

	reinterpret_cast<PyTaskletObject*>( current )->set_transfer_in_progress(false);

	return true;

}

PyObject* PyChannelObject::receive()
{
	PyThread_acquire_lock( m_lock, 1 );

    reinterpret_cast<PyTaskletObject*>( Scheduler::get_current_tasklet() )->set_transfer_in_progress(true);

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
		//If current tasklet has block_trap set to true then throw runtime error
		if( reinterpret_cast<PyTaskletObject*>( current )->blocktrap() )
		{
			PyErr_SetString( PyExc_RuntimeError, "Channel cannot block on main tasklet with block_trap set true" );
			PyThread_release_lock( m_lock );
			return nullptr;
		}

		//If current tasklet is main tasklet then throw runtime error
		if( current == Scheduler::get_main_tasklet() )
		{
			PyThread_release_lock( m_lock );

			if(!Scheduler::schedule())
			{
				reinterpret_cast<PyTaskletObject*>( current )->set_transfer_in_progress( false );

				PyObject* tasklet = pop_next_tasklet_blocked_on_receive();

				Py_DecRef( tasklet );

				return false;
            }

			if( !reinterpret_cast<PyTaskletObject*>( current )->get_transfer_arguments() )
			{
				PyErr_SetString( PyExc_RuntimeError, "The main tasklet is receiving without a sender available" );
				PyThread_release_lock( m_lock );
				return nullptr;
			}
		}
		else
		{
			reinterpret_cast<PyTaskletObject*>( current )->block( this );

			PyThread_release_lock( m_lock );

			// Continue scheduler
			if( !Scheduler::schedule() )
			{
				// Will enter here is an exception has been thrown on a tasklet
				remove_tasklet_from_blocked( current );

				reinterpret_cast<PyTaskletObject*>( current )->unblock();

				reinterpret_cast<PyTaskletObject*>( current )->set_transfer_in_progress( false );

				return false;
			}
		}
	}
	else
	{
		//Get first
		PyTaskletObject* sending_tasklet = reinterpret_cast<PyTaskletObject*>( pop_next_tasklet_blocked_on_send() );

		sending_tasklet->unblock();

        PyThread_release_lock( m_lock );

		if(!sending_tasklet->switch_to())
		{
			return false;
        }

        Py_DECREF( sending_tasklet );
	}

    //Process the exception
	if( reinterpret_cast<PyTaskletObject*>( current )->transfer_is_exception())
	{
		PyObject* arguments = reinterpret_cast<PyTaskletObject*>( current )->get_transfer_arguments();
		reinterpret_cast<PyTaskletObject*>(current)->clear_transfer_arguments();
	
        if(!PyTuple_Check(arguments))
		{
			PyErr_SetString( PyExc_RuntimeError, "This should be checked during send TODO remove this check when it is" ); //TODO
        }
		
        PyObject* exception_type = PyTuple_GetItem( arguments, 0 );

        PyObject* exception_values = PyTuple_GetSlice( arguments, 1, PyTuple_Size( arguments ) );

		PyErr_SetObject( exception_type, exception_values );

        Py_DecRef( arguments );

        reinterpret_cast<PyTaskletObject*>( current )->set_transfer_in_progress(false);  //TODO having two of these sucks

        return nullptr;

    }

    reinterpret_cast<PyTaskletObject*>( current )->set_transfer_in_progress(false);


    auto ret = reinterpret_cast<PyTaskletObject*>(current)->get_transfer_arguments();
	reinterpret_cast<PyTaskletObject*>(current)->clear_transfer_arguments();

	return ret;
}

int PyChannelObject::balance() const
{
	return m_balance;
}

void PyChannelObject::remove_tasklet_from_blocked( PyObject* tasklet )
{
	PyObject* previous = reinterpret_cast<PyTaskletObject*>( tasklet )->previous();

    PyObject* next = reinterpret_cast<PyTaskletObject*>( tasklet )->next();

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
		reinterpret_cast<PyTaskletObject*>( previous )->set_next(next);
	}

    if(next != Py_None)
	{
		reinterpret_cast<PyTaskletObject*>( next )->set_previous(previous);
    }
    
}

void PyChannelObject::run_channel_callback( PyObject* channel, PyObject* tasklet, bool sending, bool will_block ) const
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
		reinterpret_cast<PyTaskletObject*>( m_previous_blocked_send_tasklet )->set_next(tasklet);

		reinterpret_cast<PyTaskletObject*>( tasklet )->set_previous(m_previous_blocked_send_tasklet);
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
		reinterpret_cast<PyTaskletObject*>( m_previous_blocked_receive_tasklet )->set_next(tasklet);

		reinterpret_cast<PyTaskletObject*>( tasklet )->set_previous(m_previous_blocked_receive_tasklet);
	}

	m_previous_blocked_receive_tasklet = tasklet;

	m_balance--;
}

PyObject* PyChannelObject::pop_next_tasklet_blocked_on_send()
{
	PyObject* next = m_first_tasklet_waiting_to_send;

    m_first_tasklet_waiting_to_send = reinterpret_cast<PyTaskletObject*>( next )->next();

    m_balance--;

    return next;
}

PyObject* PyChannelObject::pop_next_tasklet_blocked_on_receive()
{
	PyObject* next = m_first_tasklet_waiting_to_receive;

	m_first_tasklet_waiting_to_receive = reinterpret_cast<PyTaskletObject*>( next )->next();

    m_balance++;

	return next;
}

PyObject* PyChannelObject::channel_callback()
{
	return s_channel_callback;
}

void PyChannelObject::set_channel_callback( PyObject* callback )
{
	s_channel_callback = callback;
}

int PyChannelObject::preference() const
{
	return m_preference;
}

void PyChannelObject::set_preference( int value )
{
	m_preference = value;
}