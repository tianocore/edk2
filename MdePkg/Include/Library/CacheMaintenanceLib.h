/** @file
	Cache Maintenance Functions

	Copyright (c) 2006, Intel Corporation
	All rights reserved. This program and the accompanying materials
	are licensed and made available under the terms and conditions of the BSD License
	which accompanies this distribution.  The full text of the license may be found at
	http://opensource.org/licenses/bsd-license.php

	THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
	WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

	Module Name:	CacheMaintenanceLib.h

**/

#ifndef __CACHE_MAINTENANCE_LIB__
#define __CACHE_MAINTENANCE_LIB__

VOID
EFIAPI
InvalidateInstructionCache (
  VOID
  );

VOID *
EFIAPI
InvalidateInstructionCacheRange (
  IN      VOID                      *Address,
  IN      UINTN                     Length
  );

VOID
EFIAPI
WriteBackInvalidateDataCache (
  VOID
  );

VOID *
EFIAPI
WriteBackInvalidateDataCacheRange (
  IN      VOID                      *Address,
  IN      UINTN                     Length
  );

VOID
EFIAPI
WriteBackDataCache (
  VOID
  );

VOID *
EFIAPI
WriteBackDataCacheRange (
  IN      VOID                      *Address,
  IN      UINTN                     Length
  );

VOID
EFIAPI
InvalidateDataCache (
  VOID
  );

VOID *
EFIAPI
InvalidateInstructionCacheRange (
  IN      VOID                      *Address,
  IN      UINTN                     Length
  );

#endif
