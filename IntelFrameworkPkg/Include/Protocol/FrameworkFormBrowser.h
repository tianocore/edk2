/** @file
  The EFI_FORM_BROWSER_PROTOCOL is the interface to the EFI
  Configuration Driver.  This interface enables the caller to direct the
  configuration driver to use either the HII database or the passed-in
  packet of data.  This will also allow the caller to post messages
  into the configuration drivers internal mailbox.

Copyright (c) 2006 - 2010, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials are licensed and made available under 
the terms and conditions of the BSD License that accompanies this distribution.  
The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php.                                          
    
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

  Module Name:  FrameworkFormBrowser.h

  @par Revision Reference:
  This protocol is defined in HII spec 0.92.

**/

#ifndef __FRAMEWORK_FORM_BROWSER_H__
#define __FRAMEWORK_FORM_BROWSER_H__

#include <Protocol/FrameworkHii.h>


#define EFI_FORM_BROWSER_PROTOCOL_GUID \
  { \
    0xe5a1333e, 0xe1b4, 0x4d55, {0xce, 0xeb, 0x35, 0xc3, 0xef, 0x13, 0x34, 0x43 } \
  }

#define EFI_FORM_BROWSER_COMPATIBILITY_PROTOCOL_GUID \
  { \
    0xfb7c852, 0xadca, 0x4853, { 0x8d, 0xf, 0xfb, 0xa7, 0x1b, 0x1c, 0xe1, 0x1a } \
  }

typedef struct _EFI_FORM_BROWSER_PROTOCOL EFI_FORM_BROWSER_PROTOCOL;

typedef struct {
  UINT32  Length;
  UINT16  Type;
  UINT8   Data[1];
} EFI_HII_PACKET;

typedef struct {
  EFI_HII_IFR_PACK    *IfrData;
  EFI_HII_STRING_PACK *StringData;
} EFI_IFR_PACKET;

typedef struct {
  UINTN LeftColumn;
  UINTN RightColumn;
  UINTN TopRow;
  UINTN BottomRow;
} FRAMEWORK_EFI_SCREEN_DESCRIPTOR;

/**
  Provides direction to the configuration driver whether to use the HII
  database or a passed-in set of data. This function also establishes a
  pointer to the calling driver's callback interface.

  @param  This                  A pointer to the EFI_FORM_BROWSER_PROTOCOL instance.
  @param  UseDatabase           Determines whether the HII database is to be
                                used to gather information. If the value is FALSE, 
                                the configuration driver will get the information 
                                provided in the passed-in Packet parameters.
  @param  Handle                A pointer to an array of HII handles to display. 
                                This value should correspond to the value of the 
                                HII form package that is required to be displayed.
  @param  HandleCount           The number of handles in the array specified by Handle.
  @param  Packet                A pointer to a set of data containing pointers to IFR
                                and/or string data.
  @param  CallbackHandle        The handle to the driver's callback interface.
                                This parameter is used only when the UseDatabase 
                                parameter is FALSE and an application wants to 
                                register a callback with the browser.
  @param  NvMapOverride         This buffer is used only when there is no NV variable
                                to define the current settings and the caller needs 
                                to provide to the browser the current settings for 
                                the "fake" NV variable.
  @param  ScreenDimensions      Allows the browser to be called so that it occupies
                                a portion of the physical screen instead of dynamically 
                                determining the screen dimensions.
  @param  ResetRequired         This BOOLEAN value denotes whether a reset is required
                                based on the data that might have been changed. 
                                The ResetRequired parameter is primarily applicable 
                                for configuration applications, and is an
                                optional parameter.

  @retval EFI_SUCCESS           The function completed successfully.
  @retval EFI_NOT_FOUND         The variable was not found.
  @retval EFI_BUFFER_TOO_SMALL  The DataSize is too small for the result.
                                DataSize has been updated with the size needed to 
                                complete the request.
  @retval EFI_INVALID_PARAMETER One of the parameters has an invalid value.
  @retval EFI_DEVICE_ERROR      The variable could not be saved due to a hardware failure.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_SEND_FORM)(
  IN  EFI_FORM_BROWSER_PROTOCOL       *This,
  IN  BOOLEAN                         UseDatabase,
  IN  FRAMEWORK_EFI_HII_HANDLE        *Handle,
  IN  UINTN                           HandleCount,
  IN  EFI_IFR_PACKET                  *Packet, OPTIONAL
  IN  EFI_HANDLE                      CallbackHandle, OPTIONAL
  IN  UINT8                           *NvMapOverride, OPTIONAL
  IN  FRAMEWORK_EFI_SCREEN_DESCRIPTOR *ScreenDimensions, OPTIONAL
  OUT BOOLEAN                         *ResetRequired OPTIONAL
  );

/**
  Routine used to abstract a generic dialog interface and return the selected
  key or string.

  @param  NumberOfLines         The number of lines for the dialog box.
  @param  HotKey                Defines whether a single character is parsed (TRUE)
                                and returned in KeyValue, or if a string is returned 
                                in StringBuffer.
  @param  MaximumStringSize     The maximum size in bytes of a typed-in string.
                                Because each character is a CHAR16, the minimum 
                                string returned is two bytes.
  @param  StringBuffer          The passed-in pointer to the buffer that will hold
                                the typed in string if HotKey is FALSE.
  @param  KeyValue              The EFI_INPUT_KEY value returned if HotKey is TRUE.
  @param  String                The pointer to the first string in the list of strings
                                that comprise the dialog box.
  @param  ...                   A series of NumberOfLines text strings that will be used
                                to construct the dialog box.

  @retval EFI_SUCCESS           The dialog was displayed and user interaction was received.
  @retval EFI_DEVICE_ERROR      The user typed in an ESC character to exit the routine.
  @retval EFI_INVALID_PARAMETER One of the parameters was invalid

**/
typedef
EFI_STATUS
(EFIAPI *EFI_CREATE_POP_UP)(
  IN  UINTN                           NumberOfLines,
  IN  BOOLEAN                         HotKey,
  IN  UINTN                           MaximumStringSize,
  OUT CHAR16                          *StringBuffer,
  OUT EFI_INPUT_KEY                   *KeyValue,
  IN  CHAR16                          *String,
  ...
  );

/**
  The EFI_FORM_BROWSER_PROTOCOL is the interface to call for drivers to
  leverage the EFI configuration driver interface.
**/
struct _EFI_FORM_BROWSER_PROTOCOL {
  ///
  /// Provides direction to the configuration driver whether to use the HII
  /// database or to use a passed-in set of data. This function also establishes
  /// a pointer to the calling driver's callback interface.
  ///
  EFI_SEND_FORM     SendForm;
  
  ///
  /// Routine used to abstract a generic dialog interface and return the
  /// selected key or string.  
  ///
  EFI_CREATE_POP_UP CreatePopUp;
};

extern EFI_GUID gEfiFormBrowserProtocolGuid;
extern EFI_GUID gEfiFormBrowserCompatibilityProtocolGuid;


#endif
