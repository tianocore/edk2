/** @file
  VGA Class Driver that managers VGA devices and produces Simple Text Output Protocol.

Copyright (c) 2006 - 2009, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

**/

#include "VgaClass.h"

//
// EFI Driver Binding Protocol for the VGA Class Driver
//
EFI_DRIVER_BINDING_PROTOCOL gVgaClassDriverBinding = {
  VgaClassDriverBindingSupported,
  VgaClassDriverBindingStart,
  VgaClassDriverBindingStop,
  0xa,
  NULL,
  NULL
};

//
// Local variables
//
CHAR16               CrLfString[3] = { CHAR_CARRIAGE_RETURN, CHAR_LINEFEED, CHAR_NULL };

//
// This list is used to define the valid extend chars.
// It also provides a mapping from Unicode to PCANSI or
// ASCII. The ASCII mapping we just made up.
//
//
UNICODE_TO_CHAR  UnicodeToPcAnsiOrAscii[] = {
  {
    BOXDRAW_HORIZONTAL,
    0xc4,
    L'-'
  },
  {
    BOXDRAW_VERTICAL,
    0xb3,
    L'|'
  },
  {
    BOXDRAW_DOWN_RIGHT,
    0xda,
    L'/'
  },
  {
    BOXDRAW_DOWN_LEFT,
    0xbf,
    L'\\'
  },
  {
    BOXDRAW_UP_RIGHT,
    0xc0,
    L'\\'
  },
  {
    BOXDRAW_UP_LEFT,
    0xd9,
    L'/'
  },
  {
    BOXDRAW_VERTICAL_RIGHT,
    0xc3,
    L'|'
  },
  {
    BOXDRAW_VERTICAL_LEFT,
    0xb4,
    L'|'
  },
  {
    BOXDRAW_DOWN_HORIZONTAL,
    0xc2,
    L'+'
  },
  {
    BOXDRAW_UP_HORIZONTAL,
    0xc1,
    L'+'
  },
  {
    BOXDRAW_VERTICAL_HORIZONTAL,
    0xc5,
    L'+'
  },
  {
    BOXDRAW_DOUBLE_HORIZONTAL,
    0xcd,
    L'-'
  },
  {
    BOXDRAW_DOUBLE_VERTICAL,
    0xba,
    L'|'
  },
  {
    BOXDRAW_DOWN_RIGHT_DOUBLE,
    0xd5,
    L'/'
  },
  {
    BOXDRAW_DOWN_DOUBLE_RIGHT,
    0xd6,
    L'/'
  },
  {
    BOXDRAW_DOUBLE_DOWN_RIGHT,
    0xc9,
    L'/'
  },
  {
    BOXDRAW_DOWN_LEFT_DOUBLE,
    0xb8,
    L'\\'
  },
  {
    BOXDRAW_DOWN_DOUBLE_LEFT,
    0xb7,
    L'\\'
  },
  {
    BOXDRAW_DOUBLE_DOWN_LEFT,
    0xbb,
    L'\\'
  },
  {
    BOXDRAW_UP_RIGHT_DOUBLE,
    0xd4,
    L'\\'
  },
  {
    BOXDRAW_UP_DOUBLE_RIGHT,
    0xd3,
    L'\\'
  },
  {
    BOXDRAW_DOUBLE_UP_RIGHT,
    0xc8,
    L'\\'
  },
  {
    BOXDRAW_UP_LEFT_DOUBLE,
    0xbe,
    L'/'
  },
  {
    BOXDRAW_UP_DOUBLE_LEFT,
    0xbd,
    L'/'
  },
  {
    BOXDRAW_DOUBLE_UP_LEFT,
    0xbc,
    L'/'
  },
  {
    BOXDRAW_VERTICAL_RIGHT_DOUBLE,
    0xc6,
    L'|'
  },
  {
    BOXDRAW_VERTICAL_DOUBLE_RIGHT,
    0xc7,
    L'|'
  },
  {
    BOXDRAW_DOUBLE_VERTICAL_RIGHT,
    0xcc,
    L'|'
  },
  {
    BOXDRAW_VERTICAL_LEFT_DOUBLE,
    0xb5,
    L'|'
  },
  {
    BOXDRAW_VERTICAL_DOUBLE_LEFT,
    0xb6,
    L'|'
  },
  {
    BOXDRAW_DOUBLE_VERTICAL_LEFT,
    0xb9,
    L'|'
  },
  {
    BOXDRAW_DOWN_HORIZONTAL_DOUBLE,
    0xd1,
    L'+'
  },
  {
    BOXDRAW_DOWN_DOUBLE_HORIZONTAL,
    0xd2,
    L'+'
  },
  {
    BOXDRAW_DOUBLE_DOWN_HORIZONTAL,
    0xcb,
    L'+'
  },
  {
    BOXDRAW_UP_HORIZONTAL_DOUBLE,
    0xcf,
    L'+'
  },
  {
    BOXDRAW_UP_DOUBLE_HORIZONTAL,
    0xd0,
    L'+'
  },
  {
    BOXDRAW_DOUBLE_UP_HORIZONTAL,
    0xca,
    L'+'
  },
  {
    BOXDRAW_VERTICAL_HORIZONTAL_DOUBLE,
    0xd8,
    L'+'
  },
  {
    BOXDRAW_VERTICAL_DOUBLE_HORIZONTAL,
    0xd7,
    L'+'
  },
  {
    BOXDRAW_DOUBLE_VERTICAL_HORIZONTAL,
    0xce,
    L'+'
  },

  {
    BLOCKELEMENT_FULL_BLOCK,
    0xdb,
    L'*'
  },
  {
    BLOCKELEMENT_LIGHT_SHADE,
    0xb0,
    L'+'
  },

  {
    GEOMETRICSHAPE_UP_TRIANGLE,
    0x1e,
    L'^'
  },
  {
    GEOMETRICSHAPE_RIGHT_TRIANGLE,
    0x10,
    L'>'
  },
  {
    GEOMETRICSHAPE_DOWN_TRIANGLE,
    0x1f,
    L'v'
  },
  {
    GEOMETRICSHAPE_LEFT_TRIANGLE,
    0x11,
    L'<'
  },

  {
    ARROW_LEFT,
    0x3c,
    L'<'
  },

  {
    ARROW_UP,
    0x18,
    L'^'
  },

  {
    ARROW_RIGHT,
    0x3e,
    L'>'
  },

  {
    ARROW_DOWN,
    0x19,
    L'v'
  },

  {
    0x0000,
    0x00,
    0x00
  }
};

