#include "Channel.h"

#include <vector>

#include "Tasklet.h"
#include "ScheduleManager.h"


Channel::Channel( PyObject* pythonObject ) :
	PythonCppType( pythonObject ),
	m_balance(0),
	m_preference(ChannelPreference::RECEIVER),
	m_lock( PyThread_allocate_lock() ),
	m_lastBlockedOnSend( nullptr ),
	m_firstBlockedOnSend( nullptr ),
	m_lastBlockedOnReceive( nullptr ),
	m_firstBlockedOnReceive( nullptr ),
	m_closing( false ),
	m_closed( false )
{
    // Store weak reference in central store
    // Required just in case we lose all references to channel
    // The module will then be able to unblock if needed
    s_activeChannels.push_back( this );
}

Channel::~Channel()
{
	// Note: Destructor will never be called while there are tasklets blocking

	// Remove weak ref from store
	s_activeChannels.remove( this );

	Py_DECREF( m_lock );
}

bool Channel::Send( PyObject* args, PyObject* exception /* = nullptr */, bool restoreException /* = false */)
{
    PyThread_acquire_lock( m_lock, 1 );

    ScheduleManager* scheduleManager = ScheduleManager::GetThreadScheduleManager();

    Tasklet* current = scheduleManager->GetCurrentTasklet();

	RunChannelCallback( this, current, true, m_firstBlockedOnReceive == nullptr );

    current->SetTransferInProgress(true);

	ChannelDirection direction = ChannelDirection::SENDER;

	if( m_firstBlockedOnReceive == nullptr )
	{
		direction = ChannelDirection::RECEIVER;

		// Block as there is no tasklet sending
        if( !current )
		{
			PyErr_SetString( PyExc_RuntimeError, "No current tasklet set" );

            scheduleManager->Decref();

            PyThread_release_lock( m_lock );

			return false;
		}

        //If current tasklet has block_trap set to true then throw runtime error
		if( current->IsBlocktrapped())
		{
			PyErr_SetString( PyExc_RuntimeError, "Channel cannot block on main tasklet with block_trap set true" );

            scheduleManager->Decref();

            PyThread_release_lock( m_lock );

			return false;
		}

        // Ensure channel is open
        if (m_closed || m_closing)
        {
			PyErr_SetString( PyExc_ValueError, "Send/receive operation on a closed channel" );

			scheduleManager->Decref();

            PyThread_release_lock( m_lock );

			return false;
        }

		// Block as there is no tasklet receiving
		current->Incref();

        AddTaskletToWaitingToSend( current );

		current->Block( this );

        PyThread_release_lock( m_lock );

         // Continue scheduler
		if( !scheduleManager->Yield() )
		{
			PyThread_acquire_lock( m_lock, 1 );

			current->SetTransferInProgress( false );

            if( current->IsOnChannelBlockList() )
            {
				current->Unblock();

				PopNextTaskletBlockedOnSend();

            }

			current->Decref();
            
            scheduleManager->Decref();

            PyThread_release_lock( m_lock );

			return false;
        }


        PyThread_acquire_lock( m_lock, 1 );

    }

    Tasklet* receivingTasklet = PopNextTaskletBlockedOnReceive();

    receivingTasklet->Unblock();
	
    // Store for retrieval from receiving tasklet
	receivingTasklet->SetTransferArguments( args, exception, restoreException );

	PyThread_release_lock( m_lock );

    Tasklet* current_tasklet = scheduleManager->GetCurrentTasklet();

    if (!ChannelSwitch(current_tasklet, receivingTasklet, direction, ChannelDirection::SENDER))
    {
		scheduleManager->Decref();

		return false;
    }
	

    receivingTasklet->Decref();

	current->SetTransferInProgress( false );

    scheduleManager->Decref();

	return true;

}

bool Channel::ChannelSwitch( Tasklet* caller, Tasklet* other, ChannelDirection directionOfChannelOperation, ChannelDirection callerDir )
{
	ScheduleManager* scheduleManager = ScheduleManager::GetThreadScheduleManager();

    ChannelDirection inverseChannelOperation = InvertDirection( directionOfChannelOperation );

    bool operationDirectionMatchesCallerDirection = directionOfChannelOperation == callerDir;

    //if preference is opposit from direction, switch away from caller
	if( ( inverseChannelOperation == m_preference && operationDirectionMatchesCallerDirection ) || 
        ( directionOfChannelOperation == m_preference && !operationDirectionMatchesCallerDirection ) )
    {
        scheduleManager->InsertTasklet( caller );
        if (!other->SwitchTo())
        {
			scheduleManager->Decref();

			return false;
        }
    }
    //if preference is towards caller, schedule other tasklet and continue
	else
    {
		if( m_preference == ChannelPreference::NEITHER && InvertDirection(callerDir) == directionOfChannelOperation )
        {
            scheduleManager->InsertTasklet( caller );
            if (!other->SwitchTo())
            {
				scheduleManager->Decref();

				return false;
            }
        }
        else
        {
			if( other->IsScheduled() )
			{
				other->SetReschedule( true );
			}
			else
			{
                scheduleManager->InsertTasklet( other );
			}
        }
    }

    scheduleManager->SetCurrentTasklet( caller );

    scheduleManager->Decref();

    return true;
}

