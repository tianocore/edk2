/** @file

  Copyright (c) 2008 - 2009, Apple Inc. All rights reserved.<BR>
  Portions Copyright (c) 2011 - 2013, ARM Ltd. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __AARCH64_LIB_H__
#define __AARCH64_LIB_H__

typedef VOID (*AARCH64_CACHE_OPERATION)(UINTN);


VOID
AArch64PerformPoUDataCacheOperation (
  IN  AARCH64_CACHE_OPERATION  DataCacheOperation
  );

VOID
AArch64AllDataCachesOperation (
  IN  AARCH64_CACHE_OPERATION  DataCacheOperation
  );

#endif // __AARCH64_LIB_H__

