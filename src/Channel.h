/* 
	*************************************************************************

	Channel.h

	Author:    James Hawk
	Created:   Feb. 2024
	Project:   Scheduler

	Description:   

	  Functionality related to communication between tasklets

	(c) CCP 2024

	*************************************************************************
*/
#pragma once
#ifndef Channel_H
#define Channel_H

#include "stdafx.h"

#include <list>

const int SENDER = 1;
const int RECEIVER = -1;
const int PREFER_NEITHER = 0;

class Tasklet;

class Channel
{
public:
	
	Channel( PyObject* pythonObject );

    ~Channel();

    void Incref();

	void Decref();

    int ReferenceCount();

    PyObject* PythonObject();

	bool Send( PyObject* args, PyObject* exception = nullptr );

    PyObject* Receive();

    int Balance() const;

    void UnblockTaskletFromChannel( Tasklet* tasklet );

    static PyObject* ChannelCallback();

    static void SetChannelCallback(PyObject* callback);

    int Preference() const;

    void SetPreference( int value );

    Tasklet* BlockedQueueFront();

    void ClearBlocked(bool pending);

    void Close();

    void Open();

    bool IsClosed();

	bool IsClosing(); 

    static int NumberOfActiveChannels();

    static int UnblockAllActiveChannels();

    static void Clean();

private:

    void RemoveTaskletFromBlocked( Tasklet* tasklet );

    void IncrementBalance();

	void DecrementBalance();

    void RunChannelCallback( Channel* channel, Tasklet* tasklet, bool sending, bool willBlock ) const;

    void AddTaskletToWaitingToSend( Tasklet* tasklet );

    void AddTaskletToWaitingToReceive( Tasklet* tasklet );

    Tasklet* PopNextTaskletBlockedOnSend();

    Tasklet* PopNextTaskletBlockedOnReceive();

    bool ChannelSwitch( Tasklet* caller, Tasklet* other, int dir, int callerDir );

    void UpdateCloseState();

private:

    PyObject* m_pythonObject;

    int m_balance;

	int m_preference;

    bool m_closing;

    bool m_closed;

    PyThread_type_lock m_lock;

    inline static PyObject* s_channelCallback; // This is global, not per channel

    inline static PyObject* s_callbackArguments = nullptr;

    Tasklet* m_firstBlockedOnReceive;

	Tasklet* m_lastBlockedOnReceive;

	Tasklet* m_firstBlockedOnSend;

    Tasklet* m_lastBlockedOnSend;

    inline static std::list<Channel*> s_activeChannels;

    
    
};

#endif // Channel_H
