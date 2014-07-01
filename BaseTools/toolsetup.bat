@REM @file
@REM   This stand-alone program is typically called by the edksetup.bat file, 
@REM   however it may be executed directly from the BaseTools project folder
@REM   if the file is not executed within a WORKSPACE\BaseTools folder.
@REM
@REM Copyright (c) 2006 - 2013, Intel Corporation. All rights reserved.<BR>
@REM
@REM This program and the accompanying materials are licensed and made available
@REM under the terms and conditions of the BSD License which accompanies this 
@REM distribution.  The full text of the license may be found at:
@REM   http://opensource.org/licenses/bsd-license.php
@REM
@REM THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
@REM WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR 
@REM IMPLIED.
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
  set PATH=%EDK_TOOLS_PATH%\Bin;%EDK_TOOLS_PATH%\Bin\Win32;%PATH%
  set WORKSPACE_TOOLS_PATH=%EDK_TOOLS_PATH%
  goto PATH_ok

:check_PATH
  if "%EDK_TOOLS_PATH%"=="%WORKSPACE_TOOLS_PATH%" goto PATH_ok
  set PATH=%EDK_TOOLS_PATH%\Bin;%EDK_TOOLS_PATH%\Bin\Win32;%PATH%
  set WORKSPACE_TOOLS_PATH=%EDK_TOOLS_PATH%
  echo Resetting the PATH variable to include the EDK_TOOLS_PATH for this session.

:PATH_ok
  echo           PATH      = %PATH%
  echo.
  if defined WORKSPACE (
    echo      WORKSPACE      = %WORKSPACE%
  )
  echo EDK_TOOLS_PATH      = %EDK_TOOLS_PATH%
  if defined BASE_TOOLS_PATH (
    echo BASE_TOOLS_PATH     = %BASE_TOOLS_PATH%
  )
  echo.

REM
REM copy *.template to %WORKSPACE%\Conf
REM
if not defined WORKSPACE (
   if defined RECONFIG (
     echo.
     echo !!! WARNING !!! WORKSPACE environment variable was not set, cannot Reconfig !!!
     echo.
   )
   goto skip_reconfig
)

if NOT exist %WORKSPACE%\Conf (
  mkdir %WORKSPACE%\Conf
) else (
  if defined RECONFIG (
    echo.
    echo  Over-writing the files in the WORKSPACE\Conf directory
    echo  using the default template files
    echo.
  )
)

if NOT exist %WORKSPACE%\Conf\target.txt (
  echo copying ... target.template to %WORKSPACE%\Conf\target.txt
  if NOT exist %EDK_TOOLS_PATH%\Conf\target.template (
    echo Error: target.template is missing at folder %EDK_TOOLS_PATH%\Conf\
  )
  copy %EDK_TOOLS_PATH%\Conf\target.template %WORKSPACE%\Conf\target.txt > nul
) else (
  if defined RECONFIG echo over-write ... target.template to %WORKSPACE%\Conf\target.txt
  if defined RECONFIG copy /Y %EDK_TOOLS_PATH%\Conf\target.template %WORKSPACE%\Conf\target.txt > nul
)

if NOT exist %WORKSPACE%\Conf\tools_def.txt (
  echo copying ... tools_def.template to %WORKSPACE%\Conf\tools_def.txt
  if NOT exist %EDK_TOOLS_PATH%\Conf\tools_def.template (
    echo Error: tools_def.template is missing at folder %EDK_TOOLS_PATH%\Conf\
  )
  copy %EDK_TOOLS_PATH%\Conf\tools_def.template %WORKSPACE%\Conf\tools_def.txt > nul
) else (
  if defined RECONFIG echo over-write ... tools_def.template to %WORKSPACE%\Conf\tools_def.txt
  if defined RECONFIG copy /Y %EDK_TOOLS_PATH%\Conf\tools_def.template %WORKSPACE%\Conf\tools_def.txt > nul
)

if NOT exist %WORKSPACE%\Conf\build_rule.txt (
  echo copying ... build_rule.template to %WORKSPACE%\Conf\build_rule.txt
  if NOT exist %EDK_TOOLS_PATH%\Conf\build_rule.template (
    echo Error: build_rule.template is missing at folder %EDK_TOOLS_PATH%\Conf\
  )
  copy %EDK_TOOLS_PATH%\Conf\build_rule.template %WORKSPACE%\Conf\build_rule.txt > nul
) else (
  if defined RECONFIG echo over-write ... build_rule.template to %WORKSPACE%\Conf\build_rule.txt
  if defined RECONFIG copy /Y %EDK_TOOLS_PATH%\Conf\build_rule.template %WORKSPACE%\Conf\build_rule.txt > nul
)

:skip_reconfig

@REM
@REM Test if we are going to have to do a build
@REM
if defined FORCE_REBUILD goto check_build_environment
if defined REBUILD goto check_build_environment
if not exist "%EDK_TOOLS_PATH%\Bin" goto check_build_environment

