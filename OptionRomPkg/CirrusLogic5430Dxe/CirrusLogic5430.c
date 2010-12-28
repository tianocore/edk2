/** @file
  Cirrus Logic 5430 Controller Driver.
  This driver is a sample implementation of the UGA Draw and Graphics Output
  Protocols for the Cirrus Logic 5430 family of PCI video controllers.
  This driver is only usable in the EFI pre-boot environment.
  This sample is intended to show how the UGA Draw and Graphics output Protocol
  is able to function.
  The UGA I/O Protocol is not implemented in this sample.
  A fully compliant EFI UGA driver requires both
  the UGA Draw and the UGA I/O Protocol.  Please refer to Microsoft's
  documentation on UGA for details on how to write a UGA driver that is able
  to function both in the EFI pre-boot environment and from the OS runtime.

  Copyright (c) 2006 - 2010, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

//
// Cirrus Logic 5430 Controller Driver
//
#include "CirrusLogic5430.h"

EFI_DRIVER_BINDING_PROTOCOL gCirrusLogic5430DriverBinding = {
  CirrusLogic5430ControllerDriverSupported,
  CirrusLogic5430ControllerDriverStart,
  CirrusLogic5430ControllerDriverStop,
  0x10,
  NULL,
  NULL
};

///
/// Generic Attribute Controller Register Settings
///
UINT8  AttributeController[21] = {
  0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
  0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
  0x41, 0x00, 0x0F, 0x00, 0x00
};

///
/// Generic Graphics Controller Register Settings
///
UINT8 GraphicsController[9] = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x05, 0x0F, 0xFF
};

//
// 640 x 480 x 256 color @ 60 Hertz
//
UINT8 Crtc_640_480_256_60[28] = {
  0x5d, 0x4f, 0x50, 0x82, 0x53, 0x9f, 0x00, 0x3e,
  0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0xe1, 0x83, 0xdf, 0x50, 0x00, 0xe7, 0x04, 0xe3,
  0xff, 0x00, 0x00, 0x22
};

UINT16 Seq_640_480_256_60[15] = {
  0x0100, 0x0101, 0x0f02, 0x0003, 0x0e04, 0x1107, 0x0008, 0x4a0b,
  0x5b0c, 0x450d, 0x7e0e, 0x2b1b, 0x2f1c, 0x301d, 0x331e
};

//
// 800 x 600 x 256 color @ 60 Hertz
//
UINT8 Crtc_800_600_256_60[28] = {
  0x7F, 0x63, 0x64, 0x80, 0x6B, 0x1B, 0x72, 0xF0,
  0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x58, 0x8C, 0x57, 0x64, 0x00, 0x5F, 0x91, 0xE3,
  0xFF, 0x00, 0x00, 0x22
};

UINT16 Seq_800_600_256_60[15] = {
  0x0100, 0x0101, 0x0f02, 0x0003, 0x0e04, 0x1107, 0x0008, 0x4a0b,
  0x5b0c, 0x450d, 0x510e, 0x2b1b, 0x2f1c, 0x301d, 0x3a1e
};

//
// 1024 x 768 x 256 color @ 60 Hertz
//
UINT8 Crtc_1024_768_256_60[28] = {
  0xA3, 0x7F, 0x80, 0x86, 0x85, 0x96, 0x24, 0xFD,
  0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x02, 0x88, 0xFF, 0x80, 0x00, 0x00, 0x24, 0xE3,
  0xFF, 0x4A, 0x00, 0x22
};

UINT16 Seq_1024_768_256_60[15] = {
  0x0100, 0x0101, 0x0f02, 0x0003, 0x0e04, 0x1107, 0x0008, 0x4a0b,
  0x5b0c, 0x450d, 0x760e, 0x2b1b, 0x2f1c, 0x301d, 0x341e
};

///
/// Table of supported video modes
///
CIRRUS_LOGIC_5430_VIDEO_MODES  CirrusLogic5430VideoModes[] = {
  {  640, 480, 8, 60, Crtc_640_480_256_60,  Seq_640_480_256_60,  0xe3 },
  {  800, 600, 8, 60, Crtc_800_600_256_60,  Seq_800_600_256_60,  0xef },
  { 1024, 768, 8, 60, Crtc_1024_768_256_60, Seq_1024_768_256_60, 0xef }
};


/**
  CirrusLogic5430ControllerDriverSupported

  TODO:    This - add argument and description to function comment
  TODO:    Controller - add argument and description to function comment
  TODO:    RemainingDevicePath - add argument and description to function comment
**/
EFI_STATUS
EFIAPI
CirrusLogic5430ControllerDriverSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL    *This,
  IN EFI_HANDLE                     Controller,
  IN EFI_DEVICE_PATH_PROTOCOL       *RemainingDevicePath
  )
{
  EFI_STATUS          Status;
  EFI_PCI_IO_PROTOCOL *PciIo;
  PCI_TYPE00          Pci;
  EFI_DEV_PATH        *Node;

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
  //
  // See if the I/O enable is on.  Most systems only allow one VGA device to be turned on
  // at a time, so see if this is one that is turned on.
  //
  //  if (((Pci.Hdr.Command & 0x01) == 0x01)) {
  //
  // See if this is a Cirrus Logic PCI controller
  //
  if (Pci.Hdr.VendorId == CIRRUS_LOGIC_VENDOR_ID) {
    //
    // See if this is a 5430 or a 5446 PCI controller
    //
    if (Pci.Hdr.DeviceId == CIRRUS_LOGIC_5430_DEVICE_ID || 
        Pci.Hdr.DeviceId == CIRRUS_LOGIC_5430_ALTERNATE_DEVICE_ID ||
        Pci.Hdr.DeviceId == CIRRUS_LOGIC_5446_DEVICE_ID) {
        
      Status = EFI_SUCCESS;
      //
      // If this is an Intel 945 graphics controller,
      // go further check RemainingDevicePath validation
      //
      if (RemainingDevicePath != NULL) {
        Node = (EFI_DEV_PATH *) RemainingDevicePath;
        //
        // Check if RemainingDevicePath is the End of Device Path Node, 
        // if yes, return EFI_SUCCESS
        //
        if (!IsDevicePathEnd (Node)) {
          //
          // If RemainingDevicePath isn't the End of Device Path Node,
          // check its validation
          //
          if (Node->DevPath.Type != ACPI_DEVICE_PATH ||
              Node->DevPath.SubType != ACPI_ADR_DP ||
              DevicePathNodeLength(&Node->DevPath) != sizeof(ACPI_ADR_DEVICE_PATH)) {
            Status = EFI_UNSUPPORTED;
          }
        }
      }
    }
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
  CirrusLogic5430ControllerDriverStart

  TODO:    This - add argument and description to function comment
  TODO:    Controller - add argument and description to function comment
  TODO:    RemainingDevicePath - add argument and description to function comment
**/
EFI_STATUS
EFIAPI
CirrusLogic5430ControllerDriverStart (
  IN EFI_DRIVER_BINDING_PROTOCOL    *This,
  IN EFI_HANDLE                     Controller,
  IN EFI_DEVICE_PATH_PROTOCOL       *RemainingDevicePath
  )
{
  EFI_STATUS                      Status;
  CIRRUS_LOGIC_5430_PRIVATE_DATA  *Private;
  BOOLEAN                         PciAttributesSaved;
  EFI_DEVICE_PATH_PROTOCOL        *ParentDevicePath;
  ACPI_ADR_DEVICE_PATH            AcpiDeviceNode;
  UINT64                          Supports;

  PciAttributesSaved = FALSE;
  //
  // Allocate Private context data for UGA Draw inteface.
  //
  Private = AllocateZeroPool (sizeof (CIRRUS_LOGIC_5430_PRIVATE_DATA));
  if (Private == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto Error;
  }

  //
  // Set up context record
  //
  Private->Signature  = CIRRUS_LOGIC_5430_PRIVATE_DATA_SIGNATURE;
  Private->Handle     = NULL;

  //
  // Open PCI I/O Protocol
  //
  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiPciIoProtocolGuid,
                  (VOID **) &Private->PciIo,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (EFI_ERROR (Status)) {
    goto Error;
  }

  //
  // Get supported PCI attributes
  //
  Status = Private->PciIo->Attributes (
                             Private->PciIo,
                             EfiPciIoAttributeOperationSupported,
                             0,
                             &Supports
                             );
  if (EFI_ERROR (Status)) {
    goto Error;
  }

  Supports &= (EFI_PCI_IO_ATTRIBUTE_VGA_IO | EFI_PCI_IO_ATTRIBUTE_VGA_IO_16);
  if (Supports == 0 || Supports == (EFI_PCI_IO_ATTRIBUTE_VGA_IO | EFI_PCI_IO_ATTRIBUTE_VGA_IO_16)) {
    Status = EFI_UNSUPPORTED;
    goto Error;
  }  

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
    goto Error;
  }
  PciAttributesSaved = TRUE;

  Status = Private->PciIo->Attributes (
                             Private->PciIo,
                             EfiPciIoAttributeOperationEnable,
                             EFI_PCI_DEVICE_ENABLE | EFI_PCI_IO_ATTRIBUTE_VGA_MEMORY | Supports,
                             NULL
                             );
  if (EFI_ERROR (Status)) {
    goto Error;
  }

  //
  // Get ParentDevicePath
  //
  Status = gBS->HandleProtocol (
                  Controller,
                  &gEfiDevicePathProtocolGuid,
                  (VOID **) &ParentDevicePath
                  );
  if (EFI_ERROR (Status)) {
    goto Error;
  }

  if (FeaturePcdGet (PcdSupportGop)) {
    //
    // Set Gop Device Path
    //
    if (RemainingDevicePath == NULL) {
      ZeroMem (&AcpiDeviceNode, sizeof (ACPI_ADR_DEVICE_PATH));
      AcpiDeviceNode.Header.Type = ACPI_DEVICE_PATH;
      AcpiDeviceNode.Header.SubType = ACPI_ADR_DP;
      AcpiDeviceNode.ADR = ACPI_DISPLAY_ADR (1, 0, 0, 1, 0, ACPI_ADR_DISPLAY_TYPE_VGA, 0, 0);
      SetDevicePathNodeLength (&AcpiDeviceNode.Header, sizeof (ACPI_ADR_DEVICE_PATH));

      Private->GopDevicePath = AppendDevicePathNode (
                                          ParentDevicePath,
                                          (EFI_DEVICE_PATH_PROTOCOL *) &AcpiDeviceNode
                                          );
    } else if (!IsDevicePathEnd (RemainingDevicePath)) {
      //
      // If RemainingDevicePath isn't the End of Device Path Node, 
      // only scan the specified device by RemainingDevicePath
      //
      Private->GopDevicePath = AppendDevicePathNode (ParentDevicePath, RemainingDevicePath);
    } else {
      //
      // If RemainingDevicePath is the End of Device Path Node, 
      // don't create child device and return EFI_SUCCESS
      //
      Private->GopDevicePath = NULL;
    }
      
    if (Private->GopDevicePath != NULL) {
      //
      // Creat child handle and device path protocol firstly
      //
      Private->Handle = NULL;
      Status = gBS->InstallMultipleProtocolInterfaces (
                      &Private->Handle,
                      &gEfiDevicePathProtocolGuid,
                      Private->GopDevicePath,
                      NULL
                      );
    }
  }

  //
  // Construct video mode buffer
  //
  Status = CirrusLogic5430VideoModeSetup (Private);
  if (EFI_ERROR (Status)) {
    goto Error;
  }

  if (FeaturePcdGet (PcdSupportUga)) {
    //
    // Start the UGA Draw software stack.
    //
    Status = CirrusLogic5430UgaDrawConstructor (Private);
    ASSERT_EFI_ERROR (Status);

    Private->UgaDevicePath = ParentDevicePath;
    Status = gBS->InstallMultipleProtocolInterfaces (
                    &Controller,
                    &gEfiUgaDrawProtocolGuid,
                    &Private->UgaDraw,
                    &gEfiDevicePathProtocolGuid,
                    Private->UgaDevicePath,
                    NULL
                    );

  } else if (FeaturePcdGet (PcdSupportGop)) {
    if (Private->GopDevicePath == NULL) {
      //
      // If RemainingDevicePath is the End of Device Path Node, 
      // don't create child device and return EFI_SUCCESS
      //
      Status = EFI_SUCCESS;
    } else {
  
      //
      // Start the GOP software stack.
      //
      Status = CirrusLogic5430GraphicsOutputConstructor (Private);
      ASSERT_EFI_ERROR (Status);
  
      Status = gBS->InstallMultipleProtocolInterfaces (
                      &Private->Handle,
                      &gEfiGraphicsOutputProtocolGuid,
                      &Private->GraphicsOutput,
                      &gEfiEdidDiscoveredProtocolGuid,
                      &Private->EdidDiscovered,
                      &gEfiEdidActiveProtocolGuid,
                      &Private->EdidActive,
                      NULL
                      );
    }
  } else {
    //
    // This driver must support eithor GOP or UGA or both.
    //
    ASSERT (FALSE);
    Status = EFI_UNSUPPORTED;
  }


Error:
  if (EFI_ERROR (Status)) {
    if (Private) {
      if (Private->PciIo) {
        if (PciAttributesSaved == TRUE) {
          //
          // Restore original PCI attributes
          //
          Private->PciIo->Attributes (
                          Private->PciIo,
                          EfiPciIoAttributeOperationSet,
                          Private->OriginalPciAttributes,
                          NULL
                          );
        }
        //
        // Close the PCI I/O Protocol
        //
        gBS->CloseProtocol (
              Private->Handle,
              &gEfiPciIoProtocolGuid,
              This->DriverBindingHandle,
              Private->Handle
              );
      }

      gBS->FreePool (Private);
    }
  }

  return Status;
}

/**
  CirrusLogic5430ControllerDriverStop

  TODO:    This - add argument and description to function comment
  TODO:    Controller - add argument and description to function comment
  TODO:    NumberOfChildren - add argument and description to function comment
  TODO:    ChildHandleBuffer - add argument and description to function comment
  TODO:    EFI_SUCCESS - add return value to function comment
**/
EFI_STATUS
EFIAPI
CirrusLogic5430ControllerDriverStop (
  IN EFI_DRIVER_BINDING_PROTOCOL    *This,
  IN EFI_HANDLE                     Controller,
  IN UINTN                          NumberOfChildren,
  IN EFI_HANDLE                     *ChildHandleBuffer
  )
{
  EFI_UGA_DRAW_PROTOCOL           *UgaDraw;
  EFI_GRAPHICS_OUTPUT_PROTOCOL    *GraphicsOutput;

  EFI_STATUS                      Status;
  CIRRUS_LOGIC_5430_PRIVATE_DATA  *Private;

  if (FeaturePcdGet (PcdSupportUga)) {
    Status = gBS->OpenProtocol (
                    Controller,
                    &gEfiUgaDrawProtocolGuid,
                    (VOID **) &UgaDraw,
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
    Private = CIRRUS_LOGIC_5430_PRIVATE_DATA_FROM_UGA_DRAW_THIS (UgaDraw);
    CirrusLogic5430UgaDrawDestructor (Private);

    if (FeaturePcdGet (PcdSupportGop)) {
      CirrusLogic5430GraphicsOutputDestructor (Private);
      //
      // Remove the UGA and GOP protocol interface from the system
      //
      Status = gBS->UninstallMultipleProtocolInterfaces (
                      Private->Handle,
                      &gEfiUgaDrawProtocolGuid,
                      &Private->UgaDraw,
                      &gEfiGraphicsOutputProtocolGuid,
                      &Private->GraphicsOutput,
                      NULL
                      );
    } else {
      //
      // Remove the UGA Draw interface from the system
      //
      Status = gBS->UninstallMultipleProtocolInterfaces (
                      Private->Handle,
                      &gEfiUgaDrawProtocolGuid,
                      &Private->UgaDraw,
                      NULL
                      );
    }
  } else {
    Status = gBS->OpenProtocol (
                    Controller,
                    &gEfiGraphicsOutputProtocolGuid,
                    (VOID **) &GraphicsOutput,
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
    Private = CIRRUS_LOGIC_5430_PRIVATE_DATA_FROM_GRAPHICS_OUTPUT_THIS (GraphicsOutput);

    CirrusLogic5430GraphicsOutputDestructor (Private);
    //
    // Remove the GOP protocol interface from the system
    //
    Status = gBS->UninstallMultipleProtocolInterfaces (
                    Private->Handle,
                    &gEfiUgaDrawProtocolGuid,
                    &Private->UgaDraw,
                    &gEfiGraphicsOutputProtocolGuid,
                    &Private->GraphicsOutput,
                    NULL
                    );
  }

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

  //
  // Close the PCI I/O Protocol
  //
  gBS->CloseProtocol (
        Controller,
        &gEfiPciIoProtocolGuid,
        This->DriverBindingHandle,
        Controller
        );

  //
  // Free our instance data
  //
  gBS->FreePool (Private);

  return EFI_SUCCESS;
}

/**
  CirrusLogic5430UgaDrawDestructor

  TODO:    Private - add argument and description to function comment
  TODO:    EFI_SUCCESS - add return value to function comment
**/
EFI_STATUS
CirrusLogic5430UgaDrawDestructor (
  CIRRUS_LOGIC_5430_PRIVATE_DATA  *Private
  )
{
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
  CIRRUS_LOGIC_5430_PRIVATE_DATA  *Private,
  UINTN                           Address,
  UINT8                           Data
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
  CIRRUS_LOGIC_5430_PRIVATE_DATA  *Private,
  UINTN                           Address,
  UINT16                          Data
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
  CIRRUS_LOGIC_5430_PRIVATE_DATA  *Private,
  UINTN                           Address
  )
{
  UINT8 Data;

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
  CIRRUS_LOGIC_5430_PRIVATE_DATA  *Private,
  UINTN                           Address
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
  CIRRUS_LOGIC_5430_PRIVATE_DATA  *Private,
  UINTN                           Index,
  UINT8                           Red,
  UINT8                           Green,
  UINT8                           Blue
  )
{
  outb (Private, PALETTE_INDEX_REGISTER, (UINT8) Index);
  outb (Private, PALETTE_DATA_REGISTER, (UINT8) (Red >> 2));
  outb (Private, PALETTE_DATA_REGISTER, (UINT8) (Green >> 2));
  outb (Private, PALETTE_DATA_REGISTER, (UINT8) (Blue >> 2));
}

/**
  TODO: Add function description

  @param  Private TODO: add argument description

  TODO: add return values

**/
VOID
SetDefaultPalette (
  CIRRUS_LOGIC_5430_PRIVATE_DATA  *Private
  )
{
  UINTN Index;
  UINTN RedIndex;
  UINTN GreenIndex;
  UINTN BlueIndex;

  Index = 0;
  for (RedIndex = 0; RedIndex < 8; RedIndex++) {
    for (GreenIndex = 0; GreenIndex < 8; GreenIndex++) {
      for (BlueIndex = 0; BlueIndex < 4; BlueIndex++) {
        SetPaletteColor (Private, Index, (UINT8) (RedIndex << 5), (UINT8) (GreenIndex << 5), (UINT8) (BlueIndex << 6));
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
  CIRRUS_LOGIC_5430_PRIVATE_DATA  *Private
  )
{
  UINT32  Color;

  Color = 0;
  Private->PciIo->Mem.Write (
                        Private->PciIo,
                        EfiPciIoWidthFillUint32,
                        0,
                        0,
                        0x100000 >> 2,
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
  CIRRUS_LOGIC_5430_PRIVATE_DATA  *Private,
  UINTN                           ScreenWidth,
  UINTN                           ScreenHeight
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
InitializeGraphicsMode (
  CIRRUS_LOGIC_5430_PRIVATE_DATA  *Private,
  CIRRUS_LOGIC_5430_VIDEO_MODES   *ModeData
  )
{
  UINT8 Byte;
  UINTN Index;
  UINT16 DeviceId;
  EFI_STATUS Status;

  Status = Private->PciIo->Pci.Read (
             Private->PciIo,
             EfiPciIoWidthUint16,
             PCI_DEVICE_ID_OFFSET,
             1,
             &DeviceId
             );
  //
  // Read the PCI Configuration Header from the PCI Device
  //
  ASSERT_EFI_ERROR (Status);

  outw (Private, SEQ_ADDRESS_REGISTER, 0x1206);
  outw (Private, SEQ_ADDRESS_REGISTER, 0x0012);

  for (Index = 0; Index < 15; Index++) {
    outw (Private, SEQ_ADDRESS_REGISTER, ModeData->SeqSettings[Index]);
  }

  if (DeviceId != CIRRUS_LOGIC_5446_DEVICE_ID) {
    outb (Private, SEQ_ADDRESS_REGISTER, 0x0f);
    Byte = (UINT8) ((inb (Private, SEQ_DATA_REGISTER) & 0xc7) ^ 0x30);
    outb (Private, SEQ_DATA_REGISTER, Byte);
  }

  outb (Private, MISC_OUTPUT_REGISTER, ModeData->MiscSetting);
  outw (Private, GRAPH_ADDRESS_REGISTER, 0x0506);
  outw (Private, SEQ_ADDRESS_REGISTER, 0x0300);
  outw (Private, CRTC_ADDRESS_REGISTER, 0x2011);

  for (Index = 0; Index < 28; Index++) {
    outw (Private, CRTC_ADDRESS_REGISTER, (UINT16) ((ModeData->CrtcSettings[Index] << 8) | Index));
  }

  for (Index = 0; Index < 9; Index++) {
    outw (Private, GRAPH_ADDRESS_REGISTER, (UINT16) ((GraphicsController[Index] << 8) | Index));
  }

  inb (Private, INPUT_STATUS_1_REGISTER);

  for (Index = 0; Index < 21; Index++) {
    outb (Private, ATT_ADDRESS_REGISTER, (UINT8) Index);
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

EFI_STATUS
EFIAPI
InitializeCirrusLogic5430 (
  IN EFI_HANDLE           ImageHandle,
  IN EFI_SYSTEM_TABLE     *SystemTable
  )
{
  EFI_STATUS              Status;

  Status = EfiLibInstallDriverBindingComponentName2 (
             ImageHandle,
             SystemTable,
             &gCirrusLogic5430DriverBinding,
             ImageHandle,
             &gCirrusLogic5430ComponentName,
             &gCirrusLogic5430ComponentName2
             );
  ASSERT_EFI_ERROR (Status);

  //
  // Install EFI Driver Supported EFI Version Protocol required for
  // EFI drivers that are on PCI and other plug in cards.
  //
  gCirrusLogic5430DriverSupportedEfiVersion.FirmwareVersion = PcdGet32 (PcdDriverSupportedEfiVersion);
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &ImageHandle,
                  &gEfiDriverSupportedEfiVersionProtocolGuid,
                  &gCirrusLogic5430DriverSupportedEfiVersion,
                  NULL
                  );
  ASSERT_EFI_ERROR (Status);

  return Status;
}
