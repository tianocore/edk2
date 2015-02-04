/** @file
  Main file for connect shell Driver1 function.

  (C) Copyright 2015 Hewlett-Packard Development Company, L.P.<BR>
  Copyright (c) 2010 - 2014, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "UefiShellDriver1CommandsLib.h"

/**
  Create all handles associate with every device path node.

  @param  DevicePathToConnect           The device path which will be connected.

  @retval EFI_SUCCESS                   All handles associate with every device path node
                                        have been created.
  @retval EFI_INVALID_PARAMETER         DevicePathToConnect is NULL.
  @retval EFI_NOT_FOUND                 Create the handle associate with one device path
                                        node failed

**/
EFI_STATUS
ShellConnectDevicePath (
  IN EFI_DEVICE_PATH_PROTOCOL   *DevicePathToConnect
  )
{
  EFI_DEVICE_PATH_PROTOCOL  *RemainingDevicePath;
  EFI_STATUS                Status;
  EFI_HANDLE                Handle;
  EFI_HANDLE                PreviousHandle;
  
  if (DevicePathToConnect == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  PreviousHandle = NULL;
  do{    
    RemainingDevicePath = DevicePathToConnect;
    Status = gBS->LocateDevicePath (&gEfiDevicePathProtocolGuid, &RemainingDevicePath, &Handle);
    
    if (!EFI_ERROR (Status) && (Handle != NULL)) {
      if (PreviousHandle == Handle) {
        Status = EFI_NOT_FOUND;
      } else {
        PreviousHandle = Handle;
        Status = gBS->ConnectController (Handle, NULL, RemainingDevicePath, FALSE);
      }
    }
    
  } while (!EFI_ERROR (Status) && !IsDevicePathEnd (RemainingDevicePath) );
  
  return Status;
   
}

/**
  Connect drivers for PCI root bridge.
  
  @retval EFI_SUCCESS                     Connect drivers successfully.
  @retval EFI_NOT_FOUND                   Cannot find PCI root bridge device.

**/
EFI_STATUS
ShellConnectPciRootBridge (
  VOID
  )
{  
  UINTN               RootBridgeHandleCount;
  EFI_HANDLE          *RootBridgeHandleBuffer;
  UINTN               RootBridgeIndex;
  EFI_STATUS          Status;
  
  RootBridgeHandleCount = 0;
  
  Status = gBS->LocateHandleBuffer (  
              ByProtocol,  
              &gEfiPciRootBridgeIoProtocolGuid, 
              NULL,  
              &RootBridgeHandleCount,  
              &RootBridgeHandleBuffer  
              );
  if (EFI_ERROR (Status)) {
    return Status;
  }
  
  for (RootBridgeIndex = 0; RootBridgeIndex < RootBridgeHandleCount; RootBridgeIndex++) {    
    gBS->ConnectController (RootBridgeHandleBuffer[RootBridgeIndex], NULL, NULL, FALSE);    
  }  

  FreePool (RootBridgeHandleBuffer);
  
  return EFI_SUCCESS;
}


/**
  Connect controller(s) and driver(s).

  @param[in] ControllerHandle     The handle to the controller. Should have driver binding on it.
  @param[in] DriverHandle         The handle to the driver. Should have driver binding.
  @param[in] Recursive            TRUE to connect recursively, FALSE otherwise.
  @param[in] Output               TRUE to have info on the screen, FALSE otherwise.
  @param[in] AlwaysOutput         Override Output for errors.

  @retval EFI_SUCCESS             The operation was successful.
**/
EFI_STATUS
EFIAPI
ConnectControllers (
  IN CONST EFI_HANDLE ControllerHandle OPTIONAL,
  IN CONST EFI_HANDLE DriverHandle OPTIONAL,
  IN CONST BOOLEAN    Recursive,
  IN CONST BOOLEAN    Output,
  IN CONST BOOLEAN    AlwaysOutput
  )
{
  EFI_STATUS Status;
  EFI_STATUS Status2;
  EFI_HANDLE *ControllerHandleList;
  EFI_HANDLE *DriverHandleList;
  EFI_HANDLE *HandleWalker;

  ControllerHandleList  = NULL;
  Status                = EFI_NOT_FOUND;
  Status2               = EFI_NOT_FOUND;

  //
  // If we have a single handle to connect make that a 'list'
  //
  if (DriverHandle == NULL) {
    DriverHandleList = NULL;
  } else {
    DriverHandleList = AllocateZeroPool(2*sizeof(EFI_HANDLE));
    if (DriverHandleList == NULL) {
      return (EFI_OUT_OF_RESOURCES);
    }
    DriverHandleList[0] = DriverHandle;
    DriverHandleList[1] = NULL;
  }

  //
  // do we connect all controllers (with a loop) or a single one...
  // This is where we call the gBS->ConnectController function.
  //
  if (ControllerHandle == NULL) {
    ControllerHandleList = GetHandleListByProtocol(&gEfiDevicePathProtocolGuid);
    for (HandleWalker = ControllerHandleList
      ;  HandleWalker != NULL && *HandleWalker != NULL
      ;  HandleWalker++
     ){
      Status = gBS->ConnectController(*HandleWalker, DriverHandleList, NULL, Recursive);
      if (!EFI_ERROR(Status)) {
        Status2 = EFI_SUCCESS;
      }
      if ((Output && !EFI_ERROR(Status)) || AlwaysOutput) {
        ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN(STR_HANDLE_RESULT), gShellDriver1HiiHandle, L"Connect", ConvertHandleToHandleIndex(*HandleWalker), Status);
      }
    }
  } else {
    Status = gBS->ConnectController(ControllerHandle, DriverHandleList, NULL, Recursive);
    if (!EFI_ERROR(Status)) {
      Status2 = EFI_SUCCESS;
    }
    if ((Output && !EFI_ERROR(Status)) || AlwaysOutput) {
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN(STR_HANDLE_RESULT), gShellDriver1HiiHandle, L"Connect", ConvertHandleToHandleIndex(ControllerHandle), Status);
    }
  }

  //
  // Free any memory we allocated.
  //
  if (ControllerHandleList != NULL) {
    FreePool(ControllerHandleList);
  }
  if (DriverHandleList     != NULL) {
    FreePool(DriverHandleList);
  }
  return (Status2);
}

