#include "PyChannel.h"

#include "PyTasklet.h"

#include "PyScheduler.h"

bool PyChannelObject::send( PyObject* args  )
{

	if( m_waiting_to_receive->size() == 0 )
	{
		// Block as there is no tasklet sending
		PyObject* current = PySchedulerObject::get_current_tasklet();

        if( !current )
		{
			PyErr_SetString( PyExc_RuntimeError, "No current tasklet set" );
			return false;
		}

        //If current tasklet is main tasklet then throw runtime error
		if( current == PySchedulerObject::get_main_tasklet() )
		{
			PyErr_SetString( PyExc_RuntimeError, "Channel cannot block on main tasklet" );
			return false;
        }

        //If current tasklet has block_trap set to true then throw runtime error
		if( reinterpret_cast<PyTaskletObject*>( current )->m_blocktrap)
		{
			PyErr_SetString( PyExc_RuntimeError, "Channel cannot block on main tasklet with block_trap set true" );
			return false;
		}

		// Block as there is no tasklet receiving
		Py_IncRef( current );
		m_waiting_to_send->push( current );
		reinterpret_cast<PyTaskletObject*>( current )->m_blocked = true;

         // Continue scheduler
		PySchedulerObject::schedule();
	}

    PyTaskletObject* receiving_tasklet = (PyTaskletObject*)m_waiting_to_receive->front();
	m_waiting_to_receive->pop();
	receiving_tasklet->m_blocked = false;

    // Store for retrieval from receiving tasklet
	Py_XDECREF( m_transfer_arguments );
    Py_IncRef( args );
	m_transfer_arguments = args;

    //Add this tasklet to the end of the scheduler
	PySchedulerObject::insert_tasklet( reinterpret_cast<PyTaskletObject*>( PySchedulerObject::get_current_tasklet() ) );

    //Switch to the receiving tasklet
    receiving_tasklet->switch_to( );

    Py_DECREF( receiving_tasklet );

	return true;

}

PyObject* PyChannelObject::receive()
{
	// Block as there is no tasklet sending
	PyObject* current = PySchedulerObject::get_current_tasklet();
    

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
		if( current == PySchedulerObject::get_main_tasklet() )
		{
			PyErr_SetString( PyExc_RuntimeError, "Channel cannot block on main tasklet" );
			return nullptr;
		}

		//If current tasklet has block_trap set to true then throw runtime error
		if( reinterpret_cast<PyTaskletObject*>( current )->m_blocktrap )
		{
			PyErr_SetString( PyExc_RuntimeError, "Channel cannot block on main tasklet with block_trap set true" );
			return false;
		}


		reinterpret_cast<PyTaskletObject*>(current)->m_blocked = true;

        // Continue scheduler
		PySchedulerObject::schedule();
		
    }
	else
	{
		//Get first
		PyTaskletObject* sending_tasklet = (PyTaskletObject*)m_waiting_to_send->front();
        
		m_waiting_to_send->pop();
		sending_tasklet->m_blocked = false;

        //Add this tasklet to the end of the scheduler
		PySchedulerObject::insert_tasklet( reinterpret_cast<PyTaskletObject*>( PySchedulerObject::get_current_tasklet() ) );

		sending_tasklet->switch_to();

        Py_DECREF( sending_tasklet );
	}

    return m_transfer_arguments;
}

int PyChannelObject::balance()
{
    return m_waiting_to_send->size() - m_waiting_to_receive->size();
}

void PyChannelObject::remove_tasklet_from_blocked( PyObject* tasklet )
{
    // TODO Needs thought so it isn't slow
    
}
