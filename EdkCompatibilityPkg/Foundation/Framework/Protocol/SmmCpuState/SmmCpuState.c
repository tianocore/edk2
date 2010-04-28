/*++

Copyright (c) 2005, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.


Module Name:

    SmmCpuState.c

Abstract:

    Protocol definition for EFI_SMM_CPU_SAVE_STATE protocol

Revision History

--*/

#include "Tiano.h"
#include EFI_PROTOCOL_DEFINITION (SmmCpuState)

EFI_GUID gEfiSmmCpuSaveStateProtocolGuid = EFI_SMM_CPU_SAVE_STATE_PROTOCOL_GUID;

EFI_GUID_STRING (&gEfiSmmCpuSaveStateProtocolGuid, "SMM CPU Save State Protocol", "SMM CPU Save State Protocol");
