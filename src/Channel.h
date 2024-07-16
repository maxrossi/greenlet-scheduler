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

#include "PythonCppType.h"

#include <list>

enum class ChannelDirection
{
    SENDER,
    RECEIVER,
    NEITHER
};

typedef ChannelDirection ChannelPreference;

class Tasklet;

class Channel : public PythonCppType
{
public:
	
	Channel( PyObject* pythonObject );

    ~Channel();

	bool Send( PyObject* args, PyObject* exception = nullptr );

    PyObject* Receive();

    int Balance() const;

    void UnblockTaskletFromChannel( Tasklet* tasklet );

    static PyObject* ChannelCallback();

    static void SetChannelCallback(PyObject* callback);

    int PreferenceAsInt() const;

    void SetPreferenceFromInt( int value );

    Tasklet* BlockedQueueFront() const;

    void ClearBlocked(bool pending);

    void Close();

    void Open();

    bool IsClosed() const;

	bool IsClosing() const; 

    static long NumberOfActiveChannels();

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

    bool ChannelSwitch( Tasklet* caller, Tasklet* other, ChannelDirection directionOfChannelOperation, ChannelDirection callerDir );

    void UpdateCloseState();

    int DirectionToInt( ChannelDirection preference ) const;

    ChannelDirection DirectionFromInt( int preference ) const;

    ChannelDirection InvertDirection( ChannelDirection direction ) const;

private:

    int m_balance;

	ChannelPreference m_preference;

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
