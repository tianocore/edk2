/** @file
  Implementation for a generic GOP driver.

Copyright (c) 2016, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent


**/

#include "GraphicsOutput.h"
CONST ACPI_ADR_DEVICE_PATH mGraphicsOutputAdrNode = {
  {
    ACPI_DEVICE_PATH,
    ACPI_ADR_DP,
    { sizeof (ACPI_ADR_DEVICE_PATH), 0 },
  },
  ACPI_DISPLAY_ADR (1, 0, 0, 1, 0, ACPI_ADR_DISPLAY_TYPE_VGA, 0, 0)
};

EFI_PEI_GRAPHICS_DEVICE_INFO_HOB mDefaultGraphicsDeviceInfo = {
  MAX_UINT16, MAX_UINT16, MAX_UINT16, MAX_UINT16, MAX_UINT8, MAX_UINT8
};

//
// The driver should only start on one graphics controller.
// So a global flag is used to remember that the driver is already started.
//
BOOLEAN mDriverStarted = FALSE;

/**
  Returns information for an available graphics mode that the graphics device
  and the set of active video output devices supports.

  @param  This                  The EFI_GRAPHICS_OUTPUT_PROTOCOL instance.
  @param  ModeNumber            The mode number to return information on.
  @param  SizeOfInfo            A pointer to the size, in bytes, of the Info buffer.
  @param  Info                  A pointer to callee allocated buffer that returns information about ModeNumber.

  @retval EFI_SUCCESS           Valid mode information was returned.
  @retval EFI_DEVICE_ERROR      A hardware error occurred trying to retrieve the video mode.
  @retval EFI_INVALID_PARAMETER ModeNumber is not valid.

**/
EFI_STATUS
EFIAPI
GraphicsOutputQueryMode (
  IN  EFI_GRAPHICS_OUTPUT_PROTOCOL          *This,
  IN  UINT32                                ModeNumber,
  OUT UINTN                                 *SizeOfInfo,
  OUT EFI_GRAPHICS_OUTPUT_MODE_INFORMATION  **Info
  )
{
  if (This == NULL || Info == NULL || SizeOfInfo == NULL || ModeNumber >= This->Mode->MaxMode) {
    return EFI_INVALID_PARAMETER;
  }

  *SizeOfInfo = This->Mode->SizeOfInfo;
  *Info       = AllocateCopyPool (*SizeOfInfo, This->Mode->Info);
  return EFI_SUCCESS;
}

/**
  Set the video device into the specified mode and clears the visible portions of
  the output display to black.

  @param  This              The EFI_GRAPHICS_OUTPUT_PROTOCOL instance.
  @param  ModeNumber        Abstraction that defines the current video mode.

  @retval EFI_SUCCESS       The graphics mode specified by ModeNumber was selected.
  @retval EFI_DEVICE_ERROR  The device had an error and could not complete the request.
  @retval EFI_UNSUPPORTED   ModeNumber is not supported by this device.

**/
EFI_STATUS
EFIAPI
GraphicsOutputSetMode (
  IN  EFI_GRAPHICS_OUTPUT_PROTOCOL *This,
  IN  UINT32                       ModeNumber
)
{
  RETURN_STATUS                    Status;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL    Black;
  GRAPHICS_OUTPUT_PRIVATE_DATA     *Private;

  if (ModeNumber >= This->Mode->MaxMode) {
    return EFI_UNSUPPORTED;
  }

  Private = GRAPHICS_OUTPUT_PRIVATE_FROM_THIS (This);

  Black.Blue = 0;
  Black.Green = 0;
  Black.Red = 0;
  Black.Reserved = 0;

  Status = FrameBufferBlt (
             Private->FrameBufferBltLibConfigure,
             &Black,
             EfiBltVideoFill,
             0, 0,
             0, 0,
             This->Mode->Info->HorizontalResolution,
             This->Mode->Info->VerticalResolution,
             0
             );
  return RETURN_ERROR (Status) ? EFI_DEVICE_ERROR : EFI_SUCCESS;
}

