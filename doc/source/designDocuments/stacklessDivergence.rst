Stackless Divergence
====================

Lists occurances where ``carbon-scheduler`` diverges from `Stackless Python <https://stackless.readthedocs.io/en/3.8-slp/stackless-python.html>`_ behaviour.

The original aim was to match `Stackless Python <https://stackless.readthedocs.io/en/3.8-slp/stackless-python.html>`_ functionality, however some deviation is expected due to the nature of `Stackless Python <https://stackless.readthedocs.io/en/3.8-slp/stackless-python.html>`_ being built into the interpreter.

Over time it is expected that divergence will increase.


Overview of differences
-----------------------
* :ref:`divergence-scheduler-current-scheduler-main`
* :ref:`divergence-scheduler-tasklet-execution-order` 



.. _divergence-scheduler-current-scheduler-main:
scheduler.current and scheduler.main are not implemented
--------------------------------------------------------
These attributes for current and main are not provided due to innability to keep them correct when used with threading.


**The Alternative**

use :py:func:`scheduler.getcurrent` and :py:func:`scheduler.getmain`. These will return the correct value for the Python thread which they were called.



.. _divergence-scheduler-tasklet-execution-order:
Tasklet execution order when using :py:func:`tasklet.run` can be altered
-------------------------------------------------------------
When :py:func:`tasklet.run` is called it creates a nested non linear tasklet execution order.

carbon-scheduler allows the user to turn this behaviour off and flatten the queue using :py:func:`scheduler.set_use_nested_tasklets`.

See :doc:`nestedTaskletsVsFlatSchedulingQueue` for further details.