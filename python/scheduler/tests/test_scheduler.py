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

import unittest
import contextlib
import test_utils

@contextlib.contextmanager
def switch_trapped():
    scheduler.switch_trap(1)
    try:
        yield
    finally:
        scheduler.switch_trap(-1)



class TestTaskletRunOrder(test_utils.SchedulerTestCaseBase):
    
    def testTaskletRunOrder(self):
        completedSendTasklets = [""]

        def taskletCallable(x):
            completedSendTasklets[0] += "t" + str(x)

        t1 = scheduler.tasklet(taskletCallable)(1)
        scheduler.tasklet(taskletCallable)(2)
        scheduler.tasklet(taskletCallable)(3)

        self.assertEqual(self.getruncount(), 4)

        t1.run()

        self.assertEqual(self.getruncount(), 1)

        self.assertEqual(completedSendTasklets[0],"t1t2t3")

    def testTaskletRunOrder2(self):
        completedSendTasklets = [""]

        def taskletCallable(x):
            completedSendTasklets[0] += "t" + str(x)

        t1 = scheduler.tasklet(taskletCallable)(1)
        t2 = scheduler.tasklet(taskletCallable)(2)
        scheduler.tasklet(taskletCallable)(3)

        self.assertEqual(self.getruncount(), 4)

        t2.run()

        self.assertEqual(self.getruncount(), 2)

        t1.run()

        self.assertEqual(self.getruncount(), 1)

        self.assertEqual(completedSendTasklets[0],"t2t3t1")

class TestScheduleOrderBase(object):

    def testSchedulerRunOrder(self):
        completedSendTasklets = [""]

        def taskletCallable(x):
            completedSendTasklets[0] += "t" + str(x)

        scheduler.tasklet(taskletCallable)(1)
        scheduler.tasklet(taskletCallable)(2)
        scheduler.tasklet(taskletCallable)(3)

        self.assertEqual(self.getruncount(), 4)

        scheduler.run()

        self.assertEqual(self.getruncount(), 1)

        self.assertEqual(completedSendTasklets[0],"t1t2t3")

    def testNestedTaskletRunOrder(self):
        completedSendTasklets = [""]

        def taskletCallable(x):
            completedSendTasklets[0] += "t" + str(x)

        def createNestedTaskletRun():
            t2 = scheduler.tasklet(taskletCallable)(2)
            scheduler.tasklet(taskletCallable)(3)
            scheduler.tasklet(taskletCallable)(4)
            t2.run()

        scheduler.tasklet(taskletCallable)(1)
        scheduler.tasklet(createNestedTaskletRun)()
        scheduler.tasklet(taskletCallable)(5)

        self.assertEqual(self.getruncount(), 4)

        self.run_scheduler()

        self.assertEqual(self.getruncount(), 1)

        self.assertEqual(completedSendTasklets[0],"t1t2t3t4t5")


    def testNestedTaskletRunOrderWithSchedule(self):
        completedSendTasklets = [""]

        def taskletCallable(x):
            completedSendTasklets[0] += "t" + str(x)

        def schedule():
            scheduler.schedule()

        def createNestedTaskletRun():
            t2 = scheduler.tasklet(taskletCallable)(2)
            scheduler.tasklet(schedule)()
            scheduler.tasklet(taskletCallable)(3)
            t2.run()

        scheduler.tasklet(taskletCallable)(1)
        scheduler.tasklet(createNestedTaskletRun)()
        scheduler.tasklet(taskletCallable)(4)

        self.assertEqual(self.getruncount(), 4)

        self.run_scheduler()

        self.assertEqual(self.getruncount(), 1)

        self.assertEqual(completedSendTasklets[0],"t1t2t3t4")


    def testMultiLevelNestedTaskletRunOrderWithSchedule(self):
        completedSendTasklets = [""]

        def taskletCallable(x):
            completedSendTasklets[0] += "t" + str(x)

        def schedule():
            scheduler.schedule()

        def createNestedTaskletRun2():
            t2 = scheduler.tasklet(taskletCallable)(3)
            scheduler.tasklet(schedule)()
            scheduler.tasklet(taskletCallable)(4)
            t2.run()

        def createNestedTaskletRun():
            t2 = scheduler.tasklet(taskletCallable)(2)
            scheduler.tasklet(schedule)()
            scheduler.tasklet(createNestedTaskletRun2)()
            scheduler.tasklet(taskletCallable)(5)
            t2.run()

        scheduler.tasklet(taskletCallable)(1)
        scheduler.tasklet(createNestedTaskletRun)()
        scheduler.tasklet(taskletCallable)(6)

        self.assertEqual(self.getruncount(), 4)

        self.run_scheduler()

        self.assertEqual(self.getruncount(), 1)

        self.assertEqual(completedSendTasklets[0],"t1t2t3t4t5t6")

