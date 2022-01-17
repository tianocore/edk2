/** @file
  This driver is a sample implementation of the Graphics Output Protocol for
  the QEMU (Cirrus Logic 5446) video controller.

  Copyright (c) 2006 - 2019, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "Qemu.h"
#include <IndustryStandard/Acpi.h>

EFI_DRIVER_BINDING_PROTOCOL  gQemuVideoDriverBinding = {
  QemuVideoControllerDriverSupported,
  QemuVideoControllerDriverStart,
  QemuVideoControllerDriverStop,
  0x10,
  NULL,
  NULL
};

QEMU_VIDEO_CARD  gQemuVideoCardList[] = {
  {
    PCI_CLASS_DISPLAY_VGA,
    CIRRUS_LOGIC_VENDOR_ID,
    CIRRUS_LOGIC_5430_DEVICE_ID,
    QEMU_VIDEO_CIRRUS_5430,
    L"Cirrus 5430"
  },{
    PCI_CLASS_DISPLAY_VGA,
    CIRRUS_LOGIC_VENDOR_ID,
    CIRRUS_LOGIC_5430_ALTERNATE_DEVICE_ID,
    QEMU_VIDEO_CIRRUS_5430,
    L"Cirrus 5430"
  },{
    PCI_CLASS_DISPLAY_VGA,
    CIRRUS_LOGIC_VENDOR_ID,
    CIRRUS_LOGIC_5446_DEVICE_ID,
    QEMU_VIDEO_CIRRUS_5446,
    L"Cirrus 5446"
  },{
    PCI_CLASS_DISPLAY_VGA,
    0x1234,
    0x1111,
    QEMU_VIDEO_BOCHS_MMIO,
    L"QEMU Standard VGA"
  },{
    PCI_CLASS_DISPLAY_OTHER,
    0x1234,
    0x1111,
    QEMU_VIDEO_BOCHS_MMIO,
    L"QEMU Standard VGA (secondary)"
  },{
    PCI_CLASS_DISPLAY_VGA,
    0x1b36,
    0x0100,
    QEMU_VIDEO_BOCHS,
    L"QEMU QXL VGA"
  },{
    PCI_CLASS_DISPLAY_VGA,
    0x1af4,
    0x1050,
    QEMU_VIDEO_BOCHS_MMIO,
    L"QEMU VirtIO VGA"
  },{
    PCI_CLASS_DISPLAY_VGA,
    0x15ad,
    0x0405,
    QEMU_VIDEO_VMWARE_SVGA,
    L"QEMU VMWare SVGA"
  },{
    0     /* end of list */
  }
};

static QEMU_VIDEO_CARD *
QemuVideoDetect (
  IN UINT8   SubClass,
  IN UINT16  VendorId,
  IN UINT16  DeviceId
  )
{
  UINTN  Index = 0;

  while (gQemuVideoCardList[Index].VendorId != 0) {
    if ((gQemuVideoCardList[Index].SubClass == SubClass) &&
        (gQemuVideoCardList[Index].VendorId == VendorId) &&
        (gQemuVideoCardList[Index].DeviceId == DeviceId))
    {
      return gQemuVideoCardList + Index;
    }

    Index++;
  }

  return NULL;
}

/**
  Check if this device is supported.

  @param  This                   The driver binding protocol.
  @param  Controller             The controller handle to check.
  @param  RemainingDevicePath    The remaining device path.

  @retval EFI_SUCCESS            The bus supports this controller.
  @retval EFI_UNSUPPORTED        This device isn't supported.

**/
EFI_STATUS
EFIAPI
QemuVideoControllerDriverSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   Controller,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  )
{
  EFI_STATUS           Status;
  EFI_PCI_IO_PROTOCOL  *PciIo;
  PCI_TYPE00           Pci;
  QEMU_VIDEO_CARD      *Card;

  //
  // Open the PCI I/O Protocol
  //
  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiPciIoProtocolGuid,
                  (VOID **)&PciIo,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Read the PCI Configuration Header from the PCI Device
  //
  Status = PciIo->Pci.Read (
                        PciIo,
                        EfiPciIoWidthUint32,
                        0,
                        sizeof (Pci) / sizeof (UINT32),
                        &Pci
                        );
  if (EFI_ERROR (Status)) {
    goto Done;
  }

  Status = EFI_UNSUPPORTED;
  if (!IS_PCI_DISPLAY (&Pci)) {
    goto Done;
  }

  Card = QemuVideoDetect (Pci.Hdr.ClassCode[1], Pci.Hdr.VendorId, Pci.Hdr.DeviceId);
  if (Card != NULL) {
    DEBUG ((DEBUG_INFO, "QemuVideo: %s detected\n", Card->Name));
    Status = EFI_SUCCESS;
  }

Done:
  //
  // Close the PCI I/O Protocol
  //
  gBS->CloseProtocol (
         Controller,
         &gEfiPciIoProtocolGuid,
         This->DriverBindingHandle,
         Controller
         );

  return Status;
}

