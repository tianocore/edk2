@REM @file
@REM Windows batch file, Edk2Setup.bat, to set up an EDK II build environment
@REM
@REM This script will be renamed to edksetup.bat after initial testing and
@REM acceptance by the open source community.
@REM
@REM This script will set the following system environment variables:
@REM   WORKSPACE, EDK_TOOLS_PATH, PATH
@REM If rebuilding the tools:
@REM   BASE_TOOLS_PATH, PYTHON_FREEZER_PATH, PYTHONPATH
@REM It will also create a Conf/target.txt, tools_def.txt and build_rule.txt files
@REM if they do not exist
@REM If the reset flag is set, all environment variables will be reset and 
@REM the Conf/target.txt, tools_def.txt and build_rule.txt files will be overwritten
@REM
@REM Three other scripts, located in the BaseTools\Scripts directory, may be called
@REM by this script.
@REM SetVisualStudio.bat    - will set the Visual Studio environment based on the --vs* flags
@REM                          it is also used to build the Win32 binaries, calling nmake 
@REM                          using the WORKSPACE\BaseTools\Makefile
@REM ShowEnvironment.bat    - will display the current EDK II Build environment
@REM UpdateBuildVersions.py - script is called prior to building the EDK II BaseTools from
@REM                          Sources. This tool will modify the BuildVersion.* files so that
@REM                          when the tools get built, they will have a custom version entry
@REM                          similar to the following:
@REM     e:\edk2>build --version
@REM     build.exe Version 0.51 Developer Build based on Revision: 15668
@REM
@REM Copyright (c) 2014, Intel Corporation. All rights reserved.<BR>
@REM This program and the accompanying materials
@REM are licensed and made available under the terms and conditions of the BSD License
@REM which accompanies this distribution.  The full text of the license may be found at
@REM http://opensource.org/licenses/bsd-license.php
@REM
@REM THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
@REM WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
@REM
@REM
@REM usage:
@REM   Edk2Setup.bat [--help | -h] [-v | --version] [-r | --reset] 
@REM                 [--reconfig] [--edk-tools-path DirName]
@REM                 [--pull [Directory]| --rebuild [Directory]]
@REM                 [--nt32 [X64]]

@REM ##############################################################
@REM # You should not have to modify anything below this line
@REM #
@echo off
@set SCRIPT_NAME=%0
@set SCRIPT_VERSION=0.9.2.
@set SVN_REVISION=$Revision$
@set RESET_ENVIRONMENT=FALSE
@set NT32PKG=FALSE
@set NT32_X64=
@set BUILD_TOOLS_WITH=
@set LOCATION=
@set REBUILD_TOOLS=
@set SVN_PULL=
@set SRC_CONF=
@set ARGUMENT=
@set SCRIPT=EDKSETUP_BAT

@if not defined ORIGINAL_PATH set "ORIGINAL_PATH=%PATH%"
@REM Always set the WORKSPACE environment variable to the current directory
@set "WORKSPACE=%CD%"
@if exist "%WORKSPACE%\BaseTools" @set "BASE_TOOLS_PATH=%WORKSPACE%\BaseTools"
@if not exist "%WORKSPACE%\Conf" @mkdir "%WORKSPACE%\Conf"

@@if not defined EDK_TOOLS_PATH @set "EDK_TOOLS_PATH=%WORKSPACE%\BaseTools"
@rem   @set "PATH=%WORKSPACE%\BaseTools\Bin\Win32;%PATH%"
@rem   @set WORKSPACE_TOOLS_PATH=%WORKSPACE%\BaseTools
@rem )

@REM Keep the existing EDK_TOOLS_PATH value, the --reset flag will set it
@REM back to WORKSPACE\BaseTools while the --location DIRECTORY flag will
@REM still take precedence if the location option follows the reset option
@REM on the command line.
@if defined EDK_TOOLS_PATH @set "LOCATION=%EDK_TOOLS_PATH%"

:parse_cmd_line
@if "%1"=="" @goto MainRoutine
@if /I "%1"=="-h" @goto Usage
@if /I "%1"=="--help" @goto Usage
@if /I "%1"=="/?" @goto Usage
@if /I "%1"=="--version" @goto Version

