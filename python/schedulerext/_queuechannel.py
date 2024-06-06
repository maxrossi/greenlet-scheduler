import collections
import os
import threading

flavor = os.environ.get("BUILDFLAVOR", "release")

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


class QueueChannel(scheduler.channel):
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
