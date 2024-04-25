/** @file
*
*  Copyright (c) 2015, Linaro Ltd. All rights reserved.
*  Copyright (c) 2024, Arm Limited. All rights reserved.
*
*  SPDX-License-Identifier: BSD-2-Clause-Patent
*
*  @par Reference(s):
*  - Arm Generic Interrupt Controller Architecture Specification,
*    Issue H, January 2022.
*    (https://developer.arm.com/documentation/ihi0069/)
*
**/

#ifndef ARM_GIC_ARCH_LIB_H_
#define ARM_GIC_ARCH_LIB_H_

//
// GIC definitions
//
typedef enum {
  ARM_GIC_ARCH_REVISION_2,
  ARM_GIC_ARCH_REVISION_3
} ARM_GIC_ARCH_REVISION;

ARM_GIC_ARCH_REVISION
EFIAPI
ArmGicGetSupportedArchRevision (
  VOID
  );

//
// GIC SPI and extended SPI ranges
//
#define ARM_GIC_ARCH_SPI_MIN      32
#define ARM_GIC_ARCH_SPI_MAX      1019
#define ARM_GIC_ARCH_EXT_SPI_MIN  4096
#define ARM_GIC_ARCH_EXT_SPI_MAX  5119

#endif // ARM_GIC_ARCH_LIB_H_
