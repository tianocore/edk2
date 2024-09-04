@REM @file
@REM   This stand-alone program is typically called by the edksetup.bat file,
@REM   however it may be executed directly from the BaseTools project folder
@REM   if the file is not executed within a WORKSPACE\BaseTools folder.
@REM
@REM Copyright (c) 2006 - 2019, Intel Corporation. All rights reserved.<BR>
@REM (C) Copyright 2016 Hewlett Packard Enterprise Development LP<BR>
@REM
@REM SPDX-License-Identifier: BSD-2-Clause-Patent
@REM

@echo off
pushd .
set SCRIPT_ERROR=0
set PYTHON_VER_MAJOR=3
set PYTHON_VER_MINOR=6

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
  if /I "%1"=="VS2022" (
    shift
    set VS2022=TRUE
    set VSTool=VS2022
    goto loop
  )
  if /I "%1"=="VS2019" (
    shift
    set VS2019=TRUE
    set VSTool=VS2019
    goto loop
  )
  if /I "%1"=="VS2017" (
    shift
    set VS2017=TRUE
    set VSTool=VS2017
    goto loop
  )
  if /I "%1"=="VS2015" (
    shift
    set VS2015=TRUE
    set VSTool=VS2015
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
if defined VS2022 (
  call %EDK_TOOLS_PATH%\set_vsprefix_envs.bat VS2022
) else if defined VS2019 (
  call %EDK_TOOLS_PATH%\set_vsprefix_envs.bat VS2019
) else if defined VS2017 (
  call %EDK_TOOLS_PATH%\set_vsprefix_envs.bat VS2017
) else if defined VS2015 (
  call %EDK_TOOLS_PATH%\set_vsprefix_envs.bat VS2015
  call %EDK_TOOLS_PATH%\get_vsvars.bat VS2015
) else (
  call %EDK_TOOLS_PATH%\set_vsprefix_envs.bat
  call %EDK_TOOLS_PATH%\get_vsvars.bat
)
if %SCRIPT_ERROR% NEQ 0 (
  @echo.
  @echo !!! ERROR !!! %VSTool% is not installed !!!
  @echo.
  goto end
)

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
    echo  Overwriting the files in the CONF_PATH directory
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
  if defined RECONFIG echo overwrite ... target.template to %CONF_PATH%\target.txt
  if defined RECONFIG copy /Y %EDK_TOOLS_PATH%\Conf\target.template %CONF_PATH%\target.txt > nul
)

if NOT exist %CONF_PATH%\tools_def.txt (
  echo copying ... tools_def.template to %CONF_PATH%\tools_def.txt
  if NOT exist %EDK_TOOLS_PATH%\Conf\tools_def.template (
    echo Error: tools_def.template is missing at folder %EDK_TOOLS_PATH%\Conf\
  )
  copy %EDK_TOOLS_PATH%\Conf\tools_def.template %CONF_PATH%\tools_def.txt > nul
) else (
  if defined RECONFIG echo overwrite ... tools_def.template to %CONF_PATH%\tools_def.txt
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

@REM Check Python environment

if not defined PYTHON_COMMAND (
  set PYTHON_COMMAND=py -3
  py -3 %BASE_TOOLS_PATH%\Tests\PythonTest.py %PYTHON_VER_MAJOR% %PYTHON_VER_MINOR% >NUL 2>NUL
  if %ERRORLEVEL% EQU 1 (
    echo.
    echo !!! ERROR !!! Python %PYTHON_VER_MAJOR%.%PYTHON_VER_MINOR% or newer is required.
    echo.
    goto end
  )
  if %ERRORLEVEL% NEQ 0 (
    if not defined PYTHON_HOME if not defined PYTHONHOME (
      set PYTHON_COMMAND=
      echo.
      echo !!! ERROR !!! Binary python tools are missing.
      echo PYTHON_COMMAND or PYTHON_HOME
      echo Environment variable is not set correctly.
      echo They are required to build or execute the python tools.
      echo.
      goto end
    )
  )
)

if not defined PYTHON_COMMAND (
  if defined PYTHON_HOME (
    if EXIST "%PYTHON_HOME%" (
      set PYTHON_COMMAND=%PYTHON_HOME%\python.exe
    ) else (
      echo .
      echo !!! ERROR !!!  PYTHON_HOME="%PYTHON_HOME%" does not exist.
      echo .
      goto end
    )
  )
)

%PYTHON_COMMAND% %BASE_TOOLS_PATH%\Tests\PythonTest.py %PYTHON_VER_MAJOR% %PYTHON_VER_MINOR% >NUL 2>NUL
if %ERRORLEVEL% EQU 1 (
  echo.
  echo !!! ERROR !!! Python %PYTHON_VER_MAJOR%.%PYTHON_VER_MINOR% or newer is required.
  echo.
  goto end
)
if %ERRORLEVEL% NEQ 0 (
  echo.
  echo !!! ERROR !!!  PYTHON_COMMAND="%PYTHON_COMMAND%" does not exist or is not a Python interpreter.
  echo.
  goto end
)

endlocal

  @echo Using EDK2 in-source Basetools
  if defined BASETOOLS_PYTHON_SOURCE goto print_python_info
  set "PATH=%BASE_TOOLS_PATH%\BinWrappers\WindowsLike;%PATH%"
  set PYTHONPATH=%BASE_TOOLS_PATH%\Source\Python;%PYTHONPATH%
  goto print_python_info

:print_python_info
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
  echo      PYTHON_COMMAND = %PYTHON_COMMAND%
  echo          PYTHONPATH = %PYTHONPATH%
  echo.

:VisualStudioAvailable
  if not defined FORCE_REBUILD (
    if not defined REBUILD (
      goto end
    )
  )

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
  echo  Usage: "%0 [-h | -help | --help | /h | /help | /?] [ Rebuild | ForceRebuild ] [Reconfig] [base_tools_path [edk_tools_path]] [VS2019] [VS2017] [VS2015]"
  @echo.
  @echo         base_tools_path   BaseTools project path, BASE_TOOLS_PATH will be set to this path.
  @echo         edk_tools_path    EDK_TOOLS_PATH will be set to this path.
  @echo         Rebuild           If sources are available perform an Incremental build, only
  @echo                           build those updated tools.
  @echo         ForceRebuild      If sources are available, rebuild all tools regardless of
  @echo                           whether they have been updated or not.
  @echo         Reconfig          Reinstall target.txt, tools_def.txt and build_rule.txt.
  @echo         VS2015            Set the env for VS2015 build.
  @echo         VS2017            Set the env for VS2017 build.
  @echo         VS2019            Set the env for VS2019 build.
  @echo         VS2022            Set the env for VS2022 build.
  @echo.

:end
set REBUILD=
set FORCE_REBUILD=
set RECONFIG=
set VS2022=
set VS2019=
set VS2017=
set VS2015=
set VSTool=
set PYTHON_VER_MAJOR=
set PYTHON_VER_MINOR=
popd
