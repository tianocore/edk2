/** @file
  Header file for GraphicsConsole driver.

Copyright (c) 2006 - 2008, Intel Corporation. <BR>
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _GRAPHICS_CONSOLE_H_
#define _GRAPHICS_CONSOLE_H_

#include <PiDxe.h>
#include <Protocol/SimpleTextOut.h>
#include <Protocol/GraphicsOutput.h>
#include <Protocol/UgaDraw.h>
#include <Protocol/DevicePath.h>
#include <Library/DebugLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/HiiLib.h>
#include <Library/BaseLib.h>
#include <Library/PcdLib.h>

#include <MdeModuleHii.h>

#include <Protocol/HiiFont.h>
#include <Protocol/HiiDatabase.h>


extern EFI_COMPONENT_NAME_PROTOCOL   gGraphicsConsoleComponentName;
extern EFI_COMPONENT_NAME2_PROTOCOL  gGraphicsConsoleComponentName2;

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

  @param  This[in]              A pointer to the EFI_COMPONENT_NAME2_PROTOCOL or
                                EFI_COMPONENT_NAME_PROTOCOL instance.

  @param  Language[in]          A pointer to a Null-terminated ASCII string
                                array indicating the language. This is the
                                language of the driver name that the caller is
                                requesting, and it must match one of the
                                languages specified in SupportedLanguages. The
                                number of languages supported by a driver is up
                                to the driver writer. Language is specified
                                in RFC 3066 or ISO 639-2 language code format.

  @param  DriverName[out]       A pointer to the Unicode string to return.
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
GraphicsConsoleComponentNameGetDriverName (
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

  @param  This[in]              A pointer to the EFI_COMPONENT_NAME2_PROTOCOL or
                                EFI_COMPONENT_NAME_PROTOCOL instance.

  @param  ControllerHandle[in]  The handle of a controller that the driver
                                specified by This is managing.  This handle
                                specifies the controller whose name is to be
                                returned.

  @param  ChildHandle[in]       The handle of the child controller to retrieve
                                the name of.  This is an optional parameter that
                                may be NULL.  It will be NULL for device
                                drivers.  It will also be NULL for a bus drivers
                                that wish to retrieve the name of the bus
                                controller.  It will not be NULL for a bus
                                driver that wishes to retrieve the name of a
                                child controller.

  @param  Language[in]          A pointer to a Null-terminated ASCII string
                                array indicating the language.  This is the
                                language of the driver name that the caller is
                                requesting, and it must match one of the
                                languages specified in SupportedLanguages. The
                                number of languages supported by a driver is up
                                to the driver writer. Language is specified in
                                RFC 3066 or ISO 639-2 language code format.

  @param  ControllerName[out]   A pointer to the Unicode string to return.
                                This Unicode string is the name of the
                                controller specified by ControllerHandle and
                                ChildHandle in the language specified by
                                Language from the point of view of the driver
                                specified by This.

  @retval EFI_SUCCESS           The Unicode string for the user readable name in
                                the language specified by Language for the
                                driver specified by This was returned in
                                DriverName.

  @retval EFI_INVALID_PARAMETER ControllerHandle is not a valid EFI_HANDLE.

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
GraphicsConsoleComponentNameGetControllerName (
  IN  EFI_COMPONENT_NAME_PROTOCOL                     *This,
  IN  EFI_HANDLE                                      ControllerHandle,
  IN  EFI_HANDLE                                      ChildHandle        OPTIONAL,
  IN  CHAR8                                           *Language,
  OUT CHAR16                                          **ControllerName
  );


//
// User can define valid graphic resolution here
// e.g. 640x480, 800x600, 1024x768...
//
#define CURRENT_HORIZONTAL_RESOLUTION  800
#define CURRENT_VERTICAL_RESOLUTION    600

typedef union {
  EFI_NARROW_GLYPH  NarrowGlyph;
  EFI_WIDE_GLYPH    WideGlyph;
} GLYPH_UNION;

extern EFI_NARROW_GLYPH  gUsStdNarrowGlyphData[];

//
// Device Structure
//
#define GRAPHICS_CONSOLE_DEV_SIGNATURE  EFI_SIGNATURE_32 ('g', 's', 't', 'o')

typedef struct {
  UINTN   Columns;
  UINTN   Rows;
  INTN    DeltaX;
  INTN    DeltaY;
  UINT32  GopWidth;
  UINT32  GopHeight;
  UINT32  GopModeNumber;
} GRAPHICS_CONSOLE_MODE_DATA;

#define GRAPHICS_MAX_MODE 4

typedef struct {
  UINTN                            Signature;
  EFI_GRAPHICS_OUTPUT_PROTOCOL     *GraphicsOutput;
  EFI_UGA_DRAW_PROTOCOL            *UgaDraw;
  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL  SimpleTextOutput;
  EFI_SIMPLE_TEXT_OUTPUT_MODE      SimpleTextOutputMode;
  GRAPHICS_CONSOLE_MODE_DATA       ModeData[GRAPHICS_MAX_MODE];
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL    *LineBuffer;
  EFI_HII_HANDLE                   HiiHandle;
} GRAPHICS_CONSOLE_DEV;

#define GRAPHICS_CONSOLE_CON_OUT_DEV_FROM_THIS(a) \
  CR (a, GRAPHICS_CONSOLE_DEV, SimpleTextOutput, GRAPHICS_CONSOLE_DEV_SIGNATURE)

//
// Global Variables
//
extern EFI_DRIVER_BINDING_PROTOCOL   gGraphicsConsoleDriverBinding;


/**
  Returns available Unicode glyphs narrow fonts(8*19 pixels) size.

  @return Narrow foun size.

**/
UINTN
ReturnNarrowFontSize (
  VOID
  );