/**
  Do a connect from an EFI variable via it's key name.

  @param[in] Key      The name of the EFI Variable.

  @retval EFI_SUCCESS   The operation was successful.
**/
EFI_STATUS
EFIAPI
ShellConnectFromDevPaths (
  IN CONST CHAR16 *Key
  )
{
  EFI_DEVICE_PATH_PROTOCOL  *DevPath;
  EFI_DEVICE_PATH_PROTOCOL  *CopyOfDevPath;
  EFI_DEVICE_PATH_PROTOCOL  *Instance;  
  EFI_DEVICE_PATH_PROTOCOL  *Next;
  UINTN                     Length;
  UINTN                     Index;
  UINTN                     HandleArrayCount;
  UINTN                     Size;
  EFI_HANDLE                *HandleArray;
  EFI_STATUS                Status;
  BOOLEAN                   AtLeastOneConnected;
  EFI_PCI_IO_PROTOCOL       *PciIo;
  UINT8                     Class[3];
  
  DevPath = NULL;
  Length  = 0;
  AtLeastOneConnected = FALSE;

  //
  // Get the DevicePath buffer from the variable...
  //
  Status = gRT->GetVariable((CHAR16*)Key, (EFI_GUID*)&gEfiGlobalVariableGuid, NULL, &Length, DevPath);
  if (Status == EFI_BUFFER_TOO_SMALL) {
    DevPath = AllocateZeroPool(Length);
    if (DevPath == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }
    Status = gRT->GetVariable((CHAR16*)Key, (EFI_GUID*)&gEfiGlobalVariableGuid, NULL, &Length, DevPath);
    if (EFI_ERROR (Status)) {
      if (DevPath != NULL) {
        FreePool (DevPath);
      }
      return Status;
    }
  } else if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = EFI_NOT_FOUND;

  CopyOfDevPath = DevPath;
  //
  // walk the list of devices and connect them
  //
  do {
    //
    // Check every instance of the console variable
    //
    Instance = GetNextDevicePathInstance (&CopyOfDevPath, &Size);
    if (Instance == NULL) {
      if (DevPath != NULL) {
        FreePool (DevPath);
      }
      return EFI_UNSUPPORTED;
    }

    Next = Instance;
    while (!IsDevicePathEndType (Next)) {
      Next = NextDevicePathNode (Next);
    }

    SetDevicePathEndNode (Next);
    //
    // connect short form device path
    //
    if ((DevicePathType (Instance) == MESSAGING_DEVICE_PATH) &&
      ((DevicePathSubType (Instance) == MSG_USB_CLASS_DP)
      || (DevicePathSubType (Instance) == MSG_USB_WWID_DP)
      )) {
      
      Status = ShellConnectPciRootBridge ();
      if (EFI_ERROR(Status)) {
        FreePool(Instance);
        FreePool(DevPath);
        return Status;
      }
      
      Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEfiPciIoProtocolGuid,
                  NULL,
                  &HandleArrayCount,
                  &HandleArray
                  );
      
      if (!EFI_ERROR (Status)) {
        for (Index = 0; Index < HandleArrayCount; Index++) {
          Status = gBS->HandleProtocol (
                      HandleArray[Index],
                      &gEfiPciIoProtocolGuid,
                      (VOID **)&PciIo
                      );
          
          if (!EFI_ERROR (Status)) {
            Status = PciIo->Pci.Read (PciIo, EfiPciIoWidthUint8, 0x09, 3, &Class);
            if (!EFI_ERROR (Status)) {
              if ((PCI_CLASS_SERIAL == Class[2]) &&
                  (PCI_CLASS_SERIAL_USB == Class[1])) {
                Status = gBS->ConnectController (
                              HandleArray[Index],
                              NULL,
                              Instance,
                              FALSE
                              );
                if (!EFI_ERROR(Status)) {
                  AtLeastOneConnected = TRUE;
                }
              }
            }
          }
        }
      }

      if (HandleArray != NULL) {
        FreePool (HandleArray);
      }
    } else { 
      //
      // connect the entire device path
      //
      Status = ShellConnectDevicePath (Instance);
      if (!EFI_ERROR (Status)) {
        AtLeastOneConnected = TRUE;
      }
    }
    FreePool (Instance);
    
  } while (CopyOfDevPath != NULL);
  
  if (DevPath != NULL) {
    FreePool(DevPath);
  }

  if (AtLeastOneConnected) {
    return EFI_SUCCESS;
  } else {
    return EFI_NOT_FOUND;
  }
  
}

