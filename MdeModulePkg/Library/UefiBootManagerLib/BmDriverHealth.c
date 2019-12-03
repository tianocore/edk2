/** @file
  Library functions which relates with driver health.

Copyright (c) 2011 - 2018, Intel Corporation. All rights reserved.<BR>
(C) Copyright 2015 Hewlett-Packard Development Company, L.P.<BR>
(C) Copyright 2016 Hewlett Packard Enterprise Development LP<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "InternalBm.h"

GLOBAL_REMOVE_IF_UNREFERENCED
  CHAR16 *mBmHealthStatusText[] = {
    L"Healthy",
    L"Repair Required",
    L"Configuration Required",
    L"Failed",
    L"Reconnect Required",
    L"Reboot Required"
    };

/**
  Return the controller name.

  @param DriverHealthHandle  The handle on which the Driver Health protocol instance is retrieved.
  @param ControllerHandle    The handle of a controller that the driver specified by DriverBindingHandle is managing.
                             This handle specifies the controller whose name is to be returned.
  @param ChildHandle         The handle of the child controller to retrieve the name of. This is an
                             optional parameter that may be NULL. It will be NULL for device drivers.
                             It will also be NULL for bus drivers that attempt to retrieve the name
                             of the bus controller. It will not be NULL for a bus driver that attempts
                             to retrieve the name of a child controller.

  @return A pointer to the Unicode string to return. This Unicode string is the name of the controller
          specified by ControllerHandle and ChildHandle.
**/
CHAR16 *
BmGetControllerName (
  IN  EFI_HANDLE                   DriverHealthHandle,
  IN  EFI_HANDLE                   ControllerHandle,
  IN  EFI_HANDLE                   ChildHandle
  )
{
  EFI_STATUS                       Status;
  CHAR16                           *ControllerName;
  CHAR8                            *LanguageVariable;
  CHAR8                            *BestLanguage;
  BOOLEAN                          Iso639Language;
  EFI_COMPONENT_NAME_PROTOCOL      *ComponentName;

  ControllerName = NULL;

  //
  // Locate Component Name (2) protocol on the driver binging handle.
  //
  Iso639Language = FALSE;
  Status = gBS->HandleProtocol (
                 DriverHealthHandle,
                 &gEfiComponentName2ProtocolGuid,
                 (VOID **) &ComponentName
                 );
  if (EFI_ERROR (Status)) {
    Status = gBS->HandleProtocol (
                    DriverHealthHandle,
                    &gEfiComponentNameProtocolGuid,
                    (VOID **) &ComponentName
                    );
    if (!EFI_ERROR (Status)) {
      Iso639Language = TRUE;
    }
  }

  if (!EFI_ERROR (Status)) {
    GetEfiGlobalVariable2 (Iso639Language ? L"Lang" : L"PlatformLang", (VOID**)&LanguageVariable, NULL);
    BestLanguage     = GetBestLanguage(
                         ComponentName->SupportedLanguages,
                         Iso639Language,
                         (LanguageVariable != NULL) ? LanguageVariable : "",
                         Iso639Language ? "eng" : "en-US",
                         NULL
                         );
    if (LanguageVariable != NULL) {
      FreePool (LanguageVariable);
    }

    Status = ComponentName->GetControllerName (
                              ComponentName,
                              ControllerHandle,
                              ChildHandle,
                              BestLanguage,
                              &ControllerName
                              );
  }

  if (!EFI_ERROR (Status)) {
    return AllocateCopyPool (StrSize (ControllerName), ControllerName);
  } else {
    return ConvertDevicePathToText (
             DevicePathFromHandle (ChildHandle != NULL ? ChildHandle : ControllerHandle),
             FALSE,
             FALSE
             );
  }
}

