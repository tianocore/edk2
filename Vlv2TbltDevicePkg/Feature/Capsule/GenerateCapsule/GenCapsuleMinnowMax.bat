@echo off
setlocal

set FMP_CAPSULE_VENDOR=Intel
set FMP_CAPSULE_GUID=4096267B-DA0A-42EB-B5EB-FEF31D207CB4
set FMP_CAPSULE_FILE=MinnowMax.cap
set FMP_CAPSULE_VERSION=0x00000009
set FMP_CAPSULE_STRING=0.0.0.9
set FMP_CAPSULE_NAME="Intel MinnowMax DEBUG UEFI %FMP_CAPSULE_STRING%"
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

    xcopy /s/e/v/i/y WindowsCapsule %WORKSPACE%\Build\Vlv2TbltDevicePkg\Capsules\SampleDevelopment\MinnowMaxWindowsCapsule
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

    xcopy /s/e/v/i/y WindowsCapsule %WORKSPACE%\Build\Vlv2TbltDevicePkg\Capsules\NewCert\MinnowMaxWindowsCapsule
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

  xcopy /s/e/v/i/y WindowsCapsule %WORKSPACE%\Build\Vlv2TbltDevicePkg\Capsules\TestCert\MinnowMaxWindowsCapsule
  rmdir /s /q WindowsCapsule
)

erase %FMP_CAPSULE_FILE%
