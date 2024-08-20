try:
    import blue
    _scheduler = blue.LoadExtension("_scheduler")
except ImportError:
    # TODO check if this code path is still required once blue is updated
    import os
    # Intentionally raise a KeyError from here:
    # This code path should only be hit in unittest environments which always pass on the `BUILDFLAVOR` env variable
    flavor = os.environ['BUILDFLAVOR']
    if flavor == 'debug':
        import _scheduler_debug as _scheduler
    elif flavor == 'internal':
        import _scheduler_internal as _scheduler
    elif flavor == 'trinitydev':
        import _scheduler_trinitydev as _scheduler
    else:
        import _scheduler


for member in dir(_scheduler):
    if member in ('__name__', '__file__'):
        continue
    globals()[member] = getattr(_scheduler, member)


import collections
import contextlib
import threading


class QueueChannel(_scheduler.channel):
    """
    A QueueChannel is like a channel except that it contains a queue, so that the
    sender never blocks.  If there isn't a blocked tasklet waiting for the data,
    the data is queued up internally.  The sender always continues.
    """
    def __init__(self):
        super().__init__(self)
        self.data_queue = collections.deque()
        self.preference = 1 #sender never blocks

    @property
    def balance(self):
        if self.data_queue:
            return len(self.data_queue)
        return super(QueueChannel, self).balance

    def send(self, data):
        sup = super(QueueChannel, self)
        with threading.Lock():
            if sup.balance >= 0 and not sup.closing:
                self.data_queue.append((True, data))
            else:
                sup.send(data)

    def send_exception(self, exc, *args):
        self.send_throw(exc, args)

    def send_throw(self, exc, value=None, tb=None):
        """call with similar arguments as raise keyword"""
        sup = super(QueueChannel, self)
        with threading.Lock():
            if sup.balance >= 0 and not sup.closing:
                self.data_queue.append((False, (exc, value, tb)))
            else:
                #deal with channel.send_exception signature
                sup.send_throw(exc, value, tb)

    def receive(self):
        with threading.Lock():
            if not self.data_queue:
                return super(QueueChannel, self).receive()
            ok, data = self.data_queue.popleft()
            if ok:
                return data
            exc, value, tb = data
            try:
                raise exc(value).with_traceback(tb)
            finally:
                tb = None

    #iterator protocol
    def send_sequence(self, sequence):
        for i in sequence:
            self.send(i)

    def __next__(self):
        return self.receive()

    def __len__(self):
        return len(self.data_queue)


@contextlib.contextmanager
def block_trap(trap=True):
    c = _scheduler.getcurrent()
    old = c.block_trap
    c.block_trap = trap

    try:
        yield
    finally:
        c.block_trap = old
