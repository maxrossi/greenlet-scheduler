#ifndef Py_SCHEDULER_H
#define Py_SCHEDULER_H
#ifdef __cplusplus
extern "C"
{
#endif

/* Header file for schedulermodule */

/* C API functions */
class PyTasklet;
class PyChannel;

// Tasklet Functions
#define PyTasklet_Check_NUM 0
#define PyTasklet_Check_RETURN int
#define PyTasklet_Check_PROTO ( PyTasklet* )

#define PyTasklet_Setup_NUM 1
#define PyTasklet_Setup_RETURN int
#define PyTasklet_Setup_PROTO ( PyTasklet*, PyObject* args, PyObject* kwds )

#define PyTasklet_Insert_NUM 2
#define PyTasklet_Insert_RETURN int
#define PyTasklet_Insert_PROTO ( PyTasklet* )

#define PyTasklet_GetBlockTrap_NUM 3
#define PyTasklet_GetBlockTrap_RETURN int
#define PyTasklet_GetBlockTrap_PROTO ( PyTasklet* )

#define PyTasklet_SetBlockTrap_NUM 4
#define PyTasklet_SetBlockTrap_RETURN void
#define PyTasklet_SetBlockTrap_PROTO ( PyTasklet*, int )

#define PyTasklet_IsMain_NUM 5
#define PyTasklet_IsMain_RETURN int
#define PyTasklet_IsMain_PROTO ( PyTasklet* )

// Channel Functions
#define PyChannel_New_NUM 6
#define PyChannel_New_RETURN PyChannel*
#define PyChannel_New_PROTO ( PyTypeObject* )

#define PyChannel_Send_NUM 7
#define PyChannel_Send_RETURN int
#define PyChannel_Send_PROTO ( PyChannel*, PyObject* )

#define PyChannel_Receive_NUM 8
#define PyChannel_Receive_RETURN PyObject*
#define PyChannel_Receive_PROTO ( PyChannel* )

#define PyChannel_SendException_NUM 9
#define PyChannel_SendException_RETURN int
#define PyChannel_SendException_PROTO ( PyChannel*, PyObject*, PyObject* )

#define PyChannel_GetQueue_NUM 10
#define PyChannel_GetQueue_RETURN PyObject*
#define PyChannel_GetQueue_PROTO ( PyChannel* )

#define PyChannel_SetPreference_NUM 11
#define PyChannel_SetPreference_RETURN void
#define PyChannel_SetPreference_PROTO ( PyChannel*, int )

#define PyChannel_GetBalance_NUM 12
#define PyChannel_GetBalance_RETURN int
#define PyChannel_GetBalance_PROTO ( PyChannel* )

// Scheduler Functions
#define PyScheduler_Schedule_NUM 13
#define PyScheduler_Schedule_RETURN PyObject*
#define PyScheduler_Schedule_PROTO ( PyObject*, int )

#define PyScheduler_GetRunCount_NUM 14
#define PyScheduler_GetRunCount_RETURN int
#define PyScheduler_GetRunCount_PROTO ( )

#define PyScheduler_GetCurrent_NUM 15
#define PyScheduler_GetCurrent_RETURN PyObject*
#define PyScheduler_GetCurrent_PROTO ( )

#define PyScheduler_RunWatchdogEx_NUM 16
#define PyScheduler_RunWatchdogEx_RETURN PyObject*
#define PyScheduler_RunWatchdogEx_PROTO ( long,int )

/* Total number of C API pointers */
#define PyScheduler_API_pointers 17


#ifdef SCHEDULER_MODULE
	/* This section is used when compiling schedulermodule.c */


    // Tasklet Functions
	static PyTasklet_Check_RETURN PyTasklet_Check PyTasklet_Check_PROTO;

	static PyTasklet_Setup_RETURN PyTasklet_Setup PyTasklet_Setup_PROTO;

    static PyTasklet_Insert_RETURN PyTasklet_Insert PyTasklet_Insert_PROTO;

    static PyTasklet_GetBlockTrap_RETURN PyTasklet_GetBlockTrap PyTasklet_GetBlockTrap_PROTO;

    static PyTasklet_SetBlockTrap_RETURN PyTasklet_SetBlockTrap PyTasklet_SetBlockTrap_PROTO;

    static PyTasklet_IsMain_RETURN PyTasklet_IsMain PyTasklet_IsMain_PROTO;

    // Channel Functions
	static PyChannel_New_RETURN PyChannel_New PyChannel_New_PROTO;

    static PyChannel_Send_RETURN PyChannel_Send PyChannel_Send_PROTO;

    static PyChannel_Receive_RETURN PyChannel_Receive PyChannel_Receive_PROTO;

    static PyChannel_SendException_RETURN PyChannel_SendException PyChannel_SendException_PROTO;

    static PyChannel_GetQueue_RETURN PyChannel_GetQueue PyChannel_GetQueue_PROTO;

    static PyChannel_SetPreference_RETURN PyChannel_SetPreference PyChannel_SetPreference_PROTO;

    // Scheduler Functions
    static PyScheduler_Schedule_RETURN PyScheduler_Shedule PyScheduler_Schedule_PROTO;

    static PyScheduler_GetRunCount_RETURN PyScheduler_GetRunCount PyScheduler_GetRunCount_PROTO;

	static PyScheduler_GetCurrent_RETURN PyScheduler_GetCurrent PyScheduler_GetCurrent_PROTO;

    static PyScheduler_RunWatchdogEx_RETURN PyScheduler_RunWatchdogEx PyScheduler_RunWatchdogEx_PROTO;


#else

/* This section is used in modules that use schedulermodule's API */

static void** PyScheduler_API;

// Tasklet Functions
#define PyTasklet_Check \
	( *(PyTasklet_Check_RETURN( * ) PyTasklet_Check_PROTO)PyScheduler_API[PyTasklet_Check_NUM] )

#define PyTasklet_Setup \
	( *(PyTasklet_Setup_RETURN( * ) PyTasklet_Setup_PROTO)PyScheduler_API[PyTasklet_Setup_NUM] )

#define PyTasklet_Insert \
	( *(PyTasklet_Insert_RETURN( * ) PyTasklet_Insert_PROTO)PyScheduler_API[PyTasklet_Insert_NUM] )

#define PyTasklet_GetBlockTrap \
	( *(PyTasklet_GetBlockTrap_RETURN( * ) PyTasklet_GetBlockTrap_PROTO)PyScheduler_API[PyTasklet_GetBlockTrap_NUM] )

#define PyTasklet_SetBlockTrap \
	( *(PyTasklet_SetBlockTrap_RETURN( * ) PyTasklet_SetBlockTrap_PROTO)PyScheduler_API[PyTasklet_SetBlockTrap_NUM] )

#define PyTasklet_IsMain \
	( *(PyTasklet_IsMain_RETURN( * ) PyTasklet_IsMain_PROTO)PyScheduler_API[PyTasklet_IsMain_NUM] )

// Channel Functions
#define PyChannel_New \
	( *(PyChannel_New_RETURN( * ) PyChannel_New_PROTO)PyScheduler_API[PyChannel_New_NUM] )

#define PyChannel_Send \
	( *(PyChannel_Send_RETURN( * ) PyChannel_Send_PROTO)PyScheduler_API[PyChannel_Send_NUM] )

#define PyChannel_Receive \
	( *(PyChannel_Receive_RETURN( * ) PyChannel_Receive_PROTO)PyScheduler_API[PyChannel_Receive_NUM] )

#define PyChannel_SendException \
	( *(PyChannel_SendException_RETURN( * ) PyChannel_SendException_PROTO)PyScheduler_API[PyChannel_SendException_NUM] )

#define PyChannel_GetQueue \
	( *(PyChannel_GetQueue_RETURN( * ) PyChannel_GetQueue_PROTO)PyScheduler_API[PyChannel_GetQueue_NUM] )

#define PyChannel_SetPreference \
	( *(PyChannel_SetPreference_RETURN( * ) PyChannel_SetPreference_PROTO)PyScheduler_API[PyChannel_SetPreference_NUM] )

// Scheduler Functions
#define PyScheduler_Schedule \
	( *(PyScheduler_Schedule_RETURN( * ) PyScheduler_Schedule_PROTO)PyScheduler_API[PyScheduler_Schedule_NUM] )

#define PyScheduler_GetRunCount \
	( *(PyScheduler_GetRunCount_RETURN( * ) PyScheduler_GetRunCount_PROTO)PyScheduler_API[PyScheduler_GetRunCount_NUM] )

#define PyScheduler_GetCurrent \
	( *(PyScheduler_GetCurrent_RETURN( * ) PyScheduler_GetCurrent_PROTO)PyScheduler_API[PyScheduler_GetCurrent_NUM] )

#define PyScheduler_RunWatchdogEx \
	( *(PyScheduler_RunWatchdogEx_RETURN( * ) PyScheduler_RunWatchdogEx_PROTO)PyScheduler_API[PyScheduler_RunWatchdogEx_NUM] )


/* Return -1 on error, 0 on success.
 * PyCapsule_Import will set an exception if there's an error.
 */
static int
	import_scheduler( void )
{
	PyScheduler_API = (void**)PyCapsule_Import( "scheduler._C_API", 0 );
	return ( PyScheduler_API != NULL ) ? 0 : -1;
}

#endif

#ifdef __cplusplus
}
#endif

#endif /* !defined(Py_SCHEDULERMODULE_H) */