/** @file
  Library functions which contain all the code to connect console device.

Copyright (c) 2011 - 2018, Intel Corporation. All rights reserved.<BR>
(C) Copyright 2015 Hewlett Packard Enterprise Development LP<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "InternalBm.h"

CHAR16       *mConVarName[] = {
  L"ConIn",
  L"ConOut",
  L"ErrOut",
  L"ConInDev",
  L"ConOutDev",
  L"ErrOutDev"
};

/**
  Search out the video controller.

  @return  PCI device path of the video controller.
**/
EFI_HANDLE
BmGetVideoController (
  VOID
  )
{
  EFI_STATUS                Status;
  UINTN                     RootBridgeHandleCount;
  EFI_HANDLE                *RootBridgeHandleBuffer;
  UINTN                     HandleCount;
  EFI_HANDLE                *HandleBuffer;
  UINTN                     RootBridgeIndex;
  UINTN                     Index;
  EFI_HANDLE                VideoController;
  EFI_PCI_IO_PROTOCOL       *PciIo;
  PCI_TYPE00                Pci;

  //
  // Make all the PCI_IO protocols show up
  //
  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEfiPciRootBridgeIoProtocolGuid,
                  NULL,
                  &RootBridgeHandleCount,
                  &RootBridgeHandleBuffer
                  );
  if (EFI_ERROR (Status) || (RootBridgeHandleCount == 0)) {
    return NULL;
  }

  VideoController = NULL;
  for (RootBridgeIndex = 0; RootBridgeIndex < RootBridgeHandleCount; RootBridgeIndex++) {
    gBS->ConnectController (RootBridgeHandleBuffer[RootBridgeIndex], NULL, NULL, FALSE);

    //
    // Start to check all the pci io to find the first video controller
    //
    Status = gBS->LocateHandleBuffer (
                    ByProtocol,
                    &gEfiPciIoProtocolGuid,
                    NULL,
                    &HandleCount,
                    &HandleBuffer
                    );
    if (EFI_ERROR (Status)) {
      continue;
    }

    for (Index = 0; Index < HandleCount; Index++) {
      Status = gBS->HandleProtocol (HandleBuffer[Index], &gEfiPciIoProtocolGuid, (VOID **) &PciIo);
      if (!EFI_ERROR (Status)) {
        //
        // Check for all video controller
        //
        Status = PciIo->Pci.Read (
                          PciIo,
                          EfiPciIoWidthUint32,
                          0,
                          sizeof (Pci) / sizeof (UINT32),
                          &Pci
                          );
        if (!EFI_ERROR (Status) && IS_PCI_VGA (&Pci)) {
          // TODO: use IS_PCI_DISPLAY??
          VideoController = HandleBuffer[Index];
          break;
        }
      }
    }
    FreePool (HandleBuffer);

    if (VideoController != NULL) {
      break;
    }
  }
  FreePool (RootBridgeHandleBuffer);

  return VideoController;
}

