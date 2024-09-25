::Create sphinx documentation
::REQUIRED ARGUMENTS
::path to doxygen xml ouput
::path to sphinx source folder
::path to sphinx build folder

::Pre-requirements:
::Make sure to set the following CMake option (else it won't generate documentation):
::-DBUILD_DOCUMENTATION=ON

:: Build it using "our pythonInterpreter":
"%CCP_EVE_PERFORCE_BRANCH_PATH%\eve\client\pythonInterpreter.bat" -m sphinx build -E -b html -Dbreathe_projects.doxygen=%1 -c %2 %3 %4
