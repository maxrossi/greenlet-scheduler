from schedulerext import QueueChannel, block_trap

import os
import sys
from test_utils import SchedulerTestCaseBase

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


class TestQueueChannels(SchedulerTestCaseBase):
    def testNonBlockingSend(self):
        def send(test_channel):
            test_channel.send((1, 2, 3))

        channel = QueueChannel()
        tasklet = scheduler.tasklet(send)(channel)
        self.assertEqual(self.getruncount(), 2)
        tasklet.run()
        self.assertEqual(self.getruncount(), 1)

        # The tasklet should not be blocked.
        self.assertFalse(tasklet.blocked, "The tasklet should have been run and not been blocked on a channel waiting "
                                          "for a receiver")

    def testChannelBalance(self):
        def send(test_channel):
            test_channel.send((1, 2, 3))

        channel = QueueChannel()
        self.assertEqual(channel.balance, 0, "Channel balance incorrectly instantiated")

        # Increment channel balance a couple of times
        for i in range(2):
            tasklet = scheduler.tasklet(send)(channel)
            self.assertEqual(self.getruncount(), 2)
            tasklet.run()
            self.assertEqual(self.getruncount(), 1)

        self.assertEqual(channel.balance, len(channel.data_queue),
                         "The channel balance should equal the length of the channel's data queue")

    def testQueueData(self):
        def send(test_channel):
            test_channel.send((1, 2, 3))

        channel = QueueChannel()
        tasklet = scheduler.tasklet(send)(channel)

        self.assertEqual(self.getruncount(), 2)
        tasklet.run()
        self.assertEqual(self.getruncount(), 1)

        # The tasklet should have inserted data into the queue
        exception, data = channel.data_queue.popleft()
        self.assertEqual(data, (1, 2, 3), "Channel queue received incorrect data")

        data_to_send = range(3)
        for i in data_to_send:
            channel.send(i)

        self.assertEqual(len(channel), len(data_to_send), "len(channel) does not match queue length")

    def testBlockingReceive(self):
        def receive(test_channel):
            return test_channel.receive()

        def send(test_channel):
            test_channel.send((1, 2, 3))

        channel = QueueChannel()

        receiving_tasklet = scheduler.tasklet(receive)(channel)

        self.assertEqual(self.getruncount(), 2)
        receiving_tasklet.run()
        self.assertEqual(self.getruncount(), 1)
        self.assertTrue(receiving_tasklet.blocked,
                        "Channel should block receivers with no corresponding senders")

        sending_tasklet = scheduler.tasklet(send)(channel)
        self.assertEqual(self.getruncount(), 2)
        sending_tasklet.run()
        self.assertEqual(self.getruncount(), 2,
                         "Channel should schedule blocked receivers when it accrues senders")

        self.assertFalse(receiving_tasklet.blocked, "Channel should unblock receiving tasklets when it accrues senders")

    def testSendException(self):
        # Function to send the exception
        def foo(test_channel):
            test_channel.send_exception(ValueError, *(1, 2, 3))

        channel = QueueChannel()
        tasklet = scheduler.tasklet(foo)(channel)
        self.assertEqual(self.getruncount(), 2)
        tasklet.run()
        self.assertEqual(self.getruncount(), 1)

        exception_received = False
        try:
            channel.receive()
        except ValueError:
            exception_received = True

        self.assertTrue(exception_received, "Failed to receive the exception sent over the channel")

    def testSendThrow(self):
        import traceback

        # subfunction in tasklet
        def bar():
            raise ValueError(1, 2, 3)

        # Function to send the exception
        def sendThrow(testChannel):
            try:
                bar()
            except Exception:
                testChannel.send_throw(*sys.exc_info())

        channel = QueueChannel()
        tasklet = scheduler.tasklet(sendThrow)(channel)
        self.assertEqual(self.getruncount(), 2)
        tasklet.run()
        self.assertEqual(self.getruncount(), 1)
        self.assertRaises(ValueError, channel.receive)

        tasklet = scheduler.tasklet(sendThrow)(channel)
        self.assertEqual(self.getruncount(), 2)
        tasklet.run()
        self.assertEqual(self.getruncount(), 1)

        exc = False
        try:
            channel.receive()
        except ValueError:
            exc = True

        self.assertTrue(exc)

    def testMainTaskletBlockingWithoutReceiver(self):
        c = QueueChannel()

        def test_send():
            c.receive()

        self.assertRaises(RuntimeError, test_send)

    def testBlockedTaskletsGreenletIsNotParent(self):
        taskletOrder = []

        def foo(x):
            taskletOrder.append(x)

        channel = QueueChannel()

        def receiver(c):
            scheduler.tasklet(foo)('a')
            taskletOrder.append(c.receive())
            scheduler.tasklet(foo)('b')
            taskletOrder.append(c.receive())
            scheduler.tasklet(foo)('c')

        receiverTasklet = scheduler.tasklet(receiver)(channel)

        self.assertEqual(self.getruncount(), 2)
        receiverTasklet.run()
        self.assertEqual(self.getruncount(), 2)

        def sender(c, x):
            c.send(x)

        for i in range(1, 3):
            scheduler.tasklet(sender)(channel, i)

        scheduler.run()

        self.assertEqual(taskletOrder, ['a', 1, 2, 'b', 'c'])

    def testBlockTrapSend(self):
        """
        Test that block trapping works when receiving
        """
        channel = QueueChannel()
        count = [0]

        def f():
            with block_trap():
                self.assertRaises(RuntimeError, channel.receive)
            count[0] += 1

        # Test on main tasklet and on worker
        f()
        scheduler.tasklet(f)()
        self.assertEqual(self.getruncount(), 2)
        scheduler.run()
        self.assertEqual(self.getruncount(), 1)
        self.assertEqual(count[0], 2)

    def testBlockingReceiveOnMainTasklet(self):
        receivedValues = []

        def sender(chan):
            for i in range(0, 10):
                chan.send(i)

        channel = QueueChannel()

        scheduler.tasklet(sender)(channel)

        self.assertEqual(self.getruncount(), 2)

        self.assertEqual(len(receivedValues), 0)
        self.assertEqual(channel.balance, 0)

        for i in range(0, 10):
            receivedValues.append(channel.receive())

        self.assertEqual(channel.balance, 0)
        self.assertEqual(receivedValues, [0, 1, 2, 3, 4, 5, 6, 7, 8, 9])
