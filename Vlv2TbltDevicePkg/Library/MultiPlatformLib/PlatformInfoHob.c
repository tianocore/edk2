/** @file
  Platform Hob access interface for multiplatform.

  Copyright (c) 2010 - 2014, Intel Corporation. All rights reserved.<BR>
                                                                                   
  SPDX-License-Identifier: BSD-2-Clause-Patent

                                                                                   

**/

#include <MultiPlatformLib.h>

/**
  Returns the Platform Info of the platform from the HOB.

  @param PeiServices         General purpose services available to every PEIM.
  @param PlatformInfoHob     Pointer to the PLATFORM_INFO_HOB Pointer

  @retval EFI_SUCCESS        The function completed successfully.
  @retval EFI_NOT_FOUND      PlatformInfoHob data doesn't exist, use default instead.

**/
EFI_STATUS
GetPlatformInfoHob (
  IN CONST EFI_PEI_SERVICES           **PeiServices,
  OUT EFI_PLATFORM_INFO_HOB     **PlatformInfoHob
  )
{
  EFI_PEI_HOB_POINTERS        GuidHob;

  //
  // Find the PlatformInfo HOB
  //
  GuidHob.Raw = GetHobList ();
  if (GuidHob.Raw == NULL) {
    return EFI_NOT_FOUND;
  }

  if ((GuidHob.Raw = GetNextGuidHob (&gEfiPlatformInfoGuid, GuidHob.Raw)) != NULL) {
    *PlatformInfoHob = GET_GUID_HOB_DATA (GuidHob.Guid);
  }

  //
  // PlatformInfo PEIM should provide this HOB data, if not ASSERT and return error.
  //
  ASSERT (*PlatformInfoHob != NULL);
  if (!(*PlatformInfoHob)) {
    return EFI_NOT_FOUND;
  }

  return EFI_SUCCESS;
}