/**
  Query all the children of VideoController and return the device paths of all the
  children that support GraphicsOutput protocol.

  @param VideoController       PCI handle of video controller.

  @return  Device paths of all the children that support GraphicsOutput protocol.
**/
EFI_DEVICE_PATH_PROTOCOL *
EFIAPI
EfiBootManagerGetGopDevicePath (
  IN  EFI_HANDLE                       VideoController
  )
{
  UINTN                                Index;
  EFI_STATUS                           Status;
  EFI_GUID                             **ProtocolBuffer;
  UINTN                                ProtocolBufferCount;
  UINTN                                ProtocolIndex;
  EFI_OPEN_PROTOCOL_INFORMATION_ENTRY  *OpenInfoBuffer;
  UINTN                                EntryCount;
  EFI_DEVICE_PATH_PROTOCOL             *DevicePath;
  EFI_DEVICE_PATH_PROTOCOL             *Next;
  EFI_DEVICE_PATH_PROTOCOL             *Previous;
  EFI_DEVICE_PATH_PROTOCOL             *TempDevicePath;
  EFI_DEVICE_PATH_PROTOCOL             *GopPool;
  EFI_DEVICE_PATH_PROTOCOL             *ReturnDevicePath;


  Status = gBS->ProtocolsPerHandle (
                  VideoController,
                  &ProtocolBuffer,
                  &ProtocolBufferCount
                  );
  if (EFI_ERROR (Status)) {
    return NULL;
  }

  GopPool = NULL;

  for (ProtocolIndex = 0; ProtocolIndex < ProtocolBufferCount; ProtocolIndex++) {
    Status = gBS->OpenProtocolInformation (
                    VideoController,
                    ProtocolBuffer[ProtocolIndex],
                    &OpenInfoBuffer,
                    &EntryCount
                    );
    if (EFI_ERROR (Status)) {
      continue;
    }

    for (Index = 0; Index < EntryCount; Index++) {
      //
      // Query all the children
      //
      if ((OpenInfoBuffer[Index].Attributes & EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER) != 0) {
        Status = gBS->OpenProtocol (
                        OpenInfoBuffer[Index].ControllerHandle,
                        &gEfiDevicePathProtocolGuid,
                        (VOID **) &DevicePath,
                        NULL,
                        NULL,
                        EFI_OPEN_PROTOCOL_GET_PROTOCOL
                        );
        if (EFI_ERROR (Status)) {
          continue;
        }

        Previous = NULL;
        for (Next = DevicePath; !IsDevicePathEnd (Next); Next = NextDevicePathNode (Next)) {
          Previous = Next;
        }
        ASSERT (Previous != NULL);

        if (DevicePathType (Previous) == ACPI_DEVICE_PATH && DevicePathSubType (Previous) == ACPI_ADR_DP) {
          Status = gBS->OpenProtocol (
                          OpenInfoBuffer[Index].ControllerHandle,
                          &gEfiGraphicsOutputProtocolGuid,
                          NULL,
                          NULL,
                          NULL,
                          EFI_OPEN_PROTOCOL_TEST_PROTOCOL
                          );
          if (!EFI_ERROR (Status)) {
            //
            // Append the device path to GOP pool when there is GOP protocol installed.
            //
            TempDevicePath = GopPool;
            GopPool = AppendDevicePathInstance (GopPool, DevicePath);
            gBS->FreePool (TempDevicePath);
          }
        }

        if (DevicePathType (Previous) == HARDWARE_DEVICE_PATH && DevicePathSubType (Previous) == HW_CONTROLLER_DP) {
          //
          // Recursively look for GOP child in this frame buffer handle
          //
          DEBUG ((EFI_D_INFO, "[Bds] Looking for GOP child deeper ... \n"));
          TempDevicePath = GopPool;
          ReturnDevicePath = EfiBootManagerGetGopDevicePath (OpenInfoBuffer[Index].ControllerHandle);
          GopPool = AppendDevicePathInstance (GopPool, ReturnDevicePath);
          gBS->FreePool (ReturnDevicePath);
          gBS->FreePool (TempDevicePath);
        }
      }
    }

    FreePool (OpenInfoBuffer);
  }

  FreePool (ProtocolBuffer);

  return GopPool;
}

/**
  Connect the platform active active video controller.

  @param VideoController       PCI handle of video controller.

  @retval EFI_NOT_FOUND There is no active video controller.
  @retval EFI_SUCCESS   The video controller is connected.
**/
EFI_STATUS
EFIAPI
EfiBootManagerConnectVideoController (
  EFI_HANDLE                 VideoController  OPTIONAL
  )
{
  EFI_DEVICE_PATH_PROTOCOL   *Gop;

  if (VideoController == NULL) {
    //
    // Get the platform vga device
    //
    VideoController = BmGetVideoController ();
  }

  if (VideoController == NULL) {
    return EFI_NOT_FOUND;
  }

  //
  // Try to connect the PCI device path, so that GOP driver could start on this
  // device and create child handles with GraphicsOutput Protocol installed
  // on them, then we get device paths of these child handles and select
  // them as possible console device.
  //
  gBS->ConnectController (VideoController, NULL, NULL, FALSE);

  Gop = EfiBootManagerGetGopDevicePath (VideoController);
  if (Gop == NULL) {
    return EFI_NOT_FOUND;
  }

  EfiBootManagerUpdateConsoleVariable (ConOut, Gop, NULL);
  FreePool (Gop);

  //
  // Necessary for ConPlatform and ConSplitter driver to start up again after ConOut is updated.
  //
  return gBS->ConnectController (VideoController, NULL, NULL, TRUE);
}

