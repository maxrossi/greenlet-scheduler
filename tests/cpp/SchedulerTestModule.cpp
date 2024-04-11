#define PY_SSIZE_T_CLEAN
#include "stdafx.h"

#define SCHEDULERTEST_MODULE
#include "SchedulerTestModule.h"
#include <Scheduler.h>


static PyObject*
	test_PyScheduler_GetCurrent( PyObject* self, PyObject* args )
{
   
	SchedulerAPI()->PyScheduler_GetCurrent();

	return Py_None;
}

static PyObject*
	test_PyScheduler_Schedule( PyObject* self, PyObject* args )
{
    //TODO
	SchedulerAPI()->PyScheduler_Schedule( nullptr, 0 );

	return Py_None;
}

static PyMethodDef SchedulerTestMethods[] = {
	{ "PyScheduler_GetCurrent", test_PyScheduler_GetCurrent, METH_VARARGS, "Run a test which uses all the exposed module methods" },
	{ "PyScheduler_Schedule", test_PyScheduler_Schedule, METH_VARARGS, "Run a test which uses all the exposed module methods" },
	 { NULL, NULL, 0, NULL } /* Sentinel */
};

static struct PyModuleDef schedulertestmodule = {
    PyModuleDef_HEAD_INIT,
    "Scheduler Test",   /* name of module */
    NULL, /* module documentation, may be NULL */
    -1,       /* size of per-interpreter state of the module,
                 or -1 if the module keeps state in global variables. */
    SchedulerTestMethods
};

#define CONCATENATE_DIRECT(s1, s2) s1##s2
#define CONCATENATE(s1, s2) CONCATENATE_DIRECT(s1, s2)

PyMODINIT_FUNC CONCATENATE(PyInit__schedulertest, CCP_BUILD_FLAVOR) (void)
{
    PyObject *m;
	m = PyModule_Create( &schedulertestmodule );
    if (m == NULL)
        return NULL;


    PySys_WriteStdout( "Importing scheduler \n" );
	PyObject* scheduler_module = PyImport_ImportModule( "scheduler" );

	if( !scheduler_module )
	{
		PySys_WriteStdout( "Failed to import scheduler module\n" );
	}

	PySys_WriteStdout( "Importing capsule \n" );

	
    auto api = SchedulerAPI();

    if( api == nullptr )
	{
		PySys_WriteStdout( "Failed to import scheduler capsule\n" );
		PyErr_Print();
	}

    return m;
}

