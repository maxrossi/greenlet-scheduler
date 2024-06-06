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

import sys
import contextlib
import test_utils
from schedulerext import QueueChannel

@contextlib.contextmanager
def block_trap(trap=True):
    c = scheduler.getcurrent()
    old = c.block_trap
    c.block_trap = trap

    try:
        yield
    finally:
        c.block_trap = old


class TestChannels(test_utils.SchedulerTestCaseBase):
    def setUp(self):
        super().setUp()

        self.channel = scheduler.channel

    def testBlockingSend(self):
        ''' Test that when a tasklet sends to a channel without waiting receivers, the tasklet is blocked. '''

        # Function to block when run in a tasklet.
        def f(testChannel):
            testChannel.send(1)

        # Get the tasklet blocked on the channel.
        channel = self.channel()
        tasklet = scheduler.tasklet(f)(channel)
        self.assertEqual(self.getruncount(), 2)
        tasklet.run()
        self.assertEqual(self.getruncount(), 1)

        # The tasklet should be blocked.
        self.assertTrue(tasklet.blocked,
                        "The tasklet should have been run and have blocked on the channel waiting for a corresponding receiver")

        # The channel should have a balance indicating one blocked sender.
        self.assertTrue(channel.balance == 1,
                        "The channel balance should indicate one blocked sender waiting for a corresponding receiver")

    def testBlockingReceive(self):
        ''' Test that when a tasklet receives from a channel without waiting senders, the tasklet is blocked. '''

        # Function to block when run in a tasklet.
        def f(testChannel):
            testChannel.receive()

        # Get the tasklet blocked on the channel.
        channel = self.channel()
        tasklet = scheduler.tasklet(f)(channel)
        self.assertEqual(self.getruncount(), 2)
        tasklet.run()
        self.assertEqual(self.getruncount(), 1)

        # The tasklet should be blocked.
        self.assertTrue(tasklet.blocked,
                        "The tasklet should have been run and have blocked on the channel waiting for a corresponding sender")

        # The channel should have a balance indicating one blocked sender.
        self.assertEqual(channel.balance, -1,
                         "The channel balance should indicate one blocked receiver waiting for a corresponding sender")

    def testNonBlockingSend(self):
        ''' Test that when there is a waiting receiver, we can send without blocking with normal channel behaviour. '''

        originalValue = 1
        receivedValues = []

        # Function to block when run in a tasklet.
        def f(testChannel):
            receivedValues.append(testChannel.receive())

        # Get the tasklet blocked on the channel.
        channel = self.channel()
        tasklet = scheduler.tasklet(f)(channel)
        self.assertEqual(self.getruncount(), 2)
        tasklet.run()
        self.assertEqual(self.getruncount(), 1)

        # Make sure that the current tasklet cannot block when it tries to receive.  We do not want
        # to exit this test having clobbered the block trapping value, so we make sure we restore
        # it.
        oldBlockTrap = scheduler.getcurrent().block_trap
        try:
            scheduler.getcurrent().block_trap = True
            channel.send(originalValue)
        finally:
            scheduler.getcurrent().block_trap = oldBlockTrap

        self.assertTrue(len(receivedValues) == 1 and receivedValues[0] == originalValue,
                        "We sent a value, but it was not the one we received.  Completely unexpected.")

    def testNonBlockingReceive(self):
        ''' Test that when there is a waiting sender, we can receive without blocking with normal channel behaviour. '''
        originalValue = 1

        # Function to block when run in a tasklet.
        def f(testChannel, valueToSend):
            testChannel.send(valueToSend)

        # Get the tasklet blocked on the channel.
        channel = self.channel()
        tasklet = scheduler.tasklet(f)(channel, originalValue)
        self.assertEqual(self.getruncount(), 2)
        tasklet.run()
        self.assertEqual(self.getruncount(), 1)

        # Make sure that the current tasklet cannot block when it tries to receive.  We do not want
        # to exit this test having clobbered the block trapping value, so we make sure we restore
        # it.
        oldBlockTrap = scheduler.getcurrent().block_trap
        try:
            scheduler.getcurrent().block_trap = True
            value = channel.receive()
        finally:
            scheduler.getcurrent().block_trap = oldBlockTrap

        tasklet.kill()

        self.assertEqual(value, originalValue,
                         "We received a value, but it was not the one we sent.  Completely unexpected.")

    def testBlockTrapSend(self):
        '''Test that block trapping works when receiving'''
        channel = self.channel()
        count = [0]

        def f():
            with block_trap():
                self.assertRaises(RuntimeError, channel.send, None)
            count[0] += 1

        # Test on main tasklet and on worker
        f()
        scheduler.tasklet(f)()
        self.assertEqual(self.getruncount(), 2)
        scheduler.run()
        self.assertEqual(self.getruncount(), 1)
        self.assertEqual(count[0], 2)

    def testBlockTrapRecv(self):
        '''Test that block trapping works when receiving'''
        channel = self.channel()
        count = [0]

        def f():
            with block_trap():
                self.assertRaises(RuntimeError, channel.receive)
            count[0] += 1

        f()
        scheduler.tasklet(f)()
        self.assertEqual(self.getruncount(), 2)
        scheduler.run()
        self.assertEqual(self.getruncount(), 1)
        self.assertEqual(count[0], 2)

    def testMainTaskletBlockingWithoutASender(self):
        ''' Test that the last runnable tasklet cannot be blocked on a channel receive. '''
        c = self.channel()
        self.assertRaises(RuntimeError, c.receive)

    def testMainTaskletBlockingWithoutReceiver(self):
        ''' Test that the last runnable tasklet cannot be blocked on a channel send. '''
        c = self.channel()

        def test_send():
            c.send(1)

        self.assertRaises(RuntimeError, test_send)

    def testInterthreadCommunication(self):
        ''' Test that tasklets in different threads sending over channels to each other work. '''
        import threading
        commandChannel = self.channel()

        def master_func():
            commandChannel.send("ECHO 1")
            commandChannel.send("ECHO 2")
            commandChannel.send("ECHO 3")
            commandChannel.send("QUIT")

        def slave_func():
            while 1:
                command = commandChannel.receive()
                if command == "QUIT":
                    break

        def scheduler_run(tasklet_func):
            t = scheduler.tasklet(tasklet_func)()
            while t.alive:
                scheduler.run()

        thread = threading.Thread(target=scheduler_run, args=(master_func,))
        thread.start()

        scheduler_run(slave_func)
        thread.join()

    def testSendException(self):

        # Function to send the exception
        def f(testChannel):
            testChannel.send_exception(ValueError, 1, 2, 3)

        # Get the tasklet blocked on the channel.
        channel = self.channel()
        tasklet = scheduler.tasklet(f)(channel)
        self.assertEqual(self.getruncount(), 2)
        tasklet.run()
        self.assertEqual(self.getruncount(), 1)
        self.assertRaises(ValueError, channel.receive)
        tasklet = scheduler.tasklet(f)(channel)
        self.assertEqual(self.getruncount(), 3)
        tasklet.run()
        self.assertEqual(self.getruncount(), 2)
        try:
            channel.receive()
        except ValueError as e:
            self.assertEqual(e.args, (1, 2, 3))

    def testSendThrow(self):
        import traceback

        # subfunction in tasklet
        def bar():
            raise ValueError(1, 2, 3)

        # Function to send the exception
        def f(testChannel):
            try:
                bar()
            except Exception:
                testChannel.send_throw(*sys.exc_info())

        # Get the tasklet blocked on the channel.
        channel = self.channel()
        tasklet = scheduler.tasklet(f)(channel)
        self.assertEqual(self.getruncount(), 2)
        tasklet.run()
        self.assertEqual(self.getruncount(), 1)
        self.assertRaises(ValueError, channel.receive)

        tasklet = scheduler.tasklet(f)(channel)
        self.assertEqual(self.getruncount(), 3)
        tasklet.run()
        self.assertEqual(self.getruncount(), 2)
        try:
            channel.receive()
        except ValueError:
            exc, val, _ = sys.exc_info()
            self.assertEqual(val.args[0].args, (1, 2, 3))

            # Check that the traceback is correct
            l = traceback.extract_tb(val.args[1])
            self.assertEqual(l[-1][2], "bar")

    def testBlockingReceiveOnMainTasklet(self):

        sentValues = []

        def sender(chan):
            for i in range(0, 10):
                chan.send(i)
                sentValues.append(i)

        channel = self.channel()

        sendingTasklet = scheduler.tasklet(sender)(channel)
        self.assertEqual(self.getruncount(), 2)
        sendingTasklet.run()
        self.assertEqual(self.getruncount(), 1)

        self.assertEqual(len(sentValues), 0)
        self.assertEqual(channel.balance, 1)

        for i in range(0, 10):
            r = channel.receive()

        self.assertEqual(channel.balance, 0)
        scheduler.run()
        self.assertEqual(sentValues, [0, 1, 2, 3, 4, 5, 6, 7, 8, 9])

    def testBlockedTaskletsGreenletIsNotParent(self):
        taskletOrder = []

        def foo(x):
            taskletOrder.append(x)

        channel = self.channel()

        def sender(chan):
            scheduler.tasklet(foo)("a")
            chan.send(1)
            scheduler.tasklet(foo)("b")
            chan.send(2)
            scheduler.tasklet(foo)("c")
            chan.send(3)
            scheduler.tasklet(foo)("d")

        senderTasklet = scheduler.tasklet(sender)(channel)
        self.assertEqual(self.getruncount(), 2)
        senderTasklet.run()
        self.assertEqual(self.getruncount(), 2)

        # sendingTasklet
        r = channel.receive()
        taskletOrder.append(r)
        r = channel.receive()
        taskletOrder.append(r)
        r = channel.receive()
        taskletOrder.append(r)

        self.assertEqual(taskletOrder, [1, 'a', 2, 'b', 3])

    def testBlockingSendOnMainTasklet(self):

        receivedValues = []

        def receiver(chan):
            for i in range(0, 10):
                r = chan.receive()
                receivedValues.append(r)

        channel = self.channel()

        sendingTasklet = scheduler.tasklet(receiver)(channel)
        self.assertEqual(self.getruncount(), 2)
        sendingTasklet.run()
        self.assertEqual(self.getruncount(), 1)

        self.assertEqual(len(receivedValues), 0)
        self.assertEqual(channel.balance, -1)

        for i in range(0, 10):
            channel.send(i)

        self.assertEqual(channel.balance, 0)
        self.assertEqual(receivedValues, [0, 1, 2, 3, 4, 5, 6, 7, 8, 9])

    def testPreferenceSender(self):
        completedTasklets = []

        c = self.channel()

        c.preference = 1

        def sender(chan, x):
            chan.send("test")
            completedTasklets.append(("sender", x))

        def receiver(chan, x):
            res = chan.receive()
            completedTasklets.append(("receiver", x))

        expectedExecutionOrder = [('sender', 0), ('sender', 1), ('sender', 2), ('receiver', 0), ('receiver', 1),
                                  ('receiver', 2)]

        for i in range(3):
            scheduler.tasklet(sender)(c, i)

        for i in range(3):
            scheduler.tasklet(receiver)(c, i)

        self.assertEqual(completedTasklets, [])

        scheduler.run()

        self.assertEqual(expectedExecutionOrder, completedTasklets)

        completedTasklets = []

        c2 = self.channel()
        c2.preference = 1

        for i in range(3):
            scheduler.tasklet(receiver)(c2, i)

        for i in range(3):
            scheduler.tasklet(sender)(c2, i)

        scheduler.run()

        self.assertEqual(expectedExecutionOrder, completedTasklets)

    def testPreferenceReceiver(self):
        completedSendTasklets = []

        c = self.channel()

        # this is the default, but setting it explicitly for the test
        c.preference = -1

        def sender(chan, x):
            chan.send(x)
            completedSendTasklets.append(x)

        for i in range(10):
            tasklet = scheduler.tasklet(sender)(c, i)
            tasklet.run()

        for i in range(10):
            c.receive()
            self.assertEqual(0, len(completedSendTasklets))

        scheduler.run()

        self.assertEqual(10, len(completedSendTasklets))

    def testPreferenceNeitherSimple(self):
        taskletComplete = [False]

        c = self.channel()

        c.preference = 0

        def receiving_callable():
            c.receive()
            taskletComplete[0] = True

        scheduler.tasklet(receiving_callable)()

        scheduler.run()

        self.assertFalse(taskletComplete[0])

        c.send(None)

        self.assertFalse(taskletComplete[0])

        scheduler.run()

        self.assertTrue(taskletComplete[0])

    def testPreferenceNeither(self):
        completedTasklets = []

        c = self.channel()

        c.preference = 0

        def justAnotherTasklet(x):
            completedTasklets.append(x)

        scheduler.tasklet(justAnotherTasklet)("actually first")
        scheduler.tasklet(justAnotherTasklet)("actually second")

        def sender(chan, x):
            scheduler.tasklet(justAnotherTasklet)("sender inbetween")
            chan.send("test")
            completedTasklets.append(("sender", x))

        def receiver(chan, x):
            scheduler.tasklet(justAnotherTasklet)("recever inbetween")
            res = chan.receive()
            completedTasklets.append(("receiver", x))
            self.assertEqual(res, "test")

        for i in range(10):
            scheduler.tasklet(sender)(c, i)

        for i in range(10):
            scheduler.tasklet(receiver)(c, 1)

        self.assertEqual(len(completedTasklets), 0)

        scheduler.tasklet(justAnotherTasklet)("fist")
        scheduler.tasklet(justAnotherTasklet)("second")
        scheduler.tasklet(justAnotherTasklet)("third")
        scheduler.tasklet(justAnotherTasklet)("fourth")
        scheduler.tasklet(justAnotherTasklet)("fifth")

        scheduler.run()

        self.assertEqual(completedTasklets,
                         ['actually first', 'actually second', ('receiver', 1), ('receiver', 1), ('receiver', 1),
                          ('receiver', 1), ('receiver', 1), ('receiver', 1), ('receiver', 1), ('receiver', 1),
                          ('receiver', 1), ('receiver', 1), 'fist', 'second', 'third', 'fourth', 'fifth',
                          'sender inbetween', 'sender inbetween', 'sender inbetween', 'sender inbetween',
                          'sender inbetween', 'sender inbetween', 'sender inbetween', 'sender inbetween',
                          'sender inbetween', 'sender inbetween', 'recever inbetween', ('sender', 0),
                          'recever inbetween', ('sender', 1), 'recever inbetween', ('sender', 2), 'recever inbetween',
                          ('sender', 3), 'recever inbetween', ('sender', 4), 'recever inbetween', ('sender', 5),
                          'recever inbetween', ('sender', 6), 'recever inbetween', ('sender', 7), 'recever inbetween',
                          ('sender', 8), 'recever inbetween', ('sender', 9)])

        self.assertEqual(len(completedTasklets), 47)

    def testChannelIteratorInterface(self):
        channel = self.channel()

        def send_value(x):
            channel.send(x)

        scheduler.tasklet(send_value)(1)
        scheduler.tasklet(send_value)(2)
        scheduler.tasklet(send_value)(3)

        scheduler.run()

        iterator = iter(channel)

        self.assertEqual(next(iterator), 1)
        self.assertEqual(next(iterator), 2)
        self.assertEqual(next(iterator), 3)

        scheduler.run()

        self.assertEqual(self.getruncount(), 1)

        scheduler.tasklet(send_value)(1)
        scheduler.tasklet(send_value)(2)
        scheduler.tasklet(send_value)(3)

        scheduler.run()

        iterator = iter(channel)

        count = 0

        for sent_value in iterator:
            count = count + sent_value

        self.assertEqual(count, 6)

    def testSendOnClosed(self):
        c = self.channel()
        c.close()
        self.assertRaises(ValueError, c.send, None)

    def testReceiveOnClosed(self):
        c = self.channel()
        c.close()
        self.assertRaises(ValueError, c.receive)

    def testClosing(self):
        c = self.channel()
        testSendValue = 101

        def foo():
            c.send(testSendValue)

        scheduler.tasklet(foo)()
        scheduler.run()

        self.assertEqual(c.balance,1)

        c.close()

        self.assertFalse(c.closed)
        self.assertTrue(c.closing)

        self.assertEqual(c.receive(), testSendValue)

        self.assertTrue(c.closed)
        self.assertTrue(c.closing)

        self.assertRaises(ValueError, c.receive)

    def testOpen(self):
        testValue = 101
        c = self.channel()
        c.close()

        self.assertTrue(c.closed)

        c.open()

        self.assertFalse(c.closing)
        self.assertFalse(c.closed)

        def foo():
            c.send(testValue)

        scheduler.tasklet(foo)()

        scheduler.run()

        self.assertEqual(c.receive(), testValue)


    def testIteratorOnClosed(self):
        c = self.channel()
        c.close()
        i = iter(c)

        def n():
            return next(i)
        self.assertRaises(StopIteration, n)


