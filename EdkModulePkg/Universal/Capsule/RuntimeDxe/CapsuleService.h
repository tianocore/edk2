/*++

Copyright (c) 2006, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

  CapsuleService.h

Abstract:

  Capsule Runtime Service

--*/

#ifndef  _CAPSULE_RUNTIME_H_
#define  _CAPSULE_RUNTIME_H_

#include <Common/CapsuleName.h>

extern EFI_GUID gEfiCapsuleGuid;

EFI_STATUS
EFIAPI
UpdateCapsule(
  IN UEFI_CAPSULE_HEADER    **CapsuleHeaderArray,
  IN UINTN                   CapsuleCount,
  IN EFI_PHYSICAL_ADDRESS    ScatterGatherList OPTIONAL
  );

EFI_STATUS
EFIAPI
QueryCapsuleCapabilities(
  IN  UEFI_CAPSULE_HEADER  **CapsuleHeaderArray,
  IN  UINTN                CapsuleCount,
  OUT UINT64               *MaxiumCapsuleSize,
  OUT EFI_RESET_TYPE       *ResetType
  );

#endif

