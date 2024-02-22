#include "PyChannel.h"

#include "PyTasklet.h"

#include "PyScheduler.h"

bool PyChannelObject::send( PyObject* args  )
{

	if( m_waiting_to_receive->size() == 0 )
	{
		// Block as there is no tasklet sending
		PyTaskletObject* current = PyTaskletObject::s_current;

		if( !current )
		{
			PyErr_SetString( PyExc_RuntimeError, "No current tasklet set" );
			return false;
		}

		// Block as there is no tasklet receiving
		Py_IncRef( (PyObject*)current );
		m_waiting_to_send->push( (PyObject*)current );
		current->m_blocked = true;

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
	PySchedulerObject::insert_tasklet( PyTaskletObject::s_current );

    //Switch to the receiving tasklet
    receiving_tasklet->switch_to( );

    Py_DECREF( receiving_tasklet );

	return false;

}

PyObject* PyChannelObject::receive()
{
	// Block as there is no tasklet sending
	if( PyTaskletObject::s_current == nullptr )
	{
		PyErr_SetString( PyExc_RuntimeError, "No current tasklet set" );
		return nullptr;
	}

    PyTaskletObject* current = PyTaskletObject::s_current;
	Py_IncRef( (PyObject*)current );
	m_waiting_to_receive->push( (PyObject*)current );

    if(m_waiting_to_send->size() == 0)
	{

		current->m_blocked = true;

        // Continue scheduler
		PySchedulerObject::schedule();
		
    }
	else
	{
		//Get first
		PyTaskletObject* sending_tasklet = (PyTaskletObject*)m_waiting_to_send->front();
		m_waiting_to_send->pop();
		sending_tasklet->m_blocked = false;

		sending_tasklet->switch_to();

        Py_DECREF( sending_tasklet );
	}

    return m_transfer_arguments;
}

int PyChannelObject::balance()
{
	int num_waiting_to_send = m_waiting_to_send->size();
	int num_waiting_to_receive = -m_waiting_to_receive->size();

    return num_waiting_to_send - num_waiting_to_receive;
}