/** @file
  Helper Library for ACPI

  Copyright (c) 2014-2016, ARM Ltd. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __ACPI_LIB_H__
#define __ACPI_LIB_H__

#include <Uefi.h>

#include <IndustryStandard/Acpi10.h>

//
// Macros for the Generic Address Space
//
#define NULL_GAS               { EFI_ACPI_5_0_SYSTEM_MEMORY,  0, 0, EFI_ACPI_5_0_UNDEFINED, 0L }
#define ARM_GAS8(Address)      { EFI_ACPI_5_0_SYSTEM_MEMORY,  8, 0, EFI_ACPI_5_0_BYTE,      Address }
#define ARM_GAS16(Address)     { EFI_ACPI_5_0_SYSTEM_MEMORY, 16, 0, EFI_ACPI_5_0_WORD,      Address }
#define ARM_GAS32(Address)     { EFI_ACPI_5_0_SYSTEM_MEMORY, 32, 0, EFI_ACPI_5_0_DWORD,     Address }
#define ARM_GASN(Address)      { EFI_ACPI_5_0_SYSTEM_MEMORY,  0, 0, EFI_ACPI_5_0_DWORD,     Address }

//
// Macros for the Multiple APIC Description Table (MADT)
//
#define EFI_ACPI_5_0_GIC_DISTRIBUTOR_INIT(GicDistHwId, GicDistBase, GicDistVector) \
  { \
    EFI_ACPI_5_0_GICD, sizeof (EFI_ACPI_5_0_GIC_DISTRIBUTOR_STRUCTURE), EFI_ACPI_RESERVED_WORD, \
    GicDistHwId, GicDistBase, GicDistVector, EFI_ACPI_RESERVED_DWORD \
  }

#define EFI_ACPI_6_0_GIC_DISTRIBUTOR_INIT(GicDistHwId, GicDistBase, GicDistVector, GicVersion) \
  { \
    EFI_ACPI_6_0_GICD, sizeof (EFI_ACPI_6_0_GIC_DISTRIBUTOR_STRUCTURE), EFI_ACPI_RESERVED_WORD, \
    GicDistHwId, GicDistBase, GicDistVector, GicVersion, \
    {EFI_ACPI_RESERVED_BYTE, EFI_ACPI_RESERVED_BYTE, EFI_ACPI_RESERVED_BYTE} \
  }

// Note the parking protocol is configured by UEFI if required
#define EFI_ACPI_5_0_GIC_STRUCTURE_INIT(GicId, AcpiCpuId, Flags, PmuIrq, GicBase) \
  { \
    EFI_ACPI_5_0_GIC, sizeof (EFI_ACPI_5_0_GIC_STRUCTURE), EFI_ACPI_RESERVED_WORD, \
    GicId, AcpiCpuId, Flags, 0, PmuIrq, 0, GicBase \
  }

// Note the parking protocol is configured by UEFI if required
#define EFI_ACPI_5_1_GICC_STRUCTURE_INIT(GicId, AcpiCpuUid, Mpidr, Flags, PmuIrq,    \
    GicBase, GicVBase, GicHBase, GsivId, GicRBase)                                   \
  {                                                                                  \
    EFI_ACPI_5_1_GIC, sizeof (EFI_ACPI_5_1_GIC_STRUCTURE), EFI_ACPI_RESERVED_WORD,   \
    GicId, AcpiCpuUid, Flags, 0, PmuIrq, 0, GicBase, GicVBase, GicHBase,             \
    GsivId, GicRBase, Mpidr                                                          \
  }

#define EFI_ACPI_6_0_GICC_STRUCTURE_INIT(GicId, AcpiCpuUid, Mpidr, Flags, PmuIrq,    \
    GicBase, GicVBase, GicHBase, GsivId, GicRBase, Efficiency)                       \
  {                                                                                  \
    EFI_ACPI_6_0_GIC, sizeof (EFI_ACPI_6_0_GIC_STRUCTURE), EFI_ACPI_RESERVED_WORD,   \
    GicId, AcpiCpuUid, Flags, 0, PmuIrq, 0, GicBase, GicVBase, GicHBase,             \
    GsivId, GicRBase, Mpidr, Efficiency,                                             \
    {EFI_ACPI_RESERVED_BYTE, EFI_ACPI_RESERVED_BYTE, EFI_ACPI_RESERVED_BYTE}         \
  }

#define EFI_ACPI_6_3_GICC_STRUCTURE_INIT(GicId, AcpiCpuUid, Mpidr, Flags, PmuIrq,    \
    GicBase, GicVBase, GicHBase, GsivId, GicRBase, Efficiency, SpeOvflIrq)           \
  {                                                                                  \
    EFI_ACPI_6_0_GIC, sizeof (EFI_ACPI_6_3_GIC_STRUCTURE), EFI_ACPI_RESERVED_WORD,   \
    GicId, AcpiCpuUid, Flags, 0, PmuIrq, 0, GicBase, GicVBase, GicHBase,             \
    GsivId, GicRBase, Mpidr, Efficiency, EFI_ACPI_RESERVED_BYTE, SpeOvflIrq          \
  }

#define EFI_ACPI_6_0_GIC_MSI_FRAME_INIT(GicMsiFrameId, PhysicalBaseAddress, Flags, SPICount, SPIBase) \
  { \
    EFI_ACPI_6_0_GIC_MSI_FRAME, sizeof (EFI_ACPI_6_0_GIC_MSI_FRAME_STRUCTURE), EFI_ACPI_RESERVED_WORD, \
    GicMsiFrameId, PhysicalBaseAddress, Flags, SPICount, SPIBase \
  }

//
// SBSA Generic Watchdog
//
#define EFI_ACPI_5_1_SBSA_GENERIC_WATCHDOG_STRUCTURE_INIT(RefreshFramePhysicalAddress,                  \
    ControlFramePhysicalAddress, WatchdogTimerGSIV, WatchdogTimerFlags)                                 \
  {                                                                                                     \
    EFI_ACPI_5_1_GTDT_SBSA_GENERIC_WATCHDOG, sizeof(EFI_ACPI_5_1_GTDT_SBSA_GENERIC_WATCHDOG_STRUCTURE), \
    EFI_ACPI_RESERVED_BYTE, RefreshFramePhysicalAddress, ControlFramePhysicalAddress,                   \
    WatchdogTimerGSIV, WatchdogTimerFlags                                                               \
  }

typedef
BOOLEAN
(EFIAPI *EFI_LOCATE_ACPI_CHECK) (
  IN  EFI_ACPI_DESCRIPTION_HEADER *AcpiHeader
  );

/**
  Locate and Install the ACPI tables from the Firmware Volume if it verifies
  the function condition.

  @param  AcpiFile                Guid of the ACPI file into the Firmware Volume
  @param  CheckAcpiTableFunction  Function that checks if the ACPI table should be installed

  @return EFI_SUCCESS             The function completed successfully.
  @return EFI_NOT_FOUND           The protocol could not be located.
  @return EFI_OUT_OF_RESOURCES    There are not enough resources to find the protocol.

**/
EFI_STATUS
LocateAndInstallAcpiFromFvConditional (
  IN CONST EFI_GUID*        AcpiFile,
  IN EFI_LOCATE_ACPI_CHECK  CheckAcpiTableFunction
  );

/**
  Locate and Install the ACPI tables from the Firmware Volume

  @param  AcpiFile              Guid of the ACPI file into the Firmware Volume

  @return EFI_SUCCESS           The function completed successfully.
  @return EFI_NOT_FOUND         The protocol could not be located.
  @return EFI_OUT_OF_RESOURCES  There are not enough resources to find the protocol.

**/
EFI_STATUS
LocateAndInstallAcpiFromFv (
  IN CONST EFI_GUID* AcpiFile
  );

#endif // __ACPI_LIB_H__
