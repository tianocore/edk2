@setlocal
@set ToolName=%~n0%
@%PYTHON_HOME%\python.exe %BASE_TOOLS_PATH%\Source\Python\%ToolName%\%ToolName%.py %*
