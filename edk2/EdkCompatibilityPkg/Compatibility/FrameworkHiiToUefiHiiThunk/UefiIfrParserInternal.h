/** @file
  Internal Function and Macro defintions for IFR parsing. This header file should only
  be included by UefiIfrParser.c

  Copyright (c) 2008, Intel Corporation
  All rights reserved. This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _HII_THUNK_UEFI_IFR_PARSER_INTERNAL_
#define _HII_THUNK_UEFI_IFR_PARSER_INTERNAL_

#include <Protocol/UnicodeCollation.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/HiiLib.h>
#include <Library/IfrSupportLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PrintLib.h>
#include <Library/UefiBootServicesTableLib.h>

#include <MdeModuleHii.h>

//
// Extern Variables
//
extern CONST EFI_HII_DATABASE_PROTOCOL            *mHiiDatabase;
extern CONST EFI_HII_IMAGE_PROTOCOL               *mHiiImageProtocol;
extern CONST EFI_HII_STRING_PROTOCOL              *mHiiStringProtocol;
extern CONST EFI_HII_CONFIG_ROUTING_PROTOCOL      *mHiiConfigRoutingProtocol;

//
// Incremental string lenght of ConfigRequest
//
#define CONFIG_REQUEST_STRING_INCREMENTAL  1024


#define EFI_SPECIFICATION_ERRATA_VERSION   0

#define EFI_IFR_SPECIFICATION_VERSION  \
        ((((EFI_SPECIFICATION_VERSION) >> 8) & 0xff00) | \
         (((EFI_SPECIFICATION_VERSION) & 0xf) << 4) | \
         ((EFI_SPECIFICATION_ERRATA_VERSION) & 0xf))

extern EFI_GUID          gZeroGuid;

#endif
