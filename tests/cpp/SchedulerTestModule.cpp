#define PY_SSIZE_T_CLEAN
#include "stdafx.h"

#define SCHEDULERTEST_MODULE
#include "SchedulerTestModule.h"
#include <SchedulerModule.h>

static PyObject*
	run_test( PyObject* self, PyObject* args )
{
	PySys_WriteStdout("Running test \n");

    PySys_WriteStdout( "Importing scheduler \n" );
	PyObject* scheduler_module = PyImport_ImportModule( "scheduler" );

	if( !scheduler_module )
	{
		PySys_WriteStdout( "Failed to import scheduler module\n" );
	}

    PySys_WriteStdout( "Importing capsule \n" );

	int ret = import_scheduler();

	if( ret != 0 )
	{
		PySys_WriteStdout( "Failed to import scheduler capsule\n" );
		PyErr_Print();
	}

    PySys_WriteStdout( "Testing exposed functions \n" );

	PyScheduler_GetCurrent();

    PySys_WriteStdout( "Done \n" );

	return Py_None;
}

static PyMethodDef SchedulerTestMethods[] = {
	{ "test", run_test, METH_VARARGS, "Run a test which uses all the exposed module methods" },
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


PyMODINIT_FUNC
PyInit__schedulertest(void)
{
    PyObject *m;
	m = PyModule_Create( &schedulertestmodule );
    if (m == NULL)
        return NULL;


    return m;
}