/**
  Blt a rectangle of pixels on the graphics screen. Blt stands for BLock Transfer.

  @param  This         Protocol instance pointer.
  @param  BltBuffer    The data to transfer to the graphics screen.
                       Size is at least Width*Height*sizeof(EFI_GRAPHICS_OUTPUT_BLT_PIXEL).
  @param  BltOperation The operation to perform when copying BltBuffer on to the graphics screen.
  @param  SourceX      The X coordinate of source for the BltOperation.
  @param  SourceY      The Y coordinate of source for the BltOperation.
  @param  DestinationX The X coordinate of destination for the BltOperation.
  @param  DestinationY The Y coordinate of destination for the BltOperation.
  @param  Width        The width of a rectangle in the blt rectangle in pixels.
  @param  Height       The height of a rectangle in the blt rectangle in pixels.
  @param  Delta        Not used for EfiBltVideoFill or the EfiBltVideoToVideo operation.
                       If a Delta of zero is used, the entire BltBuffer is being operated on.
                       If a subrectangle of the BltBuffer is being used then Delta
                       represents the number of bytes in a row of the BltBuffer.

  @retval EFI_SUCCESS           BltBuffer was drawn to the graphics screen.
  @retval EFI_INVALID_PARAMETER BltOperation is not valid.
  @retval EFI_DEVICE_ERROR      The device had an error and could not complete the request.

**/
EFI_STATUS
EFIAPI
GraphicsOutputBlt (
  IN  EFI_GRAPHICS_OUTPUT_PROTOCOL      *This,
  IN  EFI_GRAPHICS_OUTPUT_BLT_PIXEL     *BltBuffer, OPTIONAL
  IN  EFI_GRAPHICS_OUTPUT_BLT_OPERATION BltOperation,
  IN  UINTN                             SourceX,
  IN  UINTN                             SourceY,
  IN  UINTN                             DestinationX,
  IN  UINTN                             DestinationY,
  IN  UINTN                             Width,
  IN  UINTN                             Height,
  IN  UINTN                             Delta         OPTIONAL
  )
{
  RETURN_STATUS                         Status;
  EFI_TPL                               Tpl;
  GRAPHICS_OUTPUT_PRIVATE_DATA          *Private;

  Private = GRAPHICS_OUTPUT_PRIVATE_FROM_THIS (This);
  //
  // We have to raise to TPL_NOTIFY, so we make an atomic write to the frame buffer.
  // We would not want a timer based event (Cursor, ...) to come in while we are
  // doing this operation.
  //
  Tpl = gBS->RaiseTPL (TPL_NOTIFY);
  Status = FrameBufferBlt (
             Private->FrameBufferBltLibConfigure,
             BltBuffer,
             BltOperation,
             SourceX, SourceY,
             DestinationX, DestinationY, Width, Height,
             Delta
             );
  gBS->RestoreTPL (Tpl);

  return RETURN_ERROR (Status) ? EFI_INVALID_PARAMETER : EFI_SUCCESS;
}

CONST GRAPHICS_OUTPUT_PRIVATE_DATA mGraphicsOutputInstanceTemplate = {
  GRAPHICS_OUTPUT_PRIVATE_DATA_SIGNATURE,          // Signature
  NULL,                                            // GraphicsOutputHandle
  {
    GraphicsOutputQueryMode,
    GraphicsOutputSetMode,
    GraphicsOutputBlt,
    NULL                                           // Mode
  },
  {
    1,                                             // MaxMode
    0,                                             // Mode
    NULL,                                          // Info
    sizeof (EFI_GRAPHICS_OUTPUT_MODE_INFORMATION), // SizeOfInfo
    0,                                             // FrameBufferBase
    0                                              // FrameBufferSize
  },
  NULL,                                            // DevicePath
  NULL,                                            // PciIo
  0,                                               // PciAttributes
  NULL,                                            // FrameBufferBltLibConfigure
  0                                                // FrameBufferBltLibConfigureSize
};

