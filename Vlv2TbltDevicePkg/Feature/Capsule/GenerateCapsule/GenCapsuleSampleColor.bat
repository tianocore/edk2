@echo off
setlocal

set COLOR=%1

set FMP_CAPSULE_VENDOR=Intel
set FMP_CAPSULE_GUID=%2
set FMP_CAPSULE_FILE=%COLOR%.cap
set FMP_CAPSULE_VERSION=0x00000010
set FMP_CAPSULE_STRING=0.0.0.16
set FMP_CAPSULE_NAME="%COLOR% Progress Bar %FMP_CAPSULE_STRING%"
set FMP_CAPSULE_LSV=0x00000000
set FMP_CAPSULE_KEY=SAMPLE_DEVELOPMENT.pfx
set FMP_CAPSULE_PAYLOAD=Payload.bin
set WINDOWS_CAPSULE_KEY=SAMPLE_DEVELOPMENT.pfx

echo "%COLOR% Progress Bar" > %FMP_CAPSULE_PAYLOAD%

if not exist "%FMP_CAPSULE_PAYLOAD%" exit

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

  if exist "%WINDOWS_CAPSULE_KEY%" (
    CreateWindowsCapsule.py ^
      UEFI ^
      %FMP_CAPSULE_STRING% ^
      %FMP_CAPSULE_GUID% ^
      %FMP_CAPSULE_FILE% ^
      %FMP_CAPSULE_VERSION% ^
      %FMP_CAPSULE_VENDOR% ^
      %FMP_CAPSULE_VENDOR% ^
      %FMP_CAPSULE_NAME% %WINDOWS_CAPSULE_KEY%

    xcopy /s/e/v/i/y WindowsCapsule %WORKSPACE%\Build\Vlv2TbltDevicePkg\Capsules\SampleDevelopment\%COLOR%WindowsCapsule
    rmdir /s /q WindowsCapsule
  )
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
    --signing-tool-path=c:\OpenSSL-Win32\bin ^
    --signer-private-cert=NewCert.pem ^
    --other-public-cert=NewSub.pub.pem ^
    --trusted-public-cert=NewRoot.pub.pem ^
    -o %FMP_CAPSULE_FILE% ^
    %FMP_CAPSULE_PAYLOAD%

  copy %FMP_CAPSULE_FILE% %WORKSPACE%\Build\Vlv2TbltDevicePkg\Capsules\NewCert

  if exist "%WINDOWS_CAPSULE_KEY%" (
    CreateWindowsCapsule.py ^
      UEFI ^
      %FMP_CAPSULE_STRING% ^
      %FMP_CAPSULE_GUID% ^
      %FMP_CAPSULE_FILE% ^
      %FMP_CAPSULE_VERSION% ^
      %FMP_CAPSULE_VENDOR% ^
      %FMP_CAPSULE_VENDOR% ^
      %FMP_CAPSULE_NAME% %WINDOWS_CAPSULE_KEY%

    xcopy /s/e/v/i/y WindowsCapsule %WORKSPACE%\Build\Vlv2TbltDevicePkg\Capsules\NewCert\%COLOR%WindowsCapsule
    rmdir /s /q WindowsCapsule
  )
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
  --signing-tool-path=c:\OpenSSL-Win32\bin ^
  --signer-private-cert=%WORKSPACE%\edk2\BaseTools\Source\Python\Pkcs7Sign\TestCert.pem ^
  --other-public-cert=%WORKSPACE%\edk2\BaseTools\Source\Python\Pkcs7Sign\TestSub.pub.pem ^
  --trusted-public-cert=%WORKSPACE%\edk2\BaseTools\Source\Python\Pkcs7Sign\TestRoot.pub.pem ^
  -o %FMP_CAPSULE_FILE% ^
  %FMP_CAPSULE_PAYLOAD%

copy %FMP_CAPSULE_FILE% %WORKSPACE%\Build\Vlv2TbltDevicePkg\Capsules\TestCert

if exist "%WINDOWS_CAPSULE_KEY%" (
  CreateWindowsCapsule.py ^
    UEFI ^
    %FMP_CAPSULE_STRING% ^
    %FMP_CAPSULE_GUID% ^
    %FMP_CAPSULE_FILE% ^
    %FMP_CAPSULE_VERSION% ^
    %FMP_CAPSULE_VENDOR% ^
    %FMP_CAPSULE_VENDOR% ^
    %FMP_CAPSULE_NAME% %WINDOWS_CAPSULE_KEY%

  xcopy /s/e/v/i/y WindowsCapsule %WORKSPACE%\Build\Vlv2TbltDevicePkg\Capsules\TestCert\%COLOR%WindowsCapsule
  rmdir /s /q WindowsCapsule
)

erase %FMP_CAPSULE_FILE%

erase %FMP_CAPSULE_PAYLOAD%
