@REM @file
@REM Windows batch file to display the Windows environment
@REM
@REM This script will be used to show the current EDK II build environment.
@REM it may be called by the Edk2Setup.bat (that will be renamed to edksetup.bat) or
@REM run as stand-alone application.
@REM
@REM Copyright (c) 2014, Intel Corporation. All rights reserved.<BR>
@REM SPDX-License-Identifier: BSD-2-Clause-Patent
@REM
@echo off
@set SE_SVN_REVISION=$Revision: 8 $
@set SE_VERSION=0.7.0.
@if "%SCRIPT%"=="EDKSETUP_BAT" goto SkipCmdlineArgumentCheck

:parse_cmd_line
@if /I "%1"=="-h" @goto Usage
@if /I "%1"=="--help" @goto Usage
@if /I "%1"=="/?" @goto Usage
@if /I "%1"=="-v" @goto Version
@if /I "%1"=="--version" @goto Version

:Usage
@echo Usage: ShowEnvironment.bat [Options]
@echo Copyright(c) 2014, Intel Corporation. All rights reserved.
@echo.
@echo Options:
@echo   --help, -h     Print this help screen and exit
@echo   --version, -v  Print this tool's version and exit
@echo.
@goto End

:Version
@echo ShowEnvironment.bat Version: %SE_VERSION%%SE_SVN_REVISION:~11,-1%
@echo Copyright(c) 2014, Intel Corporation. All rights reserved.

:SkipCmdlineArgumentCheck
if defined SRC_CONF @goto SetEnv

@echo.
@echo #############################################################################
@if defined WORKSPACE @echo     WORKSPACE            = %WORKSPACE%
@if not defined WORKSPACE @echo     WORKSPACE            = Not Set
@if defined PACKAGES_PATH @echo     PACKAGES_PATH        = %PACKAGES_PATH%
@if defined EDK_TOOLS_PATH @echo     EDK_TOOLS_PATH       = %EDK_TOOLS_PATH%
@if not defined EDK_TOOLS_PATH @echo     EDK_TOOLS_PATH       = Not Set
@if defined BASE_TOOLS_PATH @echo     BASE_TOOLS_PATH      = %BASE_TOOLS_PATH%
@if defined EDK_TOOLS_BIN @echo     EDK_TOOLS_BIN        = %EDK_TOOLS_BIN%
@if "%NT32PKG%"=="TRUE" (
    @echo.
    @echo NOTE: Please configure your build to use the following TOOL_CHAIN_TAG
    @echo       when building NT32Pkg/Nt32Pkg.dsc
    @if defined VCINSTALLDIR @call :CheckVsVer
    @set TEST_VS=
)
@if defined HIDE_PATH goto End


@echo ############################## PATH #########################################
@setlocal DisableDelayedExpansion
@set "var=%PATH%"
@set "var=%var:"=""%"
@set "var=%var:^=^^%"
@set "var=%var:&=^&%"
@set "var=%var:|=^|%"
@set "var=%var:<=^<%"
@set "var=%var:>=^>%"
@set "var=%var:;=^;^;%"
@set var=%var:""="%
@set "var=%var:"=""Q%"
@set "var=%var:;;="S"S%"
@set "var=%var:^;^;=;%"
@set "var=%var:""="%"
@setlocal EnableDelayedExpansion
@set "var=!var:"Q=!"
@for %%a in ("!var:"S"S=";"!") do (
    @if "!!"=="" endlocal
    @if %%a neq "" echo     %%~a
)
@goto End

:CheckVsVer
@set "TEST_VS=C:\Program Files (x86)\Microsoft Visual Studio 9.0\"
@if "%VSINSTALLDIR%"=="%TEST_VS%" (
    @echo     TOOL_CHAIN_TAG       = VS2008x86
    @goto :EOF
)
@set "TEST_VS=C:\Program Files\Microsoft Visual Studio 9.0\"
@if "%VSINSTALLDIR%"=="%TEST_VS%" (
    @echo     TOOL_CHAIN_TAG       = VS2008
    @goto :EOF
)

@set "TEST_VS=C:\Program Files (x86)\Microsoft Visual Studio 10.0\"
@if "%VSINSTALLDIR%"=="%TEST_VS%" (
    @echo     TOOL_CHAIN_TAG       = VS2010x86
    @goto :EOF
)
@set "TEST_VS=C:\Program Files\Microsoft Visual Studio 10.0\"
@if "%VSINSTALLDIR%"=="%TEST_VS%" (
    @echo     TOOL_CHAIN_TAG       = VS2010
    @goto :EOF
)

@set "TEST_VS=C:\Program Files (x86)\Microsoft Visual Studio 11.0\"
@if "%VSINSTALLDIR%"=="%TEST_VS%" (
    @echo     TOOL_CHAIN_TAG       = VS2012x86
    @goto :EOF
)
@set "TEST_VS=C:\Program Files\Microsoft Visual Studio 11.0\"
@if "%VSINSTALLDIR%"=="%TEST_VS%" (
    @echo     TOOL_CHAIN_TAG       = VS2012
    @goto :EOF
)

@set "TEST_VS=C:\Program Files (x86)\Microsoft Visual Studio 12.0\"
@if "%VSINSTALLDIR%"=="%TEST_VS%" (
    @echo     TOOL_CHAIN_TAG       = VS2013x86
    @goto :EOF
)
@set "TEST_VS=C:\Program Files\Microsoft Visual Studio 12.0\"
@if "%VSINSTALLDIR%"=="%TEST_VS%" (
    @echo     TOOL_CHAIN_TAG       = VS2013
    @goto :EOF
)