/**
  Entrypoint of this VGA Class Driver.

  This function is the entrypoint of this VGA Class Driver. It installs Driver Binding
  Protocols together with Component Name Protocols.

  @param  ImageHandle       The firmware allocated handle for the EFI image.
  @param  SystemTable       A pointer to the EFI System Table.
  
  @retval EFI_SUCCESS       The entry point is executed successfully.

**/
EFI_STATUS
EFIAPI
InitializeVgaClass(
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
             &gVgaClassDriverBinding,
             ImageHandle,
             &gVgaClassComponentName,
             &gVgaClassComponentName2
             );
  ASSERT_EFI_ERROR (Status);

  return EFI_SUCCESS;
}

/**
  Internal worker function to program CRTC register via PCI I/O Protocol.
  
  @param VgaClassDev  device instance object
  @param Address      Address of register to write
  @param Data         Data to write to register.

**/
VOID
WriteCrtc (
  IN  VGA_CLASS_DEV  *VgaClassDev,
  IN  UINT16         Address,
  IN  UINT8          Data
  )
{
  VgaClassDev->PciIo->Io.Write (
                           VgaClassDev->PciIo,
                           EfiPciIoWidthUint8,
                           VgaClassDev->VgaMiniPort->CrtcAddressRegisterBar,
                           VgaClassDev->VgaMiniPort->CrtcAddressRegisterOffset,
                           1,
                           &Address
                           );

  VgaClassDev->PciIo->Io.Write (
                           VgaClassDev->PciIo,
                           EfiPciIoWidthUint8,
                           VgaClassDev->VgaMiniPort->CrtcDataRegisterBar,
                           VgaClassDev->VgaMiniPort->CrtcDataRegisterOffset,
                           1,
                           &Data
                           );
}

/**
  Internal worker function to set cursor's position to VgaClass device
  
  @param  VgaClassDev   Private data structure for device instance.
  @param  Column        Colomn of position to set cursor to.
  @param  Row           Row of position to set cursor to.
  @param  MaxColumn     Max value of column.
  
**/
VOID
SetVideoCursorPosition (
  IN  VGA_CLASS_DEV  *VgaClassDev,
  IN  UINTN          Column,
  IN  UINTN          Row,
  IN  UINTN          MaxColumn
  )
{
  Column    = Column & 0xff;
  Row       = Row & 0xff;
  MaxColumn = MaxColumn & 0xff;

  WriteCrtc (
    VgaClassDev,
    CRTC_CURSOR_LOCATION_HIGH,
    (UINT8) ((Row * MaxColumn + Column) >> 8)
    );
  WriteCrtc (
    VgaClassDev,
    CRTC_CURSOR_LOCATION_LOW,
    (UINT8) ((Row * MaxColumn + Column) & 0xff)
    );
}

