/**@file
  ISA Floppy Driver
  1. Support two types diskette drive  
     1.44M drive and 2.88M drive (and now only support 1.44M)
  2. Support two diskette drives
  3. Use DMA channel 2 to transfer data
  4. Do not use interrupt
  5. Support diskette change line signal and write protect
  
  conforming to EFI driver model
  
Copyright (c) 2006 - 2007, Intel Corporation.<BR>
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "IsaFloppy.h"

LIST_ENTRY              gControllerHead = INITIALIZE_LIST_HEAD_VARIABLE(gControllerHead);

//
// ISA Floppy Driver Binding Protocol
//
EFI_DRIVER_BINDING_PROTOCOL gFdcControllerDriver = {
  FdcControllerDriverSupported,
  FdcControllerDriverStart,
  FdcControllerDriverStop,
  0xa,
  NULL,
  NULL
};


/**
  The user Entry Point for module IsaFloppy. The user code starts with this function.

  @param[in] ImageHandle    The firmware allocated handle for the EFI image.  
  @param[in] SystemTable    A pointer to the EFI System Table.
  
  @retval EFI_SUCCESS       The entry point is executed successfully.
  @retval other             Some error occurs when executing this entry point.

**/
EFI_STATUS
EFIAPI
InitializeIsaFloppy(
  IN EFI_HANDLE           ImageHandle,
  IN EFI_SYSTEM_TABLE     *SystemTable
  )
{
  EFI_STATUS              Status;

  //
  // Install driver model protocol(s).
  //
  Status = EfiLibInstallDriverBindingComponentName2 (
             ImageHandle,
             SystemTable,
             &gFdcControllerDriver,
             ImageHandle,
             &gIsaFloppyComponentName,
             &gIsaFloppyComponentName2
             );
  ASSERT_EFI_ERROR (Status);


  return Status;
}

/**
  Test controller is a Floppy Disk Controller
  
  @param This                 Pointer of EFI_DRIVER_BINDING_PROTOCOL
  @param Controller           driver's controller
  @param RemainingDevicePath  children device path
  
  @retval EFI_UNSUPPORTED controller is not floppy disk
  @retval EFI_SUCCESS     controller is floppy disk
**/
EFI_STATUS
EFIAPI
FdcControllerDriverSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   Controller,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  )
{
  EFI_STATUS                          Status;
  EFI_ISA_IO_PROTOCOL                 *IsaIo;

  //
  // Open the ISA I/O Protocol
  //
  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiIsaIoProtocolGuid,
                  (VOID **) &IsaIo,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }
  //
  // Use the ISA I/O Protocol to see if Controller is a Floppy Disk Controller
  //
  Status = EFI_SUCCESS;
  if (IsaIo->ResourceList->Device.HID != EISA_PNP_ID (0x604)) {
    Status = EFI_UNSUPPORTED;
  }
  //
  // Close the ISA I/O Protocol
  //
  gBS->CloseProtocol (
         Controller,
         &gEfiIsaIoProtocolGuid,
         This->DriverBindingHandle,
         Controller
         );

  return Status;
}

