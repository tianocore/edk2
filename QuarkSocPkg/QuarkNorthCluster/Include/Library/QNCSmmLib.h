/** @file
QNC Smm Library Services header file.

Copyright (c) 2013-2015 Intel Corporation.

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __QNC_SMM_LIB_H__
#define __QNC_SMM_LIB_H__

/**
  This routine is the chipset code that accepts a request to "open" a region of SMRAM.
  The region could be legacy ABSEG, HSEG, or TSEG near top of physical memory.
  The use of "open" means that the memory is visible from all boot-service
  and SMM agents.

  @retval FALSE  Cannot open a locked SMRAM region
  @retval TRUE   Success to open SMRAM region.
**/
BOOLEAN
EFIAPI
QNCOpenSmramRegion (
  VOID
  );

/**
  This routine is the chipset code that accepts a request to "close" a region of SMRAM.
  The region could be legacy AB or TSEG near top of physical memory.
  The use of "close" means that the memory is only visible from SMM agents,
  not from BS or RT code.

  @retval FALSE  Cannot open a locked SMRAM region
  @retval TRUE   Success to open SMRAM region.
**/
BOOLEAN
EFIAPI
QNCCloseSmramRegion (
  VOID
  );

/**
  This routine is the chipset code that accepts a request to "lock" SMRAM.
  The region could be legacy AB or TSEG near top of physical memory.
  The use of "lock" means that the memory can no longer be opened
  to BS state.
**/
VOID
EFIAPI
QNCLockSmramRegion (
  VOID
  );


#endif

