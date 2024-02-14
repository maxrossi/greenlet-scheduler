from __future__ import absolute_import
# import common

import unittest
import scheduler
import sys
import traceback
import weakref
import types
import contextlib
import time
import os
import struct
import gc
import contextvars
from _scheduler import _test_nostacklesscall as apply_not_stackless
import _testscheduler

try:
    import _thread as thread
    import threading
    withThreads = True
except:
    withThreads = False

from support import (SchedulerTestCase, AsTaskletTestCase, require_one_thread,
                     testcase_leaks_references, is_zombie)


"""
Joseph Frangoudes: Keeping this around, there are two many tests here to go through them one by one,
I'm going to wait until we start running the tests against the scheduler extension to work out 
which ones we want to keep & which ones aren't relevant to us.
"""


def is_soft():
    softswitch = scheduler.enable_softswitch(0)
    scheduler.enable_softswitch(softswitch)
    return softswitch


def runtask():
    x = 0
    # evoke pickling of an range object
    dummy = range(10)
    for ii in range(1000):
        x += 1


@contextlib.contextmanager
def switch_trapped():
    scheduler.switch_trap(1)
    try:
        yield
    finally:
        scheduler.switch_trap(-1)


class TestWatchdog(SchedulerTestCase):

    def lifecycle(self, t):
        # Initial state - unrun
        self.assertTrue(t.alive)
        self.assertTrue(t.scheduled)
        self.assertEqual(t.recursion_depth, 0)
        # allow hard switching
        t.set_ignore_nesting(1)

        softSwitching = scheduler.enable_softswitch(0)
        scheduler.enable_softswitch(softSwitching)

        # Run a little
        res = scheduler.run(10)
        self.assertEqual(t, res)
        self.assertTrue(t.alive)
        self.assertTrue(t.paused)
        self.assertFalse(t.scheduled)
        self.assertEqual(t.recursion_depth, 1)

        # Push back onto queue
        t.insert()
        self.assertFalse(t.paused)
        self.assertTrue(t.scheduled)

        # Run to completion
        scheduler.run()
        self.assertFalse(t.alive)
        self.assertFalse(t.scheduled)
        self.assertEqual(t.recursion_depth, 0)

    def test_aliveness1(self):
        """ Test flags after being run. """
        t = scheduler.tasklet(runtask)()
        self.lifecycle(t)

    @SchedulerTestCase.prepare_pickle_test_method
    def test_aliveness2(self):
        """ Same as 1, but with a pickled unrun tasklet. """
        t = scheduler.tasklet(runtask)()
        t_new = self.loads(self.dumps((t)))
        t.remove()
        t_new.insert()
        self.lifecycle(t_new)

    @SchedulerTestCase.prepare_pickle_test_method
    def test_aliveness3(self):
        """ Same as 1, but with a pickled run(slightly) tasklet. """

        t = scheduler.tasklet(runtask)()
        t.set_ignore_nesting(1)

        # Initial state - unrun
        self.assertTrue(t.alive)
        self.assertTrue(t.scheduled)
        self.assertEqual(t.recursion_depth, 0)

        softSwitching = scheduler.enable_softswitch(0)
        scheduler.enable_softswitch(softSwitching)

        # Run a little
        res = scheduler.run(100)

        self.assertEqual(t, res)
        self.assertTrue(t.alive)
        self.assertTrue(t.paused)
        self.assertFalse(t.scheduled)
        self.assertEqual(t.recursion_depth, 1)

        # Now save & load
        dumped = self.dumps(t)
        t_new = self.loads(dumped)

        # Remove and insert & swap names around a bit
        t.remove()
        t = t_new
        del t_new
        t.insert()

        self.assertTrue(t.alive)
        self.assertFalse(t.paused)
        self.assertTrue(t.scheduled)
        self.assertEqual(t.recursion_depth, 1)

        # Run to completion
        if is_soft():
            scheduler.run()
        else:
            t.kill()
        self.assertFalse(t.alive)
        self.assertFalse(t.scheduled)
        self.assertEqual(t.recursion_depth, 0)


class TestTaskletSwitching(SchedulerTestCase):
    """Test the tasklet's own scheduling methods"""

    def test_raise_exception(self):
        c = scheduler.channel()

        def foo():
            self.assertRaises(IndexError, c.receive)
        s = scheduler.tasklet(foo)()
        s.run()  # necessary, since raise_exception won't automatically run it
        s.raise_exception(IndexError)

    def test_run(self):
        c = scheduler.channel()
        flag = [False]

        def foo():
            flag[0] = True
        s = scheduler.tasklet(foo)()
        s.run()
        self.assertEqual(flag[0], True)


