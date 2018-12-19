@setlocal
@set ToolName=%~n0%
@set PYTHONPATH=%PYTHONPATH%;%BASE_TOOLS_PATH%\Source\Python
@%PYTHON% -m %ToolName%.EccMain %*
