cmake_minimum_required(VERSION 3.16)
project(Scheduler)



if(NOT DEFINED ENV{CCP_EVE_PERFORCE_BRANCH_PATH})
    message(FATAL_ERROR "Missing required environment variable CCP_EVE_PERFORCE_BRANCH_PATH")
elseif(NOT IS_ABSOLUTE $ENV{CCP_EVE_PERFORCE_BRANCH_PATH})
    message(FATAL_ERROR "CCP_EVE_PERFORCE_BRANCH_PATH needs to be an absolute path")
elseif(NOT EXISTS $ENV{CCP_EVE_PERFORCE_BRANCH_PATH})
    message(FATAL_ERROR "CCP_EVE_PERFORCE_BRANCH_PATH is pointing at a non-existing location $ENV{CCP_EVE_PERFORCE_BRANCH_PATH}")
elseif(NOT IS_DIRECTORY $ENV{CCP_EVE_PERFORCE_BRANCH_PATH})
    message(FATAL_ERROR "CCP_EVE_PERFORCE_BRANCH_PATH needs to be a directory")
else()
    message(STATUS "Using CCP_EVE_PERFORCE_BRANCH_PATH located at $ENV{CCP_EVE_PERFORCE_BRANCH_PATH}")
endif()

file(TO_CMAKE_PATH "$ENV{CCP_EVE_PERFORCE_BRANCH_PATH}" BRANCH_ROOT_DIR)
list(INSERT CMAKE_MODULE_PATH 0 ${BRANCH_ROOT_DIR}/cmake)
include(CcpGlobalSettings)
include(CcpPackageConfigHelpers)


find_package(Python REQUIRED NO_CMAKE_PATH)
find_package(Greenlet REQUIRED NO_CMAKE_PATH)

get_target_property(GREENLET_INCLUDE_DIRECTORY
        Greenlet
        INTERFACE_INCLUDE_DIRECTORIES)

set(SRC_FILES
    include/Scheduler.h
    src/Scheduler.cpp
    src/PyTasklet_python.cpp
    src/PyTasklet.cpp
    src/PyTasklet.h
    src/PyChannel_python.cpp
    src/PyChannel.cpp
    src/PyChannel.h
    src/PyScheduler.cpp
    src/PyScheduler.h
    src/stdafx.cpp
)

set(PUBLIC_HEADER_FILES include/Scheduler.h)
ccp_add_library(Scheduler SHARED ${SRC_FILES})

target_include_directories(Scheduler PUBLIC ${CMAKE_CURRENT_LIST_DIR}/include)

target_link_libraries(Scheduler PRIVATE Python)

target_include_directories(Scheduler
        PRIVATE
        ${GREENLET_INCLUDE_DIRECTORY}
        )

get_target_property(_SOURCES Scheduler SOURCES)
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}"
        PREFIX "Sources"
        FILES ${_SOURCES}
)

target_precompile_headers(Scheduler PRIVATE src/stdafx.h)

set_target_properties(Scheduler PROPERTIES OUTPUT_NAME "_scheduler")


# Build test
add_subdirectory(tests/cpp)

# Provide an install target iff this is the top level project only
if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    enable_testing()
    # Install rule to ensure that our runtime and linker files are in the expected, platform-specific folders
    install(
        TARGETS Scheduler
        EXPORT SchedulerTargets
        CONFIGURATIONS ${CMAKE_CONFIGURATION_TYPES}
        LIBRARY DESTINATION lib/${CCP_PLATFORM}/${CCP_ARCHITECTURE}/${CCP_TOOLSET}/
        ARCHIVE DESTINATION lib/${CCP_PLATFORM}/${CCP_ARCHITECTURE}/${CCP_TOOLSET}/
        RUNTIME DESTINATION bin/${CCP_PLATFORM}/${CCP_ARCHITECTURE}/${CCP_TOOLSET}/
    )
    # Install rule for available public headers
    install(FILES ${PUBLIC_HEADER_FILES} DESTINATION include)

    configure_ccp_vendor_config_file(
        TARGET Scheduler
        DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/SchedulerConfig.cmake"
        DEPENDENCIES ""
    )
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/SchedulerConfig.cmake DESTINATION .)

     # Install python package
     install(DIRECTORY python/ DESTINATION python
     FILES_MATCHING
     PATTERN "*.py"
    )

    execute_process(COMMAND ${PYTHON_EXECUTABLE} discover.py
            OUTPUT_VARIABLE PYTHON_TESTS_STR
            OUTPUT_STRIP_TRAILING_WHITESPACE
            ERROR_STRIP_TRAILING_WHITESPACE
            RESULT_VARIABLE PYTHON_TEST_DISCOVERY_RETVAL
            ERROR_VARIABLE PYTHON_TEST_DISCOVERY_ERRORS
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/python/scheduler
    )

    get_filename_component(PYTHON_TEST_EXE_DIR ${PYTHON_EXECUTABLE} DIRECTORY)

    if(WIN32)
        SET(PYTHONPATH_ENV "${CMAKE_SOURCE_DIR}/python/\;$<TARGET_FILE_DIR:Scheduler>\;${BRANCH_ROOT_DIR}/carbon/common/stdlib/\;${PYTHON_TEST_EXE_DIR}\;${Greenlet_ROOT}/${CCP_VENDOR_BIN_PATH}\;${Greenlet_ROOT}/python")
    elseif(APPLE)
        SET(PYTHONPATH_ENV "${CMAKE_SOURCE_DIR}/python/:$<TARGET_FILE_DIR:Scheduler>:${BRANCH_ROOT_DIR}/carbon/common/stdlib/:${PYTHON_TEST_EXE_DIR}:${Greenlet_ROOT}/${CCP_VENDOR_BIN_PATH}:${Greenlet_ROOT}/python")
    endif()

    separate_arguments(PYTHON_TESTS UNIX_COMMAND ${PYTHON_TESTS_STR})

    foreach (PYTHON_TEST ${PYTHON_TESTS})
        message(STATUS "Found test ${PYTHON_TEST}")
        add_test(NAME ${PYTHON_TEST}
            COMMAND ${PYTHON_EXECUTABLE} -m unittest -v ${PYTHON_TEST}
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/python/scheduler/tests
        )
        set_tests_properties(${PYTHON_TEST} PROPERTIES ENVIRONMENT "PYTHONPATH=${PYTHONPATH_ENV};BUILDFLAVOR=$<LOWER_CASE:$<CONFIG>>")
    endforeach ()
endif()
