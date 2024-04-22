/* 
	*************************************************************************

	InterpreterWithSchedulerModule.h

	Author:    James Hawk
	Created:   April. 2024
	Project:   SchedulerCapiTest

	Description:   

	  PyTaskletObject python type definition

	(c) CCP 2024

	*************************************************************************
*/
#pragma once


#include <gtest/gtest.h>
#include <Scheduler.h>

struct InterpreterWithSchedulerModule : public ::testing::Test
{
	void SetUp();

	void TearDown();

    PyObject* m_scheduler_module;

    PyObject* m_main_module;

    SchedulerCAPI* m_api;
};