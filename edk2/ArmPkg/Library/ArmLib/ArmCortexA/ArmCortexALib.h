/** @file

  Copyright (c) 2008-2009 Apple Inc. All rights reserved.<BR>

  All rights reserved. This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __ARMCORTEXALIB_H__
#define __ARMCORTEXALIB_H__

typedef VOID (*ARM_CORTEX_A_CACHE_OPERATION)(UINT32);

VOID
EFIAPI
ArmDrainWriteBuffer (
  VOID
  );

VOID
EFIAPI
ArmInvalidateDataCacheEntryBySetWay (
  IN  UINT32  SetWayFormat
  );

VOID
EFIAPI
ArmCleanDataCacheEntryBySetWay (
  IN  UINT32  SetWayFormat
  );

VOID
EFIAPI
ArmCleanInvalidateDataCacheEntryBySetWay (
  IN  UINT32   SetWayFormat
  );

#endif // __ARMCORTEXALIB_H__

