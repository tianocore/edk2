/** @file

  Copyright (c) 2008 - 2009, Apple Inc. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __ARM_V7_LIB_H__
#define __ARM_V7_LIB_H__

#define ID_MMFR0_SHARELVL_SHIFT       12
#define ID_MMFR0_SHARELVL_MASK       0xf
#define ID_MMFR0_SHARELVL_ONE          0
#define ID_MMFR0_SHARELVL_TWO          1

#define ID_MMFR0_INNERSHR_SHIFT       28
#define ID_MMFR0_INNERSHR_MASK       0xf
#define ID_MMFR0_OUTERSHR_SHIFT        8
#define ID_MMFR0_OUTERSHR_MASK       0xf

#define ID_MMFR0_SHR_IMP_UNCACHED      0
#define ID_MMFR0_SHR_IMP_HW_COHERENT   1
#define ID_MMFR0_SHR_IGNORED         0xf

typedef VOID (*ARM_V7_CACHE_OPERATION)(UINT32);

VOID
ArmV7AllDataCachesOperation (
  IN  ARM_V7_CACHE_OPERATION  DataCacheOperation
  );

UINTN
EFIAPI
ArmReadIdMmfr0 (
  VOID
  );

BOOLEAN
EFIAPI
ArmHasMpExtensions (
  VOID
  );

#endif // __ARM_V7_LIB_H__

