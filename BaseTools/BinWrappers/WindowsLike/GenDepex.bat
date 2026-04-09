@setlocal
@set ToolName=%~n0%
@%PYTHON_COMMAND% %BASE_TOOLS_PATH%\Source\Python\AutoGen\%ToolName%.py %*
