/*++

Copyright (c) 2012  - 2014, Intel Corporation.  All rights reserved.
                                                                                   
  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.  
  The full text of the license may be found at                                     
  http://opensource.org/licenses/bsd-license.php.                                  
                                                                                   
  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,            
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.    
                                                                                   



Module Name:

  MiscOemType0x90Data.c

Abstract:

  This file contains the Misc Oem Data (SMBIOS data type 0x90)

--*/

#include "CommonHeader.h"

#include "MiscSubclassDriver.h"


//
// Static (possibly build generated) Oem data.
//
MISC_SMBIOS_TABLE_DATA(EFI_MISC_OEM_TYPE_0x90, MiscOemType0x90) = {

STRING_TOKEN (STR_MISC_SEC_VERSION),
STRING_TOKEN (STR_MISC_UCODE_VERSION),
STRING_TOKEN (STR_MISC_GOP_VERSION),
STRING_TOKEN (STR_MISC_PROCESSOR_STEPPING),

};