/**
  Start to process the controller.

  @param  This                   The USB bus driver binding instance.
  @param  Controller             The controller to check.
  @param  RemainingDevicePath    The remaining device patch.

  @retval EFI_SUCCESS            The controller is controlled by the usb bus.
  @retval EFI_ALREADY_STARTED    The controller is already controlled by the usb
                                 bus.
  @retval EFI_OUT_OF_RESOURCES   Failed to allocate resources.

**/
EFI_STATUS
EFIAPI
QemuVideoControllerDriverStart (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   Controller,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  )
{
  EFI_TPL                   OldTpl;
  EFI_STATUS                Status;
  QEMU_VIDEO_PRIVATE_DATA   *Private;
  BOOLEAN                   IsQxl;
  EFI_DEVICE_PATH_PROTOCOL  *ParentDevicePath;
  ACPI_ADR_DEVICE_PATH      AcpiDeviceNode;
  PCI_TYPE00                Pci;
  QEMU_VIDEO_CARD           *Card;
  EFI_PCI_IO_PROTOCOL       *ChildPciIo;
  UINT64                    SupportedVgaIo;

  OldTpl = gBS->RaiseTPL (TPL_CALLBACK);

  //
  // Allocate Private context data for GOP interface.
  //
  Private = AllocateZeroPool (sizeof (QEMU_VIDEO_PRIVATE_DATA));
  if (Private == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto RestoreTpl;
  }

  //
  // Set up context record
  //
  Private->Signature = QEMU_VIDEO_PRIVATE_DATA_SIGNATURE;

  //
  // Open PCI I/O Protocol
  //
  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiPciIoProtocolGuid,
                  (VOID **)&Private->PciIo,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (EFI_ERROR (Status)) {
    goto FreePrivate;
  }

  //
  // Read the PCI Configuration Header from the PCI Device
  //
  Status = Private->PciIo->Pci.Read (
                                 Private->PciIo,
                                 EfiPciIoWidthUint32,
                                 0,
                                 sizeof (Pci) / sizeof (UINT32),
                                 &Pci
                                 );
  if (EFI_ERROR (Status)) {
    goto ClosePciIo;
  }

  //
  // Determine card variant.
  //
  Card = QemuVideoDetect (Pci.Hdr.ClassCode[1], Pci.Hdr.VendorId, Pci.Hdr.DeviceId);
  if (Card == NULL) {
    Status = EFI_DEVICE_ERROR;
    goto ClosePciIo;
  }

  Private->Variant = Card->Variant;

  //
  // IsQxl is based on the detected Card->Variant, which at a later point might
  // not match Private->Variant.
  //
  IsQxl = (BOOLEAN)(Card->Variant == QEMU_VIDEO_BOCHS);

  //
  // Save original PCI attributes
  //
  Status = Private->PciIo->Attributes (
                             Private->PciIo,
                             EfiPciIoAttributeOperationGet,
                             0,
                             &Private->OriginalPciAttributes
                             );

  if (EFI_ERROR (Status)) {
    goto ClosePciIo;
  }

  //
  // Get supported PCI attributes
  //
  Status = Private->PciIo->Attributes (
                             Private->PciIo,
                             EfiPciIoAttributeOperationSupported,
                             0,
                             &SupportedVgaIo
                             );
  if (EFI_ERROR (Status)) {
    goto ClosePciIo;
  }

  SupportedVgaIo &= (UINT64)(EFI_PCI_IO_ATTRIBUTE_VGA_IO | EFI_PCI_IO_ATTRIBUTE_VGA_IO_16);
  if ((SupportedVgaIo == 0) && IS_PCI_VGA (&Pci)) {
    Status = EFI_UNSUPPORTED;
    goto ClosePciIo;
  }

  //
  // Set new PCI attributes
  //
  Status = Private->PciIo->Attributes (
                             Private->PciIo,
                             EfiPciIoAttributeOperationEnable,
                             EFI_PCI_DEVICE_ENABLE | EFI_PCI_IO_ATTRIBUTE_VGA_MEMORY | SupportedVgaIo,
                             NULL
                             );
  if (EFI_ERROR (Status)) {
    goto ClosePciIo;
  }

  //
  // Check whenever the qemu stdvga mmio bar is present (qemu 1.3+).
  //
  if (Private->Variant == QEMU_VIDEO_BOCHS_MMIO) {
    EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR  *MmioDesc;

    Status = Private->PciIo->GetBarAttributes (
                               Private->PciIo,
                               PCI_BAR_IDX2,
                               NULL,
                               (VOID **)&MmioDesc
                               );
    if (EFI_ERROR (Status) ||
        (MmioDesc->ResType != ACPI_ADDRESS_SPACE_TYPE_MEM))
    {
      DEBUG ((DEBUG_INFO, "QemuVideo: No mmio bar, fallback to port io\n"));
      Private->Variant = QEMU_VIDEO_BOCHS;
    } else {
      DEBUG ((
        DEBUG_INFO,
        "QemuVideo: Using mmio bar @ 0x%lx\n",
        MmioDesc->AddrRangeMin
        ));
    }

    if (!EFI_ERROR (Status)) {
      FreePool (MmioDesc);
    }
  }

  //
  // VMWare SVGA is handled like Bochs (with port IO only).
  //
  if (Private->Variant == QEMU_VIDEO_VMWARE_SVGA) {
    Private->Variant                 = QEMU_VIDEO_BOCHS;
    Private->FrameBufferVramBarIndex = PCI_BAR_IDX1;
  }

  //
  // Check if accessing the bochs interface works.
  //
  if ((Private->Variant == QEMU_VIDEO_BOCHS_MMIO) ||
      (Private->Variant == QEMU_VIDEO_BOCHS))
  {
    UINT16  BochsId;
    BochsId = BochsRead (Private, VBE_DISPI_INDEX_ID);
    if ((BochsId & 0xFFF0) != VBE_DISPI_ID0) {
      DEBUG ((DEBUG_INFO, "QemuVideo: BochsID mismatch (got 0x%x)\n", BochsId));
      Status = EFI_DEVICE_ERROR;
      goto RestoreAttributes;
    }
  }

  //
  // Get ParentDevicePath
  //
  Status = gBS->HandleProtocol (
                  Controller,
                  &gEfiDevicePathProtocolGuid,
                  (VOID **)&ParentDevicePath
                  );
  if (EFI_ERROR (Status)) {
    goto RestoreAttributes;
  }

  //
  // Set Gop Device Path
  //
  ZeroMem (&AcpiDeviceNode, sizeof (ACPI_ADR_DEVICE_PATH));
  AcpiDeviceNode.Header.Type    = ACPI_DEVICE_PATH;
  AcpiDeviceNode.Header.SubType = ACPI_ADR_DP;
  AcpiDeviceNode.ADR            = ACPI_DISPLAY_ADR (1, 0, 0, 1, 0, ACPI_ADR_DISPLAY_TYPE_VGA, 0, 0);
  SetDevicePathNodeLength (&AcpiDeviceNode.Header, sizeof (ACPI_ADR_DEVICE_PATH));

  Private->GopDevicePath = AppendDevicePathNode (
                             ParentDevicePath,
                             (EFI_DEVICE_PATH_PROTOCOL *)&AcpiDeviceNode
                             );
  if (Private->GopDevicePath == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto RestoreAttributes;
  }

  //
  // Create new child handle and install the device path protocol on it.
  //
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &Private->Handle,
                  &gEfiDevicePathProtocolGuid,
                  Private->GopDevicePath,
                  NULL
                  );
  if (EFI_ERROR (Status)) {
    goto FreeGopDevicePath;
  }

  //
  // Construct video mode buffer
  //
  switch (Private->Variant) {
    case QEMU_VIDEO_CIRRUS_5430:
    case QEMU_VIDEO_CIRRUS_5446:
      Status = QemuVideoCirrusModeSetup (Private);
      break;
    case QEMU_VIDEO_BOCHS_MMIO:
    case QEMU_VIDEO_BOCHS:
      Status = QemuVideoBochsModeSetup (Private, IsQxl);
      break;
    default:
      ASSERT (FALSE);
      Status = EFI_DEVICE_ERROR;
      break;
  }

  if (EFI_ERROR (Status)) {
    goto UninstallGopDevicePath;
  }

  //
  // Start the GOP software stack.
  //
  Status = QemuVideoGraphicsOutputConstructor (Private);
  if (EFI_ERROR (Status)) {
    goto FreeModeData;
  }

  Status = gBS->InstallMultipleProtocolInterfaces (
                  &Private->Handle,
                  &gEfiGraphicsOutputProtocolGuid,
                  &Private->GraphicsOutput,
                  NULL
                  );
  if (EFI_ERROR (Status)) {
    goto DestructQemuVideoGraphics;
  }

  //
  // Reference parent handle from child handle.
  //
  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiPciIoProtocolGuid,
                  (VOID **)&ChildPciIo,
                  This->DriverBindingHandle,
                  Private->Handle,
                  EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER
                  );
  if (EFI_ERROR (Status)) {
    goto UninstallGop;
  }

 #if defined MDE_CPU_IA32 || defined MDE_CPU_X64
  if ((Private->Variant == QEMU_VIDEO_BOCHS_MMIO) ||
      (Private->Variant == QEMU_VIDEO_BOCHS))
  {
    InstallVbeShim (Card->Name, Private->GraphicsOutput.Mode->FrameBufferBase);
  }

 #endif

  gBS->RestoreTPL (OldTpl);
  return EFI_SUCCESS;

