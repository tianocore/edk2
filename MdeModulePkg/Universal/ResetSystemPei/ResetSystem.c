/** @file
  Implementation of Reset2, ResetFilter and ResetHandler PPIs.

  Copyright (c) 2017 - 2018, Intel Corporation. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "ResetSystem.h"

GLOBAL_REMOVE_IF_UNREFERENCED CHAR16 *mResetTypeStr[] = {
  L"Cold", L"Warm", L"Shutdown", L"PlatformSpecific"
};

EFI_PEI_RESET2_PPI mPpiReset2 = {
  ResetSystem2
};

EFI_GUID                *mProcessingOrder[] = {
  &gEdkiiPlatformSpecificResetFilterPpiGuid,
  &gEdkiiPlatformSpecificResetNotificationPpiGuid,
  &gEdkiiPlatformSpecificResetHandlerPpiGuid
};

RESET_FILTER_INSTANCE   mResetFilter = {
  {
    RegisterResetNotify,
    UnregisterResetNotify
  },
  &gEdkiiPlatformSpecificResetFilterPpiGuid
};

RESET_FILTER_INSTANCE   mResetNotification = {
  {
    RegisterResetNotify,
    UnregisterResetNotify
  },
  &gEdkiiPlatformSpecificResetNotificationPpiGuid
};

RESET_FILTER_INSTANCE   mResetHandler = {
  {
    RegisterResetNotify,
    UnregisterResetNotify
  },
  &gEdkiiPlatformSpecificResetHandlerPpiGuid
};

EFI_PEI_PPI_DESCRIPTOR mPpiListReset[] = {
  {
    EFI_PEI_PPI_DESCRIPTOR_PPI,
    &gEfiPeiReset2PpiGuid,
    &mPpiReset2
  },
  {
    EFI_PEI_PPI_DESCRIPTOR_PPI,
    &gEdkiiPlatformSpecificResetFilterPpiGuid,
    &mResetFilter.ResetFilter
  },
  {
    EFI_PEI_PPI_DESCRIPTOR_PPI,
    &gEdkiiPlatformSpecificResetNotificationPpiGuid,
    &mResetNotification.ResetFilter
  },
  {
    EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST,
    &gEdkiiPlatformSpecificResetHandlerPpiGuid,
    &mResetHandler.ResetFilter
  }
};

/**
  Register a notification function to be called when ResetSystem() is called.

  The RegisterResetNotify() function registers a notification function that is called when
  ResetSystem() is called and prior to completing the reset of the platform.
  The registered functions must not perform a platform reset themselves. These
  notifications are intended only for the notification of components which may need some
  special-purpose maintenance prior to the platform resetting.
  The list of registered reset notification functions are processed if ResetSystem()is called
  before ExitBootServices(). The list of registered reset notification functions is ignored if
  ResetSystem() is called after ExitBootServices().

  @param[in]  This              A pointer to the EFI_RESET_NOTIFICATION_PROTOCOL instance.
  @param[in]  ResetFunction     Points to the function to be called when a ResetSystem() is executed.

  @retval EFI_SUCCESS           The reset notification function was successfully registered.
  @retval EFI_INVALID_PARAMETER ResetFunction is NULL.
  @retval EFI_OUT_OF_RESOURCES  There are not enough resources available to register the reset notification function.
  @retval EFI_ALREADY_STARTED   The reset notification function specified by ResetFunction has already been registered.

**/
EFI_STATUS
EFIAPI
RegisterResetNotify (
  IN EDKII_PLATFORM_SPECIFIC_RESET_FILTER_PPI *This,
  IN EFI_RESET_SYSTEM                         ResetFunction
  )
{
  RESET_FILTER_INSTANCE                       *ResetFilter;
  RESET_FILTER_LIST                           *List;
  VOID                                        *Hob;
  UINTN                                       Index;

  if (ResetFunction == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  ResetFilter = (RESET_FILTER_INSTANCE *) This;
  ASSERT (CompareGuid (ResetFilter->Guid, &gEdkiiPlatformSpecificResetFilterPpiGuid) ||
          CompareGuid (ResetFilter->Guid, &gEdkiiPlatformSpecificResetNotificationPpiGuid) ||
          CompareGuid (ResetFilter->Guid, &gEdkiiPlatformSpecificResetHandlerPpiGuid)
          );

  Hob = GetFirstGuidHob (ResetFilter->Guid);
  if (Hob == NULL) {
    //
    // When the GUIDed HOB doesn't exist, create it.
    //
    List = (RESET_FILTER_LIST *)BuildGuidHob (
                                  ResetFilter->Guid,
                                  sizeof (RESET_FILTER_LIST) + sizeof (EFI_RESET_SYSTEM) * PcdGet32 (PcdMaximumPeiResetNotifies)
                                  );
    if (List == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }
    List->Signature = RESET_FILTER_LIST_SIGNATURE;
    List->Count     = PcdGet32 (PcdMaximumPeiResetNotifies);
    ZeroMem (List->ResetFilters, sizeof (EFI_RESET_SYSTEM) * List->Count);
    List->ResetFilters[0] = ResetFunction;
    return EFI_SUCCESS;
  } else {
    List = (RESET_FILTER_LIST *)GET_GUID_HOB_DATA (Hob);
    ASSERT (List->Signature == RESET_FILTER_LIST_SIGNATURE);
    //
    // Firstly check whether the ResetFunction is already registerred.
    //
    for (Index = 0; Index < List->Count; Index++) {
      if (List->ResetFilters[Index] == ResetFunction) {
        break;
      }
    }
    if (Index != List->Count) {
      return EFI_ALREADY_STARTED;
    }

    //
    // Secondly find the first free slot.
    //
    for (Index = 0; Index < List->Count; Index++) {
      if (List->ResetFilters[Index] == NULL) {
        break;
      }
    }

    if (Index == List->Count) {
      return EFI_OUT_OF_RESOURCES;
    }
    List->ResetFilters[Index] = ResetFunction;
    return EFI_SUCCESS;
  }
}

/**
  Unregister a notification function.

  The UnregisterResetNotify() function removes the previously registered
  notification using RegisterResetNotify().

  @param[in]  This              A pointer to the EFI_RESET_NOTIFICATION_PROTOCOL instance.
  @param[in]  ResetFunction     The pointer to the ResetFunction being unregistered.

  @retval EFI_SUCCESS           The reset notification function was unregistered.
  @retval EFI_INVALID_PARAMETER ResetFunction is NULL.
  @retval EFI_INVALID_PARAMETER The reset notification function specified by ResetFunction was not previously
                                registered using RegisterResetNotify().

**/
EFI_STATUS
EFIAPI
UnregisterResetNotify (
  IN EDKII_PLATFORM_SPECIFIC_RESET_FILTER_PPI *This,
  IN EFI_RESET_SYSTEM                         ResetFunction
  )
{

  RESET_FILTER_INSTANCE                       *ResetFilter;
  RESET_FILTER_LIST                           *List;
  VOID                                        *Hob;
  UINTN                                       Index;

  if (ResetFunction == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  ResetFilter = (RESET_FILTER_INSTANCE *)This;
  ASSERT (CompareGuid (ResetFilter->Guid, &gEdkiiPlatformSpecificResetFilterPpiGuid) ||
    CompareGuid (ResetFilter->Guid, &gEdkiiPlatformSpecificResetNotificationPpiGuid) ||
    CompareGuid (ResetFilter->Guid, &gEdkiiPlatformSpecificResetHandlerPpiGuid)
  );

  Hob = GetFirstGuidHob (ResetFilter->Guid);
  if (Hob == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  List = (RESET_FILTER_LIST *)GET_GUID_HOB_DATA (Hob);
  ASSERT (List->Signature == RESET_FILTER_LIST_SIGNATURE);
  for (Index = 0; Index < List->Count; Index++) {
    if (List->ResetFilters[Index] == ResetFunction) {
      break;
    }
  }

  if (Index == List->Count) {
    return EFI_INVALID_PARAMETER;
  }

  List->ResetFilters[Index] = NULL;
  return EFI_SUCCESS;
}


/**
  The PEIM's entry point.

  It initializes the Reset2, ResetFilter and ResetHandler PPIs.

  @param[in] FileHandle  Handle of the file being invoked.
  @param[in] PeiServices Describes the list of possible PEI Services.
  
  @retval EFI_SUCCESS         The entry point is executed successfully.
  @retval EFI_ALREADY_STARTED The Reset2 PPI was already installed.
  @retval others              Status code returned from PeiServicesInstallPpi().

**/
EFI_STATUS
EFIAPI
InitializeResetSystem (
  IN EFI_PEI_FILE_HANDLE     FileHandle,
  IN CONST EFI_PEI_SERVICES  **PeiServices
  )
{
  EFI_STATUS  Status;
  VOID        *Ppi;

  Status = PeiServicesLocatePpi (&gEfiPeiReset2PpiGuid, 0, NULL, (VOID **)&Ppi);
  if (Status != EFI_NOT_FOUND) {
    return EFI_ALREADY_STARTED;
  }

  PeiServicesInstallPpi (mPpiListReset);

  return Status;
}

/**
  Resets the entire platform.

  @param[in] ResetType          The type of reset to perform.
  @param[in] ResetStatus        The status code for the reset.
  @param[in] DataSize           The size, in bytes, of ResetData.
  @param[in] ResetData          For a ResetType of EfiResetCold, EfiResetWarm, or
                                EfiResetShutdown the data buffer starts with a Null-terminated
                                string, optionally followed by additional binary data.
                                The string is a description that the caller may use to further
                                indicate the reason for the system reset. ResetData is only
                                valid if ResetStatus is something other than EFI_SUCCESS
                                unless the ResetType is EfiResetPlatformSpecific
                                where a minimum amount of ResetData is always required.
                                For a ResetType of EfiResetPlatformSpecific the data buffer
                                also starts with a Null-terminated string that is followed
                                by an EFI_GUID that describes the specific type of reset to perform.
**/
VOID
EFIAPI
ResetSystem2 (
  IN EFI_RESET_TYPE   ResetType,
  IN EFI_STATUS       ResetStatus,
  IN UINTN            DataSize,
  IN VOID             *ResetData OPTIONAL
  )
{
  VOID                *Hob;
  UINTN               Index;
  RESET_FILTER_LIST   *List;
  UINTN               OrderIndex;
  UINT8               RecursionDepth;
  UINT8               *RecursionDepthPointer;

  //
  // The recursion depth is stored in GUIDed HOB using gEfiCallerIdGuid.
  //
  Hob = GetFirstGuidHob (&gEfiCallerIdGuid);
  if (Hob == NULL) {
    RecursionDepth = 0;
    RecursionDepthPointer = BuildGuidDataHob (&gEfiCallerIdGuid, &RecursionDepth, sizeof (RecursionDepth));
  } else {
    RecursionDepthPointer = (UINT8 *)GET_GUID_HOB_DATA (Hob);
  }
  //
  // Only do REPORT_STATUS_CODE() on first call to ResetSystem()
  //
  if (*RecursionDepthPointer == 0) {
    //
    // Indicate reset system PEI service is called.
    //
    REPORT_STATUS_CODE (EFI_PROGRESS_CODE, (EFI_SOFTWARE_PEI_SERVICE | EFI_SW_PS_PC_RESET_SYSTEM));
  }

  //
  // Increase the call depth
  //
  (*RecursionDepthPointer)++;
  DEBUG ((DEBUG_INFO, "PEI ResetSystem2: Reset call depth = %d.\n", *RecursionDepthPointer));

  if (*RecursionDepthPointer <= MAX_RESET_NOTIFY_DEPTH) {
    //
    // Iteratively call Reset Filters and Reset Handlers.
    //
    for (OrderIndex = 0; OrderIndex < ARRAY_SIZE (mProcessingOrder); OrderIndex++) {
      Hob = GetFirstGuidHob (mProcessingOrder[OrderIndex]);
      if (Hob != NULL) {
        List = (RESET_FILTER_LIST *)GET_GUID_HOB_DATA (Hob);
        ASSERT (List->Signature == RESET_FILTER_LIST_SIGNATURE);

        for (Index = 0; Index < List->Count; Index++) {
          if (List->ResetFilters[Index] != NULL) {
            List->ResetFilters[Index] (ResetType, ResetStatus, DataSize, ResetData);
          }
        }
      }
    }
  } else {
    ASSERT (ResetType < ARRAY_SIZE (mResetTypeStr));
    DEBUG ((DEBUG_ERROR, "PEI ResetSystem2: Maximum reset call depth is met. Use the current reset type: %s!\n", mResetTypeStr[ResetType]));
  }

  switch (ResetType) {
  case EfiResetWarm:
    ResetWarm ();
    break;

 case EfiResetCold:
    ResetCold ();
    break;

  case EfiResetShutdown:
    ResetShutdown ();
    return ;

  case EfiResetPlatformSpecific:
    ResetPlatformSpecific (DataSize, ResetData);
    return;

  default:
    return ;
  }

  //
  // Given we should have reset getting here would be bad
  //
  ASSERT (FALSE);
}
