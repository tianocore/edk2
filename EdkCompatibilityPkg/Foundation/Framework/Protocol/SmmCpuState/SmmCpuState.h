/*++

Copyright (c) 2005, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.


Module Name:

    SmmCpuState.h

Abstract:

    Protocol definition for EFI_SMM_CPU_SAVE_STATE protocol

Revision History

--*/

#ifndef _SMMSAVESTATE_H_
#define _SMMSAVESTATE_H_

#include "CpuSaveState.h"

//
// Global ID for the Sx SMI Protocol
//
// {21F302AD-6E94-471b-84BC-B14800403A1D}
#define EFI_SMM_CPU_SAVE_STATE_PROTOCOL_GUID  \
  { 0x21f302ad, 0x6e94, 0x471b, {0x84, 0xbc, 0xb1, 0x48, 0x0, 0x40, 0x3a, 0x1d} }

typedef struct _EFI_SMM_CPU_SAVE_STATE_PROTOCOL  EFI_SMM_CPU_SAVE_STATE_PROTOCOL;

struct _EFI_SMM_CPU_SAVE_STATE_PROTOCOL {
  EFI_SMM_CPU_STATE                 **CpuSaveState;
};

extern EFI_GUID gEfiSmmCpuSaveStateProtocolGuid;

#endif  // _SMMSAVESTATE_H_