/**
  Internal worker function to detect if a Unicode char is for Box Drawing text graphics.

  @param  Graphic  Unicode char to test.
  @param  PcAnsi   Pointer to PCANSI equivalent of Graphic for output.
                   If NULL, then PCANSI value is not returned.
  @param  Ascii    Pointer to ASCII equivalent of Graphic for output.
                   If NULL, then ASCII value is not returned.

  @retval TRUE     Gpaphic is a supported Unicode Box Drawing character.
  @retval FALSE    Gpaphic is not a supported Unicode Box Drawing character.

**/
BOOLEAN
LibIsValidTextGraphics (
  IN  CHAR16  Graphic,
  OUT CHAR8   *PcAnsi, OPTIONAL
  OUT CHAR8   *Ascii OPTIONAL
  )
{
  UNICODE_TO_CHAR *Table;

  //
  // Unicode drawing code charts are all in the 0x25xx range, arrows are 0x21xx.
  // So first filter out values not in these 2 ranges.
  //
  if ((((Graphic & 0xff00) != 0x2500) && ((Graphic & 0xff00) != 0x2100))) {
    return FALSE;
  }

  //
  // Search UnicodeToPcAnsiOrAscii table for matching entry.
  //
  for (Table = UnicodeToPcAnsiOrAscii; Table->Unicode != 0x0000; Table++) {
    if (Graphic == Table->Unicode) {
      if (PcAnsi != NULL) {
        *PcAnsi = Table->PcAnsi;
      }

      if (Ascii != NULL) {
        *Ascii = Table->Ascii;
      }

      return TRUE;
    }
  }

  //
  // If value is not found in UnicodeToPcAnsiOrAscii table, then return FALSE.
  //
  return FALSE;
}

/**
  Internal worker function to check whether input value is an ASCII char.
  
  @param  Char     Character to check.

  @retval TRUE     Input value is an ASCII char.
  @retval FALSE    Input value is not an ASCII char.

**/
BOOLEAN
IsValidAscii (
  IN  CHAR16  Char
  )
{
  if ((Char >= 0x20) && (Char <= 0x7f)) {
    return TRUE;
  }

  return FALSE;
}

/**
  Internal worker function to check whether input value is a unicode control char.
  
  @param  Char    Character to check.

  @retval TRUE     Input value is a unicode control char.
  @retval FALSE    Input value is not a unicode control char.

**/
BOOLEAN
IsValidEfiCntlChar (
  IN  CHAR16  Char
  )
{
  if (Char == CHAR_NULL || Char == CHAR_BACKSPACE || Char == CHAR_LINEFEED || Char == CHAR_CARRIAGE_RETURN) {
    return TRUE;
  }

  return FALSE;
}

/**
  Tests to see if this driver supports a given controller.

  This function implments EFI_DRIVER_BINDING_PROTOCOL.Supported().
  It Checks if this driver supports the controller specified. Any Controller
  with VgaMiniPort Protocol and Pci I/O protocol can be supported.

  @param  This                A pointer to the EFI_DRIVER_BINDING_PROTOCOL instance.
  @param  ControllerHandle    Handle of device to test
  @param  RemainingDevicePath Optional parameter use to pick a specific child
                              device to start.

  @retval EFI_SUCCESS         This driver supports this device.
  @retval EFI_ALREADY_STARTED This driver is already running on this device.
  @retval EFI_UNSUPPORTED     This driver does not support this device.

**/
EFI_STATUS
EFIAPI
VgaClassDriverBindingSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   Controller,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath OPTIONAL
  )
{
  EFI_STATUS                  Status;

  //
  // Checks if Abstraction(s) needed to perform the supported test
  //
  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiVgaMiniPortProtocolGuid,
                  NULL,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_TEST_PROTOCOL
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }
  //
  // Open the IO Abstraction(s) needed to perform the supported test
  //
  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiPciIoProtocolGuid,
                  NULL,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_TEST_PROTOCOL
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  return Status;
}

