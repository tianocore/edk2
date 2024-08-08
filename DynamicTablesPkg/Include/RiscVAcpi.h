/** @file
  ACPI table definition of RHCT table

  Copyright (c) 2023-2024, Ventana Micro Systems Inc. All Rights Reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

  Temporary header till ACPI spec released and EDK2 header is updated.
**/

#ifndef RISCV_ACPI_H_
#define RISCV_ACPI_H_

#define EFI_ACPI_6_6_RINTC  0x18
#define EFI_ACPI_6_6_IMSIC  0x19
#define EFI_ACPI_6_6_APLIC  0x1A
#define EFI_ACPI_6_6_PLIC   0x1B

#define IMSIC_MMIO_PAGE_SHIFT  12
#define IMSIC_MMIO_PAGE_SZ     (1 << IMSIC_MMIO_PAGE_SHIFT)

#pragma pack (1)

///
/// RISC-V Hart Local Interrupt Controller
///
typedef struct {
  UINT8     Type;
  UINT8     Length;
  UINT8     Version;
  UINT8     Reserved1;
  UINT32    Flags;
  UINT64    HartId;
  UINT32    AcpiProcessorUid;
  UINT32    ExtIntCId;
  UINT64    ImsicBaseAddress;
  UINT32    ImsicSize;
} EFI_ACPI_6_6_RINTC_STRUCTURE;

#define EFI_ACPI_6_6_RISCV_RINTC_STRUCTURE_VERSION  1
#define EFI_ACPI_6_6_RINTC_FLAG_ENABLE              1

///
/// RISC-V Incoming MSI Controller
///
typedef struct {
  UINT8     Type;
  UINT8     Length;
  UINT8     Version;
  UINT8     Reserved1;
  UINT32    Flags;
  UINT16    NumIds;
  UINT16    NumGuestIds;
  UINT8     GuestIndexBits;
  UINT8     HartIndexBits;
  UINT8     GroupIndexBits;
  UINT8     GroupIndexShift;
} EFI_ACPI_6_6_IMSIC_STRUCTURE;

#define EFI_ACPI_6_6_RISCV_IMSIC_STRUCTURE_VERSION  1

///
/// RISC-V Advanced Platform Level Interrupt Controller (APLIC)
///
typedef struct {
  UINT8     Type;
  UINT8     Length;
  UINT8     Version;
  UINT8     AplicId;
  UINT32    Flags;
  UINT8     HwId[8];
  UINT16    NumIdcs;
  UINT16    NumSources;
  UINT32    GsiBase;
  UINT64    AplicAddress;
  UINT32    AplicSize;
} EFI_ACPI_6_6_APLIC_STRUCTURE;

#define EFI_ACPI_6_6_RISCV_APLIC_STRUCTURE_VERSION  1
///
/// RISC-V Platform Level Interrupt Controller (PLIC)
///
typedef struct {
  UINT8     Type;
  UINT8     Length;
  UINT8     Version;
  UINT8     PlicId;
  UINT8     HwId[8];
  UINT16    NumSources;
  UINT16    MaxPriority;
  UINT32    Flags;
  UINT32    PlicSize;
  UINT64    PlicAddress;
  UINT32    GsiBase;
} EFI_ACPI_6_6_PLIC_STRUCTURE;

#define EFI_ACPI_RHCT_TYPE_ISA_NODE                   0
#define EFI_ACPI_RHCT_TYPE_CMO_NODE                   1
#define EFI_ACPI_RHCT_TYPE_MMU_NODE                   2
#define EFI_ACPI_RHCT_TYPE_HART_INFO_NODE             65535
#define EFI_ACPI_6_6_RHCT_FLAG_TIMER_CANNOT_WAKE_CPU  0x1

///
/// RISC-V Hart RHCT Node Header Structure
///
typedef struct {
  UINT16    Type;
  UINT16    Length;
  UINT16    Revision;
} EFI_ACPI_6_6_RISCV_RHCT_NODE;

///
/// RISC-V Hart RHCT ISA Node Structure
///
typedef struct {
  EFI_ACPI_6_6_RISCV_RHCT_NODE    Node;
  UINT16                          IsaLength;
  //  CHAR8     *IsaString;
} EFI_ACPI_6_6_RISCV_RHCT_ISA_NODE;

#define EFI_ACPI_6_6_RISCV_RHCT_ISA_NODE_STRUCTURE_VERSION  1

///
/// RISC-V Hart RHCT CMO Node Structure
///
typedef struct {
  EFI_ACPI_6_6_RISCV_RHCT_NODE    Node;
  UINT8                           Reserved;
  UINT8                           CbomBlockSize;
  UINT8                           CbopBlockSize;
  UINT8                           CbozBlockSize;
} EFI_ACPI_6_6_RISCV_RHCT_CMO_NODE;

#define EFI_ACPI_6_6_RISCV_RHCT_CMO_NODE_STRUCTURE_VERSION  1

///
/// RISC-V Hart RHCT Hart Info Structure
///
typedef struct {
  EFI_ACPI_6_6_RISCV_RHCT_NODE    Node;
  UINT16                          NumOffsets;
  UINT32                          ACPICpuUid;
  UINT32                          Offsets[0];
} EFI_ACPI_6_6_RISCV_RHCT_HART_INFO_NODE;

#define EFI_ACPI_6_6_RISCV_RHCT_HART_INFO_NODE_STRUCTURE_VERSION  1

///
/// RISC-V Hart Capabilities Table (RHCT)
///
typedef struct {
  EFI_ACPI_DESCRIPTION_HEADER    Header;
  UINT32                         Flags;
  UINT64                         TimerFreq;
  UINT32                         NumNodes;
  UINT32                         NodeOffset;
} EFI_ACPI_6_6_RISCV_HART_CAPABILITIES_TABLE;

#define EFI_ACPI_6_6_RISCV_RHCT_TABLE_REVISION  1

///
/// "RHCT" RISC-V Hart Capabilities Table
///
#define EFI_ACPI_6_6_RISCV_HART_CAPABILITIES_TABLE_SIGNATURE  SIGNATURE_32('R', 'H', 'C', 'T')

/* 7: RINTC Affinity Structure(ACPI 6.6) */

typedef struct {
  UINT8     Type;
  UINT8     Length;
  UINT16    Reserved;
  UINT32    ProximityDomain;
  UINT32    AcpiProcessorUid;
  UINT32    Flags;
  UINT32    ClockDomain;
} EFI_ACPI_6_6_RINTC_AFFINITY_STRUCTURE;

#define EFI_ACPI_6_6_RINTC_AFFINITY  0x7
/* Flags for ACPI_SRAT_RINTC_AFFINITY */

#define ACPI_SRAT_RINTC_ENABLED  (1)            /* 00: Use affinity structure */

#pragma pack ()

#endif /* RISCV_ACPI_H_ */
