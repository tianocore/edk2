/** @file
*
*  Copyright (c) 2015, Linaro Ltd. All rights reserved.
*
*  This program and the accompanying materials are licensed and made available
*  under the terms and conditions of the BSD License which accompanies this
*  distribution.  The full text of the license may be found at
*  http://opensource.org/licenses/bsd-license.php
*
*  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
*  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
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