/**
  Display a set of messages returned by the GetHealthStatus () service of the EFI Driver Health Protocol

  @param DriverHealthInfo  Pointer to the Driver Health information entry.
**/
VOID
BmDisplayMessages (
  IN  EFI_BOOT_MANAGER_DRIVER_HEALTH_INFO *DriverHealthInfo
  )
{
  UINTN                            Index;
  EFI_STRING                       String;
  CHAR16                           *ControllerName;

  if (DriverHealthInfo->MessageList == NULL ||
      DriverHealthInfo->MessageList[0].HiiHandle == NULL) {
    return;
  }

  ControllerName = BmGetControllerName (
                     DriverHealthInfo->DriverHealthHandle,
                     DriverHealthInfo->ControllerHandle,
                     DriverHealthInfo->ChildHandle
                     );

  DEBUG ((EFI_D_INFO, "Controller: %s\n", ControllerName));
  Print (L"Controller: %s\n", ControllerName);
  for (Index = 0; DriverHealthInfo->MessageList[Index].HiiHandle != NULL; Index++) {
    String = HiiGetString (
               DriverHealthInfo->MessageList[Index].HiiHandle,
               DriverHealthInfo->MessageList[Index].StringId,
               NULL
               );
    if (String != NULL) {
      Print (L"  %s\n", String);
      DEBUG ((EFI_D_INFO, "  %s\n", String));
      FreePool (String);
    }
  }

  if (ControllerName != NULL) {
    FreePool (ControllerName);
  }
}

/**
  The repair notify function.
  @param Value  A value between 0 and Limit that identifies the current progress
                of the repair operation.
  @param Limit  The maximum value of Value for the current repair operation.
                If Limit is 0, then the completion progress is indeterminate.
                For example, a driver that wants to specify progress in percent
                would use a Limit value of 100.

  @retval EFI_SUCCESS  Successfully return from the notify function.
**/
EFI_STATUS
EFIAPI
BmRepairNotify (
  IN UINTN        Value,
  IN UINTN        Limit
  )
{
  DEBUG ((EFI_D_INFO, "[BDS]RepairNotify: %d/%d\n", Value, Limit));
  Print (L"[BDS]RepairNotify: %d/%d\n", Value, Limit);

  return EFI_SUCCESS;
}

/**
  Collect the Driver Health status of a single controller.

  @param DriverHealthInfo        A pointer to the array containing all of the platform driver health information.
  @param Count                   Return the updated array count.
  @param DriverHealthHandle      The handle on which the Driver Health protocol instance is retrieved.
  @param ControllerHandle        The handle of the controller..
  @param ChildHandle             The handle of the child controller to retrieve the health
                                 status on.  This is an optional parameter that may be NULL.

  @retval Status                 The status returned from GetHealthStatus.
  @retval EFI_ABORTED            The health status is healthy so no further query is needed.

**/
EFI_STATUS
BmGetSingleControllerHealthStatus (
  IN OUT EFI_BOOT_MANAGER_DRIVER_HEALTH_INFO **DriverHealthInfo,
  IN OUT UINTN                               *Count,
  IN  EFI_HANDLE                             DriverHealthHandle,
  IN  EFI_HANDLE                             ControllerHandle,  OPTIONAL
  IN  EFI_HANDLE                             ChildHandle        OPTIONAL
  )
{
  EFI_STATUS                     Status;
  EFI_DRIVER_HEALTH_PROTOCOL     *DriverHealth;
  EFI_DRIVER_HEALTH_HII_MESSAGE  *MessageList;
  EFI_HII_HANDLE                 FormHiiHandle;
  EFI_DRIVER_HEALTH_STATUS       HealthStatus;

  ASSERT (DriverHealthHandle != NULL);
  //
  // Retrieve the Driver Health Protocol from DriverHandle
  //
  Status = gBS->HandleProtocol (
                  DriverHealthHandle,
                  &gEfiDriverHealthProtocolGuid,
                  (VOID **) &DriverHealth
                  );
  ASSERT_EFI_ERROR (Status);


  if (ControllerHandle == NULL) {
    //
    // If ControllerHandle is NULL, the return the cumulative health status of the driver
    //
    Status = DriverHealth->GetHealthStatus (DriverHealth, NULL, NULL, &HealthStatus, NULL, NULL);
    if (!EFI_ERROR (Status) && HealthStatus == EfiDriverHealthStatusHealthy) {
      *DriverHealthInfo = ReallocatePool (
                            (*Count)     * sizeof (EFI_BOOT_MANAGER_DRIVER_HEALTH_INFO),
                            (*Count + 1) * sizeof (EFI_BOOT_MANAGER_DRIVER_HEALTH_INFO),
                            *DriverHealthInfo
                            );
      ASSERT (*DriverHealthInfo != NULL);

      (*DriverHealthInfo)[*Count].DriverHealthHandle = DriverHealthHandle;
      (*DriverHealthInfo)[*Count].DriverHealth       = DriverHealth;
      (*DriverHealthInfo)[*Count].HealthStatus       = HealthStatus;

      *Count = *Count + 1;

      Status = EFI_ABORTED;
    }
    return Status;
  }

  MessageList   = NULL;
  FormHiiHandle = NULL;

  //
  // Collect the health status with the optional HII message list
  //
  Status = DriverHealth->GetHealthStatus (DriverHealth, ControllerHandle, ChildHandle, &HealthStatus, &MessageList, &FormHiiHandle);
  if (!EFI_ERROR (Status)) {
    *DriverHealthInfo = ReallocatePool (
                          (*Count)     * sizeof (EFI_BOOT_MANAGER_DRIVER_HEALTH_INFO),
                          (*Count + 1) * sizeof (EFI_BOOT_MANAGER_DRIVER_HEALTH_INFO),
                          *DriverHealthInfo
                          );
    ASSERT (*DriverHealthInfo != NULL);
    (*DriverHealthInfo)[*Count].DriverHealth       = DriverHealth;
    (*DriverHealthInfo)[*Count].DriverHealthHandle = DriverHealthHandle;
    (*DriverHealthInfo)[*Count].ControllerHandle   = ControllerHandle;
    (*DriverHealthInfo)[*Count].ChildHandle        = ChildHandle;
    (*DriverHealthInfo)[*Count].HiiHandle          = FormHiiHandle;
    (*DriverHealthInfo)[*Count].MessageList        = MessageList;
    (*DriverHealthInfo)[*Count].HealthStatus       = HealthStatus;

    *Count = *Count + 1;
  }

  return Status;
}

