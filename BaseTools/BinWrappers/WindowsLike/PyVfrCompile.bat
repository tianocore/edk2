@setlocal
@set ToolName=IfrCompiler
@set PYTHONPATH=%PYTHONPATH%;%BASE_TOOLS_PATH%\Source\Python;%BASE_TOOLS_PATH%\Source\Python\VfrCompiler
@%PYTHON_COMMAND% -m %ToolName% %*
