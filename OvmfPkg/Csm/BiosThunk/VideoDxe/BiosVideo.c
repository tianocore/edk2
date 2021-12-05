/** @file
  ConsoleOut Routines that speak VGA.

Copyright (c) 2007 - 2018, Intel Corporation. All rights reserved.<BR>

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "BiosVideo.h"

//
// EFI Driver Binding Protocol Instance
//
EFI_DRIVER_BINDING_PROTOCOL  gBiosVideoDriverBinding = {
  BiosVideoDriverBindingSupported,
  BiosVideoDriverBindingStart,
  BiosVideoDriverBindingStop,
  0x3,
  NULL,
  NULL
};

//
// Global lookup tables for VGA graphics modes
//
UINT8  mVgaLeftMaskTable[] = { 0xff, 0x7f, 0x3f, 0x1f, 0x0f, 0x07, 0x03, 0x01 };

UINT8  mVgaRightMaskTable[] = { 0x80, 0xc0, 0xe0, 0xf0, 0xf8, 0xfc, 0xfe, 0xff };

UINT8  mVgaBitMaskTable[] = { 0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01 };

//
// Save controller attributes during first start
//
UINT64   mOriginalPciAttributes;
BOOLEAN  mPciAttributesSaved = FALSE;

EFI_GRAPHICS_OUTPUT_BLT_PIXEL  mVgaColorToGraphicsOutputColor[] = {
  { 0x00, 0x00, 0x00, 0x00 },
  { 0x98, 0x00, 0x00, 0x00 },
  { 0x00, 0x98, 0x00, 0x00 },
  { 0x98, 0x98, 0x00, 0x00 },
  { 0x00, 0x00, 0x98, 0x00 },
  { 0x98, 0x00, 0x98, 0x00 },
  { 0x00, 0x98, 0x98, 0x00 },
  { 0x98, 0x98, 0x98, 0x00 },
  { 0x10, 0x10, 0x10, 0x00 },
  { 0xff, 0x10, 0x10, 0x00 },
  { 0x10, 0xff, 0x10, 0x00 },
  { 0xff, 0xff, 0x10, 0x00 },
  { 0x10, 0x10, 0xff, 0x00 },
  { 0xf0, 0x10, 0xff, 0x00 },
  { 0x10, 0xff, 0xff, 0x00 },
  { 0xff, 0xff, 0xff, 0x00 }
};

//
// Standard timing defined by VESA EDID
//
VESA_BIOS_EXTENSIONS_EDID_TIMING  mEstablishedEdidTiming[] = {
  //
  // Established Timing I
  //
  { 800,  600,  60 },
  { 800,  600,  56 },
  { 640,  480,  75 },
  { 640,  480,  72 },
  { 640,  480,  67 },
  { 640,  480,  60 },
  { 720,  400,  88 },
  { 720,  400,  70 },
  //
  // Established Timing II
  //
  { 1280, 1024, 75 },
  { 1024, 768,  75 },
  { 1024, 768,  70 },
  { 1024, 768,  60 },
  { 1024, 768,  87 },
  { 832,  624,  75 },
  { 800,  600,  75 },
  { 800,  600,  72 },
  //
  // Established Timing III
  //
  { 1152, 870,  75 }
};

/**
  Supported.

  @param  This                   Pointer to driver binding protocol
  @param  Controller             Controller handle to connect
  @param  RemainingDevicePath    A pointer to the remaining portion of a device
                                 path

  @retval EFI_STATUS             EFI_SUCCESS:This controller can be managed by this
                                 driver, Otherwise, this controller cannot be
                                 managed by this driver

**/
EFI_STATUS
EFIAPI
BiosVideoDriverBindingSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   Controller,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  )
{
  EFI_STATUS                Status;
  EFI_LEGACY_BIOS_PROTOCOL  *LegacyBios;
  EFI_PCI_IO_PROTOCOL       *PciIo;
  PCI_TYPE00                Pci;
  EFI_DEV_PATH              *Node;

  //
  // See if the Legacy BIOS Protocol is available
  //
  Status = gBS->LocateProtocol (&gEfiLegacyBiosProtocolGuid, NULL, (VOID **)&LegacyBios);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Open the IO Abstraction(s) needed to perform the supported test
  //
  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiPciIoProtocolGuid,
                  (VOID **)&PciIo,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (EFI_ERROR (Status) && (Status != EFI_ALREADY_STARTED)) {
    return Status;
  }

  if (Status == EFI_ALREADY_STARTED) {
    //
    // If VgaMiniPort protocol is installed, EFI_ALREADY_STARTED indicates failure,
    // because VgaMiniPort protocol is installed on controller handle directly.
    //
    Status = gBS->OpenProtocol (
                    Controller,
                    &gEfiVgaMiniPortProtocolGuid,
                    NULL,
                    NULL,
                    NULL,
                    EFI_OPEN_PROTOCOL_TEST_PROTOCOL
                    );
    if (!EFI_ERROR (Status)) {
      return EFI_ALREADY_STARTED;
    }
  }

  //
  // See if this is a PCI Graphics Controller by looking at the Command register and
  // Class Code Register
  //
  Status = PciIo->Pci.Read (
                        PciIo,
                        EfiPciIoWidthUint32,
                        0,
                        sizeof (Pci) / sizeof (UINT32),
                        &Pci
                        );
  if (EFI_ERROR (Status)) {
    Status = EFI_UNSUPPORTED;
    goto Done;
  }

  Status = EFI_UNSUPPORTED;
  if ((Pci.Hdr.ClassCode[2] == 0x03) || ((Pci.Hdr.ClassCode[2] == 0x00) && (Pci.Hdr.ClassCode[1] == 0x01))) {
    Status = EFI_SUCCESS;
    //
    // If this is a graphics controller,
    // go further check RemainingDevicePath validation
    //
    if (RemainingDevicePath != NULL) {
      Node = (EFI_DEV_PATH *)RemainingDevicePath;
      //
      // Check if RemainingDevicePath is the End of Device Path Node,
      // if yes, return EFI_SUCCESS
      //
      if (!IsDevicePathEnd (Node)) {
        //
        // If RemainingDevicePath isn't the End of Device Path Node,
        // check its validation
        //
        if ((Node->DevPath.Type != ACPI_DEVICE_PATH) ||
            (Node->DevPath.SubType != ACPI_ADR_DP) ||
            (DevicePathNodeLength (&Node->DevPath) < sizeof (ACPI_ADR_DEVICE_PATH)))
        {
          Status = EFI_UNSUPPORTED;
        }
      }
    }
  }

Done:
  gBS->CloseProtocol (
         Controller,
         &gEfiPciIoProtocolGuid,
         This->DriverBindingHandle,
         Controller
         );

  return Status;
}

