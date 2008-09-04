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

#ifndef _EFI_BDS_BBS_SUPPORT_H_
#define _EFI_BDS_BBS_SUPPORT_H_

#include "BootMaint.h"

#define MAX_BBS_ENTRIES 0x100

/**
  Build Legacy Device Name String according.

  @param CurBBSEntry     BBS Table.
  @param Index           Index.
  @param BufSize         The buffer size.
  @param BootString      The output string.

  @return VOID           No output.

**/
VOID
BdsBuildLegacyDevNameString (
  IN BBS_TABLE                     *CurBBSEntry,
  IN UINTN                         Index,
  IN UINTN                         BufSize,
  OUT CHAR16                       *BootString
  );

/**
  Delete all the invalid legacy boot options.

  

  @retval EFI_SUCCESS             All invalide legacy boot options are deleted.
  @retval EFI_OUT_OF_RESOURCES    Fail to allocate necessary memory.
  @retval EFI_NOT_FOUND           Fail to retrive variable of boot order.
**/
EFI_STATUS
BdsDeleteAllInvalidLegacyBootOptions (
  VOID
  );

/**

  Add the legacy boot options from BBS table if they do not exist.

  @retval  EFI_SUCCESS        The boot options are added successfully or they are already in boot options.
  @retval  others             An error occurred when creating legacy boot options.

**/
EFI_STATUS
BdsAddNonExistingLegacyBootOptions (
  VOID
  );

/**

  Add the legacy boot devices from BBS table into 
  the legacy device boot order.

  @retval EFI_SUCCESS       The boot devices are added successfully.

**/
EFI_STATUS
BdsUpdateLegacyDevOrder (
  VOID
  );

/**
  Set the boot priority for BBS entries based on boot option entry and boot order.

  @param  Entry             The boot option is to be checked for refresh BBS table.
  
  @retval EFI_SUCCESS       The boot priority for BBS entries is refreshed successfully.
  @return status of BdsSetBootPriority4SameTypeDev()
**/
EFI_STATUS
BdsRefreshBbsTableForBoot (
  IN BDS_COMMON_OPTION        *Entry
  );

#endif
