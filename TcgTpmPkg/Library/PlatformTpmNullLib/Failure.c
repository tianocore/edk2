/** @file
  This module provides the platform specific entry and fail processing. The
  PlatformTpmLibFail() function is used to call to ExecuteCommand() in the TPM code.
  This function does whatever processing is necessary to set up the platform
  in anticipation of the call to the TPM including settup for error processing.

  The PlatformTpmLib() function is called when there is a failure in the TPM.
  The TPM code will have set the flag to indicate that the TPM is in failure mode.
  This call will then recursively call ExecuteCommand in order to build the
  failure mode response. When ExecuteCommand() returns to PlatformTpmLib(), the
  platform will do some platform specific operation to return to the environment in
  which the TPM is executing.

  To see the plat_XXX interfaces in TPM reference library, see:
    - https://github.com/TrustedComputingGroup/TPM/tree/main/TPMCmd/Platform/src

  Copyright (c) 2025, Arm Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#include <Library/BaseLib.h>
#include <Library/PlatformTpmLib.h>

STATIC BOOLEAN      InFailureMode;
STATIC CONST CHAR8  *LastFailedFuncName;
STATIC UINT32       LastFailedLineNumber;
STATIC UINT64       LastFailedLocation;
STATIC UINT32       LastFailedCode;

/**
  _plat_internal_resetFailureData()

  Reset platform specific failure data.

**/
VOID
EFIAPI
PlatformTpmLibInternalResetFailureData (
  VOID
  )
{
  InFailureMode        = FALSE;
  LastFailedFuncName   = NULL;
  LastFailedLineNumber = 0;
  LastFailedLocation   = 0;
  LastFailedCode       = 0;
}

/**
  _plat__InFailureMode()

  Check whether platform is in the failure mode.

  @return whether TPM is in failure mode or not

**/
BOOLEAN
EFIAPI
PlatformTpmLibInFailureMode (
  VOID
  )
{
  return InFailureMode;
}

/**
  _plat__Fail()

  This is the platform depended failure exit for the TPM.

  @param[in]  Function       Function name where failure happens.
  @param[in]  Line           line number where failure happens.
  @param[in]  LocationCode   Location code where failure happens.
  @param[in]  FailureCode    Fail reson.

**/
VOID
EFIAPI
PlatformTpmLibFail (
  IN CONST CHAR8  *Function,
  IN INT32        Line,
  IN UINT64       LocationCode,
  IN INT32        FailureCode
  )
{
  InFailureMode        = TRUE;
  LastFailedFuncName   = Function;
  LastFailedLineNumber = Line;
  LastFailedLocation   = LocationCode;
  LastFailedCode       = FailureCode;

  CpuDeadLoop ();
}

/**
  _plat__GetFailureCode()

  Get last failure code.

  @return Last failure code.

**/
UINT32
EFIAPI
PlatformTpmLibGetFailureCode (
  VOID
  )
{
  return LastFailedCode;
}

/**
  _plat__GetFailureLocation()

  Get last failure location.

  @return Last failure location code.

**/
UINT64
EFIAPI
PlatformTpmLibGetFailureLocation (
  VOID
  )
{
  return LastFailedLocation;
}

/**
  _plat__GetFailureFunctionName()

  Get last failure location.

  @return Last function name where failed.

**/
CONST CHAR8 *
EFIAPI
PlatformTpmLibGetFailureFunctionName (
  VOID
  )
{
  return LastFailedFuncName;
}

/**
  _plat__GetFailureLine()

  Get last failure line.

  @return Last line number where failed.

**/
UINT32
EFIAPI
PlatformTpmLibGetFailureLine (
  VOID
  )
{
  return LastFailedLineNumber;
}
