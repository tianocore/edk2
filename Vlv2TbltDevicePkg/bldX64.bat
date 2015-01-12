@REM @file
@REM   Windows batch file to build BIOS ROM
@REM
@REM Copyright (c) 2006 - 2014, Intel Corporation. All rights reserved.<BR>
@REM This program and the accompanying materials
@REM are licensed and made available under the terms and conditions of the BSD License
@REM which accompanies this distribution.  The full text of the license may be found at
@REM http://opensource.org/licenses/bsd-license.php
@REM
@REM THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
@REM WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
@REM

@echo off
@echo.
@time /t

@if exist %WORKSPACE%\edk2.log del %WORKSPACE%\edk2.log
@if exist %WORKSPACE%\unitool.log del %WORKSPACE%\unitool.log
@if exist %WORKSPACE%\Conf\build_rule.txt del %WORKSPACE%\Conf\build_rule.txt
@if exist %WORKSPACE%\Conf\FrameworkDatabase.db del %WORKSPACE%\Conf\FrameworkDatabase.db
@if exist %WORKSPACE%\Conf\target.txt del %WORKSPACE%\Conf\target.txt
@if exist %WORKSPACE%\Conf\tools_def.txt del %WORKSPACE%\Conf\tools_def.txt
@if exist conf\.cache rmdir /q /s conf\.cache

@set target=DEBUG

@if /i "%1" == "release" set TARGET=RELEASE
@if /i "%2" == "release" set TARGET=RELEASE
@if /i "%3" == "release" set TARGET=RELEASE
@if /i "%4" == "release" set TARGET=RELEASE
@if /i "%5" == "release" set TARGET=RELEASE

@set PlatformType=NO_PLATFORM
@set config_file=.\Vlv2TbltDevicePkg\PlatformPkgConfig.dsc
@set EVN_debug_file=.\Vlv2TbltDevicePkg\BiosIdx64D.env
@set EVN_release_file=.\Vlv2TbltDevicePkg\BiosIdx64R.env
@set auto_config_inc=.\Vlv2TbltDevicePkg\AutoPlatformCFG.txt

@if  "%1" == "MNW2" (
  set %PlatformType% = MNW2
  @echo  Setting Baley Bay platform configration and BIOS ID ...
  findstr /b /v BOARD_ID %EVN_debug_file% > newfile.env
  echo BOARD_ID = MNW2MAX >> newfile.env
  type newfile.env > %EVN_debug_file%
  findstr /b /v BOARD_ID %EVN_release_file% > newfile.env
  echo BOARD_ID = MNW2MAX >> newfile.env
  type newfile.env > %EVN_release_file%
  echo DEFINE ENBDT_PF_BUILD = TRUE  >> %auto_config_inc%
  echo DEFINE X64_CONFIG = TRUE  >> %auto_config_inc%
  goto PLATFORM_SETTING_DONE
)  

@if  "%PlatformType%" == "NO_PLATFORM" (
  goto BldFail
)

rem clearup the temp file
:PLATFORM_SETTING_DONE
@DEL NEWFILE.ENV
@DEL NEWFILE.CFG

@REM Define platform specific environment variables.
@REM
@set PLATFORM_PACKAGE=Vlv2TbltDevicePkg
@set SCRIPT_ERROR=0

@REM Set basic environment.
@echo.
@echo Run edksetup.bat batch file.
@echo.
@del Conf\build_rule.txt
@REM @del Conf\tools_def.txt
@call edksetup.bat


@echo.
@echo Set the VS2008 environment.
@echo.
@if defined VS90COMNTOOLS (
  if not defined VSINSTALLDIR call "%VS90COMNTOOLS%\vsvars32.bat"
  if /I "%VS90COMNTOOLS%" == "C:\Program Files\Microsoft Visual Studio 9.0\Common7\Tools\" (
    set TOOL_CHAIN_TAG=VS2008
  ) else (
    set TOOL_CHAIN_TAG=VS2008x86
  )
) else (
  echo.
  echo !!! ERROR !!! VS2008 not installed correctly. VS90COMNTOOLS not defined. !!!
  echo.
  set SCRIPT_ERROR=1
  goto :BldEnd
)

@echo.
@echo Set build environment.
@echo.
@if not exist Build\%PLATFORM_PACKAGE%\%TARGET%_%TOOL_CHAIN_TAG% (
  mkdir Build\%PLATFORM_PACKAGE%\%TARGET%_%TOOL_CHAIN_TAG%
)


@REM Set clean build option
@set CLEAN_BUILD_OPTION=-e

@findstr /V "ACTIVE_PLATFORM TARGET TARGET_ARCH TOOL_CHAIN_TAG BUILD_RULE_CONF" Conf\target.txt > Build\%PLATFORM_PACKAGE%\%TARGET%_%TOOL_CHAIN_TAG%\target.txt

@echo ACTIVE_PLATFORM = %PLATFORM_PACKAGE%/PlatformPkgX64.dsc     >> Build\%PLATFORM_PACKAGE%\%TARGET%_%TOOL_CHAIN_TAG%\target.txt
@echo TARGET          = %TARGET%                                  >> Build\%PLATFORM_PACKAGE%\%TARGET%_%TOOL_CHAIN_TAG%\target.txt
@echo TARGET_ARCH     = IA32 X64                                  >> Build\%PLATFORM_PACKAGE%\%TARGET%_%TOOL_CHAIN_TAG%\target.txt
@echo TOOL_CHAIN_TAG  = %TOOL_CHAIN_TAG%                          >> Build\%PLATFORM_PACKAGE%\%TARGET%_%TOOL_CHAIN_TAG%\target.txt
@echo BUILD_RULE_CONF = Conf/build_rule.txt                       >> Build\%PLATFORM_PACKAGE%\%TARGET%_%TOOL_CHAIN_TAG%\target.txt
@move /Y Build\%PLATFORM_PACKAGE%\%TARGET%_%TOOL_CHAIN_TAG%\target.txt Conf



