@REM @file
@REM   This stand-alone program is typically called by the edksetup.bat file,
@REM   however it may be executed directly from the BaseTools project folder
@REM   if the file is not executed within a WORKSPACE\BaseTools folder.
@REM
@REM Copyright (c) 2006 - 2018, Intel Corporation. All rights reserved.<BR>
@REM (C) Copyright 2016 Hewlett Packard Enterprise Development LP<BR>
@REM
@REM SPDX-License-Identifier: BSD-2-Clause-Patent
@REM

@echo off
pushd .

@REM ##############################################################
@REM # You should not have to modify anything below this line
@REM #

if /I "%1"=="-h" goto Usage
if /I "%1"=="-help" goto Usage
if /I "%1"=="--help" goto Usage
if /I "%1"=="/h" goto Usage
if /I "%1"=="/help" goto Usage
if /I "%1"=="/?" goto Usage


:loop
  if "%1"=="" goto setup_workspace
  if /I "%1"=="--nt32" (
    if /I "%2" == "X64" (
      shift
    )
    @REM Ignore --nt32 flag
    shift
    goto loop
  )
  if /I "%1"=="Reconfig" (
    shift
    set RECONFIG=TRUE
    goto loop
  )
  if /I "%1"=="Rebuild" (
    shift
    set REBUILD=TRUE
    goto loop
  )
  if /I "%1"=="ForceRebuild" (
    shift
    set FORCE_REBUILD=TRUE
    goto loop
  )
  if "%1"=="" goto setup_workspace
  if exist %1 (
    if not defined BASE_TOOLS_PATH (
      if exist %1\Source set BASE_TOOLS_PATH=%1
      shift
      goto loop
    )
    if not defined EDK_TOOLS_PATH (
      if exist %1\Bin\Win32 set EDK_TOOLS_PATH=%1
      shift
      goto loop
    )
    echo.
    echo !!! ERROR !!! Unknown argument, %1 !!!
    echo.
    goto end
  ) else (
    echo.
    echo !!! ERROR !!! Unknown argument, %1 !!!
    echo.
    goto end
  )
  goto loop


@REM
@REM Check the required system environment variables
@REM

:setup_workspace
  REM
  REM check the EDK_TOOLS_PATH
  REM
  if not defined EDK_TOOLS_PATH goto no_EDK_TOOLS_PATH
  if exist %EDK_TOOLS_PATH% goto set_PATH

:no_EDK_TOOLS_PATH
  if not defined WORKSPACE (
    if defined BASE_TOOLS_PATH (
      set EDK_TOOLS_PATH=%BASE_TOOLS_PATH%
      goto set_PATH
    ) else (
      echo.
      echo !!! ERROR !!! Neither BASE_TOOLS_PATH nor EDK_TOOLS_PATH are set. !!!
      echo.
      goto end
    )
  ) else (
    if exist %WORKSPACE%\BaseTools\Bin (
      set EDK_TOOLS_PATH=%WORKSPACE%\BaseTools
      goto set_PATH
    ) else (
      echo.
      echo !!! ERROR !!! No tools path available. Please set EDK_TOOLS_PATH !!!
      echo.
      goto end
    )
  )

:set_PATH
  if defined WORKSPACE_TOOLS_PATH goto check_PATH
  if not defined EDK_TOOLS_BIN (
    set EDK_TOOLS_BIN=%EDK_TOOLS_PATH%\Bin\Win32
    if not exist %EDK_TOOLS_PATH%\Bin\Win32 (
      echo.
      echo !!! ERROR !!! Cannot find BaseTools Bin Win32!!!
      echo Please check the directory %EDK_TOOLS_PATH%\Bin\Win32
      echo Or configure EDK_TOOLS_BIN env to point Win32 directory.
      echo.
    )
  )
  set PATH=%EDK_TOOLS_BIN%;%PATH%
  set WORKSPACE_TOOLS_PATH=%EDK_TOOLS_PATH%
  goto PATH_ok