/**
  Create floppy control instance on controller.
  
  @param This         Pointer of EFI_DRIVER_BINDING_PROTOCOL
  @param Controller   driver controller handle
  @param RemainingDevicePath Children's device path
  
  @retval whether success to create floppy control instance.
**/
EFI_STATUS
EFIAPI
FdcControllerDriverStart (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   Controller,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  )
{
  EFI_STATUS                                Status;
  FDC_BLK_IO_DEV                            *FdcDev;
  EFI_ISA_IO_PROTOCOL                       *IsaIo;
  UINTN                                     Index;
  LIST_ENTRY                                *List;
  BOOLEAN                                   Found;
  EFI_DEVICE_PATH_PROTOCOL                  *ParentDevicePath;

  FdcDev  = NULL;
  IsaIo   = NULL;

  //
  // Open the device path protocol
  //
  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiDevicePathProtocolGuid,
                  (VOID **) &ParentDevicePath,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }
  //
  // Report enable progress code
  //
  REPORT_STATUS_CODE_WITH_DEVICE_PATH (
    EFI_PROGRESS_CODE,
    EFI_PERIPHERAL_REMOVABLE_MEDIA | EFI_P_PC_ENABLE,
    ParentDevicePath
    );

  //
  // Open the ISA I/O Protocol
  //
  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiIsaIoProtocolGuid,
                  (VOID **) &IsaIo,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (EFI_ERROR (Status)) {
    goto Done;
  }
  //
  // Allocate the Floppy Disk Controller's Device structure
  //
  FdcDev = AllocateZeroPool (sizeof (FDC_BLK_IO_DEV));
  if (FdcDev == NULL) {
    goto Done;
  }
  //
  // Initialize the Floppy Disk Controller's Device structure
  //
  FdcDev->Signature       = FDC_BLK_IO_DEV_SIGNATURE;
  FdcDev->Handle          = Controller;
  FdcDev->IsaIo           = IsaIo;
  FdcDev->Disk            = (EFI_FDC_DISK) IsaIo->ResourceList->Device.UID;
  FdcDev->Cache           = NULL;
  FdcDev->Event           = NULL;
  FdcDev->ControllerState = NULL;
  FdcDev->DevicePath      = ParentDevicePath;

  ADD_FLOPPY_NAME (FdcDev);
  
  //
  // Look up the base address of the Floppy Disk Controller
  //
  for (Index = 0; FdcDev->IsaIo->ResourceList->ResourceItem[Index].Type != EfiIsaAcpiResourceEndOfList; Index++) {
    if (FdcDev->IsaIo->ResourceList->ResourceItem[Index].Type == EfiIsaAcpiResourceIo) {
      FdcDev->BaseAddress = (UINT16) FdcDev->IsaIo->ResourceList->ResourceItem[Index].StartRange;
    }
  }
  //
  // Maintain the list of controller list
  //
  Found = FALSE;
  List  = gControllerHead.ForwardLink;
  while (List != &gControllerHead) {
    FdcDev->ControllerState = FLOPPY_CONTROLLER_FROM_LIST_ENTRY (List);
    if (FdcDev->BaseAddress == FdcDev->ControllerState->BaseAddress) {
      Found = TRUE;
      break;
    }

    List = List->ForwardLink;
  }

  if (!Found) {
    //
    // The Controller is new
    //
    FdcDev->ControllerState = AllocatePool (sizeof (FLOPPY_CONTROLLER_CONTEXT));
    if (FdcDev->ControllerState == NULL) {
      goto Done;
    }

    FdcDev->ControllerState->Signature          = FLOPPY_CONTROLLER_CONTEXT_SIGNATURE;
    FdcDev->ControllerState->FddResetPerformed  = FALSE;
    FdcDev->ControllerState->NeedRecalibrate    = FALSE;
    FdcDev->ControllerState->BaseAddress        = FdcDev->BaseAddress;
    FdcDev->ControllerState->NumberOfDrive      = 0;

    InsertTailList (&gControllerHead, &FdcDev->ControllerState->Link);
  }
  //
  // Create a timer event for each Floppd Disk Controller.
  // This timer event is used to control the motor on and off
  //
  Status = gBS->CreateEvent (
                  EVT_TIMER | EVT_NOTIFY_SIGNAL,
                  TPL_NOTIFY,
                  FddTimerProc,
                  FdcDev,
                  &FdcDev->Event
                  );
  if (EFI_ERROR (Status)) {
    goto Done;
  }
  //
  // Reset the Floppy Disk Controller
  //
  if (!FdcDev->ControllerState->FddResetPerformed) {
    FdcDev->ControllerState->FddResetPerformed  = TRUE;
    FdcDev->ControllerState->FddResetStatus     = FddReset (FdcDev);
  }

  if (EFI_ERROR (FdcDev->ControllerState->FddResetStatus)) {
    Status = EFI_DEVICE_ERROR;
    goto Done;
  }

  REPORT_STATUS_CODE_WITH_DEVICE_PATH (
    EFI_PROGRESS_CODE,
    EFI_PERIPHERAL_REMOVABLE_MEDIA | EFI_P_PC_PRESENCE_DETECT,
    ParentDevicePath
    );

  //
  // Discover the Floppy Drive
  //
  Status = DiscoverFddDevice (FdcDev);
  if (EFI_ERROR (Status)) {
    Status = EFI_DEVICE_ERROR;
    goto Done;
  }
  //
  // Install protocol interfaces for the serial device.
  //
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &Controller,
                  &gEfiBlockIoProtocolGuid,
                  &FdcDev->BlkIo,
                  NULL
                  );

  FdcDev->ControllerState->NumberOfDrive++;