/**
  Return all the Driver Health information.

  When the cumulative health status of all the controllers managed by the
  driver who produces the EFI_DRIVER_HEALTH_PROTOCOL is healthy, only one
  EFI_BOOT_MANAGER_DRIVER_HEALTH_INFO entry is created for such
  EFI_DRIVER_HEALTH_PROTOCOL instance.
  Otherwise, every controller creates one EFI_BOOT_MANAGER_DRIVER_HEALTH_INFO
  entry. Additionally every child controller creates one
  EFI_BOOT_MANAGER_DRIVER_HEALTH_INFO entry if the driver is a bus driver.

  @param Count      Return the count of the Driver Health information.

  @retval NULL      No Driver Health information is returned.
  @retval !NULL     Pointer to the Driver Health information array.
**/
EFI_BOOT_MANAGER_DRIVER_HEALTH_INFO *
EFIAPI
EfiBootManagerGetDriverHealthInfo (
  UINTN                      *Count
  )
{
  EFI_STATUS                 Status;
  UINTN                      NumHandles;
  EFI_HANDLE                 *DriverHealthHandles;
  UINTN                      DriverHealthIndex;
  EFI_HANDLE                 *Handles;
  UINTN                      HandleCount;
  UINTN                      ControllerIndex;
  UINTN                      ChildIndex;
  EFI_BOOT_MANAGER_DRIVER_HEALTH_INFO  *DriverHealthInfo;

  //
  // Initialize local variables
  //
  *Count                  = 0;
  DriverHealthInfo        = NULL;
  Handles                 = NULL;
  DriverHealthHandles     = NULL;
  NumHandles              = 0;
  HandleCount             = 0;

  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEfiDriverHealthProtocolGuid,
                  NULL,
                  &NumHandles,
                  &DriverHealthHandles
                  );

  if (Status == EFI_NOT_FOUND || NumHandles == 0) {
    //
    // If there are no Driver Health Protocols handles, then return EFI_NOT_FOUND
    //
    return NULL;
  }

  ASSERT_EFI_ERROR (Status);
  ASSERT (DriverHealthHandles != NULL);

  //
  // Check the health status of all controllers in the platform
  // Start by looping through all the Driver Health Protocol handles in the handle database
  //
  for (DriverHealthIndex = 0; DriverHealthIndex < NumHandles; DriverHealthIndex++) {
    //
    // Get the cumulative health status of the driver
    //
    Status = BmGetSingleControllerHealthStatus (&DriverHealthInfo, Count, DriverHealthHandles[DriverHealthIndex], NULL, NULL);
    if (EFI_ERROR (Status)) {
      continue;
    }

    //
    // See if the list of all handles in the handle database has been retrieved
    //
    //
    if (Handles == NULL) {
      //
      // Retrieve the list of all handles from the handle database
      //
      Status = gBS->LocateHandleBuffer (
        AllHandles,
        NULL,
        NULL,
        &HandleCount,
        &Handles
        );
      ASSERT_EFI_ERROR (Status);
    }
    //
    // Loop through all the controller handles in the handle database
    //
    for (ControllerIndex = 0; ControllerIndex < HandleCount; ControllerIndex++) {
      Status = BmGetSingleControllerHealthStatus (&DriverHealthInfo, Count, DriverHealthHandles[DriverHealthIndex], Handles[ControllerIndex], NULL);
      if (EFI_ERROR (Status)) {
        continue;
      }

      //
      // Loop through all the child handles in the handle database
      //
      for (ChildIndex = 0; ChildIndex < HandleCount; ChildIndex++) {
        Status = BmGetSingleControllerHealthStatus (&DriverHealthInfo, Count, DriverHealthHandles[DriverHealthIndex], Handles[ControllerIndex], Handles[ChildIndex]);
        if (EFI_ERROR (Status)) {
          continue;
        }
      }
    }
  }

  Status = EFI_SUCCESS;

  if (Handles != NULL) {
    FreePool (Handles);
  }
  if (DriverHealthHandles != NULL) {
    FreePool (DriverHealthHandles);
  }

  return DriverHealthInfo;
}

