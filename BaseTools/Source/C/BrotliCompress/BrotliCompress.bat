@echo off
@setlocal

set LVL=--quality 9

:Begin
if "%1"=="" goto End

if "%1"=="-d" (
  set ARGS=%ARGS% --decompress
  shift
  goto Begin
)

if "%1"=="-e" (
  shift
  goto Begin
)

if "%1"=="-g" (
  set ARGS=%ARGS% --gap %2
  shift
  shift
  goto Begin
)

if "%1"=="-l" (
  set LVL=--quality %2
  shift
  shift
  goto Begin
)

if "%1"=="-o" (
  set ARGS=%ARGS% --output %2
  set INTMP=%2
  shift
  shift
  goto Begin
)

set ARGS=%ARGS% --input %1
shift
goto Begin

:End
Brotli %ARGS% %LVL%
@echo on