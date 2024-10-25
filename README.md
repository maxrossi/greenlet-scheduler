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

## Current requirements for documentation generation

1. Documentaion can be built on either Windows or macOS
2. Uses our custom PythonInterpreter
3. Built against the `MAINLINE` branch

## Building the documentation

Documentation is built using the following defaults:
- TeamCity build agents: ON
- Local dev build: OFF

To override the default documentation build settings, turn the CMake option `BUILD_DOCUMENTATION` to `ON/OFF`.

Building the `INSTALL` target will build all documentation and place it in the path specified by `CMAKE_INSTALL_PREFIX`.

Entry point for the documentation is `documentation/index.html`.

Documentation can be built using either .rst (restructuredText) or .md (markdown) files as sources.

# Contributing

Contribution follows the standard GIT PR model.

When altering Python or C-API exposure ensure that docstrings and c++ documentation blocks reflect changes.