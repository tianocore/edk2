@REM @file
@REM   Windows batch file to generate UEFI capsules for system firmware
@REM
@REM Copyright (c) 2018, Intel Corporation. All rights reserved.<BR>
@REM
@REM SPDX-License-Identifier: BSD-2-Clause-Patent
@REM

@echo off
setlocal

set FMP_CAPSULE_VENDOR=Intel
set FMP_CAPSULE_GUID=4096267b-da0a-42eb-b5eb-fef31d207cb4
set FMP_CAPSULE_BASE_NAME=MinnowMax
set FMP_CAPSULE_FILE=%FMP_CAPSULE_BASE_NAME%.cap
set FMP_CAPSULE_VERSION=0x0000000C
set FMP_CAPSULE_VERSION_DECIMAL=12
set FMP_CAPSULE_STRING=0.0.0.12
set FMP_CAPSULE_NAME="Intel %FMP_CAPSULE_BASE_NAME% DEBUG UEFI %FMP_CAPSULE_STRING%"
set FMP_CAPSULE_LSV=0x00000000
set FMP_CAPSULE_KEY=SAMPLE_DEVELOPMENT.pfx
set FMP_CAPSULE_PAYLOAD=%WORKSPACE%\Build\Vlv2TbltDevicePkg\DEBUG_VS2015x86\FV\Vlv.ROM
set WINDOWS_CAPSULE_KEY=SAMPLE_DEVELOPMENT.pfx

if not exist "%FMP_CAPSULE_PAYLOAD%" exit /b

if exist "%FMP_CAPSULE_KEY%" (
  REM
  REM Sign capsule using signtool
  REM
  call GenerateCapsule ^
    --encode ^
    -v ^
    --guid %FMP_CAPSULE_GUID% ^
    --fw-version %FMP_CAPSULE_VERSION% ^
    --lsv %FMP_CAPSULE_LSV% ^
    --capflag PersistAcrossReset ^
    --capflag InitiateReset ^
    --signing-tool-path="c:\Program Files (x86)\Windows Kits\8.1\bin\x86" ^
    --pfx-file %FMP_CAPSULE_KEY% ^
    -o %FMP_CAPSULE_FILE% ^
    %FMP_CAPSULE_PAYLOAD%

  copy %FMP_CAPSULE_FILE% %WORKSPACE%\Build\Vlv2TbltDevicePkg\Capsules\SampleDevelopment

  copy %FMP_CAPSULE_FILE% firmware.bin
  copy template.metainfo.xml firmware.metainfo.xml
  powershell -Command "(gc firmware.metainfo.xml) -replace 'FMP_CAPSULE_GUID', '%FMP_CAPSULE_GUID%' | Out-File firmware.metainfo.xml -encoding ASCII"
  powershell -Command "(gc firmware.metainfo.xml) -replace 'FMP_CAPSULE_BASE_NAME', '%FMP_CAPSULE_BASE_NAME%' | Out-File firmware.metainfo.xml -encoding ASCII"
  powershell -Command "(gc firmware.metainfo.xml) -replace 'FMP_CAPSULE_VERSION_DECIMAL', '%FMP_CAPSULE_VERSION_DECIMAL%' | Out-File firmware.metainfo.xml -encoding ASCII"
  powershell -Command "(gc firmware.metainfo.xml) -replace 'FMP_CAPSULE_STRING', '%FMP_CAPSULE_STRING%' | Out-File firmware.metainfo.xml -encoding ASCII"
  powershell -Command "(gc firmware.metainfo.xml) -replace 'FMP_CAPSULE_DATE', '%date%' | Out-File firmware.metainfo.xml -encoding ASCII"
  makecab /f Lvfs.ddf
  copy firmware.cab %WORKSPACE%\Build\Vlv2TbltDevicePkg\Capsules\TestCert\%FMP_CAPSULE_BASE_NAME%-%FMP_CAPSULE_STRING%.cab

  erase firmware.cab
  erase setup.inf
  erase setup.rpt

  erase firmware.metainfo.xml
  erase firmware.bin
  erase %FMP_CAPSULE_FILE%
)