/**
  Free the Driver Health information array.

  @param DriverHealthInfo       Pointer to array of the Driver Health information.
  @param Count                  Count of the array.

  @retval EFI_SUCCESS           The array is freed.
  @retval EFI_INVALID_PARAMETER The array is NULL.
**/
EFI_STATUS
EFIAPI
EfiBootManagerFreeDriverHealthInfo (
  EFI_BOOT_MANAGER_DRIVER_HEALTH_INFO *DriverHealthInfo,
  UINTN                               Count
  )
{
  UINTN                               Index;

  for (Index = 0; Index < Count; Index++) {
    if (DriverHealthInfo[Index].MessageList != NULL) {
      FreePool (DriverHealthInfo[Index].MessageList);
    }
  }
  return gBS->FreePool (DriverHealthInfo);
}

/**
  Repair all the controllers according to the Driver Health status queried.

  @param ReconnectRepairCount     To record the number of recursive call of
                                  this function itself.
**/
VOID
BmRepairAllControllers (
  UINTN       ReconnectRepairCount
  )
{
  EFI_STATUS                          Status;
  EFI_BOOT_MANAGER_DRIVER_HEALTH_INFO *DriverHealthInfo;
  EFI_DRIVER_HEALTH_STATUS            HealthStatus;
  UINTN                               Count;
  UINTN                               Index;
  BOOLEAN                             RepairRequired;
  BOOLEAN                             ConfigurationRequired;
  BOOLEAN                             ReconnectRequired;
  BOOLEAN                             RebootRequired;
  EFI_HII_HANDLE                      *HiiHandles;
  EFI_FORM_BROWSER2_PROTOCOL          *FormBrowser2;
  UINT32                              MaxRepairCount;
  UINT32                              RepairCount;

  //
  // Configure PcdDriverHealthConfigureForm to ZeroGuid to disable driver health check.
  //
  if (IsZeroGuid (PcdGetPtr (PcdDriverHealthConfigureForm))) {
    return;
  }

  Status = gBS->LocateProtocol (&gEfiFormBrowser2ProtocolGuid, NULL, (VOID **) &FormBrowser2);
  ASSERT_EFI_ERROR (Status);

  MaxRepairCount = PcdGet32 (PcdMaxRepairCount);
  RepairCount = 0;

  do {
    RepairRequired        = FALSE;
    ConfigurationRequired = FALSE;

    //
    // Deal with Repair Required
    //
    DriverHealthInfo = EfiBootManagerGetDriverHealthInfo (&Count);
    for (Index = 0; Index < Count; Index++) {
      if (DriverHealthInfo[Index].HealthStatus == EfiDriverHealthStatusConfigurationRequired) {
        ConfigurationRequired = TRUE;
      }

      if (DriverHealthInfo[Index].HealthStatus == EfiDriverHealthStatusRepairRequired) {
        RepairRequired        = TRUE;

        BmDisplayMessages (&DriverHealthInfo[Index]);

        Status = DriverHealthInfo[Index].DriverHealth->Repair (
                                                         DriverHealthInfo[Index].DriverHealth,
                                                         DriverHealthInfo[Index].ControllerHandle,
                                                         DriverHealthInfo[Index].ChildHandle,
                                                         BmRepairNotify
                                                         );
        if (!EFI_ERROR (Status) && !ConfigurationRequired) {
          Status = DriverHealthInfo[Index].DriverHealth->GetHealthStatus (
                                                           DriverHealthInfo[Index].DriverHealth,
                                                           DriverHealthInfo[Index].ControllerHandle,
                                                           DriverHealthInfo[Index].ChildHandle,
                                                           &HealthStatus,
                                                           NULL,
                                                           NULL
                                                           );
          if (!EFI_ERROR (Status) && (HealthStatus == EfiDriverHealthStatusConfigurationRequired)) {
            ConfigurationRequired = TRUE;
          }
        }
      }
    }

    if (ConfigurationRequired) {
      HiiHandles = HiiGetHiiHandles (NULL);
      if (HiiHandles != NULL) {
        for (Index = 0; HiiHandles[Index] != NULL; Index++) {
          Status = FormBrowser2->SendForm (
                                   FormBrowser2,
                                   &HiiHandles[Index],
                                   1,
                                   PcdGetPtr (PcdDriverHealthConfigureForm),
                                   0,
                                   NULL,
                                   NULL
                                   );
          if (!EFI_ERROR (Status)) {
            break;
          }
        }
        FreePool (HiiHandles);
      }
    }

    EfiBootManagerFreeDriverHealthInfo (DriverHealthInfo, Count);
    RepairCount++;
  } while ((RepairRequired || ConfigurationRequired) && ((MaxRepairCount == 0) || (RepairCount < MaxRepairCount)));

  RebootRequired    = FALSE;
  ReconnectRequired = FALSE;
  DriverHealthInfo  = EfiBootManagerGetDriverHealthInfo (&Count);
  for (Index = 0; Index < Count; Index++) {

    BmDisplayMessages (&DriverHealthInfo[Index]);

    if (DriverHealthInfo[Index].HealthStatus == EfiDriverHealthStatusReconnectRequired) {
      Status = gBS->DisconnectController (DriverHealthInfo[Index].ControllerHandle, NULL, NULL);
      if (EFI_ERROR (Status)) {
        //
        // Disconnect failed. Need to promote reconnect to a reboot.
        //
        RebootRequired    = TRUE;
      } else {
        gBS->ConnectController (DriverHealthInfo[Index].ControllerHandle, NULL, NULL, TRUE);
        ReconnectRequired = TRUE;
      }
    }

    if (DriverHealthInfo[Index].HealthStatus == EfiDriverHealthStatusRebootRequired) {
      RebootRequired      = TRUE;
    }
  }
  EfiBootManagerFreeDriverHealthInfo (DriverHealthInfo, Count);


  DEBUG_CODE (
    CHAR16 *ControllerName;

    DriverHealthInfo = EfiBootManagerGetDriverHealthInfo (&Count);
    for (Index = 0; Index < Count; Index++) {
      ControllerName = BmGetControllerName (
                         DriverHealthInfo[Index].DriverHealthHandle,
                         DriverHealthInfo[Index].ControllerHandle,
                         DriverHealthInfo[Index].ChildHandle
                         );
      DEBUG ((
        EFI_D_INFO,
        "%02d: %s - %s\n",
        Index,
        ControllerName,
        mBmHealthStatusText[DriverHealthInfo[Index].HealthStatus]
        ));
      if (ControllerName != NULL) {
        FreePool (ControllerName);
      }
    }
    EfiBootManagerFreeDriverHealthInfo (DriverHealthInfo, Count);
    );

  if (ReconnectRequired) {
    if (ReconnectRepairCount < MAX_RECONNECT_REPAIR) {
      BmRepairAllControllers (ReconnectRepairCount + 1);
    } else {
      DEBUG ((DEBUG_ERROR, "[%a:%d] Repair failed after %d retries.\n",
        __FUNCTION__, __LINE__, ReconnectRepairCount));
    }
  }

  if (RebootRequired) {
    DEBUG ((EFI_D_INFO, "[BDS] One of the Driver Health instances requires rebooting.\n"));
    gRT->ResetSystem (EfiResetWarm, EFI_SUCCESS, 0, NULL);
  }
}
