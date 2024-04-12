#include <iostream>

#include "Channel.h"

#include "Tasklet.h"

#include "ScheduleManager.h"

Channel::Channel( PyObject* python_object ) :
	m_python_object( python_object ),
	m_balance(0),
	m_preference(-1),
	m_lock( PyThread_allocate_lock() )
{

}

Channel::~Channel()
{
	//TODO need to clean up tasklets that are stored in waiting_to_send and waiting_to_receive lists

	Py_DECREF( m_lock );
}

PyObject* Channel::python_object()
{
	return m_python_object;
}

bool Channel::send( PyObject* args, bool exception /* = false */)
{
    PyThread_acquire_lock( m_lock, 1 );

    Tasklet* current = ScheduleManager::get_current_tasklet();  //TODO naming clean up

	run_channel_callback( this, current, true, blocked_on_receive.empty() );  //TODO will_block logic here will change with addition of preference

    current->set_transfer_in_progress(true);

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
		if( current->blocktrap())
		{
			PyErr_SetString( PyExc_RuntimeError, "Channel cannot block on main tasklet with block_trap set true" );

			PyThread_release_lock( m_lock );

			return false;
		}

		// Block as there is no tasklet receiving
		Py_IncRef( current->python_object() );

        add_tasklet_to_waiting_to_send( current );

		current->block( this );

        PyThread_release_lock( m_lock );

         // Continue scheduler
		if( !ScheduleManager::yield() )
		{
			current->unblock();

			current->set_transfer_in_progress( false );

			Tasklet* tasklet = pop_next_tasklet_blocked_on_send();

            Py_DecRef( tasklet->python_object() );

			return false;
        }

        PyThread_acquire_lock( m_lock, 1 );

	}

    Tasklet* receiving_tasklet = pop_next_tasklet_blocked_on_receive();

    receiving_tasklet->unblock();
	
    // Store for retrieval from receiving tasklet
	receiving_tasklet->set_transfer_arguments( args, exception );

    PyThread_release_lock( m_lock );

	if (m_preference == PREFER_RECEIVER)
    {
		//Add this tasklet to the end of the scheduler
		Tasklet* current_tasklet = ScheduleManager::get_current_tasklet();
		ScheduleManager::insert_tasklet( current_tasklet );

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
			ScheduleManager::set_current_tasklet( current_tasklet );
		}
		
	} 
    else if (m_preference == PREFER_SENDER)
    {
		ScheduleManager::insert_tasklet(receiving_tasklet);
	} 
    else if (m_preference == PREFER_NEITHER) 
    {
		ScheduleManager::insert_tasklet(receiving_tasklet);

		ScheduleManager::insert_tasklet( ScheduleManager::get_current_tasklet() );

		// TODO handle failure case
		ScheduleManager::yield();
	}
	else
	{
        // Invalid preference - Should never get here
        // Preference attribute is sanitised in PyTasklet_python.cpp
		PyErr_SetString( PyExc_RuntimeError, "Channel preference invalid." );
		
		return false;
	}

    Py_DecRef( receiving_tasklet->python_object() );

	current->set_transfer_in_progress( false );

	return true;

}