@REM These options will reset the system environment
@if /I "%1"=="-r" (
    @setlocal EnableDelayedExpansion
    @set "WORKSPACE=%CD%"
    @set "EDK_TOOLS_PATH=%CD%\BaseTools"
    @set "LOCATION=!EDK_TOOLS_PATH!"
    @endlocal
    @shift
    @goto parse_cmd_line
)
@if /I "%1"=="--reset" (
    @set "WORKSPACE=%CD%"
    @set "EDK_TOOLS_PATH=%WORKSPACE%\BaseTools"
    @set "LOCATION=%WORKSPACE%\BaseTools"
    @shift
    @goto parse_cmd_line
)

@REM This option is used to overwrite the Conf/*.txt files with the
@REM WORKSPACE\BaseTools\Conf\*.template files.
@if /I "%1"=="--reconfig" (
    @set RECONFIG=TRUE
    @shift
    @goto parse_cmd_line
)

@REM This option can be used to set the EDK_TOOLS_PATH containing the Win32 binaries to an
@REM alternate directory
@if /I "%1"=="--edk-tools-path" (
    @setlocal EnableDelayedExpansion
    @set ARGUMENT=%2
    @if "!ARGUMENT:~0,2!"=="--" (
        @echo.
        @echo ERROR : The --edk-tools-path flag requires an argument
        @echo.
        @endlocal
        @goto Usage
    )
    @endlocal
    @set "LOCATION=%WORKSPACE%\%2"
    @shift
    @shift
    @goto parse_cmd_line
)

@REM Force pulling updated (or checkout if they do not exist) from SVN for the BaseTools\Bin\Win32 directory
@REM or the directory pointed to by the --location option
@if /I "%1"=="--pull" (
    @if "%REBUILD_TOOLS%"=="TRUE" (
        @echo.
        @echo ERROR: The --pull option may not be used with the --rebuild option
        @shift
        @goto ExitFailure
    )
    @set SVN_PULL=TRUE
    @setlocal EnableDelayedExpansion
    @set ARGUMENT=%2
    @if not "!ARGUMENT:~0,2!"=="--" (
        @endlocal
        @set "LOCATION=%2"
        @shift
    )
    @shift
    @goto parse_cmd_line
)

@REM This options forces rebuilding the tools (provided the required tools are available
@if /I "%1"=="--rebuild" (
    @if "%SVN_PULL%"=="TRUE" (
        @echo.
        @echo ERROR: The --reset option may not be used with the --pull option
        @shift
        @goto ExitFailure
    )
    @set REBUILD_TOOLS=TRUE
    @setlocal EnableDelayedExpansion
    @set ARGUMENT=%2
    @if not "!ARGUMENT:~0,2!"=="--" (
        @endlocal
        @set "LOCATION=%2"
        @shift
    )
    @shift
    goto parse_cmd_line
)

@REM This option will try to set the compiler environment for building Nt32Pkg/Nt32Pkg.dsc
@REM If the compiler environment is already installed, then no additional work is required.
@if /I "%1"=="--nt32" (
    @set NT32PKG=TRUE
    @if not defined BUILD_TOOLS_WITH (
        @set BUILD_TOOLS_WITH=Latest
    )
    @REM This option will try to set the environment for building the Nt32Pkg/Nt32Pkg; on a 64-bit 
    @REM Windows OS
    @if /I "%2"=="X64" (
        @set NT32_X64=TRUE
        @shift
    )
  @shift
  @goto parse_cmd_line
)

@if not "%1"=="" goto UnknownOptionOrArgument
@goto MainRoutine

:Usage
@echo Usage: %SCRIPT_NAME% [Options]
@echo Copyright(c) 2014, Intel Corporation. All rights reserved.
@echo.
@echo The system environment variable, WORKSPACE, is always set to the current 
@echo working directory.
@echo.
@echo Options:
@echo   --help, -h          Print this help screen and exit.
@echo.
@echo   --version           Print this script's version and exit.
@echo.
@echo   --reset, -r         Reset the EDK_TOOLS_PATH and PATH system environment
@echo                       variables. The EDK_TOOLS_PATH default is 
@echo                       WORKSPACE\BaseTools, however, it may be overridden by
@echo                       arguments given to the --edk-tools-path, --pull and/or
@echo                       --rebuild options.
@echo                       Once set, the EDK_TOOLS_PATH environment variable reset
@echo                       by opening up a new command prompt window or through one
@echo                       of the options provided by this tool
@echo.
@echo   --reconfig          Overwrite the WORKSPACE/Conf/*.txt files with the
@echo                       template files from the BaseTools/Conf directory.
@echo.
@echo   --edk-tools-path  DIRECTORY
@echo                       This option sets the EDK_TOOLS_PATH to the DIRECTORY
@echo                       value instead of the default (WORKSPACE\BaseTools).
@echo.
@echo   --nt32 [X64]        If a compiler tool chain is not available in the
@echo                       environment, call a script to attempt to set one up.
@echo                       This flag is only required if building the
@echo                       Nt32Pkg/Nt32Pkg.dsc system emulator.
@echo                       If the X64 argument is set, and a compiler tool chain is
@echo                       not available, attempt to set up a tool chain that will
@echo                       create X64 binaries. Setting these two options have the
@echo                       potential side effect of changing tool chains used for a
@echo                       rebuild.
@echo.
@pause
@echo.
@echo   --pull [DIRECTORY]  Get the EDK II BaseTools binaries from source control 
@echo                       (must not be used with --rebuild).
@echo                       If the optional DIRECTORY argument is specified, the tool
@echo                       sets EDK_TOOLS_PATH to DIRECTORY.
@echo                       If the DIRECTORY argument is not specified, the tools are
@echo                       placed in the directory tree pointed to by the current 
@echo                       EDK_TOOLS_PATH environment variable. If the binaries
@echo                       cannot be obtained from source control, the 
@echo                       EDK_TOOLS_PATH will be set to the default, 
@echo                       WORKSPACE\BaseTools directory.
@echo.
@echo   --rebuild  [DIRECTORY]
@echo                       Force Rebuilding the EDK II BaseTools from source
@echo                      (must not be used with --pull).
@echo                           NOTE: The build will use whatever compiler tool set
@echo                                 is available in the environment prior to
@echo                                 running edksetup.bat.
@echo                       If the optional DIRECTORY argument is specified, the tool
@echo                       sets EDK_TOOLS_PATH to DIRECTORY. Tools binaries will be
@echo                       placed in the appropriate subdirectory in the 
@echo                       EDK_TOOLS_PATH directory. If the build fails, the
@echo                       EDK_TOOLS_PATH will be set to the default,
@echo                       WORKSPACE\BaseTools directory.
@goto ExitSuccess

:Version
@echo %SCRIPT_NAME% Version: %SCRIPT_VERSION%%SVN_REVISION:~11,-1%
@echo Copyright(c) 2014, Intel Corporation. All rights reserved.
@set HIDE_PATH=TRUE
@call "%WORKSPACE%\BaseTools\Scripts\ShowEnvironment.bat"
@set HIDE_PATH=
@goto ExitSuccess

:UnknownOptionOrArgument
@echo. ERROR : This argument is not valid: %1
@echo.
@goto ExitFailure

:NoVisualStudio
@echo ERROR : Unable to determine if a compiler tool chain has been enabled in this
@echo         command-prompt window. Rebuilding of the tools with this script is not
@echo         possible.
@echo         Refer to the BaseTools\BuildNotes.txt for directions for building 
@echo         the BaseTools binaries.
@echo.
@goto ExitFailure

:NoPython
@echo ERROR : Unable to rebuild the BaseTools binaries, python does not appear to be
@echo         installed. If python is installed, please set the environment
@echo         variable, PYTHONHOME to the Path to the python.exe, for example,
@echo         if python.exe is located in the C:\Python27 directory, then:
@echo         set PYTHONHOME=C:\Python27
@echo.
@goto ExitFailure

:BadPython
@echo ERROR : Unable to rebuild the BaseTools binaries, python does not appear to be
@echo         installed. 
@echo         The python executable was not found in the PYTHONHOME: %PYTHONHOME%
@echo         If python is installed, please set the environment variable, PYTHONHOME 
@echo         to the Path that contains python.exe, for example, if python.exe is
@echo         located in the C:\Python27 directory, then:
@echo         set PYTHONHOME=C:\Python27
@echo.
@goto ExitFailure

:NoCxFreeze
@echo ERROR : Unable to locate cx_Freeze 4.2.3. The cxfreeze.bat file must be located
@echo         in the %PYTHONHOME%\Scripts directoryin order to rebuild the BaseTools
@echo         binaries.
@echo.
@goto ExitFailure

:NoBaseTools
@echo ERROR: Unable to locate the BaseTools directory containing the Source tree
@echo.
@goto ExitFailure

@REM #########################################################################################
@REM MAIN ROUTINE
@REM Set up the Build System environment
@REM #########################################################################################
:MainRoutine
@if defined LOCATION @set "EDK_TOOLS_PATH=%LOCATION%"
@REM SET the EDK_TOOLS_PATH.
@if not exist "%EDK_TOOLS_PATH%" (
    @mkdir %EDK_TOOLS_PATH%
)
@if not defined NASM_PREFIX (
    @echo.
    @echo WARNING : NASM_PREFIX environment variable is not set
    @if exist "C:\nasm\nasm.exe" @set "NASM_PREFIX=C:\nasm\"
    @if exist "C:\nasm\nasm.exe" @echo   Found nasm.exe, setting the environment variable to C:\nasm\
    @if not exist "C:\nasm\nasm.exe" echo   Attempting to build modules that require NASM will fail.
)
@REM Set up the path to include the EDK_TOOLS_PATH\Bin\Win32 directory; this test determines
@REM whether the path is in the workspace or a fully qualified path that may be outside of
@REM the workspace
@if exist "%WORKSPACE%\%EDK_TOOLS_PATH%" @set "EDK_TOOLS_PATH=%WORKSPACE%\%EDK_TOOLS_PATH%"

@if defined REBUILD_TOOLS goto SetConf
@if defined SVN_PULL goto SetConf
@if not exist "%EDK_TOOLS_PATH%\Bin\Win32\build.exe" (
    @echo ERROR : %EDK_TOOLS_PATH%\Bin\Win32\build.exe does not exist
    @echo         Re-run this script using --reset, --pull or --rebuild
    @echo.
    @goto ExitFailure
)
@echo.
@echo Rebuilding of the tools is not required. Binaries of the latest,
@echo tested versions of the tools have been tested and included in the
@echo EDK II repository.
@echo.
@echo If you really want to build the tools, use the --rebuild option.
@echo.
@if not defined CYGWIN_HOME @echo "!!! WARNING !!! No CYGWIN_HOME set, gcc build may not be used !!!"
@if not defined CYGWIN_HOME @echo.
@REM Make sure the WORKSPACE\Conf directory contains the required text files that will be
@REM copied or replaced from the WORKSPACE\BaseTools\Conf directories' template files.
:SetConf
@if not exist "%EDK_TOOLS_PATH%\Conf" (
    @if exist "%WORKSPACE%\BaseTools\Conf" (
        @set "SRC_CONF=%WORKSPACE%\BaseTools\Conf"
    )
) else (
    @set "SRC_CONF=%EDK_TOOLS_PATH%\Conf"
)
@if not defined SRC_CONF (
    @echo ERROR : Unable to locate the BaseTools directory tree
    @goto ExitFailure
)

@REM The script will test to see if the files exist, and also use the RESET_ENVIRONMENT flag
@REM to overwrite the WORKSPACE\Conf *.txt files.
@call "%WORKSPACE%\BaseTools\Scripts\ShowEnvironment.bat"
@if errorlevel 1 (
    @echo Unable to copy the template files from "%SRC_CONF%" to "%WORKSPACE%\Conf"
    @goto ExitFailure
)
@set SRC_CONF=

@REM Set up Visual Studio if required to build the Nt32Pkg/Nt32Pkg.dsc emulator
@if "%NT32PKG%"=="TRUE" (
    @if not defined VSINSTALLDIR @set "PATH=%ORIGINAL_PATH%"
    @if not defined NT32_X64 @call "%WORKSPACE%\BaseTools\get_vsvars.bat"
    @if defined NT32_X64 call "%WORKSPACE%\BaseTools\Scripts\SetVisualStudio.bat"
)
@if "%NT32PKG%"=="TRUE" (
    @if not defined VS_PATH set "VS_PATH=%PATH%"
)
@if defined VS_PATH @set "PATH=%VS_PATH%"
@if not defined VS_PATH @set "PATH=%ORIGINAL_PATH%"
@set "PATH=%EDK_TOOLS_PATH%\Bin\Win32;%PATH%"

@if "%REBUILD_TOOLS%"=="TRUE" @goto Rebuild
@if "%SVN_PULL%"== "TRUE" (
    if defined PYTHONHOME (
        @REM Use the python script if possible to test is the svn command is available, if it fails, the user may be
        @REM able to rebuild the Win32 binaries
        @call "%WORKSPACE%\BaseTools\Scripts\UpdateBuildVersions.py" --svn-test -v
        @if errorlevel 1 (
            @echo ERROR : The command-line svn tool is not available and the Win32 binaries do not exist
            @echo         Please re-run this script again with the --rebuild option to attempt to build 
            @echo         the binaries
            @echo.
            @goto ExitFailure
        )
        @if exist %EDK_TOOLS_PATH%\Bin\Win32 @rmdir /S /Q %EDK_TOOLS_PATH%\Bin\Win32
        @call svn co https://svn.code.sf.net/p/edk2-toolbinaries/code/trunk/Win32 "%EDK_TOOLS_PATH%\Bin\Win32"
    ) else (
        @call svn co https://svn.code.sf.net/p/edk2-toolbinaries/code/trunk/Win32 "%EDK_TOOLS_PATH%\Bin\Win32"
        @if errorlevel 1 (
            @echo ERROR : The command-line svn tool is not available and the Win32 binaries do not exist
            @echo         Python does not appear to be available either. This script cannot be used to
            @echo         build the Win32 binaries or to obtain them from this repository:
            @echo            https://svn.code.sf.net/p/edk2-toolbinaries/code/trunk/Win32
            @goto ExitFailure
        )
    )
    @goto ShowAndExit
)

@if not "%REBUILD_TOOLS%"=="TRUE" @goto ShowAndExit

@REM The following code is used to rebuild the Win32 BaseTools binaries - check that required tools are available
:Rebuild
@if not defined BASE_TOOLS_PATH @set "BASE_TOOLS_PATH=%WORKSPACE%\BaseTools"
@if not exist "%BASE_TOOLS_PATH%\Source" @goto NoBaseTools
@endlocal
@if not defined VCINSTALLDIR @goto NoVisualStudio
@if not defined PYTHONHOME @goto NoPython
@if not exist "%PYTHONHOME%\python.exe" @goto BadPython
@REM python.exe has been located, now make sure it's in the PATH
@call python --version > nul 2>&1
@if errorlevel 1 @set "PATH=%PYTHONHOME%\python.exe;%PATH%"
@if not defined PYTHON_FREEZER_PATH (
    @if not exist "%PYTHONHOME%\Scripts\cxfreeze.bat" @goto NoCxFreeze
    @set "PYTHON_FREEZER_PATH=%PYTHONHOME%\Scripts"
)
@call "%WORKSPACE%\BaseTools\Scripts\SetVisualStudio.bat"
@if errorlevel 1 @goto ExitFailure

:ShowAndExit
@call "%WORKSPACE%\BaseTools\Scripts\ShowEnvironment.bat"

@REM #########################################################################################
@REM EXIT ROUTINES
@REM #########################################################################################
:ExitSuccess
@set SCRIPT_NAME=
@set SCRIPT_VERSION=
@set SVN_REVISION=
@set RESET_ENVIRONMENT=
@set RECONFIG=
@set NT32PKG=
@set BUILD_TOOLS_WITH=
@set LOCATION=
@set REBUILD_TOOLS=
@set SVN_PULL=
@set SRC_CONF=
@set ARGUMENT=
@set SCRIPT=
@set LIST_VS_VERSIONS=
@set PYTHON_FREEZER_PATH=
@echo on
@exit /B 0

:ExitFailure
@set SCRIPT_NAME=
@set SCRIPT_VERSION=
@set SVN_REVISION=
@set RESET_ENVIRONMENT=
@set RECONFIG=
@set NT32PKG=
@set BUILD_TOOLS_WITH=
@set LOCATION=
@set REBUILD_TOOLS=
@set SVN_PULL=
@set SRC_CONF=
@set ARGUMENT=
@set SCRIPT=
@set LIST_VS_VERSIONS=
@set PYTHON_FREEZER_PATH=
@echo on
@exit /B 1