class TestTaskletThrowBase(object):

    def test_throw_noargs(self):
        c = scheduler.channel()

        def foo():
            self.assertRaises(IndexError, c.receive)
        s = scheduler.tasklet(foo)()
        s.run()  # It needs to have started to run
        self.throw(s, IndexError)
        self.aftercheck(s)

    def test_throw_args(self):
        c = scheduler.channel()

        def foo():
            try:
                c.receive()
            except Exception as e:
                self.assertTrue(isinstance(e, IndexError))
                self.assertEqual(e.args, (1, 2, 3))
        s = scheduler.tasklet(foo)()
        s.run()  # It needs to have started to run
        self.throw(s, IndexError, (1, 2, 3))
        self.aftercheck(s)

    def test_throw_inst(self):
        c = scheduler.channel()

        def foo():
            try:
                c.receive()
            except Exception as e:
                self.assertTrue(isinstance(e, IndexError))
                self.assertEqual(e.args, (1, 2, 3))
        s = scheduler.tasklet(foo)()
        s.run()  # It needs to have started to run
        self.throw(s, IndexError(1, 2, 3))
        self.aftercheck(s)

    def test_throw_exc_info(self):
        c = scheduler.channel()

        def foo():
            try:
                c.receive()
            except Exception as e:
                self.assertTrue(isinstance(e, ZeroDivisionError))
        s = scheduler.tasklet(foo)()
        s.run()  # It needs to have started to run

        def errfunc():
            1 / 0
        try:
            errfunc()
        except Exception:
            self.throw(s, *sys.exc_info())
        self.aftercheck(s)

    def test_throw_traceback(self):
        c = scheduler.channel()

        def foo():
            try:
                c.receive()
            except Exception:
                s = "".join(traceback.format_tb(sys.exc_info()[2]))
                self.assertTrue("errfunc" in s)
        s = scheduler.tasklet(foo)()
        s.run()  # It needs to have started to run

        def errfunc():
            1 / 0
        try:
            errfunc()
        except Exception:
            self.throw(s, *sys.exc_info())
        self.aftercheck(s)

    def test_new(self):
        c = scheduler.channel()

        def foo():
            try:
                c.receive()
            except Exception as e:
                self.assertTrue(isinstance(e, IndexError))
                raise
        s = scheduler.tasklet(foo)()
        self.assertEqual(s.frame, None)
        self.assertTrue(s.alive)
        # Test that the current "unhandled exception behaviour"
        # is invoked for the not-yet-running tasklet.

        def doit():
            self.throw(s, IndexError)
        if not self.pending:
            self.assertRaises(IndexError, doit)
        else:
            doit()
            self.assertRaises(IndexError, scheduler.run)

    def test_kill_new(self):
        def t():
            self.assertFalse("should not run this")
        s = scheduler.tasklet(t)()

        # Should not do anything
        s.throw(TaskletExit)
        # the tasklet should be dead
        scheduler.run()
        self.assertRaisesRegex(RuntimeError, "dead", s.run)

    def test_dead(self):
        c = scheduler.channel()

        def foo():
            c.receive()
        s = scheduler.tasklet(foo)()
        s.run()
        c.send(None)
        scheduler.run()
        self.assertFalse(s.alive)

        def doit():
            self.throw(s, IndexError)
        self.assertRaises(RuntimeError, doit)

    def test_kill_dead(self):
        c = scheduler.channel()

        def foo():
            c.receive()
        s = scheduler.tasklet(foo)()
        s.run()
        c.send(None)
        scheduler.run()
        self.assertFalse(s.alive)

        def doit():
            self.throw(s, TaskletExit)
        # nothing should happen here.
        doit()

    def test_throw_invalid(self):
        s = scheduler.getcurrent()

        def t():
            self.throw(s)
        self.assertRaises(TypeError, t)

        def t():  # @DuplicatedSignature
            self.throw(s, IndexError(1), (1, 2, 3))
        self.assertRaises(TypeError, t)


class TestTaskletThrowImmediate(SchedulerTestCase, TestTaskletThrowBase):
    pending = False

    @classmethod
    def throw(cls, s, *args):
        s.throw(*args, pending=cls.pending)

    def aftercheck(self, s):
        # the tasklet ran immediately
        self.assertFalse(s.alive)


class TestTaskletThrowNonImmediate(TestTaskletThrowImmediate):
    pending = True

    def aftercheck(self, s):
        # After the throw, the tasklet still hasn't run
        self.assertTrue(s.alive)
        s.run()
        self.assertFalse(s.alive)


class TestSwitchTrap(SchedulerTestCase):

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