# Run all tasklets in queue
class TestScheduleOrderRunAll(test_utils.SchedulerTestCaseBase, TestScheduleOrderBase):
    Watchdog = False

    def run_scheduler(self):
        if self.Watchdog:
            while self.getruncount() > 1:
                scheduler.run_n_tasklets(1)
        else:
            scheduler.run()

# Run one tasklet at a time until queue has been evaluated
class TestScheduleOrderRunOne(TestScheduleOrderRunAll):
    Watchdog = True

class TestSchedule(test_utils.SchedulerTestCaseBase):

    def setUp(self):
        super().setUp()
        self.events = []

    def testSchedule(self):
        def foo(previous):
            self.events.append("foo")
            self.assertTrue(previous.scheduled)
        t = scheduler.tasklet(foo)(scheduler.getcurrent())
        self.assertEqual(self.getruncount(), 2)
        self.assertTrue(t.scheduled)
        scheduler.schedule()
        self.assertEqual(self.getruncount(), 1)
        self.assertEqual(self.events, ["foo"])

    def testScheduleRemoveFail(self):
        def nestedTasklet():
            def foo(previous):
                self.events.append("foo")
                self.assertFalse(previous.scheduled)
                previous.insert()
                self.assertTrue(previous.scheduled)
            t = scheduler.tasklet(foo)(scheduler.getcurrent())
            self.assertEqual(self.getruncount(), 3)
            scheduler.schedule_remove()
            self.assertEqual(self.getruncount(), 2)
            self.assertEqual(self.events, ["foo"])
        t = scheduler.tasklet(nestedTasklet)()
        t.run()


class TestRun(test_utils.SchedulerTestCaseBase):
    def testCallingRunFromNonMainTasklet(self):

        values = []
        def foo(x):
            values.append(x)

        def bar(chan):
            t = scheduler.tasklet(foo)("a")
            scheduler.tasklet(foo)("b")
            scheduler.tasklet(foo)("c")
            scheduler.tasklet(foo)("d")
            chan.send(1)
            scheduler.tasklet(foo)("e")
            scheduler.tasklet(foo)("f")
            scheduler.tasklet(foo)("g")
            scheduler.run()

        channel = scheduler.channel()
        t = scheduler.tasklet(bar)(channel)
        t.run()
        channel.receive()
        scheduler.run()
        self.assertEqual(values, ["a", "b", "c", "d", "e", "f", "g"])