if exist "NewCert.pem" (
  REM
  REM Sign capsule using OpenSSL with a new certificate
  REM
  call GenerateCapsule ^
    --encode ^
    -v ^
    --guid %FMP_CAPSULE_GUID% ^
    --fw-version %FMP_CAPSULE_VERSION% ^
    --lsv %FMP_CAPSULE_LSV% ^
    --capflag PersistAcrossReset ^
    --capflag InitiateReset ^
    --signer-private-cert=NewCert.pem ^
    --other-public-cert=NewSub.pub.pem ^
    --trusted-public-cert=NewRoot.pub.pem ^
    -o %FMP_CAPSULE_FILE% ^
    %FMP_CAPSULE_PAYLOAD%

  copy %FMP_CAPSULE_FILE% %WORKSPACE%\Build\Vlv2TbltDevicePkg\Capsules\NewCert

  copy %FMP_CAPSULE_FILE% firmware.bin
  copy template.metainfo.xml firmware.metainfo.xml
  powershell -Command "(gc firmware.metainfo.xml) -replace 'FMP_CAPSULE_GUID', '%FMP_CAPSULE_GUID%' | Out-File firmware.metainfo.xml -encoding ASCII"
  powershell -Command "(gc firmware.metainfo.xml) -replace 'FMP_CAPSULE_BASE_NAME', '%FMP_CAPSULE_BASE_NAME%' | Out-File firmware.metainfo.xml -encoding ASCII"
  powershell -Command "(gc firmware.metainfo.xml) -replace 'FMP_CAPSULE_VERSION_DECIMAL', '%FMP_CAPSULE_VERSION_DECIMAL%' | Out-File firmware.metainfo.xml -encoding ASCII"
  powershell -Command "(gc firmware.metainfo.xml) -replace 'FMP_CAPSULE_STRING', '%FMP_CAPSULE_STRING%' | Out-File firmware.metainfo.xml -encoding ASCII"
  powershell -Command "(gc firmware.metainfo.xml) -replace 'FMP_CAPSULE_DATE', '%date%' | Out-File firmware.metainfo.xml -encoding ASCII"
  makecab /f Lvfs.ddf
  copy firmware.cab %WORKSPACE%\Build\Vlv2TbltDevicePkg\Capsules\TestCert\%FMP_CAPSULE_BASE_NAME%-%FMP_CAPSULE_STRING%.cab

  erase firmware.cab
  erase setup.inf
  erase setup.rpt

  erase firmware.metainfo.xml
  erase firmware.bin
  erase %FMP_CAPSULE_FILE%
)

REM
REM Sign capsule using OpenSSL with EDK II Test Certificate
REM
call GenerateCapsule ^
  --encode ^
  -v ^
  --guid %FMP_CAPSULE_GUID% ^
  --fw-version %FMP_CAPSULE_VERSION% ^
  --lsv %FMP_CAPSULE_LSV% ^
  --capflag PersistAcrossReset ^
  --capflag InitiateReset ^
  --signer-private-cert=%WORKSPACE%\edk2\BaseTools\Source\Python\Pkcs7Sign\TestCert.pem ^
  --other-public-cert=%WORKSPACE%\edk2\BaseTools\Source\Python\Pkcs7Sign\TestSub.pub.pem ^
  --trusted-public-cert=%WORKSPACE%\edk2\BaseTools\Source\Python\Pkcs7Sign\TestRoot.pub.pem ^
  -o %FMP_CAPSULE_FILE% ^
  %FMP_CAPSULE_PAYLOAD%

copy %FMP_CAPSULE_FILE% %WORKSPACE%\Build\Vlv2TbltDevicePkg\Capsules\TestCert

copy %FMP_CAPSULE_FILE% firmware.bin
copy template.metainfo.xml firmware.metainfo.xml
powershell -Command "(gc firmware.metainfo.xml) -replace 'FMP_CAPSULE_GUID', '%FMP_CAPSULE_GUID%' | Out-File firmware.metainfo.xml -encoding ASCII"
powershell -Command "(gc firmware.metainfo.xml) -replace 'FMP_CAPSULE_BASE_NAME', '%FMP_CAPSULE_BASE_NAME%' | Out-File firmware.metainfo.xml -encoding ASCII"
powershell -Command "(gc firmware.metainfo.xml) -replace 'FMP_CAPSULE_VERSION_DECIMAL', '%FMP_CAPSULE_VERSION_DECIMAL%' | Out-File firmware.metainfo.xml -encoding ASCII"
powershell -Command "(gc firmware.metainfo.xml) -replace 'FMP_CAPSULE_STRING', '%FMP_CAPSULE_STRING%' | Out-File firmware.metainfo.xml -encoding ASCII"
powershell -Command "(gc firmware.metainfo.xml) -replace 'FMP_CAPSULE_DATE', '%date%' | Out-File firmware.metainfo.xml -encoding ASCII"
makecab /f Lvfs.ddf
copy firmware.cab %WORKSPACE%\Build\Vlv2TbltDevicePkg\Capsules\TestCert\%FMP_CAPSULE_BASE_NAME%-%FMP_CAPSULE_STRING%.cab

erase firmware.cab
erase setup.inf
erase setup.rpt

erase firmware.metainfo.xml
erase firmware.bin
erase %FMP_CAPSULE_FILE%