class TestKill(SchedulerTestCase):
    SLP_TASKLET_KILL_REBINDS_THREAD = False  # see tasklet.c function impl_tasklet_kill()

    def test_kill_pending_true(self):
        killed = [False]

        def foo():
            try:
                scheduler.schedule()
            except TaskletExit:
                killed[0] = True
                raise
        t = scheduler.tasklet(foo)()
        t.run()
        self.assertFalse(killed[0])
        t.kill(pending=True)
        self.assertFalse(killed[0])
        t.run()
        self.assertTrue(killed[0])

    def test_kill_pending_False(self):
        killed = [False]

        def foo():
            try:
                scheduler.schedule()
            except TaskletExit:
                killed[0] = True
                raise
        t = scheduler.tasklet(foo)()
        t.run()
        self.assertFalse(killed[0])
        t.kill(pending=False)
        self.assertTrue(killed[0])

    def test_kill_current(self):
        killed = [False]

        def task():
            try:
                scheduler.current.kill()
            except TaskletExit:
                killed[0] = True
                raise
        t = scheduler.tasklet(task)()
        t.run()
        self.assertTrue(killed[0])
        self.assertFalse(t.alive)
        self.assertEqual(t.thread_id, scheduler.current.thread_id)

    @unittest.skipUnless(withThreads, "requires thread support")
    @require_one_thread
    def test_kill_thread_without_main_tasklet(self):
        # this test depends on a race condition.
        # unfortunately I do not have any better test case

        # This lock is used as a simple event variable.
        ready = thread.allocate_lock()
        ready.acquire()

        channel = scheduler.channel()
        tlet = scheduler.tasklet()
        self.tlet = tlet

        class DelayError(Exception):
            def __str__(self):
                time.sleep(0.05)
                return super(DelayError, self).__str__()

        # catch stderr
        self.addCleanup(setattr, sys, "stderr", sys.stderr)
        sys.stderr = open(os.devnull, "w")
        self.addCleanup(sys.stderr.close)

        def other_thread_main():
            tlet.bind_thread()
            tlet.bind(channel.receive, ())
            tlet.run()
            ready.release()
            raise DelayError("a slow exception")
            # during the processing of this exception the
            # thread has no main tasklet. Exception processing
            # takes some time. During this time the main thread
            # kills the tasklet

        thread.start_new_thread(other_thread_main, ())
        ready.acquire()  # Be sure the other thread is ready.
        #print("at end")
        is_blocked = tlet.blocked
        #tlet.bind_thread()
        try:
            tlet.kill(pending=True)
        except RuntimeError as e:
            self.assertIn("Target thread isn't initialised", str(e))
            # print("got exception")
        else:
            # print("no exception")
            pass
        self.assertTrue(is_blocked)
        time.sleep(0.5)
        # print("unbinding done")

    def _test_kill_without_thread_state(self, nl, block):
        channel = scheduler.channel()
        loop = True

        def task():
            while loop:
                try:
                    if block:
                        channel.receive()
                    else:
                        scheduler.main.run()
                except TaskletExit:
                    pass

        def other_thread_main():
            tlet.bind_thread()
            tlet.run()

        if nl == 0:
            tlet = scheduler.tasklet().bind(task, ())
        else:
            tlet = scheduler.tasklet().bind(apply_not_stackless, (task,))
        t = threading.Thread(target=other_thread_main, name="other thread")
        t.start()
        t.join()
        time.sleep(0.05)  # time for other_thread to clear its state

        loop = False
        if block:
            self.assertTrue(tlet.blocked)
        else:
            self.assertFalse(tlet.blocked)
        self.assertFalse(tlet.alive)
        self.assertEqual(tlet.thread_id, -1)
        self.assertRaisesRegex(RuntimeError, "tasklet has no thread", tlet.throw, TaskletExit, pending=True)
        tlet.kill(pending=True)
        self.assertFalse(tlet.blocked)
        if self.SLP_TASKLET_KILL_REBINDS_THREAD and scheduler.enable_softswitch(None) and nl == 0:
            # rebinding and soft switching
            self.assertTrue(tlet.scheduled)
            self.assertTrue(tlet.alive)
            tlet.remove()
            tlet.bind(None)
        else:
            # hard switching
            self.assertFalse(tlet.scheduled)
            self.assertIsNone(tlet.next)
            self.assertIsNone(tlet.prev)
            self.assertFalse(tlet.alive)
            tlet.remove()
            tlet.kill()

    @unittest.skipUnless(withThreads, "requires thread support")
    @testcase_leaks_references("chatches TaskletExit and does not die in its own thread", soft_switching=False)
    def test_kill_without_thread_state_nl0(self):
        return self._test_kill_without_thread_state(0, False)

    @unittest.skipUnless(withThreads, "requires thread support")
    @testcase_leaks_references("chatches TaskletExit and does not die in its own thread")
    def test_kill_without_thread_state_nl1(self):
        return self._test_kill_without_thread_state(1, False)

    @unittest.skipUnless(withThreads, "requires thread support")
    @testcase_leaks_references("chatches TaskletExit and does not die in its own thread", soft_switching=False)
    def test_kill_without_thread_state_blocked_nl0(self):
        return self._test_kill_without_thread_state(0, True)

    @unittest.skipUnless(withThreads, "requires thread support")
    @testcase_leaks_references("chatches TaskletExit and does not die in its own thread")
    def test_kill_without_thread_state_blocked_nl1(self):
        return self._test_kill_without_thread_state(1, True)


class TestErrorHandler(SchedulerTestCase):

    def setUp(self):
        super(TestErrorHandler, self).setUp()
        self.handled = self.ran = 0
        self.handled_tasklet = None

    def test_set(self):
        def foo():
            pass
        self.assertEqual(scheduler.set_error_handler(foo), None)
        self.assertEqual(scheduler.set_error_handler(None), foo)
        self.assertEqual(scheduler.set_error_handler(None), None)

    @contextlib.contextmanager
    def handlerctxt(self, handler):
        old = scheduler.set_error_handler(handler)
        try:
            yield()
        finally:
            scheduler.set_error_handler(old)

    def handler(self, exc, val, tb):
        self.assertTrue(exc)
        self.handled = 1
        self.handled_tasklet = scheduler.getcurrent()

    def borken_handler(self, exc, val, tb):
        self.handled = 1
        raise IndexError("we are the mods")

    def get_handler(self):
        h = scheduler.set_error_handler(None)
        scheduler.set_error_handler(h)
        return h

    def func(self, handler):
        self.ran = 1
        self.assertEqual(self.get_handler(), handler)
        raise ZeroDivisionError("I am borken")

    def test_handler(self):
        scheduler.tasklet(self.func)(self.handler)
        with self.handlerctxt(self.handler):
            scheduler.run()
        self.assertTrue(self.ran)
        self.assertTrue(self.handled)

    def test_borken_handler(self):
        scheduler.tasklet(self.func)(self.borken_handler)
        with self.handlerctxt(self.borken_handler):
            self.assertRaisesRegex(IndexError, "mods", scheduler.run)
        self.assertTrue(self.ran)
        self.assertTrue(self.handled)

    def test_early_hrow(self):
        "test that we handle errors thrown before the tasklet function runs"
        s = scheduler.tasklet(self.func)(self.handler)
        with self.handlerctxt(self.handler):
            s.throw(ZeroDivisionError, "thrown error")
        self.assertFalse(self.ran)
        self.assertTrue(self.handled)

    def test_getcurrent(self):
        # verify that the error handler runs in the context of the exiting tasklet
        s = scheduler.tasklet(self.func)(self.handler)
        with self.handlerctxt(self.handler):
            s.throw(ZeroDivisionError, "thrown error")
        self.assertTrue(self.handled_tasklet is s)

        self.handled_tasklet = None
        s = scheduler.tasklet(self.func)(self.handler)
        with self.handlerctxt(self.handler):
            s.run()
        self.assertTrue(self.handled_tasklet is s)

    def test_throw_pending(self):
        # make sure that throwing a pending error doesn't immediately throw
        def func(h):
            self.ran = 1
            scheduler.schedule()
            self.func(h)
        s = scheduler.tasklet(func)(self.handler)
        s.run()
        self.assertTrue(self.ran)
        self.assertFalse(self.handled)
        with self.handlerctxt(self.handler):
            s.throw(ZeroDivisionError, "thrown error", pending=True)
        self.assertEqual(self.handled_tasklet, None)
        with self.handlerctxt(self.handler):
            s.run()
        self.assertEqual(self.handled_tasklet, s)