/**
  Starts the device controller.

  This function implments EFI_DRIVER_BINDING_PROTOCOL.Start().
  It starts the device specified by Controller with the driver based on PCI I/O Protocol
  and VgaMiniPort Protocol. It creates context for device instance and install EFI_SIMPLE_TEXT_OUT_PROTOCOL.

  @param  This                 A pointer to the EFI_DRIVER_BINDING_PROTOCOL instance.
  @param  ControllerHandle     Handle of device to bind driver to
  @param  RemainingDevicePath  Optional parameter use to pick a specific child
                               device to start.

  @retval EFI_SUCCESS          The device was started.
  @retval other                Fail to start the device.

**/
EFI_STATUS
EFIAPI
VgaClassDriverBindingStart (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   Controller,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath OPTIONAL
  )
{
  EFI_STATUS                  Status;
  EFI_VGA_MINI_PORT_PROTOCOL  *VgaMiniPort;
  EFI_PCI_IO_PROTOCOL         *PciIo;
  VGA_CLASS_DEV               *VgaClassPrivate;
  EFI_DEVICE_PATH_PROTOCOL    *DevicePath;

  Status = gBS->HandleProtocol (
                  Controller,
                  &gEfiDevicePathProtocolGuid,
                  (VOID **) &DevicePath
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }
  //
  // Report that VGA Class driver is being enabled
  //
  REPORT_STATUS_CODE_WITH_DEVICE_PATH (
    EFI_PROGRESS_CODE,
    EFI_PERIPHERAL_LOCAL_CONSOLE | EFI_P_PC_ENABLE,
    DevicePath
    );

  //
  // Open the PCI I/O Protocol
  //
  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiPciIoProtocolGuid,
                  (VOID **) &PciIo,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }
  //
  // Open the VGA Mini Port Protocol
  //
  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiVgaMiniPortProtocolGuid,
                  (VOID **) &VgaMiniPort,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }
  //
  // Allocate the private device structure
  //
  VgaClassPrivate = AllocateZeroPool (sizeof (VGA_CLASS_DEV));
  ASSERT (VgaClassPrivate != NULL);

  //
  // Initialize the private device structure
  //
  VgaClassPrivate->Signature   = VGA_CLASS_DEV_SIGNATURE;
  VgaClassPrivate->Handle      = Controller;
  VgaClassPrivate->VgaMiniPort = VgaMiniPort;
  VgaClassPrivate->PciIo       = PciIo;

  VgaClassPrivate->SimpleTextOut.Reset             = VgaClassReset;
  VgaClassPrivate->SimpleTextOut.OutputString      = VgaClassOutputString;
  VgaClassPrivate->SimpleTextOut.TestString        = VgaClassTestString;
  VgaClassPrivate->SimpleTextOut.ClearScreen       = VgaClassClearScreen;
  VgaClassPrivate->SimpleTextOut.SetAttribute      = VgaClassSetAttribute;
  VgaClassPrivate->SimpleTextOut.SetCursorPosition = VgaClassSetCursorPosition;
  VgaClassPrivate->SimpleTextOut.EnableCursor      = VgaClassEnableCursor;
  VgaClassPrivate->SimpleTextOut.QueryMode         = VgaClassQueryMode;
  VgaClassPrivate->SimpleTextOut.SetMode           = VgaClassSetMode;

  VgaClassPrivate->SimpleTextOut.Mode              = &VgaClassPrivate->SimpleTextOutputMode;
  VgaClassPrivate->SimpleTextOutputMode.MaxMode    = VgaMiniPort->MaxMode;
  VgaClassPrivate->DevicePath                      = DevicePath;

  //
  // Initialize the VGA device.
  //
  Status = VgaClassPrivate->SimpleTextOut.SetAttribute (
                                            &VgaClassPrivate->SimpleTextOut,
                                            EFI_TEXT_ATTR (EFI_WHITE, EFI_BLACK)
                                            );
  if (EFI_ERROR (Status)) {
    goto ErrorExit;
  }

  Status = VgaClassPrivate->SimpleTextOut.Reset (
                                            &VgaClassPrivate->SimpleTextOut,
                                            FALSE
                                            );
  if (EFI_ERROR (Status)) {
    goto ErrorExit;
  }

  Status = VgaClassPrivate->SimpleTextOut.EnableCursor (
                                            &VgaClassPrivate->SimpleTextOut,
                                            TRUE
                                            );
  if (EFI_ERROR (Status)) {
    goto ErrorExit;
  }

  Status = gBS->InstallMultipleProtocolInterfaces (
                  &Controller,
                  &gEfiSimpleTextOutProtocolGuid,
                  &VgaClassPrivate->SimpleTextOut,
                  NULL
                  );

  return Status;

