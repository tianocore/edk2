/** @file
  Provides global variables that are pointers to the UEFI HII related protocols.
  All of the UEFI HII related protocols are optional, so the consumers of this
  library class must verify that the global variable pointers are not NULL before
  use.   

  Copyright (c) 2006 - 2009, Intel Corporation<BR>
  All rights reserved. This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __UEFI_HII_SERVICES_LIB_H__
#define __UEFI_HII_SERVICES_LIB_H__

#include <Protocol/HiiFont.h>
#include <Protocol/HiiString.h>
#include <Protocol/HiiImage.h>
#include <Protocol/HiiDatabase.h>
#include <Protocol/HiiConfigRouting.h>

///
/// Pointer to the UEFI HII Font Protocol
///
extern EFI_HII_FONT_PROTOCOL  *gHiiFont;

///
/// Pointer to the UEFI HII String Protocol
///
extern EFI_HII_STRING_PROTOCOL  *gHiiString;

///
/// Pointer to the UEFI HII Image Protocol
///
extern EFI_HII_IMAGE_PROTOCOL  *gHiiImage;

///
/// Pointer to the UEFI HII Database Protocol
///
extern EFI_HII_DATABASE_PROTOCOL  *gHiiDatabase;

///
/// Pointer to the UEFI HII Config Rounting Protocol
///
extern EFI_HII_CONFIG_ROUTING_PROTOCOL  *gHiiConfigRouting;

#endif
