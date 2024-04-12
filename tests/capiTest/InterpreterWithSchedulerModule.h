/* 
	*************************************************************************

	PyTasklet.h

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

struct InterpreterWithSchedulerModule : public ::testing::Test
{
	void SetUp();

	void TearDown();
};