/**
  Fill console handle in System Table if there are no valid console handle in.

  Firstly, check the validation of console handle in System Table. If it is invalid,
  update it by the first console device handle from EFI console variable.

  @param  VarName            The name of the EFI console variable.
  @param  ConsoleGuid        Specified Console protocol GUID.
  @param  ConsoleHandle      On IN,  console handle in System Table to be checked.
                             On OUT, new console handle in system table.
  @param  ProtocolInterface  On IN,  console protocol on console handle in System Table to be checked.
                             On OUT, new console protocol on new console handle in system table.

  @retval TRUE               System Table has been updated.
  @retval FALSE              System Table hasn't been updated.

**/
BOOLEAN
BmUpdateSystemTableConsole (
  IN     CHAR16                   *VarName,
  IN     EFI_GUID                 *ConsoleGuid,
  IN OUT EFI_HANDLE               *ConsoleHandle,
  IN OUT VOID                     **ProtocolInterface
  )
{
  EFI_STATUS                      Status;
  UINTN                           DevicePathSize;
  EFI_DEVICE_PATH_PROTOCOL        *FullDevicePath;
  EFI_DEVICE_PATH_PROTOCOL        *VarConsole;
  EFI_DEVICE_PATH_PROTOCOL        *Instance;
  EFI_DEVICE_PATH_PROTOCOL        *FullInstance;
  VOID                            *Interface;
  EFI_HANDLE                      NewHandle;
  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *TextOut;

  ASSERT (VarName != NULL);
  ASSERT (ConsoleHandle != NULL);
  ASSERT (ConsoleGuid != NULL);
  ASSERT (ProtocolInterface != NULL);

  if (*ConsoleHandle != NULL) {
    Status = gBS->HandleProtocol (
                   *ConsoleHandle,
                   ConsoleGuid,
                   &Interface
                   );
    if (Status == EFI_SUCCESS && Interface == *ProtocolInterface) {
      //
      // If ConsoleHandle is valid and console protocol on this handle also
      // also matched, just return.
      //
      return FALSE;
    }
  }

  //
  // Get all possible consoles device path from EFI variable
  //
  GetEfiGlobalVariable2 (VarName, (VOID **) &VarConsole, NULL);
  if (VarConsole == NULL) {
    //
    // If there is no any console device, just return.
    //
    return FALSE;
  }

  FullDevicePath = VarConsole;

  do {
    //
    // Check every instance of the console variable
    //
    Instance  = GetNextDevicePathInstance (&VarConsole, &DevicePathSize);
    if (Instance == NULL) {
      DEBUG ((EFI_D_ERROR, "[Bds] No valid console instance is found for %s!\n", VarName));
      // We should not ASSERT when all the console devices are removed.
      // ASSERT_EFI_ERROR (EFI_NOT_FOUND);
      FreePool (FullDevicePath);
      return FALSE;
    }

    //
    // Find console device handle by device path instance
    //
    FullInstance = Instance;
    Status = gBS->LocateDevicePath (
                    ConsoleGuid,
                    &Instance,
                    &NewHandle
                    );
    FreePool (FullInstance);
    if (!EFI_ERROR (Status)) {
      //
      // Get the console protocol on this console device handle
      //
      Status = gBS->HandleProtocol (
                      NewHandle,
                      ConsoleGuid,
                      &Interface
                      );
      if (!EFI_ERROR (Status)) {
        //
        // Update new console handle in System Table.
        //
        *ConsoleHandle     = NewHandle;
        *ProtocolInterface = Interface;
        if (CompareGuid (ConsoleGuid, &gEfiSimpleTextOutProtocolGuid)) {
          //
          // If it is console out device, set console mode 80x25 if current mode is invalid.
          //
          TextOut = (EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *) Interface;
          if (TextOut->Mode->Mode == -1) {
            TextOut->SetMode (TextOut, 0);
          }
        }
        FreePool (FullDevicePath);
        return TRUE;
      }
    }

  } while (Instance != NULL);

  //
  // No any available console devcie found.
  //
  FreePool (FullDevicePath);
  return FALSE;
}