/**
  Implements SIMPLE_TEXT_OUTPUT.Reset().
  If ExtendeVerification is TRUE, then perform dependent Graphics Console
  device reset, and set display mode to mode 0.
  If ExtendedVerification is FALSE, only set display mode to mode 0.

  @param  This                  Indicates the calling context.
  @param  ExtendedVerification  Indicates that the driver may perform a more
                                exhaustive verification operation of the device
                                during reset.

  @return EFI_SUCCESS
  @return The reset operation succeeds.
  @return EFI_DEVICE_ERROR
  @return The Graphics Console is not functioning correctly

**/
EFI_STATUS
EFIAPI
GraphicsConsoleConOutReset (
  IN  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL    *This,
  IN  BOOLEAN                            ExtendedVerification
  );

/**
  Write a Unicode string to the output device.

  Implements SIMPLE_TEXT_OUTPUT.OutputString(). 
  The Unicode string will be converted to Glyphs and will be
  sent to the Graphics Console.

  @param  This                    Protocol instance pointer.
  @param  WString                 The NULL-terminated Unicode string to be displayed
                                  on the output device(s). All output devices must
                                  also support the Unicode drawing defined in this file.

  @retval EFI_SUCCESS             The string was output to the device.
  @retval EFI_DEVICE_ERROR        The device reported an error while attempting to output
                                  the text.
  @retval EFI_UNSUPPORTED         The output device's mode is not currently in a
                                  defined text mode.
  @retval EFI_WARN_UNKNOWN_GLYPH  This warning code indicates that some of the
                                  characters in the Unicode string could not be
                                  rendered and were skipped.

**/
EFI_STATUS
EFIAPI
GraphicsConsoleConOutOutputString (
  IN  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL  *This,
  IN  CHAR16                           *WString
  );

