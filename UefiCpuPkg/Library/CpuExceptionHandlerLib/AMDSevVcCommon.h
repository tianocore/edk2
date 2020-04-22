/** @file
  Common header file for SEV-ES #VC Exception Handler Support.

  Copyright (c) 2020, Advanced Micro Devices, Inc. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _AMD_SEV_VC_COMMON_H_
#define _AMD_SEV_VC_COMMON_H_

#include <Protocol/DebugSupport.h>
#include <Register/Amd/Ghcb.h>

/**
  #VC exception handling routine.

  Called by the UefiCpuPkg exception handling support when a #VC is encountered.

  @param[in, out] Context  Pointer to EFI_SYSTEM_CONTEXT.

  @retval 0                Exception handled
  @retval Others           New exception value to propagate

**/
UINTN
DoVcException (
  IN OUT EFI_SYSTEM_CONTEXT  Context
  );

/**
  Common #VC exception handling routine.

  Used to bridge different phases of UEFI execution.

  @param[in, out] Ghcb     Pointer to the Guest-Hypervisor Communication Block
  @param[in, out] Context  Pointer to EFI_SYSTEM_CONTEXT.

  @retval 0                Exception handled
  @retval Others           New exception value to propagate

**/
UINTN
DoVcCommon (
  IN OUT GHCB                *Ghcb,
  IN OUT EFI_SYSTEM_CONTEXT  Context
  );

#endif
