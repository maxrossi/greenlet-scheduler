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
    'run_n_tasklets',
    'set_schedule_callback',
    'get_schedule_callback',
    'get_thread_info',
    'set_channel_callback',
    'get_channel_callback',
    'switch_trap',
    'enable_softswitch',
    'TaskletExit'
]

try:
    import blue
    _scheduler = blue.LoadExtension("_scheduler")
except ImportError:
    import os
    flavor = os.environ.get("BUILDFLAVOR", "release")
    if flavor == 'release':
        import _scheduler
    elif flavor == 'debug':
        import _scheduler_debug as _scheduler
    elif flavor == 'trinitydev':
        import _scheduler_trinitydev as _scheduler
    elif flavor == 'internal':
        import _scheduler_internal as _scheduler
    else:
        _scheduler = None
        raise RuntimeError("Unknown build flavor: {}".format(flavor))
        
# Expose main scheduler methods and members at base level of extension
_C_API = _scheduler._C_API
getcurrent = _scheduler.getcurrent
getmain = _scheduler.getmain
tasklet = _scheduler.tasklet
channel = _scheduler.channel
getruncount = _scheduler.getruncount
schedule = _scheduler.schedule
schedule_remove = _scheduler.schedule_remove
run = _scheduler.run
run_n_tasklets = _scheduler.run_n_tasklets
set_schedule_callback = _scheduler.set_schedule_callback
get_schedule_callback = _scheduler.get_schedule_callback
get_thread_info = _scheduler.get_thread_info
set_channel_callback = _scheduler.set_channel_callback
get_channel_callback = _scheduler.get_channel_callback
switch_trap = _scheduler.switch_trap
enable_softswitch = _scheduler.enable_softswitch
TaskletExit = _scheduler.TaskletExit

#NOTE: Attributes main and current are not currently supported