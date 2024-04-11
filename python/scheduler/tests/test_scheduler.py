import scheduler
import unittest
import contextlib

@contextlib.contextmanager
def switch_trapped():
    scheduler.switch_trap(1)
    try:
        yield
    finally:
        scheduler.switch_trap(-1)



class TestTaskletRunOrder(unittest.TestCase):
    
    def testTaskletRunOrder(self):
        completedSendTasklets = [""]

        def taskletCallable(x):
            completedSendTasklets[0] += "t" + str(x)

        t1 = scheduler.tasklet(taskletCallable)(1)
        scheduler.tasklet(taskletCallable)(2)
        scheduler.tasklet(taskletCallable)(3)

        t1.run()

        self.assertEqual(completedSendTasklets[0],"t1t2t3")

    def testTaskletRunOrder2(self):
        completedSendTasklets = [""]

        def taskletCallable(x):
            completedSendTasklets[0] += "t" + str(x)

        t1 = scheduler.tasklet(taskletCallable)(1)
        t2 = scheduler.tasklet(taskletCallable)(2)
        scheduler.tasklet(taskletCallable)(3)

        t2.run()
        t1.run()

        self.assertEqual(completedSendTasklets[0],"t2t3t1")

class TestScheduleOrderBase(object):

    def testSchedulerRunOrder(self):
        completedSendTasklets = [""]

        def taskletCallable(x):
            completedSendTasklets[0] += "t" + str(x)

        scheduler.tasklet(taskletCallable)(1)
        scheduler.tasklet(taskletCallable)(2)
        scheduler.tasklet(taskletCallable)(3)

        scheduler.run()

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

        self.run_scheduler()

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

        self.run_scheduler()

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

        self.run_scheduler()

        self.assertEqual(completedSendTasklets[0],"t1t2t3t4t5t6")

# Run all tasklets in queue
class TestScheduleOrderRunAll(unittest.TestCase, TestScheduleOrderBase):
    Watchdog = False

    def setUp(self):
        pass
        
    def tearDown(self):
        pass

    @classmethod
    def run_scheduler(cls):
        if cls.Watchdog:
            while scheduler.getruncount() > 1:
                scheduler.run_n_tasklets(1)
        else:
            scheduler.run()

# Run one tasklet at a time until queue has been evaluated
class TestScheduleOrderRunOne(TestScheduleOrderRunAll):
    Watchdog = True

    def setUp(self):
        pass
        
    def tearDown(self):
        pass

class TestSchedule(unittest.TestCase):

    def setUp(self):
        self.events = []
        
    def tearDown(self):
        pass

    def testSchedule(self):
        def foo(previous):
            self.events.append("foo")
            self.assertTrue(previous.scheduled)
        t = scheduler.tasklet(foo)(scheduler.getcurrent())
        self.assertTrue(t.scheduled)
        scheduler.schedule()
        self.assertEqual(self.events, ["foo"])

    def testScheduleRemoveFail(self):
        def foo(previous):
            self.events.append("foo")
            self.assertFalse(previous.scheduled)
            previous.insert()
            self.assertTrue(previous.scheduled)
        t = scheduler.tasklet(foo)(scheduler.getcurrent())
        scheduler.schedule_remove()
        self.assertEqual(self.events, ["foo"])


class TestSwitch(unittest.TestCase):
    """Test the new tasklet.switch() method, which allows
    explicit switching
    """

    def setUp(self):
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
        t.switch()
        self.assertTrue(self.finished)

    @unittest.skip('TODO - This test looks broken, Stackless giving the same result')
    def test_switch_self(self):
        t = scheduler.getcurrent()
        t.switch()

    def test_switch_blocked(self):
        t = scheduler.tasklet(self.blocked_target)()
        t.run()
        self.assertTrue(t.blocked)
        self.assertRaisesRegex(RuntimeError, "blocked", t.switch)
        self.c.send(None)
        self.assertTrue(self.finished)

    def test_switch_paused(self):
        t = scheduler.tasklet(self.target)
        t.bind(args=())
        self.assertTrue(t.paused)
        t.switch()
        self.assertTrue(self.finished)

    def test_switch_trapped(self):
        t = scheduler.tasklet(self.target)()
        self.assertFalse(t.paused)
        with switch_trapped():
            self.assertRaisesRegex(RuntimeError, "switch_trap", t.switch)
        self.assertFalse(t.paused)
        t.switch()
        self.assertTrue(self.finished)

    @unittest.skip('TODO - This test looks broken, Stackless giving the same result')
    def test_switch_self_trapped(self):
        t = scheduler.getcurrent()
        with switch_trapped():
            t.switch()  # ok, switching to ourselves!

    def test_switch_blocked_trapped(self):
        t = scheduler.tasklet(self.blocked_target)()
        t.run()
        self.assertTrue(t.blocked)
        with switch_trapped():
            self.assertRaisesRegex(RuntimeError, "blocked", t.switch)
        self.assertTrue(t.blocked)
        self.c.send(None)
        self.assertTrue(self.finished)

    def test_switch_paused_trapped(self):
        t = scheduler.tasklet(self.target)
        t.bind(args=())
        self.assertTrue(t.paused)
        with switch_trapped():
            self.assertRaisesRegex(RuntimeError, "switch_trap", t.switch)
        self.assertTrue(t.paused)
        t.switch()
        self.assertTrue(self.finished)

