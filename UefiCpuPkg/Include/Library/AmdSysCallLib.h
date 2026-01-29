/** @file
  Provides function interface to perform syscall.

Copyright (C) Microsoft Corporation.
Copyright (C) 2025 Advanced Micro Devices, Inc. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef AMD_SYS_CALL_LIB_H_
#define AMD_SYS_CALL_LIB_H_

#include <Uefi.h>

typedef enum {
  SMM_SC_RDMSR          = 0,
  SMM_SC_WRMSR          = 1,
  SMM_SC_CLI            = 2,
  SMM_SC_IO_READ        = 3,
  SMM_SC_IO_WRITE       = 4,
  SMM_SC_WBINVD         = 5,
  SMM_SC_HLT            = 6,
  SMM_SC_SVST_READ      = 7,
  SMM_SC_IHV_SUPV_READ  = 8,
  SMM_SC_IHV_SUPV_WRITE = 9,
} AMD_SMM_SYS_CALL;

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
  );

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
  );

#endif
