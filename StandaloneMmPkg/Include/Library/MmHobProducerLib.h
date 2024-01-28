/** @file
  HOB Producer Library implementation for Standalone MM Core.

  The MM Hob Producer Library provides function for creating the HOB list
  which StandaloneMm Core needed.

  Copyright (c) 2024, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _MM_HOB_PRODUCER_LIB_H_
#define _MM_HOB_PRODUCER_LIB_H_

/**
  Create the HOB list which StandaloneMm Core needed.

  This function searches the HOBs needed by StandaloneMm Core among the whole
  HOB list. If the input pointer to the HOB list is NULL, then ASSERT().

  @param  HobList          Pointer to whole hob list under PEI phase.

  @return The pointer of the whole HOB list which StandaloneMm Core needed.

**/
VOID *
EFIAPI
CreateMmCoreHobList (
  IN VOID  *HobList
  );

#endif
