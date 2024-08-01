::Create sphinx documentation
::REQUIRED ARGUMENTS
::path to doxygen xml ouput
::path to sphinx source folder
::path to sphinx build folder

python -m venv .venv
call .\.venv\Scripts\Activate.bat
pip install sphinx
pip install sphinx_rtd_theme
pip install breathe
python --version
sphinx-build -E -b html -Dbreathe_projects.doxygen=%1 -c %2 %3 %4 