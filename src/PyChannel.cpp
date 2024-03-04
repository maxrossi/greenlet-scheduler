#include "PyChannel.h"

#include "PyTasklet.h"

#include "PyScheduler.h"



bool PyChannelObject::send( PyObject* args  )
{
    PyThread_acquire_lock( m_lock, 1 );

    PyObject* current2 = Scheduler::get_current_tasklet();  //TODO naming !!

    reinterpret_cast<PyTaskletObject*>( current2 )->m_transfer_in_progress = true;

	if( m_waiting_to_receive->size() == 0 )
	{
		
		// Block as there is no tasklet sending
		PyObject* current = Scheduler::get_current_tasklet();

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
		m_waiting_to_send->push( current );
		reinterpret_cast<PyTaskletObject*>( current )->m_blocked = true;

        PyThread_release_lock( m_lock );

         // Continue scheduler
		Scheduler::schedule();

        PyThread_acquire_lock( m_lock, 1 );

	}

    PyTaskletObject* receiving_tasklet = (PyTaskletObject*)m_waiting_to_receive->front();

	m_waiting_to_receive->pop();

	receiving_tasklet->m_blocked = false;
	
    // Store for retrieval from receiving tasklet
	receiving_tasklet->set_transfer_arguments( args );

    //Add this tasklet to the end of the scheduler
	Scheduler::insert_tasklet( reinterpret_cast<PyTaskletObject*>( Scheduler::get_current_tasklet() ) );

    PyThread_release_lock( m_lock );

    //Switch to the receiving tasklet
    receiving_tasklet->switch_to( );

    Py_DECREF( receiving_tasklet );

	reinterpret_cast<PyTaskletObject*>( current2 )->m_transfer_in_progress = false;

	return true;

}

PyObject* PyChannelObject::receive()
{
	PyThread_acquire_lock( m_lock, 1 );

    reinterpret_cast<PyTaskletObject*>( Scheduler::get_current_tasklet() )->m_transfer_in_progress = true;

	// Block as there is no tasklet sending
	PyObject* current = Scheduler::get_current_tasklet();
    

    if( current == nullptr )
	{
		PyErr_SetString( PyExc_RuntimeError, "No current tasklet set" );
		return nullptr;
	}

	Py_IncRef( current );

	m_waiting_to_receive->push( current );

    if(m_waiting_to_send->size() == 0)
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
			return false;
		}

		reinterpret_cast<PyTaskletObject*>(current)->m_blocked = true;

        PyThread_release_lock( m_lock );

        // Continue scheduler
		Scheduler::schedule();
	
    }
	else
	{
		//Get first
		PyTaskletObject* sending_tasklet = (PyTaskletObject*)m_waiting_to_send->front();
        
		m_waiting_to_send->pop();
		sending_tasklet->m_blocked = false;

        PyThread_release_lock( m_lock );

		sending_tasklet->switch_to();

        Py_DECREF( sending_tasklet );
	}

    reinterpret_cast<PyTaskletObject*>( current )->m_transfer_in_progress = false;

    return reinterpret_cast<PyTaskletObject*>(current)->get_transfer_arguments();
}

int PyChannelObject::balance()
{
    return m_waiting_to_send->size() - m_waiting_to_receive->size();
}

void PyChannelObject::remove_tasklet_from_blocked( PyObject* tasklet )
{
    // TODO Needs thought so it isn't slow
    
}
