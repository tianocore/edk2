/** @file
  Support functions to connect/disconnect UEFI Driver model Protocol

Copyright (c) 2006 - 2012, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "DxeMain.h"
#include "Handle.h"


//
// Driver Support Functions
//
/**
  Connects one or more drivers to a controller.

  @param  ControllerHandle      The handle of the controller to which driver(s) are to be connected.
  @param  DriverImageHandle     A pointer to an ordered list handles that support the
                                EFI_DRIVER_BINDING_PROTOCOL.
  @param  RemainingDevicePath   A pointer to the device path that specifies a child of the
                                controller specified by ControllerHandle.
  @param  Recursive             If TRUE, then ConnectController() is called recursively
                                until the entire tree of controllers below the controller specified
                                by ControllerHandle have been created. If FALSE, then
                                the tree of controllers is only expanded one level.

  @retval EFI_SUCCESS           1) One or more drivers were connected to ControllerHandle.
                                2) No drivers were connected to ControllerHandle, but
                                RemainingDevicePath is not NULL, and it is an End Device
                                Path Node.
  @retval EFI_INVALID_PARAMETER ControllerHandle is NULL.
  @retval EFI_NOT_FOUND         1) There are no EFI_DRIVER_BINDING_PROTOCOL instances
                                present in the system.
                                2) No drivers were connected to ControllerHandle.
  @retval EFI_SECURITY_VIOLATION 
                                The user has no permission to start UEFI device drivers on the device path 
                                associated with the ControllerHandle or specified by the RemainingDevicePath.

**/
EFI_STATUS
EFIAPI
CoreConnectController (
  IN  EFI_HANDLE                ControllerHandle,
  IN  EFI_HANDLE                *DriverImageHandle    OPTIONAL,
  IN  EFI_DEVICE_PATH_PROTOCOL  *RemainingDevicePath  OPTIONAL,
  IN  BOOLEAN                   Recursive
  )
{
  EFI_STATUS                           Status;
  EFI_STATUS                           ReturnStatus;
  IHANDLE                              *Handle;
  PROTOCOL_INTERFACE                   *Prot;
  LIST_ENTRY                           *Link;
  LIST_ENTRY                           *ProtLink;
  OPEN_PROTOCOL_DATA                   *OpenData;
  EFI_DEVICE_PATH_PROTOCOL             *AlignedRemainingDevicePath;
  EFI_HANDLE                           *ChildHandleBuffer;
  UINTN                                ChildHandleCount;
  UINTN                                Index;
  UINTN                                HandleFilePathSize;
  UINTN                                RemainingDevicePathSize;
  EFI_DEVICE_PATH_PROTOCOL             *HandleFilePath;
  EFI_DEVICE_PATH_PROTOCOL             *FilePath;
  EFI_DEVICE_PATH_PROTOCOL             *TempFilePath;

  //
  // Make sure ControllerHandle is valid
  //
  Status = CoreValidateHandle (ControllerHandle);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (gSecurity2 != NULL) {
    //
    // Check whether the user has permission to start UEFI device drivers.
    //
    Status = CoreHandleProtocol (ControllerHandle, &gEfiDevicePathProtocolGuid, (VOID **)&HandleFilePath);
    if (!EFI_ERROR (Status)) {
      FilePath     = HandleFilePath;
      TempFilePath = NULL;
      if (RemainingDevicePath != NULL && !Recursive) {
        HandleFilePathSize      = GetDevicePathSize (HandleFilePath) - sizeof (EFI_DEVICE_PATH_PROTOCOL);
        RemainingDevicePathSize = GetDevicePathSize (RemainingDevicePath);
        TempFilePath = AllocateZeroPool (HandleFilePathSize + RemainingDevicePathSize);
        ASSERT (TempFilePath != NULL);
        CopyMem (TempFilePath, HandleFilePath, HandleFilePathSize);
        CopyMem ((UINT8 *) TempFilePath + HandleFilePathSize, RemainingDevicePath, RemainingDevicePathSize);
        FilePath = TempFilePath;
      }
      Status = gSecurity2->FileAuthentication (
                            gSecurity2,
                            FilePath,
                            NULL,
                            0,
                            FALSE
                            );
      if (TempFilePath != NULL) {
        FreePool (TempFilePath);
      }
      if (EFI_ERROR (Status)) {
        return Status;
      }
    }
  }
  
  Handle = ControllerHandle;

  //
  // Make a copy of RemainingDevicePath to guanatee it is aligned
  //
  AlignedRemainingDevicePath = NULL;
  if (RemainingDevicePath != NULL) {
    AlignedRemainingDevicePath = DuplicateDevicePath (RemainingDevicePath);

    if (AlignedRemainingDevicePath == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }
  }

  //
  // Connect all drivers to ControllerHandle
  // If CoreConnectSingleController returns EFI_NOT_READY, then the number of
  // Driver Binding Protocols in the handle database has increased during the call
  // so the connect operation must be restarted
  //
  do {
    ReturnStatus = CoreConnectSingleController (
                     ControllerHandle,
                     DriverImageHandle,
                     AlignedRemainingDevicePath
                     );
  } while (ReturnStatus == EFI_NOT_READY);

  //
  // Free the aligned copy of RemainingDevicePath
  //
  if (AlignedRemainingDevicePath != NULL) {
    CoreFreePool (AlignedRemainingDevicePath);
  }

  //
  // If recursive, then connect all drivers to all of ControllerHandle's children
  //
  if (Recursive) {
    //
    // Acquire the protocol lock on the handle database so the child handles can be collected
    //
    CoreAcquireProtocolLock ();

    //
    // Make sure the DriverBindingHandle is valid
    //
    Status = CoreValidateHandle (ControllerHandle);
    if (EFI_ERROR (Status)) {
      //
      // Release the protocol lock on the handle database
      //
      CoreReleaseProtocolLock ();

      return ReturnStatus;
    }


    //
    // Count ControllerHandle's children
    //
    for (Link = Handle->Protocols.ForwardLink, ChildHandleCount = 0; Link != &Handle->Protocols; Link = Link->ForwardLink) {
      Prot = CR(Link, PROTOCOL_INTERFACE, Link, PROTOCOL_INTERFACE_SIGNATURE);
      for (ProtLink = Prot->OpenList.ForwardLink;
          ProtLink != &Prot->OpenList;
          ProtLink = ProtLink->ForwardLink) {
        OpenData = CR (ProtLink, OPEN_PROTOCOL_DATA, Link, OPEN_PROTOCOL_DATA_SIGNATURE);
        if ((OpenData->Attributes & EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER) != 0) {
          ChildHandleCount++;
        }
      }
    }

    //
    // Allocate a handle buffer for ControllerHandle's children
    //
    ChildHandleBuffer = AllocatePool (ChildHandleCount * sizeof(EFI_HANDLE));
    if (ChildHandleBuffer == NULL) {
      CoreReleaseProtocolLock ();
      return EFI_OUT_OF_RESOURCES;
    }

    //
    // Fill in a handle buffer with ControllerHandle's children
    //
    for (Link = Handle->Protocols.ForwardLink, ChildHandleCount = 0; Link != &Handle->Protocols; Link = Link->ForwardLink) {
      Prot = CR(Link, PROTOCOL_INTERFACE, Link, PROTOCOL_INTERFACE_SIGNATURE);
      for (ProtLink = Prot->OpenList.ForwardLink;
          ProtLink != &Prot->OpenList;
          ProtLink = ProtLink->ForwardLink) {
        OpenData = CR (ProtLink, OPEN_PROTOCOL_DATA, Link, OPEN_PROTOCOL_DATA_SIGNATURE);
        if ((OpenData->Attributes & EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER) != 0) {
          ChildHandleBuffer[ChildHandleCount] = OpenData->ControllerHandle;
          ChildHandleCount++;
        }
      }
    }

    //
    // Release the protocol lock on the handle database
    //
    CoreReleaseProtocolLock ();

    //
    // Recursively connect each child handle
    //
    for (Index = 0; Index < ChildHandleCount; Index++) {
      CoreConnectController (
        ChildHandleBuffer[Index],
        NULL,
        NULL,
        TRUE
        );
    }

    //
    // Free the handle buffer of ControllerHandle's children
    //
    CoreFreePool (ChildHandleBuffer);
  }

  return ReturnStatus;
}


