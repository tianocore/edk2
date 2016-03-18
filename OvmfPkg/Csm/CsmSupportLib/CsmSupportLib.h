/** @file
  Platform CSM Support Library

  Copyright (c) 2008 - 2011, Intel Corporation. All rights reserved.<BR>

  This program and the accompanying materials are
  licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

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