Done:
  if (EFI_ERROR (Status)) {

    REPORT_STATUS_CODE_WITH_DEVICE_PATH (
      EFI_ERROR_CODE | EFI_ERROR_MINOR,
      EFI_PERIPHERAL_REMOVABLE_MEDIA | EFI_P_EC_CONTROLLER_ERROR,
      ParentDevicePath
      );

    //
    // Close the device path protocol
    //
    gBS->CloseProtocol (
           Controller,
           &gEfiDevicePathProtocolGuid,
           This->DriverBindingHandle,
           Controller
           );

    //
    // Close the ISA I/O Protocol
    //
    if (IsaIo != NULL) {
      gBS->CloseProtocol (
             Controller,
             &gEfiIsaIoProtocolGuid,
             This->DriverBindingHandle,
             Controller
             );
    }
    //
    // If a Floppy Disk Controller Device structure was allocated, then free it
    //
    if (FdcDev != NULL) {
      if (FdcDev->Event != NULL) {
        //
        // Close the event for turning the motor off
        //
        gBS->CloseEvent (FdcDev->Event);
      }

      FreeUnicodeStringTable (FdcDev->ControllerNameTable);
      gBS->FreePool (FdcDev);
    }
  }

  return Status;
}

/**
  Stop this driver on ControllerHandle. Support stoping any child handles
  created by this driver.

  @param  This              Protocol instance pointer.
  @param  Controller        Handle of device to stop driver on
  @param  NumberOfChildren  Number of Handles in ChildHandleBuffer. If number of
                            children is zero stop the entire bus driver.
  @param  ChildHandleBuffer List of Child Handles to Stop.

  @retval EFI_SUCCESS       This driver is removed ControllerHandle
  @retval other             This driver was not removed from this device

**/
EFI_STATUS
EFIAPI
FdcControllerDriverStop (
  IN  EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN  EFI_HANDLE                   Controller,
  IN  UINTN                        NumberOfChildren,
  IN  EFI_HANDLE                   *ChildHandleBuffer
  )
{
  EFI_STATUS            Status;
  EFI_BLOCK_IO_PROTOCOL *BlkIo;
  FDC_BLK_IO_DEV        *FdcDev;

  //
  // Get the Block I/O Protocol on Controller
  //
  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiBlockIoProtocolGuid,
                  (VOID **) &BlkIo,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }
  //
  // Get the Floppy Disk Controller's Device structure
  //
  FdcDev = FDD_BLK_IO_FROM_THIS (BlkIo);

  //
  // Report disable progress code
  //
  REPORT_STATUS_CODE_WITH_DEVICE_PATH (
    EFI_PROGRESS_CODE,
    EFI_PERIPHERAL_REMOVABLE_MEDIA | EFI_P_PC_DISABLE,
    FdcDev->DevicePath
    );

  //
  // Turn the motor off on the Floppy Disk Controller
  //
  FddTimerProc (FdcDev->Event, FdcDev);

  //
  // Uninstall the Block I/O Protocol
  //
  Status = gBS->UninstallProtocolInterface (
                  Controller,
                  &gEfiBlockIoProtocolGuid,
                  &FdcDev->BlkIo
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }
  //
  // Close the device path protocol
  //
  gBS->CloseProtocol (
         Controller,
         &gEfiDevicePathProtocolGuid,
         This->DriverBindingHandle,
         Controller
         );

  //
  // Close the ISA I/O Protocol
  //
  gBS->CloseProtocol (
         Controller,
         &gEfiIsaIoProtocolGuid,
         This->DriverBindingHandle,
         Controller
         );

  //
  // Free the controller list if needed
  //
  FdcDev->ControllerState->NumberOfDrive--;

  //
  // Close the event for turning the motor off
  //
  gBS->CloseEvent (FdcDev->Event);

  //
  // Free the cache if one was allocated
  //
  FdcFreeCache (FdcDev);

  //
  // Free the Floppy Disk Controller's Device structure
  //
  FreeUnicodeStringTable (FdcDev->ControllerNameTable);
  gBS->FreePool (FdcDev);

  return EFI_SUCCESS;
}

