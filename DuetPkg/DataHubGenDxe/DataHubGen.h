/** @file

Copyright (c) 2006 - 2007, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name: 

  DataHubGen.h

Abstract:

**/

#ifndef _DATA_HUB_GEN_H_
#define _DATA_HUB_GEN_H_

#include <FrameworkDxe.h>
#include <IndustryStandard/SmBios.h>

#include <Guid/HobList.h>
#include <Guid/SmBios.h>
#include <Guid/DataHubProducer.h>
#include <Guid/DataHubRecords.h>

#include <Protocol/DataHub.h>
#include <Protocol/FrameworkHii.h>
#include <Protocol/HiiDatabase.h>

#include <Library/BaseLib.h>
#include <Library/UefiLib.h>
#include <Library/HobLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/FrameworkHiiLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>

#define   PRODUCT_NAME                  L"DUET"
#define   PRODUCT_VERSION               L"Beta"

#define   FIRMWARE_PRODUCT_NAME         (PRODUCT_NAME L": ")
#ifdef EFI32
#if (EFI_SPECIFICATION_VERSION >= 0x00020000)
#define   FIRMWARE_BIOS_VERSIONE        (PRODUCT_NAME L"(IA32.UEFI)" PRODUCT_VERSION L": ")
#else
#define   FIRMWARE_BIOS_VERSIONE        (PRODUCT_NAME L"(IA32.EFI)"  PRODUCT_VERSION L": ")
#endif
#else  // EFIX64
#if (EFI_SPECIFICATION_VERSION >= 0x00020000)
#define   FIRMWARE_BIOS_VERSIONE        (PRODUCT_NAME L"(X64.UEFI)"  PRODUCT_VERSION L": ")
#else
#define   FIRMWARE_BIOS_VERSIONE        (PRODUCT_NAME L"(X64.EFI)"   PRODUCT_VERSION L": ")
#endif
#endif

SMBIOS_STRUCTURE_POINTER
GetSmbiosTableFromType (
  IN SMBIOS_TABLE_ENTRY_POINT  *Smbios,
  IN UINT8                 Type,
  IN UINTN                 Index
  );

CHAR8 *
GetSmbiosString (
  IN SMBIOS_STRUCTURE_POINTER  SmbiosTable,
  IN SMBIOS_TABLE_STRING       String
  );

#endif
