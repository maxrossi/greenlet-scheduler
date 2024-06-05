#include "Channel.h"

#include "Tasklet.h"

#include "ScheduleManager.h"

#include <vector>

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
    // Store weak reference in central store
    // Required just in case we lose all references to channel
    // The module will then be able to unblock if needed
    s_active_channels.push_back( this );
}

Channel::~Channel()
{
	// Destructor will never be called while there are tasklets blocking

	// Remove weak ref from store
	s_active_channels.remove( this );
	
	Py_DECREF( m_lock );
}

void Channel::incref()
{
	Py_IncRef( m_python_object );
}

void Channel::decref()
{
	Py_DecRef( m_python_object );
}

int Channel::refcount()
{
	return m_python_object->ob_refcnt;
}

PyObject* Channel::python_object()
{
	return m_python_object;
}

bool Channel::send( PyObject* args, PyObject* exception /* = nullptr */)
{
    PyThread_acquire_lock( m_lock, 1 );

    ScheduleManager* schedule_manager = ScheduleManager::get_scheduler();

    Tasklet* current = schedule_manager->get_current_tasklet(); //TODO naming clean up

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

            schedule_manager->decref();

			return false;
		}

        //If current tasklet has block_trap set to true then throw runtime error
		if( current->blocktrap())
		{
			PyErr_SetString( PyExc_RuntimeError, "Channel cannot block on main tasklet with block_trap set true" );

			PyThread_release_lock( m_lock );

            schedule_manager->decref();

			return false;
		}

		// Block as there is no tasklet receiving
		Py_IncRef( current->python_object() );

        add_tasklet_to_waiting_to_send( current );

		current->block( this );

        PyThread_release_lock( m_lock );

         // Continue scheduler
		if( !schedule_manager->yield() )
		{
			current->unblock();

			current->set_transfer_in_progress( false );
			Tasklet* tasklet = pop_next_tasklet_blocked_on_send();

            Py_DecRef( tasklet->python_object() );

            schedule_manager->decref();

			return false;
        }


        PyThread_acquire_lock( m_lock, 1 );

	}

    Tasklet* receiving_tasklet = pop_next_tasklet_blocked_on_receive();

    receiving_tasklet->unblock();
	
    // Store for retrieval from receiving tasklet
	receiving_tasklet->set_transfer_arguments( args, exception );

    PyThread_release_lock( m_lock );

    Tasklet* current_tasklet = schedule_manager->get_current_tasklet();

    if (!channel_switch(current_tasklet, receiving_tasklet, direction, SENDER))
    {
		schedule_manager->decref();

		return false;
    }
	

    Py_DecRef( receiving_tasklet->python_object() );

	current->set_transfer_in_progress( false );

    schedule_manager->decref();

	return true;

}

bool Channel::channel_switch(Tasklet* caller, Tasklet* other, int dir, int caller_dir)
{
	ScheduleManager* schedule_manager = ScheduleManager::get_scheduler();

    //if preference is opposit from direction, switch away from caller
	if( ( -dir == m_preference && dir == caller_dir ) || (dir == m_preference && dir != caller_dir))
    {
		schedule_manager->insert_tasklet( caller );
        if (!other->switch_to())
        {
			schedule_manager->decref();

			return false;
        }
    }
    //if preference is towards caller, schedule other tasklet and continue
	else
    {
		if( m_preference == PREFER_NEITHER && -caller_dir == dir )
        {
			schedule_manager->insert_tasklet( caller );
            if (!other->switch_to())
            {
				schedule_manager->decref();

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
				schedule_manager->insert_tasklet( other );
			}
        }
    }

    schedule_manager->set_current_tasklet( caller );

    schedule_manager->decref();

    return true;
}

PyObject* Channel::receive()
{
	PyThread_acquire_lock( m_lock, 1 );

    ScheduleManager* schedule_manager = ScheduleManager::get_scheduler();

    schedule_manager->get_current_tasklet()->set_transfer_in_progress( true );

	// Block as there is no tasklet sending
	Tasklet* current = schedule_manager->get_current_tasklet();

	run_channel_callback( this , current, false, m_first_blocked_on_send == nullptr );    //TODO will_block logic here will change with addition of preference

    if( current == nullptr )
	{
		PyErr_SetString( PyExc_RuntimeError, "No current tasklet set" );

        schedule_manager->decref();

		return nullptr;
	}

	current->incref();

    add_tasklet_to_waiting_to_receive( current );

    if( m_first_blocked_on_send == nullptr )
	{
		//If current tasklet has block_trap set to true then throw runtime error
		if( current->blocktrap() )
		{
			PyErr_SetString( PyExc_RuntimeError, "Channel cannot block on main tasklet with block_trap set true" );

			PyThread_release_lock( m_lock );

            schedule_manager->decref();

            current->decref();

			return nullptr;
		}
		else
		{
			current->block( this );

			PyThread_release_lock( m_lock );

			// Continue scheduler
			if( !schedule_manager->yield() )
			{
				// Will enter here is an exception has been thrown on a tasklet
				remove_tasklet_from_blocked( current );
				m_balance++;

				current->unblock();

                current->decref();

				current->set_transfer_in_progress( false );

                schedule_manager->decref();

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

        Tasklet* current_tasklet = schedule_manager->get_current_tasklet();
		
		if(!sending_tasklet->switch_to())
		{
			current->decref();

			schedule_manager->decref();

			return nullptr;
        }
		else
		{
			// Update current tasklet back to the correct calling tasklet
			// Required as the switch_to circumvents the scheduling queue
			// Which would normally deal with this
			schedule_manager->set_current_tasklet( current_tasklet );
        }

        Py_DecRef( sending_tasklet->python_object() );
	}

    
    //Process the exception
	PyObject* transfer_exception = current->transfer_exception();

	if( transfer_exception )
	{	
		PyObject* arguments = current->get_transfer_arguments();
		
		current->clear_transfer_arguments();
        
        PyErr_SetObject( transfer_exception, arguments );

        Py_DecRef( transfer_exception );

        Py_DecRef( arguments );
         
		current->set_transfer_in_progress( false );
        
        schedule_manager->decref();
       
        return nullptr;

    }

	current->set_transfer_in_progress( false );

	auto ret = current->get_transfer_arguments();

	current->clear_transfer_arguments();

    schedule_manager->decref();

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

void Channel::clear_blocked( bool pending )
{
    // Kill all blocked tasklets
    Tasklet* current = m_first_blocked_on_receive;

	while( current )
    {
		current->kill( pending );

        current = current->next_blocked();
    }

    current = m_first_blocked_on_send;

    while( current )
	{
		current->kill( pending );

        current = current->next_blocked();
	}

}

int Channel::num_active_channels()
{
	return s_active_channels.size();
}

int Channel::unblock_all_channels()
{
	int num_channels_unblocked = 0;

	auto iter = s_active_channels.begin();
	std::vector<Channel*> channels_to_unblock;
	while(iter != s_active_channels.end())
	{
		Channel* channel = *iter;
		if (channel->m_balance != 0)
		{
			channels_to_unblock.push_back(channel);
		}
		iter++;
	}

	for (auto chan : channels_to_unblock)
	{
		num_channels_unblocked++;
		chan->clear_blocked( false );
	}

    return num_channels_unblocked;
}