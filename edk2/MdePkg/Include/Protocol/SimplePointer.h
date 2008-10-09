/** @file
  Simple Pointer protocol from the UEFI 2.0 specification.

  Abstraction of a very simple pointer device like a mice or tracekballs.

  Copyright (c) 2006 - 2008, Intel Corporation                                                         
  All rights reserved. This program and the accompanying materials                          
  are licensed and made available under the terms and conditions of the BSD License         
  which accompanies this distribution.  The full text of the license may be found at        
  http://opensource.org/licenses/bsd-license.php                                            

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

**/

#ifndef __SIMPLE_POINTER_H__
#define __SIMPLE_POINTER_H__

#define EFI_SIMPLE_POINTER_PROTOCOL_GUID \
  { \
    0x31878c87, 0xb75, 0x11d5, {0x9a, 0x4f, 0x0, 0x90, 0x27, 0x3f, 0xc1, 0x4d } \
  }

typedef struct _EFI_SIMPLE_POINTER_PROTOCOL  EFI_SIMPLE_POINTER_PROTOCOL;

//
// Data structures
//
typedef struct {
  INT32   RelativeMovementX;
  INT32   RelativeMovementY;
  INT32   RelativeMovementZ;
  BOOLEAN LeftButton;
  BOOLEAN RightButton;
} EFI_SIMPLE_POINTER_STATE;

typedef struct {
  UINT64  ResolutionX;
  UINT64  ResolutionY;
  UINT64  ResolutionZ;
  BOOLEAN LeftButton;
  BOOLEAN RightButton;
} EFI_SIMPLE_POINTER_MODE;

/**                                                                 
  Resets the pointer device hardware.
    
  @param  This                  A pointer to the EFI_SIMPLE_POINTER_PROTOCOL
                                instance.                                   
  @param  ExtendedVerification  Indicates that the driver may perform a more exhaustive
                                verification operation of the device during reset.                                       
                                
  @retval EFI_SUCCESS           The device was reset.
  @retval EFI_DEVICE_ERROR      The device is not functioning correctly and could not be reset.  
                                   
**/
typedef
EFI_STATUS
(EFIAPI *EFI_SIMPLE_POINTER_RESET)(
  IN EFI_SIMPLE_POINTER_PROTOCOL            *This,
  IN BOOLEAN                                ExtendedVerification
  );

/**                                                                 
  Retrieves the current state of a pointer device.
    
  @param  This                  A pointer to the EFI_SIMPLE_POINTER_PROTOCOL
                                instance.                                   
  @param  State                 A pointer to the state information on the pointer device.
                                
  @retval EFI_SUCCESS           The state of the pointer device was returned in State.
  @retval EFI_NOT_READY         The state of the pointer device has not changed since the last call to
                                GetState().                                                           
  @retval EFI_DEVICE_ERROR      A device error occurred while attempting to retrieve the pointer device's
                                current state.                                                           
                                   
**/
typedef
EFI_STATUS
(EFIAPI *EFI_SIMPLE_POINTER_GET_STATE)(
  IN EFI_SIMPLE_POINTER_PROTOCOL          *This,
  IN OUT EFI_SIMPLE_POINTER_STATE         *State
  );

/**  
  @par Protocol Description:
  The EFI_SIMPLE_POINTER_PROTOCOL provides a set of services for a pointer 
  device that can use used as an input device from an application written 
  to this specification. The services include the ability to reset the 
  pointer device, retrieve get the state of the pointer device, and 
  retrieve the capabilities of the pointer device.
**/
struct _EFI_SIMPLE_POINTER_PROTOCOL {
  EFI_SIMPLE_POINTER_RESET      Reset;
  EFI_SIMPLE_POINTER_GET_STATE  GetState;
  ///
  /// Event to use with WaitForEvent() to wait for input from the pointer device.
  ///
  EFI_EVENT                     WaitForInput;
  EFI_SIMPLE_POINTER_MODE       *Mode;
};

extern EFI_GUID gEfiSimplePointerProtocolGuid;

#endif