:check_PATH
  if "%EDK_TOOLS_PATH%"=="%WORKSPACE_TOOLS_PATH%" goto PATH_ok
  if not defined EDK_TOOLS_BIN (
    set EDK_TOOLS_BIN=%EDK_TOOLS_PATH%\Bin\Win32
    if not exist %EDK_TOOLS_PATH%\Bin\Win32 (
      echo.
      echo !!! ERROR !!! Cannot find BaseTools Bin Win32!!!
      echo Please check the directory %EDK_TOOLS_PATH%\Bin\Win32
      echo Or configure EDK_TOOLS_BIN env to point Win32 directory.
      echo.
    )
  )
  set PATH=%EDK_TOOLS_BIN%;%PATH%
  set WORKSPACE_TOOLS_PATH=%EDK_TOOLS_PATH%
  echo Resetting the PATH variable to include the EDK_TOOLS_PATH for this session.

:PATH_ok
REM
REM copy *.template to %CONF_PATH%
REM
if not defined WORKSPACE (
   if defined RECONFIG (
     echo.
     echo !!! WARNING !!! WORKSPACE environment variable was not set, cannot Reconfig !!!
     echo.
   )
   goto skip_reconfig
)

IF NOT exist "%EDK_TOOLS_PATH%\set_vsprefix_envs.bat" (
  @echo.
  @echo !!! ERROR !!! The set_vsprefix_envs.bat was not found !!!
  @echo.
  goto end
)
call %EDK_TOOLS_PATH%\set_vsprefix_envs.bat

if not defined CONF_PATH (
  set CONF_PATH=%WORKSPACE%\Conf
)

if NOT exist %CONF_PATH% (
  if defined PACKAGES_PATH (
    for %%i IN (%PACKAGES_PATH%) DO (
      if exist %%~fi\Conf (
        set CONF_PATH=%%i\Conf
        goto CopyConf
      )
    )
  )
)

:CopyConf
if NOT exist %CONF_PATH% (
  mkdir %CONF_PATH%
) else (
  if defined RECONFIG (
    echo.
    echo  Over-writing the files in the CONF_PATH directory
    echo  using the default template files
    echo.
  )
)

if NOT exist %CONF_PATH%\target.txt (
  echo copying ... target.template to %CONF_PATH%\target.txt
  if NOT exist %EDK_TOOLS_PATH%\Conf\target.template (
    echo Error: target.template is missing at folder %EDK_TOOLS_PATH%\Conf\
  )
  copy %EDK_TOOLS_PATH%\Conf\target.template %CONF_PATH%\target.txt > nul
) else (
  if defined RECONFIG echo over-write ... target.template to %CONF_PATH%\target.txt
  if defined RECONFIG copy /Y %EDK_TOOLS_PATH%\Conf\target.template %CONF_PATH%\target.txt > nul
)

if NOT exist %CONF_PATH%\tools_def.txt (
  echo copying ... tools_def.template to %CONF_PATH%\tools_def.txt
  if NOT exist %EDK_TOOLS_PATH%\Conf\tools_def.template (
    echo Error: tools_def.template is missing at folder %EDK_TOOLS_PATH%\Conf\
  )
  copy %EDK_TOOLS_PATH%\Conf\tools_def.template %CONF_PATH%\tools_def.txt > nul
) else (
  if defined RECONFIG echo over-write ... tools_def.template to %CONF_PATH%\tools_def.txt
  if defined RECONFIG copy /Y %EDK_TOOLS_PATH%\Conf\tools_def.template %CONF_PATH%\tools_def.txt > nul
)

if NOT exist %CONF_PATH%\build_rule.txt (
  echo copying ... build_rule.template to %CONF_PATH%\build_rule.txt
  if NOT exist %EDK_TOOLS_PATH%\Conf\build_rule.template (
    echo Error: build_rule.template is missing at folder %EDK_TOOLS_PATH%\Conf\
  )
  copy %EDK_TOOLS_PATH%\Conf\build_rule.template %CONF_PATH%\build_rule.txt > nul
) else (
  if defined RECONFIG echo over-write ... build_rule.template to %CONF_PATH%\build_rule.txt
  if defined RECONFIG copy /Y %EDK_TOOLS_PATH%\Conf\build_rule.template %CONF_PATH%\build_rule.txt > nul
)

