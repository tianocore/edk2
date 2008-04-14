/** @file
  Internal include file for the HII Library instance.

  Copyright (c) 2007, Intel Corporation                                                         
  All rights reserved. This program and the accompanying materials                          
  are licensed and made available under the terms and conditions of the BSD License         
  which accompanies this distribution.  The full text of the license may be found at        
  http://opensource.org/licenses/bsd-license.php                                            

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

**/

#ifndef __INTERNAL_HII_LIB_H__
#define __INTERNAL_HII_LIB_H__

#include <PiDxe.h>

#include <Protocol/HiiDatabase.h>
#include <Protocol/HiiString.h>
#include <Protocol/DevicePath.h>

#include <Guid/GlobalVariable.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/HiiLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/PcdLib.h>
#include <Library/DevicePathLib.h>

#define HII_LIB_DEFAULT_STRING_SIZE     0x200


extern CONST EFI_HII_DATABASE_PROTOCOL   *mHiiDatabaseProt;
extern CONST EFI_HII_STRING_PROTOCOL     *mHiiStringProt;

BOOLEAN
IsHiiHandleRegistered (
  EFI_HII_HANDLE    HiiHandle
  )
;

#endif