class TestSwitch(test_utils.SchedulerTestCaseBase):
    """Test the new tasklet.switch() method, which allows
    explicit switching
    """
    def setUp(self):
        super().setUp()

        self.source = scheduler.getcurrent()
        self.finished = False
        self.c = scheduler.channel()

    def target(self):
        self.assertTrue(self.source.paused)
        self.source.insert()
        self.finished = True

    def blocked_target(self):
        self.c.receive()
        self.finished = True

    def test_switch(self):
        """Simple switch"""
        t = scheduler.tasklet(self.target)()
        self.assertEqual(self.getruncount(), 2)
        t.switch()
        self.assertEqual(self.getruncount(), 1)
        self.assertTrue(self.finished)

    @unittest.skip('TODO - This test looks broken, Stackless giving the same result')
    def test_switch_self(self):
        t = scheduler.getcurrent()
        t.switch()

    def test_switch_blocked(self):
        t = scheduler.tasklet(self.blocked_target)()
        self.assertEqual(self.getruncount(), 2)
        t.run()
        self.assertEqual(self.getruncount(), 1)
        self.assertTrue(t.blocked)
        self.assertRaisesRegex(RuntimeError, "blocked", t.switch)
        self.c.send(None)
        self.assertTrue(self.finished)

    def test_switch_paused(self):
        t = scheduler.tasklet(self.target)
        self.assertEqual(self.getruncount(), 1)
        t.bind(args=())
        self.assertEqual(self.getruncount(), 1)
        self.assertTrue(t.paused)
        t.switch()
        self.assertTrue(self.finished)
        self.assertEqual(self.getruncount(), 1)

    def test_switch_trapped(self):
        t = scheduler.tasklet(self.target)()
        self.assertEqual(self.getruncount(), 2)
        self.assertFalse(t.paused)
        with switch_trapped():
            self.assertRaisesRegex(RuntimeError, "switch_trap", t.switch)
        self.assertFalse(t.paused)
        t.switch()
        self.assertTrue(self.finished)
        self.assertEqual(self.getruncount(), 1)

    @unittest.skip('TODO - This test looks broken, Stackless giving the same result')
    def test_switch_self_trapped(self):
        t = scheduler.getcurrent()
        with switch_trapped():
            t.switch()  # ok, switching to ourselves!

    def test_switch_blocked_trapped(self):
        t = scheduler.tasklet(self.blocked_target)()
        self.assertEqual(self.getruncount(), 2)
        t.run()
        self.assertEqual(self.getruncount(), 1)
        self.assertTrue(t.blocked)
        with switch_trapped():
            self.assertRaisesRegex(RuntimeError, "blocked", t.switch)
        self.assertTrue(t.blocked)
        self.c.send(None)
        self.assertTrue(self.finished)

    def test_switch_paused_trapped(self):
        t = scheduler.tasklet(self.target)
        self.assertEqual(self.getruncount(), 1)
        t.bind(args=())
        self.assertEqual(self.getruncount(), 1)
        self.assertTrue(t.paused)
        with switch_trapped():
            self.assertRaisesRegex(RuntimeError, "switch_trap", t.switch)
        self.assertTrue(t.paused)
        t.switch()
        self.assertTrue(self.finished)
        self.assertEqual(self.getruncount(), 1)

