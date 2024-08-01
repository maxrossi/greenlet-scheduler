# carbon-scheduler

Provides channels and a scheduler for Greenlet coroutines. 
Tasklet and channel scheduling order and behaviour has been designed to match that of Stackless Python as possible.

Only functionality required by Carbon has been implemented from the Stackless Python API.

# Building

Build using provided `CMakeLists` in the repositry root.

# Accessing the Documentation

Documentation building is currently WIP and experimental.

Documentation provides:
1. Generated Python API.
2. Generated C-API.
3. carbon-scheduler guides.
4. carbon-scheduler usage examples.

## Current requirements for WIP documentation generation

1. Windows
2. Python3.12 installed on system
3. Built against the `PYTHON3-WITH-GREENLET` branch

Requirements will change once direction is agreed.

## Building the documentation

To build the documentation turn the CMake option `BUILD_DOCUMENTATION` to `ON`.

Building the `INSTALL` target will build all documentation and place it in the path specified by `CMAKE_INSTALL_PREFIX`.

Entry point for the documentation is `documentation/index.html`.

# Contributing

Contribution follows the standard GIT PR model.

When altering Python or C-API exposure ensure that docstrings and c++ documentation blocks reflect changes.