/**
  Test whether the Controller can be managed by the driver.

  @param  This                 Driver Binding protocol instance pointer.
  @param  Controller           The PCI controller.
  @param  RemainingDevicePath  Optional parameter use to pick a specific child
                               device to start.

  @retval EFI_SUCCESS          The driver can manage the video device.
  @retval other                The driver cannot manage the video device.
**/
EFI_STATUS
EFIAPI
GraphicsOutputDriverBindingSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL    *This,
  IN EFI_HANDLE                     Controller,
  IN EFI_DEVICE_PATH_PROTOCOL       *RemainingDevicePath
  )
{
  EFI_STATUS                        Status;
  EFI_PCI_IO_PROTOCOL               *PciIo;
  EFI_DEVICE_PATH_PROTOCOL          *DevicePath;

  //
  // Since there is only one GraphicsInfo HOB, the driver only manages one video device.
  //
  if (mDriverStarted) {
    return EFI_ALREADY_STARTED;
  }

  //
  // Test the PCI I/O Protocol
  //
  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiPciIoProtocolGuid,
                  (VOID **) &PciIo,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (Status == EFI_ALREADY_STARTED) {
    Status = EFI_SUCCESS;
  }
  if (EFI_ERROR (Status)) {
    return Status;
  }
  gBS->CloseProtocol (
         Controller,
         &gEfiPciIoProtocolGuid,
         This->DriverBindingHandle,
         Controller
         );

  //
  // Test the DevicePath protocol
  //
  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiDevicePathProtocolGuid,
                  (VOID **) &DevicePath,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (Status == EFI_ALREADY_STARTED) {
    Status = EFI_SUCCESS;
  }
  if (EFI_ERROR (Status)) {
    return Status;
  }
  gBS->CloseProtocol (
         Controller,
         &gEfiDevicePathProtocolGuid,
         This->DriverBindingHandle,
         Controller
         );

  if ((RemainingDevicePath == NULL) ||
      IsDevicePathEnd (RemainingDevicePath) ||
      CompareMem (RemainingDevicePath, &mGraphicsOutputAdrNode, sizeof (mGraphicsOutputAdrNode)) == 0) {
    return EFI_SUCCESS;
  } else {
    return EFI_INVALID_PARAMETER;
  }
}

