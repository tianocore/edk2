/** @file

  Copyright (c) 2004  - 2014, Intel Corporation. All rights reserved.<BR>
                                                                                   
  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.  
  The full text of the license may be found at                                     
  http://opensource.org/licenses/bsd-license.php.                                  
                                                                                   
  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,            
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.    
                                                                                   

**/
#include "PiPei.h"
#include <Library/HobLib.h>
#include <Library\BaseLib.h>
#include <Library/DebugLib.h>
#include <Guid/MemoryConfigData.h>
#include <PlatformFspLib.h>

EFI_STATUS
PlatformHobCreateFromFsp (
  IN CONST EFI_PEI_SERVICES     **PeiServices,
  VOID                          *HobList
  )
{
  VOID       *HobData;
  VOID       *NewHobData;
  UINTN      DataSize;

  //
  // Other hob, todo: put this into FspWrapPlatformLib
  //
  if ((HobList = GetNextGuidHob (&gEfiMemoryConfigDataGuid, HobList)) != NULL) {
    HobData = GET_GUID_HOB_DATA (HobList);
    DataSize = GET_GUID_HOB_DATA_SIZE(HobList);
    DEBUG((EFI_D_ERROR, "gEfiMemoryConfigDataGuid Hob found: 0x%x.\n", DataSize));

    NewHobData = BuildGuidHob (&gEfiMemoryConfigDataGuid, DataSize);
    (*PeiServices)->CopyMem (
                      NewHobData,
                      HobData,
                      DataSize
                      );
  }

  return EFI_SUCCESS;
}
