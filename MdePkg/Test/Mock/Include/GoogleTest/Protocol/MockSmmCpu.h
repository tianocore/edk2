/** @file MockSmmCpu.h
  This file declares a mock of Smm CPU Protocol.

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#pragma once

#include <Library/GoogleTestLib.h>
#include <Library/FunctionMockLib.h>

extern "C" {
  #include <Uefi.h>
  #include <Protocol/SmmCpu.h>
}

struct MockSmmCpu {
  MOCK_INTERFACE_DECLARATION (MockSmmCpu);

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    ReadSaveState,
    (
     IN CONST EFI_MM_CPU_PROTOCOL    *This,
     IN UINTN                        Width,
     IN EFI_MM_SAVE_STATE_REGISTER   Register,
     IN UINTN                        CpuIndex,
     OUT VOID                        *Buffer
    )
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    WriteSaveState,
    (
     IN CONST EFI_MM_CPU_PROTOCOL    *This,
     IN UINTN                        Width,
     IN EFI_MM_SAVE_STATE_REGISTER   Register,
     IN UINTN                        CpuIndex,
     IN CONST VOID                   *Buffer
    )
    );
};

MOCK_INTERFACE_DEFINITION (MockSmmCpu);
MOCK_FUNCTION_DEFINITION (MockSmmCpu, ReadSaveState, 5, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockSmmCpu, WriteSaveState, 5, EFIAPI);

static EFI_SMM_CPU_PROTOCOL  SMMCPU_PROTOCOL_INSTANCE = {
  ReadSaveState,
  WriteSaveState
};

extern "C" {
  EFI_SMM_CPU_PROTOCOL  *gSmmCpuProtocol = &SMMCPU_PROTOCOL_INSTANCE;
}
