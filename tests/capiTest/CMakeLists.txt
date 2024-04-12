cmake_minimum_required(VERSION 3.16)

project(SchedulerCapiTest)

find_package(Python REQUIRED NO_CMAKE_PATH)
find_package(GTest REQUIRED NO_CMAKE_PATH)
find_package(CcpCore REQUIRED NO_CMAKE_PATH)

include(GoogleTest)

set(SRC_FILES
    InterpreterWithSchedulerModule.cpp
    InterpreterWithSchedulerModule.h
    Scheduler.cpp
    StdAfx.h
    StdAfx.cpp
)

ccp_add_executable(SchedulerCapiTest ${SRC_FILES})

target_include_directories(SchedulerCapiTest PUBLIC ${CMAKE_CURRENT_LIST_DIR}/include)

target_precompile_headers(SchedulerCapiTest PRIVATE StdAfx.h)

target_link_libraries(SchedulerCapiTest PRIVATE GTest::GTest GTest::Main Python CcpCore Scheduler)

# The modules have been supplied separately as passing together as a PYTHONPATH encounters problems with separators
gtest_discover_tests(SchedulerCapiTest capiTests PROPERTIES ENVIRONMENT "SCHEDULER_MODULE_PATH=${CMAKE_SOURCE_DIR}/python/"
                                                            ENVIRONMENT "SCHEDULER_CEXTENSION_MODULE_PATH=$<TARGET_FILE_DIR:Scheduler>"
                                                            ENVIRONMENT "STDLIB_PATH=${BRANCH_ROOT_DIR}/carbon/common/stdlib/"
                                                            ENVIRONMENT "GREENLET_CEXTENSION_MODULE_PATH=${Greenlet_ROOT}/${CCP_VENDOR_BIN_PATH}"
                                                            ENVIRONMENT "GREENLET_MODULE_PATH=${Greenlet_ROOT}/python"
                                                            ENVIRONMENT "BUILDFLAVOR=$<LOWER_CASE:$<CONFIG>>")