/**
  Add Driver Binding Protocols from Context Driver Image Handles to sorted
  Driver Binding Protocol list.

  @param  DriverBindingHandle                   Handle of the driver binding
                                                protocol.
  @param  NumberOfSortedDriverBindingProtocols  Number Of sorted driver binding
                                                protocols
  @param  SortedDriverBindingProtocols          The sorted protocol list.
  @param  DriverBindingHandleCount              Driver Binding Handle Count.
  @param  DriverBindingHandleBuffer             The buffer of driver binding
                                                protocol to be modified.
  @param  IsImageHandle                         Indicate whether
                                                DriverBindingHandle is an image
                                                handle

  @return None.

**/
VOID
AddSortedDriverBindingProtocol (
  IN      EFI_HANDLE                   DriverBindingHandle,
  IN OUT  UINTN                        *NumberOfSortedDriverBindingProtocols,
  IN OUT  EFI_DRIVER_BINDING_PROTOCOL  **SortedDriverBindingProtocols,
  IN      UINTN                        DriverBindingHandleCount,
  IN OUT  EFI_HANDLE                   *DriverBindingHandleBuffer,
  IN      BOOLEAN                      IsImageHandle
  )
{
  EFI_STATUS                   Status;
  EFI_DRIVER_BINDING_PROTOCOL  *DriverBinding;
  UINTN                        Index;

  //
  // Make sure the DriverBindingHandle is valid
  //
  Status = CoreValidateHandle (DriverBindingHandle);
  if (EFI_ERROR (Status)) {
    return;
  }

  //
  // If IsImageHandle is TRUE, then DriverBindingHandle is an image handle
  // Find all the DriverBindingHandles associated with that image handle and add them to the sorted list
  //
  if (IsImageHandle) {
    //
    // Loop through all the Driver Binding Handles
    //
    for (Index = 0; Index < DriverBindingHandleCount; Index++) {
      //
      // Retrieve the Driver Binding Protocol associated with each Driver Binding Handle
      //
      Status = CoreHandleProtocol (
                DriverBindingHandleBuffer[Index],
                &gEfiDriverBindingProtocolGuid,
                (VOID **) &DriverBinding
                );
      if (EFI_ERROR (Status) || DriverBinding == NULL) {
        continue;
      }

      //
      // If the ImageHandle associated with DriverBinding matches DriverBindingHandle,
      // then add the DriverBindingProtocol[Index] to the sorted list
      //
      if (DriverBinding->ImageHandle == DriverBindingHandle) {
        AddSortedDriverBindingProtocol (
          DriverBindingHandleBuffer[Index],
          NumberOfSortedDriverBindingProtocols,
          SortedDriverBindingProtocols,
          DriverBindingHandleCount,
          DriverBindingHandleBuffer,
          FALSE
          );
      }
    }
    return;
  }

  //
  // Retrieve the Driver Binding Protocol from DriverBindingHandle
  //
  Status = CoreHandleProtocol(
             DriverBindingHandle,
             &gEfiDriverBindingProtocolGuid,
             (VOID **) &DriverBinding
             );
  //
  // If DriverBindingHandle does not support the Driver Binding Protocol then return
  //
  if (EFI_ERROR (Status) || DriverBinding == NULL) {
    return;
  }

  //
  // See if DriverBinding is already in the sorted list
  //
  for (Index = 0; Index < *NumberOfSortedDriverBindingProtocols && Index < DriverBindingHandleCount; Index++) {
    if (DriverBinding == SortedDriverBindingProtocols[Index]) {
      return;
    }
  }

  //
  // Add DriverBinding to the end of the list
  //
  if (*NumberOfSortedDriverBindingProtocols < DriverBindingHandleCount) {
    SortedDriverBindingProtocols[*NumberOfSortedDriverBindingProtocols] = DriverBinding;
  }
  *NumberOfSortedDriverBindingProtocols = *NumberOfSortedDriverBindingProtocols + 1;

  //
  // Mark the cooresponding handle in DriverBindingHandleBuffer as used
  //
  for (Index = 0; Index < DriverBindingHandleCount; Index++) {
    if (DriverBindingHandleBuffer[Index] == DriverBindingHandle) {
      DriverBindingHandleBuffer[Index] = NULL;
    }
  }
}


