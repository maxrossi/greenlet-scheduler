# -*- coding: utf-8 -*-

__all__ = [
    '_C_API',
    'current'
    'getcurrent',
    'main'
    'getmain',
    'tasklet',
    'channel',
    'current',
    'getruncount',
    'schedule',
    'schedule_remove',
    'run',
    'set_schedule_callback',
    'get_schedule_callback',
    'get_thread_info',
    'set_channel_callback',
    'get_channel_callback',
    'switch_trap',
    'enable_softswitch',
]

import _scheduler

from _scheduler import _C_API

# Get main scheduler
mainScheduler = _scheduler.Scheduler()


# Expose main scheduler methods and members at base level of extension
getcurrent = mainScheduler.getcurrent
getmain = mainScheduler.getmain
tasklet = _scheduler.Tasklet
channel = _scheduler.Channel
getruncount = mainScheduler.getruncount
schedule = mainScheduler.schedule
schedule_remove = mainScheduler.schedule_remove
run = mainScheduler.run
set_schedule_callback = mainScheduler.set_schedule_callback
get_schedule_callback = mainScheduler.get_schedule_callback
get_thread_info = mainScheduler.get_thread_info
set_channel_callback = _scheduler.set_channel_callback
get_channel_callback = _scheduler.get_channel_callback
switch_trap = mainScheduler.switch_trap
enable_softswitch = _scheduler.enable_softswitch

# Set initial values
main = tasklet(run)
current = main
mainScheduler.set_scheduler_tasklet(main)

# This is done two give the ability to access current tasklet through scheduler.current
# It would be better to deprecate this functionality and prefer getcurrent
# The callback functionality could then be removed
def UpdateCurrentCallback(newCurrent):
    current = newCurrent
mainScheduler.set_current_tasklet_changed_callback(UpdateCurrentCallback)


    