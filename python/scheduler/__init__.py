# -*- coding: utf-8 -*-

__all__ = [
    '_C_API',
    'getcurrent',
    'getmain',
    'tasklet',
    'channel',
    'getruncount',
    'schedule',
    'schedule_remove',
    'run',
    'set_schedule_callback',
    'get_schedule_callback',
    'get_thread_info',
    'set_channel_callback',
    'get_channel_callback',
]

import _scheduler

from _scheduler import _C_API

# Get main scheduler
mainScheduler = _scheduler.getscheduler()

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

current = mainScheduler.current
main = mainScheduler.main
    