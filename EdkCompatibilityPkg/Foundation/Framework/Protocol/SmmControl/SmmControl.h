/*++

Copyright (c) 1999 - 2002, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.


Module Name:

  SmmControl.h

Abstract:

  This file defines SMM Control abstraction protocol defined 
  by the SMM CIS.

--*/

#ifndef _SMM_CONTROL_H_
#define _SMM_CONTROL_H_

EFI_FORWARD_DECLARATION (EFI_SMM_CONTROL_PROTOCOL);

#define EFI_SMM_CONTROL_PROTOCOL_GUID \
  { \
    0x8d12e231, 0xc667, 0x4fd1, {0x98, 0xf2, 0x24, 0x49, 0xa7, 0xe7, 0xb2, 0xe5} \
  }

//
// SMM Control specification constant and types
//
// typedef EFI_SMM_PERIOD UINTN
//
// SMM Access specification Data Structures
//
typedef struct {
  UINT8 SmiTriggerRegister;
  UINT8 SmiDataRegister;
} EFI_SMM_CONTROL_REGISTER;

typedef
EFI_STATUS
(EFIAPI *EFI_SMM_ACTIVATE) (
  IN EFI_SMM_CONTROL_PROTOCOL                             * This,
  IN OUT INT8                                             *ArgumentBuffer OPTIONAL,
  IN OUT UINTN                                            *ArgumentBufferSize OPTIONAL,
  IN BOOLEAN                                              Periodic OPTIONAL,
  IN UINTN                                                ActivationInterval OPTIONAL
  );

typedef
EFI_STATUS
(EFIAPI *EFI_SMM_DEACTIVATE) (
  IN EFI_SMM_CONTROL_PROTOCOL                   * This,
  IN BOOLEAN                                    Periodic OPTIONAL
  );

typedef
EFI_STATUS
(EFIAPI *EFI_SMM_GET_REGISTER_INFO) (
  IN EFI_SMM_CONTROL_PROTOCOL           * This,
  IN OUT EFI_SMM_CONTROL_REGISTER       * SmiRegister
  );

struct _EFI_SMM_CONTROL_PROTOCOL {
  EFI_SMM_ACTIVATE          Trigger;
  EFI_SMM_DEACTIVATE        Clear;
  EFI_SMM_GET_REGISTER_INFO GetRegisterInfo;
  UINTN                     MinimumTriggerPeriod;
};

extern EFI_GUID gEfiSmmControlProtocolGuid;

#endif
