#include "Channel.h"

#include "Tasklet.h"

#include "ScheduleManager.h"

Channel::Channel( PyObject* python_object ) :
	m_python_object( python_object ),
	m_balance(0),
	m_preference(-1),
	m_lock( PyThread_allocate_lock() ),
	m_last_blocked_on_send( nullptr ),
	m_first_blocked_on_send( nullptr ),
	m_last_blocked_on_receive( nullptr ),
	m_first_blocked_on_receive( nullptr )
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

	run_channel_callback( this, current, true, m_first_blocked_on_receive == nullptr );  //TODO will_block logic here will change with addition of preference

    current->set_transfer_in_progress(true);
	int direction = SENDER;
	if( m_first_blocked_on_receive == nullptr )
	{
		direction = RECEIVER;
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

    Tasklet* current_tasklet = ScheduleManager::get_current_tasklet();

    if (!channel_switch(current_tasklet, receiving_tasklet, direction, SENDER))
    {
		return false;
    }
	

    Py_DecRef( receiving_tasklet->python_object() );

	current->set_transfer_in_progress( false );

	return true;

}

bool Channel::channel_switch(Tasklet* caller, Tasklet* other, int dir, int caller_dir)
{
    //if preference is opposit from direction, switch away from caller
	if( ( -dir == m_preference && dir == caller_dir ) || (dir == m_preference && dir != caller_dir))
    {
		ScheduleManager::insert_tasklet( caller );
        if (!other->switch_to())
        {
			return false;
        }
    }
    //if preference is towards caller, schedule other tasklet and continue
	else
    {
		if( m_preference == PREFER_NEITHER && -caller_dir == dir )
        {
			ScheduleManager::insert_tasklet( caller );
            if (!other->switch_to())
            {
				return false;
            }
        }
        else
        {
			if( other->scheduled() )
			{
				other->set_reschedule( true );
			}
			else
			{
				ScheduleManager::insert_tasklet( other );
			}
        }
    }

    ScheduleManager::set_current_tasklet( caller );

    return true;
}

PyObject* Channel::receive()
{
	PyThread_acquire_lock( m_lock, 1 );

    ScheduleManager::get_current_tasklet()->set_transfer_in_progress(true);

	// Block as there is no tasklet sending
	Tasklet* current = ScheduleManager::get_current_tasklet();

	run_channel_callback( this , current, false, m_first_blocked_on_send == nullptr );    //TODO will_block logic here will change with addition of preference

    if( current == nullptr )
	{
		PyErr_SetString( PyExc_RuntimeError, "No current tasklet set" );
		return nullptr;
	}

	Py_IncRef( current->python_object() );

    add_tasklet_to_waiting_to_receive( current );

    if( m_first_blocked_on_send == nullptr )
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
				m_balance++;

				current->unblock();

				current->set_transfer_in_progress( false );

				return nullptr;
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
			return nullptr;
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
	bool end_node = false;
    if (tasklet == m_first_blocked_on_receive)
    {
		m_first_blocked_on_receive = tasklet->next_blocked();
        if (m_first_blocked_on_receive != nullptr)
        {
			m_first_blocked_on_receive->set_previous_blocked( nullptr );
        }

		end_node = true;
    }

    if (tasklet == m_first_blocked_on_send)
    {
		m_first_blocked_on_send = tasklet->next_blocked();
        if (m_first_blocked_on_send != nullptr)
        {
			m_first_blocked_on_send->set_previous_blocked( nullptr );
        }

        end_node = true;
    }

    if (tasklet == m_last_blocked_on_receive)
    {
		m_last_blocked_on_receive = tasklet->previous_blocked();
        if (m_last_blocked_on_receive != nullptr)
        {
			m_last_blocked_on_receive->set_next_blocked( nullptr );
        }
		
		end_node = true;
    }

    if (tasklet == m_last_blocked_on_send)
    {
		m_last_blocked_on_send = tasklet->previous_blocked();
        if (m_last_blocked_on_send != nullptr)
        {
			m_last_blocked_on_send->set_next_blocked( nullptr );
        }
		
		end_node = true;
    }

    if (!end_node)
    {
		tasklet->previous_blocked()->set_next_blocked( tasklet->next_blocked() );
		tasklet->next_blocked()->set_previous_blocked( tasklet->previous_blocked() );
    }

    tasklet->set_next_blocked( nullptr );
	tasklet->set_previous_blocked( nullptr );


}

void Channel::run_channel_callback( Channel* channel, Tasklet* tasklet, bool sending, bool will_block ) const
{
	if( s_channel_callback )
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
    if( m_first_blocked_on_send == nullptr )
    {
		m_first_blocked_on_send = tasklet;
		m_last_blocked_on_send = tasklet;
    }
	else
	{
		m_first_blocked_on_send->set_previous_blocked( tasklet );
		tasklet->set_next_blocked( m_first_blocked_on_send );
		m_first_blocked_on_send = tasklet;
    }

    m_balance++;
}

void Channel::add_tasklet_to_waiting_to_receive( Tasklet* tasklet )
{
	if( m_first_blocked_on_receive == nullptr )
    {
		m_first_blocked_on_receive = tasklet;
        m_last_blocked_on_receive = tasklet;
    }
    else
    {
		m_first_blocked_on_receive->set_previous_blocked( tasklet );
		tasklet->set_next_blocked( m_first_blocked_on_receive );
		m_first_blocked_on_receive = tasklet;
    }

    m_balance--;
}

Tasklet* Channel::pop_next_tasklet_blocked_on_send()
{
	Tasklet* next = nullptr;
    if (m_last_blocked_on_send != nullptr)
    {
		next = m_last_blocked_on_send;
		remove_tasklet_from_blocked( next );
		m_balance--;
    }
	
    return next;
}

Tasklet* Channel::pop_next_tasklet_blocked_on_receive()
{
	Tasklet* next = nullptr;
    if (m_last_blocked_on_receive != nullptr)
    {
		next = m_last_blocked_on_receive;
		remove_tasklet_from_blocked( next );
		m_balance++;
    }

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

Tasklet* Channel::blocked_queue_front()
{
    if (m_first_blocked_on_receive != nullptr)
    {
		return m_first_blocked_on_receive;
    }
    else if (m_first_blocked_on_send != nullptr)
    {
		return m_first_blocked_on_send;
    }
	return nullptr;
}
