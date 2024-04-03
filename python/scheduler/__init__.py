# -*- coding: utf-8 -*-

import os

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
    'TaskletExit'
]

flavor = os.environ.get("BUILDFLAVOR", "release")

if flavor == 'release':
    import _scheduler
    from _scheduler import _C_API
elif flavor == 'debug':
    import _scheduler_debug as _scheduler
    from _scheduler_debug import _C_API
elif flavor == 'trinitydev':
    import _scheduler_trinitydev as _scheduler
    from _scheduler_trinitydev import _C_API
elif flavor == 'internal':
    import _scheduler_internal as _scheduler
    from _scheduler_internal import _C_API
else:
    raise RuntimeError("Unknown build flavor: {}".format(flavor))



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
TaskletExit = _scheduler.TaskletExit

# Attributes
main = getmain()
current = None # TODO so far not updated