#
# Test context manager soft switching support
# See http://www.stackless.com/ticket/22
#


def _create_contextlib_test_classes():
    import test.test_contextlib as module
    g = globals()
    for name in dir(module):
        obj = getattr(module, name, None)
        if not (isinstance(obj, type) and issubclass(obj, unittest.TestCase)):
            continue
        g[name] = type(name, (AsTaskletTestCase, obj), {})

_create_contextlib_test_classes()


class TestContextManager(SchedulerTestCase):

    def nestingLevel(self):
        self.assertFalse(scheduler.getcurrent().nesting_level)

        class C(object):

            def __enter__(self_):  # @NoSelf
                self.assertFalse(scheduler.getcurrent().nesting_level)
                return self_

            def __exit__(self_, exc_type, exc_val, exc_tb):  # @NoSelf
                self.assertFalse(scheduler.getcurrent().nesting_level)
                return False
        with C() as c:
            self.assertTrue(isinstance(c, C))

    def test_nestingLevel(self):
        if not scheduler.enable_softswitch(None):
            # the test requires softswitching
            return
        scheduler.tasklet(self.nestingLevel)()
        scheduler.run()


class TestAtomic(SchedulerTestCase):
    """Test the getting and setting of the tasklet's 'atomic' flag, and the
       context manager to set it to True
    """

    def testAtomic(self):
        old = scheduler.getcurrent().atomic
        try:
            val = scheduler.getcurrent().set_atomic(False)
            self.assertEqual(val, old)
            self.assertEqual(scheduler.getcurrent().atomic, False)

            val = scheduler.getcurrent().set_atomic(True)
            self.assertEqual(val, False)
            self.assertEqual(scheduler.getcurrent().atomic, True)

            val = scheduler.getcurrent().set_atomic(True)
            self.assertEqual(val, True)
            self.assertEqual(scheduler.getcurrent().atomic, True)

            val = scheduler.getcurrent().set_atomic(False)
            self.assertEqual(val, True)
            self.assertEqual(scheduler.getcurrent().atomic, False)

        finally:
            scheduler.getcurrent().set_atomic(old)
        self.assertEqual(scheduler.getcurrent().atomic, old)

    def testAtomicCtxt(self):
        old = scheduler.getcurrent().atomic
        scheduler.getcurrent().set_atomic(False)
        try:
            with scheduler.atomic():
                self.assertTrue(scheduler.getcurrent().atomic)
        finally:
            scheduler.getcurrent().set_atomic(old)

    def testAtomicNopCtxt(self):
        old = scheduler.getcurrent().atomic
        scheduler.getcurrent().set_atomic(True)
        try:
            with scheduler.atomic():
                self.assertTrue(scheduler.getcurrent().atomic)
        finally:
            scheduler.getcurrent().set_atomic(old)


class TestSchedule(AsTaskletTestCase):

    def setUp(self):
        super(TestSchedule, self).setUp()
        self.events = []

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


