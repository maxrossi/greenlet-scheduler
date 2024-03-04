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

# Expose main scheduler methods and members at base level of extension
getcurrent = _scheduler.getcurrent
getmain = _scheduler.getmain
tasklet = _scheduler.Tasklet
channel = _scheduler.Channel
getruncount = _scheduler.getruncount
schedule = _scheduler.schedule
schedule_remove = _scheduler.schedule_remove
run = _scheduler.run
set_schedule_callback = _scheduler.set_schedule_callback
get_schedule_callback = _scheduler.get_schedule_callback
get_thread_info = _scheduler.get_thread_info
set_channel_callback = _scheduler.set_channel_callback
get_channel_callback = _scheduler.get_channel_callback
switch_trap = _scheduler.switch_trap
enable_softswitch = _scheduler.enable_softswitch

# Attributes
main = getmain()
current = None # TODO so far not updated
    