IF NOT EXIST "%EDK_TOOLS_PATH%\Bin\Win32\BootSectImage.exe" goto check_build_environment
IF NOT EXIST "%EDK_TOOLS_PATH%\Bin\Win32\build.exe" goto check_build_environment
IF NOT EXIST "%EDK_TOOLS_PATH%\Bin\Win32\EfiLdrImage.exe" goto check_build_environment
IF NOT EXIST "%EDK_TOOLS_PATH%\Bin\Win32\EfiRom.exe" goto check_build_environment
IF NOT EXIST "%EDK_TOOLS_PATH%\Bin\Win32\GenBootSector.exe" goto check_build_environment
IF NOT EXIST "%EDK_TOOLS_PATH%\Bin\Win32\GenFds.exe" goto check_build_environment
IF NOT EXIST "%EDK_TOOLS_PATH%\Bin\Win32\GenFfs.exe" goto check_build_environment
IF NOT EXIST "%EDK_TOOLS_PATH%\Bin\Win32\GenFv.exe" goto check_build_environment
IF NOT EXIST "%EDK_TOOLS_PATH%\Bin\Win32\GenFw.exe" goto check_build_environment
IF NOT EXIST "%EDK_TOOLS_PATH%\Bin\Win32\GenPage.exe" goto check_build_environment
IF NOT EXIST "%EDK_TOOLS_PATH%\Bin\Win32\GenSec.exe" goto check_build_environment
IF NOT EXIST "%EDK_TOOLS_PATH%\Bin\Win32\GenVtf.exe" goto check_build_environment
IF NOT EXIST "%EDK_TOOLS_PATH%\Bin\Win32\MigrationMsa2Inf.exe" goto check_build_environment
IF NOT EXIST "%EDK_TOOLS_PATH%\Bin\Win32\Split.exe" goto check_build_environment
IF NOT EXIST "%EDK_TOOLS_PATH%\Bin\Win32\TargetTool.exe" goto check_build_environment
IF NOT EXIST "%EDK_TOOLS_PATH%\Bin\Win32\TianoCompress.exe" goto check_build_environment
IF NOT EXIST "%EDK_TOOLS_PATH%\Bin\Win32\Trim.exe" goto check_build_environment
IF NOT EXIST "%EDK_TOOLS_PATH%\Bin\Win32\VfrCompile.exe" goto check_build_environment
IF NOT EXIST "%EDK_TOOLS_PATH%\Bin\Win32\Fpd2Dsc.exe" goto check_build_environment
IF NOT EXIST "%EDK_TOOLS_PATH%\Bin\Win32\VolInfo.exe" goto check_build_environment

goto end

:check_build_environment

  if not defined FORCE_REBUILD (
    echo.
    echo Rebuilding of tools is not required.  Binaries of the latest,
    echo tested versions of the tools have been tested and included in the
    echo EDK II repository.
    echo.
    echo If you really want to build the tools, use the ForceRebuild option.
    echo.
    goto end
  )

  if not defined BASE_TOOLS_PATH (
     if not exist "Source\C\Makefile" (
       if not exist "%EDK_TOOLS_PATH%\Source\C\Makefile" goto no_source_files
       set BASE_TOOLS_PATH=%EDK_TOOLS_PATH%
     ) else (
       set BASE_TOOLS_PATH=%CD%
     )
  )
  set PATH=%BASE_TOOLS_PATH%\Bin\Win32;%PATH%

  set BASETOOLS_PYTHON_SOURCE=%BASE_TOOLS_PATH%\Source\Python
  set PYTHONPATH=%BASETOOLS_PYTHON_SOURCE%;%PYTHONPATH%

  if not defined PYTHON_HOME (
    if defined PYTHONHOME (
      set PYTHON_HOME=%PYTHONHOME%
    ) else (
      echo.
      echo  !!! ERROR !!! PYTHON_HOME is required to build or execute the tools, please set it. !!!
      echo.
      goto end
    )
  )

  @REM We have Python, now test for FreezePython application
  if not defined PYTHON_FREEZER_PATH (
    @REM see if we can find FreezePython.ex
    if exist "%PYTHON_HOME%\Tools\cx_Freeze-3.0.3\FreezePython.exe" (
      set PYTHON_FREEZER_PATH=%PYTHON_HOME%\Tools\cx_Freeze-3.0.3
    ) 
    if exist "%PYTHON_HOME%\Tools\cx_Freeze\FreezePython.exe" (
      set PYTHON_FREEZER_PATH=%PYTHON_HOME%\Tools\cx_Freeze
    ) 
    if exist "C:\cx_Freeze\FreezePython.exe" (
        set PYTHON_FREEZER_PATH=C:\cx_Freeze
    )
    if exist "C:\cx_Freeze-3.0.3" (
        set PYTHON_FREEZER_PATH=C:\cx_Freeze-3.0.3
    )
    if not defined PYTHON_FREEZER_PATH (
      echo.
      echo !!! WARNING !!! Will not be able to compile Python programs to .exe
      echo Will setup environment to run Python scripts directly.
      echo.
      set PATH=%BASETOOLS_PYTHON_SOURCE%\Trim;%PATH%
      set PATH=%BASETOOLS_PYTHON_SOURCE%\GenFds;%PATH%
      set PATH=%BASETOOLS_PYTHON_SOURCE%\build;%PATH%
      set PATHEXT=%PATHEXT%;.py
    )
  )
  
  echo BASE_TOOLS_PATH     = %BASE_TOOLS_PATH%
  echo     PYTHON_PATH     = %PYTHON_PATH%
  echo PYTHON_FREEZER_PATH = %PYTHON_FREEZER_PATH%
  echo.

  call "%EDK_TOOLS_PATH%\get_vsvars.bat"
  if not defined VCINSTALLDIR (
    @echo.
    @echo !!! ERROR !!!! Cannot find Visual Studio, required to build C tools !!!
    @echo.
    goto end
  )

:VisualStudioAvailable
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

  if defined PYTHON_FREEZER_PATH (
    echo BUILDING PYTHON TOOLS
    pushd .
    cd %BASE_TOOLS_PATH%
    call nmake python
    popd
  ) else (
    echo.
    echo !!! WARNING !!! Cannot make executable from Python code, executing python scripts instead !!!
    echo.
  )
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

