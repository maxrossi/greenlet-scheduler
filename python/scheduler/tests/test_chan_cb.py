from __future__ import absolute_import
import unittest

from support import SchedulerTestCase

class ChannelMonitor:
    "A channel monitor acting as a callback for set_channel_callback()."

    def __init__(self):
        self.history = []

    def __call__(self, channel, tasklet, isSending, willBlock):
        tup = (channel, tasklet, isSending, willBlock, tasklet.tempval)
        self.history.append(tup)


class ChannelCallbackTestCase(SchedulerTestCase):
    "A collection of channel callback tests."

    def test0(self):
        "Simple monitored channel send from main tasklet."

        import scheduler

        # create players
        chan = scheduler.channel()
        main = scheduler.getmain()  # implicit sender
        receiver = scheduler.tasklet(lambda ch: ch.receive())
        receiver = receiver(chan)

        # send a value to a monitored channel
        chanMon = ChannelMonitor()
        scheduler.set_channel_callback(chanMon)
        val = 42
        chan.send(val)
        scheduler.set_channel_callback(None)

        # compare sent value with monitored one
        # found = chanMon.history[0][1].tempval
        # self.assertEqual(val, found) # FAILS - why?
        #
        # fails, because the value is moved from sender to receiver
        # also, I need to modify channels a little :-)
        # this one works, because I keep a copy of the value.
        #
        # print chanMon.history
        found = chanMon.history[0][-1]
        self.assertEqual(val, found)

    def testGetCallback(self):
        import scheduler
        mon = ChannelMonitor()
        self.assertIsNone(scheduler.get_channel_callback())
        old = scheduler.set_channel_callback(mon)
        self.assertIsNone(old)
        self.assertIs(scheduler.get_channel_callback(), mon)
        old = scheduler.set_channel_callback(None)
        self.assertIs(old, mon)
        self.assertIsNone(scheduler.get_channel_callback())


if __name__ == "__main__":
    unittest.main()