PyObject* Channel::Receive()
{
	PyThread_acquire_lock( m_lock, 1 );

    ScheduleManager* scheduleManager = ScheduleManager::GetThreadScheduleManager();

    scheduleManager->GetCurrentTasklet()->SetTransferInProgress( true );

	// Block as there is no tasklet sending
	Tasklet* current = scheduleManager->GetCurrentTasklet();

	RunChannelCallback( this , current, false, m_firstBlockedOnSend == nullptr );

    if( current == nullptr )
	{
		PyErr_SetString( PyExc_RuntimeError, "No current tasklet set" );

        scheduleManager->Decref();

        PyThread_release_lock( m_lock );

		return nullptr;
	}

	current->Incref();

    AddTaskletToWaitingToReceive( current );

    if( m_firstBlockedOnSend == nullptr )
	{
		//If current tasklet has block_trap set to true then throw runtime error
		if( current->IsBlocktrapped() )
		{
			RemoveTaskletFromBlocked( current );

			PyErr_SetString( PyExc_RuntimeError, "Channel cannot block on main tasklet with block_trap set true" );

            scheduleManager->Decref();

            current->Decref();

            PyThread_release_lock( m_lock );

			return nullptr;
		}

        // Ensure channel is open
        if( m_closed || m_closing )
		{
			PyErr_SetString( PyExc_ValueError, "Send/receive operation on a closed channel" );

			scheduleManager->Decref();

            PyThread_release_lock( m_lock );

			return nullptr;
		}
		
		current->Block( this );

		PyThread_release_lock( m_lock );

		// Continue scheduler
		if( !scheduleManager->Yield() )
		// Will enter here if an exception has been thrown on a tasklet
		{
			PyThread_acquire_lock( m_lock, 1 );

            if( current->IsOnChannelBlockList() )
            {
				RemoveTaskletFromBlocked( current );
            }

			current->Unblock();

            current->Decref();

			current->SetTransferInProgress( false );

            scheduleManager->Decref();

            PyThread_release_lock( m_lock );

			return nullptr;
		}
	}
	else
	{
		//Get first
		Tasklet* sendingTasklet = PopNextTaskletBlockedOnSend();

		sendingTasklet->Unblock();

        Tasklet* current_tasklet = scheduleManager->GetCurrentTasklet();
		
        PyThread_release_lock( m_lock );

		if(!sendingTasklet->SwitchTo())
		{
			current->Decref();

			scheduleManager->Decref();

			return nullptr;
        }
		else
		{
			// Update current tasklet back to the correct calling tasklet
			// Required as the switch_to circumvents the scheduling queue
			// Which would normally deal with this
			scheduleManager->SetCurrentTasklet( current_tasklet );
        }

        sendingTasklet->Decref();
	}

    
    //Process the exception
	PyObject* transferException = current->TransferException();

	if( transferException )
	{	
        PyObject* arguments = current->GetTransferArguments();

        // If arguments are Py_None, then we want to use the exception data as it is set in send_throw
        if (current->ShouldRestoreTransferException())
        {
            auto exceptionType = PyTuple_GetItem( transferException, 0 );
			auto exceptionValue = PyTuple_GetItem( transferException, 1 );
			auto exceptionTb = PyTuple_GetItem( transferException, 2 );

            Py_INCREF( exceptionType );
			Py_INCREF( exceptionValue );
			Py_INCREF( exceptionTb );

            Py_DECREF( transferException );
            
			PyErr_Restore( exceptionType, exceptionValue == Py_None ? nullptr : exceptionValue, exceptionTb == Py_None ? nullptr : exceptionTb );
        }
        else
        {
			PyErr_SetObject( transferException, arguments );

			Py_DecRef( transferException );

        }

        Py_DecRef( arguments );

        current->ClearTransferArguments();

		current->SetTransferInProgress( false );
        
        scheduleManager->Decref();
       
        return nullptr;

    }

	current->SetTransferInProgress( false );

	auto ret = current->GetTransferArguments();

	current->ClearTransferArguments();

    scheduleManager->Decref();

	return ret;
}