/**
  Start the video controller.

  @param  This                 Driver Binding protocol instance pointer.
  @param  ControllerHandle     The PCI controller.
  @param  RemainingDevicePath  Optional parameter use to pick a specific child
                               device to start.

  @retval EFI_SUCCESS          The driver starts to manage the video device.
  @retval other                The driver cannot manage the video device.
**/
EFI_STATUS
EFIAPI
GraphicsOutputDriverBindingStart (
  IN EFI_DRIVER_BINDING_PROTOCOL    *This,
  IN EFI_HANDLE                     Controller,
  IN EFI_DEVICE_PATH_PROTOCOL       *RemainingDevicePath
  )
{
  EFI_STATUS                        Status;
  RETURN_STATUS                     ReturnStatus;
  GRAPHICS_OUTPUT_PRIVATE_DATA      *Private;
  EFI_PCI_IO_PROTOCOL               *PciIo;
  EFI_DEVICE_PATH                   *PciDevicePath;
  PCI_TYPE00                        Pci;
  UINT8                             Index;
  EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR *Resources;
  VOID                              *HobStart;
  EFI_PEI_GRAPHICS_INFO_HOB         *GraphicsInfo;
  EFI_PEI_GRAPHICS_DEVICE_INFO_HOB  *DeviceInfo;
  EFI_PHYSICAL_ADDRESS              FrameBufferBase;

  FrameBufferBase = 0;

  HobStart = GetFirstGuidHob (&gEfiGraphicsInfoHobGuid);
  ASSERT ((HobStart != NULL) && (GET_GUID_HOB_DATA_SIZE (HobStart) == sizeof (EFI_PEI_GRAPHICS_INFO_HOB)));
  GraphicsInfo = (EFI_PEI_GRAPHICS_INFO_HOB *) (GET_GUID_HOB_DATA (HobStart));

  HobStart = GetFirstGuidHob (&gEfiGraphicsDeviceInfoHobGuid);
  if ((HobStart == NULL) || (GET_GUID_HOB_DATA_SIZE (HobStart) < sizeof (*DeviceInfo))) {
    //
    // Use default device infomation when the device info HOB doesn't exist
    //
    DeviceInfo = &mDefaultGraphicsDeviceInfo;
    DEBUG ((DEBUG_INFO, "[%a]: GraphicsDeviceInfo HOB doesn't exist!\n", gEfiCallerBaseName));
  } else {
    DeviceInfo = (EFI_PEI_GRAPHICS_DEVICE_INFO_HOB *) (GET_GUID_HOB_DATA (HobStart));
    DEBUG ((DEBUG_INFO, "[%a]: GraphicsDeviceInfo HOB:\n"
            "  VendorId = %04x, DeviceId = %04x,\n"
            "  RevisionId = %02x, BarIndex = %x,\n"
            "  SubsystemVendorId = %04x, SubsystemId = %04x\n",
            gEfiCallerBaseName,
            DeviceInfo->VendorId, DeviceInfo->DeviceId,
            DeviceInfo->RevisionId, DeviceInfo->BarIndex,
            DeviceInfo->SubsystemVendorId, DeviceInfo->SubsystemId));
  }

  //
  // Open the PCI I/O Protocol
  //
  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiPciIoProtocolGuid,
                  (VOID **) &PciIo,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (Status == EFI_ALREADY_STARTED) {
    Status = EFI_SUCCESS;
  }
  ASSERT_EFI_ERROR (Status);

  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiDevicePathProtocolGuid,
                  (VOID **) &PciDevicePath,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (Status == EFI_ALREADY_STARTED) {
    Status = EFI_SUCCESS;
  }
  ASSERT_EFI_ERROR (Status);

  //
  // Read the PCI Class Code from the PCI Device
  //
  Status = PciIo->Pci.Read (PciIo, EfiPciIoWidthUint8, 0, sizeof (Pci), &Pci);
  if (!EFI_ERROR (Status)) {
    if (!IS_PCI_DISPLAY (&Pci) || (
        ((DeviceInfo->VendorId != MAX_UINT16) && (DeviceInfo->VendorId != Pci.Hdr.VendorId)) ||
        ((DeviceInfo->DeviceId != MAX_UINT16) && (DeviceInfo->DeviceId != Pci.Hdr.DeviceId)) ||
        ((DeviceInfo->RevisionId != MAX_UINT8) && (DeviceInfo->RevisionId != Pci.Hdr.RevisionID)) ||
        ((DeviceInfo->SubsystemVendorId != MAX_UINT16) && (DeviceInfo->SubsystemVendorId != Pci.Device.SubsystemVendorID)) ||
        ((DeviceInfo->SubsystemId != MAX_UINT16) && (DeviceInfo->SubsystemId != Pci.Device.SubsystemID))
        )
        ) {
      //
      // It's not a video device, or device infomation doesn't match.
      //
      Status = EFI_UNSUPPORTED;
    } else {
      //
      // If it's a video device and device information matches, use the BarIndex
      // from device information, or any BAR if BarIndex is not specified
      // whose size >= the frame buffer size from GraphicsInfo HOB.
      // Store the new frame buffer base.
      //
      for (Index = 0; Index < MAX_PCI_BAR; Index++) {
        if ((DeviceInfo->BarIndex != MAX_UINT8) && (DeviceInfo->BarIndex != Index)) {
          continue;
        }
        Status = PciIo->GetBarAttributes (PciIo, Index, NULL, (VOID**) &Resources);
        if (!EFI_ERROR (Status)) {
          DEBUG ((DEBUG_INFO, "[%a]: BAR[%d]: Base = %lx, Length = %lx\n",
                  gEfiCallerBaseName, Index, Resources->AddrRangeMin, Resources->AddrLen));
          if ((Resources->Desc == ACPI_ADDRESS_SPACE_DESCRIPTOR) &&
            (Resources->Len == (UINT16) (sizeof (EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR) - 3)) &&
              (Resources->ResType == ACPI_ADDRESS_SPACE_TYPE_MEM) &&
              (Resources->AddrLen >= GraphicsInfo->FrameBufferSize)
              ) {
            if (FrameBufferBase == 0) {
              FrameBufferBase = Resources->AddrRangeMin;
            }
            if (DeviceInfo->BarIndex == MAX_UINT8) {
              if (Resources->AddrRangeMin == GraphicsInfo->FrameBufferBase) {
                FrameBufferBase = Resources->AddrRangeMin;
                break;
              }
            } else {
              break;
            }
          }
        }
      }
      if (Index == MAX_PCI_BAR) {
        Status = EFI_UNSUPPORTED;
      } else {
        DEBUG ((DEBUG_INFO, "[%a]: ... matched!\n", gEfiCallerBaseName));
      }
    }
  }

  if (EFI_ERROR (Status)) {
    goto CloseProtocols;
  }

  if ((RemainingDevicePath != NULL) && IsDevicePathEnd (RemainingDevicePath)) {
    return EFI_SUCCESS;
  }

  Private = AllocateCopyPool (sizeof (mGraphicsOutputInstanceTemplate), &mGraphicsOutputInstanceTemplate);
  if (Private == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto CloseProtocols;
  }

  Private->GraphicsOutputMode.FrameBufferBase = FrameBufferBase;
  Private->GraphicsOutputMode.FrameBufferSize = GraphicsInfo->FrameBufferSize;
  Private->GraphicsOutputMode.Info = &GraphicsInfo->GraphicsMode;

  //
  // Fix up Mode pointer in GraphicsOutput
  //
  Private->GraphicsOutput.Mode = &Private->GraphicsOutputMode;

  //
  // Set attributes
  //
  Status = PciIo->Attributes (
                    PciIo,
                    EfiPciIoAttributeOperationGet,
                    0,
                    &Private->PciAttributes
                    );
  if (!EFI_ERROR (Status)) {
    Status = PciIo->Attributes (
                      PciIo,
                      EfiPciIoAttributeOperationEnable,
                      EFI_PCI_DEVICE_ENABLE,
                      NULL
                      );
  }

  if (EFI_ERROR (Status)) {
    goto FreeMemory;
  }

  //
  // Create the FrameBufferBltLib configuration.
  //
  ReturnStatus = FrameBufferBltConfigure (
                   (VOID *) (UINTN) Private->GraphicsOutput.Mode->FrameBufferBase,
                   Private->GraphicsOutput.Mode->Info,
                   Private->FrameBufferBltLibConfigure,
                   &Private->FrameBufferBltLibConfigureSize
                   );
  if (ReturnStatus == RETURN_BUFFER_TOO_SMALL) {
    Private->FrameBufferBltLibConfigure = AllocatePool (Private->FrameBufferBltLibConfigureSize);
    if (Private->FrameBufferBltLibConfigure != NULL) {
      ReturnStatus = FrameBufferBltConfigure (
                       (VOID *) (UINTN) Private->GraphicsOutput.Mode->FrameBufferBase,
                       Private->GraphicsOutput.Mode->Info,
                       Private->FrameBufferBltLibConfigure,
                       &Private->FrameBufferBltLibConfigureSize
                       );
    }
  }
  if (RETURN_ERROR (ReturnStatus)) {
    Status = EFI_OUT_OF_RESOURCES;
    goto RestorePciAttributes;
  }

  Private->DevicePath = AppendDevicePathNode (PciDevicePath, (EFI_DEVICE_PATH_PROTOCOL *) &mGraphicsOutputAdrNode);
  if (Private->DevicePath == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto RestorePciAttributes;
  }

  Status = gBS->InstallMultipleProtocolInterfaces (
                  &Private->GraphicsOutputHandle,
                  &gEfiGraphicsOutputProtocolGuid, &Private->GraphicsOutput,
                  &gEfiDevicePathProtocolGuid, Private->DevicePath,
                  NULL
                  );

  if (!EFI_ERROR (Status)) {
    Status = gBS->OpenProtocol (
                    Controller,
                    &gEfiPciIoProtocolGuid,
                    (VOID **) &Private->PciIo,
                    This->DriverBindingHandle,
                    Private->GraphicsOutputHandle,
                    EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER
                    );
    if (!EFI_ERROR (Status)) {
      mDriverStarted = TRUE;
    } else {
      gBS->UninstallMultipleProtocolInterfaces (
             Private->GraphicsOutputHandle,
             &gEfiGraphicsOutputProtocolGuid, &Private->GraphicsOutput,
             &gEfiDevicePathProtocolGuid, Private->DevicePath,
             NULL
             );
    }
  }

RestorePciAttributes:
  if (EFI_ERROR (Status)) {
    //
    // Restore original PCI attributes
    //
    PciIo->Attributes (
             PciIo,
             EfiPciIoAttributeOperationSet,
             Private->PciAttributes,
             NULL
             );
  }

FreeMemory:
  if (EFI_ERROR (Status)) {
    if (Private != NULL) {
      if (Private->DevicePath != NULL) {
        FreePool (Private->DevicePath);
      }
      if (Private->FrameBufferBltLibConfigure != NULL) {
        FreePool (Private->FrameBufferBltLibConfigure);
      }
      FreePool (Private);
    }
  }

CloseProtocols:
  if (EFI_ERROR (Status)) {
    //
    // Close the PCI I/O Protocol
    //
    gBS->CloseProtocol (
           Controller,
           &gEfiDevicePathProtocolGuid,
           This->DriverBindingHandle,
           Controller
           );

    //
    // Close the PCI I/O Protocol
    //
    gBS->CloseProtocol (
           Controller,
           &gEfiPciIoProtocolGuid,
           This->DriverBindingHandle,
           Controller
           );
  }
  return Status;
}

