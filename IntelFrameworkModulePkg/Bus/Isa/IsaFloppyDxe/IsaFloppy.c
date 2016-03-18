/** @file
  ISA Floppy Disk UEFI Driver conforming to the UEFI driver model

  1. Support two types diskette drive  
     1.44M drive and 2.88M drive (and now only support 1.44M)
  2. Support two diskette drives per floppy disk controller
  3. Use DMA channel 2 to transfer data
  4. Do not use interrupt
  5. Support diskette change line signal and write protect
  
Copyright (c) 2006 - 2009, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "IsaFloppy.h"

LIST_ENTRY  mControllerHead = INITIALIZE_LIST_HEAD_VARIABLE (mControllerHead);

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
  The main Entry Point for this driver.

  @param[in] ImageHandle  The firmware allocated handle for the EFI image.  
  @param[in] SystemTable  A pointer to the EFI System Table.
  
  @retval EFI_SUCCESS     The entry point is executed successfully.
  @retval other           Some error occurs when executing this entry point.
**/
EFI_STATUS
EFIAPI
InitializeIsaFloppy(
  IN EFI_HANDLE           ImageHandle,
  IN EFI_SYSTEM_TABLE     *SystemTable
  )
{
  EFI_STATUS  Status;

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
  Test if the controller is a floppy disk drive device
  
  @param[in] This                 A pointer to the EFI_DRIVER_BINDING_PROTOCOL instance.  
  @param[in] Controller           The handle of the controller to test.
  @param[in] RemainingDevicePath  A pointer to the remaining portion of a device path.
  
  @retval EFI_SUCCESS             The device is supported by this driver.
  @retval EFI_ALREADY_STARTED     The device is already being managed by this driver.
  @retval EFI_ACCESS_DENIED       The device is already being managed by a different driver 
                                  or an application that requires exclusive access.
  @retval EFI_UNSUPPORTED         The device is is not supported by this driver.
**/
EFI_STATUS
EFIAPI
FdcControllerDriverSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   Controller,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  )
{
  EFI_STATUS                Status;
  EFI_ISA_IO_PROTOCOL       *IsaIo;
  EFI_DEVICE_PATH_PROTOCOL  *ParentDevicePath;

  //
  // Ignore the parameter RemainingDevicePath because this is a device driver.
  //

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

  gBS->CloseProtocol (
         Controller,
         &gEfiDevicePathProtocolGuid,
         This->DriverBindingHandle,
         Controller
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
    return Status;
  }
  //
  // Use the ISA I/O Protocol to see if Controller is a floppy disk drive device
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
  Start this driver on Controller.

  @param[in] This                  A pointer to the EFI_DRIVER_BINDING_PROTOCOL instance.
  @param[in] ControllerHandle      The handle of the controller to start. This handle 
                                   must support a protocol interface that supplies 
                                   an I/O abstraction to the driver.
  @param[in] RemainingDevicePath   A pointer to the remaining portion of a device path. 
                                   This parameter is ignored by device drivers, and is optional for bus drivers.

  @retval EFI_SUCCESS              The device was started.
  @retval EFI_DEVICE_ERROR         The device could not be started due to a device error.
                                   Currently not implemented.
  @retval EFI_OUT_OF_RESOURCES     The request could not be completed due to a lack of resources.
  @retval Others                   The driver failded to start the device.
**/
EFI_STATUS
EFIAPI
FdcControllerDriverStart (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   Controller,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  )
{
  EFI_STATUS                Status;
  FDC_BLK_IO_DEV            *FdcDev;
  EFI_ISA_IO_PROTOCOL       *IsaIo;
  UINTN                     Index;
  LIST_ENTRY                *List;
  BOOLEAN                   Found;
  EFI_DEVICE_PATH_PROTOCOL  *ParentDevicePath;

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
  // Allocate the floppy device's Device structure
  //
  FdcDev = AllocateZeroPool (sizeof (FDC_BLK_IO_DEV));
  if (FdcDev == NULL) {
    goto Done;
  }
  //
  // Initialize the floppy device's device structure
  //
  FdcDev->Signature       = FDC_BLK_IO_DEV_SIGNATURE;
  FdcDev->Handle          = Controller;
  FdcDev->IsaIo           = IsaIo;
  FdcDev->Disk            = (EFI_FDC_DISK) IsaIo->ResourceList->Device.UID;
  FdcDev->Cache           = NULL;
  FdcDev->Event           = NULL;
  FdcDev->ControllerState = NULL;
  FdcDev->DevicePath      = ParentDevicePath;

  FdcDev->ControllerNameTable = NULL;
  AddName (FdcDev);
  
  //
  // Look up the base address of the Floppy Disk Controller which controls this floppy device
  //
  for (Index = 0; FdcDev->IsaIo->ResourceList->ResourceItem[Index].Type != EfiIsaAcpiResourceEndOfList; Index++) {
    if (FdcDev->IsaIo->ResourceList->ResourceItem[Index].Type == EfiIsaAcpiResourceIo) {
      FdcDev->BaseAddress = (UINT16) FdcDev->IsaIo->ResourceList->ResourceItem[Index].StartRange;
    }
  }
  //
  // Maintain the list of floppy disk controllers
  //
  Found = FALSE;
  List  = mControllerHead.ForwardLink;
  while (List != &mControllerHead) {
    FdcDev->ControllerState = FLOPPY_CONTROLLER_FROM_LIST_ENTRY (List);
    if (FdcDev->BaseAddress == FdcDev->ControllerState->BaseAddress) {
      Found = TRUE;
      break;
    }

    List = List->ForwardLink;
  }

  if (!Found) {
    //
    // A new floppy disk controller controlling this floppy disk drive is found
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

    InsertTailList (&mControllerHead, &FdcDev->ControllerState->Link);
  }
  //
  // Create a timer event for each floppy disk drive device.
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
  if (!EFI_ERROR (Status)) {
    FdcDev->ControllerState->NumberOfDrive++;
  }

Done:
  if (EFI_ERROR (Status)) {

    REPORT_STATUS_CODE_WITH_DEVICE_PATH (
      EFI_ERROR_CODE | EFI_ERROR_MINOR,
      EFI_PERIPHERAL_REMOVABLE_MEDIA | EFI_P_EC_CONTROLLER_ERROR,
      ParentDevicePath
      );

    //
    // If a floppy drive device structure was allocated, then free it
    //
    if (FdcDev != NULL) {
      if (FdcDev->Event != NULL) {
        //
        // Close the event for turning the motor off
        //
        gBS->CloseEvent (FdcDev->Event);
      }

      FreeUnicodeStringTable (FdcDev->ControllerNameTable);
      FreePool (FdcDev);
    }

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
    // Close the device path protocol
    //
    gBS->CloseProtocol (
           Controller,
           &gEfiDevicePathProtocolGuid,
           This->DriverBindingHandle,
           Controller
           );
  }

  return Status;
}

/**
  Stop this driver on ControllerHandle.

  @param[in] This               A pointer to the EFI_DRIVER_BINDING_PROTOCOL instance.
  @param[in] ControllerHandle   A handle to the device being stopped. The handle must 
                                support a bus specific I/O protocol for the driver 
                                to use to stop the device.
  @param[in] NumberOfChildren   The number of child device handles in ChildHandleBuffer.
  @param[in] ChildHandleBuffer  An array of child handles to be freed. May be NULL 
                                if NumberOfChildren is 0.

  @retval EFI_SUCCESS           The device was stopped.
  @retval EFI_DEVICE_ERROR      The device could not be stopped due to a device error.
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
  // Ignore NumberOfChildren since this is a device driver
  //

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
  // Get the floppy drive device's Device structure
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
  // Close the event for turning the motor off
  //
  gBS->CloseEvent (FdcDev->Event);

  //
  // Turn the motor off on the floppy drive device
  //
  FddTimerProc (FdcDev->Event, FdcDev);

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
  // Free the cache if one was allocated
  //
  FdcFreeCache (FdcDev);

  //
  // Free the floppy drive device's device structure
  //
  FreeUnicodeStringTable (FdcDev->ControllerNameTable);
  FreePool (FdcDev);

  return EFI_SUCCESS;
}

