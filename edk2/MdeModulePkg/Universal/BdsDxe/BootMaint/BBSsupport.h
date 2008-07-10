/** @file
  declares interface functions

Copyright (c) 2004 - 2008, Intel Corporation. <BR>
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _EFI_BDS_BBS_SUPPORT_H
#define _EFI_BDS_BBS_SUPPORT_H

#include "BootMaint.h"
//
// Bugbug: Candidate for a PCD entries
//
#define MAX_BBS_ENTRIES 0x100

/**
  EDES_TODO: Add function description.

  @param CurBBSEntry     EDES_TODO: Add parameter description
  @param Index           EDES_TODO: Add parameter description
  @param BufSize         EDES_TODO: Add parameter description
  @param BootString      EDES_TODO: Add parameter description

  @return EDES_TODO: Add description for return value

**/
VOID
BdsBuildLegacyDevNameString (
  IN BBS_TABLE                     *CurBBSEntry,
  IN UINTN                         Index,
  IN UINTN                         BufSize,
  OUT CHAR16                       *BootString
  );

/**
  EDES_TODO: Add function description.

  @param VOID            EDES_TODO: Add parameter description

  @return EDES_TODO: Add description for return value

**/
EFI_STATUS
BdsDeleteAllInvalidLegacyBootOptions (
  VOID
  );

/**

  Add the legacy boot options from BBS table if they do not exist.


  @param VOID            EDES_TODO: Add parameter description

  @retval  EFI_SUCCESS        The boot options are added successfully or they are already in boot options.
  @retval  others             An error occurred when creating legacy boot options.

**/
EFI_STATUS
BdsAddNonExistingLegacyBootOptions (
  VOID
  )
;

/**
  EDES_TODO: Add function description.

  @param VOID            EDES_TODO: Add parameter description

  @return EDES_TODO: Add description for return value

**/
EFI_STATUS
BdsUpdateLegacyDevOrder (
  VOID
  );

/**
  EDES_TODO: Add function description.

  @param Entry           EDES_TODO: Add parameter description

  @return EDES_TODO: Add description for return value

**/
EFI_STATUS
BdsRefreshBbsTableForBoot (
  IN BDS_COMMON_OPTION        *Entry
  );

#endif
