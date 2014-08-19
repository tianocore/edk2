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

typedef VOID (*ARM_V7_CACHE_OPERATION)(UINT32);


VOID
ArmV7PerformPoUDataCacheOperation (
  IN  ARM_V7_CACHE_OPERATION  DataCacheOperation
  );

VOID
ArmV7AllDataCachesOperation (
  IN  ARM_V7_CACHE_OPERATION  DataCacheOperation
  );

#endif // __ARM_V7_LIB_H__