@set "TEST_VS=C:\Program Files (x86)\Microsoft Visual Studio 14.0\"
@if "%VSINSTALLDIR%"=="%TEST_VS%" (
    @echo     TOOL_CHAIN_TAG       = VS2015x86
    @goto :EOF
)
@set "TEST_VS=C:\Program Files\Microsoft Visual Studio 14.0\"
@if "%VSINSTALLDIR%"=="%TEST_VS%" (
    @echo     TOOL_CHAIN_TAG       = VS2015
    @goto :EOF
)
@goto :EOF

:SetEnv
@set FIRST_COPY=FALSE
@set MISSING_TARGET_TEMPLATE=FALSE
@set MISSING_TOOLS_DEF_TEMPLATE=FALSE
@set MISSING_BUILD_RULE_TEMPLATE=FALSE
@if not exist "%SRC_CONF%\target.template"  @set MISSING_TARGET_TEMPLATE=TRUE
@if not exist "%SRC_CONF%\tools_def.template" @set MISSING_TOOLS_DEF_TEMPLATE=TRUE
@if not exist "%SRC_CONF%\build_rule.template" @set MISSING_BUILD_RULE_TEMPLATE=TRUE

@if not exist "%WORKSPACE%\Conf\target.txt" (
    @if "%MISSING_TARGET_TEMPLATE%"=="TRUE" @goto MissingTemplates
    @echo copying ... target.template to %WORKSPACE%\Conf\target.txt
    @copy /Y "%SRC_CONF%\target.template" "%WORKSPACE%\Conf\target.txt" > nul
    @set FIRST_COPY=TRUE
)
@if not exist "%WORKSPACE%\Conf\tools_def.txt" (
    @if "%MISSING_TOOLS_DEF_TEMPLATE%"=="TRUE" @goto MissingTemplates
    @echo copying ... tools_def.template to %WORKSPACE%\Conf\tools_def.txt
    @copy /Y "%SRC_CONF%\tools_def.template" "%WORKSPACE%\Conf\tools_def.txt" > nul
    @set FIRST_COPY=TRUE
)
@if not exist "%WORKSPACE%\Conf\build_rule.txt" (
    @if "%MISSING_BUILD_RULE_TEMPLATE%"=="TRUE" @goto MissingTemplates
    @echo copying ... build_rule.template to %WORKSPACE%\Conf\build_rule.txt
    @copy /Y "%SRC_CONF%\build_rule.template" "%WORKSPACE%\Conf\build_rule.txt" > nul
    @set FIRST_COPY=TRUE
)

@if "%FIRST_COPY%"=="TRUE" @goto End
@if not "%RECONFIG%"=="TRUE" @goto End

@if "%RECONFIG%"=="TRUE" (
    @echo.
    @echo  Over-writing the files in the WORKSPACE\Conf directory
    @echo  using the default template files
    @echo.
    @if "%MISSING_TARGET_TEMPLATE%"=="TRUE" @goto MissingTemplates
    @echo over-write ... target.template to %WORKSPACE%\Conf\target.txt
    @copy /Y "%SRC_CONF%\target.template" "%WORKSPACE%\Conf\target.txt" > nul

    @if "%MISSING_TOOLS_DEF_TEMPLATE%"=="TRUE" @goto MissingTemplates
    @echo over-write ... tools_def.template to %WORKSPACE%\Conf\tools_def.txt
    @copy /Y "%SRC_CONF%\tools_def.template" "%WORKSPACE%\Conf\tools_def.txt" > nul

    @if "%MISSING_BUILD_RULE_TEMPLATE%"=="TRUE" @goto MissingTemplates
    @echo over-write ... build_rule.template to %WORKSPACE%\Conf\build_rule.txt
    @copy /Y "%SRC_CONF%\build_rule.template" "%WORKSPACE%\Conf\build_rule.txt" > nul
    @goto End
)

:MissingTemplates
@echo.
@if "%RECONFIG%"=="TRUE" @echo ERROR : Reconfig failed
@if "%MISSING_TARGET_TEMPLATE%"=="TRUE" @echo ERROR : Unable to locate: "%SRC_CONF%\target.template"
@if "%MISSING_TOOLS_DEF_TEMPLATE%"=="TRUE" @echo ERROR : Unable to locate: "%SRC_CONF%\tools_def.template"
@if "%MISSING_BUILD_RULE_TEMPLATE%"=="TRUE" @echo ERROR : Unable to locate: "%SRC_CONF%\build_rule.template"
@echo.
@set MISSING_TARGET_TEMPLATE=
@set MISSING_TOOLS_DEF_TEMPLATE=
@set MISSING_BUILD_RULE_TEMPLATE=
@set FIRST_COPY=
@set SE_VERSION=
@set SE_SVN_REVISION=
@if not "%SCRIPT%"=="EDKSETUP_BAT" @echo on
exit /B 1

:End
@set MISSING_TARGET_TEMPLATE=
@set MISSING_TOOLS_DEF_TEMPLATE=
@set MISSING_BUILD_RULE_TEMPLATE=
@set FIRST_COPY=
@set SE_VERSION=
@set SE_SVN_REVISION=
@if not "%SCRIPT%"=="EDKSETUP_BAT" @echo on
exit /B 0
