#include <iostream>

#include "PyChannel.h"

#include "PyTasklet.h"

#include "PyScheduler.h"

PyChannelObject::PyChannelObject():
	m_balance(0),
	m_preference(-1),
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

	run_channel_callback( reinterpret_cast<PyObject*>(this), current, true, blocked_on_receive.empty() );  //TODO will_block logic here will change with addition of preference

    reinterpret_cast<PyTaskletObject*>( current )->set_transfer_in_progress(true);

	if( blocked_on_receive.empty() )
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

    PyThread_release_lock( m_lock );

	if (m_preference == PREFER_RECEIVER)
    {
		//Add this tasklet to the end of the scheduler
		PyTaskletObject* current_tasklet = reinterpret_cast<PyTaskletObject*>(Scheduler::get_current_tasklet());
		Scheduler::insert_tasklet( current_tasklet );

		//Switch BACK to the receiving tasklet
		if (!receiving_tasklet->switch_to())
        {
            return false; 
        }
		else
		{
            // Update current tasklet back to the correct calling tasklet
            // Required as the switch_to circumvents the scheduling queue
            // Which would normally deal with this
			Scheduler::set_current_tasklet( current_tasklet );
		}
		
	} 
    else if (m_preference == PREFER_SENDER)
    {
		Scheduler::insert_tasklet(receiving_tasklet);
	} 
    else if (m_preference == PREFER_NEITHER) 
    {
		Scheduler::insert_tasklet(receiving_tasklet);

		Scheduler::insert_tasklet( reinterpret_cast<PyTaskletObject*>( Scheduler::get_current_tasklet() ) );

		Scheduler::schedule();
	}
	else
	{
        // Invalid preference - Should never get here
        // Preference attribute is sanitised in PyTasklet_python.cpp
		PyErr_SetString( PyExc_RuntimeError, "Channel preference invalid." );
		
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

	run_channel_callback( reinterpret_cast<PyObject*>( this ), current, false, blocked_on_send.empty() );    //TODO will_block logic here will change with addition of preference

    if( current == nullptr )
	{
		PyErr_SetString( PyExc_RuntimeError, "No current tasklet set" );
		return nullptr;
	}

	Py_IncRef( current );

    add_tasklet_to_waiting_to_receive( current );

    if( blocked_on_send.empty() )
	{
		//If current tasklet has block_trap set to true then throw runtime error
		if( reinterpret_cast<PyTaskletObject*>( current )->blocktrap() )
		{
			PyErr_SetString( PyExc_RuntimeError, "Channel cannot block on main tasklet with block_trap set true" );
			PyThread_release_lock( m_lock );
			return nullptr;
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

        PyTaskletObject* current_tasklet = reinterpret_cast<PyTaskletObject*>( Scheduler::get_current_tasklet() );

		if(!sending_tasklet->switch_to())
		{
			return false;
        }
		else
		{
			// Update current tasklet back to the correct calling tasklet
			// Required as the switch_to circumvents the scheduling queue
			// Which would normally deal with this
			Scheduler::set_current_tasklet( current_tasklet );
        }

        Py_DECREF( sending_tasklet );
	}

    //Process the exception
	if( reinterpret_cast<PyTaskletObject*>(current)->transfer_is_exception())
	{
		PyObject* arguments = reinterpret_cast<PyTaskletObject*>(current)->get_transfer_arguments();
		reinterpret_cast<PyTaskletObject*>(current)->clear_transfer_arguments();

        if(!PyTuple_Check(arguments))
		{
			PyErr_SetString( PyExc_RuntimeError, "This should be checked during send TODO remove this check when it is" ); //TODO
        }
		
        PyObject* exception_type = PyTuple_GetItem( arguments, 0 );

        PyObject* exception_values = PyTuple_GetSlice( arguments, 1, PyTuple_Size( arguments ) );

		PyErr_SetObject( exception_type, exception_values );

        Py_DecRef( arguments );

		reinterpret_cast<PyTaskletObject*>(current)->set_transfer_in_progress(false);  //TODO having two of these sucks

        return nullptr;

    }

	reinterpret_cast<PyTaskletObject*>( current )->set_transfer_in_progress(false);

	auto ret = reinterpret_cast<PyTaskletObject*>( current )->get_transfer_arguments();

	reinterpret_cast<PyTaskletObject*>( current )->clear_transfer_arguments();

	return ret;
}

int PyChannelObject::balance() const
{
	return m_balance;
}

void PyChannelObject::remove_tasklet_from_blocked( PyObject* tasklet )
{
	//	TODO: This is not performant. convert this to a linkedList implementation
	// https://en.cppreference.com/w/cpp/container/deque:
	// - Insertion or removal of elements - linear O(n).
	// - As opposed to std::vector, the elements of a deque are not stored contiguously
	auto it = std::find( blocked_on_send.begin(), blocked_on_send.end(),  tasklet);
	if (it != blocked_on_send.end())
	{
		blocked_on_send.erase(it);
		return;
	}

	it = std::find( blocked_on_receive.begin(), blocked_on_receive.end(),  tasklet);
	if (it != blocked_on_receive.end())
	{
		blocked_on_receive.erase(it);
		return;
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
	blocked_on_send.push_back(tasklet);

    m_balance++;
}

void PyChannelObject::add_tasklet_to_waiting_to_receive( PyObject* tasklet )
{
	blocked_on_receive.push_back(tasklet);

	m_balance--;
}

PyObject* PyChannelObject::pop_next_tasklet_blocked_on_send()
{
	auto next = blocked_on_send.front();
	blocked_on_send.pop_front();

    m_balance--;

    return next;
}

PyObject* PyChannelObject::pop_next_tasklet_blocked_on_receive()
{
	auto next = blocked_on_receive.front();
	blocked_on_receive.pop_front();

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
