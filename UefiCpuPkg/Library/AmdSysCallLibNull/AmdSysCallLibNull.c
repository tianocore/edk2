/** @file
  Null Library for AmdSysCallLib

Copyright (C) 2025 Advanced Micro Devices, Inc. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Library/AmdSysCallLib.h>

/**
 Check if high privilege instruction need go through Syscall.


 @param  NONE

 @return TRUE  Syscall required
 @return FALSE Syscall not required

**/
BOOLEAN
EFIAPI
NeedSysCall (
  VOID
  )
{
  return FALSE;
}

/**
 Use SysCall to access resource.

  @param[in]  CallIndex  The index of the resource. The AMD_SMM_SYS_CALL resource.
  @param[in]  Arg1       Value specific to SysCall. This is usually the Logical CPU number.
  @param[in]  Arg2       Value specific to SysCall. This is usually the resource address.
  @param[in]  Arg3       Value specific to SysCall. This is usually resource size.

  @return Value of the resource. System may hang or reset if resource value is protected.
**/
UINT64
EFIAPI
SysCall (
  UINTN  CallIndex,
  UINTN  Arg1,
  UINTN  Arg2,
  UINTN  Arg3
  )
{
  return 0;
}
