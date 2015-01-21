/** @file

Copyright (c) 2004  - 2014, Intel Corporation. All rights reserved.<BR>
                                                                                   
  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.  
  The full text of the license may be found at                                     
  http://opensource.org/licenses/bsd-license.php.                                  
                                                                                   
  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,            
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.    
                                                                                   


Module Name:

    MiscOemStringData.c

Abstract:

  Static data of OEM String information.
  OEM String information is Misc. subclass type 9 and SMBIOS type 11.


**/


#include "CommonHeader.h"

#include "MiscSubclassDriver.h"

//
// Static (possibly build generated) Bios Vendor data.
//
MISC_SMBIOS_TABLE_DATA(EFI_MISC_OEM_STRING, MiscOemString) = {
  STRING_TOKEN(STR_INTEL_ETK_VER)
};
MISC_SMBIOS_TABLE_DATA(EFI_MISC_OEM_STRING_DATA, OemString)
= { STRING_TOKEN(STR_MISC_OEM_EN_US) };
