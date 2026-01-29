/** @file
  Common code PlatformBootManager and PlatformBootManagerLight.

  Copyright (C) 2025, Red Hat, Inc.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef __PLATFORMBOOTMANAGERCOMMON_LIB_H__
#define __PLATFORMBOOTMANAGERCOMMON_LIB_H__

#include <Uefi/UefiBaseType.h>
#include <Base.h>

VOID
PlatformRegisterFvBootOption (
  EFI_GUID  *FileGuid,
  CHAR16    *Description,
  UINT32    Attributes,
  BOOLEAN   Enabled
  );

VOID
RemoveStaleFvFileOptions (
  VOID
  );

#endif
