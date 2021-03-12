/** @file
  Definition for the CPU_HOT_EJECT_DATA structure, which shares
  CPU hot-eject state between OVMF's SmmCpuFeaturesLib instance in
  PiSmmCpuDxeSmm, and CpuHotplugSmm.

  CPU_HOT_EJECT_DATA is allocated in SMRAM, and pointed-to by
  PcdCpuHotEjectDataAddress.

  PcdCpuHotEjectDataAddress is valid when SMM_REQUIRE is TRUE
  and PcdCpuMaxLogicalProcessorNumber > 1.

  Copyright (C) 2021, Oracle Corporation.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef CPU_HOT_EJECT_DATA_H_
#define CPU_HOT_EJECT_DATA_H_

/**
  CPU Hot-eject handler, called from SmmCpuFeaturesRendezvousExit()
  on each CPU at exit from SMM.

  @param[in] ProcessorNum      ProcessorNum denotes the CPU exiting SMM,
                               and will be used as an index into
                               CPU_HOT_EJECT_DATA->QemuSelectorMap. It is
                               identical to the processor handle in
                               EFI_SMM_CPU_SERVICE_PROTOCOL.
**/
typedef
VOID
(EFIAPI *CPU_HOT_EJECT_HANDLER) (
  IN UINTN  ProcessorNum
  );

//
// CPU_EJECT_QEMU_SELECTOR_INVALID marks CPUs not being ejected in
// CPU_HOT_EJECT_DATA->QemuSelectorMap.
//
// QEMU CPU Selector is UINT32, so we choose an invalid value larger
// than that type.
//
#define CPU_EJECT_QEMU_SELECTOR_INVALID       (MAX_UINT64)

typedef struct {
  //
  // Maps ProcessorNum -> QemuSelector for pending hot-ejects
  //
  volatile UINT64 *QemuSelectorMap;
  //
  // Handler to do the CPU ejection
  //
  volatile CPU_HOT_EJECT_HANDLER Handler;
  //
  // Entries in the QemuSelectorMap
  //
  UINT32 ArrayLength;
} CPU_HOT_EJECT_DATA;

#endif // CPU_HOT_EJECT_DATA_H_