class TestQueueChannels(TestChannels):
    """
    Inherits from TestChannels in order to help ensure parity where the
    channel behaviors intersects.
    """
    def setUp(self):
        super().setUp()

        self.channel = QueueChannel

    def testNonBlockingSend(self):
        def send(test_channel):
            test_channel.send((1, 2, 3))

        channel = self.channel()
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

        channel = self.channel()
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

        channel = self.channel()
        tasklet = scheduler.tasklet(send)(channel)

        self.assertEqual(self.getruncount(), 2)
        tasklet.run()
        self.assertEqual(self.getruncount(), 1)

        # The tasklet should have inserted data into the queue
        data, exception = channel.data_queue.popleft()
        self.assertEqual(data, (1, 2, 3), "Channel queue received incorrect data")

    def testBlockingReceive(self):
        def receive(test_channel):
            return test_channel.receive()

        def send(test_channel):
            test_channel.send((1, 2, 3))

        channel = self.channel()

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

        channel = self.channel()
        tasklet = scheduler.tasklet(foo)(channel)
        self.assertEqual(self.getruncount(), 2)
        tasklet.run()
        self.assertEqual(self.getruncount(), 1)

        exception_received = False
        try:
            channel.receive()
        except Exception as e:
            self.assertIsInstance(e, ValueError, "Exceptions sent and received over the channel did not match")
            self.assertEqual(e.args, (1, 2, 3), "Incorrect data received from exception")
            exception_received = True

        self.assertTrue(exception_received, "Channel did not receive an exception")

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

        channel = self.channel()
        tasklet = scheduler.tasklet(sendThrow)(channel)
        self.assertEqual(self.getruncount(), 2)
        tasklet.run()
        self.assertEqual(self.getruncount(), 1)
        self.assertRaises(ValueError, channel.receive)

        tasklet = scheduler.tasklet(sendThrow)(channel)
        self.assertEqual(self.getruncount(), 2)
        tasklet.run()
        self.assertEqual(self.getruncount(), 1)

        try:
            channel.receive()
        except ValueError:
            exc, val, _ = sys.exc_info()
            self.assertEqual(val.args[0].args, (1, 2, 3))
            # Check that the traceback is correct
            l = traceback.extract_tb(val.args[1])
            self.assertEqual(l[-1][2], "bar")

    def testMainTaskletBlockingWithoutReceiver(self):
        c = self.channel()

        def test_send():
            c.receive()

        self.assertRaises(RuntimeError, test_send)

    def testBlockedTaskletsGreenletIsNotParent(self):
        taskletOrder = []

        def foo(x):
            taskletOrder.append(x)

        channel = self.channel()

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
        channel = self.channel()
        count = [0]

        def f():
            with block_trap():
                self.assertRaises(RuntimeError, channel.receive, None)
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

        channel = self.channel()

        scheduler.tasklet(sender)(channel)

        self.assertEqual(self.getruncount(), 2)

        self.assertEqual(len(receivedValues), 0)
        self.assertEqual(channel.balance, 0)

        for i in range(0, 10):
            receivedValues.append(channel.receive())

        self.assertEqual(channel.balance, 0)
        self.assertEqual(receivedValues, [0, 1, 2, 3, 4, 5, 6, 7, 8, 9])

    def testPreferenceNeither(self):
        """
        QueueChannel is hardwired to prefer sender as we never want Queue channels to block on send.
        We therefore skip testing against other channel preferences.
        """
        pass

    def testPreferenceReceiver(self):
        """
        QueueChannel is hardwired to prefer sender as we never want Queue channels to block on send.
        We therefore skip testing against other channel preferences.
        """
        pass

    def testPreferenceNeitherSimple(self):
        """
        QueueChannel is hardwired to prefer sender as we never want Queue channels to block on send.
        We therefore skip testing against other channel preferences.
        """
        pass

    def testBlockingSendOnMainTasklet(self):
        """
        QueueChannel does not block on send.
        """
        pass

    def testBlockingSend(self):
        """
        QueueChannel should never block on send
        """
        pass
