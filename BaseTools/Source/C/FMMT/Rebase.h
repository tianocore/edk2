/** @file  Rebase.h

 Library to rebase PE image.

 Copyright (c)  2019, Intel Corporation. All rights reserved.<BR>
 SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _FMMT_REBASE_H
#define _FMMT_REBASE_H

#include <Common/UefiBaseTypes.h>
#include <Common/PiFirmwareFile.h>

EFI_STATUS
RebaseFfs(
IN OUT  UINT64                 BaseAddress,
IN      CHAR8                 *FileName,
IN OUT  EFI_FFS_FILE_HEADER   *FfsFile,
IN      UINTN                 XipOffset
);

EFI_STATUS
GetChildFvFromFfs (
  IN      UINT64                 BaseAddress,
  IN      EFI_FFS_FILE_HEADER   *FfsFile,
  IN      UINTN                 XipOffset
);

#endif