class TestBind(SchedulerTestCase):

    def setUp(self):
        super(TestBind, self).setUp()
        self.finally_run_count = 0
        self.args = self.kwargs = None

    def task(self, with_c_state):
        try:
            if with_c_state:
                _testscheduler.test_cstate(lambda: scheduler.schedule_remove(None))
            else:
                scheduler.schedule_remove(None)
        finally:
            self.finally_run_count += 1

    def argstest(self, *args, **kwargs):
        self.args = args
        self.kwargs = dict(kwargs)

    def assertArgs(self, args, kwargs):
        self.assertEqual(args, self.args)
        self.assertEqual(kwargs, self.kwargs)

    def test_bind(self):
        t = scheduler.tasklet()
        wr = weakref.ref(t)

        self.assertFalse(t.alive)
        self.assertIsNone(t.frame)
        self.assertEqual(t.nesting_level, 0)

        t.bind(None)  # must not change the tasklet

        self.assertFalse(t.alive)
        self.assertIsNone(t.frame)
        self.assertEqual(t.nesting_level, 0)

        t.bind(self.task)
        t.setup(False)

        scheduler.run()
        self.assertFalse(t.scheduled)
        self.assertTrue(t.alive)
        if scheduler.enable_softswitch(None):
            self.assertTrue(t.restorable)
        self.assertIsInstance(t.frame, types.FrameType)

        t.insert()
        scheduler.run()

        # remove the tasklet. Must run the finally clause
        t = None
        self.assertIsNone(wr())  # tasklet has been deleted
        self.assertEqual(self.finally_run_count, 1)

    def test_bind_fail_not_callable(self):
        class C(object):
            pass
        self.assertRaisesRegex(TypeError, "callable", scheduler.getcurrent().bind, C())

    def test_unbind_ok(self):
        if not scheduler.enable_softswitch(None):
            # the test requires softswitching
            return
        t = scheduler.tasklet(self.task)(False)
        wr = weakref.ref(t)

        # prepare a paused tasklet
        scheduler.run()
        self.assertFalse(t.scheduled)
        self.assertTrue(t.alive)
        self.assertEqual(t.nesting_level, 0)
        self.assertIsInstance(t.frame, types.FrameType)

        t.bind(None)
        self.assertFalse(t.alive)
        self.assertIsNone(t.frame)

        # remove the tasklet. Must not run the finally clause
        t = None
        self.assertIsNone(wr())  # tasklet has been deleted
        self.assertEqual(self.finally_run_count, 0)

    def test_unbind_fail_current(self):
        self.assertRaisesRegex(RuntimeError, "current tasklet", scheduler.getcurrent().bind, None)

    def test_unbind_fail_scheduled(self):
        t = scheduler.tasklet(self.task)(False)

        # prepare a paused tasklet
        scheduler.run()
        t.insert()
        self.assertTrue(t.scheduled)
        self.assertTrue(t.alive)
        self.assertIsInstance(t.frame, types.FrameType)

        self.assertRaisesRegex(RuntimeError, "scheduled", t.bind, None)

    def test_unbind_fail_cstate(self):
        t = scheduler.tasklet(self.task)(True)
        wr = weakref.ref(t)

        # prepare a paused tasklet
        scheduler.run()
        self.assertFalse(t.scheduled)
        self.assertTrue(t.alive)
        self.assertGreaterEqual(t.nesting_level, 1)
        self.assertIsInstance(t.frame, types.FrameType)

        self.assertRaisesRegex(RuntimeError, "C state", t.bind, None)

        # remove the tasklet. Must run the finally clause
        t = None
        self.assertIsNone(wr())  # tasklet has been deleted
        self.assertEqual(self.finally_run_count, 1)

    def test_bind_noargs(self):
        t = scheduler.tasklet(self.task)
        t.bind(self.argstest)
        self.assertRaises(RuntimeError, t.run)

    def test_bind_args(self):
        args = "foo", "bar"
        t = scheduler.tasklet(self.task)
        t.bind(self.argstest, args)
        t.run()
        self.assertArgs(args, {})

        t = scheduler.tasklet(self.task)
        t.bind(self.argstest, args=args)
        t.run()
        self.assertArgs(args, {})

    def test_bind_kwargs(self):
        t = scheduler.tasklet(self.task)
        kwargs = {"hello": "world"}
        t.bind(self.argstest, None, kwargs)
        t.run()
        self.assertArgs((), kwargs)

        t = scheduler.tasklet(self.task)
        t.bind(self.argstest, kwargs=kwargs)
        t.run()
        self.assertArgs((), kwargs)

    def test_bind_args_kwargs(self):
        args = ("foo", "bar")
        kwargs = {"hello": "world"}

        t = scheduler.tasklet(self.task)
        t.bind(self.argstest, args, kwargs)
        t.run()
        self.assertArgs(args, kwargs)

        t = scheduler.tasklet(self.task)
        t.bind(self.argstest, args=args, kwargs=kwargs)
        t.run()
        self.assertArgs(args, kwargs)

    def test_bind_args_kwargs_nofunc(self):
        args = ("foo", "bar")
        kwargs = {"hello": "world"}

        t = scheduler.tasklet(self.argstest)
        t.bind(None, args, kwargs)
        t.run()
        self.assertArgs(args, kwargs)

        t = scheduler.tasklet(self.argstest)
        t.bind(args=args, kwargs=kwargs)
        t.run()
        self.assertArgs(args, kwargs)

    def test_bind_args_not_runnable(self):
        args = ("foo", "bar")
        kwargs = {"hello": "world"}

        t = scheduler.tasklet(self.task)
        t.bind(self.argstest, args, kwargs)
        self.assertFalse(t.scheduled)
        t.run()

    @unittest.skipUnless(withThreads, "requires thread support")
    def test_unbind_main(self):
        self.skipUnlessSoftswitching()

        done = []

        def other():
            main = scheduler.main
            self.assertRaisesRegex(RuntimeError, "can't unbind the main tasklet", main.bind, None)

        # the initial nesting level depends on the test runner.
        # We need a main tasklet with nesting_level == 0. Therefore we
        # use a thread
        def other_thread():
            self.assertEqual(scheduler.current.nesting_level, 0)
            self.assertIs(scheduler.current, scheduler.main)
            scheduler.tasklet(other)().switch()
            done.append(True)

        t = threading.Thread(target=other_thread, name="other thread")
        t.start()
        t.join()
        self.assertTrue(done[0])

    @unittest.skipUnless(withThreads, "requires thread support")
    def test_rebind_main(self):
        # rebind the main tasklet of a thread. This is highly discouraged,
        # because it will deadlock, if the thread is a non daemon threading.Thread.
        self.skipUnlessSoftswitching()

        ready = thread.allocate_lock()
        ready.acquire()

        self.target_called = False
        self.main_returned = False

        def target():
            self.target_called = True
            ready.release()

        def other_thread_main():
            self.assertTrue(scheduler.current.is_main)
            try:
                scheduler.tasklet(scheduler.main.bind)(target, ()).switch()
            finally:
                self.main_returned = True
                ready.release()

        thread.start_new_thread(other_thread_main, ())
        ready.acquire()

        self.assertTrue(self.target_called)
        self.assertFalse(self.main_returned)

    def test_rebind_recursion_depth(self):
        self.skipUnlessSoftswitching()
        self.recursion_depth_in_test = None

        def tasklet_outer():
            tasklet_inner()

        def tasklet_inner():
            scheduler.main.switch()

        def test():
            self.recursion_depth_in_test = scheduler.current.recursion_depth

        tlet = scheduler.tasklet(tasklet_outer)()
        self.assertEqual(tlet.recursion_depth, 0)
        tlet.run()
        self.assertEqual(tlet.recursion_depth, 2)
        tlet.bind(test, ())
        self.assertEqual(tlet.recursion_depth, 0)
        tlet.run()
        self.assertEqual(tlet.recursion_depth, 0)
        self.assertEqual(self.recursion_depth_in_test, 1)