/**
  Implements SIMPLE_TEXT_OUTPUT.TestString().
  If one of the characters in the *Wstring is
  neither valid valid Unicode drawing characters,
  not ASCII code, then this function will return
  EFI_UNSUPPORTED.

  @param  This                  Indicates the calling context.
  @param  WString               The Null-terminated Unicode string to be tested.

  @return EFI_SUCCESS
  @return The Graphics Console is capable of rendering the output string.
  @return EFI_UNSUPPORTED
  @return Some of the characters in the Unicode string cannot be rendered.

**/
EFI_STATUS
EFIAPI
GraphicsConsoleConOutTestString (
  IN  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL  *This,
  IN  CHAR16                           *WString
  );

/**
  Returns information for an available text mode that the output device(s)
  supports

  Implements SIMPLE_TEXT_OUTPUT.QueryMode().
  It returnes information for an available text mode that the Graphics Console supports.
  In this driver,we only support text mode 80x25, which is defined as mode 0.

  @param  This                  Protocol instance pointer.
  @param  ModeNumber            The mode number to return information on.
  @param  Columns               The returned columns of the requested mode.
  @param  Rows                  The returned rows of the requested mode.

  @retval EFI_SUCCESS           The requested mode information is returned.
  @retval EFI_UNSUPPORTED       The mode number is not valid.

**/
EFI_STATUS
EFIAPI
GraphicsConsoleConOutQueryMode (
  IN  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL  *This,
  IN  UINTN                            ModeNumber,
  OUT UINTN                            *Columns,
  OUT UINTN                            *Rows
  );


/**
  Sets the output device(s) to a specified mode.
  
  Implements SIMPLE_TEXT_OUTPUT.SetMode().
  Set the Graphics Console to a specified mode. In this driver, we only support mode 0.

  @param  This                  Protocol instance pointer.
  @param  ModeNumber            The text mode to set.

  @retval EFI_SUCCESS           The requested text mode is set.
  @retval EFI_DEVICE_ERROR      The requested text mode cannot be set because of 
                                Graphics Console device error.
  @retval EFI_UNSUPPORTED       The text mode number is not valid.

**/
EFI_STATUS
EFIAPI
GraphicsConsoleConOutSetMode (
  IN  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL  *This,
  IN  UINTN                            ModeNumber
  );

/**
  Implements SIMPLE_TEXT_OUTPUT.SetAttribute().

  @param  This                  Indicates the calling context.
  @param  Attribute             The attribute to set. Only bit0..6 are valid, all
                                other bits are undefined and must be zero.

  @return EFI_SUCCESS           The requested attribute is set.
  @return EFI_DEVICE_ERROR      The requested attribute cannot be set due to Graphics Console port error.
  @return EFI_UNSUPPORTED       The attribute requested is not defined by EFI spec.

**/
EFI_STATUS
EFIAPI
GraphicsConsoleConOutSetAttribute (
  IN  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL  *This,
  IN  UINTN                            Attribute
  );

/**
  Clears the output device(s) display to the currently selected background 
  color.

  Implements SIMPLE_TEXT_OUTPUT.ClearScreen().

  @param  This                  Protocol instance pointer.

  @retval  EFI_SUCCESS      The operation completed successfully.
  @retval  EFI_DEVICE_ERROR The device had an error and could not complete the request.
  @retval  EFI_UNSUPPORTED  The output device is not in a valid text mode.

**/
EFI_STATUS
EFIAPI
GraphicsConsoleConOutClearScreen (
  IN  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL  *This
  );

/**
  Implements SIMPLE_TEXT_OUTPUT.SetCursorPosition().

  @param  This                  Indicates the calling context.
  @param  Column                The row to set cursor to.
  @param  Row                   The column to set cursor to.

  @return EFI_SUCCESS
  @return The operation completed successfully.
  @return EFI_DEVICE_ERROR
  @return The request fails due to Graphics Console device error.
  @return EFI_UNSUPPORTED
  @return The Graphics Console is not in a valid text mode, or the cursor position
  @return is invalid for current mode.

**/
EFI_STATUS
EFIAPI
GraphicsConsoleConOutSetCursorPosition (
  IN  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL  *This,
  IN  UINTN                            Column,
  IN  UINTN                            Row
  );