@echo.
@echo Create BiosIdx64.
@echo.
@if not exist Build\%PLATFORM_PACKAGE%\%TARGET%_%TOOL_CHAIN_TAG%\X64 (
  mkdir Build\%PLATFORM_PACKAGE%\%TARGET%_%TOOL_CHAIN_TAG%\X64
)

@if "%TARGET%" == "DEBUG" (
  set BIOS_ID_FILE=BiosIdx64D.env

) else (
  set BIOS_ID_FILE=BiosIdx64R.env

)


GenBiosId.exe -i %PLATFORM_PACKAGE%\%BIOS_ID_FILE% -o Build\%PLATFORM_PACKAGE%\%TARGET%_%TOOL_CHAIN_TAG%\X64\BiosId.bin
@if %ERRORLEVEL% NEQ 0 goto BldFail

@echo off
echo Copy reference code ASL files.


@if /i "/s" == "%1" goto BldSilent
@if /i "/s" == "%2" goto BldSilent
@if /i "/s" == "%3" goto BldSilent
@if /i "/s" == "%4" goto BldSilent
@if /i "/s" == "%5" goto BldSilent

build -n %NUMBER_OF_PROCESSORS%
@if %ERRORLEVEL% NEQ 0 goto BldFail

@If %SCRIPT_ERROR% EQU 1 goto BldFail
@goto BldSuccess

:BldSilent

build -n %NUMBER_OF_PROCESSORS% 1>>EDK2.log 2>&1
@if %ERRORLEVEL% NEQ 0 goto BldFail


@If %SCRIPT_ERROR% EQU 1 goto BldFail

:BldSuccess
@echo off
del Build\%PLATFORM_PACKAGE%\%TARGET%_%TOOL_CHAIN_TAG%\FV\VlvX64.beforeconfig.fd 1>>EDK2.log 2>&1
del Build\%PLATFORM_PACKAGE%\%TARGET%_%TOOL_CHAIN_TAG%\FV\HiiDefaultData.txt 1>>EDK2.log 2>&1
copy /y Build\%PLATFORM_PACKAGE%\%TARGET%_%TOOL_CHAIN_TAG%\FV\Vlv.fd     Build\%PLATFORM_PACKAGE%\%TARGET%_%TOOL_CHAIN_TAG%\FV\VlvX64.beforeconfig.fd 1>>EDK2.log 2>&1

@echo.
@echo Extract setup default value from VFR (Build\%PLATFORM_PACKAGE%\%TARGET%_%TOOL_CHAIN_TAG%\FV\HiiDefaultData.txt)
@echo.
fce read -i Build\%PLATFORM_PACKAGE%\%TARGET%_%TOOL_CHAIN_TAG%\FV\VlvX64.beforeconfig.fd > Build\%PLATFORM_PACKAGE%\%TARGET%_%TOOL_CHAIN_TAG%\FV\HiiDefaultData.txt


@echo Update FD with default Hii value successfully! (both 'Setup' and 'SetupDefault')
@echo.
fce mirror -i Build\%PLATFORM_PACKAGE%\%TARGET%_%TOOL_CHAIN_TAG%\FV\VlvX64.beforeconfig.fd -o Build\%PLATFORM_PACKAGE%\%TARGET%_%TOOL_CHAIN_TAG%\FV\VlvX64.fd Setup SetupDefault 1>>EDK2.log 2>&1

@echo off
del Build\%PLATFORM_PACKAGE%\%TARGET%_%TOOL_CHAIN_TAG%\FV\Vlv.fd
del Vlv2TbltDevicePkg\RomImage\bios.rom
if not exist Build\%PLATFORM_PACKAGE%\%TARGET%_%TOOL_CHAIN_TAG%\ROM goto Gen8MImage
del /q /f Build\%PLATFORM_PACKAGE%\%TARGET%_%TOOL_CHAIN_TAG%\ROM\*
rd /Q Build\%PLATFORM_PACKAGE%\%TARGET%_%TOOL_CHAIN_TAG%\ROM
:Gen8MImage

copy /b Build\%PLATFORM_PACKAGE%\%TARGET%_%TOOL_CHAIN_TAG%\FV\VlvX64.fd Vlv2TbltDevicePkg\RomImage\bios.rom

pushd Vlv2TbltDevicePkg\RomImage\
@if "%TARGET%" == "DEBUG" (
    call signbiosX64_debug.bat
) else (
    call signbiosX64_release.bat
)

call ftoolbuild.bat
call ftoolbuild_sec_enable.bat
popd

@echo off
mkdir Build\%PLATFORM_PACKAGE%\%TARGET%_%TOOL_CHAIN_TAG%\ROM
move Vlv2TbltDevicePkg\RomImage\SPI_Image\* Build\%PLATFORM_PACKAGE%\%TARGET%_%TOOL_CHAIN_TAG%\ROM\ > NUL

call GenBIOS.bat X64

@echo on

@echo TARGET:               %TARGET%
@echo TOOL_CHAIN_TAG:       %TOOL_CHAIN_TAG%
@echo BIOS location:        Build\%PLATFORM_PACKAGE%\%TARGET%_%TOOL_CHAIN_TAG%\FV
@echo SPI Images location:  Build\%PLATFORM_PACKAGE%\%TARGET%_%TOOL_CHAIN_TAG%\ROM
@echo.
@echo The EDKII BIOS build has successfully completed!
@echo.
@goto BldEnd

:BldFail
@echo.
@echo The EDKII BIOS Build has failed!
@echo.
exit /b 1

:BldEnd
@time /t
exit /b 0
