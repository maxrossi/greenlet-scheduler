import unittest
import sys
import os
import gc
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



class SchedulerTestCaseBase(unittest.TestCase):

    def setUp(self):
        # Store reference to schedule manager to keep it alive during test
        self.scheduleManager = scheduler.get_schedule_manager()

        #Sanity check
        self.assertEqual(sys.getrefcount(self.scheduleManager), 2)

    def tearDown(self):
        # Finish any remaining tasklets
        scheduler.run()

        # Tests should clean up after themselves
        self.assertEqual(self.getruncount(), 1)

        # Ensure garbage collector has run
        gc.collect()
        
        # Check references in schedule manager are all cleaned up and ready for removal
        self.assertEqual(sys.getrefcount(self.scheduleManager), 2)

        # remove reference to schedule manager, it should then die
        # TODO if it doesn't then this should fail test as it implies mem leak
        self.scheduleManager = None

        # Ensure garbage collector has run and collected last schedule manager
        gc.collect()

        # None should then remain
        self.assertEqual(scheduler.get_number_of_active_schedule_managers(), 0)

    # Method to wrap getruncount with an added check that running total matches calculated value
    def getruncount(self):
        runCount = scheduler.getruncount()
        
        #Ensure that running value matches calculated value
        self.assertEqual(runCount, scheduler.calculateruncount())

        return runCount
