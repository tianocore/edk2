/** @file
*
*  Copyright (c) 2015, Linaro Ltd. All rights reserved.
*
*  SPDX-License-Identifier: BSD-2-Clause-Patent
*
**/

#ifndef __ARM_GIC_ARCH_LIB_H__
#define __ARM_GIC_ARCH_LIB_H__

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

#endif
