@echo off
REM Prepare the Python sources for EDK II by copying
REM the .h files from the PyMod tree into the Python tree.
REM Directory correspondence is maintained.

FOR %%d IN (Include Modules Objects Python) DO (
  echo.
  echo Processing the %%d directory.
  XCOPY /S /Y /Q PyMod-2.7.10\%%d\*.h %%d
)

echo.
echo DONE!