ErrorExit:
  REPORT_STATUS_CODE_WITH_DEVICE_PATH (
    EFI_ERROR_CODE | EFI_ERROR_MINOR,
    EFI_PERIPHERAL_LOCAL_CONSOLE | EFI_P_EC_CONTROLLER_ERROR,
    DevicePath
    );

  return Status;

}

/**
  Starts the device controller.
  
  This function implments EFI_DRIVER_BINDING_PROTOCOL.Stop().
  It stops this driver on Controller. Support stoping any child handles
  created by this driver.

  @param  This              A pointer to the EFI_DRIVER_BINDING_PROTOCOL instance.
  @param  ControllerHandle  A handle to the device being stopped.
  @param  NumberOfChildren  The number of child device handles in ChildHandleBuffer.
  @param  ChildHandleBuffer An array of child handles to be freed.

  @retval EFI_SUCCESS       This driver is removed ControllerHandle
  @retval other             This driver was not removed from this device

**/
EFI_STATUS
EFIAPI
VgaClassDriverBindingStop (
  IN  EFI_DRIVER_BINDING_PROTOCOL     *This,
  IN  EFI_HANDLE                      Controller,
  IN  UINTN                           NumberOfChildren,
  IN  EFI_HANDLE                      *ChildHandleBuffer OPTIONAL
  )
{
  EFI_STATUS                    Status;
  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL  *SimpleTextOut;
  VGA_CLASS_DEV                 *VgaClassPrivate;

  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiSimpleTextOutProtocolGuid,
                  (VOID **) &SimpleTextOut,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  VgaClassPrivate = VGA_CLASS_DEV_FROM_THIS (SimpleTextOut);

  //
  // Report that VGA Class driver is being disabled
  //
  REPORT_STATUS_CODE_WITH_DEVICE_PATH (
    EFI_PROGRESS_CODE,
    EFI_PERIPHERAL_LOCAL_CONSOLE | EFI_P_PC_DISABLE,
    VgaClassPrivate->DevicePath
    );

  Status = gBS->UninstallProtocolInterface (
                  Controller,
                  &gEfiSimpleTextOutProtocolGuid,
                  &VgaClassPrivate->SimpleTextOut
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }
  //
  // Release PCI I/O and VGA Mini Port Protocols on the controller handle.
  //
  gBS->CloseProtocol (
         Controller,
         &gEfiPciIoProtocolGuid,
         This->DriverBindingHandle,
         Controller
         );

  gBS->CloseProtocol (
         Controller,
         &gEfiVgaMiniPortProtocolGuid,
         This->DriverBindingHandle,
         Controller
         );

  FreePool (VgaClassPrivate);

  return EFI_SUCCESS;
}

/**
  Resets the text output device hardware.

  This function implements EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL.Reset().
  It resets the text output device hardware. The cursor position is set to (0, 0),
  and the screen is cleared to the default background color for the output device.
  
  @param  This                 Pointer to EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL instance.
  @param  ExtendedVerification Indicates that the driver may perform a more exhaustive
                               verification operation of the device during reset.

  @retval EFI_SUCCESS          The text output device was reset.
  @retval EFI_DEVICE_ERROR     The text output device is not functioning correctly and could not be reset.

**/
EFI_STATUS
EFIAPI
VgaClassReset (
  IN  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL     *This,
  IN  BOOLEAN                             ExtendedVerification
  )
{
  EFI_STATUS    Status;
  VGA_CLASS_DEV *VgaClassPrivate;

  VgaClassPrivate = VGA_CLASS_DEV_FROM_THIS (This);

  REPORT_STATUS_CODE_WITH_DEVICE_PATH (
    EFI_PROGRESS_CODE,
    EFI_PERIPHERAL_LOCAL_CONSOLE | EFI_P_PC_RESET,
    VgaClassPrivate->DevicePath
    );

  This->SetAttribute (This, EFI_TEXT_ATTR (This->Mode->Attribute & 0x0F, EFI_BACKGROUND_BLACK));

  Status = This->SetMode (This, 0);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  return This->ClearScreen (This);
}

