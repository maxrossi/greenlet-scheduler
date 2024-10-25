import unittest
import sys
import gc


import scheduler


class SchedulerTestCaseBase(unittest.TestCase):

    def setUp(self):
        # Store reference to schedule manager to keep it alive during test
        self.scheduleManager = scheduler.get_schedule_manager()

        #Sanity check
        self.assertEqual(sys.getrefcount(self.scheduleManager), 3)

    def tearDown(self):

        # Clean up left over channels that still have blocked tasklets
        scheduler.run()
        
        scheduler.unblock_all_channels()
        
        gc.collect()

        self.assertEqual(scheduler.get_number_of_active_channels(),0)

        # Finish any remaining tasklets
        scheduler.run()

        # Ensure garbage collector has run
        gc.collect()

        # Run possible garbage collection tasklets
        scheduler.run()
        
        # Check references in schedule manager are all cleaned up and ready for removal
        self.assertEqual(sys.getrefcount(self.scheduleManager), 3)

        # remove reference to schedule manager, it should then die
        # TODO if it doesn't then this should fail test as it implies mem leak
        self.scheduleManager = None

        # Ensure garbage collector has run and collected last schedule manager
        gc.collect()

        # 1 should remain
        self.assertEqual(scheduler.get_number_of_active_schedule_managers(), 1)

        # Return nested tasklet usage to default ON
        scheduler.set_use_nested_tasklets(True)

    # Method to wrap getruncount with an added check that running total matches calculated value
    def getruncount(self):
        runCount = scheduler.getruncount()
        
        #Ensure that running value matches calculated value
        self.assertEqual(runCount, scheduler.calculateruncount())

        return runCount


# Scheduling options
class TestWithWatchdog(object):
    def run_scheduler(self):
        while self.getruncount() > 1:
            scheduler.run_n_tasklets(1)

class TestWithoutWatchdog(object):
    def run_scheduler(self):
        scheduler.run()

class TestNoNestedTasklets(object):

    def setUp(self):
        super().setUp()
        scheduler.set_use_nested_tasklets(False)

    def tearDown(self):
        super().tearDown()
        scheduler.set_use_nested_tasklets(True)

# End of options