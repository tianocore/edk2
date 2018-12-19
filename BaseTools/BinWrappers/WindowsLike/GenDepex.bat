@setlocal
@set ToolName=%~n0%
@%PYTHON% %BASE_TOOLS_PATH%\Source\Python\AutoGen\%ToolName%.py %*