class TestSwitchTrap(test_utils.SchedulerTestCaseBase):

    class SwitchTrap(object):

        def __enter__(self):
            scheduler.switch_trap(1)

        def __exit__(self, exc, val, tb):
            scheduler.switch_trap(-1)
    switch_trap = SwitchTrap()

    def test_schedule(self):
        s = scheduler.tasklet(lambda: None)()
        self.assertEqual(self.getruncount(), 2)
        with self.switch_trap:
            self.assertRaisesRegex(RuntimeError, "switch_trap", scheduler.schedule)
        scheduler.run()
        self.assertEqual(self.getruncount(), 1)
    
    def test_schedule_remove(self):
        main = []
        s = scheduler.tasklet(lambda: main[0].insert())()
        self.assertEqual(self.getruncount(), 2)
        with self.switch_trap:
            self.assertRaisesRegex(RuntimeError, "switch_trap", scheduler.schedule_remove)
        main.append(scheduler.getcurrent())
        scheduler.schedule_remove()
        self.assertEqual(self.getruncount(), 1)
    
    def test_run(self):
        s = scheduler.tasklet(lambda: None)()
        self.assertEqual(self.getruncount(), 2)
        with self.switch_trap:
            self.assertRaisesRegex(RuntimeError, "switch_trap", scheduler.run)
        scheduler.run()
        self.assertEqual(self.getruncount(), 1)
    
    def test_run_specific(self):
        s = scheduler.tasklet(lambda: None)()
        self.assertEqual(self.getruncount(), 2)
        with self.switch_trap:
            self.assertRaisesRegex(RuntimeError, "switch_trap", s.run)
        s.run()
        self.assertEqual(self.getruncount(), 1)
    
    def test_run_paused(self):
        s = scheduler.tasklet(lambda: None)
        self.assertEqual(self.getruncount(), 1)
        s.bind(args=())
        self.assertEqual(self.getruncount(), 1)
        self.assertTrue(s.paused)
        with self.switch_trap:
            self.assertRaisesRegex(RuntimeError, "switch_trap", s.run)
        self.assertTrue(s.paused)
        scheduler.run()
        self.assertEqual(self.getruncount(), 1)
    
    def test_send(self):
        c = scheduler.channel()
        s = scheduler.tasklet(lambda: c.receive())()
        self.assertEqual(self.getruncount(), 2)
        with self.switch_trap:
            self.assertRaisesRegex(RuntimeError, "switch_trap", c.send, None)
        c.send(None)
        self.assertEqual(self.getruncount(), 1)
    
    @unittest.skip('Not currently part of required stub, send_exception is implemented')
    def test_send_throw(self):
        c = scheduler.channel()
        def f():
            self.assertRaises(NotImplementedError, c.receive)
        s = scheduler.tasklet(f)()
        self.assertEqual(self.getruncount(), 2)
        with self.switch_trap:
            self.assertRaisesRegex(RuntimeError, "switch_trap", c.send_throw, NotImplementedError)
        c.send_throw(NotImplementedError)
        self.assertEqual(self.getruncount(), 1)

    def test_send_exception(self):
        c = scheduler.channel()
        def f():
            self.assertRaises(NotImplementedError, c.receive)
        s = scheduler.tasklet(f)()
        self.assertEqual(self.getruncount(), 2)
        with self.switch_trap:
            self.assertRaisesRegex(RuntimeError, "switch_trap", c.send_exception, NotImplementedError)
        c.send_exception(NotImplementedError)
        self.assertEqual(self.getruncount(), 1)
    
    def test_receive(self):
        c = scheduler.channel()
        s = scheduler.tasklet(lambda: c.send(1))()
        self.assertEqual(self.getruncount(), 2)
        with self.switch_trap:
            self.assertRaisesRegex(RuntimeError, "switch_trap", c.receive)
        self.assertEqual(c.receive(), 1)
        self.assertEqual(self.getruncount(), 2)
    
    @unittest.skip('TODO - send_throw not in required stub so not implemented')
    def test_receive_throw(self):
        c = scheduler.channel()
        s = scheduler.tasklet(lambda: c.send_throw(NotImplementedError))()
        self.assertEqual(self.getruncount(), 2)
        with self.switch_trap:
            self.assertRaisesRegex(RuntimeError, "switch_trap", c.receive)
        self.assertRaises(NotImplementedError, c.receive)
        self.assertEqual(self.getruncount(), 1)
    
    def test_raise_exception(self):
        c = scheduler.channel()
        def foo():
            self.assertRaises(IndexError, c.receive)
        s = scheduler.tasklet(foo)()
        self.assertEqual(self.getruncount(), 2)
        s.run()  # necessary, since raise_exception won't automatically run it
        self.assertEqual(self.getruncount(), 1)
        with self.switch_trap:
            self.assertRaisesRegex(RuntimeError, "switch_trap", s.raise_exception, RuntimeError)
        s.raise_exception(IndexError)
        self.assertEqual(self.getruncount(), 1)
    
    def test_kill(self):
        c = scheduler.channel()
        def foo():
            self.assertRaises(scheduler.TaskletExit, c.receive)
        s = scheduler.tasklet(foo)()
        self.assertEqual(self.getruncount(), 2)
        s.run()  # necessary, since raise_exception won't automatically run it
        with self.switch_trap:
            self.assertRaisesRegex(RuntimeError, "switch_trap", s.kill)
        s.kill()
        self.assertEqual(self.getruncount(), 1)
    
    def test_run2(self):
        c = scheduler.channel()
        def foo():
            pass
        s = scheduler.tasklet(foo)()
        self.assertEqual(self.getruncount(), 2)
        with self.switch_trap:
            self.assertRaisesRegex(RuntimeError, "switch_trap", s.run)
        s.run()
        self.assertEqual(self.getruncount(), 1)