PyObject* Channel::receive()
{
	PyThread_acquire_lock( m_lock, 1 );

    ScheduleManager::get_current_tasklet()->set_transfer_in_progress(true);

	// Block as there is no tasklet sending
	Tasklet* current = ScheduleManager::get_current_tasklet();

	run_channel_callback( this , current, false, blocked_on_send.empty() );    //TODO will_block logic here will change with addition of preference

    if( current == nullptr )
	{
		PyErr_SetString( PyExc_RuntimeError, "No current tasklet set" );
		return nullptr;
	}

	Py_IncRef( current->python_object() );

    add_tasklet_to_waiting_to_receive( current );

    if( blocked_on_send.empty() )
	{
		//If current tasklet has block_trap set to true then throw runtime error
		if( current->blocktrap() )
		{
			PyErr_SetString( PyExc_RuntimeError, "Channel cannot block on main tasklet with block_trap set true" );

			PyThread_release_lock( m_lock );

			return nullptr;
		}
		else
		{
			current->block( this );

			PyThread_release_lock( m_lock );

			// Continue scheduler
			if( !ScheduleManager::yield() )
			{
				// Will enter here is an exception has been thrown on a tasklet
				remove_tasklet_from_blocked( current );

				current->unblock();

				current->set_transfer_in_progress( false );

				return false;
			}
		}
	}
	else
	{
		//Get first
		Tasklet* sending_tasklet = pop_next_tasklet_blocked_on_send();

		sending_tasklet->unblock();

        PyThread_release_lock( m_lock );

        Tasklet* current_tasklet = ScheduleManager::get_current_tasklet();

		if(!sending_tasklet->switch_to())
		{
			return false;
        }
		else
		{
			// Update current tasklet back to the correct calling tasklet
			// Required as the switch_to circumvents the scheduling queue
			// Which would normally deal with this
			ScheduleManager::set_current_tasklet( current_tasklet );
        }

        Py_DecRef( sending_tasklet->python_object() );
	}

    //Process the exception
	if( current->transfer_is_exception())
	{
		PyObject* arguments = current->get_transfer_arguments();
		current->clear_transfer_arguments();

        if(!PyTuple_Check(arguments))
		{
			PyErr_SetString( PyExc_RuntimeError, "This should be checked during send TODO remove this check when it is" ); //TODO
        }
		
        PyObject* exception_type = PyTuple_GetItem( arguments, 0 );

        PyObject* exception_values = PyTuple_GetSlice( arguments, 1, PyTuple_Size( arguments ) );

		PyErr_SetObject( exception_type, exception_values );

        Py_DecRef( arguments );

		current->set_transfer_in_progress( false );  //TODO having two of these sucks

        return nullptr;

    }

	current->set_transfer_in_progress( false );

	auto ret = current->get_transfer_arguments();

	current->clear_transfer_arguments();

	return ret;
}

int Channel::balance() const
{
	return m_balance;
}

void Channel::remove_tasklet_from_blocked( Tasklet* tasklet )
{
	//	TODO: This is not performant. convert this to a linkedList implementation
	// https://en.cppreference.com/w/cpp/container/deque:
	// - Insertion or removal of elements - linear O(n).
	// - As opposed to std::vector, the elements of a deque are not stored contiguously
    // 
    // NOTE! This has been added just to get functionality working, it must be fixed before release
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

void Channel::run_channel_callback( Channel* channel, Tasklet* tasklet, bool sending, bool will_block ) const
{
	if( s_channel_callback != Py_None )
	{
		PyObject* args = PyTuple_New( 4 ); // TODO don't create this each time

        PyObject* py_channel = channel->python_object();

        PyObject* py_tasklet = tasklet->python_object();

		Py_IncRef( py_channel );

		Py_IncRef( py_tasklet );

		PyTuple_SetItem( args, 0, py_channel );

		PyTuple_SetItem( args, 1, py_tasklet );

        PyTuple_SetItem( args, 2, sending ? Py_True : Py_False );

        PyTuple_SetItem( args, 3, will_block ? Py_True : Py_False );

		PyObject_Call( s_channel_callback, args, nullptr );

		Py_DecRef( args );
	}
}

void Channel::add_tasklet_to_waiting_to_send( Tasklet* tasklet )
{
	blocked_on_send.push_back(tasklet);

    m_balance++;
}

void Channel::add_tasklet_to_waiting_to_receive( Tasklet* tasklet )
{
	blocked_on_receive.push_back(tasklet);

	m_balance--;
}

Tasklet* Channel::pop_next_tasklet_blocked_on_send()
{
	auto next = blocked_on_send.front();
	blocked_on_send.pop_front();

    m_balance--;

    return next;
}

Tasklet* Channel::pop_next_tasklet_blocked_on_receive()
{
	auto next = blocked_on_receive.front();
	blocked_on_receive.pop_front();

    m_balance++;

	return next;
}

PyObject* Channel::channel_callback()
{
	return s_channel_callback;
}

void Channel::set_channel_callback( PyObject* callback )
{
	s_channel_callback = callback;
}

int Channel::preference() const
{
	return m_preference;
}

void Channel::set_preference( int value )
{
	m_preference = value;
}
