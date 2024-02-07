# -*- coding: utf-8 -*-

__all__ = [
    '_C_API',
]

import _scheduler

from _scheduler import _C_API

# Get main scheduler
mainScheduler = _scheduler.getscheduler()

# Expose main scheduler methods and members at base level of extension
getcurrent = mainScheduler.getcurrent
current = mainScheduler.current
    