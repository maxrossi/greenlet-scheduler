import collections
import os

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

    Each queue entry is composed of a (is_exception, data) tuple where data
    is the value sent over the channel, and is_exception is used internally to
    explicitly determine whether an exception was sent over the channel.
    """

    def __init__(self):
        super().__init__(self)

        self.data_queue = collections.deque()
        self.preference = 1  # sender never blocks

    @property
    def balance(self):
        if self.data_queue:  # Queue has data
            return len(self.data_queue)
        return super().balance

    @property
    def closed(self):
        return self.balance == 0 and self.closing

    def send(self, *args):
        if super().balance >= 0 and not self.closed:
            self.data_queue.append((*args, False))
        else:
            super().send(*args)

    def send_exception(self, *args, **kwargs):
        if super().balance >= 0 and not self.closed:
            self.data_queue.append((args, True))
        else:
            super().send_exception(args)

    def send_throw(self, exc, value=None, tb=None):
        if super().balance >= 0 and not self.closed:
            self.data_queue.append(((exc, value, tb), True))
        else:
            super().send_throw(exc, value, tb)

    def receive(self, *args):
        if not self.data_queue or self.closed:
            return super().receive()

        data, is_exception = self.data_queue.popleft()

        if is_exception:
            exception_type = data[0]
            exception_values = data[1:]
            raise exception_type(*exception_values)

        return data

    # iterator protocol
    def send_sequence(self, sequence):
        for i in sequence:
            self.send(i)

    def __next__(self):
        if self.data_queue:
            return self.receive()
        raise StopIteration()

    def __len__(self):
        return len(self.data_queue)
