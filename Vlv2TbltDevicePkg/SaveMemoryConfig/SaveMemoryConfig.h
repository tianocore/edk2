/** 
  Copyright (c) 2009 - 2013, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.


Module Name:

  SaveMemoryConfig.h

Abstract:

  Header file for Save Previous Memory Configuration Driver.

 

--*/


#ifndef _SAVE_MEMORY_CONFIG_DRIVER_H
#define _SAVE_MEMORY_CONFIG_DRIVER_H

#include "Protocol/SetupMode.h"
#include "Guid/PlatformInfo.h"
#include "Library/HobLib.h"
#include "Library/DebugLib.h"
#include "Library/UefiBootServicesTableLib.h"
#include "Library/BaseMemoryLib.h"
#include "PlatformBootMode.h"
#include "Library/BaseLib.h"
#include "Library/UefiRuntimeServicesTableLib.h"
#include "Guid/GlobalVariable.h"
#include "Library/UefiLib.h"
#include "Guid/HobList.h"
#include "Guid/MemoryConfigData.h"
#include "Protocol/MemInfo.h"
#include "Library/MemoryAllocationLib.h"
#include <Guid/Vlv2Variable.h>

//
// Prototypes
//
EFI_STATUS
EFIAPI
SaveMemoryConfigEntryPoint (
  IN EFI_HANDLE         ImageHandle,
  IN EFI_SYSTEM_TABLE   *SystemTable
  )
/*++
  
  Routine Description:
    This is the standard EFI driver point that detects whether there is a
    MemoryConfigurationData HOB and, if so, saves its data to nvRAM.

  Arguments:
    ImageHandle   - Handle for the image of this driver
    SystemTable   - Pointer to the EFI System Table

  Returns:
    EFI_SUCCESS   - if the data is successfully saved or there was no data
    EFI_NOT_FOUND - if the HOB list could not be located.
    EFI_UNLOAD_IMAGE - It is not success
    
--*/
;

#endif
