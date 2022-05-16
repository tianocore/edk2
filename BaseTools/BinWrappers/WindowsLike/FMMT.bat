@setlocal
@set ToolName=%~n0%
@set PYTHONPATH=%PYTHONPATH%;%BASE_TOOLS_PATH%\Source\Python;%BASE_TOOLS_PATH%\Source\Python\FMMT
@%PYTHON_COMMAND% -m %ToolName%.%ToolName% %*
