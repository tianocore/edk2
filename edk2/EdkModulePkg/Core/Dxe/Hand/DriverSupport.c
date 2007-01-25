/*++

Copyright (c) 2006, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

    DriverSupport.c
    
Abstract:

    EFI Driver Support Protocol

Revision History

--*/

#include <DxeMain.h>



STATIC
EFI_STATUS
GetHandleFromDriverBinding (
  IN EFI_DRIVER_BINDING_PROTOCOL           *DriverBindingNeed,
  OUT  EFI_HANDLE                          *Handle 
  );


//
// Driver Support Function Prototypes
//
STATIC
EFI_STATUS 
CoreConnectSingleController (
  IN  EFI_HANDLE                ControllerHandle,
  IN  EFI_HANDLE                *DriverImageHandle    OPTIONAL,
  IN  EFI_DEVICE_PATH_PROTOCOL  *RemainingDevicePath  OPTIONAL
  );

//
// Driver Support Functions
//


EFI_STATUS 
EFIAPI
CoreConnectController (
  IN  EFI_HANDLE                ControllerHandle,
  IN  EFI_HANDLE                *DriverImageHandle    OPTIONAL,
  IN  EFI_DEVICE_PATH_PROTOCOL  *RemainingDevicePath  OPTIONAL,
  IN  BOOLEAN                   Recursive
  )
/*++

Routine Description:

  Connects one or more drivers to a controller.

Arguments:

  ControllerHandle            - Handle of the controller to be connected.

  DriverImageHandle           - DriverImageHandle A pointer to an ordered list of driver image handles.

  RemainingDevicePath         - RemainingDevicePath A pointer to the device path that specifies a child of the
                                controller specified by ControllerHandle.
    
  Recursive                   - Whether the function would be called recursively or not.

Returns:

  Status code.

--*/
{
  EFI_STATUS                           Status;
  EFI_STATUS                           ReturnStatus;
  IHANDLE                              *Handle;
  PROTOCOL_INTERFACE                   *Prot;
  LIST_ENTRY                           *Link;
  LIST_ENTRY                           *ProtLink;
  OPEN_PROTOCOL_DATA                   *OpenData;
  EFI_DEVICE_PATH_PROTOCOL             *AlignedRemainingDevicePath;
  
  //
  // Make sure ControllerHandle is valid
  //
  Status = CoreValidateHandle (ControllerHandle);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Handle = ControllerHandle;

  //
  // Connect all drivers to ControllerHandle 
  //
  AlignedRemainingDevicePath = NULL;
  if (RemainingDevicePath != NULL) {
    AlignedRemainingDevicePath = CoreDuplicateDevicePath (RemainingDevicePath);
  }
  ReturnStatus = CoreConnectSingleController (
                   ControllerHandle,
                   DriverImageHandle,
                   AlignedRemainingDevicePath
                   );
  if (AlignedRemainingDevicePath != NULL) {
    CoreFreePool (AlignedRemainingDevicePath);
  }

  //
  // If not recursive, then just return after connecting drivers to ControllerHandle
  //
  if (!Recursive) {
    return ReturnStatus;
  }

  //
  // If recursive, then connect all drivers to all of ControllerHandle's children
  //
  CoreAcquireProtocolLock ();
  for (Link = Handle->Protocols.ForwardLink; Link != &Handle->Protocols; Link = Link->ForwardLink) {
    Prot = CR(Link, PROTOCOL_INTERFACE, Link, PROTOCOL_INTERFACE_SIGNATURE);
    for (ProtLink = Prot->OpenList.ForwardLink; 
           ProtLink != &Prot->OpenList; 
           ProtLink = ProtLink->ForwardLink) {
        OpenData = CR (ProtLink, OPEN_PROTOCOL_DATA, Link, OPEN_PROTOCOL_DATA_SIGNATURE);
        if ((OpenData->Attributes & EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER) != 0) {
          CoreReleaseProtocolLock ();
          Status = CoreConnectController (
                          OpenData->ControllerHandle,
                          NULL,
                          NULL,
                          TRUE
                          ); 
          CoreAcquireProtocolLock ();
        }
    }
  }
  CoreReleaseProtocolLock ();
  
  return ReturnStatus;
}

STATIC
VOID
AddSortedDriverBindingProtocol (
  IN      EFI_HANDLE                   DriverBindingHandle,
  IN OUT  UINTN                        *NumberOfSortedDriverBindingProtocols, 
  IN OUT  EFI_DRIVER_BINDING_PROTOCOL  **SortedDriverBindingProtocols,
  IN      UINTN                        DriverBindingHandleCount,
  IN OUT  EFI_HANDLE                   *DriverBindingHandleBuffer
  )
