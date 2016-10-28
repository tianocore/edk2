@REM @file
@REM   This stand-alone program is typically called by the toolsetup.bat file,
@REM   however it may be executed directly from the BaseTools project folder
@REM   if the file is not executed within a WORKSPACE\BaseTools folder.
@REM
@REM Copyright (c) 2016, Intel Corporation. All rights reserved.<BR>
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

if defined VS71COMNTOOLS (
  if not defined VS2003_PREFIX (
    set "VS2003_PREFIX=%VS71COMNTOOLS:~0,-14%"
  )
)

if defined VS80COMNTOOLS (
  if not defined VS2005_PREFIX (
    set "VS2005_PREFIX=%VS80COMNTOOLS:~0,-14%"
  )
)

if defined VS90COMNTOOLS (
  if not defined VS2008_PREFIX (
    set "VS2008_PREFIX=%VS90COMNTOOLS:~0,-14%"
  )
  if not defined WINSDK_PREFIX (
    set "WINSDK_PREFIX=c:\Program Files\Microsoft SDKs\Windows\v6.0A\bin\"
  )
  if not defined WINSDKx86_PREFIX (
    set "WINSDKx86_PREFIX=c:\Program Files (x86)\Microsoft SDKs\Windows\v6.0A\bin\"
  )
)

if defined VS100COMNTOOLS (
  if not defined VS2010_PREFIX (
    set "VS2010_PREFIX=%VS100COMNTOOLS:~0,-14%"
  )
  if not defined WINSDK7_PREFIX (
    set "WINSDK7_PREFIX=c:\Program Files\Microsoft SDKs\Windows\v7.0A\Bin\"
  )
  if not defined WINSDK7x86_PREFIX (
    set "WINSDK7x86_PREFIX=c:\Program Files (x86)\Microsoft SDKs\Windows\v7.0A\Bin\"
  )
)

if defined VS110COMNTOOLS (
  if not defined VS2012_PREFIX (
    set "VS2012_PREFIX=%VS110COMNTOOLS:~0,-14%"
  )
  if not defined WINSDK71_PREFIX (
    set "WINSDK71_PREFIX=c:\Program Files\Microsoft SDKs\Windows\v7.1A\Bin\"
  )
  if not defined WINSDK71x86_PREFIX (
    set "WINSDK71x86_PREFIX=c:\Program Files (x86)\Microsoft SDKs\Windows\v7.1A\Bin\"
  )
)

if defined VS120COMNTOOLS (
  if not defined VS2013_PREFIX (
    set "VS2013_PREFIX=%VS120COMNTOOLS:~0,-14%"
  )
  if not defined WINSDK8_PREFIX (
    set "WINSDK8_PREFIX=c:\Program Files\Windows Kits\8.0\bin\"
  )
  if not defined WINSDK8x86_PREFIX (
    set "WINSDK8x86_PREFIX=c:\Program Files (x86)\Windows Kits\8.0\bin\"
  )
)

if defined VS140COMNTOOLS (
  if not defined VS2015_PREFIX (
    set "VS2015_PREFIX=%VS140COMNTOOLS:~0,-14%"
  )
  if not defined WINSDK81_PREFIX (
    set "WINSDK81_PREFIX=c:\Program Files\Windows Kits\8.1\bin\"
  )
  if not defined WINSDK81x86_PREFIX (
    set "WINSDK81x86_PREFIX=c:\Program Files (x86)\Windows Kits\8.1\bin\"
  )
)

if not defined WINDDK3790_PREFIX (
  set WINDDK3790_PREFIX=C:\WINDDK\3790.1830\bin\
)

if not defined IASL_PREFIX (
  set IASL_PREFIX=C:\ASL\
)

popd