/**
  Writes a Unicode string to the output device.
  
  This function implements EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL.OutputString().
  It writes a Unicode string to the output device. This is the most basic output mechanism
  on an output device.

  @param  This                   Pointer to EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL instance.
  @param  String                 The Null-terminated Unicode string to be displayed on the output device(s).

  @retval EFI_SUCCESS            The string was output to the device.
  @retval EFI_DEVICE_ERROR       The device reported an error while attempting to output the text.
  @retval EFI_UNSUPPORTED        The output device's mode is not currently in a defined text mode.
  @retval EFI_WARN_UNKNOWN_GLYPH This warning code indicates that some of the characters in
                                 the Unicode string could not be rendered and were skipped.

**/
EFI_STATUS
EFIAPI
VgaClassOutputString (
  IN  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This,
  IN  CHAR16                          *String
  )
{
  EFI_STATUS                  Status;
  VGA_CLASS_DEV               *VgaClassDev;
  EFI_SIMPLE_TEXT_OUTPUT_MODE *Mode;
  UINTN                       MaxColumn;
  UINTN                       MaxRow;
  UINT32                      VideoChar;
  CHAR8                       GraphicChar;

  VgaClassDev = VGA_CLASS_DEV_FROM_THIS (This);
  Mode        = This->Mode;

  Status = This->QueryMode (
                   This,
                   Mode->Mode,
                   &MaxColumn,
                   &MaxRow
                   );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Parse each character of the string to output
  //
  for (; *String != CHAR_NULL; String++) {

    switch (*String) {
    case CHAR_BACKSPACE:
      if (Mode->CursorColumn > 0) {
        Mode->CursorColumn--;
      }
      break;

    case CHAR_LINEFEED:
      if (Mode->CursorRow == (INT32) (MaxRow - 1)) {
        //
        // Scroll the screen by copying the contents
        // of the VGA display up one line
        //
        VgaClassDev->PciIo->CopyMem (
                              VgaClassDev->PciIo,
                              EfiPciIoWidthUint32,
                              VgaClassDev->VgaMiniPort->VgaMemoryBar,
                              VgaClassDev->VgaMiniPort->VgaMemoryOffset,
                              VgaClassDev->VgaMiniPort->VgaMemoryBar,
                              VgaClassDev->VgaMiniPort->VgaMemoryOffset + MaxColumn * 2,
                              ((MaxRow - 1) * MaxColumn) >> 1
                              );

        //
        // Print Blank Line of spaces with the current color attributes
        //
        VideoChar = (Mode->Attribute << 8) | ' ';
        VideoChar = (VideoChar << 16) | VideoChar;
        VgaClassDev->PciIo->Mem.Write (
                                  VgaClassDev->PciIo,
                                  EfiPciIoWidthFillUint32,
                                  VgaClassDev->VgaMiniPort->VgaMemoryBar,
                                  VgaClassDev->VgaMiniPort->VgaMemoryOffset + (MaxRow - 1) * MaxColumn * 2,
                                  MaxColumn >> 1,
                                  &VideoChar
                                  );
      }

      if (Mode->CursorRow < (INT32) (MaxRow - 1)) {
        Mode->CursorRow++;
      }
      break;

    case CHAR_CARRIAGE_RETURN:
      Mode->CursorColumn = 0;
      break;

    default:
      if (!LibIsValidTextGraphics (*String, &GraphicChar, NULL)) {
        //
        // If this character is not ,Box Drawing text graphics, then convert it to ASCII.
        //
        GraphicChar = (CHAR8) *String;
        if (!IsValidAscii (GraphicChar)) {
          //
          // If not valid ASCII char, convert it to "?"
          //
          GraphicChar = '?';
        }
      }

      VideoChar = (Mode->Attribute << 8) | GraphicChar;
      VgaClassDev->PciIo->Mem.Write (
                                VgaClassDev->PciIo,
                                EfiPciIoWidthUint16,
                                VgaClassDev->VgaMiniPort->VgaMemoryBar,
                                VgaClassDev->VgaMiniPort->VgaMemoryOffset + ((Mode->CursorRow * MaxColumn + Mode->CursorColumn) * 2),
                                1,
                                &VideoChar
                                );

      if (Mode->CursorColumn >= (INT32) (MaxColumn - 1)) {
        This->OutputString (This, CrLfString);
      } else {
        Mode->CursorColumn++;
      }
      break;
    }
  }

  SetVideoCursorPosition (
    VgaClassDev,
    (UINTN) Mode->CursorColumn,
    (UINTN) Mode->CursorRow,
    MaxColumn
    );

  return EFI_SUCCESS;
}

/**
  Verifies that all characters in a Unicode string can be output to the target device.
  
  This function implements EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL.TestString().
  It verifies that all characters in a Unicode string can be output to the target device.

  @param  This                   Pointer to EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL instance.
  @param  String                 The Null-terminated Unicode string to be examined for the output device(s).

  @retval EFI_SUCCESS            The device(s) are capable of rendering the output string.
  @retval EFI_UNSUPPORTED        Some of the characters in the Unicode string cannot be rendered by
                                 one or more of the output devices mapped by the EFI handle.

**/
EFI_STATUS
EFIAPI
VgaClassTestString (
  IN  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This,
  IN  CHAR16                          *String
  )
{
  while (*String != CHAR_NULL) {
    if (!(IsValidAscii (*String) || IsValidEfiCntlChar (*String) || LibIsValidTextGraphics (*String, NULL, NULL))) {
      return EFI_UNSUPPORTED;
    }

    String++;
  }

  return EFI_SUCCESS;
}