/**
  Install Graphics Output Protocol onto VGA device handles.

  @param  This                   Pointer to driver binding protocol
  @param  Controller             Controller handle to connect
  @param  RemainingDevicePath    A pointer to the remaining portion of a device
                                 path

  @return EFI_STATUS

**/
EFI_STATUS
EFIAPI
BiosVideoDriverBindingStart (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   Controller,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  )
{
  EFI_STATUS                Status;
  EFI_DEVICE_PATH_PROTOCOL  *ParentDevicePath;
  EFI_PCI_IO_PROTOCOL       *PciIo;
  EFI_LEGACY_BIOS_PROTOCOL  *LegacyBios;
  UINTN                     Flags;
  UINT64                    Supports;

  //
  // Initialize local variables
  //
  PciIo            = NULL;
  ParentDevicePath = NULL;

  //
  //
  // See if the Legacy BIOS Protocol is available
  //
  Status = gBS->LocateProtocol (&gEfiLegacyBiosProtocolGuid, NULL, (VOID **)&LegacyBios);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Prepare for status code
  //
  Status = gBS->HandleProtocol (
                  Controller,
                  &gEfiDevicePathProtocolGuid,
                  (VOID **)&ParentDevicePath
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Open the IO Abstraction(s) needed
  //
  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiPciIoProtocolGuid,
                  (VOID **)&PciIo,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (EFI_ERROR (Status) && (Status != EFI_ALREADY_STARTED)) {
    return Status;
  }

  //
  // Save original PCI attributes
  //
  if (!mPciAttributesSaved) {
    Status = PciIo->Attributes (
                      PciIo,
                      EfiPciIoAttributeOperationGet,
                      0,
                      &mOriginalPciAttributes
                      );

    if (EFI_ERROR (Status)) {
      goto Done;
    }

    mPciAttributesSaved = TRUE;
  }

  //
  // Get supported PCI attributes
  //
  Status = PciIo->Attributes (
                    PciIo,
                    EfiPciIoAttributeOperationSupported,
                    0,
                    &Supports
                    );
  if (EFI_ERROR (Status)) {
    goto Done;
  }

  Supports &= (UINT64)(EFI_PCI_IO_ATTRIBUTE_VGA_IO | EFI_PCI_IO_ATTRIBUTE_VGA_IO_16);
  if ((Supports == 0) || (Supports == (EFI_PCI_IO_ATTRIBUTE_VGA_IO | EFI_PCI_IO_ATTRIBUTE_VGA_IO_16))) {
    Status = EFI_UNSUPPORTED;
    goto Done;
  }

  REPORT_STATUS_CODE_WITH_DEVICE_PATH (
    EFI_PROGRESS_CODE,
    EFI_PERIPHERAL_LOCAL_CONSOLE | EFI_P_PC_ENABLE,
    ParentDevicePath
    );
  //
  // Enable the device and make sure VGA cycles are being forwarded to this VGA device
  //
  Status = PciIo->Attributes (
                    PciIo,
                    EfiPciIoAttributeOperationEnable,
                    EFI_PCI_DEVICE_ENABLE | EFI_PCI_IO_ATTRIBUTE_VGA_MEMORY | Supports,
                    NULL
                    );
  if (EFI_ERROR (Status)) {
    REPORT_STATUS_CODE_WITH_DEVICE_PATH (
      EFI_ERROR_CODE | EFI_ERROR_MINOR,
      EFI_PERIPHERAL_LOCAL_CONSOLE | EFI_P_EC_RESOURCE_CONFLICT,
      ParentDevicePath
      );
    goto Done;
  }

  //
  // Check to see if there is a legacy option ROM image associated with this PCI device
  //
  Status = LegacyBios->CheckPciRom (
                         LegacyBios,
                         Controller,
                         NULL,
                         NULL,
                         &Flags
                         );
  if (EFI_ERROR (Status)) {
    goto Done;
  }

  //
  // Post the legacy option ROM if it is available.
  //
  REPORT_STATUS_CODE_WITH_DEVICE_PATH (
    EFI_PROGRESS_CODE,
    EFI_P_PC_RESET,
    ParentDevicePath
    );
  Status = LegacyBios->InstallPciRom (
                         LegacyBios,
                         Controller,
                         NULL,
                         &Flags,
                         NULL,
                         NULL,
                         NULL,
                         NULL
                         );
  if (EFI_ERROR (Status)) {
    REPORT_STATUS_CODE_WITH_DEVICE_PATH (
      EFI_ERROR_CODE | EFI_ERROR_MINOR,
      EFI_PERIPHERAL_LOCAL_CONSOLE | EFI_P_EC_CONTROLLER_ERROR,
      ParentDevicePath
      );
    goto Done;
  }

  if (RemainingDevicePath != NULL) {
    if (IsDevicePathEnd (RemainingDevicePath) &&
        (FeaturePcdGet (PcdBiosVideoCheckVbeEnable) || FeaturePcdGet (PcdBiosVideoCheckVgaEnable)))
    {
      //
      // If RemainingDevicePath is the End of Device Path Node,
      // don't create any child device and return EFI_SUCCESS
      Status = EFI_SUCCESS;
      goto Done;
    }
  }

  //
  // Create child handle and install GraphicsOutputProtocol on it
  //
  Status = BiosVideoChildHandleInstall (
             This,
             Controller,
             PciIo,
             LegacyBios,
             ParentDevicePath,
             RemainingDevicePath
             );

Done:
  if ((EFI_ERROR (Status)) && (Status != EFI_ALREADY_STARTED)) {
    REPORT_STATUS_CODE_WITH_DEVICE_PATH (
      EFI_PROGRESS_CODE,
      EFI_PERIPHERAL_LOCAL_CONSOLE | EFI_P_PC_DISABLE,
      ParentDevicePath
      );

    REPORT_STATUS_CODE_WITH_DEVICE_PATH (
      EFI_PROGRESS_CODE,
      EFI_PERIPHERAL_LOCAL_CONSOLE | EFI_P_EC_NOT_DETECTED,
      ParentDevicePath
      );
    if (!HasChildHandle (Controller)) {
      if (mPciAttributesSaved) {
        //
        // Restore original PCI attributes
        //
        PciIo->Attributes (
                 PciIo,
                 EfiPciIoAttributeOperationSet,
                 mOriginalPciAttributes,
                 NULL
                 );
      }
    }

    //
    // Release PCI I/O Protocols on the controller handle.
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
  Stop.

  @param  This                   Pointer to driver binding protocol
  @param  Controller             Controller handle to connect
  @param  NumberOfChildren       Number of children handle created by this driver
  @param  ChildHandleBuffer      Buffer containing child handle created

  @retval EFI_SUCCESS            Driver disconnected successfully from controller
  @retval EFI_UNSUPPORTED        Cannot find BIOS_VIDEO_DEV structure

**/
EFI_STATUS
EFIAPI
BiosVideoDriverBindingStop (
  IN  EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN  EFI_HANDLE                   Controller,
  IN  UINTN                        NumberOfChildren,
  IN  EFI_HANDLE                   *ChildHandleBuffer
  )
{
  EFI_STATUS           Status;
  BOOLEAN              AllChildrenStopped;
  UINTN                Index;
  EFI_PCI_IO_PROTOCOL  *PciIo;

  AllChildrenStopped = TRUE;

  if (NumberOfChildren == 0) {
    //
    // Close PCI I/O protocol on the controller handle
    //
    gBS->CloseProtocol (
           Controller,
           &gEfiPciIoProtocolGuid,
           This->DriverBindingHandle,
           Controller
           );

    return EFI_SUCCESS;
  }

  for (Index = 0; Index < NumberOfChildren; Index++) {
    Status = BiosVideoChildHandleUninstall (This, Controller, ChildHandleBuffer[Index]);

    if (EFI_ERROR (Status)) {
      AllChildrenStopped = FALSE;
    }
  }

  if (!AllChildrenStopped) {
    return EFI_DEVICE_ERROR;
  }

  if (!HasChildHandle (Controller)) {
    if (mPciAttributesSaved) {
      Status = gBS->HandleProtocol (
                      Controller,
                      &gEfiPciIoProtocolGuid,
                      (VOID **)&PciIo
                      );
      ASSERT_EFI_ERROR (Status);

      //
      // Restore original PCI attributes
      //
      Status = PciIo->Attributes (
                        PciIo,
                        EfiPciIoAttributeOperationSet,
                        mOriginalPciAttributes,
                        NULL
                        );
      ASSERT_EFI_ERROR (Status);
    }
  }

  return EFI_SUCCESS;
}

/**
  Install child handles if the Handle supports MBR format.

  @param  This                   Calling context.
  @param  ParentHandle           Parent Handle
  @param  ParentPciIo            Parent PciIo interface
  @param  ParentLegacyBios       Parent LegacyBios interface
  @param  ParentDevicePath       Parent Device Path
  @param  RemainingDevicePath    Remaining Device Path

  @retval EFI_SUCCESS            If a child handle was added
  @retval other                  A child handle was not added

**/
EFI_STATUS
BiosVideoChildHandleInstall (
  IN  EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN  EFI_HANDLE                   ParentHandle,
  IN  EFI_PCI_IO_PROTOCOL          *ParentPciIo,
  IN  EFI_LEGACY_BIOS_PROTOCOL     *ParentLegacyBios,
  IN  EFI_DEVICE_PATH_PROTOCOL     *ParentDevicePath,
  IN  EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  )
{
  EFI_STATUS            Status;
  BIOS_VIDEO_DEV        *BiosVideoPrivate;
  PCI_TYPE00            Pci;
  ACPI_ADR_DEVICE_PATH  AcpiDeviceNode;
  BOOLEAN               ProtocolInstalled;

  //
  // Allocate the private device structure for video device
  //
  BiosVideoPrivate = (BIOS_VIDEO_DEV *)AllocateZeroPool (
                                         sizeof (BIOS_VIDEO_DEV)
                                         );
  if (NULL == BiosVideoPrivate) {
    Status = EFI_OUT_OF_RESOURCES;
    goto Done;
  }

  //
  // See if this is a VGA compatible controller or not
  //
  Status = ParentPciIo->Pci.Read (
                              ParentPciIo,
                              EfiPciIoWidthUint32,
                              0,
                              sizeof (Pci) / sizeof (UINT32),
                              &Pci
                              );
  if (EFI_ERROR (Status)) {
    REPORT_STATUS_CODE_WITH_DEVICE_PATH (
      EFI_ERROR_CODE | EFI_ERROR_MINOR,
      EFI_PERIPHERAL_LOCAL_CONSOLE | EFI_P_EC_CONTROLLER_ERROR,
      ParentDevicePath
      );
    goto Done;
  }

  BiosVideoPrivate->VgaCompatible = FALSE;
  if ((Pci.Hdr.ClassCode[2] == 0x00) && (Pci.Hdr.ClassCode[1] == 0x01)) {
    BiosVideoPrivate->VgaCompatible = TRUE;
  }

  if ((Pci.Hdr.ClassCode[2] == 0x03) && (Pci.Hdr.ClassCode[1] == 0x00) && (Pci.Hdr.ClassCode[0] == 0x00)) {
    BiosVideoPrivate->VgaCompatible = TRUE;
  }

  if (PcdGetBool (PcdBiosVideoSetTextVgaModeEnable)) {
    //
    // Create EXIT_BOOT_SERIVES Event
    //
    Status = gBS->CreateEventEx (
                    EVT_NOTIFY_SIGNAL,
                    TPL_NOTIFY,
                    BiosVideoNotifyExitBootServices,
                    BiosVideoPrivate,
                    &gEfiEventExitBootServicesGuid,
                    &BiosVideoPrivate->ExitBootServicesEvent
                    );
    if (EFI_ERROR (Status)) {
      goto Done;
    }
  }

  //
  // Initialize the child private structure
  //
  BiosVideoPrivate->Signature = BIOS_VIDEO_DEV_SIGNATURE;

  //
  // Fill in Graphics Output specific mode structures
  //
  BiosVideoPrivate->HardwareNeedsStarting = TRUE;
  BiosVideoPrivate->ModeData              = NULL;
  BiosVideoPrivate->LineBuffer            = NULL;
  BiosVideoPrivate->VgaFrameBuffer        = NULL;
  BiosVideoPrivate->VbeFrameBuffer        = NULL;

  //
  // Fill in the Graphics Output Protocol
  //
  BiosVideoPrivate->GraphicsOutput.QueryMode = BiosVideoGraphicsOutputQueryMode;
  BiosVideoPrivate->GraphicsOutput.SetMode   = BiosVideoGraphicsOutputSetMode;

  //
  // Allocate buffer for Graphics Output Protocol mode information
  //
  BiosVideoPrivate->GraphicsOutput.Mode = (EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE *)AllocatePool (
                                                                                 sizeof (EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE)
                                                                                 );
  if (NULL == BiosVideoPrivate->GraphicsOutput.Mode) {
    Status = EFI_OUT_OF_RESOURCES;
    goto Done;
  }

  BiosVideoPrivate->GraphicsOutput.Mode->Info = (EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *)AllocatePool (
                                                                                          sizeof (EFI_GRAPHICS_OUTPUT_MODE_INFORMATION)
                                                                                          );
  if (NULL ==  BiosVideoPrivate->GraphicsOutput.Mode->Info) {
    Status = EFI_OUT_OF_RESOURCES;
    goto Done;
  }

  //
  // Assume that Graphics Output Protocol will be produced until proven otherwise
  //
  BiosVideoPrivate->ProduceGraphicsOutput = TRUE;

  //
  // Set Gop Device Path, here RemainingDevicePath will not be one End of Device Path Node.
  //
  if ((RemainingDevicePath == NULL) || (!IsDevicePathEnd (RemainingDevicePath))) {
    if (RemainingDevicePath == NULL) {
      ZeroMem (&AcpiDeviceNode, sizeof (ACPI_ADR_DEVICE_PATH));
      AcpiDeviceNode.Header.Type    = ACPI_DEVICE_PATH;
      AcpiDeviceNode.Header.SubType = ACPI_ADR_DP;
      AcpiDeviceNode.ADR            = ACPI_DISPLAY_ADR (1, 0, 0, 1, 0, ACPI_ADR_DISPLAY_TYPE_VGA, 0, 0);
      SetDevicePathNodeLength (&AcpiDeviceNode.Header, sizeof (ACPI_ADR_DEVICE_PATH));

      BiosVideoPrivate->GopDevicePath = AppendDevicePathNode (
                                          ParentDevicePath,
                                          (EFI_DEVICE_PATH_PROTOCOL *)&AcpiDeviceNode
                                          );
    } else {
      BiosVideoPrivate->GopDevicePath = AppendDevicePathNode (ParentDevicePath, RemainingDevicePath);
    }

    //
    // Creat child handle and device path protocol firstly
    //
    BiosVideoPrivate->Handle = NULL;
    Status                   = gBS->InstallMultipleProtocolInterfaces (
                                      &BiosVideoPrivate->Handle,
                                      &gEfiDevicePathProtocolGuid,
                                      BiosVideoPrivate->GopDevicePath,
                                      NULL
                                      );
    if (EFI_ERROR (Status)) {
      goto Done;
    }
  }

  //
  // Fill in the VGA Mini Port Protocol fields
  //
  BiosVideoPrivate->VgaMiniPort.SetMode                   = BiosVideoVgaMiniPortSetMode;
  BiosVideoPrivate->VgaMiniPort.VgaMemoryOffset           = 0xb8000;
  BiosVideoPrivate->VgaMiniPort.CrtcAddressRegisterOffset = 0x3d4;
  BiosVideoPrivate->VgaMiniPort.CrtcDataRegisterOffset    = 0x3d5;
  BiosVideoPrivate->VgaMiniPort.VgaMemoryBar              = EFI_PCI_IO_PASS_THROUGH_BAR;
  BiosVideoPrivate->VgaMiniPort.CrtcAddressRegisterBar    = EFI_PCI_IO_PASS_THROUGH_BAR;
  BiosVideoPrivate->VgaMiniPort.CrtcDataRegisterBar       = EFI_PCI_IO_PASS_THROUGH_BAR;

  //
  // Child handle need to consume the Legacy Bios protocol
  //
  BiosVideoPrivate->LegacyBios = ParentLegacyBios;

  //
  // When check for VBE, PCI I/O protocol is needed, so use parent's protocol interface temporally
  //
  BiosVideoPrivate->PciIo = ParentPciIo;

  //
  // Check for VESA BIOS Extensions for modes that are compatible with Graphics Output
  //
  if (FeaturePcdGet (PcdBiosVideoCheckVbeEnable)) {
    Status = BiosVideoCheckForVbe (BiosVideoPrivate);
    DEBUG ((DEBUG_INFO, "BiosVideoCheckForVbe - %r\n", Status));
  } else {
    Status = EFI_UNSUPPORTED;
  }

  if (EFI_ERROR (Status)) {
    //
    // The VESA BIOS Extensions are not compatible with Graphics Output, so check for support
    // for the standard 640x480 16 color VGA mode
    //
    DEBUG ((DEBUG_INFO, "VgaCompatible - %x\n", BiosVideoPrivate->VgaCompatible));
    if (BiosVideoPrivate->VgaCompatible) {
      if (FeaturePcdGet (PcdBiosVideoCheckVgaEnable)) {
        Status = BiosVideoCheckForVga (BiosVideoPrivate);
        DEBUG ((DEBUG_INFO, "BiosVideoCheckForVga - %r\n", Status));
      } else {
        Status = EFI_UNSUPPORTED;
      }
    }

    if (EFI_ERROR (Status)) {
      //
      // Free GOP mode structure if it is not freed before
      // VgaMiniPort does not need this structure any more
      //
      if (BiosVideoPrivate->GraphicsOutput.Mode != NULL) {
        if (BiosVideoPrivate->GraphicsOutput.Mode->Info != NULL) {
          FreePool (BiosVideoPrivate->GraphicsOutput.Mode->Info);
          BiosVideoPrivate->GraphicsOutput.Mode->Info = NULL;
        }

        FreePool (BiosVideoPrivate->GraphicsOutput.Mode);
        BiosVideoPrivate->GraphicsOutput.Mode = NULL;
      }

      //
      // Neither VBE nor the standard 640x480 16 color VGA mode are supported, so do
      // not produce the Graphics Output protocol.  Instead, produce the VGA MiniPort Protocol.
      //
      BiosVideoPrivate->ProduceGraphicsOutput = FALSE;

      //
      // INT services are available, so on the 80x25 and 80x50 text mode are supported
      //
      BiosVideoPrivate->VgaMiniPort.MaxMode = 2;
    }
  }

  ProtocolInstalled = FALSE;

  if (BiosVideoPrivate->ProduceGraphicsOutput) {
    //
    // Creat child handle and install Graphics Output Protocol,EDID Discovered/Active Protocol
    //
    Status = gBS->InstallMultipleProtocolInterfaces (
                    &BiosVideoPrivate->Handle,
                    &gEfiGraphicsOutputProtocolGuid,
                    &BiosVideoPrivate->GraphicsOutput,
                    &gEfiEdidDiscoveredProtocolGuid,
                    &BiosVideoPrivate->EdidDiscovered,
                    &gEfiEdidActiveProtocolGuid,
                    &BiosVideoPrivate->EdidActive,
                    NULL
                    );

    if (!EFI_ERROR (Status)) {
      //
      // Open the Parent Handle for the child
      //
      Status = gBS->OpenProtocol (
                      ParentHandle,
                      &gEfiPciIoProtocolGuid,
                      (VOID **)&BiosVideoPrivate->PciIo,
                      This->DriverBindingHandle,
                      BiosVideoPrivate->Handle,
                      EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER
                      );
      if (EFI_ERROR (Status)) {
        goto Done;
      }

      ProtocolInstalled = TRUE;
    }
  }

  if (!ProtocolInstalled) {
    //
    // Install VGA Mini Port Protocol
    //
    Status = gBS->InstallMultipleProtocolInterfaces (
                    &ParentHandle,
                    &gEfiVgaMiniPortProtocolGuid,
                    &BiosVideoPrivate->VgaMiniPort,
                    NULL
                    );
  }

Done:
  if (EFI_ERROR (Status)) {
    if ((BiosVideoPrivate != NULL) && (BiosVideoPrivate->ExitBootServicesEvent != NULL)) {
      gBS->CloseEvent (BiosVideoPrivate->ExitBootServicesEvent);
    }

    //
    // Free private data structure
    //
    BiosVideoDeviceReleaseResource (BiosVideoPrivate);
  }

  return Status;
}

/**
  Deregister an video child handle and free resources.

  @param  This                   Protocol instance pointer.
  @param  Controller             Video controller handle
  @param  Handle                 Video child handle

  @return EFI_STATUS

**/
EFI_STATUS
BiosVideoChildHandleUninstall (
  EFI_DRIVER_BINDING_PROTOCOL  *This,
  EFI_HANDLE                   Controller,
  EFI_HANDLE                   Handle
  )
{
  EFI_STATUS                    Status;
  EFI_IA32_REGISTER_SET         Regs;
  EFI_GRAPHICS_OUTPUT_PROTOCOL  *GraphicsOutput;
  EFI_VGA_MINI_PORT_PROTOCOL    *VgaMiniPort;
  BIOS_VIDEO_DEV                *BiosVideoPrivate;
  EFI_PCI_IO_PROTOCOL           *PciIo;

  BiosVideoPrivate = NULL;
  GraphicsOutput   = NULL;
  PciIo            = NULL;
  Status           = EFI_UNSUPPORTED;

  Status = gBS->OpenProtocol (
                  Handle,
                  &gEfiGraphicsOutputProtocolGuid,
                  (VOID **)&GraphicsOutput,
                  This->DriverBindingHandle,
                  Handle,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
  if (!EFI_ERROR (Status)) {
    BiosVideoPrivate = BIOS_VIDEO_DEV_FROM_GRAPHICS_OUTPUT_THIS (GraphicsOutput);
  }

  if (EFI_ERROR (Status)) {
    Status = gBS->OpenProtocol (
                    Handle,
                    &gEfiVgaMiniPortProtocolGuid,
                    (VOID **)&VgaMiniPort,
                    This->DriverBindingHandle,
                    Handle,
                    EFI_OPEN_PROTOCOL_GET_PROTOCOL
                    );
    if (!EFI_ERROR (Status)) {
      BiosVideoPrivate = BIOS_VIDEO_DEV_FROM_VGA_MINI_PORT_THIS (VgaMiniPort);
    }
  }

  if (BiosVideoPrivate == NULL) {
    return EFI_UNSUPPORTED;
  }

  //
  // Set the 80x25 Text VGA Mode
  //
  Regs.H.AH = 0x00;
  Regs.H.AL = 0x03;
  BiosVideoPrivate->LegacyBios->Int86 (BiosVideoPrivate->LegacyBios, 0x10, &Regs);

  Regs.H.AH = 0x11;
  Regs.H.AL = 0x14;
  Regs.H.BL = 0;
  BiosVideoPrivate->LegacyBios->Int86 (BiosVideoPrivate->LegacyBios, 0x10, &Regs);

  //
  // Close PCI I/O protocol that opened by child handle
  //
  Status = gBS->CloseProtocol (
                  Controller,
                  &gEfiPciIoProtocolGuid,
                  This->DriverBindingHandle,
                  Handle
                  );

  //
  // Uninstall protocols on child handle
  //
  if (BiosVideoPrivate->ProduceGraphicsOutput) {
    Status = gBS->UninstallMultipleProtocolInterfaces (
                    BiosVideoPrivate->Handle,
                    &gEfiDevicePathProtocolGuid,
                    BiosVideoPrivate->GopDevicePath,
                    &gEfiGraphicsOutputProtocolGuid,
                    &BiosVideoPrivate->GraphicsOutput,
                    &gEfiEdidDiscoveredProtocolGuid,
                    &BiosVideoPrivate->EdidDiscovered,
                    &gEfiEdidActiveProtocolGuid,
                    &BiosVideoPrivate->EdidActive,
                    NULL
                    );
  }

  if (!BiosVideoPrivate->ProduceGraphicsOutput) {
    Status = gBS->UninstallMultipleProtocolInterfaces (
                    Controller,
                    &gEfiVgaMiniPortProtocolGuid,
                    &BiosVideoPrivate->VgaMiniPort,
                    NULL
                    );
  }

  if (EFI_ERROR (Status)) {
    gBS->OpenProtocol (
           Controller,
           &gEfiPciIoProtocolGuid,
           (VOID **)&PciIo,
           This->DriverBindingHandle,
           Handle,
           EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER
           );
    return Status;
  }

  if (PcdGetBool (PcdBiosVideoSetTextVgaModeEnable)) {
    //
    // Close EXIT_BOOT_SERIVES Event
    //
    gBS->CloseEvent (BiosVideoPrivate->ExitBootServicesEvent);
  }

  //
  // Release all allocated resources
  //
  BiosVideoDeviceReleaseResource (BiosVideoPrivate);

  return EFI_SUCCESS;
}

/**
  Release resource for biso video instance.

  @param  BiosVideoPrivate       Video child device private data structure

**/
VOID
BiosVideoDeviceReleaseResource (
  BIOS_VIDEO_DEV  *BiosVideoPrivate
  )
{
  if (BiosVideoPrivate == NULL) {
    return;
  }

  //
  // Release all the resourses occupied by the BIOS_VIDEO_DEV
  //

  //
  // Free VGA Frame Buffer
  //
  if (BiosVideoPrivate->VgaFrameBuffer != NULL) {
    FreePool (BiosVideoPrivate->VgaFrameBuffer);
  }

  //
  // Free VBE Frame Buffer
  //
  if (BiosVideoPrivate->VbeFrameBuffer != NULL) {
    FreePool (BiosVideoPrivate->VbeFrameBuffer);
  }

  //
  // Free line buffer
  //
  if (BiosVideoPrivate->LineBuffer != NULL) {
    FreePool (BiosVideoPrivate->LineBuffer);
  }

  //
  // Free mode data
  //
  if (BiosVideoPrivate->ModeData != NULL) {
    FreePool (BiosVideoPrivate->ModeData);
  }

  //
  // Free memory allocated below 1MB
  //
  if (BiosVideoPrivate->PagesBelow1MB != 0) {
    gBS->FreePages (BiosVideoPrivate->PagesBelow1MB, BiosVideoPrivate->NumberOfPagesBelow1MB);
  }

  if (BiosVideoPrivate->VbeSaveRestorePages != 0) {
    gBS->FreePages (BiosVideoPrivate->VbeSaveRestoreBuffer, BiosVideoPrivate->VbeSaveRestorePages);
  }

  //
  // Free graphics output protocol occupied resource
  //
  if (BiosVideoPrivate->GraphicsOutput.Mode != NULL) {
    if (BiosVideoPrivate->GraphicsOutput.Mode->Info != NULL) {
      FreePool (BiosVideoPrivate->GraphicsOutput.Mode->Info);
      BiosVideoPrivate->GraphicsOutput.Mode->Info = NULL;
    }

    FreePool (BiosVideoPrivate->GraphicsOutput.Mode);
    BiosVideoPrivate->GraphicsOutput.Mode = NULL;
  }

  //
  // Free EDID discovered protocol occupied resource
  //
  if (BiosVideoPrivate->EdidDiscovered.Edid != NULL) {
    FreePool (BiosVideoPrivate->EdidDiscovered.Edid);
  }

  //
  // Free EDID active protocol occupied resource
  //
  if (BiosVideoPrivate->EdidActive.Edid != NULL) {
    FreePool (BiosVideoPrivate->EdidActive.Edid);
  }

  if (BiosVideoPrivate->GopDevicePath != NULL) {
    FreePool (BiosVideoPrivate->GopDevicePath);
  }

  FreePool (BiosVideoPrivate);

  return;
}

/**
  Generate a search key for a specified timing data.

  @param  EdidTiming             Pointer to EDID timing

  @return The 32 bit unique key for search.

**/
UINT32
CalculateEdidKey (
  VESA_BIOS_EXTENSIONS_EDID_TIMING  *EdidTiming
  )
{
  UINT32  Key;

  //
  // Be sure no conflicts for all standard timing defined by VESA.
  //
  Key = (EdidTiming->HorizontalResolution * 2) + EdidTiming->VerticalResolution;
  return Key;
}

/**
  Parse the Established Timing and Standard Timing in EDID data block.

  @param  EdidBuffer             Pointer to EDID data block
  @param  ValidEdidTiming        Valid EDID timing information

  @retval TRUE                   The EDID data is valid.
  @retval FALSE                  The EDID data is invalid.

**/
BOOLEAN
ParseEdidData (
  UINT8                                   *EdidBuffer,
  VESA_BIOS_EXTENSIONS_VALID_EDID_TIMING  *ValidEdidTiming
  )
{
  UINT8                                 CheckSum;
  UINT32                                Index;
  UINT32                                ValidNumber;
  UINT32                                TimingBits;
  UINT8                                 *BufferIndex;
  UINT16                                HorizontalResolution;
  UINT16                                VerticalResolution;
  UINT8                                 AspectRatio;
  UINT8                                 RefreshRate;
  VESA_BIOS_EXTENSIONS_EDID_TIMING      TempTiming;
  VESA_BIOS_EXTENSIONS_EDID_DATA_BLOCK  *EdidDataBlock;

  EdidDataBlock = (VESA_BIOS_EXTENSIONS_EDID_DATA_BLOCK *)EdidBuffer;

  //
  // Check the checksum of EDID data
  //
  CheckSum = 0;
  for (Index = 0; Index < VESA_BIOS_EXTENSIONS_EDID_BLOCK_SIZE; Index++) {
    CheckSum = (UINT8)(CheckSum + EdidBuffer[Index]);
  }

  if (CheckSum != 0) {
    return FALSE;
  }

  ValidNumber = 0;
  gBS->SetMem (ValidEdidTiming, sizeof (VESA_BIOS_EXTENSIONS_VALID_EDID_TIMING), 0);

  if ((EdidDataBlock->EstablishedTimings[0] != 0) ||
      (EdidDataBlock->EstablishedTimings[1] != 0) ||
      (EdidDataBlock->EstablishedTimings[2] != 0)
      )
  {
    //
    // Established timing data
    //
    TimingBits = EdidDataBlock->EstablishedTimings[0] |
                 (EdidDataBlock->EstablishedTimings[1] << 8) |
                 ((EdidDataBlock->EstablishedTimings[2] & 0x80) << 9);
    for (Index = 0; Index < VESA_BIOS_EXTENSIONS_EDID_ESTABLISHED_TIMING_MAX_NUMBER; Index++) {
      if ((TimingBits & 0x1) != 0) {
        DEBUG ((
          DEBUG_INFO,
          "Established Timing: %d x %d\n",
          mEstablishedEdidTiming[Index].HorizontalResolution,
          mEstablishedEdidTiming[Index].VerticalResolution
          ));
        ValidEdidTiming->Key[ValidNumber] = CalculateEdidKey (&mEstablishedEdidTiming[Index]);
        ValidNumber++;
      }

      TimingBits = TimingBits >> 1;
    }
  }

  //
  // Parse the standard timing data
  //
  BufferIndex = &EdidDataBlock->StandardTimingIdentification[0];
  for (Index = 0; Index < 8; Index++) {
    //
    // Check if this is a valid Standard Timing entry
    // VESA documents unused fields should be set to 01h
    //
    if ((BufferIndex[0] != 0x1) && (BufferIndex[1] != 0x1)) {
      //
      // A valid Standard Timing
      //
      HorizontalResolution = (UINT16)(BufferIndex[0] * 8 + 248);
      AspectRatio          = (UINT8)(BufferIndex[1] >> 6);
      switch (AspectRatio) {
        case 0:
          VerticalResolution = (UINT16)(HorizontalResolution / 16 * 10);
          break;
        case 1:
          VerticalResolution = (UINT16)(HorizontalResolution / 4 * 3);
          break;
        case 2:
          VerticalResolution = (UINT16)(HorizontalResolution / 5 * 4);
          break;
        case 3:
          VerticalResolution = (UINT16)(HorizontalResolution / 16 * 9);
          break;
        default:
          VerticalResolution = (UINT16)(HorizontalResolution / 4 * 3);
          break;
      }

      RefreshRate = (UINT8)((BufferIndex[1] & 0x1f) + 60);
      DEBUG ((DEBUG_INFO, "Standard Timing: %d x %d\n", HorizontalResolution, VerticalResolution));
      TempTiming.HorizontalResolution   = HorizontalResolution;
      TempTiming.VerticalResolution     = VerticalResolution;
      TempTiming.RefreshRate            = RefreshRate;
      ValidEdidTiming->Key[ValidNumber] = CalculateEdidKey (&TempTiming);
      ValidNumber++;
    }

    BufferIndex += 2;
  }

  //
  // Parse the Detailed Timing data
  //
  BufferIndex = &EdidDataBlock->DetailedTimingDescriptions[0];
  for (Index = 0; Index < 4; Index++, BufferIndex += VESA_BIOS_EXTENSIONS_DETAILED_TIMING_EACH_DESCRIPTOR_SIZE) {
    if ((BufferIndex[0] == 0x0) && (BufferIndex[1] == 0x0)) {
      //
      // Check if this is a valid Detailed Timing Descriptor
      // If first 2 bytes are zero, it is monitor descriptor other than detailed timing descriptor
      //
      continue;
    }

    //
    // Calculate Horizontal and Vertical resolution
    //
    TempTiming.HorizontalResolution = ((UINT16)(BufferIndex[4] & 0xF0) << 4) | (BufferIndex[2]);
    TempTiming.VerticalResolution   = ((UINT16)(BufferIndex[7] & 0xF0) << 4) | (BufferIndex[5]);
    DEBUG ((
      DEBUG_INFO,
      "Detailed Timing %d: %d x %d\n",
      Index,
      TempTiming.HorizontalResolution,
      TempTiming.VerticalResolution
      ));
    ValidEdidTiming->Key[ValidNumber] = CalculateEdidKey (&TempTiming);
    ValidNumber++;
  }

  ValidEdidTiming->ValidNumber = ValidNumber;
  return TRUE;
}

/**
  Search a specified Timing in all the valid EDID timings.

  @param  ValidEdidTiming        All valid EDID timing information.
  @param  EdidTiming             The Timing to search for.

  @retval TRUE                   Found.
  @retval FALSE                  Not found.

**/
BOOLEAN
SearchEdidTiming (
  VESA_BIOS_EXTENSIONS_VALID_EDID_TIMING  *ValidEdidTiming,
  VESA_BIOS_EXTENSIONS_EDID_TIMING        *EdidTiming
  )
{
  UINT32  Index;
  UINT32  Key;

  Key = CalculateEdidKey (EdidTiming);

  for (Index = 0; Index < ValidEdidTiming->ValidNumber; Index++) {
    if (Key == ValidEdidTiming->Key[Index]) {
      return TRUE;
    }
  }

  return FALSE;
}

/**
  Check if all video child handles have been uninstalled.

  @param  Controller             Video controller handle

  @return TRUE                   Child handles exist.
  @return FALSE                  All video child handles have been uninstalled.

**/
BOOLEAN
HasChildHandle (
  IN EFI_HANDLE  Controller
  )
{
  UINTN                                Index;
  EFI_OPEN_PROTOCOL_INFORMATION_ENTRY  *OpenInfoBuffer;
  UINTN                                EntryCount;
  BOOLEAN                              HasChild;

  EntryCount = 0;
  HasChild   = FALSE;
  gBS->OpenProtocolInformation (
         Controller,
         &gEfiPciIoProtocolGuid,
         &OpenInfoBuffer,
         &EntryCount
         );
  for (Index = 0; Index < EntryCount; Index++) {
    if ((OpenInfoBuffer[Index].Attributes & EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER) != 0) {
      HasChild = TRUE;
    }
  }

  return HasChild;
}

/**
  Check for VBE device.

  @param  BiosVideoPrivate       Pointer to BIOS_VIDEO_DEV structure

  @retval EFI_SUCCESS            VBE device found

**/
EFI_STATUS
BiosVideoCheckForVbe (
  IN OUT BIOS_VIDEO_DEV  *BiosVideoPrivate
  )
{
  EFI_STATUS                              Status;
  EFI_IA32_REGISTER_SET                   Regs;
  UINT16                                  *ModeNumberPtr;
  UINT16                                  VbeModeNumber;
  BOOLEAN                                 ModeFound;
  BOOLEAN                                 EdidFound;
  BIOS_VIDEO_MODE_DATA                    *ModeBuffer;
  BIOS_VIDEO_MODE_DATA                    *CurrentModeData;
  UINTN                                   PreferMode;
  UINTN                                   ModeNumber;
  VESA_BIOS_EXTENSIONS_EDID_TIMING        Timing;
  VESA_BIOS_EXTENSIONS_VALID_EDID_TIMING  ValidEdidTiming;
  EFI_EDID_OVERRIDE_PROTOCOL              *EdidOverride;
  UINT32                                  EdidAttributes;
  BOOLEAN                                 EdidOverrideFound;
  UINTN                                   EdidOverrideDataSize;
  UINT8                                   *EdidOverrideDataBlock;
  UINTN                                   EdidActiveDataSize;
  UINT8                                   *EdidActiveDataBlock;
  UINT32                                  HighestHorizontalResolution;
  UINT32                                  HighestVerticalResolution;
  UINTN                                   HighestResolutionMode;

  EdidFound                   = TRUE;
  EdidOverrideFound           = FALSE;
  EdidOverrideDataBlock       = NULL;
  EdidActiveDataSize          = 0;
  EdidActiveDataBlock         = NULL;
  HighestHorizontalResolution = 0;
  HighestVerticalResolution   = 0;
  HighestResolutionMode       = 0;

  //
  // Allocate buffer under 1MB for VBE data structures
  //
  BiosVideoPrivate->NumberOfPagesBelow1MB = EFI_SIZE_TO_PAGES (
                                              sizeof (VESA_BIOS_EXTENSIONS_INFORMATION_BLOCK) +
                                              sizeof (VESA_BIOS_EXTENSIONS_MODE_INFORMATION_BLOCK) +
                                              sizeof (VESA_BIOS_EXTENSIONS_EDID_DATA_BLOCK) +
                                              sizeof (VESA_BIOS_EXTENSIONS_CRTC_INFORMATION_BLOCK)
                                              );

  BiosVideoPrivate->PagesBelow1MB = 0x00100000 - 1;

  Status = gBS->AllocatePages (
                  AllocateMaxAddress,
                  EfiBootServicesData,
                  BiosVideoPrivate->NumberOfPagesBelow1MB,
                  &BiosVideoPrivate->PagesBelow1MB
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  ZeroMem (&ValidEdidTiming, sizeof (VESA_BIOS_EXTENSIONS_VALID_EDID_TIMING));

  //
  // Fill in the VBE related data structures
  //
  BiosVideoPrivate->VbeInformationBlock     = (VESA_BIOS_EXTENSIONS_INFORMATION_BLOCK *)(UINTN)(BiosVideoPrivate->PagesBelow1MB);
  BiosVideoPrivate->VbeModeInformationBlock = (VESA_BIOS_EXTENSIONS_MODE_INFORMATION_BLOCK *)(BiosVideoPrivate->VbeInformationBlock + 1);
  BiosVideoPrivate->VbeEdidDataBlock        = (VESA_BIOS_EXTENSIONS_EDID_DATA_BLOCK *)(BiosVideoPrivate->VbeModeInformationBlock + 1);
  BiosVideoPrivate->VbeCrtcInformationBlock = (VESA_BIOS_EXTENSIONS_CRTC_INFORMATION_BLOCK *)(BiosVideoPrivate->VbeEdidDataBlock + 1);
  BiosVideoPrivate->VbeSaveRestorePages     = 0;
  BiosVideoPrivate->VbeSaveRestoreBuffer    = 0;

  //
  // Test to see if the Video Adapter is compliant with VBE 3.0
  //
  gBS->SetMem (&Regs, sizeof (Regs), 0);
  Regs.X.AX = VESA_BIOS_EXTENSIONS_RETURN_CONTROLLER_INFORMATION;
  gBS->SetMem (BiosVideoPrivate->VbeInformationBlock, sizeof (VESA_BIOS_EXTENSIONS_INFORMATION_BLOCK), 0);
  BiosVideoPrivate->VbeInformationBlock->VESASignature = VESA_BIOS_EXTENSIONS_VBE2_SIGNATURE;
  Regs.X.ES                                            = EFI_SEGMENT ((UINTN)BiosVideoPrivate->VbeInformationBlock);
  Regs.X.DI                                            = EFI_OFFSET ((UINTN)BiosVideoPrivate->VbeInformationBlock);

  BiosVideoPrivate->LegacyBios->Int86 (BiosVideoPrivate->LegacyBios, 0x10, &Regs);

  Status = EFI_DEVICE_ERROR;

  //
  // See if the VESA call succeeded
  //
  if (Regs.X.AX != VESA_BIOS_EXTENSIONS_STATUS_SUCCESS) {
    return Status;
  }

  //
  // Check for 'VESA' signature
  //
  if (BiosVideoPrivate->VbeInformationBlock->VESASignature != VESA_BIOS_EXTENSIONS_VESA_SIGNATURE) {
    return Status;
  }

  //
  // Check to see if this is VBE 2.0 or higher
  //
  if (BiosVideoPrivate->VbeInformationBlock->VESAVersion < VESA_BIOS_EXTENSIONS_VERSION_2_0) {
    return Status;
  }

  EdidFound            = FALSE;
  EdidAttributes       = 0xff;
  EdidOverrideDataSize = 0;

  //
  // Find EDID Override protocol firstly, this protocol is installed by platform if needed.
  //
  Status = gBS->LocateProtocol (
                  &gEfiEdidOverrideProtocolGuid,
                  NULL,
                  (VOID **)&EdidOverride
                  );
  if (!EFI_ERROR (Status)) {
    //
    // Allocate double size of VESA_BIOS_EXTENSIONS_EDID_BLOCK_SIZE to avoid overflow
    //
    EdidOverrideDataBlock = AllocatePool (VESA_BIOS_EXTENSIONS_EDID_BLOCK_SIZE * 2);
    if (NULL == EdidOverrideDataBlock) {
      Status = EFI_OUT_OF_RESOURCES;
      goto Done;
    }

    Status = EdidOverride->GetEdid (
                             EdidOverride,
                             BiosVideoPrivate->Handle,
                             &EdidAttributes,
                             &EdidOverrideDataSize,
                             (UINT8 **)&EdidOverrideDataBlock
                             );
    if (!EFI_ERROR (Status)  &&
        (EdidAttributes == 0) &&
        (EdidOverrideDataSize != 0))
    {
      //
      // Succeeded to get EDID Override Data
      //
      EdidOverrideFound = TRUE;
    }
  }

  if (!EdidOverrideFound || (EdidAttributes == EFI_EDID_OVERRIDE_DONT_OVERRIDE)) {
    //
    // If EDID Override data doesn't exist or EFI_EDID_OVERRIDE_DONT_OVERRIDE returned,
    // read EDID information through INT10 call
    //

    gBS->SetMem (&Regs, sizeof (Regs), 0);
    Regs.X.AX = VESA_BIOS_EXTENSIONS_EDID;
    Regs.X.BX = 1;
    Regs.X.CX = 0;
    Regs.X.DX = 0;
    Regs.X.ES = EFI_SEGMENT ((UINTN)BiosVideoPrivate->VbeEdidDataBlock);
    Regs.X.DI = EFI_OFFSET ((UINTN)BiosVideoPrivate->VbeEdidDataBlock);

    BiosVideoPrivate->LegacyBios->Int86 (BiosVideoPrivate->LegacyBios, 0x10, &Regs);
    //
    // See if the VESA call succeeded
    //
    if (Regs.X.AX == VESA_BIOS_EXTENSIONS_STATUS_SUCCESS) {
      //
      // Set EDID Discovered Data
      //
      BiosVideoPrivate->EdidDiscovered.SizeOfEdid = VESA_BIOS_EXTENSIONS_EDID_BLOCK_SIZE;
      BiosVideoPrivate->EdidDiscovered.Edid       = (UINT8 *)AllocateCopyPool (
                                                               VESA_BIOS_EXTENSIONS_EDID_BLOCK_SIZE,
                                                               BiosVideoPrivate->VbeEdidDataBlock
                                                               );

      if (NULL == BiosVideoPrivate->EdidDiscovered.Edid) {
        Status = EFI_OUT_OF_RESOURCES;
        goto Done;
      }

      EdidFound = TRUE;
    }
  }

  if (EdidFound) {
    EdidActiveDataSize  = VESA_BIOS_EXTENSIONS_EDID_BLOCK_SIZE;
    EdidActiveDataBlock = BiosVideoPrivate->EdidDiscovered.Edid;
  } else if (EdidOverrideFound) {
    EdidActiveDataSize  = EdidOverrideDataSize;
    EdidActiveDataBlock = EdidOverrideDataBlock;
    EdidFound           = TRUE;
  }

  if (EdidFound) {
    //
    // Parse EDID data structure to retrieve modes supported by monitor
    //
    if (ParseEdidData ((UINT8 *)EdidActiveDataBlock, &ValidEdidTiming)) {
      //
      // Copy EDID Override Data to EDID Active Data
      //
      BiosVideoPrivate->EdidActive.SizeOfEdid = (UINT32)EdidActiveDataSize;
      BiosVideoPrivate->EdidActive.Edid       = (UINT8 *)AllocateCopyPool (
                                                           EdidActiveDataSize,
                                                           EdidActiveDataBlock
                                                           );
      if (NULL ==  BiosVideoPrivate->EdidActive.Edid) {
        Status = EFI_OUT_OF_RESOURCES;
        goto Done;
      }
    }
  } else {
    BiosVideoPrivate->EdidActive.SizeOfEdid = 0;
    BiosVideoPrivate->EdidActive.Edid       = NULL;
    EdidFound                               = FALSE;
  }

  //
  // Walk through the mode list to see if there is at least one mode the is compatible with the EDID mode
  //
  ModeNumberPtr = (UINT16 *)
                  (
                   (((UINTN)BiosVideoPrivate->VbeInformationBlock->VideoModePtr & 0xffff0000) >> 12) |
                   ((UINTN)BiosVideoPrivate->VbeInformationBlock->VideoModePtr & 0x0000ffff)
                  );

  PreferMode = 0;
  ModeNumber = 0;

  //
  // ModeNumberPtr may be not 16-byte aligned, so ReadUnaligned16 is used to access the buffer pointed by ModeNumberPtr.
  //
  for (VbeModeNumber = ReadUnaligned16 (ModeNumberPtr);
       VbeModeNumber != VESA_BIOS_EXTENSIONS_END_OF_MODE_LIST;
       VbeModeNumber = ReadUnaligned16 (++ModeNumberPtr))
  {
    //
    // Make sure this is a mode number defined by the VESA VBE specification.  If it isn'tm then skip this mode number.
    //
    if ((VbeModeNumber & VESA_BIOS_EXTENSIONS_MODE_NUMBER_VESA) == 0) {
      continue;
    }

    //
    // Get the information about the mode
    //
    gBS->SetMem (&Regs, sizeof (Regs), 0);
    Regs.X.AX = VESA_BIOS_EXTENSIONS_RETURN_MODE_INFORMATION;
    Regs.X.CX = VbeModeNumber;
    gBS->SetMem (BiosVideoPrivate->VbeModeInformationBlock, sizeof (VESA_BIOS_EXTENSIONS_MODE_INFORMATION_BLOCK), 0);
    Regs.X.ES = EFI_SEGMENT ((UINTN)BiosVideoPrivate->VbeModeInformationBlock);
    Regs.X.DI = EFI_OFFSET ((UINTN)BiosVideoPrivate->VbeModeInformationBlock);

    BiosVideoPrivate->LegacyBios->Int86 (BiosVideoPrivate->LegacyBios, 0x10, &Regs);

    //
    // See if the call succeeded.  If it didn't, then try the next mode.
    //
    if (Regs.X.AX != VESA_BIOS_EXTENSIONS_STATUS_SUCCESS) {
      continue;
    }

    //
    // See if the mode supports color.  If it doesn't then try the next mode.
    //
    if ((BiosVideoPrivate->VbeModeInformationBlock->ModeAttributes & VESA_BIOS_EXTENSIONS_MODE_ATTRIBUTE_COLOR) == 0) {
      continue;
    }

    //
    // See if the mode supports graphics.  If it doesn't then try the next mode.
    //
    if ((BiosVideoPrivate->VbeModeInformationBlock->ModeAttributes & VESA_BIOS_EXTENSIONS_MODE_ATTRIBUTE_GRAPHICS) == 0) {
      continue;
    }

    //
    // See if the mode supports a linear frame buffer.  If it doesn't then try the next mode.
    //
    if ((BiosVideoPrivate->VbeModeInformationBlock->ModeAttributes & VESA_BIOS_EXTENSIONS_MODE_ATTRIBUTE_LINEAR_FRAME_BUFFER) == 0) {
      continue;
    }

    //
    // See if the mode supports 32 bit color.  If it doesn't then try the next mode.
    // 32 bit mode can be implemented by 24 Bits Per Pixels. Also make sure the
    // number of bits per pixel is a multiple of 8 or more than 32 bits per pixel
    //
    if (BiosVideoPrivate->VbeModeInformationBlock->BitsPerPixel < 24) {
      continue;
    }

    if (BiosVideoPrivate->VbeModeInformationBlock->BitsPerPixel > 32) {
      continue;
    }

    if ((BiosVideoPrivate->VbeModeInformationBlock->BitsPerPixel % 8) != 0) {
      continue;
    }

    //
    // See if the physical base pointer for the linear mode is valid.  If it isn't then try the next mode.
    //
    if (BiosVideoPrivate->VbeModeInformationBlock->PhysBasePtr == 0) {
      continue;
    }

    DEBUG ((
      DEBUG_INFO,
      "Video Controller Mode 0x%x: %d x %d\n",
      VbeModeNumber,
      BiosVideoPrivate->VbeModeInformationBlock->XResolution,
      BiosVideoPrivate->VbeModeInformationBlock->YResolution
      ));

    if (EdidFound && (ValidEdidTiming.ValidNumber > 0)) {
      //
      // EDID exist, check whether this mode match with any mode in EDID
      //
      Timing.HorizontalResolution = BiosVideoPrivate->VbeModeInformationBlock->XResolution;
      Timing.VerticalResolution   = BiosVideoPrivate->VbeModeInformationBlock->YResolution;
      if (!SearchEdidTiming (&ValidEdidTiming, &Timing)) {
        //
        // When EDID comes from INT10 call, EDID does not include 800x600, 640x480 and 1024x768,
        // but INT10 can support these modes, we add them into GOP mode.
        //
        if ((BiosVideoPrivate->EdidDiscovered.SizeOfEdid != 0) &&
            !(((Timing.HorizontalResolution) == 1024) && (Timing.VerticalResolution == 768)) &&
            !(((Timing.HorizontalResolution) == 800) && (Timing.VerticalResolution == 600)) &&
            !(((Timing.HorizontalResolution) == 640) && (Timing.VerticalResolution == 480)))
        {
          continue;
        }
      }
    }

    //
    // Select a reasonable mode to be set for current display mode
    //
    ModeFound = FALSE;

    if ((BiosVideoPrivate->VbeModeInformationBlock->XResolution == 1024) &&
        (BiosVideoPrivate->VbeModeInformationBlock->YResolution == 768)
        )
    {
      ModeFound = TRUE;
    }

    if ((BiosVideoPrivate->VbeModeInformationBlock->XResolution == 800) &&
        (BiosVideoPrivate->VbeModeInformationBlock->YResolution == 600)
        )
    {
      ModeFound  = TRUE;
      PreferMode = ModeNumber;
    }

    if ((BiosVideoPrivate->VbeModeInformationBlock->XResolution == 640) &&
        (BiosVideoPrivate->VbeModeInformationBlock->YResolution == 480)
        )
    {
      ModeFound = TRUE;
    }

    if ((!EdidFound) && (!ModeFound)) {
      //
      // When no EDID exist, only select three possible resolutions, i.e. 1024x768, 800x600, 640x480
      //
      continue;
    }

    //
    // Record the highest resolution mode to set later
    //
    if ((BiosVideoPrivate->VbeModeInformationBlock->XResolution > HighestHorizontalResolution) ||
        ((BiosVideoPrivate->VbeModeInformationBlock->XResolution == HighestHorizontalResolution) &&
         (BiosVideoPrivate->VbeModeInformationBlock->YResolution > HighestVerticalResolution)))
    {
      HighestHorizontalResolution = BiosVideoPrivate->VbeModeInformationBlock->XResolution;
      HighestVerticalResolution   = BiosVideoPrivate->VbeModeInformationBlock->YResolution;
      HighestResolutionMode       = ModeNumber;
    }

    //
    // Add mode to the list of available modes
    //
    ModeNumber++;
    ModeBuffer = (BIOS_VIDEO_MODE_DATA *)AllocatePool (
                                           ModeNumber * sizeof (BIOS_VIDEO_MODE_DATA)
                                           );
    if (NULL == ModeBuffer) {
      Status = EFI_OUT_OF_RESOURCES;
      goto Done;
    }

    if (ModeNumber > 1) {
      CopyMem (
        ModeBuffer,
        BiosVideoPrivate->ModeData,
        (ModeNumber - 1) * sizeof (BIOS_VIDEO_MODE_DATA)
        );
    }

    if (BiosVideoPrivate->ModeData != NULL) {
      FreePool (BiosVideoPrivate->ModeData);
    }

    CurrentModeData                = &ModeBuffer[ModeNumber - 1];
    CurrentModeData->VbeModeNumber = VbeModeNumber;
    if (BiosVideoPrivate->VbeInformationBlock->VESAVersion >= VESA_BIOS_EXTENSIONS_VERSION_3_0) {
      CurrentModeData->BytesPerScanLine  = BiosVideoPrivate->VbeModeInformationBlock->LinBytesPerScanLine;
      CurrentModeData->Red.Position      = BiosVideoPrivate->VbeModeInformationBlock->LinRedFieldPosition;
      CurrentModeData->Red.Mask          = (UINT8)((1 << BiosVideoPrivate->VbeModeInformationBlock->LinRedMaskSize) - 1);
      CurrentModeData->Blue.Position     = BiosVideoPrivate->VbeModeInformationBlock->LinBlueFieldPosition;
      CurrentModeData->Blue.Mask         = (UINT8)((1 << BiosVideoPrivate->VbeModeInformationBlock->LinBlueMaskSize) - 1);
      CurrentModeData->Green.Position    = BiosVideoPrivate->VbeModeInformationBlock->LinGreenFieldPosition;
      CurrentModeData->Green.Mask        = (UINT8)((1 << BiosVideoPrivate->VbeModeInformationBlock->LinGreenMaskSize) - 1);
      CurrentModeData->Reserved.Position = BiosVideoPrivate->VbeModeInformationBlock->LinRsvdFieldPosition;
      CurrentModeData->Reserved.Mask     = (UINT8)((1 << BiosVideoPrivate->VbeModeInformationBlock->LinRsvdMaskSize) - 1);
    } else {
      CurrentModeData->BytesPerScanLine  = BiosVideoPrivate->VbeModeInformationBlock->BytesPerScanLine;
      CurrentModeData->Red.Position      = BiosVideoPrivate->VbeModeInformationBlock->RedFieldPosition;
      CurrentModeData->Red.Mask          = (UINT8)((1 << BiosVideoPrivate->VbeModeInformationBlock->RedMaskSize) - 1);
      CurrentModeData->Blue.Position     = BiosVideoPrivate->VbeModeInformationBlock->BlueFieldPosition;
      CurrentModeData->Blue.Mask         = (UINT8)((1 << BiosVideoPrivate->VbeModeInformationBlock->BlueMaskSize) - 1);
      CurrentModeData->Green.Position    = BiosVideoPrivate->VbeModeInformationBlock->GreenFieldPosition;
      CurrentModeData->Green.Mask        = (UINT8)((1 << BiosVideoPrivate->VbeModeInformationBlock->GreenMaskSize) - 1);
      CurrentModeData->Reserved.Position = BiosVideoPrivate->VbeModeInformationBlock->RsvdFieldPosition;
      CurrentModeData->Reserved.Mask     = (UINT8)((1 << BiosVideoPrivate->VbeModeInformationBlock->RsvdMaskSize) - 1);
    }

    CurrentModeData->PixelFormat = PixelBitMask;
    if ((BiosVideoPrivate->VbeModeInformationBlock->BitsPerPixel == 32) &&
        (CurrentModeData->Red.Mask == 0xff) && (CurrentModeData->Green.Mask == 0xff) && (CurrentModeData->Blue.Mask == 0xff))
    {
      if ((CurrentModeData->Red.Position == 0) && (CurrentModeData->Green.Position == 8) && (CurrentModeData->Blue.Position == 16)) {
        CurrentModeData->PixelFormat = PixelRedGreenBlueReserved8BitPerColor;
      } else if ((CurrentModeData->Blue.Position == 0) && (CurrentModeData->Green.Position == 8) && (CurrentModeData->Red.Position == 16)) {
        CurrentModeData->PixelFormat = PixelBlueGreenRedReserved8BitPerColor;
      }
    }

    CurrentModeData->PixelBitMask.RedMask      = ((UINT32)CurrentModeData->Red.Mask) << CurrentModeData->Red.Position;
    CurrentModeData->PixelBitMask.GreenMask    = ((UINT32)CurrentModeData->Green.Mask) << CurrentModeData->Green.Position;
    CurrentModeData->PixelBitMask.BlueMask     = ((UINT32)CurrentModeData->Blue.Mask) << CurrentModeData->Blue.Position;
    CurrentModeData->PixelBitMask.ReservedMask = ((UINT32)CurrentModeData->Reserved.Mask) << CurrentModeData->Reserved.Position;

    CurrentModeData->LinearFrameBuffer    = (VOID *)(UINTN)BiosVideoPrivate->VbeModeInformationBlock->PhysBasePtr;
    CurrentModeData->HorizontalResolution = BiosVideoPrivate->VbeModeInformationBlock->XResolution;
    CurrentModeData->VerticalResolution   = BiosVideoPrivate->VbeModeInformationBlock->YResolution;

    CurrentModeData->BitsPerPixel    = BiosVideoPrivate->VbeModeInformationBlock->BitsPerPixel;
    CurrentModeData->FrameBufferSize = CurrentModeData->BytesPerScanLine * CurrentModeData->VerticalResolution;
    //
    // Make sure the FrameBufferSize does not exceed the max available frame buffer size reported by VEB.
    //
    ASSERT (CurrentModeData->FrameBufferSize <= ((UINT32)BiosVideoPrivate->VbeInformationBlock->TotalMemory * 64 * 1024));

    BiosVideoPrivate->ModeData = ModeBuffer;
  }

  //
  // Check to see if we found any modes that are compatible with GRAPHICS OUTPUT
  //
  if (ModeNumber == 0) {
    Status = EFI_DEVICE_ERROR;
    goto Done;
  }

  //
  // Assign Gop's Blt function
  //
  BiosVideoPrivate->GraphicsOutput.Blt = BiosVideoGraphicsOutputVbeBlt;

  BiosVideoPrivate->GraphicsOutput.Mode->MaxMode = (UINT32)ModeNumber;
  //
  // Current mode is unknow till now, set it to an invalid mode.
  //
  BiosVideoPrivate->GraphicsOutput.Mode->Mode = GRAPHICS_OUTPUT_INVALIDE_MODE_NUMBER;

  //
  // Find the best mode to initialize
  //
  if ((PcdGet32 (PcdVideoHorizontalResolution) == 0x0) || (PcdGet32 (PcdVideoVerticalResolution) == 0x0)) {
    DEBUG_CODE (
      BIOS_VIDEO_MODE_DATA    *ModeData;
      ModeData = &BiosVideoPrivate->ModeData[HighestResolutionMode];
      DEBUG ((
        DEBUG_INFO,
        "BiosVideo set highest resolution %d x %d\n",
        ModeData->HorizontalResolution,
        ModeData->VerticalResolution
        ));
      );
    PreferMode = HighestResolutionMode;
  }

  Status = BiosVideoGraphicsOutputSetMode (&BiosVideoPrivate->GraphicsOutput, (UINT32)PreferMode);
  if (EFI_ERROR (Status)) {
    for (PreferMode = 0; PreferMode < ModeNumber; PreferMode++) {
      Status = BiosVideoGraphicsOutputSetMode (
                 &BiosVideoPrivate->GraphicsOutput,
                 (UINT32)PreferMode
                 );
      if (!EFI_ERROR (Status)) {
        break;
      }
    }

    if (PreferMode == ModeNumber) {
      //
      // None mode is set successfully.
      //
      goto Done;
    }
  }

Done:
  //
  // If there was an error, then free the mode structure
  //
  if (EFI_ERROR (Status)) {
    if (BiosVideoPrivate->ModeData != NULL) {
      FreePool (BiosVideoPrivate->ModeData);
      BiosVideoPrivate->ModeData = NULL;
      BiosVideoPrivate->MaxMode  = 0;
    }

    if (EdidOverrideDataBlock != NULL) {
      FreePool (EdidOverrideDataBlock);
    }
  }

  return Status;
}

/**
  Check for VGA device.

  @param  BiosVideoPrivate       Pointer to BIOS_VIDEO_DEV structure

  @retval EFI_SUCCESS            Standard VGA device found

**/
EFI_STATUS
BiosVideoCheckForVga (
  IN OUT BIOS_VIDEO_DEV  *BiosVideoPrivate
  )
{
  EFI_STATUS            Status;
  BIOS_VIDEO_MODE_DATA  *ModeBuffer;

  Status = EFI_UNSUPPORTED;

  //
  // Assign Gop's Blt function
  //
  BiosVideoPrivate->GraphicsOutput.Blt = BiosVideoGraphicsOutputVgaBlt;

  //
  // Add mode to the list of available modes
  // caller should guarantee that Mode has been allocated.
  //
  ASSERT (BiosVideoPrivate->GraphicsOutput.Mode != NULL);
  BiosVideoPrivate->GraphicsOutput.Mode->MaxMode = 1;

  ModeBuffer = (BIOS_VIDEO_MODE_DATA *)AllocatePool (
                                         sizeof (BIOS_VIDEO_MODE_DATA)
                                         );
  if (NULL == ModeBuffer) {
    Status = EFI_OUT_OF_RESOURCES;
    goto Done;
  }

  ModeBuffer->VbeModeNumber        = 0x0012;
  ModeBuffer->BytesPerScanLine     = 640;
  ModeBuffer->LinearFrameBuffer    = (VOID *)(UINTN)(0xa0000);
  ModeBuffer->HorizontalResolution = 640;
  ModeBuffer->VerticalResolution   = 480;
  ModeBuffer->PixelFormat          = PixelBltOnly;
  ModeBuffer->BitsPerPixel         = 8;
  ModeBuffer->ColorDepth           = 32;
  ModeBuffer->RefreshRate          = 60;

  BiosVideoPrivate->ModeData = ModeBuffer;

  //
  // Test to see if the Video Adapter support the 640x480 16 color mode
  //
  BiosVideoPrivate->GraphicsOutput.Mode->Mode = GRAPHICS_OUTPUT_INVALIDE_MODE_NUMBER;
  Status                                      = BiosVideoGraphicsOutputSetMode (&BiosVideoPrivate->GraphicsOutput, 0);

Done:
  //
  // If there was an error, then free the mode structure
  //
  if (EFI_ERROR (Status)) {
    if (BiosVideoPrivate->ModeData != NULL) {
      FreePool (BiosVideoPrivate->ModeData);
      BiosVideoPrivate->ModeData = NULL;
    }

    if (BiosVideoPrivate->GraphicsOutput.Mode != NULL) {
      if (BiosVideoPrivate->GraphicsOutput.Mode->Info != NULL) {
        FreePool (BiosVideoPrivate->GraphicsOutput.Mode->Info);
        BiosVideoPrivate->GraphicsOutput.Mode->Info = NULL;
      }

      FreePool (BiosVideoPrivate->GraphicsOutput.Mode);
      BiosVideoPrivate->GraphicsOutput.Mode = NULL;
    }
  }

  return Status;
}

//
// Graphics Output Protocol Member Functions for VESA BIOS Extensions
//

/**
  Graphics Output protocol interface to get video mode.

  @param  This                   Protocol instance pointer.
  @param  ModeNumber             The mode number to return information on.
  @param  SizeOfInfo             A pointer to the size, in bytes, of the Info
                                 buffer.
  @param  Info                   Caller allocated buffer that returns information
                                 about ModeNumber.

  @retval EFI_SUCCESS            Mode information returned.
  @retval EFI_DEVICE_ERROR       A hardware error occurred trying to retrieve the
                                 video mode.
  @retval EFI_NOT_STARTED        Video display is not initialized. Call SetMode ()
  @retval EFI_INVALID_PARAMETER  One of the input args was NULL.

**/
EFI_STATUS
EFIAPI
BiosVideoGraphicsOutputQueryMode (
  IN  EFI_GRAPHICS_OUTPUT_PROTOCOL          *This,
  IN  UINT32                                ModeNumber,
  OUT UINTN                                 *SizeOfInfo,
  OUT EFI_GRAPHICS_OUTPUT_MODE_INFORMATION  **Info
  )
{
  BIOS_VIDEO_DEV        *BiosVideoPrivate;
  BIOS_VIDEO_MODE_DATA  *ModeData;

  BiosVideoPrivate = BIOS_VIDEO_DEV_FROM_GRAPHICS_OUTPUT_THIS (This);

  if (BiosVideoPrivate->HardwareNeedsStarting) {
    REPORT_STATUS_CODE_WITH_DEVICE_PATH (
      EFI_ERROR_CODE | EFI_ERROR_MINOR,
      EFI_PERIPHERAL_LOCAL_CONSOLE | EFI_P_EC_OUTPUT_ERROR,
      BiosVideoPrivate->GopDevicePath
      );
    return EFI_NOT_STARTED;
  }

  if ((This == NULL) || (Info == NULL) || (SizeOfInfo == NULL) || (ModeNumber >= This->Mode->MaxMode)) {
    return EFI_INVALID_PARAMETER;
  }

  *Info = (EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *)AllocatePool (
                                                    sizeof (EFI_GRAPHICS_OUTPUT_MODE_INFORMATION)
                                                    );
  if (NULL == *Info) {
    return EFI_OUT_OF_RESOURCES;
  }

  *SizeOfInfo = sizeof (EFI_GRAPHICS_OUTPUT_MODE_INFORMATION);

  ModeData                      = &BiosVideoPrivate->ModeData[ModeNumber];
  (*Info)->Version              = 0;
  (*Info)->HorizontalResolution = ModeData->HorizontalResolution;
  (*Info)->VerticalResolution   = ModeData->VerticalResolution;
  (*Info)->PixelFormat          = ModeData->PixelFormat;
  CopyMem (&((*Info)->PixelInformation), &(ModeData->PixelBitMask), sizeof (ModeData->PixelBitMask));

  (*Info)->PixelsPerScanLine =  (ModeData->BytesPerScanLine * 8) / ModeData->BitsPerPixel;

  return EFI_SUCCESS;
}

/**
  Worker function to set video mode.

  @param  BiosVideoPrivate       Instance of BIOS_VIDEO_DEV.
  @param  ModeData               The mode data to be set.
  @param  DevicePath             Pointer to Device Path Protocol.

  @retval EFI_SUCCESS            Graphics mode was changed.
  @retval EFI_DEVICE_ERROR       The device had an error and could not complete the
                                 request.
  @retval EFI_UNSUPPORTED        ModeNumber is not supported by this device.

**/
EFI_STATUS
BiosVideoSetModeWorker (
  IN  BIOS_VIDEO_DEV            *BiosVideoPrivate,
  IN  BIOS_VIDEO_MODE_DATA      *ModeData,
  IN  EFI_DEVICE_PATH_PROTOCOL  *DevicePath
  )
{
  EFI_STATUS             Status;
  EFI_IA32_REGISTER_SET  Regs;

  if (BiosVideoPrivate->LineBuffer != NULL) {
    FreePool (BiosVideoPrivate->LineBuffer);
  }

  if (BiosVideoPrivate->VgaFrameBuffer != NULL) {
    FreePool (BiosVideoPrivate->VgaFrameBuffer);
  }

  if (BiosVideoPrivate->VbeFrameBuffer != NULL) {
    FreePool (BiosVideoPrivate->VbeFrameBuffer);
  }

  BiosVideoPrivate->LineBuffer = (UINT8 *)AllocatePool (
                                            ModeData->BytesPerScanLine
                                            );
  if (NULL == BiosVideoPrivate->LineBuffer) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Clear all registers
  //
  ZeroMem (&Regs, sizeof (Regs));

  if (ModeData->VbeModeNumber < 0x100) {
    //
    // Allocate a working buffer for BLT operations to the VGA frame buffer
    //
    BiosVideoPrivate->VgaFrameBuffer = (UINT8 *)AllocatePool (4 * 480 * 80);
    if (NULL == BiosVideoPrivate->VgaFrameBuffer) {
      return EFI_OUT_OF_RESOURCES;
    }

    //
    // Set VGA Mode
    //
    Regs.X.AX = ModeData->VbeModeNumber;
    BiosVideoPrivate->LegacyBios->Int86 (BiosVideoPrivate->LegacyBios, 0x10, &Regs);
  } else {
    //
    // Allocate a working buffer for BLT operations to the VBE frame buffer
    //
    BiosVideoPrivate->VbeFrameBuffer =
      (EFI_GRAPHICS_OUTPUT_BLT_PIXEL *)AllocatePool (
                                         ModeData->BytesPerScanLine * ModeData->VerticalResolution
                                         );
    if (NULL == BiosVideoPrivate->VbeFrameBuffer) {
      return EFI_OUT_OF_RESOURCES;
    }

    //
    // Set VBE mode
    //
    Regs.X.AX = VESA_BIOS_EXTENSIONS_SET_MODE;
    Regs.X.BX = (UINT16)(ModeData->VbeModeNumber | VESA_BIOS_EXTENSIONS_MODE_NUMBER_LINEAR_FRAME_BUFFER);
    ZeroMem (BiosVideoPrivate->VbeCrtcInformationBlock, sizeof (VESA_BIOS_EXTENSIONS_CRTC_INFORMATION_BLOCK));
    Regs.X.ES = EFI_SEGMENT ((UINTN)BiosVideoPrivate->VbeCrtcInformationBlock);
    Regs.X.DI = EFI_OFFSET ((UINTN)BiosVideoPrivate->VbeCrtcInformationBlock);
    BiosVideoPrivate->LegacyBios->Int86 (BiosVideoPrivate->LegacyBios, 0x10, &Regs);

    //
    // Check to see if the call succeeded
    //
    if (Regs.X.AX != VESA_BIOS_EXTENSIONS_STATUS_SUCCESS) {
      REPORT_STATUS_CODE_WITH_DEVICE_PATH (
        EFI_ERROR_CODE | EFI_ERROR_MINOR,
        EFI_PERIPHERAL_LOCAL_CONSOLE | EFI_P_EC_OUTPUT_ERROR,
        DevicePath
        );
      return EFI_DEVICE_ERROR;
    }

    //
    // Initialize the state of the VbeFrameBuffer
    //
    Status = BiosVideoPrivate->PciIo->Mem.Read (
                                            BiosVideoPrivate->PciIo,
                                            EfiPciIoWidthUint32,
                                            EFI_PCI_IO_PASS_THROUGH_BAR,
                                            (UINT64)(UINTN)ModeData->LinearFrameBuffer,
                                            (ModeData->BytesPerScanLine * ModeData->VerticalResolution) >> 2,
                                            BiosVideoPrivate->VbeFrameBuffer
                                            );
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  return EFI_SUCCESS;
}

/**
  Graphics Output protocol interface to set video mode.

  @param  This                   Protocol instance pointer.
  @param  ModeNumber             The mode number to be set.

  @retval EFI_SUCCESS            Graphics mode was changed.
  @retval EFI_DEVICE_ERROR       The device had an error and could not complete the
                                 request.
  @retval EFI_UNSUPPORTED        ModeNumber is not supported by this device.

**/
EFI_STATUS
EFIAPI
BiosVideoGraphicsOutputSetMode (
  IN  EFI_GRAPHICS_OUTPUT_PROTOCOL  *This,
  IN  UINT32                        ModeNumber
  )
{
  EFI_STATUS                     Status;
  BIOS_VIDEO_DEV                 *BiosVideoPrivate;
  BIOS_VIDEO_MODE_DATA           *ModeData;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL  Background;

  if (This == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  BiosVideoPrivate = BIOS_VIDEO_DEV_FROM_GRAPHICS_OUTPUT_THIS (This);

  ModeData = &BiosVideoPrivate->ModeData[ModeNumber];

  if (ModeNumber >= This->Mode->MaxMode) {
    return EFI_UNSUPPORTED;
  }

  if (ModeNumber == This->Mode->Mode) {
    //
    // Clear screen to black
    //
    ZeroMem (&Background, sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL));
    BiosVideoGraphicsOutputVbeBlt (
      This,
      &Background,
      EfiBltVideoFill,
      0,
      0,
      0,
      0,
      ModeData->HorizontalResolution,
      ModeData->VerticalResolution,
      0
      );
    return EFI_SUCCESS;
  }

  Status = BiosVideoSetModeWorker (BiosVideoPrivate, ModeData, BiosVideoPrivate->GopDevicePath);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  This->Mode->Mode                       = ModeNumber;
  This->Mode->Info->Version              = 0;
  This->Mode->Info->HorizontalResolution = ModeData->HorizontalResolution;
  This->Mode->Info->VerticalResolution   = ModeData->VerticalResolution;
  This->Mode->Info->PixelFormat          = ModeData->PixelFormat;
  CopyMem (&(This->Mode->Info->PixelInformation), &(ModeData->PixelBitMask), sizeof (ModeData->PixelBitMask));
  This->Mode->Info->PixelsPerScanLine =  (ModeData->BytesPerScanLine * 8) / ModeData->BitsPerPixel;
  This->Mode->SizeOfInfo              = sizeof (EFI_GRAPHICS_OUTPUT_MODE_INFORMATION);
  This->Mode->FrameBufferSize         = ModeData->FrameBufferSize;
  This->Mode->FrameBufferBase         = (EFI_PHYSICAL_ADDRESS)(UINTN)ModeData->LinearFrameBuffer;

  BiosVideoPrivate->HardwareNeedsStarting = FALSE;

  return EFI_SUCCESS;
}

/**
  Update physical frame buffer, copy 4 bytes block, then copy remaining bytes.

  @param   PciIo              The pointer of EFI_PCI_IO_PROTOCOL
  @param   VbeBuffer          The data to transfer to screen
  @param   MemAddress         Physical frame buffer base address
  @param   DestinationX       The X coordinate of the destination for BltOperation
  @param   DestinationY       The Y coordinate of the destination for BltOperation
  @param   TotalBytes         The total bytes of copy
  @param   VbePixelWidth      Bytes per pixel
  @param   BytesPerScanLine   Bytes per scan line

**/
VOID
CopyVideoBuffer (
  IN  EFI_PCI_IO_PROTOCOL  *PciIo,
  IN  UINT8                *VbeBuffer,
  IN  VOID                 *MemAddress,
  IN  UINTN                DestinationX,
  IN  UINTN                DestinationY,
  IN  UINTN                TotalBytes,
  IN  UINT32               VbePixelWidth,
  IN  UINTN                BytesPerScanLine
  )
{
  UINTN       FrameBufferAddr;
  UINTN       CopyBlockNum;
  UINTN       RemainingBytes;
  UINTN       UnalignedBytes;
  EFI_STATUS  Status;

  FrameBufferAddr = (UINTN)MemAddress + (DestinationY * BytesPerScanLine) + DestinationX * VbePixelWidth;

  //
  // If TotalBytes is less than 4 bytes, only start byte copy.
  //
  if (TotalBytes < 4) {
    Status = PciIo->Mem.Write (
                          PciIo,
                          EfiPciIoWidthUint8,
                          EFI_PCI_IO_PASS_THROUGH_BAR,
                          (UINT64)FrameBufferAddr,
                          TotalBytes,
                          VbeBuffer
                          );
    ASSERT_EFI_ERROR (Status);
    return;
  }

  //
  // If VbeBuffer is not 4-byte aligned, start byte copy.
  //
  UnalignedBytes = (4 - ((UINTN)VbeBuffer & 0x3)) & 0x3;

  if (UnalignedBytes != 0) {
    Status = PciIo->Mem.Write (
                          PciIo,
                          EfiPciIoWidthUint8,
                          EFI_PCI_IO_PASS_THROUGH_BAR,
                          (UINT64)FrameBufferAddr,
                          UnalignedBytes,
                          VbeBuffer
                          );
    ASSERT_EFI_ERROR (Status);
    FrameBufferAddr += UnalignedBytes;
    VbeBuffer       += UnalignedBytes;
  }

  //
  // Calculate 4-byte block count and remaining bytes.
  //
  CopyBlockNum   = (TotalBytes - UnalignedBytes) >> 2;
  RemainingBytes = (TotalBytes - UnalignedBytes) &  3;

  //
  // Copy 4-byte block and remaining bytes to physical frame buffer.
  //
  if (CopyBlockNum != 0) {
    Status = PciIo->Mem.Write (
                          PciIo,
                          EfiPciIoWidthUint32,
                          EFI_PCI_IO_PASS_THROUGH_BAR,
                          (UINT64)FrameBufferAddr,
                          CopyBlockNum,
                          VbeBuffer
                          );
    ASSERT_EFI_ERROR (Status);
  }

  if (RemainingBytes != 0) {
    FrameBufferAddr += (CopyBlockNum << 2);
    VbeBuffer       += (CopyBlockNum << 2);
    Status           = PciIo->Mem.Write (
                                    PciIo,
                                    EfiPciIoWidthUint8,
                                    EFI_PCI_IO_PASS_THROUGH_BAR,
                                    (UINT64)FrameBufferAddr,
                                    RemainingBytes,
                                    VbeBuffer
                                    );
    ASSERT_EFI_ERROR (Status);
  }
}

/**
  Worker function to block transfer for VBE device.

  @param  BiosVideoPrivate       Instance of BIOS_VIDEO_DEV
  @param  BltBuffer              The data to transfer to screen
  @param  BltOperation           The operation to perform
  @param  SourceX                The X coordinate of the source for BltOperation
  @param  SourceY                The Y coordinate of the source for BltOperation
  @param  DestinationX           The X coordinate of the destination for
                                 BltOperation
  @param  DestinationY           The Y coordinate of the destination for
                                 BltOperation
  @param  Width                  The width of a rectangle in the blt rectangle in
                                 pixels
  @param  Height                 The height of a rectangle in the blt rectangle in
                                 pixels
  @param  Delta                  Not used for EfiBltVideoFill and
                                 EfiBltVideoToVideo operation. If a Delta of 0 is
                                 used, the entire BltBuffer will be operated on. If
                                 a subrectangle of the BltBuffer is used, then
                                 Delta represents the number of bytes in a row of
                                 the BltBuffer.
  @param  Mode                   Mode data.

  @retval EFI_INVALID_PARAMETER  Invalid parameter passed in
  @retval EFI_SUCCESS            Blt operation success

**/
EFI_STATUS
BiosVideoVbeBltWorker (
  IN  BIOS_VIDEO_DEV                     *BiosVideoPrivate,
  IN  EFI_GRAPHICS_OUTPUT_BLT_PIXEL      *BltBuffer  OPTIONAL,
  IN  EFI_GRAPHICS_OUTPUT_BLT_OPERATION  BltOperation,
  IN  UINTN                              SourceX,
  IN  UINTN                              SourceY,
  IN  UINTN                              DestinationX,
  IN  UINTN                              DestinationY,
  IN  UINTN                              Width,
  IN  UINTN                              Height,
  IN  UINTN                              Delta,
  IN  BIOS_VIDEO_MODE_DATA               *Mode
  )
{
  EFI_PCI_IO_PROTOCOL            *PciIo;
  EFI_TPL                        OriginalTPL;
  UINTN                          DstY;
  UINTN                          SrcY;
  UINTN                          DstX;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL  *Blt;
  VOID                           *MemAddress;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL  *VbeFrameBuffer;
  UINTN                          BytesPerScanLine;
  UINTN                          Index;
  UINT8                          *VbeBuffer;
  UINT8                          *VbeBuffer1;
  UINT8                          *BltUint8;
  UINT32                         VbePixelWidth;
  UINT32                         Pixel;
  UINTN                          TotalBytes;

  PciIo = BiosVideoPrivate->PciIo;

  VbeFrameBuffer   = BiosVideoPrivate->VbeFrameBuffer;
  MemAddress       = Mode->LinearFrameBuffer;
  BytesPerScanLine = Mode->BytesPerScanLine;
  VbePixelWidth    = Mode->BitsPerPixel / 8;
  BltUint8         = (UINT8 *)BltBuffer;
  TotalBytes       = Width * VbePixelWidth;

  if (((UINTN)BltOperation) >= EfiGraphicsOutputBltOperationMax) {
    return EFI_INVALID_PARAMETER;
  }

  if ((Width == 0) || (Height == 0)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // We need to fill the Virtual Screen buffer with the blt data.
  // The virtual screen is upside down, as the first row is the bootom row of
  // the image.
  //
  if (BltOperation == EfiBltVideoToBltBuffer) {
    //
    // Video to BltBuffer: Source is Video, destination is BltBuffer
    //
    if (SourceY + Height > Mode->VerticalResolution) {
      return EFI_INVALID_PARAMETER;
    }

    if (SourceX + Width > Mode->HorizontalResolution) {
      return EFI_INVALID_PARAMETER;
    }
  } else {
    //
    // BltBuffer to Video: Source is BltBuffer, destination is Video
    //
    if (DestinationY + Height > Mode->VerticalResolution) {
      return EFI_INVALID_PARAMETER;
    }

    if (DestinationX + Width > Mode->HorizontalResolution) {
      return EFI_INVALID_PARAMETER;
    }
  }

  //
  // If Delta is zero, then the entire BltBuffer is being used, so Delta
  // is the number of bytes in each row of BltBuffer.  Since BltBuffer is Width pixels size,
  // the number of bytes in each row can be computed.
  //
  if (Delta == 0) {
    Delta = Width * sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL);
  }

  //
  // We have to raise to TPL Notify, so we make an atomic write the frame buffer.
  // We would not want a timer based event (Cursor, ...) to come in while we are
  // doing this operation.
  //
  OriginalTPL = gBS->RaiseTPL (TPL_NOTIFY);

  switch (BltOperation) {
    case EfiBltVideoToBltBuffer:
      for (SrcY = SourceY, DstY = DestinationY; DstY < (Height + DestinationY); SrcY++, DstY++) {
        Blt = (EFI_GRAPHICS_OUTPUT_BLT_PIXEL *)(BltUint8 + DstY * Delta + DestinationX * sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL));
        //
        // Shuffle the packed bytes in the hardware buffer to match EFI_GRAPHICS_OUTPUT_BLT_PIXEL
        //
        VbeBuffer = ((UINT8 *)VbeFrameBuffer + (SrcY * BytesPerScanLine + SourceX * VbePixelWidth));
        for (DstX = DestinationX; DstX < (Width + DestinationX); DstX++) {
          Pixel         = VbeBuffer[0] | VbeBuffer[1] << 8 | VbeBuffer[2] << 16 | VbeBuffer[3] << 24;
          Blt->Red      = (UINT8)((Pixel >> Mode->Red.Position) & Mode->Red.Mask);
          Blt->Blue     = (UINT8)((Pixel >> Mode->Blue.Position) & Mode->Blue.Mask);
          Blt->Green    = (UINT8)((Pixel >> Mode->Green.Position) & Mode->Green.Mask);
          Blt->Reserved = 0;
          Blt++;
          VbeBuffer += VbePixelWidth;
        }
      }

      break;

    case EfiBltVideoToVideo:
      for (Index = 0; Index < Height; Index++) {
        if (DestinationY <= SourceY) {
          SrcY = SourceY + Index;
          DstY = DestinationY + Index;
        } else {
          SrcY = SourceY + Height - Index - 1;
          DstY = DestinationY + Height - Index - 1;
        }

        VbeBuffer  = ((UINT8 *)VbeFrameBuffer + DstY * BytesPerScanLine + DestinationX * VbePixelWidth);
        VbeBuffer1 = ((UINT8 *)VbeFrameBuffer + SrcY * BytesPerScanLine + SourceX * VbePixelWidth);

        gBS->CopyMem (
               VbeBuffer,
               VbeBuffer1,
               TotalBytes
               );

        //
        // Update physical frame buffer.
        //
        CopyVideoBuffer (
          PciIo,
          VbeBuffer,
          MemAddress,
          DestinationX,
          DstY,
          TotalBytes,
          VbePixelWidth,
          BytesPerScanLine
          );
      }

      break;

    case EfiBltVideoFill:
      VbeBuffer = (UINT8 *)((UINTN)VbeFrameBuffer + (DestinationY * BytesPerScanLine) + DestinationX * VbePixelWidth);
      Blt       = (EFI_GRAPHICS_OUTPUT_BLT_PIXEL *)BltUint8;
      //
      // Shuffle the RGB fields in EFI_GRAPHICS_OUTPUT_BLT_PIXEL to match the hardware buffer
      //
      Pixel = ((Blt->Red & Mode->Red.Mask) << Mode->Red.Position) |
              (
               (Blt->Green & Mode->Green.Mask) <<
               Mode->Green.Position
              ) |
              ((Blt->Blue & Mode->Blue.Mask) << Mode->Blue.Position);

      for (Index = 0; Index < Width; Index++) {
        gBS->CopyMem (
               VbeBuffer,
               &Pixel,
               VbePixelWidth
               );
        VbeBuffer += VbePixelWidth;
      }

      VbeBuffer = (UINT8 *)((UINTN)VbeFrameBuffer + (DestinationY * BytesPerScanLine) + DestinationX * VbePixelWidth);
      for (DstY = DestinationY + 1; DstY < (Height + DestinationY); DstY++) {
        gBS->CopyMem (
               (VOID *)((UINTN)VbeFrameBuffer + (DstY * BytesPerScanLine) + DestinationX * VbePixelWidth),
               VbeBuffer,
               TotalBytes
               );
      }

      for (DstY = DestinationY; DstY < (Height + DestinationY); DstY++) {
        //
        // Update physical frame buffer.
        //
        CopyVideoBuffer (
          PciIo,
          VbeBuffer,
          MemAddress,
          DestinationX,
          DstY,
          TotalBytes,
          VbePixelWidth,
          BytesPerScanLine
          );
      }

      break;

    case EfiBltBufferToVideo:
      for (SrcY = SourceY, DstY = DestinationY; SrcY < (Height + SourceY); SrcY++, DstY++) {
        Blt       = (EFI_GRAPHICS_OUTPUT_BLT_PIXEL *)(BltUint8 + (SrcY * Delta) + (SourceX) * sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL));
        VbeBuffer = ((UINT8 *)VbeFrameBuffer + (DstY * BytesPerScanLine + DestinationX * VbePixelWidth));
        for (DstX = DestinationX; DstX < (Width + DestinationX); DstX++) {
          //
          // Shuffle the RGB fields in EFI_GRAPHICS_OUTPUT_BLT_PIXEL to match the hardware buffer
          //
          Pixel = ((Blt->Red & Mode->Red.Mask) << Mode->Red.Position) |
                  ((Blt->Green & Mode->Green.Mask) << Mode->Green.Position) |
                  ((Blt->Blue & Mode->Blue.Mask) << Mode->Blue.Position);
          gBS->CopyMem (
                 VbeBuffer,
                 &Pixel,
                 VbePixelWidth
                 );
          Blt++;
          VbeBuffer += VbePixelWidth;
        }

        VbeBuffer = ((UINT8 *)VbeFrameBuffer + (DstY * BytesPerScanLine + DestinationX * VbePixelWidth));

        //
        // Update physical frame buffer.
        //
        CopyVideoBuffer (
          PciIo,
          VbeBuffer,
          MemAddress,
          DestinationX,
          DstY,
          TotalBytes,
          VbePixelWidth,
          BytesPerScanLine
          );
      }

      break;

    default:;
  }

  gBS->RestoreTPL (OriginalTPL);

  return EFI_SUCCESS;
}

/**
  Graphics Output protocol instance to block transfer for VBE device.

  @param  This                   Pointer to Graphics Output protocol instance
  @param  BltBuffer              The data to transfer to screen
  @param  BltOperation           The operation to perform
  @param  SourceX                The X coordinate of the source for BltOperation
  @param  SourceY                The Y coordinate of the source for BltOperation
  @param  DestinationX           The X coordinate of the destination for
                                 BltOperation
  @param  DestinationY           The Y coordinate of the destination for
                                 BltOperation
  @param  Width                  The width of a rectangle in the blt rectangle in
                                 pixels
  @param  Height                 The height of a rectangle in the blt rectangle in
                                 pixels
  @param  Delta                  Not used for EfiBltVideoFill and
                                 EfiBltVideoToVideo operation. If a Delta of 0 is
                                 used, the entire BltBuffer will be operated on. If
                                 a subrectangle of the BltBuffer is used, then
                                 Delta represents the number of bytes in a row of
                                 the BltBuffer.

  @retval EFI_INVALID_PARAMETER  Invalid parameter passed in
  @retval EFI_SUCCESS            Blt operation success

**/
EFI_STATUS
EFIAPI
BiosVideoGraphicsOutputVbeBlt (
  IN  EFI_GRAPHICS_OUTPUT_PROTOCOL       *This,
  IN  EFI_GRAPHICS_OUTPUT_BLT_PIXEL      *BltBuffer  OPTIONAL,
  IN  EFI_GRAPHICS_OUTPUT_BLT_OPERATION  BltOperation,
  IN  UINTN                              SourceX,
  IN  UINTN                              SourceY,
  IN  UINTN                              DestinationX,
  IN  UINTN                              DestinationY,
  IN  UINTN                              Width,
  IN  UINTN                              Height,
  IN  UINTN                              Delta
  )
{
  BIOS_VIDEO_DEV        *BiosVideoPrivate;
  BIOS_VIDEO_MODE_DATA  *Mode;

  if (This == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  BiosVideoPrivate = BIOS_VIDEO_DEV_FROM_GRAPHICS_OUTPUT_THIS (This);
  Mode             = &BiosVideoPrivate->ModeData[This->Mode->Mode];

  return BiosVideoVbeBltWorker (
           BiosVideoPrivate,
           BltBuffer,
           BltOperation,
           SourceX,
           SourceY,
           DestinationX,
           DestinationY,
           Width,
           Height,
           Delta,
           Mode
           );
}

/**
  Write graphics controller registers.

  @param  PciIo                  Pointer to PciIo protocol instance of the
                                 controller
  @param  Address                Register address
  @param  Data                   Data to be written to register

  @return None

**/
VOID
WriteGraphicsController (
  IN  EFI_PCI_IO_PROTOCOL  *PciIo,
  IN  UINTN                Address,
  IN  UINTN                Data
  )
{
  Address = Address | (Data << 8);
  PciIo->Io.Write (
              PciIo,
              EfiPciIoWidthUint16,
              EFI_PCI_IO_PASS_THROUGH_BAR,
              VGA_GRAPHICS_CONTROLLER_ADDRESS_REGISTER,
              1,
              &Address
              );
}

/**
  Read the four bit plane of VGA frame buffer.

  @param  PciIo                  Pointer to PciIo protocol instance of the
                                 controller
  @param  HardwareBuffer         Hardware VGA frame buffer address
  @param  MemoryBuffer           Memory buffer address
  @param  WidthInBytes           Number of bytes in a line to read
  @param  Height                 Height of the area to read

  @return None

**/
VOID
VgaReadBitPlanes (
  EFI_PCI_IO_PROTOCOL  *PciIo,
  UINT8                *HardwareBuffer,
  UINT8                *MemoryBuffer,
  UINTN                WidthInBytes,
  UINTN                Height
  )
{
  UINTN  BitPlane;
  UINTN  Rows;
  UINTN  FrameBufferOffset;
  UINT8  *Source;
  UINT8  *Destination;

  //
  // Program the Mode Register Write mode 0, Read mode 0
  //
  WriteGraphicsController (
    PciIo,
    VGA_GRAPHICS_CONTROLLER_MODE_REGISTER,
    VGA_GRAPHICS_CONTROLLER_READ_MODE_0 | VGA_GRAPHICS_CONTROLLER_WRITE_MODE_0
    );

  for (BitPlane = 0, FrameBufferOffset = 0;
       BitPlane < VGA_NUMBER_OF_BIT_PLANES;
       BitPlane++, FrameBufferOffset += VGA_BYTES_PER_BIT_PLANE
       )
  {
    //
    // Program the Read Map Select Register to select the correct bit plane
    //
    WriteGraphicsController (
      PciIo,
      VGA_GRAPHICS_CONTROLLER_READ_MAP_SELECT_REGISTER,
      BitPlane
      );

    Source      = HardwareBuffer;
    Destination = MemoryBuffer + FrameBufferOffset;

    for (Rows = 0; Rows < Height; Rows++, Source += VGA_BYTES_PER_SCAN_LINE, Destination += VGA_BYTES_PER_SCAN_LINE) {
      PciIo->Mem.Read (
                   PciIo,
                   EfiPciIoWidthUint8,
                   EFI_PCI_IO_PASS_THROUGH_BAR,
                   (UINT64)(UINTN)Source,
                   WidthInBytes,
                   (VOID *)Destination
                   );
    }
  }
}

/**
  Internal routine to convert VGA color to Grahpics Output color.

  @param  MemoryBuffer           Buffer containing VGA color
  @param  CoordinateX            The X coordinate of pixel on screen
  @param  CoordinateY            The Y coordinate of pixel on screen
  @param  BltBuffer              Buffer to contain converted Grahpics Output color

  @return None

**/
VOID
VgaConvertToGraphicsOutputColor (
  UINT8                          *MemoryBuffer,
  UINTN                          CoordinateX,
  UINTN                          CoordinateY,
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL  *BltBuffer
  )
{
  UINTN  Mask;
  UINTN  Bit;
  UINTN  Color;

  MemoryBuffer += ((CoordinateY << 6) + (CoordinateY << 4) + (CoordinateX >> 3));
  Mask          = mVgaBitMaskTable[CoordinateX & 0x07];
  for (Bit = 0x01, Color = 0; Bit < 0x10; Bit <<= 1, MemoryBuffer += VGA_BYTES_PER_BIT_PLANE) {
    if ((*MemoryBuffer & Mask) != 0) {
      Color |= Bit;
    }
  }

  *BltBuffer = mVgaColorToGraphicsOutputColor[Color];
}

/**
  Internal routine to convert Grahpics Output color to VGA color.

  @param  BltBuffer              buffer containing Grahpics Output color

  @return Converted VGA color

**/
UINT8
VgaConvertColor (
  IN  EFI_GRAPHICS_OUTPUT_BLT_PIXEL  *BltBuffer
  )
{
  UINT8  Color;

  Color = (UINT8)((BltBuffer->Blue >> 7) | ((BltBuffer->Green >> 6) & 0x02) | ((BltBuffer->Red >> 5) & 0x04));
  if ((BltBuffer->Red + BltBuffer->Green + BltBuffer->Blue) > 0x180) {
    Color |= 0x08;
  }

  return Color;
}

/**
  Grahpics Output protocol instance to block transfer for VGA device.

  @param  This                   Pointer to Grahpics Output protocol instance
  @param  BltBuffer              The data to transfer to screen
  @param  BltOperation           The operation to perform
  @param  SourceX                The X coordinate of the source for BltOperation
  @param  SourceY                The Y coordinate of the source for BltOperation
  @param  DestinationX           The X coordinate of the destination for
                                 BltOperation
  @param  DestinationY           The Y coordinate of the destination for
                                 BltOperation
  @param  Width                  The width of a rectangle in the blt rectangle in
                                 pixels
  @param  Height                 The height of a rectangle in the blt rectangle in
                                 pixels
  @param  Delta                  Not used for EfiBltVideoFill and
                                 EfiBltVideoToVideo operation. If a Delta of 0 is
                                 used, the entire BltBuffer will be operated on. If
                                 a subrectangle of the BltBuffer is used, then
                                 Delta represents the number of bytes in a row of
                                 the BltBuffer.

  @retval EFI_INVALID_PARAMETER  Invalid parameter passed in
  @retval EFI_SUCCESS            Blt operation success

**/
EFI_STATUS
EFIAPI
BiosVideoGraphicsOutputVgaBlt (
  IN  EFI_GRAPHICS_OUTPUT_PROTOCOL       *This,
  IN  EFI_GRAPHICS_OUTPUT_BLT_PIXEL      *BltBuffer  OPTIONAL,
  IN  EFI_GRAPHICS_OUTPUT_BLT_OPERATION  BltOperation,
  IN  UINTN                              SourceX,
  IN  UINTN                              SourceY,
  IN  UINTN                              DestinationX,
  IN  UINTN                              DestinationY,
  IN  UINTN                              Width,
  IN  UINTN                              Height,
  IN  UINTN                              Delta
  )
{
  BIOS_VIDEO_DEV       *BiosVideoPrivate;
  EFI_TPL              OriginalTPL;
  UINT8                *MemAddress;
  UINTN                BytesPerScanLine;
  UINTN                Bit;
  UINTN                Index;
  UINTN                Index1;
  UINTN                StartAddress;
  UINTN                Bytes;
  UINTN                Offset;
  UINT8                LeftMask;
  UINT8                RightMask;
  UINTN                Address;
  UINTN                AddressFix;
  UINT8                *Address1;
  UINT8                *SourceAddress;
  UINT8                *DestinationAddress;
  EFI_PCI_IO_PROTOCOL  *PciIo;
  UINT8                Data;
  UINT8                PixelColor;
  UINT8                *VgaFrameBuffer;
  UINTN                SourceOffset;
  UINTN                SourceWidth;
  UINTN                Rows;
  UINTN                Columns;
  UINTN                CoordinateX;
  UINTN                CoordinateY;
  UINTN                CurrentMode;

  if ((This == NULL) || (((UINTN)BltOperation) >= EfiGraphicsOutputBltOperationMax)) {
    return EFI_INVALID_PARAMETER;
  }

  BiosVideoPrivate = BIOS_VIDEO_DEV_FROM_GRAPHICS_OUTPUT_THIS (This);

  CurrentMode      = This->Mode->Mode;
  PciIo            = BiosVideoPrivate->PciIo;
  MemAddress       = BiosVideoPrivate->ModeData[CurrentMode].LinearFrameBuffer;
  BytesPerScanLine = BiosVideoPrivate->ModeData[CurrentMode].BytesPerScanLine >> 3;
  VgaFrameBuffer   = BiosVideoPrivate->VgaFrameBuffer;

  if ((Width == 0) || (Height == 0)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // We need to fill the Virtual Screen buffer with the blt data.
  // The virtual screen is upside down, as the first row is the bootom row of
  // the image.
  //
  if (BltOperation == EfiBltVideoToBltBuffer) {
    //
    // Video to BltBuffer: Source is Video, destination is BltBuffer
    //
    if (SourceY + Height > BiosVideoPrivate->ModeData[CurrentMode].VerticalResolution) {
      return EFI_INVALID_PARAMETER;
    }

    if (SourceX + Width > BiosVideoPrivate->ModeData[CurrentMode].HorizontalResolution) {
      return EFI_INVALID_PARAMETER;
    }
  } else {
    //
    // BltBuffer to Video: Source is BltBuffer, destination is Video
    //
    if (DestinationY + Height > BiosVideoPrivate->ModeData[CurrentMode].VerticalResolution) {
      return EFI_INVALID_PARAMETER;
    }

    if (DestinationX + Width > BiosVideoPrivate->ModeData[CurrentMode].HorizontalResolution) {
      return EFI_INVALID_PARAMETER;
    }
  }

  //
  // If Delta is zero, then the entire BltBuffer is being used, so Delta
  // is the number of bytes in each row of BltBuffer.  Since BltBuffer is Width pixels size,
  // the number of bytes in each row can be computed.
  //
  if (Delta == 0) {
    Delta = Width * sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL);
  }

  //
  // We have to raise to TPL Notify, so we make an atomic write the frame buffer.
  // We would not want a timer based event (Cursor, ...) to come in while we are
  // doing this operation.
  //
  OriginalTPL = gBS->RaiseTPL (TPL_NOTIFY);

  //
  // Compute some values we need for VGA
  //
  switch (BltOperation) {
    case EfiBltVideoToBltBuffer:

      SourceOffset = (SourceY << 6) + (SourceY << 4) + (SourceX >> 3);
      SourceWidth  = ((SourceX + Width - 1) >> 3) - (SourceX >> 3) + 1;

      //
      // Read all the pixels in the 4 bit planes into a memory buffer that looks like the VGA buffer
      //
      VgaReadBitPlanes (
        PciIo,
        MemAddress + SourceOffset,
        VgaFrameBuffer + SourceOffset,
        SourceWidth,
        Height
        );

      //
      // Convert VGA Bit Planes to a Graphics Output 32-bit color value
      //
      BltBuffer += (DestinationY * (Delta >> 2) + DestinationX);
      for (Rows = 0, CoordinateY = SourceY; Rows < Height; Rows++, CoordinateY++, BltBuffer += (Delta >> 2)) {
        for (Columns = 0, CoordinateX = SourceX; Columns < Width; Columns++, CoordinateX++, BltBuffer++) {
          VgaConvertToGraphicsOutputColor (VgaFrameBuffer, CoordinateX, CoordinateY, BltBuffer);
        }

        BltBuffer -= Width;
      }

      break;

    case EfiBltVideoToVideo:
      //
      // Check for an aligned Video to Video operation
      //
      if (((SourceX & 0x07) == 0x00) && ((DestinationX & 0x07) == 0x00) && ((Width & 0x07) == 0x00)) {
        //
        // Program the Mode Register Write mode 1, Read mode 0
        //
        WriteGraphicsController (
          PciIo,
          VGA_GRAPHICS_CONTROLLER_MODE_REGISTER,
          VGA_GRAPHICS_CONTROLLER_READ_MODE_0 | VGA_GRAPHICS_CONTROLLER_WRITE_MODE_1
          );

        SourceAddress      = (UINT8 *)(MemAddress + (SourceY << 6) + (SourceY << 4) + (SourceX >> 3));
        DestinationAddress = (UINT8 *)(MemAddress + (DestinationY << 6) + (DestinationY << 4) + (DestinationX >> 3));
        Bytes              = Width >> 3;
        for (Index = 0, Offset = 0; Index < Height; Index++, Offset += BytesPerScanLine) {
          PciIo->CopyMem (
                   PciIo,
                   EfiPciIoWidthUint8,
                   EFI_PCI_IO_PASS_THROUGH_BAR,
                   (UINT64)(UINTN)(DestinationAddress + Offset),
                   EFI_PCI_IO_PASS_THROUGH_BAR,
                   (UINT64)(UINTN)(SourceAddress + Offset),
                   Bytes
                   );
        }
      } else {
        SourceOffset = (SourceY << 6) + (SourceY << 4) + (SourceX >> 3);
        SourceWidth  = ((SourceX + Width - 1) >> 3) - (SourceX >> 3) + 1;

        //
        // Read all the pixels in the 4 bit planes into a memory buffer that looks like the VGA buffer
        //
        VgaReadBitPlanes (
          PciIo,
          MemAddress + SourceOffset,
          VgaFrameBuffer + SourceOffset,
          SourceWidth,
          Height
          );
      }

      break;

    case EfiBltVideoFill:
      StartAddress = (UINTN)(MemAddress + (DestinationY << 6) + (DestinationY << 4) + (DestinationX >> 3));
      Bytes        = ((DestinationX + Width - 1) >> 3) - (DestinationX >> 3);
      LeftMask     = mVgaLeftMaskTable[DestinationX & 0x07];
      RightMask    = mVgaRightMaskTable[(DestinationX + Width - 1) & 0x07];
      if (Bytes == 0) {
        LeftMask  = (UINT8)(LeftMask & RightMask);
        RightMask = 0;
      }

      if (LeftMask == 0xff) {
        StartAddress--;
        Bytes++;
        LeftMask = 0;
      }

      if (RightMask == 0xff) {
        Bytes++;
        RightMask = 0;
      }

      PixelColor = VgaConvertColor (BltBuffer);

      //
      // Program the Mode Register Write mode 2, Read mode 0
      //
      WriteGraphicsController (
        PciIo,
        VGA_GRAPHICS_CONTROLLER_MODE_REGISTER,
        VGA_GRAPHICS_CONTROLLER_READ_MODE_0 | VGA_GRAPHICS_CONTROLLER_WRITE_MODE_2
        );

      //
      // Program the Data Rotate/Function Select Register to replace
      //
      WriteGraphicsController (
        PciIo,
        VGA_GRAPHICS_CONTROLLER_DATA_ROTATE_REGISTER,
        VGA_GRAPHICS_CONTROLLER_FUNCTION_REPLACE
        );

      if (LeftMask != 0) {
        //
        // Program the BitMask register with the Left column mask
        //
        WriteGraphicsController (
          PciIo,
          VGA_GRAPHICS_CONTROLLER_BIT_MASK_REGISTER,
          LeftMask
          );

        for (Index = 0, Address = StartAddress; Index < Height; Index++, Address += BytesPerScanLine) {
          //
          // Read data from the bit planes into the latches
          //
          PciIo->Mem.Read (
                       PciIo,
                       EfiPciIoWidthUint8,
                       EFI_PCI_IO_PASS_THROUGH_BAR,
                       (UINT64)(UINTN)Address,
                       1,
                       &Data
                       );
          //
          // Write the lower 4 bits of PixelColor to the bit planes in the pixels enabled by BitMask
          //
          PciIo->Mem.Write (
                       PciIo,
                       EfiPciIoWidthUint8,
                       EFI_PCI_IO_PASS_THROUGH_BAR,
                       (UINT64)(UINTN)Address,
                       1,
                       &PixelColor
                       );
        }
      }

      if (Bytes > 1) {
        //
        // Program the BitMask register with the middle column mask of 0xff
        //
        WriteGraphicsController (
          PciIo,
          VGA_GRAPHICS_CONTROLLER_BIT_MASK_REGISTER,
          0xff
          );

        for (Index = 0, Address = StartAddress + 1; Index < Height; Index++, Address += BytesPerScanLine) {
          PciIo->Mem.Write (
                       PciIo,
                       EfiPciIoWidthFillUint8,
                       EFI_PCI_IO_PASS_THROUGH_BAR,
                       (UINT64)(UINTN)Address,
                       Bytes - 1,
                       &PixelColor
                       );
        }
      }

      if (RightMask != 0) {
        //
        // Program the BitMask register with the Right column mask
        //
        WriteGraphicsController (
          PciIo,
          VGA_GRAPHICS_CONTROLLER_BIT_MASK_REGISTER,
          RightMask
          );

        for (Index = 0, Address = StartAddress + Bytes; Index < Height; Index++, Address += BytesPerScanLine) {
          //
          // Read data from the bit planes into the latches
          //
          PciIo->Mem.Read (
                       PciIo,
                       EfiPciIoWidthUint8,
                       EFI_PCI_IO_PASS_THROUGH_BAR,
                       (UINT64)(UINTN)Address,
                       1,
                       &Data
                       );
          //
          // Write the lower 4 bits of PixelColor to the bit planes in the pixels enabled by BitMask
          //
          PciIo->Mem.Write (
                       PciIo,
                       EfiPciIoWidthUint8,
                       EFI_PCI_IO_PASS_THROUGH_BAR,
                       (UINT64)(UINTN)Address,
                       1,
                       &PixelColor
                       );
        }
      }

      break;

    case EfiBltBufferToVideo:
      StartAddress = (UINTN)(MemAddress + (DestinationY << 6) + (DestinationY << 4) + (DestinationX >> 3));
      LeftMask     = mVgaBitMaskTable[DestinationX & 0x07];

      //
      // Program the Mode Register Write mode 2, Read mode 0
      //
      WriteGraphicsController (
        PciIo,
        VGA_GRAPHICS_CONTROLLER_MODE_REGISTER,
        VGA_GRAPHICS_CONTROLLER_READ_MODE_0 | VGA_GRAPHICS_CONTROLLER_WRITE_MODE_2
        );

      //
      // Program the Data Rotate/Function Select Register to replace
      //
      WriteGraphicsController (
        PciIo,
        VGA_GRAPHICS_CONTROLLER_DATA_ROTATE_REGISTER,
        VGA_GRAPHICS_CONTROLLER_FUNCTION_REPLACE
        );

      for (Index = 0, Address = StartAddress; Index < Height; Index++, Address += BytesPerScanLine) {
        for (Index1 = 0; Index1 < Width; Index1++) {
          BiosVideoPrivate->LineBuffer[Index1] = VgaConvertColor (&BltBuffer[(SourceY + Index) * (Delta >> 2) + SourceX + Index1]);
        }

        AddressFix = Address;

        for (Bit = 0; Bit < 8; Bit++) {
          //
          // Program the BitMask register with the Left column mask
          //
          WriteGraphicsController (
            PciIo,
            VGA_GRAPHICS_CONTROLLER_BIT_MASK_REGISTER,
            LeftMask
            );

          for (Index1 = Bit, Address1 = (UINT8 *)AddressFix; Index1 < Width; Index1 += 8, Address1++) {
            //
            // Read data from the bit planes into the latches
            //
            PciIo->Mem.Read (
                         PciIo,
                         EfiPciIoWidthUint8,
                         EFI_PCI_IO_PASS_THROUGH_BAR,
                         (UINT64)(UINTN)Address1,
                         1,
                         &Data
                         );

            PciIo->Mem.Write (
                         PciIo,
                         EfiPciIoWidthUint8,
                         EFI_PCI_IO_PASS_THROUGH_BAR,
                         (UINT64)(UINTN)Address1,
                         1,
                         &BiosVideoPrivate->LineBuffer[Index1]
                         );
          }

          LeftMask = (UINT8)(LeftMask >> 1);
          if (LeftMask == 0) {
            LeftMask = 0x80;
            AddressFix++;
          }
        }
      }

      break;

    default:;
  }

  gBS->RestoreTPL (OriginalTPL);

  return EFI_SUCCESS;
}

//
// VGA Mini Port Protocol Functions
//

/**
  VgaMiniPort protocol interface to set mode.

  @param  This                   Pointer to VgaMiniPort protocol instance
  @param  ModeNumber             The index of the mode

  @retval EFI_UNSUPPORTED        The requested mode is not supported
  @retval EFI_SUCCESS            The requested mode is set successfully

**/
EFI_STATUS
EFIAPI
BiosVideoVgaMiniPortSetMode (
  IN  EFI_VGA_MINI_PORT_PROTOCOL  *This,
  IN  UINTN                       ModeNumber
  )
{
  BIOS_VIDEO_DEV         *BiosVideoPrivate;
  EFI_IA32_REGISTER_SET  Regs;

  if (This == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Make sure the ModeNumber is a valid value
  //
  if (ModeNumber >= This->MaxMode) {
    return EFI_UNSUPPORTED;
  }

  //
  // Get the device structure for this device
  //
  BiosVideoPrivate = BIOS_VIDEO_DEV_FROM_VGA_MINI_PORT_THIS (This);

  switch (ModeNumber) {
    case 0:
      //
      // Set the 80x25 Text VGA Mode
      //
      Regs.H.AH = 0x00;
      Regs.H.AL = 0x83;
      BiosVideoPrivate->LegacyBios->Int86 (BiosVideoPrivate->LegacyBios, 0x10, &Regs);

      Regs.H.AH = 0x11;
      Regs.H.AL = 0x14;
      Regs.H.BL = 0;
      BiosVideoPrivate->LegacyBios->Int86 (BiosVideoPrivate->LegacyBios, 0x10, &Regs);
      break;

    case 1:
      //
      // Set the 80x50 Text VGA Mode
      //
      Regs.H.AH = 0x00;
      Regs.H.AL = 0x83;
      BiosVideoPrivate->LegacyBios->Int86 (BiosVideoPrivate->LegacyBios, 0x10, &Regs);
      Regs.H.AH = 0x11;
      Regs.H.AL = 0x12;
      Regs.H.BL = 0;
      BiosVideoPrivate->LegacyBios->Int86 (BiosVideoPrivate->LegacyBios, 0x10, &Regs);
      break;

    default:
      return EFI_UNSUPPORTED;
  }

  return EFI_SUCCESS;
}

/**
  Event handler for Exit Boot Service.

  @param  Event       The event that be signalled when exiting boot service.
  @param  Context     Pointer to instance of BIOS_VIDEO_DEV.

**/
VOID
EFIAPI
BiosVideoNotifyExitBootServices (
  IN  EFI_EVENT  Event,
  IN  VOID       *Context
  )
{
  BIOS_VIDEO_DEV         *BiosVideoPrivate;
  EFI_IA32_REGISTER_SET  Regs;

  BiosVideoPrivate = (BIOS_VIDEO_DEV *)Context;

  //
  // Set the 80x25 Text VGA Mode
  //
  Regs.H.AH = 0x00;
  Regs.H.AL = 0x03;
  BiosVideoPrivate->LegacyBios->Int86 (BiosVideoPrivate->LegacyBios, 0x10, &Regs);

  Regs.H.AH = 0x00;
  Regs.H.AL = 0x83;
  BiosVideoPrivate->LegacyBios->Int86 (BiosVideoPrivate->LegacyBios, 0x10, &Regs);

  Regs.H.AH = 0x11;
  Regs.H.AL = 0x04;
  Regs.H.BL = 0;
  BiosVideoPrivate->LegacyBios->Int86 (BiosVideoPrivate->LegacyBios, 0x10, &Regs);
}

/**
  The user Entry Point for module UefiBiosVideo. The user code starts with this function.

  @param[in] ImageHandle    The firmware allocated handle for the EFI image.
  @param[in] SystemTable    A pointer to the EFI System Table.

  @retval EFI_SUCCESS       The entry point is executed successfully.
  @retval other             Some error occurs when executing this entry point.

**/
EFI_STATUS
EFIAPI
BiosVideoEntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  //
  // Install driver model protocol(s).
  //
  Status = EfiLibInstallDriverBindingComponentName2 (
             ImageHandle,
             SystemTable,
             &gBiosVideoDriverBinding,
             ImageHandle,
             &gBiosVideoComponentName,
             &gBiosVideoComponentName2
             );
  ASSERT_EFI_ERROR (Status);

  //
  // Install Legacy BIOS GUID to mark this driver as a BIOS Thunk Driver
  //
  return gBS->InstallMultipleProtocolInterfaces (
                &ImageHandle,
                &gEfiLegacyBiosGuid,
                NULL,
                NULL
                );
}