class TestSwitch(SchedulerTestCase):
    """Test the new tasklet.switch() method, which allows
    explicit switching
    """

    def setUp(self):
        super(TestSwitch, self).setUp()
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


class TestModule(SchedulerTestCase):

    def test_get_debug(self):
        self.assertIn(scheduler.getdebug(), [True, False])

    def test_debug(self):
        self.assertIn(scheduler.debug, [True, False])

    def test_get_uncollectables(self):
        self.assertEqual(type(scheduler.getuncollectables()), list)

    def test_uncollectables(self):
        self.assertEqual(type(scheduler.uncollectables), list)

    def test_get_threads(self):
        self.assertEqual(type(scheduler.getthreads()), list)

    def test_threads(self):
        self.assertEqual(type(scheduler.threads), list)


class TestCstate(SchedulerTestCase):
    def test_cstate(self):
        self.assertIsInstance(scheduler.main.cstate, scheduler.cstack)

    def test_str_size(self):
        c = scheduler.main.cstate
        s = str(c)
        self.assertEqual(len(s), c.size * struct.calcsize("P"))

    def test_nesting_level(self):
        c = scheduler.main.cstate
        l1 = c.nesting_level
        self.assertIsInstance(l1, int)

    def test_chain(self):
        # create at least one additional C-stack
        t = scheduler.tasklet(apply_not_stackless)(scheduler.main.switch, )
        t.run()
        self.addCleanup(t.run)
        start = scheduler.main.cstate
        c = start.next
        self.assertIsNot(c, start)
        while(c is not start):
            self.assertIsInstance(c, scheduler.cstack)
            self.assertIs(c.prev.next, c)
            c = c.next


class TestTaskletFinalizer(SchedulerTestCase):
    def test_zombie(self):
        loop = True

        def task():
            while loop:
                try:
                    scheduler.schedule_remove()
                except TaskletExit:
                    pass
        t = scheduler.tasklet(apply_not_stackless)(task, )
        t.run()
        self.assertTrue(t.paused)
        t.__del__()
        self.assertTrue(is_zombie(t))
        self.assertIn(t, gc.garbage)
        gc.garbage.remove(t)
        # clean up
        loop = False
        t.kill()


