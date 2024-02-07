#ifndef Py_SCHEDULER_H
#define Py_SCHEDULER_H
#ifdef __cplusplus
extern "C"
{
#endif

/* Header file for schedulermodule */

/* C API functions */
#define PyScheduler_GetCurrent_NUM 0
#define PyScheduler_GetCurrent_RETURN PyObject*
#define PyScheduler_GetCurrent_PROTO ( )

/* Total number of C API pointers */
#define PyScheduler_API_pointers 1


#ifdef SCHEDULER_MODULE
	/* This section is used when compiling schedulermodule.c */

	class PyTasklet;

	static PyScheduler_GetCurrent_RETURN PyScheduler_GetCurrent PyScheduler_GetCurrent_PROTO;

#else
	class PyTasklet;

/* This section is used in modules that use schedulermodule's API */

static void** PyScheduler_API;

#define PyScheduler_GetCurrent \
	( *(PyScheduler_GetCurrent_RETURN( * ) PyScheduler_GetCurrent_PROTO)PyScheduler_API[PyScheduler_GetCurrent_NUM] )

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