/**
  Makes the cursor visible or invisible.

  Implements SIMPLE_TEXT_OUTPUT.EnableCursor().

  @param  This                  Protocol instance pointer.
  @param  Visible               If TRUE, the cursor is set to be visible, If FALSE,
                                the cursor is set to be invisible.

  @retval EFI_SUCCESS           The operation completed successfully.

**/
EFI_STATUS
EFIAPI
GraphicsConsoleConOutEnableCursor (
  IN  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL  *This,
  IN  BOOLEAN                          Visible
  );

/**
  Test to see if Graphics Console could be supported on the Controller.

  Graphics Console could be supported if Graphics Output Protocol or UGA Draw
  Protocol exists on the Controller. (UGA Draw Protocol could be skipped
  if PcdUgaConsumeSupport is set to FALSE.)

  @param  This                Protocol instance pointer.
  @param  Controller          Handle of device to test.
  @param  RemainingDevicePath Optional parameter use to pick a specific child
                              device to start.

  @retval EFI_SUCCESS         This driver supports this device.
  @retval other               This driver does not support this device.

**/
EFI_STATUS
EFIAPI
GraphicsConsoleControllerDriverSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL    *This,
  IN EFI_HANDLE                     Controller,
  IN EFI_DEVICE_PATH_PROTOCOL       *RemainingDevicePath
  );


/**
  Start this driver on Controller by opening Graphics Output protocol or 
  UGA Draw protocol, and installing Simple Text Out protocol on Controller.
  (UGA Draw protocol could be shkipped if PcdUgaConsumeSupport is set to FALSE.)
  
  @param  This                 Protocol instance pointer.
  @param  Controller           Handle of device to bind driver to
  @param  RemainingDevicePath  Optional parameter use to pick a specific child
                               device to start.

  @retval EFI_SUCCESS          This driver is added to Controller.
  @retval other                This driver does not support this device.

**/
EFI_STATUS
EFIAPI
GraphicsConsoleControllerDriverStart (
  IN EFI_DRIVER_BINDING_PROTOCOL    *This,
  IN EFI_HANDLE                     Controller,
  IN EFI_DEVICE_PATH_PROTOCOL       *RemainingDevicePath
  );

/**
  Stop this driver on Controller by removing Simple Text Out protocol 
  and closing the Graphics Output Protocol or UGA Draw protocol on Controller.
  (UGA Draw protocol could be shkipped if PcdUgaConsumeSupport is set to FALSE.)
  

  @param  This              Protocol instance pointer.
  @param  Controller        Handle of device to stop driver on
  @param  NumberOfChildren  Number of Handles in ChildHandleBuffer. If number of
                            children is zero stop the entire bus driver.
  @param  ChildHandleBuffer List of Child Handles to Stop.

  @retval EFI_SUCCESS       This driver is removed Controller.
  @retval EFI_NOT_STARTED   Simple Text Out protocol could not be found the 
                            Controller.
  @retval other             This driver was not removed from this device.

**/
EFI_STATUS
EFIAPI
GraphicsConsoleControllerDriverStop (
  IN  EFI_DRIVER_BINDING_PROTOCOL    *This,
  IN  EFI_HANDLE                     Controller,
  IN  UINTN                          NumberOfChildren,
  IN  EFI_HANDLE                     *ChildHandleBuffer
  );


/**
  Locate HII Database protocol and HII Font protocol.

  @retval  EFI_SUCCESS     HII Database protocol and HII Font protocol 
                           are located successfully.
  @return  other           Failed to locate HII Database protocol or 
                           HII Font protocol.

**/
EFI_STATUS
EfiLocateHiiProtocol (
  VOID
  )
;


#endif
