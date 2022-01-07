/** @file
  SBI inline function calls.

  Copyright (c) 2021-2022, Hewlett Packard Enterprise Development LP. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef EDK2_SBI_H_
#define EDK2_SBI_H_

#include <RiscVImpl.h>
#include <sbi/riscv_asm.h>
#include <sbi/riscv_atomic.h>
#include <sbi/sbi_ecall_interface.h>
#include <sbi/sbi_error.h>
#include <sbi/sbi_platform.h>
#include <sbi/sbi_scratch.h>
#include <sbi/sbi_types.h>
#include <sbi/sbi_hartmask.h>

#define RISC_V_MAX_HART_SUPPORTED  SBI_HARTMASK_MAX_BITS

typedef
VOID
(EFIAPI *RISCV_HART_SWITCH_MODE)(
  IN  UINTN   FuncArg0,
  IN  UINTN   FuncArg1,
  IN  UINTN   NextAddr,
  IN  UINTN   NextMode,
  IN  BOOLEAN NextVirt
  );

//
// Keep the structure member in 64-bit alignment.
//
typedef struct {
  UINT64                    IsaExtensionSupported; // The ISA extension this core supported.
  RISCV_UINT128             MachineVendorId;       // Machine vendor ID
  RISCV_UINT128             MachineArchId;         // Machine Architecture ID
  RISCV_UINT128             MachineImplId;         // Machine Implementation ID
  RISCV_HART_SWITCH_MODE    HartSwitchMode;        // OpenSBI's function to switch the mode of a hart
} EFI_RISCV_FIRMWARE_CONTEXT_HART_SPECIFIC;
#define FIRMWARE_CONTEXT_HART_SPECIFIC_SIZE  (64 * 8) // This is the size of EFI_RISCV_FIRMWARE_CONTEXT_HART_SPECIFIC
                                                      // structure. Referred by both C code and assembly code.

typedef struct {
  UINT64                                      BootHartId;
  VOID                                        *PeiServiceTable;    // PEI Service table
  UINT64                                      FlattenedDeviceTree; // Pointer to Flattened Device tree
  UINT64                                      SecPeiHandOffData;   // This is EFI_SEC_PEI_HAND_OFF passed to PEI Core.
  EFI_RISCV_FIRMWARE_CONTEXT_HART_SPECIFIC    *HartSpecific[RISC_V_MAX_HART_SUPPORTED];
} EFI_RISCV_OPENSBI_FIRMWARE_CONTEXT;

//
// Typedefs of OpenSBI type to make them conform to EDK2 coding guidelines
//
typedef struct sbi_scratch   SBI_SCRATCH;
typedef struct sbi_platform  SBI_PLATFORM;

#endif
