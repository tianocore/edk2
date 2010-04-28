/*++

Copyright (c) 1999 - 2006, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.


Module Name:

  SmmAccess.h

Abstract:

  This file defines SMM SMRAM Access abstraction protocol defined 
  by the SMM CIS.

--*/

#ifndef _SMM_ACCESS_H_
#define _SMM_ACCESS_H_

#include EFI_GUID_DEFINITION (SmramMemoryReserve)

EFI_FORWARD_DECLARATION (EFI_SMM_ACCESS_PROTOCOL);

#define EFI_SMM_ACCESS_PROTOCOL_GUID \
  { \
    0x3792095a, 0xe309, 0x4c1e, {0xaa, 0x01, 0x85, 0xf5, 0x65, 0x5a, 0x17, 0xf1} \
  }

//
// SMM Access specification Data Structures
//
typedef
EFI_STATUS
(EFIAPI *EFI_SMM_OPEN) (
  IN EFI_SMM_ACCESS_PROTOCOL         * This,
  UINTN                              DescriptorIndex
  );

typedef
EFI_STATUS
(EFIAPI *EFI_SMM_CLOSE) (
  IN EFI_SMM_ACCESS_PROTOCOL          * This,
  UINTN                               DescriptorIndex
  );

typedef
EFI_STATUS
(EFIAPI *EFI_SMM_LOCK) (
  IN EFI_SMM_ACCESS_PROTOCOL         * This,
  UINTN                              DescriptorIndex
  );

typedef
EFI_STATUS
(EFIAPI *EFI_SMM_CAPABILITIES) (
  IN EFI_SMM_ACCESS_PROTOCOL             * This,
  IN OUT UINTN                           *SmramMapSize,
  IN OUT EFI_SMRAM_DESCRIPTOR            * SmramMap
  );

struct _EFI_SMM_ACCESS_PROTOCOL {
  EFI_SMM_OPEN          Open;
  EFI_SMM_CLOSE         Close;
  EFI_SMM_LOCK          Lock;
  EFI_SMM_CAPABILITIES  GetCapabilities;
  BOOLEAN               LockState;
  BOOLEAN               OpenState;
};

extern EFI_GUID gEfiSmmAccessProtocolGuid;

#endif
