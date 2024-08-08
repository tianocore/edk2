/** @file MockSmmCpu.cpp
  Google Test mock for SmmCpu Protocol

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <GoogleTest/Protocol/MockSmmCpu.h>

MOCK_INTERFACE_DEFINITION (MockSmmCpu);
MOCK_FUNCTION_DEFINITION (MockSmmCpu, ReadSaveState, 5, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockSmmCpu, WriteSaveState, 5, EFIAPI);

EFI_SMM_CPU_PROTOCOL  SMMCPU_PROTOCOL_INSTANCE = {
  ReadSaveState, // EFI_MM_READ_SAVE_STATE
  WriteSaveState // EFI_MM_WRITE_SAVE_STATE
};

extern "C" {
  EFI_SMM_CPU_PROTOCOL  *gSmmCpuProtocol = &SMMCPU_PROTOCOL_INSTANCE;
}
