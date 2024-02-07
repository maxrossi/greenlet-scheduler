cmake_minimum_required(VERSION 3.16)
project(schedulertest)

find_package(Python REQUIRED NO_CMAKE_PATH)

set(SRC_FILES
    SchedulerTestModule.cpp
    SchedulerTestModule.h
    stdafx.cpp
)

set(PUBLIC_HEADER_FILES "")
ccp_add_library(schedulertest SHARED ${SRC_FILES})

target_include_directories(schedulertest
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../../src
)

target_link_libraries(schedulertest PRIVATE Python)

get_target_property(_SOURCES schedulertest SOURCES)
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}"
        PREFIX "Sources"
        FILES ${_SOURCES}
)

target_precompile_headers(schedulertest PRIVATE stdafx.h)

set_target_properties(schedulertest PROPERTIES OUTPUT_NAME "_schedulertest")

# Provide an install target iff this is the top level project only
if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    # Install rule to ensure that our runtime and linker files are in the expected, platform-specific folders
    install(
        TARGETS schedulertest
        EXPORT schedulertestTargets
        CONFIGURATIONS ${CMAKE_CONFIGURATION_TYPES}
        LIBRARY DESTINATION lib/${CCP_PLATFORM}/${CCP_ARCHITECTURE}/${CCP_TOOLSET}/
        ARCHIVE DESTINATION lib/${CCP_PLATFORM}/${CCP_ARCHITECTURE}/${CCP_TOOLSET}/
        RUNTIME DESTINATION bin/${CCP_PLATFORM}/${CCP_ARCHITECTURE}/${CCP_TOOLSET}/
    )
    # Install rule for available public headers
    install(FILES ${PUBLIC_HEADER_FILES} DESTINATION include)

    configure_ccp_vendor_config_file(
        TARGET schedulertest
        DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/schedulertestConfig.cmake"
        DEPENDENCIES ""
    )
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/schedulertestConfig.cmake DESTINATION .)

     # Install python package
     install(DIRECTORY python/ DESTINATION python
     FILES_MATCHING
     PATTERN "*.py"
)
endif()