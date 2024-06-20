Using Scheduler From C
======================

A :doc:`../cApi` is provided exposing much of the functionality to c.

Functions are provided via a Python Capsule which requires importing prior to use.

.. code-block:: c++

    #import <scheduler.h>

    // Importing the cextension
    scheduler_module = PyImport_ImportModule("_scheduler")

    // Importing the capsule
    m_scheduler_api = SchedulerAPI();

    // Get runnables queue size - example api usage
    int runcount = m_api->PyScheduler_GetRunCount();


Pyd naming convention
---------------------
Each build configuration creates a pyd file with a name which reflects its flavour.
This 'flavour' naming convention is specific to CCP Games.

+---------------+---------------------------+
| Build Type    | Pyd Name                  |
+===============+===========================+
| Release       | _scheduler                |
+---------------+---------------------------+
| Debug         | _scheduler_debug          |
+---------------+---------------------------+
| TrinityDebug  | _scheduler_trinitydebug   |
+---------------+---------------------------+
| Internal      | _scheduler_internal       |
+---------------+---------------------------+

As it can be inconvenient to deal with this it is often necessary to alias the module import to simply ``scheduler``

.. code-block:: c++

    PyObject* sysmodule = PyImport_ImportModule( "sys" );
    PyObject* dict = PyModule_GetDict( sysmodule );
    PyObject* modules = PyDict_GetItemString( dict, "modules" );
    PyDict_SetItemString( modules, "scheduler", scheduler_module );
    Py_DECREF( sysmodule );

Or in Python

.. code-block:: python

    import _scheduler_debug as scheduler

For more examples refer to the project gtests in the target ``SchedulerCapiTest``.