class TestSwitchTrap(unittest.TestCase):

    class SwitchTrap(object):

        def __enter__(self):
            scheduler.switch_trap(1)

        def __exit__(self, exc, val, tb):
            scheduler.switch_trap(-1)
    switch_trap = SwitchTrap()

    def test_schedule(self):
        s = scheduler.tasklet(lambda: None)()
        with self.switch_trap:
            self.assertRaisesRegex(RuntimeError, "switch_trap", scheduler.schedule)
        scheduler.run()
    
    def test_schedule_remove(self):
        main = []
        s = scheduler.tasklet(lambda: main[0].insert())()
        with self.switch_trap:
            self.assertRaisesRegex(RuntimeError, "switch_trap", scheduler.schedule_remove)
        main.append(scheduler.getcurrent())
        scheduler.schedule_remove()
    
    def test_run(self):
        s = scheduler.tasklet(lambda: None)()
        with self.switch_trap:
            self.assertRaisesRegex(RuntimeError, "switch_trap", scheduler.run)
        scheduler.run()
    
    def test_run_specific(self):
        s = scheduler.tasklet(lambda: None)()
        with self.switch_trap:
            self.assertRaisesRegex(RuntimeError, "switch_trap", s.run)
        s.run()
    
    def test_run_paused(self):
        s = scheduler.tasklet(lambda: None)
        s.bind(args=())
        self.assertTrue(s.paused)
        with self.switch_trap:
            self.assertRaisesRegex(RuntimeError, "switch_trap", s.run)
        self.assertTrue(s.paused)
        scheduler.run()
    
    def test_send(self):
        c = scheduler.channel()
        s = scheduler.tasklet(lambda: c.receive())()
        with self.switch_trap:
            self.assertRaisesRegex(RuntimeError, "switch_trap", c.send, None)
        c.send(None)
    
    def test_send_throw(self):
        c = scheduler.channel()
        def f():
            self.assertRaises(NotImplementedError, c.receive)
        s = scheduler.tasklet(f)()
        with self.switch_trap:
            self.assertRaisesRegex(RuntimeError, "switch_trap", c.send_throw, NotImplementedError)
        c.send_throw(NotImplementedError)
    
    def test_receive(self):
        c = scheduler.channel()
        s = scheduler.tasklet(lambda: c.send(1))()
        with self.switch_trap:
            self.assertRaisesRegex(RuntimeError, "switch_trap", c.receive)
        self.assertEqual(c.receive(), 1)
    
    @unittest.skip('TODO - send_throw not in required stub so not implemented')
    def test_receive_throw(self):
        c = scheduler.channel()
        s = scheduler.tasklet(lambda: c.send_throw(NotImplementedError))()
        with self.switch_trap:
            self.assertRaisesRegex(RuntimeError, "switch_trap", c.receive)
        self.assertRaises(NotImplementedError, c.receive)
    
    def test_raise_exception(self):
        c = scheduler.channel()
        def foo():
            self.assertRaises(IndexError, c.receive)
        s = scheduler.tasklet(foo)()
        s.run()  # necessary, since raise_exception won't automatically run it
        with self.switch_trap:
            self.assertRaisesRegex(RuntimeError, "switch_trap", s.raise_exception, RuntimeError)
        s.raise_exception(IndexError)
    
    def test_kill(self):
        from scheduler import TaskletExit
        c = scheduler.channel()
        def foo():
            self.assertRaises(TaskletExit, c.receive)
        s = scheduler.tasklet(foo)()
        s.run()  # necessary, since raise_exception won't automatically run it
        with self.switch_trap:
            self.assertRaisesRegex(RuntimeError, "switch_trap", s.kill)
        s.kill()
    
    def test_run2(self):
        c = scheduler.channel()
        def foo():
            pass
        s = scheduler.tasklet(foo)()
        with self.switch_trap:
            self.assertRaisesRegex(RuntimeError, "switch_trap", s.run)
        s.run()