echo           PATH      = %PATH%
echo.
if defined WORKSPACE (
  echo      WORKSPACE      = %WORKSPACE%
)
if defined PACKAGES_PATH (
  echo  PACKAGES_PATH      = %PACKAGES_PATH%
)
echo EDK_TOOLS_PATH      = %EDK_TOOLS_PATH%
if defined BASE_TOOLS_PATH (
  echo BASE_TOOLS_PATH     = %BASE_TOOLS_PATH%
)
if defined EDK_TOOLS_BIN (
  echo  EDK_TOOLS_BIN      = %EDK_TOOLS_BIN%
)
echo      CONF_PATH      = %CONF_PATH%
echo.

:skip_reconfig

@REM
@REM Test if we are going to have to do a build
@REM
if defined FORCE_REBUILD goto check_build_environment
if defined REBUILD goto check_build_environment
if not exist "%EDK_TOOLS_PATH%" goto check_build_environment
if not exist "%EDK_TOOLS_BIN%"  goto check_build_environment

IF NOT EXIST "%EDK_TOOLS_BIN%\EfiRom.exe" goto check_c_tools
IF NOT EXIST "%EDK_TOOLS_BIN%\GenFfs.exe" goto check_c_tools
IF NOT EXIST "%EDK_TOOLS_BIN%\GenFv.exe" goto check_c_tools
IF NOT EXIST "%EDK_TOOLS_BIN%\GenFw.exe" goto check_c_tools
IF NOT EXIST "%EDK_TOOLS_BIN%\GenSec.exe" goto check_c_tools
IF NOT EXIST "%EDK_TOOLS_BIN%\Split.exe" goto check_c_tools
IF NOT EXIST "%EDK_TOOLS_BIN%\TianoCompress.exe" goto check_c_tools
IF NOT EXIST "%EDK_TOOLS_BIN%\VfrCompile.exe" goto check_c_tools
IF NOT EXIST "%EDK_TOOLS_BIN%\VolInfo.exe" goto check_c_tools

goto check_build_environment

:check_c_tools
  echo.
  echo !!! ERROR !!! Binary C tools are missing. They are required to be built from BaseTools Source.
  echo.

:check_build_environment
  set PYTHONHASHSEED=1

  if not defined BASE_TOOLS_PATH (
     if not exist "Source\C\Makefile" (
       if not exist "%EDK_TOOLS_PATH%\Source\C\Makefile" goto no_source_files
       set BASE_TOOLS_PATH=%EDK_TOOLS_PATH%
     ) else (
       set BASE_TOOLS_PATH=%CD%
     )
  )

:defined_python
if defined PYTHON_COMMAND if not defined PYTHON3_ENABLE (
  goto check_python_available
)
if defined PYTHON3_ENABLE (
  if "%PYTHON3_ENABLE%" EQU "TRUE" (
    set PYTHON_COMMAND=py -3
    goto check_python_available
  ) else (
    goto check_python2
  )
)
if not defined PYTHON_COMMAND if not defined PYTHON3_ENABLE (
  set PYTHON_COMMAND=py -3
  py -3 %BASE_TOOLS_PATH%\Tests\PythonTest.py >PythonCheck.txt 2>&1
  setlocal enabledelayedexpansion
  set /p PythonCheck=<"PythonCheck.txt"
  del PythonCheck.txt
  if "!PythonCheck!" NEQ "TRUE" (
    if not defined PYTHON_HOME if not defined PYTHONHOME (
      endlocal
      set PYTHON_COMMAND=
      echo.
      echo !!! ERROR !!! Binary python tools are missing.
      echo PYTHON_COMMAND, PYTHON3_ENABLE or PYTHON_HOME
      echo Environment variable is not set successfully.
      echo They is required to build or execute the python tools.
      echo.
      goto end
    ) else (
      goto check_python2
    )
  ) else (
    goto check_freezer_path
  )
)

