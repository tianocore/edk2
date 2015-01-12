/**@file
  Multiplatform initialization header file.

  This file includes package header files, library classes.

  Copyright (c) 2013 - 2014, Intel Corporation. All rights reserved.<BR>
                                                                                   
  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.  
  The full text of the license may be found at                                     
  http://opensource.org/licenses/bsd-license.php.                                  
                                                                                   
  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,            
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.    
                                                                                   
**/

#ifndef _MULTIPLATFORM_LIB_H_
#define _MULTIPLATFORM_LIB_H_


#define LEN_64M       0x4000000

//
// Default PCI32 resource size
//
#define RES_MEM32_MIN_LEN   0x38000000

#define RES_IO_BASE   0x0D00
#define RES_IO_LIMIT  0xFFFF

#include <PiDxe.h>
#include <Library/BaseLib.h>
#include <FrameworkPei.h>

#include "PlatformBaseAddresses.h"
#include "PchAccess.h"
#include "SetupMode.h"
#include "PlatformBootMode.h"
#include "Platform.h"

#include <Ppi/Stall.h>
#include <Guid/SetupVariable.h>
#include <Ppi/AtaController.h>
#include <Ppi/FindFv.h>
#include <Ppi/BootInRecoveryMode.h>
#include <Ppi/ReadOnlyVariable2.h>
#include <Ppi/Capsule.h>
#include <Guid/EfiVpdData.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/PcdLib.h>
#include <Library/IoLib.h>
#include <Library/HobLib.h>
#include <Library/BaseLib.h>
#include <IndustryStandard/Pci22.h>
#include <Ppi/Speaker.h>
#include <Guid/FirmwareFileSystem.h>
#include <Guid/MemoryTypeInformation.h>
#include <Ppi/Cache.h>
#include <Ppi/Reset.h>
#include <Ppi/EndOfPeiPhase.h>
#include <Ppi/MemoryDiscovered.h>
#include <Guid/GlobalVariable.h>
#include <Ppi/RecoveryModule.h>
#include <Ppi/DeviceRecoveryModule.h>
#include <Guid/Capsule.h>
#include <Guid/RecoveryDevice.h>
#include <Ppi/MasterBootMode.h>
#include <Guid/PlatformInfo.h>

#include <BoardOemIds/BoardOemIds.h>
#include <BoardSsidSvid/BoardSsidSvid.h>


EFI_STATUS
GetPlatformInfoHob (
  IN CONST EFI_PEI_SERVICES           **PeiServices,
  OUT EFI_PLATFORM_INFO_HOB     **PlatformInfoHob
  );

EFI_STATUS
MultiPlatformGpioTableInit (
  IN CONST EFI_PEI_SERVICES     **PeiServices,
  IN EFI_PLATFORM_INFO_HOB      *PlatformInfoHob
  );

EFI_STATUS
MultiPlatformGpioProgram (
  IN CONST EFI_PEI_SERVICES     **PeiServices,
  IN EFI_PLATFORM_INFO_HOB      *PlatformInfoHob
  );

#endif
