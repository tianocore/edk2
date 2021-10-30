/** @file

  Copyright (c) 2020 - 2021, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef VMTD_EXIT_HANDLER_H_
#define VMTD_EXIT_HANDLER_H_

#include <Base.h>
#include <Uefi.h>

/**
  This function enable the TD guest to request the VMM to emulate CPUID
  operation, especially for non-architectural, CPUID leaves.

  @param[in]  Eax        Main leaf of the CPUID
  @param[in]  Ecx        Sub-leaf of the CPUID
  @param[out] Results    Returned result of CPUID operation

  @return EFI_SUCCESS
**/
EFI_STATUS
EFIAPI
TdVmCallCpuid (
  IN UINT64  Eax,
  IN UINT64  Ecx,
  OUT VOID   *Results
  );

#endif
