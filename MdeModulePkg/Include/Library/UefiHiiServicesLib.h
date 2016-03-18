/** @file
  Provides global variables that are pointers to the UEFI HII related protocols.
  All of the UEFI HII related protocols are optional, so the consumers of this
  library class must verify that the global variable pointers are not NULL before
  use.   

Copyright (c) 2006 - 2010, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials are licensed and made available under 
the terms and conditions of the BSD License that accompanies this distribution.  
The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php.                                            

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
/// The pointer to the UEFI HII Font Protocol.
///
extern EFI_HII_FONT_PROTOCOL  *gHiiFont;

///
/// The pointer to the UEFI HII String Protocol.
///
extern EFI_HII_STRING_PROTOCOL  *gHiiString;

///
/// The pointer to the UEFI HII Image Protocol.
///
extern EFI_HII_IMAGE_PROTOCOL  *gHiiImage;

///
/// The pointer to the UEFI HII Database Protocol.
///
extern EFI_HII_DATABASE_PROTOCOL  *gHiiDatabase;

///
/// The pointer to the UEFI HII Config Rounting Protocol.
///
extern EFI_HII_CONFIG_ROUTING_PROTOCOL  *gHiiConfigRouting;

#endif
