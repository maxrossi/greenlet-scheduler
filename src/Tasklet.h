/* 
	*************************************************************************

	Tasklet.h

	Author:    James Hawk
	Created:   Feb. 2024
	Project:   Scheduler

	Description:   

	  Functions related to tasklets

	(c) CCP 2024

	*************************************************************************
*/
#pragma once
#ifndef Tasklet_H
#define Tasklet_H

#include "stdafx.h"

class Channel;
class ScheduleManager;

class Tasklet
{
public:

	Tasklet( PyObject* pythonObject, PyObject* taskletExitException, bool isMain );

    ~Tasklet();

    PyObject* PythonObject();

    void Incref();

	void Decref();

    int ReferenceCount();

	void SetToCurrentGreenlet();

    bool Remove();

    bool Initialise();

    void Uninitialise();

	bool Insert();

    bool SwitchImplementation();

    bool SwitchTo();

    bool Run();

    bool Kill( bool pending = false );

    PyObject* GetTransferArguments();

	void ClearTransferArguments();

	void SetTransferArguments( PyObject * args, PyObject* exception, bool transfer_exception_is_from_send_throw );

    void Block( Channel* channel );

    void Unblock();

    bool IsBlocked() const;

    bool IsOnChannelBlockList() const;

    void SetAlive( bool value );

    bool IsAlive() const;

    bool IsScheduled() const;

    void SetScheduled( bool value );

    bool IsBlocktrapped() const;

    void SetBlocktrap( bool value );

    bool IsMain() const;

    void MarkAsMain( bool value );

    unsigned long ThreadId() const;

	Tasklet* Next() const;

	void SetNext( Tasklet* next );

    Tasklet* Previous() const;

	void SetPrevious( Tasklet* previous );

    Tasklet* NextBlocked() const;

    void SetNextBlocked(Tasklet* next);

    Tasklet* PreviousBlocked() const;

    void SetPreviousBlocked( Tasklet* previous );

    PyObject* Arguments() const;

    void SetArguments(PyObject* arguments);

    PyObject* KwArguments() const;

    void SetKwArguments( PyObject* kwarguments );

    bool TransferInProgress() const;

    void SetTransferInProgress( bool value );

    PyObject* TransferException() const;

    bool transfer_exception_is_from_send_throw() const;

    bool ThrowException( PyObject* exception, PyObject* value, PyObject* tb, bool pending );

    bool IsPaused();

    Tasklet* GetParent();

    int SetParent( Tasklet* parent );

    void ClearParent();

    bool TaskletExceptionRaised();

    void ClearTaskletException();

    void SetReschedule( bool value );

    bool RequiresReschedule();

    void SetTaggedForRemoval( bool value );

    void SetCallable( PyObject* callable );

    bool RequiresRemoval();

    int GetBlockedDirection();

    void SetBlockedDirection( int direction );

    PyGreenlet* get_greenlet();

private:

    void SetExceptionState( PyObject* exception, PyObject* arguments = Py_None );

	void SetPythonExceptionStateFromTaskletExceptionState();

    void ClearException();

private:

    PyObject* m_pythonObject;

	PyGreenlet* m_greenlet;

	PyObject* m_callable;

	PyObject* m_arguments;

    PyObject* m_kwArguments;

    bool m_isMain;

    bool m_transferInProgress;;

    bool m_scheduled;

	bool m_alive;

    bool m_blocktrap;

    Tasklet* m_previous;

    Tasklet* m_next;

    Tasklet* m_nextBlocked;

	Tasklet* m_previousBlocked;

    unsigned long m_threadId;

    PyObject* m_transferArguments;

    PyObject* m_transferException;

	bool m_transfer_exception_is_from_send_throw;

    Channel* m_channelBlockedOn;

	bool m_blocked;

    // TODO should be handled as an enum rather than int
    int m_blockedDirection;

    bool m_hasStarted;

    bool m_paused;

    bool m_firstRun;

    bool m_reschedule;

    bool m_remove;

    bool m_taggedForRemoval;  // This flag set will ensure that the tasklet doesn't get marked as not alive

    Tasklet* m_taskletParent; // Weak ref

    //Exception
    PyObject* m_exceptionState;
	PyObject* m_exceptionArguments;

    PyObject* m_taskletExitException; //Weak ref

    ScheduleManager* m_scheduleManager;

    bool m_killPending;
};

#endif // Tasklet_H