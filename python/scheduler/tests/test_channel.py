import scheduler
import contextlib
import unittest
    
@contextlib.contextmanager
def block_trap(trap=True):
    c = scheduler.getcurrent()
    old = c.block_trap
    c.block_trap = trap
    
    try:
        yield
    finally:
        c.block_trap = old
    
    
    
    
    
class TestChannels(unittest.TestCase):

    def setUp(self):
        pass
        
    def tearDown(self):
        pass
        
    def testBlockingSend(self):
        ''' Test that when a tasklet sends to a channel without waiting receivers, the tasklet is blocked. '''

        # Function to block when run in a tasklet.
        def f(testChannel):
            testChannel.send(1)

        # Get the tasklet blocked on the channel.
        channel = scheduler.channel()
        tasklet = scheduler.tasklet(f)(channel)
        tasklet.run()

        # The tasklet should be blocked.
        self.assertTrue(tasklet.blocked, "The tasklet should have been run and have blocked on the channel waiting for a corresponding receiver")

        # The channel should have a balance indicating one blocked sender.
        self.assertTrue(channel.balance == 1, "The channel balance should indicate one blocked sender waiting for a corresponding receiver")
        
    def testBlockingReceive(self):
        ''' Test that when a tasklet receives from a channel without waiting senders, the tasklet is blocked. '''

        # Function to block when run in a tasklet.
        def f(testChannel):
            testChannel.receive()

        # Get the tasklet blocked on the channel.
        channel = scheduler.channel()
        tasklet = scheduler.tasklet(f)(channel)
        tasklet.run()

        # The tasklet should be blocked.
        self.assertTrue(tasklet.blocked, "The tasklet should have been run and have blocked on the channel waiting for a corresponding sender")

        # The channel should have a balance indicating one blocked sender.
        self.assertEqual(channel.balance, -1, "The channel balance should indicate one blocked receiver waiting for a corresponding sender")

    def testNonBlockingSend(self):
        ''' Test that when there is a waiting receiver, we can send without blocking with normal channel behaviour. '''

        originalValue = 1
        receivedValues = []

        # Function to block when run in a tasklet.
        def f(testChannel):
            receivedValues.append(testChannel.receive())

        # Get the tasklet blocked on the channel.
        channel = scheduler.channel()
        tasklet = scheduler.tasklet(f)(channel)
        tasklet.run()

        # Make sure that the current tasklet cannot block when it tries to receive.  We do not want
        # to exit this test having clobbered the block trapping value, so we make sure we restore
        # it.
        oldBlockTrap = scheduler.getcurrent().block_trap
        try:
            scheduler.getcurrent().block_trap = True
            channel.send(originalValue)
        finally:
            scheduler.getcurrent().block_trap = oldBlockTrap

        self.assertTrue(len(receivedValues) == 1 and receivedValues[0] == originalValue, "We sent a value, but it was not the one we received.  Completely unexpected.")
        
    def testNonBlockingReceive(self):
        ''' Test that when there is a waiting sender, we can receive without blocking with normal channel behaviour. '''
        originalValue = 1

        # Function to block when run in a tasklet.
        def f(testChannel, valueToSend):
            testChannel.send(valueToSend)

        # Get the tasklet blocked on the channel.
        channel = scheduler.channel()
        tasklet = scheduler.tasklet(f)(channel, originalValue)
        tasklet.run()

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

        self.assertEqual(value, originalValue, "We received a value, but it was not the one we sent.  Completely unexpected.")
        
    def testBlockTrapSend(self):
        '''Test that block trapping works when receiving'''
        channel = scheduler.channel()
        count = [0]

        
        def f():
            with block_trap():
                self.assertRaises(RuntimeError, channel.send, None)
            count[0] += 1

        # Test on main tasklet and on worker
        f()
        scheduler.tasklet(f)()
        scheduler.run()
        self.assertEqual(count[0], 2)
        
    def testBlockTrapRecv(self):
        '''Test that block trapping works when receiving'''
        channel = scheduler.channel()
        count = [0]

        def f():
            with block_trap():
                self.assertRaises(RuntimeError, channel.receive)
            count[0] += 1

        f()
        scheduler.tasklet(f)()
        scheduler.run()
        self.assertEqual(count[0], 2)

    def testMainTaskletBlockingWithoutASender(self):
        ''' Test that the last runnable tasklet cannot be blocked on a channel receive. '''
        c = scheduler.channel()
        self.assertRaises(RuntimeError, c.receive)

    def testMainTaskletBlockingWithoutReceiver(self):
        ''' Test that the last runnable tasklet cannot be blocked on a channel send. '''
        c = scheduler.channel()
        def test_send():
            c.send(1)

        self.assertRaises(RuntimeError, test_send)

    def testInterthreadCommunication(self):
        ''' Test that tasklets in different threads sending over channels to each other work. '''
        import threading
        commandChannel = scheduler.channel()

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
        channel = scheduler.channel()
        tasklet = scheduler.tasklet(f)(channel)
        tasklet.run()
        self.assertRaises(ValueError, channel.receive)
        tasklet = scheduler.tasklet(f)(channel)
        tasklet.run()
        try:
            channel.receive()
        except ValueError as e:
            self.assertEqual(e.args, (1, 2, 3))

    def testBlockingReceiveOnMainTasklet(self):

        sentValues = []

        def sender(chan):
            for i in range(0, 10):
                chan.send(i)
                sentValues.append(i)

        channel = scheduler.channel()

        sendingTasklet = scheduler.tasklet(sender)(channel)
        sendingTasklet.run()

        self.assertEqual(len(sentValues), 0)
        self.assertEqual(channel.balance, 1)

        for i in range(0, 10):
            channel.receive()

        self.assertEqual(channel.balance, 0)
        self.assertEqual(sentValues, [0,1,2,3,4,5,6,7,8,9])

    def testBlockingSendOnMainTasklet(self):


        receivedValues = []

        def receiver(chan):
            for i in range(0, 10):
                r = chan.receive()
                receivedValues.append(r)

        channel = scheduler.channel()

        sendingTasklet = scheduler.tasklet(receiver)(channel)
        sendingTasklet.run()

        self.assertEqual(len(receivedValues), 0)
        self.assertEqual(channel.balance, -1)

        for i in range(0, 10):
            channel.send(i)

        self.assertEqual(channel.balance, 0)
        self.assertEqual(receivedValues, [0,1,2,3,4,5,6,7,8,9])

    def testPreferenceSender(self):
        completedSendTasklets = []

        c = scheduler.channel()
        c.preference = 1

        def sender(chan, x):
            chan.send(x)
            completedSendTasklets.append(x)

        for i in range(10):
            tasklet = scheduler.tasklet(sender)(c, i)
            tasklet.run()

        for i in range(10):
            c.receive()
            self.assertEqual(i+1, len(completedSendTasklets))

    def testPreferenceReceiver(self):
        completedSendTasklets = []

        c = scheduler.channel()

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

    def testPreferenceNeither(self):
        completedTasklets = []

        c = scheduler.channel()

        c.preference = 0

        def sender(chan, x):
            chan.send("test")
            completedTasklets.append(("sender", x))

        def receiver(chan, x):
            res = chan.receive()
            completedTasklets.append(("receiver", x))
            self.assertEqual(res, "test")

        def justAnotherTasklet(x):
            completedTasklets.append(x)

        for i in range(10):
            scheduler.tasklet(sender)(c, i)

        for i in range(10):
            scheduler.tasklet(receiver)(c, 1)

        self.assertEqual(len(completedTasklets), 0)

        scheduler.tasklet(justAnotherTasklet)("first")

        scheduler.run()

        self.assertEqual(len(completedTasklets), 21)