int Channel::Balance() const
{
	return m_balance;
}

void Channel::UnblockTaskletFromChannel( Tasklet* tasklet )
{
    // Public exposed remove_tasklet_from_blocked wrapped in lock for thread safety
	PyThread_acquire_lock( m_lock, 1 );

    RemoveTaskletFromBlocked( tasklet );

    PyThread_release_lock( m_lock );
}

void Channel::RemoveTaskletFromBlocked( Tasklet* tasklet )
{
	bool endNode = false;
    if (tasklet == m_firstBlockedOnReceive)
    {
		m_firstBlockedOnReceive = tasklet->NextBlocked();
        if (m_firstBlockedOnReceive != nullptr)
        {
			m_firstBlockedOnReceive->SetPreviousBlocked( nullptr );
        }

		endNode = true;
    }

    if (tasklet == m_firstBlockedOnSend)
    {
		m_firstBlockedOnSend = tasklet->NextBlocked();
        if (m_firstBlockedOnSend != nullptr)
        {
			m_firstBlockedOnSend->SetPreviousBlocked( nullptr );
        }

        endNode = true;
    }

    if (tasklet == m_lastBlockedOnReceive)
    {
		m_lastBlockedOnReceive = tasklet->PreviousBlocked();
        if (m_lastBlockedOnReceive != nullptr)
        {
			m_lastBlockedOnReceive->SetNextBlocked( nullptr );
        }
		
		endNode = true;
    }

    if (tasklet == m_lastBlockedOnSend)
    {
		m_lastBlockedOnSend = tasklet->PreviousBlocked();
        if (m_lastBlockedOnSend != nullptr)
        {
			m_lastBlockedOnSend->SetNextBlocked( nullptr );
        }
		
		endNode = true;
    }

    if (!endNode)
    {
		tasklet->PreviousBlocked()->SetNextBlocked( tasklet->NextBlocked() );
		tasklet->NextBlocked()->SetPreviousBlocked( tasklet->PreviousBlocked() );
    }

    tasklet->SetNextBlocked( nullptr );
	tasklet->SetPreviousBlocked( nullptr );

    if (tasklet->GetBlockedDirection() == ChannelDirection::SENDER)
    {
		DecrementBalance();
    }
	else if( tasklet->GetBlockedDirection() == ChannelDirection ::RECEIVER)
    {
		IncrementBalance();
    }

    tasklet->SetBlockedDirection( ChannelDirection::NEITHER );
    
}

void Channel::RunChannelCallback( Channel* channel, Tasklet* tasklet, bool sending, bool willBlock ) const
{
	if( s_channelCallback )
	{
        channel->Incref();

		PyTuple_SetItem( s_callbackArguments, 0, channel->PythonObject() );

		tasklet->Incref();

		PyTuple_SetItem( s_callbackArguments, 1, tasklet->PythonObject() );

        PyTuple_SetItem( s_callbackArguments, 2, sending ? Py_True : Py_False );

        PyTuple_SetItem( s_callbackArguments, 3, willBlock ? Py_True : Py_False );

		PyObject_Call( s_channelCallback, s_callbackArguments, nullptr );
	}
}

void Channel::AddTaskletToWaitingToSend( Tasklet* tasklet )
{
    if( m_firstBlockedOnSend == nullptr )
    {
		m_firstBlockedOnSend = tasklet;
		m_lastBlockedOnSend = tasklet;
    }
	else
	{
		m_firstBlockedOnSend->SetPreviousBlocked( tasklet );
		tasklet->SetNextBlocked( m_firstBlockedOnSend );
		m_firstBlockedOnSend = tasklet;
    }

    tasklet->SetBlockedDirection( ChannelDirection::SENDER );
    IncrementBalance();
}

void Channel::AddTaskletToWaitingToReceive( Tasklet* tasklet )
{
	if( m_firstBlockedOnReceive == nullptr )
    {
		m_firstBlockedOnReceive = tasklet;
        m_lastBlockedOnReceive = tasklet;
    }
    else
    {
		m_firstBlockedOnReceive->SetPreviousBlocked( tasklet );
		tasklet->SetNextBlocked( m_firstBlockedOnReceive );
		m_firstBlockedOnReceive = tasklet;
    }

    tasklet->SetBlockedDirection( ChannelDirection::RECEIVER );
    DecrementBalance();
}

