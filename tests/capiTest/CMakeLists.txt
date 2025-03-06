cmake_minimum_required(VERSION 3.31)

project(SchedulerCapiTest)

find_package(Python3 COMPONENTS Development REQUIRED)
find_package(GTest CONFIG REQUIRED)

include(GoogleTest)

set(SRC_FILES
    InterpreterWithSchedulerModule.cpp
    InterpreterWithSchedulerModule.h
    Scheduler.cpp
    Channel.cpp
    Tasklet.cpp
    StdAfx.h
    StdAfx.cpp
)

add_executable(SchedulerCapiTest ${SRC_FILES})

target_include_directories(SchedulerCapiTest PUBLIC ${CMAKE_CURRENT_LIST_DIR}/include)

target_include_directories(SchedulerCapiTest PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

target_precompile_headers(SchedulerCapiTest PRIVATE StdAfx.h)

target_link_libraries(SchedulerCapiTest PRIVATE GTest::gtest GTest::gtest_main Python3::Python Scheduler)

# Generate paths Include for each config
# Allows for easy use inside IDE

if(WIN32)
    set(PYTHON_STDLIB_PATH ${PYTHON_TOOL_ROOT_DIR}/Lib)
elseif(APPLE)
    set(PYTHON_STDLIB_PATH $<TARGET_FILE_DIR:Python3::Python>/python3.12)
endif()

file (GENERATE
    OUTPUT "PackagePaths_$<CONFIG>.h"
    CONTENT
"
#pragma once
#include <string>
std::wstring SCHEDULER_CEXTENSION_MODULE_PATH = L\"$<TARGET_FILE_DIR:Scheduler>\";
std::wstring SCHEDULER_PACKAGE_PATH = L\"${CMAKE_SOURCE_DIR}/python\";
std::wstring STDLIB_PATH = L\"${PYTHON_STDLIB_PATH}\";
std::wstring GREENLET_CEXTENSION_MODULE_PATH = L\"${GREENLET_IMPORTED_LOCATION_DIR}\";
std::wstring GREENLET_MODULE_PATH = L\"${GREENLET_IMPORTED_LOCATION_DIR}/python\";
"
)

# Copy the python dll to target build directory
if(WIN32)
    set(PythonLibName "python312.dll")
elseif(APPLE)
    set(PythonLibName "libpython3.12.dylib")
endif()


cmake_policy(SET CMP0175 OLD) # Allows us to use OUTPUT with PRE_BUILD in add_custom_command
add_custom_command (
        COMMAND ${CMAKE_COMMAND} "-E" "copy_if_different" "PackagePaths_$<CONFIG>.h" "PackagePaths.h"
        VERBATIM
        PRE_BUILD
        DEPENDS  "PackagePaths_$<CONFIG>.h"
        OUTPUT   "PackagePaths.h"
        COMMENT  "Creating PackagePaths.h file"
)

add_custom_target(GeneratePackagePathsHeader DEPENDS "PackagePaths.h")
add_dependencies(SchedulerCapiTest GeneratePackagePathsHeader)


if (APPLE)
    add_custom_command(
            TARGET SchedulerCapiTest POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy
            $<TARGET_FILE_DIR:Python3::Python>/${PythonLibName}
            $<TARGET_FILE_DIR:SchedulerCapiTest>)

    gtest_discover_tests(
            SchedulerCapiTest capiTests
            DISCOVERY_MODE PRE_TEST
            WORKING_DIRECTORY ${PYTHON_TOOL_ROOT_DIR}
    )

elseif (WIN32)
    add_custom_command(
            TARGET SchedulerCapiTest POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy
            ${PYTHON_TOOL_ROOT_DIR}/${PythonLibName}
            $<TARGET_FILE_DIR:SchedulerCapiTest>)

    gtest_discover_tests(
            SchedulerCapiTest capiTests
            DISCOVERY_MODE PRE_TEST
            WORKING_DIRECTORY $<TARGET_FILE_DIR:Python3::Python>
    )

endif ()



