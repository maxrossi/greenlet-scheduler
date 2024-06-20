Stackless Divergence
====================

Lists occurances where ``carbon-scheduler`` diverges from `Stackless Python <https://stackless.readthedocs.io/en/3.8-slp/stackless-python.html>`_ behaviour.

The original aim was to match `Stackless Python <https://stackless.readthedocs.io/en/3.8-slp/stackless-python.html>`_ functionality, however some deviation is expected due to the nature of `Stackless Python <https://stackless.readthedocs.io/en/3.8-slp/stackless-python.html>`_ being built into the interpreter.

Over time it is expected that divergence will increase.


Overview of differences
-----------------------
* :ref:`divergence-scheduler-current-scheduler-main`
* :ref:`divergence-scheduler-getcurrent-scheduler-getmain` 



.. _divergence-scheduler-current-scheduler-main:
scheduler.current and scheduler.main are not implemented
--------------------------------------------------------
These attributes for current and main are not provided due to innability to keep them correct when used with threading.


**The Alternative**

use :py:func:`scheduler.getcurrent` and :py:func:`scheduler.getmain`. These will return the correct value for the Python thread which they were called.


.. _divergence-scheduler-getcurrent-scheduler-getmain:
Calling :py:func:`scheduler.getcurrent` or :py:func:`scheduler.getmain`  on Main Tasklet can give inconsistent results
----------------------------------------------------------------------------------------------------------------------

Due to how the :doc:`memoryDesign` works :py:func:`scheduler.getcurrent` can return different Tasklets when called on the Main Tasklet.

For this to be the case there needs to be no reference to any Tasklets held that had been created on the Main Tasklet of any Python Thread.

The below example will print ``Divergence`` in ``carbon-scheduler`` whereas the `Stackless Python <https://stackless.readthedocs.io/en/3.8-slp/stackless-python.html>`_ equivalent would not.

.. code-block:: python

   t1 = scheduler.getcurrent()

   t2 = scheduler.getcurrent()

   if t1 != t2:
      print("Divergence")
   else:
      print("OK")


This is because each thread creates a :doc:`../pythonApi/scheduleManager` on demand and will therefore also clean up an unused :doc:`../pythonApi/scheduleManager` after it is no longer used.

A :doc:`../pythonApi/scheduleManager` will stay alive while there are either:
1. References to it held by the user.
2. Tasklet references which are associated to the same Python thread.

The :doc:`../pythonApi/tasklet` returned in ``t1`` and ``t2`` are slightly special, they are :doc:`../pythonApi/scheduleManager` Tasklets most commonly referred to as Main tasklets (:doc:`../guides/theMainTasklet`).

Main Tasklets are special in that they **Do not** hold a reference to the :doc:`../pythonApi/scheduleManager` they are associated with.

Therefore, when ``t1 = scheduler.getcurrent()`` is called the following occcurs:
1. A :doc:`../pythonApi/scheduleManager` for the Python thread is created on demand.
2. A :doc:`../pythonApi/scheduleManager` :doc:`../pythonApi/tasklet` (Main Tasklet) is created.
3. A reference to the :doc:`../pythonApi/scheduleManager` :doc:`../pythonApi/tasklet` is increffed.
4. No further references to :doc:`../pythonApi/scheduleManager` exist so the :doc:`../pythonApi/scheduleManager` is cleaned up.
5. The :doc:`../pythonApi/scheduleManager` :doc:`../pythonApi/tasklet` is returned from the call to :py:func:`scheduler.getcurrent`.

So ``t1`` at this point is a :doc:`../pythonApi/scheduleManager` :doc:`../pythonApi/tasklet` (Main Tasklet) for a :doc:`../pythonApi/scheduleManager` that no longer exists.

Similarly the call to ``t2 = scheduler.getcurrent()`` will follow the exact same procedure, thus producing a different :doc:`../pythonApi/scheduleManager` :doc:`../pythonApi/tasklet` (Main Tasklet).

**The Alternative**

If a reference to the :doc:`../pythonApi/scheduleManager` is first held then both ``t1`` and ``t2`` will be the same :doc:`../pythonApi/scheduleManager` :doc:`../pythonApi/tasklet` as expected. This is due to the :doc:`../pythonApi/scheduleManager` not being cleaned up as there is an extra reference held.

To illustrate, the below example will print ``OK``.

.. code-block:: python

   s = scheduler.get_schedule_manager()

   t1 = scheduler.getcurrent()

   t2 = scheduler.getcurrent()

   if t1 != t2:
      print("Divergence")
   else:
      print("OK")

**Can it be fixed**

It is possible to get the behaviour to match `Stackless Python <https://stackless.readthedocs.io/en/3.8-slp/stackless-python.html>`_ but the change is not elegant and complicates the code.

If GC is turned on for Tasklets then they can manage their own destruction inside the traverse function:

To achieve this 

1. :py:func:`scheduler.getcurrent` needs to add an incref for :doc:`../pythonApi/scheduleManager` if the Tasklet returned doesn't already hold one (Main Tasklet).

2. An extra ref needs to be added to the :doc:`../pythonApi/scheduleManager` on creation of Main Tasklets.

3. This final strong reference for the Main Tasklet would be handled in gc traverse.

4. If during traverse the ref value for the Main Tasklet is 1 it suggests there are no references remaining other than the extra one created.

5. When this is the case traverse must release the extra reference to :doc:`../pythonApi/scheduleManager` which will now rightly be cleaned up.

6. The Tasklet refererence cannot be cleaned up in traverse as Python will crash (this isn't really what traverse is for). Instead the scheduler itself can schedule a Tasklet which will conduct this.

7. When the scheduler reaches the scheduled Tasklet the Main Tasklet will finally be decreffed and cleaned up.

The above approach has been explored and does work however the required changes make the codebase confusing. 