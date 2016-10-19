/** @file
  Internal include file of the VGA Class Driver.

Copyright (c) 2006 - 2011, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

**/


#ifndef _VGA_CLASS_H__
#define _VGA_CLASS_H__

#include <FrameworkDxe.h>

#include <Protocol/SimpleTextOut.h>
#include <Protocol/PciIo.h>
#include <Protocol/VgaMiniPort.h>
#include <Protocol/DevicePath.h>

#include <Library/DebugLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/ReportStatusCodeLib.h>

#include <IndustryStandard/Pci.h>

//
// Global Variables
//
extern EFI_DRIVER_BINDING_PROTOCOL   gVgaClassDriverBinding;
extern EFI_COMPONENT_NAME_PROTOCOL   gVgaClassComponentName;
extern EFI_COMPONENT_NAME2_PROTOCOL  gVgaClassComponentName2;


//
// Structure for tuple containing mapping among uniocde, PC Ansi and ASCII code.
//
typedef struct {
  CHAR16  Unicode;
  CHAR8   PcAnsi;
  CHAR8   Ascii;
} UNICODE_TO_CHAR;

//
// VGA specific registers
//
#define CRTC_CURSOR_START         0xA
#define CRTC_CURSOR_END           0xB

#define CRTC_CURSOR_LOCATION_HIGH 0xE
#define CRTC_CURSOR_LOCATION_LOW  0xF

#define EFI_MAX_ATTRIBUTE         0x7f

//
// VGA Class Device Structure
//
#define VGA_CLASS_DEV_SIGNATURE SIGNATURE_32 ('V', 'G', 'A', 'C')

typedef struct {
  UINTN                            Signature;
  EFI_HANDLE                       Handle;
  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL  SimpleTextOut;
  EFI_SIMPLE_TEXT_OUTPUT_MODE      SimpleTextOutputMode;
  EFI_VGA_MINI_PORT_PROTOCOL       *VgaMiniPort;
  EFI_PCI_IO_PROTOCOL              *PciIo;
  EFI_DEVICE_PATH_PROTOCOL         *DevicePath;
} VGA_CLASS_DEV;

#define VGA_CLASS_DEV_FROM_THIS(a)  CR (a, VGA_CLASS_DEV, SimpleTextOut, VGA_CLASS_DEV_SIGNATURE)

//
// Driver Binding Protocol functions
//

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
  );

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
  );

/**
  Starts the device controller.
  
  This function implments EFI_DRIVER_BINDING_PROTOCOL.Stop().
  It stops this driver on Controller. Support stopping any child handles
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
  );

//
// EFI Component Name Functions
//

/**
  Retrieves a Unicode string that is the user readable name of the driver.

  This function retrieves the user readable name of a driver in the form of a
  Unicode string. If the driver specified by This has a user readable name in
  the language specified by Language, then a pointer to the driver name is
  returned in DriverName, and EFI_SUCCESS is returned. If the driver specified
  by This does not support the language specified by Language,
  then EFI_UNSUPPORTED is returned.

  @param  This                  A pointer to the EFI_COMPONENT_NAME2_PROTOCOL or
                                EFI_COMPONENT_NAME_PROTOCOL instance.
  @param  Language              A pointer to a Null-terminated ASCII string
                                array indicating the language. This is the
                                language of the driver name that the caller is
                                requesting, and it must match one of the
                                languages specified in SupportedLanguages. The
                                number of languages supported by a driver is up
                                to the driver writer. Language is specified
                                in RFC 4646 or ISO 639-2 language code format.
  @param  DriverName            A pointer to the Unicode string to return.
                                This Unicode string is the name of the
                                driver specified by This in the language
                                specified by Language.

  @retval EFI_SUCCESS           The Unicode string for the Driver specified by
                                This and the language specified by Language was
                                returned in DriverName.
  @retval EFI_INVALID_PARAMETER Language is NULL.
  @retval EFI_INVALID_PARAMETER DriverName is NULL.
  @retval EFI_UNSUPPORTED       The driver specified by This does not support
                                the language specified by Language.

**/
EFI_STATUS
EFIAPI
VgaClassComponentNameGetDriverName (
  IN  EFI_COMPONENT_NAME_PROTOCOL  *This,
  IN  CHAR8                        *Language,
  OUT CHAR16                       **DriverName
  );

/**
  Retrieves a Unicode string that is the user readable name of the controller
  that is being managed by a driver.

  This function retrieves the user readable name of the controller specified by
  ControllerHandle and ChildHandle in the form of a Unicode string. If the
  driver specified by This has a user readable name in the language specified by
  Language, then a pointer to the controller name is returned in ControllerName,
  and EFI_SUCCESS is returned.  If the driver specified by This is not currently
  managing the controller specified by ControllerHandle and ChildHandle,
  then EFI_UNSUPPORTED is returned.  If the driver specified by This does not
  support the language specified by Language, then EFI_UNSUPPORTED is returned.

  @param  This                  A pointer to the EFI_COMPONENT_NAME2_PROTOCOL or
                                EFI_COMPONENT_NAME_PROTOCOL instance.
  @param  ControllerHandle      The handle of a controller that the driver
                                specified by This is managing.  This handle
                                specifies the controller whose name is to be
                                returned.
  @param  ChildHandle           The handle of the child controller to retrieve
                                the name of.  This is an optional parameter that
                                may be NULL.  It will be NULL for device
                                drivers.  It will also be NULL for a bus drivers
                                that wish to retrieve the name of the bus
                                controller.  It will not be NULL for a bus
                                driver that wishes to retrieve the name of a
                                child controller.
  @param  Language              A pointer to a Null-terminated ASCII string
                                array indicating the language.  This is the
                                language of the driver name that the caller is
                                requesting, and it must match one of the
                                languages specified in SupportedLanguages. The
                                number of languages supported by a driver is up
                                to the driver writer. Language is specified in
                                RFC 4646 or ISO 639-2 language code format.
  @param  ControllerName        A pointer to the Unicode string to return.
                                This Unicode string is the name of the
                                controller specified by ControllerHandle and
                                ChildHandle in the language specified by
                                Language from the point of view of the driver
                                specified by This.

  @retval EFI_SUCCESS           The Unicode string for the user readable name in
                                the language specified by Language for the
                                driver specified by This was returned in
                                DriverName.
  @retval EFI_INVALID_PARAMETER ControllerHandle is NULL.
  @retval EFI_INVALID_PARAMETER ChildHandle is not NULL and it is not a valid
                                EFI_HANDLE.
  @retval EFI_INVALID_PARAMETER Language is NULL.
  @retval EFI_INVALID_PARAMETER ControllerName is NULL.
  @retval EFI_UNSUPPORTED       The driver specified by This is not currently
                                managing the controller specified by
                                ControllerHandle and ChildHandle.
  @retval EFI_UNSUPPORTED       The driver specified by This does not support
                                the language specified by Language.

**/
EFI_STATUS
EFIAPI
VgaClassComponentNameGetControllerName (
  IN  EFI_COMPONENT_NAME_PROTOCOL                     *This,
  IN  EFI_HANDLE                                      ControllerHandle,
  IN  EFI_HANDLE                                      ChildHandle        OPTIONAL,
  IN  CHAR8                                           *Language,
  OUT CHAR16                                          **ControllerName
  );

//
// Simple Text Output Protocol functions
//
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
  );

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
  );

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
  );

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
  );

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
  );

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
  );

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
  );

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
  );

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
  );

#endif
