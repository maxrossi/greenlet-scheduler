Memory Design
=============

This document outlines how memory reference counting is designed and show how the custom types interact on the memory level.

There are three custom types offered:
1. :doc:`../pythonApi/scheduleManager`
2. :doc:`../pythonApi/tasklet`
3. :doc:`../pythonApi/channel`



The ScheduleManager
-------------------

The central component being the :doc:`../pythonApi/scheduleManager`.

:doc:`../pythonApi/scheduleManager` objects are bound to a Python thread.

To match `Stackless Python <https://stackless.readthedocs.io/en/3.8-slp/stackless-python.html>`_ :doc:`../pythonApi/scheduleManager` are created on demand per thread.

Therefore if an oporation which requires a :doc:`../pythonApi/scheduleManager` is performed on a Python thread and one isn't already present, one will be created on demand.

Each :doc:`../pythonApi/scheduleManager` requires its own :doc:`../pythonApi/tasklet`, this is commonly referred to as the Main :doc:`../pythonApi/tasklet`. (see :doc:`../guides/theMainTasklet`).

From a memory design point of view Main Tasklets are special in that they **do not** hold a strong reference to their associated :doc:`../pythonApi/scheduleManager`.

This is because the :doc:`../pythonApi/scheduleManager` holds a strong reference to its Main tasklet and that would produce a circular dependancy.

It is possible to request a strong reference to a :doc:`../pythonApi/scheduleManager` via :py:func:`scheduler.get_schedule_manager`. Keeping a strong reference will naturally prevent the :doc:`../pythonApi/scheduleManager` auto cleanup.

The :doc:`../pythonApi/scheduleManager` will hold a strong reference to any :doc:`../pythonApi/tasklet` added to its runnables queue. These will be released when removed from the queue.


Tasklets (Non Main)
-------------------

Tasklets are also bound to a Python thread.

when a :doc:`../pythonApi/tasklet` is created it stores a strong reference to the :doc:`../pythonApi/tasklet` of the same Python thread.

This is only released when the :doc:`../pythonApi/tasklet` is destroyed.

This means that while a refernce to a :doc:`../pythonApi/tasklet` on a Python thread exits the :doc:`../pythonApi/scheduleManager` will be kept alive.

The :doc:`../pythonApi/tasklet` can even be outside the :doc:`../pythonApi/scheduleManager` for example after a :py:func:`scheduler.tasklet.schedule_remove`.

Once all Tasklets in a thread and all manual references to :doc:`../pythonApi/scheduleManager` are destroyed then the :doc:`../pythonApi/scheduleManager` will automatically clean up.


Channels
--------

All reference to channels are managed by the user.

Channels hold a store of Tasklets that are 'blocked' on them (see :doc:`../guides/sendingDataBetweenTaskletsUsingChannels).

When a :doc:`../pythonApi/tasklet` is 'blocked' on a :doc:`../pythonApi/channel` the :doc:`../pythonApi/channel` will hold a strong reference to it to keep it alive.

If a :doc:`../pythonApi/tasklet` is 'unblocked' due to a completed data transfer the strong reference is removed.


Greenlet and References
-----------------------

When a Tasklet has yielded before completing Greenlet will hold references to objects related to the call currently yielded on. Arguments passed to the callable will also only be released on the :doc:`../pythonApi/tasklet` completing.


Loosing a reference to a objects until module teardown
------------------------------------------------------
It is possible to loose all references to objects that are part of an unfinished Tasklet oporation.

The objects will then not be released until module teardown when Greenlet quits.

eg.

.. code-block:: python
   
    c = scheduler.channel()

    def foo(channel):
        channel.receive()

    t = scheduler.tasklet(foo)()

    t.run()

    t = None
    c = None


1. :doc:`../pythonApi/tasklet` ``t`` is run using :py:func:`scheduler.tasklet.run`.
2. :py:func:`scheduler.channel.receive` causes ``t`` to yield and so ``t`` is added to the channels block list which will store a strong reference of ``t``.
3. ``t = None`` is set to ``None``. ``t`` still has a reference on the :doc:`../pythonApi/channel` as it is in the 'blocked' list. The loss of the reference is no big deal, a call to :py:func:`scheduler.channel.send` will still continue execution of ``t``.
4. ``c = None`` is set to ``None`` which is our only reference to it. On the surface it looks like there are now no references to ``c`` remaining but this is incorrect.
5. The unfinished Greenlet function was passed ``c`` as an argument, this reference is still around. What's more the call to :py:func:`scheduler.channel.receive` is not complete and that unfinished function too holds a reference to ``c``.

As you can see at this point ``t`` and ``c`` are still alive and ``t`` is in an uncompletable state as the user cannot call :py:func:`scheduler.channel.send` on anything.

This is only a problem if the client code is incorrect.

It can cause a leak until the module is cleaned up, but this is only at full system tear down.



getcurrent and getmain can produce inconsistent behaviour
----------------------------------------------------------
See :ref:`divergence-scheduler-getcurrent-scheduler-getmain` 