UninstallGop:
  gBS->UninstallProtocolInterface (
         Private->Handle,
         &gEfiGraphicsOutputProtocolGuid,
         &Private->GraphicsOutput
         );

DestructQemuVideoGraphics:
  QemuVideoGraphicsOutputDestructor (Private);

FreeModeData:
  FreePool (Private->ModeData);

UninstallGopDevicePath:
  gBS->UninstallProtocolInterface (
         Private->Handle,
         &gEfiDevicePathProtocolGuid,
         Private->GopDevicePath
         );

FreeGopDevicePath:
  FreePool (Private->GopDevicePath);

RestoreAttributes:
  Private->PciIo->Attributes (
                    Private->PciIo,
                    EfiPciIoAttributeOperationSet,
                    Private->OriginalPciAttributes,
                    NULL
                    );

ClosePciIo:
  gBS->CloseProtocol (
         Controller,
         &gEfiPciIoProtocolGuid,
         This->DriverBindingHandle,
         Controller
         );

FreePrivate:
  FreePool (Private);

RestoreTpl:
  gBS->RestoreTPL (OldTpl);

  return Status;
}

/**
  Stop this device

  @param  This                   The USB bus driver binding protocol.
  @param  Controller             The controller to release.
  @param  NumberOfChildren       The number of children of this device that
                                 opened the controller BY_CHILD.
  @param  ChildHandleBuffer      The array of child handle.

  @retval EFI_SUCCESS            The controller or children are stopped.
  @retval EFI_DEVICE_ERROR       Failed to stop the driver.

**/
EFI_STATUS
EFIAPI
QemuVideoControllerDriverStop (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   Controller,
  IN UINTN                        NumberOfChildren,
  IN EFI_HANDLE                   *ChildHandleBuffer
  )
{
  EFI_GRAPHICS_OUTPUT_PROTOCOL  *GraphicsOutput;

  EFI_STATUS               Status;
  QEMU_VIDEO_PRIVATE_DATA  *Private;

  if (NumberOfChildren == 0) {
    //
    // Close the PCI I/O Protocol
    //
    gBS->CloseProtocol (
           Controller,
           &gEfiPciIoProtocolGuid,
           This->DriverBindingHandle,
           Controller
           );
    return EFI_SUCCESS;
  }

  //
  // free all resources for whose access we need the child handle, because the
  // child handle is going away
  //
  ASSERT (NumberOfChildren == 1);
  Status = gBS->OpenProtocol (
                  ChildHandleBuffer[0],
                  &gEfiGraphicsOutputProtocolGuid,
                  (VOID **)&GraphicsOutput,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Get our private context information
  //
  Private = QEMU_VIDEO_PRIVATE_DATA_FROM_GRAPHICS_OUTPUT_THIS (GraphicsOutput);
  ASSERT (Private->Handle == ChildHandleBuffer[0]);

  QemuVideoGraphicsOutputDestructor (Private);
  //
  // Remove the GOP protocol interface from the system
  //
  Status = gBS->UninstallMultipleProtocolInterfaces (
                  Private->Handle,
                  &gEfiGraphicsOutputProtocolGuid,
                  &Private->GraphicsOutput,
                  NULL
                  );

  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Restore original PCI attributes
  //
  Private->PciIo->Attributes (
                    Private->PciIo,
                    EfiPciIoAttributeOperationSet,
                    Private->OriginalPciAttributes,
                    NULL
                    );

  gBS->CloseProtocol (
         Controller,
         &gEfiPciIoProtocolGuid,
         This->DriverBindingHandle,
         Private->Handle
         );

  FreePool (Private->ModeData);
  gBS->UninstallProtocolInterface (
         Private->Handle,
         &gEfiDevicePathProtocolGuid,
         Private->GopDevicePath
         );
  FreePool (Private->GopDevicePath);

  //
  // Free our instance data
  //
  gBS->FreePool (Private);

  return EFI_SUCCESS;
}

/**
  TODO: Add function description

  @param  Private TODO: add argument description
  @param  Address TODO: add argument description
  @param  Data TODO: add argument description

  TODO: add return values

**/
VOID
outb (
  QEMU_VIDEO_PRIVATE_DATA  *Private,
  UINTN                    Address,
  UINT8                    Data
  )
{
  Private->PciIo->Io.Write (
                       Private->PciIo,
                       EfiPciIoWidthUint8,
                       EFI_PCI_IO_PASS_THROUGH_BAR,
                       Address,
                       1,
                       &Data
                       );
}

/**
  TODO: Add function description

  @param  Private TODO: add argument description
  @param  Address TODO: add argument description
  @param  Data TODO: add argument description

  TODO: add return values

**/
VOID
outw (
  QEMU_VIDEO_PRIVATE_DATA  *Private,
  UINTN                    Address,
  UINT16                   Data
  )
{
  Private->PciIo->Io.Write (
                       Private->PciIo,
                       EfiPciIoWidthUint16,
                       EFI_PCI_IO_PASS_THROUGH_BAR,
                       Address,
                       1,
                       &Data
                       );
}

/**
  TODO: Add function description

  @param  Private TODO: add argument description
  @param  Address TODO: add argument description

  TODO: add return values

**/
UINT8
inb (
  QEMU_VIDEO_PRIVATE_DATA  *Private,
  UINTN                    Address
  )
{
  UINT8  Data;

  Private->PciIo->Io.Read (
                       Private->PciIo,
                       EfiPciIoWidthUint8,
                       EFI_PCI_IO_PASS_THROUGH_BAR,
                       Address,
                       1,
                       &Data
                       );
  return Data;
}

/**
  TODO: Add function description

  @param  Private TODO: add argument description
  @param  Address TODO: add argument description

  TODO: add return values

**/
UINT16
inw (
  QEMU_VIDEO_PRIVATE_DATA  *Private,
  UINTN                    Address
  )
{
  UINT16  Data;

  Private->PciIo->Io.Read (
                       Private->PciIo,
                       EfiPciIoWidthUint16,
                       EFI_PCI_IO_PASS_THROUGH_BAR,
                       Address,
                       1,
                       &Data
                       );
  return Data;
}

/**
  TODO: Add function description

  @param  Private TODO: add argument description
  @param  Index TODO: add argument description
  @param  Red TODO: add argument description
  @param  Green TODO: add argument description
  @param  Blue TODO: add argument description

  TODO: add return values

**/
VOID
SetPaletteColor (
  QEMU_VIDEO_PRIVATE_DATA  *Private,
  UINTN                    Index,
  UINT8                    Red,
  UINT8                    Green,
  UINT8                    Blue
  )
{
  VgaOutb (Private, PALETTE_INDEX_REGISTER, (UINT8)Index);
  VgaOutb (Private, PALETTE_DATA_REGISTER, (UINT8)(Red >> 2));
  VgaOutb (Private, PALETTE_DATA_REGISTER, (UINT8)(Green >> 2));
  VgaOutb (Private, PALETTE_DATA_REGISTER, (UINT8)(Blue >> 2));
}

/**
  TODO: Add function description

  @param  Private TODO: add argument description

  TODO: add return values

**/
VOID
SetDefaultPalette (
  QEMU_VIDEO_PRIVATE_DATA  *Private
  )
{
  UINTN  Index;
  UINTN  RedIndex;
  UINTN  GreenIndex;
  UINTN  BlueIndex;

  Index = 0;
  for (RedIndex = 0; RedIndex < 8; RedIndex++) {
    for (GreenIndex = 0; GreenIndex < 8; GreenIndex++) {
      for (BlueIndex = 0; BlueIndex < 4; BlueIndex++) {
        SetPaletteColor (Private, Index, (UINT8)(RedIndex << 5), (UINT8)(GreenIndex << 5), (UINT8)(BlueIndex << 6));
        Index++;
      }
    }
  }
}

/**
  TODO: Add function description

  @param  Private TODO: add argument description

  TODO: add return values

**/
VOID
ClearScreen (
  QEMU_VIDEO_PRIVATE_DATA  *Private
  )
{
  UINT32  Color;

  Color = 0;
  Private->PciIo->Mem.Write (
                        Private->PciIo,
                        EfiPciIoWidthFillUint32,
                        Private->FrameBufferVramBarIndex,
                        0,
                        0x400000 >> 2,
                        &Color
                        );
}

/**
  TODO: Add function description

  @param  Private TODO: add argument description

  TODO: add return values

**/
VOID
DrawLogo (
  QEMU_VIDEO_PRIVATE_DATA  *Private,
  UINTN                    ScreenWidth,
  UINTN                    ScreenHeight
  )
{
}

/**
  TODO: Add function description

  @param  Private TODO: add argument description
  @param  ModeData TODO: add argument description

  TODO: add return values

**/
VOID
InitializeCirrusGraphicsMode (
  QEMU_VIDEO_PRIVATE_DATA  *Private,
  QEMU_VIDEO_CIRRUS_MODES  *ModeData
  )
{
  UINT8  Byte;
  UINTN  Index;

  outw (Private, SEQ_ADDRESS_REGISTER, 0x1206);
  outw (Private, SEQ_ADDRESS_REGISTER, 0x0012);

  for (Index = 0; Index < 15; Index++) {
    outw (Private, SEQ_ADDRESS_REGISTER, ModeData->SeqSettings[Index]);
  }

  if (Private->Variant == QEMU_VIDEO_CIRRUS_5430) {
    outb (Private, SEQ_ADDRESS_REGISTER, 0x0f);
    Byte = (UINT8)((inb (Private, SEQ_DATA_REGISTER) & 0xc7) ^ 0x30);
    outb (Private, SEQ_DATA_REGISTER, Byte);
  }

  outb (Private, MISC_OUTPUT_REGISTER, ModeData->MiscSetting);
  outw (Private, GRAPH_ADDRESS_REGISTER, 0x0506);
  outw (Private, SEQ_ADDRESS_REGISTER, 0x0300);
  outw (Private, CRTC_ADDRESS_REGISTER, 0x2011);

  for (Index = 0; Index < 28; Index++) {
    outw (Private, CRTC_ADDRESS_REGISTER, (UINT16)((ModeData->CrtcSettings[Index] << 8) | Index));
  }

  for (Index = 0; Index < 9; Index++) {
    outw (Private, GRAPH_ADDRESS_REGISTER, (UINT16)((GraphicsController[Index] << 8) | Index));
  }

  inb (Private, INPUT_STATUS_1_REGISTER);

  for (Index = 0; Index < 21; Index++) {
    outb (Private, ATT_ADDRESS_REGISTER, (UINT8)Index);
    outb (Private, ATT_ADDRESS_REGISTER, AttributeController[Index]);
  }

  outb (Private, ATT_ADDRESS_REGISTER, 0x20);

  outw (Private, GRAPH_ADDRESS_REGISTER, 0x0009);
  outw (Private, GRAPH_ADDRESS_REGISTER, 0x000a);
  outw (Private, GRAPH_ADDRESS_REGISTER, 0x000b);
  outb (Private, DAC_PIXEL_MASK_REGISTER, 0xff);

  SetDefaultPalette (Private);
  ClearScreen (Private);
}

VOID
BochsWrite (
  QEMU_VIDEO_PRIVATE_DATA  *Private,
  UINT16                   Reg,
  UINT16                   Data
  )
{
  EFI_STATUS  Status;

  if (Private->Variant == QEMU_VIDEO_BOCHS_MMIO) {
    Status = Private->PciIo->Mem.Write (
                                   Private->PciIo,
                                   EfiPciIoWidthUint16,
                                   PCI_BAR_IDX2,
                                   0x500 + (Reg << 1),
                                   1,
                                   &Data
                                   );
    ASSERT_EFI_ERROR (Status);
  } else {
    outw (Private, VBE_DISPI_IOPORT_INDEX, Reg);
    outw (Private, VBE_DISPI_IOPORT_DATA, Data);
  }
}

UINT16
BochsRead (
  QEMU_VIDEO_PRIVATE_DATA  *Private,
  UINT16                   Reg
  )
{
  EFI_STATUS  Status;
  UINT16      Data;

  if (Private->Variant == QEMU_VIDEO_BOCHS_MMIO) {
    Status = Private->PciIo->Mem.Read (
                                   Private->PciIo,
                                   EfiPciIoWidthUint16,
                                   PCI_BAR_IDX2,
                                   0x500 + (Reg << 1),
                                   1,
                                   &Data
                                   );
    ASSERT_EFI_ERROR (Status);
  } else {
    outw (Private, VBE_DISPI_IOPORT_INDEX, Reg);
    Data = inw (Private, VBE_DISPI_IOPORT_DATA);
  }

  return Data;
}

VOID
VgaOutb (
  QEMU_VIDEO_PRIVATE_DATA  *Private,
  UINTN                    Reg,
  UINT8                    Data
  )
{
  EFI_STATUS  Status;

  if (Private->Variant == QEMU_VIDEO_BOCHS_MMIO) {
    Status = Private->PciIo->Mem.Write (
                                   Private->PciIo,
                                   EfiPciIoWidthUint8,
                                   PCI_BAR_IDX2,
                                   0x400 - 0x3c0 + Reg,
                                   1,
                                   &Data
                                   );
    ASSERT_EFI_ERROR (Status);
  } else {
    outb (Private, Reg, Data);
  }
}

VOID
InitializeBochsGraphicsMode (
  QEMU_VIDEO_PRIVATE_DATA  *Private,
  QEMU_VIDEO_MODE_DATA     *ModeData
  )
{
  DEBUG ((
    DEBUG_INFO,
    "InitializeBochsGraphicsMode: %dx%d @ %d\n",
    ModeData->HorizontalResolution,
    ModeData->VerticalResolution,
    ModeData->ColorDepth
    ));

  /* unblank */
  VgaOutb (Private, ATT_ADDRESS_REGISTER, 0x20);

  BochsWrite (Private, VBE_DISPI_INDEX_ENABLE, 0);
  BochsWrite (Private, VBE_DISPI_INDEX_BANK, 0);
  BochsWrite (Private, VBE_DISPI_INDEX_X_OFFSET, 0);
  BochsWrite (Private, VBE_DISPI_INDEX_Y_OFFSET, 0);

  BochsWrite (Private, VBE_DISPI_INDEX_BPP, (UINT16)ModeData->ColorDepth);
  BochsWrite (Private, VBE_DISPI_INDEX_XRES, (UINT16)ModeData->HorizontalResolution);
  BochsWrite (Private, VBE_DISPI_INDEX_VIRT_WIDTH, (UINT16)ModeData->HorizontalResolution);
  BochsWrite (Private, VBE_DISPI_INDEX_YRES, (UINT16)ModeData->VerticalResolution);
  BochsWrite (Private, VBE_DISPI_INDEX_VIRT_HEIGHT, (UINT16)ModeData->VerticalResolution);

  BochsWrite (
    Private,
    VBE_DISPI_INDEX_ENABLE,
    VBE_DISPI_ENABLED | VBE_DISPI_LFB_ENABLED
    );

  SetDefaultPalette (Private);
  ClearScreen (Private);
}

EFI_STATUS
EFIAPI
InitializeQemuVideo (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  Status = EfiLibInstallDriverBindingComponentName2 (
             ImageHandle,
             SystemTable,
             &gQemuVideoDriverBinding,
             ImageHandle,
             &gQemuVideoComponentName,
             &gQemuVideoComponentName2
             );
  ASSERT_EFI_ERROR (Status);

  return Status;
}
