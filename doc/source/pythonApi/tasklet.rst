Tasklet
=======

Python Api for custom Tasklet type.

Refer to guide section :ref:`tasklet-guides` for further usage information.

Methods
-------
.. autofunction:: scheduler.tasklet.__init__

    Sets up tasklet and calls :py:func:`scheduler.tasklet.bind`

.. autofunction:: scheduler.tasklet.insert

    For further information see :doc:`../guides/understandingTaskletScheduleOrder`.

.. autofunction:: scheduler.tasklet.remove

.. autofunction:: scheduler.tasklet.run

    For further information see:
    
        :doc:`../guides/understandingTaskletScheduleOrder`.

        :doc:`../guides/schedulingAcrossMultiplePythonThreads`.


.. autofunction:: scheduler.tasklet.switch

    For further information regarding manual switching see :doc:`../guides/manualControlScheduling`.

.. autofunction:: scheduler.tasklet.throw

    :seealso: :py:func:`scheduler.TaskletExit`

    For further information see :doc:`../guides/howExceptionsAreManaged`.

.. autofunction:: scheduler.tasklet.raise_exception

    :seealso: :py:func:`scheduler.TaskletExit`

    For further information see :doc:`../guides/howExceptionsAreManaged`.

.. autofunction:: scheduler.tasklet.kill

    :seealso: :py:func:`scheduler.TaskletExit`

    For further information see :doc:`../guides/howExceptionsAreManaged`.

.. autofunction:: scheduler.tasklet.set_context

.. autofunction:: scheduler.tasklet.bind

.. autofunction:: scheduler.tasklet.__call__

    Calls :py:func:`scheduler.tasklet.setup`

.. autofunction:: scheduler.tasklet.setup

Attributes
----------

.. autoattribute:: scheduler.tasklet.alive

.. autoattribute:: scheduler.tasklet.blocked

    :seealso: :py:func:`scheduler.channel`

.. autoattribute:: scheduler.tasklet.scheduled

    For further information see :doc:`../guides/understandingTaskletScheduleOrder`.

.. autoattribute:: scheduler.tasklet.block_trap

    For further information see :doc:`../guides/restrictingTaskletControlFlow`.

.. autoattribute:: scheduler.tasklet.is_current

.. autoattribute:: scheduler.tasklet.is_main

.. autoattribute:: scheduler.tasklet.thread_id

    For further information see :doc:`../guides/schedulingAcrossMultiplePythonThreads`.

.. autoattribute:: scheduler.tasklet.next

.. autoattribute:: scheduler.tasklet.prev

.. autoattribute:: scheduler.tasklet.paused