/**
  This function updates the console variable based on ConVarName. It can
  add or remove one specific console device path from the variable

  @param  ConsoleType              ConIn, ConOut, ErrOut, ConInDev, ConOutDev or ErrOutDev.
  @param  CustomizedConDevicePath  The console device path to be added to
                                   the console variable. Cannot be multi-instance.
  @param  ExclusiveDevicePath      The console device path to be removed
                                   from the console variable. Cannot be multi-instance.

  @retval EFI_UNSUPPORTED          The added device path is the same as a removed one.
  @retval EFI_SUCCESS              Successfully added or removed the device path from the
                                   console variable.
  @retval others                   Return status of RT->SetVariable().

**/
EFI_STATUS
EFIAPI
EfiBootManagerUpdateConsoleVariable (
  IN  CONSOLE_TYPE              ConsoleType,
  IN  EFI_DEVICE_PATH_PROTOCOL  *CustomizedConDevicePath,
  IN  EFI_DEVICE_PATH_PROTOCOL  *ExclusiveDevicePath
  )
{
  EFI_STATUS                Status;
  EFI_DEVICE_PATH_PROTOCOL  *VarConsole;
  EFI_DEVICE_PATH_PROTOCOL  *NewDevicePath;
  EFI_DEVICE_PATH_PROTOCOL  *TempNewDevicePath;

  if (ConsoleType >= ARRAY_SIZE (mConVarName)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Notes: check the device path point, here should check
  // with compare memory
  //
  if (CustomizedConDevicePath == ExclusiveDevicePath) {
    return EFI_UNSUPPORTED;
  }
  //
  // Delete the ExclusiveDevicePath from current default console
  //
  GetEfiGlobalVariable2 (mConVarName[ConsoleType], (VOID **) &VarConsole, NULL);
  //
  // Initialize NewDevicePath
  //
  NewDevicePath = VarConsole;

  //
  // If ExclusiveDevicePath is even the part of the instance in VarConsole, delete it.
  // In the end, NewDevicePath is the final device path.
  //
  if (ExclusiveDevicePath != NULL && VarConsole != NULL) {
      NewDevicePath = BmDelPartMatchInstance (VarConsole, ExclusiveDevicePath);
  }
  //
  // Try to append customized device path to NewDevicePath.
  //
  if (CustomizedConDevicePath != NULL) {
    if (!BmMatchDevicePaths (NewDevicePath, CustomizedConDevicePath)) {
      //
      // Check if there is part of CustomizedConDevicePath in NewDevicePath, delete it.
      //
      NewDevicePath = BmDelPartMatchInstance (NewDevicePath, CustomizedConDevicePath);
      //
      // In the first check, the default console variable will be _ModuleEntryPoint,
      // just append current customized device path
      //
      TempNewDevicePath = NewDevicePath;
      NewDevicePath = AppendDevicePathInstance (NewDevicePath, CustomizedConDevicePath);
      if (TempNewDevicePath != NULL) {
        FreePool(TempNewDevicePath);
      }
    }
  }

  //
  // Finally, Update the variable of the default console by NewDevicePath
  //
  Status = gRT->SetVariable (
                  mConVarName[ConsoleType],
                  &gEfiGlobalVariableGuid,
                  EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS
                                                  | ((ConsoleType < ConInDev) ? EFI_VARIABLE_NON_VOLATILE : 0),
                  GetDevicePathSize (NewDevicePath),
                  NewDevicePath
                  );

  if (VarConsole == NewDevicePath) {
    if (VarConsole != NULL) {
      FreePool(VarConsole);
    }
  } else {
    if (VarConsole != NULL) {
      FreePool(VarConsole);
    }
    if (NewDevicePath != NULL) {
      FreePool(NewDevicePath);
    }
  }

  return Status;
}


/**
  Connect the console device base on the variable ConsoleType.

  @param  ConsoleType              ConIn, ConOut or ErrOut.

  @retval EFI_NOT_FOUND            There is not any console devices connected
                                   success
  @retval EFI_SUCCESS              Success connect any one instance of the console
                                   device path base on the variable ConVarName.

**/
EFI_STATUS
EFIAPI
EfiBootManagerConnectConsoleVariable (
  IN  CONSOLE_TYPE              ConsoleType
  )
{
  EFI_STATUS                Status;
  EFI_DEVICE_PATH_PROTOCOL  *StartDevicePath;
  EFI_DEVICE_PATH_PROTOCOL  *Instance;
  EFI_DEVICE_PATH_PROTOCOL  *Next;
  EFI_DEVICE_PATH_PROTOCOL  *CopyOfDevicePath;
  UINTN                     Size;
  BOOLEAN                   DeviceExist;
  EFI_HANDLE                Handle;

  if ((ConsoleType != ConIn) && (ConsoleType != ConOut) && (ConsoleType != ErrOut)) {
    return EFI_INVALID_PARAMETER;
  }

  Status      = EFI_SUCCESS;
  DeviceExist = FALSE;
  Handle      = NULL;

  //
  // Check if the console variable exist
  //
  GetEfiGlobalVariable2 (mConVarName[ConsoleType], (VOID **) &StartDevicePath, NULL);
  if (StartDevicePath == NULL) {
    return EFI_UNSUPPORTED;
  }

  CopyOfDevicePath = StartDevicePath;
  do {
    //
    // Check every instance of the console variable
    //
    Instance  = GetNextDevicePathInstance (&CopyOfDevicePath, &Size);
    if (Instance == NULL) {
      FreePool (StartDevicePath);
      return EFI_UNSUPPORTED;
    }

    Next      = Instance;
    while (!IsDevicePathEndType (Next)) {
      Next = NextDevicePathNode (Next);
    }

    SetDevicePathEndNode (Next);
    //
    // Connect the USB console
    // USB console device path is a short-form device path that
    //  starts with the first element being a USB WWID
    //  or a USB Class device path
    //
    if ((DevicePathType (Instance) == MESSAGING_DEVICE_PATH) &&
        ((DevicePathSubType (Instance) == MSG_USB_CLASS_DP) || (DevicePathSubType (Instance) == MSG_USB_WWID_DP))
       ) {
      Status = BmConnectUsbShortFormDevicePath (Instance);
      if (!EFI_ERROR (Status)) {
        DeviceExist = TRUE;
      }
    } else {
      for (Next = Instance; !IsDevicePathEnd (Next); Next = NextDevicePathNode (Next)) {
        if (DevicePathType (Next) == ACPI_DEVICE_PATH && DevicePathSubType (Next) == ACPI_ADR_DP) {
          break;
        } else if (DevicePathType (Next) == HARDWARE_DEVICE_PATH &&
                   DevicePathSubType (Next) == HW_CONTROLLER_DP &&
                   DevicePathType (NextDevicePathNode (Next)) == ACPI_DEVICE_PATH &&
                   DevicePathSubType (NextDevicePathNode (Next)) == ACPI_ADR_DP
                   ) {
          break;
        }
      }
      if (!IsDevicePathEnd (Next)) {
        //
        // For GOP device path, start the video driver with NULL remaining device path
        //
        SetDevicePathEndNode (Next);
        Status = EfiBootManagerConnectDevicePath (Instance, &Handle);
        if (!EFI_ERROR (Status)) {
          gBS->ConnectController (Handle, NULL, NULL, TRUE);
        }
      } else {
        Status = EfiBootManagerConnectDevicePath (Instance, NULL);
      }
      if (EFI_ERROR (Status)) {
        //
        // Delete the instance from the console varialbe
        //
        EfiBootManagerUpdateConsoleVariable (ConsoleType, NULL, Instance);
      } else {
        DeviceExist = TRUE;
      }
    }
    FreePool(Instance);
  } while (CopyOfDevicePath != NULL);

  FreePool (StartDevicePath);

  if (!DeviceExist) {
    return EFI_NOT_FOUND;
  }

  return EFI_SUCCESS;
}


/**
  This function will search every input/output device in current system,
  and make every input/output device as potential console device.
**/
VOID
EFIAPI
EfiBootManagerConnectAllConsoles (
  VOID
  )
{
  UINTN                     Index;
  EFI_DEVICE_PATH_PROTOCOL  *ConDevicePath;
  UINTN                     HandleCount;
  EFI_HANDLE                *HandleBuffer;

  Index         = 0;
  HandleCount   = 0;
  HandleBuffer  = NULL;
  ConDevicePath = NULL;

  //
  // Update all the console variables
  //
  gBS->LocateHandleBuffer (
          ByProtocol,
          &gEfiSimpleTextInProtocolGuid,
          NULL,
          &HandleCount,
          &HandleBuffer
          );

  for (Index = 0; Index < HandleCount; Index++) {
    gBS->HandleProtocol (
            HandleBuffer[Index],
            &gEfiDevicePathProtocolGuid,
            (VOID **) &ConDevicePath
            );
    EfiBootManagerUpdateConsoleVariable (ConIn, ConDevicePath, NULL);
  }

  if (HandleBuffer != NULL) {
    FreePool(HandleBuffer);
    HandleBuffer = NULL;
  }

  gBS->LocateHandleBuffer (
          ByProtocol,
          &gEfiSimpleTextOutProtocolGuid,
          NULL,
          &HandleCount,
          &HandleBuffer
          );
  for (Index = 0; Index < HandleCount; Index++) {
    gBS->HandleProtocol (
            HandleBuffer[Index],
            &gEfiDevicePathProtocolGuid,
            (VOID **) &ConDevicePath
            );
    EfiBootManagerUpdateConsoleVariable (ConOut, ConDevicePath, NULL);
    EfiBootManagerUpdateConsoleVariable (ErrOut, ConDevicePath, NULL);
  }

  if (HandleBuffer != NULL) {
    FreePool(HandleBuffer);
  }

  //
  // Connect all console variables
  //
  EfiBootManagerConnectAllDefaultConsoles ();
}


/**
  This function will connect all the console devices base on the console
  device variable ConIn, ConOut and ErrOut.

  @retval EFI_DEVICE_ERROR         All the consoles were not connected due to an error.
  @retval EFI_SUCCESS              Success connect any one instance of the console
                                   device path base on the variable ConVarName.
**/
EFI_STATUS
EFIAPI
EfiBootManagerConnectAllDefaultConsoles (
  VOID
  )
{
  EFI_STATUS                Status;
  BOOLEAN                   OneConnected;
  BOOLEAN                   SystemTableUpdated;

  OneConnected = FALSE;

  Status = EfiBootManagerConnectConsoleVariable (ConOut);
  if (!EFI_ERROR (Status)) {
    OneConnected = TRUE;
  }
  PERF_EVENT ("ConOutReady");


  Status = EfiBootManagerConnectConsoleVariable (ConIn);
  if (!EFI_ERROR (Status)) {
    OneConnected = TRUE;
  }
  PERF_EVENT ("ConInReady");

  Status = EfiBootManagerConnectConsoleVariable (ErrOut);
  if (!EFI_ERROR (Status)) {
    OneConnected = TRUE;
  }
  PERF_EVENT ("ErrOutReady");

  SystemTableUpdated = FALSE;
  //
  // Fill console handles in System Table if no console device assignd.
  //
  if (BmUpdateSystemTableConsole (L"ConIn", &gEfiSimpleTextInProtocolGuid, &gST->ConsoleInHandle, (VOID **) &gST->ConIn)) {
    SystemTableUpdated = TRUE;
  }
  if (BmUpdateSystemTableConsole (L"ConOut", &gEfiSimpleTextOutProtocolGuid, &gST->ConsoleOutHandle, (VOID **) &gST->ConOut)) {
    SystemTableUpdated = TRUE;
  }
  if (BmUpdateSystemTableConsole (L"ErrOut", &gEfiSimpleTextOutProtocolGuid, &gST->StandardErrorHandle, (VOID **) &gST->StdErr)) {
    SystemTableUpdated = TRUE;
  }

  if (SystemTableUpdated) {
    //
    // Update the CRC32 in the EFI System Table header
    //
    gST->Hdr.CRC32 = 0;
    gBS->CalculateCrc32 (
          (UINT8 *) &gST->Hdr,
          gST->Hdr.HeaderSize,
          &gST->Hdr.CRC32
          );
  }

  return OneConnected ? EFI_SUCCESS : EFI_DEVICE_ERROR;
}
