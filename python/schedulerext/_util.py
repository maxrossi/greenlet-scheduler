import contextlib
import os
import sys

flavor = os.environ.get("BUILDFLAVOR", "release")

if "scheduler" in sys.modules:
    import scheduler
else:
    if flavor == 'release':
        import _scheduler as scheduler
    elif flavor == 'debug':
        import _scheduler_debug as scheduler
    elif flavor == 'trinitydev':
        import _scheduler_trinitydev as scheduler
    elif flavor == 'internal':
        import _scheduler_internal as scheduler
    else:
        scheduler = None
        raise RuntimeError("Unknown build flavor: {}".format(flavor))


@contextlib.contextmanager
def block_trap(trap=True):
    c = scheduler.getcurrent()
    old = c.block_trap
    c.block_trap = trap

    try:
        yield
    finally:
        c.block_trap = old