/*++

Routine Description:

  Add Driver Binding Protocols from Context Driver Image Handles to sorted 
   Driver Binding Protocol list.

Arguments:

  DriverBindingHandle - Handle of the driver binding protocol.

  NumberOfSortedDriverBindingProtocols - Number Of sorted driver binding protocols

  SortedDriverBindingProtocols - The sorted protocol list.                        
    
  DriverBindingHandleCount - Driver Binding Handle Count.

  DriverBindingHandleBuffer - The buffer of driver binding protocol to be modified.

Returns:

  None.

--*/
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
  // Retrieve the Driver Binding Protocol from DriverBindingHandle
  //
  Status = CoreHandleProtocol(
             DriverBindingHandle,
             &gEfiDriverBindingProtocolGuid,
             (VOID **)&DriverBinding
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
  for (Index = 0; Index < *NumberOfSortedDriverBindingProtocols; Index++) {
    if (DriverBinding == SortedDriverBindingProtocols[Index]) {
      return;
    }
  }

  //
  // Add DriverBinding to the end of the list
  //
  SortedDriverBindingProtocols[*NumberOfSortedDriverBindingProtocols] = DriverBinding;
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
 
STATIC
EFI_STATUS 
CoreConnectSingleController (
  IN  EFI_HANDLE                ControllerHandle,
  IN  EFI_HANDLE                *ContextDriverImageHandles OPTIONAL,
  IN  EFI_DEVICE_PATH_PROTOCOL  *RemainingDevicePath       OPTIONAL     
  )
/*++

Routine Description:

  Connects a controller to a driver.

Arguments:

  ControllerHandle            - Handle of the controller to be connected.
  ContextDriverImageHandles   - DriverImageHandle A pointer to an ordered list of driver image handles.
  RemainingDevicePath         - RemainingDevicePath A pointer to the device path that specifies a child 
                                of the controller specified by ControllerHandle.
    
Returns:

  EFI_SUCCESS           - One or more drivers were connected to ControllerHandle.
  EFI_OUT_OF_RESOURCES  - No enough system resources to complete the request.
  EFI_NOT_FOUND         - No drivers were connected to ControllerHandle.

--*/
{
  EFI_STATUS                                 Status;
  UINTN                                      Index;
  EFI_HANDLE                                 DriverImageHandle;
  EFI_PLATFORM_DRIVER_OVERRIDE_PROTOCOL      *PlatformDriverOverride;
  EFI_BUS_SPECIFIC_DRIVER_OVERRIDE_PROTOCOL  *BusSpecificDriverOverride;
  UINTN                                      DriverBindingHandleCount;
  EFI_HANDLE                                 *DriverBindingHandleBuffer;
  EFI_DRIVER_BINDING_PROTOCOL                *DriverBinding;
  UINTN                                      NumberOfSortedDriverBindingProtocols;
  EFI_DRIVER_BINDING_PROTOCOL                **SortedDriverBindingProtocols;
  UINT32                                     HighestVersion;
  UINTN                                      HighestIndex;
  UINTN                                      SortIndex;
  BOOLEAN                                    OneStarted;
  BOOLEAN                                    DriverFound;
  EFI_HANDLE                                 DriverBindingHandle;

  //
  // DriverBindingHandle is used for performance measurement, initialize it here just in case.
  //
  DriverBindingHandle                   = NULL;
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
  SortedDriverBindingProtocols = CoreAllocateBootServicesPool (sizeof (VOID *) * DriverBindingHandleCount);
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
        DriverBindingHandleBuffer
        );
    }
  }

  //
  // Add the Platform Driver Override Protocol drivers for ControllerHandle next
  //
  Status = CoreLocateProtocol (
             &gEfiPlatformDriverOverrideProtocolGuid, 
             NULL, 
             (VOID **)&PlatformDriverOverride
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
          DriverBindingHandleBuffer
          );
      }
    } while (!EFI_ERROR (Status));
  }

  //
  // Get the Bus Specific Driver Override Protocol instance on the Controller Handle
  //
  Status = CoreHandleProtocol(
             ControllerHandle,  
             &gEfiBusSpecificDriverOverrideProtocolGuid, 
             (VOID **)&BusSpecificDriverOverride
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
          DriverBindingHandleBuffer
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
      DriverBindingHandleBuffer
      );
  }

  //
  // Free the Driver Binding Handle Buffer
  //
  CoreFreePool (DriverBindingHandleBuffer);

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
        Status = DriverBinding->Supported(
                                  DriverBinding, 
                                  ControllerHandle,
                                  RemainingDevicePath
                                  );
        if (!EFI_ERROR (Status)) {
          SortedDriverBindingProtocols[Index] = NULL;
          DriverFound = TRUE;

          //
          // A driver was found that supports ControllerHandle, so attempt to start the driver
          // on ControllerHandle.
          //
          PERF_CODE_BEGIN ();
          GetHandleFromDriverBinding (DriverBinding, &DriverBindingHandle);
          PERF_CODE_END ();

          PERF_START (DriverBindingHandle, DRIVERBINDING_START_TOK, NULL, 0);
          Status = DriverBinding->Start (
                                    DriverBinding, 
                                    ControllerHandle,
                                    RemainingDevicePath
                                    );
          PERF_END (DriverBindingHandle, DRIVERBINDING_START_TOK, NULL, 0);

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


EFI_STATUS 
EFIAPI
CoreDisconnectController (
  IN  EFI_HANDLE  ControllerHandle,
  IN  EFI_HANDLE  DriverImageHandle  OPTIONAL,
  IN  EFI_HANDLE  ChildHandle        OPTIONAL
  )
/*++

Routine Description:

  Disonnects a controller from a driver

Arguments:

  ControllerHandle  - ControllerHandle The handle of the controller from which driver(s) 
                        are to be disconnected.
  DriverImageHandle - DriverImageHandle The driver to disconnect from ControllerHandle.
  ChildHandle       - ChildHandle The handle of the child to destroy.

Returns:

  EFI_SUCCESS           -  One or more drivers were disconnected from the controller.
  EFI_SUCCESS           -  On entry, no drivers are managing ControllerHandle.
  EFI_SUCCESS           -  DriverImageHandle is not NULL, and on entry DriverImageHandle is not managing ControllerHandle.
  EFI_INVALID_PARAMETER -  ControllerHandle is not a valid EFI_HANDLE.
  EFI_INVALID_PARAMETER -  DriverImageHandle is not NULL, and it is not a valid EFI_HANDLE.
  EFI_INVALID_PARAMETER -  ChildHandle is not NULL, and it is not a valid EFI_HANDLE.
  EFI_OUT_OF_RESOURCES  -  There are not enough resources available to disconnect any drivers from ControllerHandle.
  EFI_DEVICE_ERROR      -  The controller could not be disconnected because of a device error.

--*/
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

    DriverImageHandleBuffer = CoreAllocateBootServicesPool (sizeof (EFI_HANDLE) * DriverImageHandleCount);
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
    if (EFI_ERROR (Status)) {
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
        ChildBuffer = CoreAllocateBootServicesPool (sizeof (EFI_HANDLE) * ChildBufferCount);
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



STATIC
EFI_STATUS
GetHandleFromDriverBinding (
  IN   EFI_DRIVER_BINDING_PROTOCOL           *DriverBindingNeed,
  OUT  EFI_HANDLE                            *Handle 
 )
/*++

Routine Description:

  Locate the driver binding handle which a specified driver binding protocol installed on.

Arguments:

  DriverBindingNeed  - The specified driver binding protocol.
  
  Handle             - The driver binding handle which the protocol installed on.
  

Returns:

  EFI_NOT_FOUND         - Could not find the handle.
  
  EFI_SUCCESS           - Successfully find the associated driver binding handle.
  
--*/ 
 {
  EFI_STATUS                          Status ;
  EFI_DRIVER_BINDING_PROTOCOL         *DriverBinding;
  UINTN                               DriverBindingHandleCount;
  EFI_HANDLE                          *DriverBindingHandleBuffer;
  UINTN                               Index;
  
  DriverBindingHandleCount = 0;
  DriverBindingHandleBuffer = NULL;
  *Handle = NULL_HANDLE;
  Status = CoreLocateHandleBuffer (
              ByProtocol,   
              &gEfiDriverBindingProtocolGuid,  
              NULL,
              &DriverBindingHandleCount, 
              &DriverBindingHandleBuffer
              );
  if (EFI_ERROR (Status) || DriverBindingHandleCount == 0) {
    return EFI_NOT_FOUND;
  }
  
  for (Index = 0 ; Index < DriverBindingHandleCount; Index++ ) {
    Status = CoreOpenProtocol(
                      DriverBindingHandleBuffer[Index],
                      &gEfiDriverBindingProtocolGuid,
                      (VOID **)&DriverBinding,
                      gDxeCoreImageHandle,
                      NULL,
                      EFI_OPEN_PROTOCOL_GET_PROTOCOL
                      );
                      
   if (!EFI_ERROR (Status) && DriverBinding != NULL) {
    
    if ( DriverBinding == DriverBindingNeed ) {
      *Handle = DriverBindingHandleBuffer[Index];
      CoreFreePool (DriverBindingHandleBuffer);         
      return EFI_SUCCESS ;
    }
   }
 }
 
 CoreFreePool (DriverBindingHandleBuffer);
 return EFI_NOT_FOUND ;
}