class TestTaskletContext(AsTaskletTestCase):
    cvar = contextvars.ContextVar('TestTaskletContext', default='unset')

    def test_contexct_id(self):
        ct = scheduler.current

        # prepare a context
        sentinel = object()
        self.cvar.set(sentinel)  # make sure, a context is active
        self.assertIsInstance(ct.context_id, int)

        # no context
        t = scheduler.tasklet()  # new tasklet without context
        self.assertEqual(t.context_run(self.cvar.get), "unset")

        # known context
        ctx = contextvars.Context()
        self.assertNotEqual(id(ctx), ct.context_id)
        self.assertIs(ctx.run(scheduler.getcurrent), ct)
        cid = ctx.run(getattr, ct, "context_id")
        self.assertEqual(id(ctx), cid)

    def test_set_context_same_thread_not_current(self):
        # same thread, tasklet is not current

        # prepare a context
        ctx = contextvars.Context()
        sentinel = object()
        ctx.run(self.cvar.set, sentinel)

        tasklet_started = False
        t = scheduler.tasklet()
        def task():
            nonlocal tasklet_started
            self.assertEqual(t.context_id, id(ctx))
            tasklet_started = True

        t.bind(task)()

        # set the context
        t.set_context(ctx)

        # validate the context
        scheduler.run()
        self.assertTrue(tasklet_started)

    def test_set_context_same_thread_not_current_entered(self):
        # prepare a context
        sentinel = object()
        ctx = contextvars.Context()
        ctx.run(self.cvar.set, sentinel)
        tasklet_started = False

        t = scheduler.tasklet()
        def task():
            nonlocal tasklet_started
            self.assertEqual(t.context_id, id(ctx))
            tasklet_started = True
            scheduler.schedule_remove()
            self.assertEqual(t.context_id, id(ctx))

        t.bind(ctx.run)(task)
        scheduler.run()
        self.assertTrue(tasklet_started)
        self.assertTrue(t.alive)
        self.assertIsNot(t, scheduler.current)
        self.assertRaisesRegex(RuntimeError, "the current context of the tasklet has been entered",
                               t.set_context, contextvars.Context())
        t.insert()
        scheduler.run()

    def test_set_context_same_thread_current(self):
        # same thread, current tasklet

        # make sure a context exists
        sentinel = object()
        self.cvar.set(sentinel)
        self.assertIs(self.cvar.get(), sentinel)
        self.assertIsInstance(scheduler.current.context_id, int)

        # prepare another context
        sentinel2 = object()
        ctx = contextvars.Context()
        ctx.run(self.cvar.set, sentinel2)
        self.assertNotEqual(scheduler.current.context_id, id(ctx))

        # change the context of the current tasklet
        scheduler.current.set_context(ctx)

        # check that the new context is the context of the current tasklet
        self.assertEqual(scheduler.current.context_id, id(ctx))
        self.assertIs(self.cvar.get(), sentinel2)

    def test_set_context_same_thread_current_entered(self):
        # entered current context on the same thread
        sentinel = object()
        self.cvar.set(sentinel)
        cid = scheduler.current.context_id
        self.assertRaisesRegex(RuntimeError, "the current context of the tasklet has been entered",
                               contextvars.Context().run, scheduler.current.set_context, contextvars.Context())
        self.assertEqual(scheduler.current.context_id, cid)
        self.assertIs(self.cvar.get(), sentinel)

    def test_set_context_other_thread_paused(self):
        # other thread, tasklet is not current
        # prepare a context
        ctx = contextvars.Context()
        sentinel = object()
        ctx.run(self.cvar.set, sentinel)

        tasklet_started = False
        t = scheduler.tasklet()
        def task():
            nonlocal tasklet_started
            self.assertEqual(t.context_id, id(ctx))
            tasklet_started = True

        t.bind(task, ())  # paused

        # set the context
        thr = threading.Thread(target=t.set_context, args=(ctx,), name="other thread")
        thr.start()
        thr.join()

        # validate the context
        t.insert()
        scheduler.run()
        self.assertTrue(tasklet_started)

    def test_set_context_other_thread_scheduled(self):
        # other thread, tasklet is not current
        # prepare a context
        ctx = contextvars.Context()
        sentinel = object()
        ctx.run(self.cvar.set, sentinel)

        tasklet_started = False
        t = scheduler.tasklet()
        def task():
            nonlocal tasklet_started
            self.assertEqual(t.context_id, id(ctx))
            tasklet_started = True

        t.bind(task)()  # scheduled
        t.set_context(ctx)

        # set the context
        got_exception = None
        def other_thread():
            nonlocal got_exception
            try:
                t.set_context(contextvars.Context())
            except RuntimeError as e:
                got_exception = e
        thr = threading.Thread(target=other_thread, name="other thread")
        thr.start()
        thr.join()

        # validate the result
        self.assertIsInstance(got_exception, RuntimeError)
        with self.assertRaisesRegex(RuntimeError, "tasklet belongs to a different thread"):
            raise got_exception

        scheduler.run()
        self.assertTrue(tasklet_started)

    def test_set_context_other_thread_not_current_entered(self):
        # prepare a context
        sentinel = object()
        ctx = contextvars.Context()
        ctx.run(self.cvar.set, sentinel)
        tasklet_started = False

        t = scheduler.tasklet()
        def task():
            nonlocal tasklet_started
            self.assertEqual(t.context_id, id(ctx))
            tasklet_started = True
            scheduler.schedule_remove()
            self.assertEqual(t.context_id, id(ctx))

        t.bind(ctx.run)(task)
        scheduler.run()
        self.assertTrue(tasklet_started)
        self.assertTrue(t.alive)
        self.assertIsNot(t, scheduler.current)

        # set the context
        got_exception = None
        def other_thread():
            nonlocal got_exception
            try:
                t.set_context(contextvars.Context())
            except RuntimeError as e:
                got_exception = e
        thr = threading.Thread(target=other_thread, name="other thread")
        thr.start()
        thr.join()

        # validate the result
        self.assertIsInstance(got_exception, RuntimeError)
        with self.assertRaisesRegex(RuntimeError, "the current context of the tasklet has been entered"):
            raise got_exception
        t.insert()
        scheduler.run()

    def test_set_context_other_thread_current(self):
        # other thread, current tasklet

        # make sure a context exists
        sentinel = object()
        self.cvar.set(sentinel)
        self.assertIs(self.cvar.get(), sentinel)
        cid = scheduler.current.context_id

        t = scheduler.current
        # set the context
        got_exception = None
        def other_thread():
            nonlocal got_exception
            try:
                t.set_context(contextvars.Context())
            except RuntimeError as e:
                got_exception = e

        thr = threading.Thread(target=other_thread, name="other thread")
        thr.start()
        thr.join()

        self.assertIsInstance(got_exception, RuntimeError)
        with self.assertRaisesRegex(RuntimeError, "tasklet belongs to a different thread"):
            raise got_exception
        self.assertEqual(scheduler.current.context_id, cid)

    def test_set_context_other_thread_current_entered(self):
        # entered current context on other thread

        t = scheduler.current
        # set the context
        got_exception = None
        def other_thread():
            nonlocal got_exception
            try:
                t.set_context(contextvars.Context())
            except RuntimeError as e:
                got_exception = e

        thr = threading.Thread(target=other_thread, name="other thread")

        # prepare a context
        sentinel = object()
        ctx = contextvars.Context()
        ctx.run(self.cvar.set, sentinel)

        def in_ctx():
            self.assertEqual(scheduler.current.context_id, id(ctx))
            thr.start()
            thr.join()
            self.assertEqual(scheduler.current.context_id, id(ctx))

        ctx.run(in_ctx)
        self.assertIsInstance(got_exception, RuntimeError)
        with self.assertRaisesRegex(RuntimeError, "tasklet belongs to a different thread"):
            raise got_exception


    def test_context_init_null_main(self):
        # test the set_context in tasklet.bind, if the current context is NULL in main tasklet

        cid = None
        t = scheduler.tasklet()

        def other_thread():
            nonlocal cid
            self.assertIs(scheduler.current, scheduler.main)

            # this creates a context, because all tasklets initialized by the main-tasklet
            # shall share a common context
            t.bind(lambda: None)
            cid = scheduler.current.context_id

        thr = threading.Thread(target=other_thread, name="other thread")
        thr.start()
        thr.join()
        self.assertIsInstance(cid, int)
        self.assertEqual(t.context_id, cid)

    def test_context_init_main(self):
        # test the set_context in tasklet.bind, in main tasklet

        sentinel = object()
        cid = None
        t = scheduler.tasklet()

        def other_thread():
            nonlocal cid
            self.cvar.set(sentinel)
            self.assertIs(scheduler.current, scheduler.main)
            cid = scheduler.current.context_id
            self.assertIsInstance(cid, int)
            t.bind(lambda: None)
            self.assertEqual(scheduler.current.context_id, cid)

        thr = threading.Thread(target=other_thread, name="other thread")
        thr.start()
        thr.join()
        self.assertEqual(t.context_id, cid)

    def test_context_init_nonmain(self):
        # test the set_context in tasklet.bind
        # make sure a context exists
        sentinel = object()
        self.cvar.set(sentinel)
        self.assertIs(self.cvar.get(), sentinel)

        cid = scheduler.current.context_id
        t = scheduler.tasklet()

        # this creates
        t.bind(lambda: None)

        self.assertEqual(scheduler.current.context_id, cid)
        self.assertEqual(t.context_id, cid)

    @staticmethod
    def _test_context_setstate_alive_task():
        scheduler.schedule_remove(100)
        return 200

    def test_context_setstate_alive(self):
        # prepare a state of a half executed tasklet
        t = scheduler.tasklet(self._test_context_setstate_alive_task)()
        scheduler.run()
        self.assertEqual(t.tempval, 100)
        self.assertTrue(t.paused)

        state = t.__reduce__()[2]
        for i, fw in enumerate(state[3]):
            frame_factory, frame_args, frame_state = fw.__reduce__()
            state[3][i] = frame_factory(*frame_args)
            state[3][i].__setstate__(frame_state)
        # from pprint import pprint ; pprint(state)

        # make sure a context exists
        sentinel = object()
        self.cvar.set(sentinel)
        self.assertIs(self.cvar.get(), sentinel)

        cid = scheduler.current.context_id
        t = scheduler.tasklet()

        # this creates
        t.__setstate__(state)

        self.assertTrue(t.alive)
        self.assertEqual(scheduler.current.context_id, cid)
        self.assertEqual(t.context_id, cid)
        t.bind(None)

    def test_context_setstate_notalive(self):
        # prepare a state of a new tasklet
        state = scheduler.tasklet().__reduce__()[2]
        self.assertEqual(state[3], [])  # no frames

        # make sure a context exists
        sentinel = object()
        self.cvar.set(sentinel)
        self.assertIs(self.cvar.get(), sentinel)

        cid = scheduler.current.context_id
        t = scheduler.tasklet()

        # this creates
        t.__setstate__(state)

        self.assertFalse(t.alive)
        self.assertEqual(scheduler.current.context_id, cid)
        self.assertEqual(t.context_run(self.cvar.get), "unset")
        t.bind(None)

    def test_context_run_no_context(self):
        # make sure a context exists
        sentinel = object()
        self.cvar.set(sentinel)
        self.assertIs(self.cvar.get(), sentinel)
        cid0 = scheduler.current.context_id

        t = scheduler.tasklet()
        def get_cid():
            self.assertEqual(self.cvar.get(), "unset")
            return scheduler.current.context_id

        cid = t.context_run(get_cid)

        self.assertEqual(scheduler.current.context_id, cid0)
        self.assertIsInstance(cid, int)
        self.assertNotEqual(cid0, cid)
        self.assertEqual(t.context_id, cid)

    def test_context_run(self):
        # make sure a context exists
        sentinel = object()
        self.cvar.set(sentinel)
        self.assertIs(self.cvar.get(), sentinel)
        cid0 = scheduler.current.context_id

        t = scheduler.tasklet()
        ctx = contextvars.Context()
        t.set_context(ctx)

        def get_cid():
            self.assertEqual(self.cvar.get(), "unset")
            return scheduler.current.context_id

        cid = t.context_run(get_cid)

        self.assertEqual(scheduler.current.context_id, cid0)
        self.assertIsInstance(cid, int)
        self.assertNotEqual(cid0, cid)
        self.assertEqual(id(ctx), cid)

    def test_main_tasklet_init(self):
        # This test succeeds, if Stackless copies ts->context to into the main
        # tasklet, when Stackless creates the main tasklet.
        # This is important, if there is already a context set, when the interpreter
        # gets called. Example: an interactive python prompt.
        # See also: test_main_tasklet_fini
        ctx_holder1 = None  # use a tasklet to keep the context alive
        ctx_holder2 = None
        def task():
            nonlocal ctx_holder2
            ctx_holder2 = scheduler.main.context_run(scheduler.tasklet, id)
            self.assertEqual(ctx_holder2.context_id, scheduler.main.context_id)

        t = scheduler.tasklet(task, ())
        def other_thread():
            nonlocal ctx_holder1
            t.bind_thread()
            t.insert()
            ctx_holder1 = scheduler.tasklet(id)
            self.assertEqual(ctx_holder1.context_id, scheduler.current.context_id)
            scheduler._scheduler._test_outside()

        tr = threading.Thread(target=other_thread, name="other thread")
        tr.start()
        tr.join()
        self.assertIsNot(ctx_holder1, ctx_holder2)
        self.assertEqual(ctx_holder1.context_id, ctx_holder2.context_id)

    def test_main_tasklet_fini(self):
        # for a main tasklet of a thread initially ts->context == NULL
        # This test succeeds, if Stackless copies the context of the main
        # tasklet to ts->context after the main tasklet exits
        # This way the last context of the main tasklet is preserved and available
        # on the next invocation of the interpreter.
        ctx_holder1 = None  # use a tasklet to keep the context alive
        ctx_holder2 = None
        def task():
            nonlocal ctx_holder1
            ctx_holder1 = scheduler.main.context_run(scheduler.tasklet, id)
            self.assertEqual(ctx_holder1.context_id, scheduler.main.context_id)

        t = scheduler.tasklet(task, ())
        def other_thread():
            nonlocal ctx_holder2
            t.bind_thread()
            t.insert()
            scheduler._scheduler._test_outside()
            ctx_holder2 = scheduler.tasklet(id)
            self.assertEqual(ctx_holder2.context_id, scheduler.current.context_id)

        tr = threading.Thread(target=other_thread, name="other thread")
        tr.start()
        tr.join()
        self.assertIsNot(ctx_holder1, ctx_holder2)
        self.assertEqual(ctx_holder1.context_id, ctx_holder2.context_id)


#///////////////////////////////////////////////////////////////////////////////

if __name__ == '__main__':
    if not sys.argv[1:]:
        sys.argv.append('-v')

    unittest.main()