Tasklet* Channel::PopNextTaskletBlockedOnSend()
{
	Tasklet* next = nullptr;
    if (m_lastBlockedOnSend != nullptr)
    {
		next = m_lastBlockedOnSend;

		RemoveTaskletFromBlocked( next );
    }
	
    return next;
}

Tasklet* Channel::PopNextTaskletBlockedOnReceive()
{
	Tasklet* next = nullptr;
    if (m_lastBlockedOnReceive != nullptr)
    {
		next = m_lastBlockedOnReceive;

		RemoveTaskletFromBlocked( next );
    }

    return next;
}

PyObject* Channel::ChannelCallback()
{
	return s_channelCallback;
}

void Channel::SetChannelCallback( PyObject* callback )
{
	Py_XDECREF( s_channelCallback ); 

    if (callback)
    {
		if( !s_callbackArguments )
		{
			s_callbackArguments = PyTuple_New( 4 );
		}
    }
    else
    {
		Py_XDECREF( s_callbackArguments ); 

        s_callbackArguments = nullptr;
    }

    s_channelCallback = callback;
}

int Channel::PreferenceAsInt() const
{
	return DirectionToInt( m_preference );
}

void Channel::SetPreferenceFromInt( int value )
{
	m_preference = DirectionFromInt( value );
}

Tasklet* Channel::BlockedQueueFront() const
{
    if (m_firstBlockedOnReceive != nullptr)
    {
		return m_firstBlockedOnReceive;
    }
    else if (m_firstBlockedOnSend != nullptr)
    {
		return m_firstBlockedOnSend;
    }
	return nullptr;
}

void Channel::ClearBlocked( bool pending )
{
    // Kill all blocked tasklets
    Tasklet* current = m_firstBlockedOnReceive;

	while( current )
    {
		current->Kill( pending );

        current = current->NextBlocked();
    }

    current = m_firstBlockedOnSend;

    while( current )
	{
		current->Kill( pending );

        current = current->NextBlocked();
	}

}

long Channel::NumberOfActiveChannels()
{
	return static_cast<long>(s_activeChannels.size());
}

int Channel::UnblockAllActiveChannels()
{
	int numberOfChannelsUnblocked = 0;

	auto iter = s_activeChannels.begin();

	std::vector<Channel*> channelsToUnblock;

	while(iter != s_activeChannels.end())
	{
		Channel* channel = *iter;
		if (channel->m_balance != 0)
		{
			channelsToUnblock.push_back(channel);
		}
		iter++;
	}

	for (auto chan : channelsToUnblock)
	{
		numberOfChannelsUnblocked++;

		chan->ClearBlocked( false );
	}

    return numberOfChannelsUnblocked;
}

void Channel::Close()
{
	PyThread_acquire_lock( m_lock, 1 );

	m_closing = true;

    UpdateCloseState();

    PyThread_release_lock( m_lock );
}

void Channel::Open()
{
	m_closing = false;

	m_closed = false;
}

bool Channel::IsClosed() const
{
	return m_closed;
}

bool Channel::IsClosing() const
{
	return m_closing;
}

void Channel::IncrementBalance()
{
	m_balance++;

    UpdateCloseState();
}

void Channel::DecrementBalance()
{
	m_balance--;

    UpdateCloseState();
}

void Channel::UpdateCloseState()
{
    // If channel is set to close and the balance is zero then set as closed

	if((m_closing) && ( m_balance == 0 ))
	{

		m_closed = true;

	}
}

// Called by module destructor
void Channel::Clean()
{
	Py_XDECREF( s_callbackArguments );
}

int Channel::DirectionToInt( ChannelDirection preference ) const
{
    switch (preference)
	{
	
        case ChannelDirection::RECEIVER:
		    return -1;

        case ChannelDirection::NEITHER:
            return 0;

        case ChannelDirection::SENDER:
			return 1;

        default:
			return -1;

    }
}

ChannelDirection Channel::DirectionFromInt( int preference ) const
{
	switch( preference )
	{
        case -1: 
	        return ChannelDirection::RECEIVER;

        case 0:
			return ChannelDirection::NEITHER;

        case 1:
			return ChannelDirection::SENDER;

        default:
			return ChannelDirection::RECEIVER;
	}
}

ChannelDirection Channel::InvertDirection( ChannelDirection direction ) const
{
	switch( direction )
	{
	case ChannelDirection::RECEIVER:
		return ChannelDirection::SENDER;

	case ChannelDirection::SENDER:
		return ChannelDirection::RECEIVER;

	case ChannelDirection::NEITHER:
		return ChannelDirection::NEITHER;

    default:
		return ChannelDirection::NEITHER;
	}
}