/**
  Convert the handle identifiers from strings and then connect them.

  One of them should have driver binding and either can be NULL.

  @param[in] Handle1            The first handle.
  @param[in] Handle2            The second handle.
  @param[in] Recursive          TRUE to do connect recursively. FALSE otherwise.
  @param[in] Output             TRUE to have output to screen. FALSE otherwise.

  @retval EFI_SUCCESS           The operation was successful.
**/
EFI_STATUS
EFIAPI
ConvertAndConnectControllers (
  IN EFI_HANDLE     *Handle1 OPTIONAL,
  IN EFI_HANDLE     *Handle2 OPTIONAL,
  IN CONST BOOLEAN  Recursive,
  IN CONST BOOLEAN  Output
  )
{
  //
  // if only one is NULL verify it's the proper one...
  //
  if ( (Handle1 == NULL && Handle2 != NULL)
    || (Handle1 != NULL && Handle2 == NULL)
   ){
    //
    // Figure out which one should be NULL and move the handle to the right place.
    // If Handle1 is NULL then test Handle2 and vise versa.
    // The one that DOES has driver binding must be Handle2
    //
    if (Handle1 == NULL) {
      if (EFI_ERROR(gBS->OpenProtocol(Handle2, &gEfiDriverBindingProtocolGuid, NULL, NULL, gImageHandle, EFI_OPEN_PROTOCOL_TEST_PROTOCOL))) {
        // swap
        Handle1 = Handle2;
        Handle2 = NULL;
      } else {
        // We're all good...
      }
    } else {
      if (EFI_ERROR(gBS->OpenProtocol(Handle1, &gEfiDriverBindingProtocolGuid, NULL, NULL, gImageHandle, EFI_OPEN_PROTOCOL_TEST_PROTOCOL))) {
        // We're all good...
      } else {
        // swap
        Handle2 = Handle1;
        Handle1 = NULL;
      }
    }
  }

  return (ConnectControllers(Handle1, Handle2, Recursive, Output, (BOOLEAN)(Handle2 != NULL && Handle1 != NULL)));
}

