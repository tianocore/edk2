/** @file

  Copyright (c) 2008 - 2009, Apple Inc. All rights reserved.<BR>
  Portions Copyright (c) 2011 - 2013, ARM Ltd. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __AARCH64_LIB_H__
#define __AARCH64_LIB_H__

typedef VOID (*AARCH64_CACHE_OPERATION)(UINTN);

VOID
AArch64AllDataCachesOperation (
  IN  AARCH64_CACHE_OPERATION  DataCacheOperation
  );

#endif // __AARCH64_LIB_H__

