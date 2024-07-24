/* 
	*************************************************************************

	ScheduleManager.h

	Author:    James Hawk
	Created:   Feb. 2024
	Project:   Scheduler

	Description:   

	  Functionality related to scheduling tasklets

	(c) CCP 2024

	*************************************************************************
*/
#pragma once
#ifndef ScheduleManager_H
#define ScheduleManager_H

#include "stdafx.h"

#include "PythonCppType.h"

#include <map>
#include <chrono>

typedef int( schedule_hook_func )( struct PyTaskletObject* from, struct PyTaskletObject* to );  // TODO remove redef

class Tasklet;

class ScheduleManager : public PythonCppType
{
public:
	ScheduleManager( PyObject* pythonObject );

	~ScheduleManager();

    static int NumberOfActiveScheduleManagers();

    static ScheduleManager* GetThreadScheduleManager();

	void SetCurrentTasklet( Tasklet* tasklet );

	Tasklet* GetCurrentTasklet();

    bool RemoveTasklet( Tasklet* tasklet );

    void InsertTaskletAtBeginning( Tasklet* tasklet );

    void InsertTasklet( Tasklet* tasklet );

    int GetCachedTaskletCount();

    int GetCalculatedTaskletCount();

    bool Schedule(bool remove = false);

    bool Yield();

    bool RunNTasklets( int n );

    bool RunTaskletsForTime( long long timeout );

    bool Run( Tasklet* startTasklet = nullptr );

    Tasklet* GetMainTasklet();

    void SetSchedulerFastCallback( schedule_hook_func* func );

    static void SetSchedulerCallback( PyObject* callback );

    void RunSchedulerCallback( Tasklet* previous, Tasklet* next );

    static PyObject* SchedulerCallback();

    bool IsSwitchTrapped();

    int SwitchTrapLevel();

    void SetSwitchTrapLevel( int level );

    void CreateSchedulerTasklet();

public:

    inline static PyTypeObject* s_taskletType;

    inline static PyTypeObject* s_scheduleManagerType;

    inline static PyThread_type_lock s_scheduleManagerLock;

    inline static Py_tss_t s_threadLocalStorageKey = Py_tss_NEEDS_INIT;

private:

    long m_threadId;

    Tasklet* m_schedulerTasklet;

    Tasklet* m_currentTasklet; //Weak ref

    Tasklet* m_previousTasklet; //Weak ref

    long m_switchTrapLevel;

	inline static PyObject* s_schedulerCallback = nullptr; // This is global, not per schedule manager

    schedule_hook_func* m_schedulerFastCallback;

    int m_taskletLimit;

    long long m_totalTaskletRunTimeLimit;

    std::chrono::steady_clock::time_point m_startTime;

    bool m_stopScheduler;

    int m_numberOfTaskletsInQueue;

    static inline int s_numberOfActiveScheduleManagers = 0;
    
};

#endif // ScheduleManager_H