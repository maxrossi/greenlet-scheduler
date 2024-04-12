
#include "StdAfx.h"
#include <Python.h>
#include <Scheduler.h>

#include "InterpreterWithSchedulerModule.h"

struct SchedulerCapi : public InterpreterWithSchedulerModule{};

TEST_F( SchedulerCapi, GetRunCount )
{
	int run_count = SchedulerAPI()->PyScheduler_GetRunCount();

	EXPECT_EQ( run_count, 1 );

	// TODO: WIP requires more scenarios
}
