/** @file
  This library parses the coreboot table in memory to extract required
  information.

  Copyright (c) 2021, Star Labs Systems. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef SMM_STORE_PARSE_LIB_H_
#define SMM_STORE_PARSE_LIB_H_

#include <Guid/SmmStoreInfoGuid.h>

/**
  Find the SmmStore HOB.

  @param  SmmStoreInfo       Pointer to the SMMSTORE_INFO structure

  @retval RETURN_SUCCESS     Successfully found the Smm store buffer information.
  @retval RETURN_NOT_FOUND   Failed to find the Smm store buffer information.
**/
RETURN_STATUS
EFIAPI
ParseSmmStoreInfo (
  OUT SMMSTORE_INFO  *SmmStoreInfo
  );

#endif // SMM_STORE_PARSE_LIB_H_