/**
  Stop the video controller.

  @param  This                 Driver Binding protocol instance pointer.
  @param  Controller           The PCI controller.
  @param  NumberOfChildren     The number of child device handles in ChildHandleBuffer.
  @param  ChildHandleBuffer    An array of child handles to be freed. May be NULL
                               if NumberOfChildren is 0.

  @retval EFI_SUCCESS          The device was stopped.
  @retval EFI_DEVICE_ERROR     The device could not be stopped due to a device error.
**/
EFI_STATUS
EFIAPI
GraphicsOutputDriverBindingStop (
  IN EFI_DRIVER_BINDING_PROTOCOL    *This,
  IN EFI_HANDLE                     Controller,
  IN UINTN                          NumberOfChildren,
  IN EFI_HANDLE                     *ChildHandleBuffer
  )
{
  EFI_STATUS                        Status;
  EFI_GRAPHICS_OUTPUT_PROTOCOL      *Gop;
  GRAPHICS_OUTPUT_PRIVATE_DATA      *Private;

  if (NumberOfChildren == 0) {

    //
    // Close the PCI I/O Protocol
    //
    Status = gBS->CloseProtocol (
                    Controller,
                    &gEfiPciIoProtocolGuid,
                    This->DriverBindingHandle,
                    Controller
                    );
    ASSERT_EFI_ERROR (Status);

    Status = gBS->CloseProtocol (
                    Controller,
                    &gEfiDevicePathProtocolGuid,
                    This->DriverBindingHandle,
                    Controller
                    );
    ASSERT_EFI_ERROR (Status);
    return EFI_SUCCESS;
  }

  ASSERT (NumberOfChildren == 1);
  Status = gBS->OpenProtocol (
                  ChildHandleBuffer[0],
                  &gEfiGraphicsOutputProtocolGuid,
                  (VOID **) &Gop,
                  This->DriverBindingHandle,
                  ChildHandleBuffer[0],
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Private = GRAPHICS_OUTPUT_PRIVATE_FROM_THIS (Gop);

  Status = gBS->CloseProtocol (
                  Controller,
                  &gEfiPciIoProtocolGuid,
                  This->DriverBindingHandle,
                  Private->GraphicsOutputHandle
                  );
  ASSERT_EFI_ERROR (Status);
  //
  // Remove the GOP protocol interface from the system
  //
  Status = gBS->UninstallMultipleProtocolInterfaces (
                  Private->GraphicsOutputHandle,
                  &gEfiGraphicsOutputProtocolGuid, &Private->GraphicsOutput,
                  &gEfiDevicePathProtocolGuid, Private->DevicePath,
                  NULL
                  );
  if (!EFI_ERROR (Status)) {
    //
    // Restore original PCI attributes
    //
    Status = Private->PciIo->Attributes (
                               Private->PciIo,
                               EfiPciIoAttributeOperationSet,
                               Private->PciAttributes,
                               NULL
                               );
    ASSERT_EFI_ERROR (Status);

    FreePool (Private->DevicePath);
    FreePool (Private->FrameBufferBltLibConfigure);
    mDriverStarted = FALSE;
  } else {
    Status = gBS->OpenProtocol (
                    Controller,
                    &gEfiPciIoProtocolGuid,
                    (VOID **) &Private->PciIo,
                    This->DriverBindingHandle,
                    Private->GraphicsOutputHandle,
                    EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER
                    );
    ASSERT_EFI_ERROR (Status);
  }
  return Status;
}

EFI_DRIVER_BINDING_PROTOCOL mGraphicsOutputDriverBinding = {
  GraphicsOutputDriverBindingSupported,
  GraphicsOutputDriverBindingStart,
  GraphicsOutputDriverBindingStop,
  0x10,
  NULL,
  NULL
};

/**
  The Entry Point for GraphicsOutput driver.

  It installs DriverBinding, ComponentName and ComponentName2 protocol if there is
  GraphicsInfo HOB passed from Graphics PEIM.

  @param[in] ImageHandle    The firmware allocated handle for the EFI image.
  @param[in] SystemTable    A pointer to the EFI System Table.

  @retval EFI_SUCCESS       The entry point is executed successfully.
  @retval other             Some error occurs when executing this entry point.

**/
EFI_STATUS
EFIAPI
InitializeGraphicsOutput (
  IN EFI_HANDLE                        ImageHandle,
  IN EFI_SYSTEM_TABLE                  *SystemTable
  )
{
  EFI_STATUS                           Status;
  VOID                                 *HobStart;

  HobStart = GetFirstGuidHob (&gEfiGraphicsInfoHobGuid);

  if ((HobStart == NULL) || (GET_GUID_HOB_DATA_SIZE (HobStart) < sizeof (EFI_PEI_GRAPHICS_INFO_HOB))) {
    return EFI_NOT_FOUND;
  }

  Status = EfiLibInstallDriverBindingComponentName2 (
             ImageHandle,
             SystemTable,
             &mGraphicsOutputDriverBinding,
             ImageHandle,
             &mGraphicsOutputComponentName,
             &mGraphicsOutputComponentName2
             );
  ASSERT_EFI_ERROR (Status);

  return Status;
}
