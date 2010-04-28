/*++

Copyright (c) 2007, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  AbsolutePointer.h

Abstract:

  EFI_ABSOLUTE_POINTER_PROTOCOL from the UEFI 2.1 specification.

  This protocol specifies a simple method for accessing absolute pointer devices.  

--*/

#ifndef __ABSOLUTE_POINTER_H__
#define __ABSOLUTE_POINTER_H__

#define EFI_ABSOLUTE_POINTER_PROTOCOL_GUID \
  { \
      0x8D59D32B, 0xC655, 0x4AE9, {0x9B, 0x15, 0xF2, 0x59, 0x04, 0x99, 0x2A, 0x43} \
  }

EFI_FORWARD_DECLARATION (EFI_ABSOLUTE_POINTER_PROTOCOL);

//
// Data structures
//

typedef struct {
  UINT64 AbsoluteMinX;
  UINT64 AbsoluteMinY;
  UINT64 AbsoluteMinZ;
  UINT64 AbsoluteMaxX;
  UINT64 AbsoluteMaxY;
  UINT64 AbsoluteMaxZ;
  UINT32 Attributes;
} EFI_ABSOLUTE_POINTER_MODE;

typedef struct {
  UINT64 CurrentX;
  UINT64 CurrentY;
  UINT64 CurrentZ;
  UINT32 ActiveButtons;
} EFI_ABSOLUTE_POINTER_STATE;

#define EFI_ABSP_SupportsAltActive   0x00000001
#define EFI_ABSP_SupportsPressureAsZ 0x00000002

#define EFI_ABSP_TouchActive         0x00000001
#define EFI_ABS_AltActive            0x00000002   

typedef
EFI_STATUS
(EFIAPI *EFI_ABSOLUTE_POINTER_RESET) (
  IN EFI_ABSOLUTE_POINTER_PROTOCOL   *This,
  IN BOOLEAN                         ExtendedVerification
  )
/*++

  Routine Description:
    Resets the pointer device hardware.

  Arguments:
    This                  - Protocol instance pointer.
    ExtendedVerification  - Driver may perform diagnostics on reset.

  Returns:
    EFI_SUCCESS           - The device was reset.
    EFI_DEVICE_ERROR      - The device is not functioning correctly and could 
                            not be reset.
                            
--*/
;

typedef
EFI_STATUS
(EFIAPI *EFI_ABSOLUTE_POINTER_GET_STATE) (
  IN EFI_ABSOLUTE_POINTER_PROTOCOL   *This,
  IN OUT EFI_ABSOLUTE_POINTER_STATE  *State
  )
/*++

  Routine Description:
    Retrieves the current state of a pointer device.

  Arguments:
    This                  - Protocol instance pointer.
    State                 - A pointer to the state information on the pointer device.

  Returns:
    EFI_SUCCESS           - The state of the pointer device was returned in State..
    EFI_NOT_READY         - The state of the pointer device has not changed since the last call to
                            GetState().                                                           
    EFI_DEVICE_ERROR      - A device error occurred while attempting to retrieve the pointer
                            device's current state.                                         
--*/
;

struct _EFI_ABSOLUTE_POINTER_PROTOCOL {
  EFI_ABSOLUTE_POINTER_RESET         Reset;
  EFI_ABSOLUTE_POINTER_GET_STATE     GetState;
  EFI_EVENT                          WaitForInput;
  EFI_ABSOLUTE_POINTER_MODE          *Mode;
};

extern EFI_GUID gEfiAbsolutePointerProtocolGuid;

#endif
