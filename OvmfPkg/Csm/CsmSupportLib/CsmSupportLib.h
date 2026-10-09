/** @file
  Platform CSM Support Library

  Copyright (c) 2008 - 2011, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _CSM_SUPPORT_LIB_H_
#define _CSM_SUPPORT_LIB_H_

#include <Uefi.h>

/**
  Initialize Legacy Region support

  @retval EFI_SUCCESS   Successfully initialized

**/
EFI_STATUS
LegacyRegionInit (
  VOID
  );

/**
  Initialize Legacy Interrupt support

  @retval EFI_SUCCESS   Successfully initialized

**/
EFI_STATUS
LegacyInterruptInstall (
  VOID
  );

/**
  Initialize Legacy Platform support

  @retval EFI_SUCCESS   Successfully initialized

**/
EFI_STATUS
LegacyBiosPlatformInstall (
  VOID
  );

#endif