/**
  Clears the output device(s) display to the currently selected background color.
  
  This function implements EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL.ClearScreen().
  The ClearScreen() function clears the output device(s) display to the currently
  selected background color. The cursor position is set to (0, 0).

  @param  This                   Pointer to EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL instance.
  
  @retval EFI_SUCESS             The operation completed successfully.
  @retval EFI_DEVICE_ERROR       The device had an error and could not complete the request.
  @retval EFI_UNSUPPORTED        The output device is not in a valid text mode.

**/
EFI_STATUS
EFIAPI
VgaClassClearScreen (
  IN  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL    *This
  )
{
  EFI_STATUS    Status;
  VGA_CLASS_DEV *VgaClassDev;
  UINTN         MaxRow;
  UINTN         MaxColumn;
  UINT32        VideoChar;

  VgaClassDev = VGA_CLASS_DEV_FROM_THIS (This);

  Status = This->QueryMode (
                   This,
                   This->Mode->Mode,
                   &MaxColumn,
                   &MaxRow
                   );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  VideoChar = (This->Mode->Attribute << 8) | ' ';
  VideoChar = (VideoChar << 16) | VideoChar;
  VgaClassDev->PciIo->Mem.Write (
                            VgaClassDev->PciIo,
                            EfiPciIoWidthFillUint32,
                            VgaClassDev->VgaMiniPort->VgaMemoryBar,
                            VgaClassDev->VgaMiniPort->VgaMemoryOffset,
                            (MaxRow * MaxColumn) >> 1,
                            &VideoChar
                            );

  This->SetCursorPosition (This, 0, 0);

  return EFI_SUCCESS;
}

/**
  Sets the background and foreground colors for theOutputString() and ClearScreen() functions.
  
  This function implements EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL.SetAttribute().
  It sets the background and foreground colors for the OutputString() and ClearScreen() functions.
  The color mask can be set even when the device is in an invalid text mode.
  Devices supporting a different number of text colors are required to emulate the above colors
  to the best of the device's capabilities.

  @param  This                   Pointer to EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL instance.
  @param  Attribute              The attribute to set.
                                 Bits 0..3 are the foreground color,
                                 and bits 4..6 are the background color.
  
  @retval EFI_SUCCESS            The requested attributes were set.
  @retval EFI_DEVICE_ERROR       The device had an error and could not complete the request.

**/
EFI_STATUS
EFIAPI
VgaClassSetAttribute (
  IN  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This,
  IN  UINTN                           Attribute
  )
{
  if (Attribute <= EFI_MAX_ATTRIBUTE) {
    This->Mode->Attribute = (INT32) Attribute;
    return EFI_SUCCESS;
  }

  return EFI_UNSUPPORTED;
}

/**
  Sets the current coordinates of the cursor position.
  
  This function implements EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL.SetCursorPosition().
  It sets the current coordinates of the cursor position.
  The upper left corner of the screen is defined as coordinate (0, 0).
  
  @param  This                   Pointer to EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL instance.
  @param  Column                 Column of position to set the cursor to.
  @param  Row                    Row of position to set the cursor to.
  
  @retval EFI_SUCCESS            The operation completed successfully.
  @retval EFI_DEVICE_ERROR       The device had an error and could not complete the request.
  @retval EFI_UNSUPPORTED        The output device is not in a valid text mode, or the cursor
                                 position is invalid for the current mode.

**/
EFI_STATUS
EFIAPI
VgaClassSetCursorPosition (
  IN  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This,
  IN  UINTN                           Column,
  IN  UINTN                           Row
  )
{
  EFI_STATUS    Status;
  VGA_CLASS_DEV *VgaClassDev;
  UINTN         MaxColumn;
  UINTN         MaxRow;

  VgaClassDev = VGA_CLASS_DEV_FROM_THIS (This);

  Status = This->QueryMode (
                   This,
                   This->Mode->Mode,
                   &MaxColumn,
                   &MaxRow
                   );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (Column >= MaxColumn || Row >= MaxRow) {
    return EFI_UNSUPPORTED;
  }

  SetVideoCursorPosition (VgaClassDev, Column, Row, MaxColumn);

  This->Mode->CursorColumn  = (INT32) Column;
  This->Mode->CursorRow     = (INT32) Row;

  return EFI_SUCCESS;
}