STATIC CONST SHELL_PARAM_ITEM ParamList[] = {
  {L"-c", TypeFlag},
  {L"-r", TypeFlag},
  {NULL, TypeMax}
  };

/**
  Function for 'connect' command.

  @param[in] ImageHandle  Handle to the Image (NULL if Internal).
  @param[in] SystemTable  Pointer to the System Table (NULL if Internal).
**/
SHELL_STATUS
EFIAPI
ShellCommandRunConnect (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS          Status;
  LIST_ENTRY          *Package;
  CHAR16              *ProblemParam;
  SHELL_STATUS        ShellStatus;
  CONST CHAR16        *Param1;
  CONST CHAR16        *Param2;
  UINTN               Count;
  EFI_HANDLE          Handle1;
  EFI_HANDLE          Handle2;
  UINT64              Intermediate;

  ShellStatus         = SHELL_SUCCESS;
  //
  // initialize the shell lib (we must be in non-auto-init...)
  //
  Status = ShellInitialize();
  ASSERT_EFI_ERROR(Status);

  Status = CommandInit();
  ASSERT_EFI_ERROR(Status);

  //
  // parse the command line
  //
  Status = ShellCommandLineParse (ParamList, &Package, &ProblemParam, TRUE);
  if (EFI_ERROR(Status)) {
    if (Status == EFI_VOLUME_CORRUPTED && ProblemParam != NULL) {
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_PROBLEM), gShellDriver1HiiHandle, L"connect", ProblemParam);  
      FreePool(ProblemParam);
      ShellStatus = SHELL_INVALID_PARAMETER;
    } else {
      ASSERT(FALSE);
    }
  } else {
    //
    // if more than 2 'value' parameters (plus the name one) or either -r or -c with any value parameters we have too many parameters
    //
    Count = (gInReconnect?0x4:0x3);
    if ((ShellCommandLineGetCount(Package) > Count)
      ||((ShellCommandLineGetFlag(Package, L"-r") || ShellCommandLineGetFlag(Package, L"-c")) && ShellCommandLineGetCount(Package)>1)
      ||(ShellCommandLineGetFlag(Package, L"-r") && ShellCommandLineGetFlag(Package, L"-c") )
     ){
      //
      // error for too many parameters
      //
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_TOO_MANY), gShellDriver1HiiHandle, L"connect");  
      ShellStatus = SHELL_INVALID_PARAMETER;
    } else if (ShellCommandLineGetFlag(Package, L"-c")) {
      //
      // do the conin and conout from EFI variables
      // if the first fails dont 'loose' the error
      //
      Status = ShellConnectFromDevPaths(L"ConInDev");
      if (EFI_ERROR(Status)) {
        ShellConnectFromDevPaths(L"ConOutDev");
      } else {
        Status = ShellConnectFromDevPaths(L"ConOutDev");
      }
      if (EFI_ERROR(Status)) {
        ShellConnectFromDevPaths(L"ErrOutDev");
      } else {
        Status = ShellConnectFromDevPaths(L"ErrOutDev");
      }
      if (EFI_ERROR(Status)) {
        ShellConnectFromDevPaths(L"ErrOut");
      } else {
        Status = ShellConnectFromDevPaths(L"ErrOut");
      }
      if (EFI_ERROR(Status)) {
        ShellConnectFromDevPaths(L"ConIn");
      } else {
        Status = ShellConnectFromDevPaths(L"ConIn");
      }
      if (EFI_ERROR(Status)) {
        ShellConnectFromDevPaths(L"ConOut");
      } else {
        Status = ShellConnectFromDevPaths(L"ConOut");
      }
      if (EFI_ERROR(Status)) {
        ShellStatus = SHELL_DEVICE_ERROR;
      }
    } else {
      //
      // 0, 1, or 2 specific handles and possibly recursive
      //
      Param1  = ShellCommandLineGetRawValue(Package, 1);
      Param2  = ShellCommandLineGetRawValue(Package, 2);
      Count   = ShellCommandLineGetCount(Package);

      if (Param1 != NULL) {
        Status  = ShellConvertStringToUint64(Param1, &Intermediate, TRUE, FALSE);
        Handle1 = ConvertHandleIndexToHandle((UINTN)Intermediate);
        if (EFI_ERROR(Status)) {
          ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_INV_HANDLE), gShellDriver1HiiHandle, L"connect", Param1);  
          ShellStatus = SHELL_INVALID_PARAMETER;
        }
      } else {
        Handle1 = NULL;
      }

      if (Param2 != NULL) {
        Status  = ShellConvertStringToUint64(Param2, &Intermediate, TRUE, FALSE);
        Handle2 = ConvertHandleIndexToHandle((UINTN)Intermediate);
        if (EFI_ERROR(Status)) {
          ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_INV_HANDLE), gShellDriver1HiiHandle, L"connect", Param2);  
          ShellStatus = SHELL_INVALID_PARAMETER;
        }
      } else {
        Handle2 = NULL;
      }
      
      if (ShellStatus == SHELL_SUCCESS) {
        if (Param1 != NULL && Handle1 == NULL){
          ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_INV_HANDLE), gShellDriver1HiiHandle, L"connect", Param1);  
          ShellStatus = SHELL_INVALID_PARAMETER;
        } else if (Param2 != NULL && Handle2 == NULL) {
          ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_INV_HANDLE), gShellDriver1HiiHandle, L"connect", Param2);  
          ShellStatus = SHELL_INVALID_PARAMETER;
        } else if (Handle2 != NULL && Handle1 != NULL && EFI_ERROR(gBS->OpenProtocol(Handle2, &gEfiDriverBindingProtocolGuid, NULL, gImageHandle, NULL, EFI_OPEN_PROTOCOL_TEST_PROTOCOL))) {
          ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_INV_HANDLE), gShellDriver1HiiHandle, L"connect", Param2);  
          ShellStatus = SHELL_INVALID_PARAMETER;
        } else {
          Status = ConvertAndConnectControllers(Handle1, Handle2, ShellCommandLineGetFlag(Package, L"-r"), (BOOLEAN)(Count!=0));
          if (EFI_ERROR(Status)) {
            ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_CONNECT_NONE), gShellDriver1HiiHandle);
            ShellStatus = SHELL_DEVICE_ERROR;
          }
        }
      }
    }

    ShellCommandLineFreeVarList (Package);
  }
  return (ShellStatus);
}

