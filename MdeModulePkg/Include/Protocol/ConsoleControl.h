/** @file

  This protocol provides the interfaces to Get/Set the current video mode for GOP/UGA screen

Copyright (c) 2006 - 2008, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

**/

#ifndef __CONSOLE_CONTROL_H__
#define __CONSOLE_CONTROL_H__

#define EFI_CONSOLE_CONTROL_PROTOCOL_GUID \
  { 0xf42f7782, 0x12e, 0x4c12, {0x99, 0x56, 0x49, 0xf9, 0x43, 0x4, 0xf7, 0x21 } }

typedef struct _EFI_CONSOLE_CONTROL_PROTOCOL   EFI_CONSOLE_CONTROL_PROTOCOL;


typedef enum {
  EfiConsoleControlScreenText,     /// Text Mode
  EfiConsoleControlScreenGraphics, /// Graphics Mode
  EfiConsoleControlScreenMaxValue
} EFI_CONSOLE_CONTROL_SCREEN_MODE;

/**
  Return the current video mode information. Also returns info about existence
  of Graphics Output devices or UGA Draw devices in system, and whether the Std In device is locked. 
  GopUgaExists and StdInLocked parameters are optional.

  @param  This                    Protocol instance pointer.
  @param  Mode                    Current video mode.
  @param  GopExists               TRUE if GOP Spliter has found a GOP/UGA device
  @param  StdInLocked             TRUE if StdIn device is keyboard locked

  @retval EFI_SUCCESS             Video mode information is returned.
  @retval EFI_INVALID_PARAMETER   Invalid parameters if Mode == NULL.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_CONSOLE_CONTROL_PROTOCOL_GET_MODE)(
  IN  EFI_CONSOLE_CONTROL_PROTOCOL      *This,
  OUT EFI_CONSOLE_CONTROL_SCREEN_MODE   *Mode,
  OUT BOOLEAN                           *GopUgaExists,  OPTIONAL  
  OUT BOOLEAN                           *StdInLocked    OPTIONAL
  );

/**
  Set the current video mode to either text or graphics. 

  @param  This                    Protocol instance pointer.
  @param  Mode                    Video mode is to be set.

  @retval EFI_SUCCESS             Mode is set successfully.
  @retval EFI_INVALID_PARAMETER   Mode is not the valid mode value.
  @retval EFI_UNSUPPORTED         Mode is unsupported by console device.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_CONSOLE_CONTROL_PROTOCOL_SET_MODE)(
  IN  EFI_CONSOLE_CONTROL_PROTOCOL      *This,
  OUT EFI_CONSOLE_CONTROL_SCREEN_MODE   Mode
  );

/**
  Store the password, enable state variable and arm the periodic timer.
  If Password is NULL unlock the password state variable and set the event
  timer. If the Password is too big return an error. If the Password is valid
  Copy the Password and enable state variable and then arm the periodic timer

  @param  This                     Console Control protocol pointer.
  @param  Password                 The password input.

  @retval EFI_SUCCESS              Lock the StdIn device successfully.
  @retval EFI_INVALID_PARAMETER    Password is NULL
  @retval EFI_OUT_OF_RESOURCES     Buffer allocation to store the big password fails

**/
typedef
EFI_STATUS
(EFIAPI *EFI_CONSOLE_CONTROL_PROTOCOL_LOCK_STD_IN)(
  IN  EFI_CONSOLE_CONTROL_PROTOCOL      *This,
  IN  CHAR16                            *Password
  );

struct _EFI_CONSOLE_CONTROL_PROTOCOL {
  EFI_CONSOLE_CONTROL_PROTOCOL_GET_MODE           GetMode;
  EFI_CONSOLE_CONTROL_PROTOCOL_SET_MODE           SetMode;
  EFI_CONSOLE_CONTROL_PROTOCOL_LOCK_STD_IN        LockStdIn;
};

extern EFI_GUID gEfiConsoleControlProtocolGuid;

#endif