:check_python2
endlocal
if defined PYTHON_HOME (
  if EXIST "%PYTHON_HOME%" (
    set PYTHON_COMMAND=%PYTHON_HOME%\python.exe
    goto check_python_available
  )
)
if defined PYTHONHOME (
  if EXIST "%PYTHONHOME%" (
    set PYTHON_HOME=%PYTHONHOME%
    set PYTHON_COMMAND=%PYTHON_HOME%\python.exe
    goto check_python_available
  )
)
echo.
echo !!! ERROR !!!  PYTHON_HOME is not defined or The value of this variable does not exist
echo.
goto end
:check_python_available
%PYTHON_COMMAND% %BASE_TOOLS_PATH%\Tests\PythonTest.py >PythonCheck.txt 2>&1
  setlocal enabledelayedexpansion
  set /p PythonCheck=<"PythonCheck.txt"
  del PythonCheck.txt
  if "!PythonCheck!" NEQ "TRUE" (
    echo.
    echo ! ERROR !  "%PYTHON_COMMAND%" is not installed or added to environment variables
    echo.
    goto end
  ) else (
    goto check_freezer_path
  )

:check_freezer_path
  endlocal
  if defined BASETOOLS_PYTHON_SOURCE goto print_python_info
  set "PATH=%BASE_TOOLS_PATH%\BinWrappers\WindowsLike;%PATH%"
  set BASETOOLS_PYTHON_SOURCE=%BASE_TOOLS_PATH%\Source\Python
  set PYTHONPATH=%BASETOOLS_PYTHON_SOURCE%;%PYTHONPATH%

:print_python_info
  echo                PATH = %PATH%
  if defined PYTHON3_ENABLE if "%PYTHON3_ENABLE%" EQU "TRUE" (
    echo      PYTHON3_ENABLE = %PYTHON3_ENABLE%
    echo             PYTHON3 = %PYTHON_COMMAND%
  ) else (
    echo      PYTHON3_ENABLE = FALSE
    echo      PYTHON_COMMAND = %PYTHON_COMMAND%
  )
  echo          PYTHONPATH = %PYTHONPATH%
  echo.

:VisualStudioAvailable
  if not defined FORCE_REBUILD (
    if not defined REBUILD (
      goto end
    )
  )
  call "%EDK_TOOLS_PATH%\get_vsvars.bat"
  if not defined VCINSTALLDIR (
    @echo.
    @echo !!! ERROR !!!! Cannot find Visual Studio, required to build C tools !!!
    @echo.
    goto end
  )
  if not defined FORCE_REBUILD goto IncrementalBuild

:CleanAndBuild
  pushd .
  cd %BASE_TOOLS_PATH%
  call nmake cleanall
  del /f /q %BASE_TOOLS_PATH%\Bin\Win32\*.*
  popd
  @REM Let CleanAndBuild fall through to IncrementalBuild


:IncrementalBuild
  pushd .
  cd %BASE_TOOLS_PATH%
  call nmake c
  popd
  goto end


:no_source_files
  echo.
  echo !!! ERROR !!! Cannot build BaseTools applications - no source directory located !!!
  echo.
  goto end

:Usage
  @echo.
  echo  Usage: "%0 [-h | -help | --help | /h | /help | /?] [ Rebuild | ForceRebuild ] [Reconfig] [base_tools_path [edk_tools_path]]"
  @echo.
  @echo         base_tools_path   BaseTools project path, BASE_TOOLS_PATH will be set to this path.
  @echo         edk_tools_path    EDK_TOOLS_PATH will be set to this path.
  @echo         Rebuild           If sources are available perform an Incremental build, only
  @echo                           build those updated tools.
  @echo         ForceRebuild      If sources are available, rebuild all tools regardless of
  @echo                           whether they have been updated or not.
  @echo         Reconfig          Reinstall target.txt, tools_def.txt and build_rule.txt.
  @echo.

:end
set REBUILD=
set FORCE_REBUILD=
set RECONFIG=
popd