/**
  Connects a controller to a driver.

  @param  ControllerHandle                      Handle of the controller to be
                                                connected.
  @param  ContextDriverImageHandles             DriverImageHandle A pointer to an
                                                ordered list of driver image
                                                handles.
  @param  RemainingDevicePath                   RemainingDevicePath A pointer to
                                                the device path that specifies a
                                                child  of the controller
                                                specified by ControllerHandle.

  @retval EFI_SUCCESS                           One or more drivers were
                                                connected to ControllerHandle.
  @retval EFI_OUT_OF_RESOURCES                  No enough system resources to
                                                complete the request.
  @retval EFI_NOT_FOUND                         No drivers were connected to
                                                ControllerHandle.

**/
EFI_STATUS
CoreConnectSingleController (
  IN  EFI_HANDLE                ControllerHandle,
  IN  EFI_HANDLE                *ContextDriverImageHandles OPTIONAL,
  IN  EFI_DEVICE_PATH_PROTOCOL  *RemainingDevicePath       OPTIONAL
  )
{
  EFI_STATUS                                 Status;
  UINTN                                      Index;
  EFI_HANDLE                                 DriverImageHandle;
  EFI_PLATFORM_DRIVER_OVERRIDE_PROTOCOL      *PlatformDriverOverride;
  EFI_BUS_SPECIFIC_DRIVER_OVERRIDE_PROTOCOL  *BusSpecificDriverOverride;
  UINTN                                      DriverBindingHandleCount;
  EFI_HANDLE                                 *DriverBindingHandleBuffer;
  UINTN                                      NewDriverBindingHandleCount;
  EFI_HANDLE                                 *NewDriverBindingHandleBuffer;
  EFI_DRIVER_BINDING_PROTOCOL                *DriverBinding;
  EFI_DRIVER_FAMILY_OVERRIDE_PROTOCOL        *DriverFamilyOverride;
  UINTN                                      NumberOfSortedDriverBindingProtocols;
  EFI_DRIVER_BINDING_PROTOCOL                **SortedDriverBindingProtocols;
  UINT32                                     DriverFamilyOverrideVersion;
  UINT32                                     HighestVersion;
  UINTN                                      HighestIndex;
  UINTN                                      SortIndex;
  BOOLEAN                                    OneStarted;
  BOOLEAN                                    DriverFound;

  //
  // Initialize local variables
  //
  DriverBindingHandleCount              = 0;
  DriverBindingHandleBuffer             = NULL;
  NumberOfSortedDriverBindingProtocols  = 0;
  SortedDriverBindingProtocols          = NULL;

  //
  // Get list of all Driver Binding Protocol Instances
  //
  Status = CoreLocateHandleBuffer (
             ByProtocol,
             &gEfiDriverBindingProtocolGuid,
             NULL,
             &DriverBindingHandleCount,
             &DriverBindingHandleBuffer
             );
  if (EFI_ERROR (Status) || (DriverBindingHandleCount == 0)) {
    return EFI_NOT_FOUND;
  }

  //
  // Allocate a duplicate array for the sorted Driver Binding Protocol Instances
  //
  SortedDriverBindingProtocols = AllocatePool (sizeof (VOID *) * DriverBindingHandleCount);
  if (SortedDriverBindingProtocols == NULL) {
    CoreFreePool (DriverBindingHandleBuffer);
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Add Driver Binding Protocols from Context Driver Image Handles first
  //
  if (ContextDriverImageHandles != NULL) {
    for (Index = 0; ContextDriverImageHandles[Index] != NULL; Index++) {
      AddSortedDriverBindingProtocol (
        ContextDriverImageHandles[Index],
        &NumberOfSortedDriverBindingProtocols,
        SortedDriverBindingProtocols,
        DriverBindingHandleCount,
        DriverBindingHandleBuffer,
        FALSE
        );
    }
  }

  //
  // Add the Platform Driver Override Protocol drivers for ControllerHandle next
  //
  Status = CoreLocateProtocol (
             &gEfiPlatformDriverOverrideProtocolGuid,
             NULL,
             (VOID **) &PlatformDriverOverride
             );
  if (!EFI_ERROR (Status) && (PlatformDriverOverride != NULL)) {
    DriverImageHandle = NULL;
    do {
      Status = PlatformDriverOverride->GetDriver (
                                         PlatformDriverOverride,
                                         ControllerHandle,
                                         &DriverImageHandle
                                         );
      if (!EFI_ERROR (Status)) {
        AddSortedDriverBindingProtocol (
          DriverImageHandle,
          &NumberOfSortedDriverBindingProtocols,
          SortedDriverBindingProtocols,
          DriverBindingHandleCount,
          DriverBindingHandleBuffer,
          TRUE
          );
      }
    } while (!EFI_ERROR (Status));
  }

  //
  // Add the Driver Family Override Protocol drivers for ControllerHandle
  //
  while (TRUE) {
    HighestIndex   = DriverBindingHandleCount;
    HighestVersion = 0;
    for (Index = 0; Index < DriverBindingHandleCount; Index++) {
      Status = CoreHandleProtocol (
                 DriverBindingHandleBuffer[Index],
                 &gEfiDriverFamilyOverrideProtocolGuid,
                 (VOID **) &DriverFamilyOverride
                 );
      if (!EFI_ERROR (Status) && (DriverFamilyOverride != NULL)) {
        DriverFamilyOverrideVersion = DriverFamilyOverride->GetVersion (DriverFamilyOverride);
        if ((HighestIndex == DriverBindingHandleCount) || (DriverFamilyOverrideVersion > HighestVersion)) {
          HighestVersion = DriverFamilyOverrideVersion;
          HighestIndex   = Index;
        }
      }
    }

    if (HighestIndex == DriverBindingHandleCount) {
      break;
    }

    AddSortedDriverBindingProtocol (
      DriverBindingHandleBuffer[HighestIndex],
      &NumberOfSortedDriverBindingProtocols,
      SortedDriverBindingProtocols,
      DriverBindingHandleCount,
      DriverBindingHandleBuffer,
      FALSE
      );
  }

  //
  // Get the Bus Specific Driver Override Protocol instance on the Controller Handle
  //
  Status = CoreHandleProtocol (
             ControllerHandle,
             &gEfiBusSpecificDriverOverrideProtocolGuid,
             (VOID **) &BusSpecificDriverOverride
             );
  if (!EFI_ERROR (Status) && (BusSpecificDriverOverride != NULL)) {
    DriverImageHandle = NULL;
    do {
      Status = BusSpecificDriverOverride->GetDriver (
                                            BusSpecificDriverOverride,
                                            &DriverImageHandle
                                            );
      if (!EFI_ERROR (Status)) {
        AddSortedDriverBindingProtocol (
          DriverImageHandle,
          &NumberOfSortedDriverBindingProtocols,
          SortedDriverBindingProtocols,
          DriverBindingHandleCount,
          DriverBindingHandleBuffer,
          TRUE
          );
      }
    } while (!EFI_ERROR (Status));
  }

  //
  // Then add all the remaining Driver Binding Protocols
  //
  SortIndex = NumberOfSortedDriverBindingProtocols;
  for (Index = 0; Index < DriverBindingHandleCount; Index++) {
    AddSortedDriverBindingProtocol (
      DriverBindingHandleBuffer[Index],
      &NumberOfSortedDriverBindingProtocols,
      SortedDriverBindingProtocols,
      DriverBindingHandleCount,
      DriverBindingHandleBuffer,
      FALSE
      );
  }

  //
  // Free the Driver Binding Handle Buffer
  //
  CoreFreePool (DriverBindingHandleBuffer);

  //
  // If the number of Driver Binding Protocols has increased since this function started, then return
  // EFI_NOT_READY, so it will be restarted
  //
  Status = CoreLocateHandleBuffer (
             ByProtocol,
             &gEfiDriverBindingProtocolGuid,
             NULL,
             &NewDriverBindingHandleCount,
             &NewDriverBindingHandleBuffer
             );
  CoreFreePool (NewDriverBindingHandleBuffer);
  if (NewDriverBindingHandleCount > DriverBindingHandleCount) {
    //
    // Free any buffers that were allocated with AllocatePool()
    //
    CoreFreePool (SortedDriverBindingProtocols);

    return EFI_NOT_READY;
  }

  //
  // Sort the remaining DriverBinding Protocol based on their Version field from
  // highest to lowest.
  //
  for ( ; SortIndex < NumberOfSortedDriverBindingProtocols; SortIndex++) {
    HighestVersion = SortedDriverBindingProtocols[SortIndex]->Version;
    HighestIndex   = SortIndex;
    for (Index = SortIndex + 1; Index < NumberOfSortedDriverBindingProtocols; Index++) {
      if (SortedDriverBindingProtocols[Index]->Version > HighestVersion) {
        HighestVersion = SortedDriverBindingProtocols[Index]->Version;
        HighestIndex   = Index;
      }
    }
    if (SortIndex != HighestIndex) {
      DriverBinding = SortedDriverBindingProtocols[SortIndex];
      SortedDriverBindingProtocols[SortIndex] = SortedDriverBindingProtocols[HighestIndex];
      SortedDriverBindingProtocols[HighestIndex] = DriverBinding;
    }
  }

  //
  // Loop until no more drivers can be started on ControllerHandle
  //
  OneStarted = FALSE;
  do {

    //
    // Loop through the sorted Driver Binding Protocol Instances in order, and see if
    // any of the Driver Binding Protocols support the controller specified by
    // ControllerHandle.
    //
    DriverBinding = NULL;
    DriverFound = FALSE;
    for (Index = 0; (Index < NumberOfSortedDriverBindingProtocols) && !DriverFound; Index++) {
      if (SortedDriverBindingProtocols[Index] != NULL) {
        DriverBinding = SortedDriverBindingProtocols[Index];
        PERF_START (DriverBinding->DriverBindingHandle, "DB:Support:", NULL, 0);
        Status = DriverBinding->Supported(
                                  DriverBinding,
                                  ControllerHandle,
                                  RemainingDevicePath
                                  );
        PERF_END (DriverBinding->DriverBindingHandle, "DB:Support:", NULL, 0);
        if (!EFI_ERROR (Status)) {
          SortedDriverBindingProtocols[Index] = NULL;
          DriverFound = TRUE;

          //
          // A driver was found that supports ControllerHandle, so attempt to start the driver
          // on ControllerHandle.
          //
          PERF_START (DriverBinding->DriverBindingHandle, "DB:Start:", NULL, 0);
          Status = DriverBinding->Start (
                                    DriverBinding,
                                    ControllerHandle,
                                    RemainingDevicePath
                                    );
          PERF_END (DriverBinding->DriverBindingHandle, "DB:Start:", NULL, 0);

          if (!EFI_ERROR (Status)) {
            //
            // The driver was successfully started on ControllerHandle, so set a flag
            //
            OneStarted = TRUE;
          }
        }
      }
    }
  } while (DriverFound);

  //
  // Free any buffers that were allocated with AllocatePool()
  //
  CoreFreePool (SortedDriverBindingProtocols);

  //
  // If at least one driver was started on ControllerHandle, then return EFI_SUCCESS.
  //
  if (OneStarted) {
    return EFI_SUCCESS;
  }

  //
  // If no drivers started and RemainingDevicePath is an End Device Path Node, then return EFI_SUCCESS
  //
  if (RemainingDevicePath != NULL) {
    if (IsDevicePathEnd (RemainingDevicePath)) {
      return EFI_SUCCESS;
    }
  }

  //
  // Otherwise, no drivers were started on ControllerHandle, so return EFI_NOT_FOUND
  //
  return EFI_NOT_FOUND;
}



/**
  Disonnects a controller from a driver

  @param  ControllerHandle                      ControllerHandle The handle of
                                                the controller from which
                                                driver(s)  are to be
                                                disconnected.
  @param  DriverImageHandle                     DriverImageHandle The driver to
                                                disconnect from ControllerHandle.
  @param  ChildHandle                           ChildHandle The handle of the
                                                child to destroy.

  @retval EFI_SUCCESS                           One or more drivers were
                                                disconnected from the controller.
  @retval EFI_SUCCESS                           On entry, no drivers are managing
                                                ControllerHandle.
  @retval EFI_SUCCESS                           DriverImageHandle is not NULL,
                                                and on entry DriverImageHandle is
                                                not managing ControllerHandle.
  @retval EFI_INVALID_PARAMETER                 ControllerHandle is NULL.
  @retval EFI_INVALID_PARAMETER                 DriverImageHandle is not NULL,
                                                and it is not a valid EFI_HANDLE.
  @retval EFI_INVALID_PARAMETER                 ChildHandle is not NULL, and it
                                                is not a valid EFI_HANDLE.
  @retval EFI_OUT_OF_RESOURCES                  There are not enough resources
                                                available to disconnect any
                                                drivers from ControllerHandle.
  @retval EFI_DEVICE_ERROR                      The controller could not be
                                                disconnected because of a device
                                                error.

**/
EFI_STATUS
EFIAPI
CoreDisconnectController (
  IN  EFI_HANDLE  ControllerHandle,
  IN  EFI_HANDLE  DriverImageHandle  OPTIONAL,
  IN  EFI_HANDLE  ChildHandle        OPTIONAL
  )
{
  EFI_STATUS                          Status;
  IHANDLE                             *Handle;
  EFI_HANDLE                          *DriverImageHandleBuffer;
  EFI_HANDLE                          *ChildBuffer;
  UINTN                               Index;
  UINTN                               HandleIndex;
  UINTN                               DriverImageHandleCount;
  UINTN                               ChildrenToStop;
  UINTN                               ChildBufferCount;
  UINTN                               StopCount;
  BOOLEAN                             Duplicate;
  BOOLEAN                             ChildHandleValid;
  BOOLEAN                             DriverImageHandleValid;
  LIST_ENTRY                          *Link;
  LIST_ENTRY                          *ProtLink;
  OPEN_PROTOCOL_DATA                  *OpenData;
  PROTOCOL_INTERFACE                  *Prot;
  EFI_DRIVER_BINDING_PROTOCOL         *DriverBinding;

  //
  // Make sure ControllerHandle is valid
  //
  Status = CoreValidateHandle (ControllerHandle);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Make sure ChildHandle is valid if it is not NULL
  //
  if (ChildHandle != NULL) {
    Status = CoreValidateHandle (ChildHandle);
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  Handle = ControllerHandle;

  //
  // Get list of drivers that are currently managing ControllerHandle
  //
  DriverImageHandleBuffer = NULL;
  DriverImageHandleCount  = 1;

  if (DriverImageHandle == NULL) {
    //
    // Look at each protocol interface for a match
    //
    DriverImageHandleCount = 0;

    CoreAcquireProtocolLock ();
    for (Link = Handle->Protocols.ForwardLink; Link != &Handle->Protocols; Link = Link->ForwardLink) {
      Prot = CR(Link, PROTOCOL_INTERFACE, Link, PROTOCOL_INTERFACE_SIGNATURE);
      for (ProtLink = Prot->OpenList.ForwardLink;
           ProtLink != &Prot->OpenList;
           ProtLink = ProtLink->ForwardLink) {
        OpenData = CR (ProtLink, OPEN_PROTOCOL_DATA, Link, OPEN_PROTOCOL_DATA_SIGNATURE);
        if ((OpenData->Attributes & EFI_OPEN_PROTOCOL_BY_DRIVER) != 0) {
          DriverImageHandleCount++;
        }
      }
    }
    CoreReleaseProtocolLock ();

    //
    // If there are no drivers managing this controller, then return EFI_SUCCESS
    //
    if (DriverImageHandleCount == 0) {
      Status = EFI_SUCCESS;
      goto Done;
    }

    DriverImageHandleBuffer = AllocatePool (sizeof (EFI_HANDLE) * DriverImageHandleCount);
    if (DriverImageHandleBuffer == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
      goto Done;
    }

    DriverImageHandleCount = 0;

    CoreAcquireProtocolLock ();
    for (Link = Handle->Protocols.ForwardLink; Link != &Handle->Protocols; Link = Link->ForwardLink) {
      Prot = CR(Link, PROTOCOL_INTERFACE, Link, PROTOCOL_INTERFACE_SIGNATURE);
      for (ProtLink = Prot->OpenList.ForwardLink;
           ProtLink != &Prot->OpenList;
           ProtLink = ProtLink->ForwardLink) {
        OpenData = CR (ProtLink, OPEN_PROTOCOL_DATA, Link, OPEN_PROTOCOL_DATA_SIGNATURE);
        if ((OpenData->Attributes & EFI_OPEN_PROTOCOL_BY_DRIVER) != 0) {
          Duplicate = FALSE;
          for (Index = 0; Index< DriverImageHandleCount; Index++) {
            if (DriverImageHandleBuffer[Index] == OpenData->AgentHandle) {
              Duplicate = TRUE;
              break;
            }
          }
          if (!Duplicate) {
            DriverImageHandleBuffer[DriverImageHandleCount] = OpenData->AgentHandle;
            DriverImageHandleCount++;
          }
        }
      }
    }
    CoreReleaseProtocolLock ();
  }

  StopCount = 0;
  for (HandleIndex = 0; HandleIndex < DriverImageHandleCount; HandleIndex++) {

    if (DriverImageHandleBuffer != NULL) {
      DriverImageHandle = DriverImageHandleBuffer[HandleIndex];
    }

    //
    // Get the Driver Binding Protocol of the driver that is managing this controller
    //
    Status = CoreHandleProtocol (
               DriverImageHandle,
               &gEfiDriverBindingProtocolGuid,
               (VOID **)&DriverBinding
               );
    if (EFI_ERROR (Status) || DriverBinding == NULL) {
      Status = EFI_INVALID_PARAMETER;
      goto Done;
    }

    //
    // Look at each protocol interface for a match
    //
    DriverImageHandleValid = FALSE;
    ChildBufferCount = 0;

    CoreAcquireProtocolLock ();
    for (Link = Handle->Protocols.ForwardLink; Link != &Handle->Protocols; Link = Link->ForwardLink) {
      Prot = CR(Link, PROTOCOL_INTERFACE, Link, PROTOCOL_INTERFACE_SIGNATURE);
      for (ProtLink = Prot->OpenList.ForwardLink;
           ProtLink != &Prot->OpenList;
           ProtLink = ProtLink->ForwardLink) {
        OpenData = CR (ProtLink, OPEN_PROTOCOL_DATA, Link, OPEN_PROTOCOL_DATA_SIGNATURE);
        if (OpenData->AgentHandle == DriverImageHandle) {
          if ((OpenData->Attributes & EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER) != 0) {
            ChildBufferCount++;
          }
          if ((OpenData->Attributes & EFI_OPEN_PROTOCOL_BY_DRIVER) != 0) {
            DriverImageHandleValid = TRUE;
          }
        }
      }
    }
    CoreReleaseProtocolLock ();

    if (DriverImageHandleValid) {
      ChildHandleValid = FALSE;
      ChildBuffer = NULL;
      if (ChildBufferCount != 0) {
        ChildBuffer = AllocatePool (sizeof (EFI_HANDLE) * ChildBufferCount);
        if (ChildBuffer == NULL) {
          Status = EFI_OUT_OF_RESOURCES;
          goto Done;
        }

        ChildBufferCount = 0;

        CoreAcquireProtocolLock ();
        for (Link = Handle->Protocols.ForwardLink; Link != &Handle->Protocols; Link = Link->ForwardLink) {
          Prot = CR(Link, PROTOCOL_INTERFACE, Link, PROTOCOL_INTERFACE_SIGNATURE);
          for (ProtLink = Prot->OpenList.ForwardLink;
               ProtLink != &Prot->OpenList;
               ProtLink = ProtLink->ForwardLink) {
            OpenData = CR (ProtLink, OPEN_PROTOCOL_DATA, Link, OPEN_PROTOCOL_DATA_SIGNATURE);
            if ((OpenData->AgentHandle == DriverImageHandle) &&
                ((OpenData->Attributes & EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER) != 0)) {
              Duplicate = FALSE;
              for (Index = 0; Index < ChildBufferCount; Index++) {
                if (ChildBuffer[Index] == OpenData->ControllerHandle) {
                  Duplicate = TRUE;
                  break;
                }
              }
              if (!Duplicate) {
                ChildBuffer[ChildBufferCount] = OpenData->ControllerHandle;
                if (ChildHandle == ChildBuffer[ChildBufferCount]) {
                  ChildHandleValid = TRUE;
                }
                ChildBufferCount++;
              }
            }
          }
        }
        CoreReleaseProtocolLock ();
      }

      if (ChildHandle == NULL || ChildHandleValid) {
        ChildrenToStop = 0;
        Status = EFI_SUCCESS;
        if (ChildBufferCount > 0) {
          if (ChildHandle != NULL) {
            ChildrenToStop = 1;
            Status = DriverBinding->Stop (DriverBinding, ControllerHandle, ChildrenToStop, &ChildHandle);
          } else {
            ChildrenToStop = ChildBufferCount;
            Status = DriverBinding->Stop (DriverBinding, ControllerHandle, ChildrenToStop, ChildBuffer);
          }
        }
        if (!EFI_ERROR (Status) && ((ChildHandle == NULL) || (ChildBufferCount == ChildrenToStop))) {
          Status = DriverBinding->Stop (DriverBinding, ControllerHandle, 0, NULL);
        }
        if (!EFI_ERROR (Status)) {
          StopCount++;
        }
      }

      if (ChildBuffer != NULL) {
        CoreFreePool (ChildBuffer);
      }
    }
  }

  if (StopCount > 0) {
    Status = EFI_SUCCESS;
  } else {
    Status = EFI_NOT_FOUND;
  }

Done:

  if (DriverImageHandleBuffer != NULL) {
    CoreFreePool (DriverImageHandleBuffer);
  }

  return Status;
}