/**
  Makes the cursor visible or invisible.
  
  This function implements EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL.EnableCursor().
  It makes the cursor visible or invisible.

  @param  This                   Pointer to EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL instance.
  @param  Visible                If TRUE, the cursor is set to be visible.
                                 If FALSE, the cursor is set to be invisible.
  
  @retval EFI_SUCESS             The operation completed successfully.
  @retval EFI_DEVICE_ERROR       The device had an error and could not complete the request or the
                                 device does not support changing the cursor mode.
  @retval EFI_UNSUPPORTED        The output device does not support visibility control of the cursor.

**/
EFI_STATUS
EFIAPI
VgaClassEnableCursor (
  IN  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This,
  IN  BOOLEAN                         Visible
  )
{
  VGA_CLASS_DEV *VgaClassDev;

  VgaClassDev = VGA_CLASS_DEV_FROM_THIS (This);
  if (Visible) {
    if (This->Mode->Mode == 1) {
      //
      // 80 * 50
      //
      WriteCrtc (VgaClassDev, CRTC_CURSOR_START, 0x06);
      WriteCrtc (VgaClassDev, CRTC_CURSOR_END, 0x07);
    } else {
      //
      // 80 * 25
      //
      WriteCrtc (VgaClassDev, CRTC_CURSOR_START, 0x0e);
      WriteCrtc (VgaClassDev, CRTC_CURSOR_END, 0x0f);
    }
  } else {
    WriteCrtc (VgaClassDev, CRTC_CURSOR_START, 0x20);
  }

  This->Mode->CursorVisible = Visible;
  return EFI_SUCCESS;
}

/**
  Returns information for an available text mode that the output device(s) supports.

  This function implements EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL.QueryMode().
  It returns information for an available text mode that the output device(s) supports.
  It is required that all output devices support at least 80x25 text mode. This mode is defined to be mode 0.
  If the output devices support 80x50, that is defined to be mode 1.

  @param  This                   Pointer to EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL instance.
  @param  ModeNumber             The mode number to return information on.
  @param  Columns                Columen in current mode number
  @param  Rows                   Row in current mode number.
  
  @retval EFI_SUCCESS            The requested mode information was returned.
  @retval EFI_DEVICE_ERROR       The device had an error and could not complete the request.
  @retval EFI_UNSUPPORTED        The mode number was not valid.

**/
EFI_STATUS
EFIAPI
VgaClassQueryMode (
  IN  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This,
  IN  UINTN                           ModeNumber,
  OUT UINTN                           *Columns,
  OUT UINTN                           *Rows
  )
{
  if ((INT32) ModeNumber >= This->Mode->MaxMode) {
    *Columns  = 0;
    *Rows     = 0;
    return EFI_UNSUPPORTED;
  }

  switch (ModeNumber) {
  case 0:
    *Columns  = 80;
    *Rows     = 25;
    break;

  case 1:
    *Columns  = 80;
    *Rows     = 50;
    break;

  default:
    *Columns  = 0;
    *Rows     = 0;
    return EFI_UNSUPPORTED;
  }

  return EFI_SUCCESS;
}

/**
  Sets the output device(s) to a specified mode.
  
  This function implements EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL.QueryMode().
  It sets the output device(s) to the requested mode.
  On success the device is in the geometry for the requested mode,
  and the device has been cleared to the current background color with the cursor at (0,0).

  @param  This                   Pointer to EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL instance.
  @param  ModeNumber             The text mode to set.
  
  @retval EFI_SUCCESS            The requested text mode was set.
  @retval EFI_DEVICE_ERROR       The device had an error and could not complete the request.
  @retval EFI_UNSUPPORTED        The mode number was not valid.

**/
EFI_STATUS
EFIAPI
VgaClassSetMode (
  IN  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This,
  IN  UINTN                           ModeNumber
  )
{
  VGA_CLASS_DEV *VgaClassDev;

  VgaClassDev = VGA_CLASS_DEV_FROM_THIS (This);

  if ((INT32) ModeNumber >= This->Mode->MaxMode) {
    return EFI_UNSUPPORTED;
  }

  This->ClearScreen (This);

  This->Mode->Mode  = (INT32) ModeNumber;

  return VgaClassDev->VgaMiniPort->SetMode (VgaClassDev->VgaMiniPort, ModeNumber);
}
