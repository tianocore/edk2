/** @file
  RISC-V IOMMU structures.

  FIXME: In-memory queue interfaces.

  Copyright (c) 2025, 9elements GmbH. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _RISC_V_IO_MMU_STRUCTURES_
#define _RISC_V_IO_MMU_STRUCTURES_

#include <PiDxe.h>

//
// DDT
//
typedef union {
  struct {
    UINT64  V         : 1;
    UINT64  Reserved0 : 9;
    UINT64  PPN       : 44;
    UINT64  Reserved1 : 10;
  } Bits;
  UINT64  Uint64;
} RISCV_IOMMU_DDT_NON_LEAF;

#define N_RISCV_IOMMU_DEVICE_ID_BASE_I0  0
#define N_RISCV_IOMMU_DEVICE_ID_BASE_I1  7
#define N_RISCV_IOMMU_DEVICE_ID_BASE_I2  16

#define N_RISCV_IOMMU_DEVICE_ID_EXTENDED_I0  0
#define N_RISCV_IOMMU_DEVICE_ID_EXTENDED_I1  6
#define N_RISCV_IOMMU_DEVICE_ID_EXTENDED_I2  15

typedef union {
  struct {
    UINT32  Ddi0 : 7;
    UINT32  Ddi1 : 9;
    UINT32  Ddi2 : 8;
    UINT32  Padding : 8;
  } BaseFormat;
  struct {
    UINT32  Ddi0 : 6;
    UINT32  Ddi1 : 9;
    UINT32  Ddi2 : 9;
    UINT32  Padding : 8;
  } ExtendedFormat;
  UINT32 Uint32;

  //
  // Alias to the spec-declared PCI routing ID for ease of use.
  //
  struct {
    UINT32  Function : 3;
    UINT32  Device   : 5;
    UINT32  Bus      : 8;
    UINT32  Segment  : 8;
    UINT32  Padding : 8;
  } PciBdf;
} RISCV_IOMMU_DEVICE_ID;

//
// Device context
//
#define V_RISCV_IOMMU_DC_IOMMU_MODE_BARE  0
#define V_RISCV_IOMMU_DC_IOMMU_MODE_SV32X4  8
#define V_RISCV_IOMMU_DC_IOMMU_MODE_SV39X4  8
#define V_RISCV_IOMMU_DC_IOMMU_MODE_SV48X4  9
#define V_RISCV_IOMMU_DC_IOMMU_MODE_SV57X4  10

typedef struct {
  union {
    struct {
      UINT64  V         : 1;
      UINT64  EN_ATS    : 1;
      UINT64  EN_PRI    : 1;
      UINT64  T2GPA     : 1;
      UINT64  DTF       : 1;
      UINT64  PDTV      : 1;
      UINT64  PRPR      : 1;
      UINT64  GADE      : 1;
      UINT64  SADE      : 1;
      UINT64  DPE       : 1;
      UINT64  SBE       : 1;
      UINT64  SXL       : 1;
      UINT64  Reserved0 : 4;
      UINT64  Reserved1 : 8;
      UINT64  Custom    : 8;
      UINT64  Reserved2 : 16;
      UINT64  Reserved3 : 16;
    } Bits;
    UINT64  Uint64;
  } TranslationControl;
  union {
    struct {
      UINT64  PPN   : 44;
      UINT64  GSCID : 16;
      UINT64  MODE  : 4;
    } Bits;
    UINT64  Uint64;
  } IoHypervisorGuestAddressTranslationAndProtection;
  union {
    struct {
      UINT64  Reserved0 : 12;
      UINT64  PSCID     : 20;
      UINT64  Reserved1 : 8;
      UINT64  RCID      : 12;
      UINT64  MCID      : 12;
    } Bits;
    UINT64  Uint64;
  } TranslationAttributes;
  union {
    struct {
      UINT64  PPN      : 44;
      UINT64  Reserved : 16;
      UINT64  MODE     : 4;
    } Bits;
    UINT64  Uint64;
  } FirstStageContext;
} RISCV_IOMMU_DEVICE_CONTEXT_BASE;

typedef struct {
  //RISCV_IOMMU_DEVICE_CONTEXT_BASE  BaseContext;
  union {
    struct {
      UINT64  PPN      : 44;
      UINT64  Reserved : 16;
      UINT64  MODE     : 4;
    } Bits;
    UINT64  Uint64;
  } MsiPageTablePointer;
  union {
    struct {
      UINT64  mask     : 52;
      UINT64  Reserved : 12;
    } Bits;
    UINT64  Uint64;
  } MsiAddressMask;
  union {
    struct {
      UINT64  pattern  : 52;
      UINT64  Reserved : 12;
    } Bits;
    UINT64  Uint64;
  } MsiAddressPattern;
  UINT64  Reserved;
} RISCV_IOMMU_DEVICE_CONTEXT_EXTENDED;

#endif
