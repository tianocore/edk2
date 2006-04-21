/** @file
  This file declares EFI Smm USB Smi Child Protocol.

  Copyright (c) 2006, Intel Corporation                                                         
  All rights reserved. This program and the accompanying materials                          
  are licensed and made available under the terms and conditions of the BSD License         
  which accompanies this distribution.  The full text of the license may be found at        
  http://opensource.org/licenses/bsd-license.php                                            

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

  Module Name:  SmmUsbDispatch.h

  @par Revision Reference:
  This Protocol is defined in Framework of EFI SMM Core Interface Spec
  Version 0.9.
**/

#ifndef _EFI_SMM_USB_DISPATCH_H_
#define _EFI_SMM_USB_DISPATCH_H_

//
// Global ID for the USB Protocol
//
#define EFI_SMM_USB_DISPATCH_PROTOCOL_GUID \
  { \
    0xa05b6ffd, 0x87af, 0x4e42, {0x95, 0xc9, 0x62, 0x28, 0xb6, 0x3c, 0xf3, 0xf3 } \
  }

typedef struct _EFI_SMM_USB_DISPATCH_PROTOCOL  EFI_SMM_USB_DISPATCH_PROTOCOL;

//
// Related Definitions
//
typedef enum {
  UsbLegacy,
  UsbWake
} EFI_USB_SMI_TYPE;

typedef struct {
  EFI_USB_SMI_TYPE          Type;
  EFI_DEVICE_PATH_PROTOCOL  *Device;
} EFI_SMM_USB_DISPATCH_CONTEXT;

//
// Member functions
//
/**
  Dispatch function for a USB SMI handler.

  @param  DispatchHandle Handle of this dispatch function.
  @param  DispatchContext Pointer to the dispatch function's context.
  The DispatchContext fields are filled in
  by the dispatching driver prior to
  invoking this dispatch function.

  Nothing

**/
typedef
VOID
(EFIAPI *EFI_SMM_USB_DISPATCH) (
  IN  EFI_HANDLE                    DispatchHandle,
  IN  EFI_SMM_USB_DISPATCH_CONTEXT  *DispatchContext
  );

/**
  Register a child SMI source dispatch function with a parent SMM driver

  @param  This Protocol instance pointer.
  @param  DispatchFunction Pointer to dispatch function to be invoked for
  this SMI source
  @param  DispatchContext Pointer to the dispatch function's context.
  The caller fills this context in before calling
  the register function to indicate to the register
  function the USB SMI types for which the dispatch
  function should be invoked.
  @param  DispatchHandle Handle of dispatch function, for when interfacing
  with the parent Sx state SMM driver.

  @retval  EFI_SUCCESS The dispatch function has been successfully
  registered and the SMI source has been enabled.
  @retval  EFI_DEVICE_ERROR The driver was unable to enable the SMI source.
  @retval  EFI_OUT_OF_RESOURCES Not enough memory (system or SMM) to manage this
  child.
  @retval  EFI_INVALID_PARAMETER DispatchContext is invalid. The USB SMI type
  is not within valid range.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_SMM_USB_REGISTER) (
  IN EFI_SMM_USB_DISPATCH_PROTOCOL            *This,
  IN  EFI_SMM_USB_DISPATCH                    DispatchFunction,
  IN  EFI_SMM_USB_DISPATCH_CONTEXT            *DispatchContext,
  OUT EFI_HANDLE                              *DispatchHandle
  );

/**
  Unregister a child SMI source dispatch function with a parent SMM driver

  @param  This Protocol instance pointer.
  @param  DispatchHandle Handle of dispatch function to deregister.

  @retval  EFI_SUCCESS The dispatch function has been successfully
  unregistered and the SMI source has been disabled
  if there are no other registered child dispatch
  functions for this SMI source.
  @retval  EFI_INVALID_PARAMETER Handle is invalid.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_SMM_USB_UNREGISTER) (
  IN EFI_SMM_USB_DISPATCH_PROTOCOL            *This,
  IN  EFI_HANDLE                              DispatchHandle
  );

//
// Interface structure for the SMM USB SMI Dispatch Protocol
//
/**
  @par Protocol Description:
  Provides the parent dispatch service for the USB SMI source generator.

  @param Register
  Installs a child service to be dispatched by this protocol.

  @param UnRegister
  Removes a child service dispatched by this protocol.

**/
struct _EFI_SMM_USB_DISPATCH_PROTOCOL {
  EFI_SMM_USB_REGISTER    Register;
  EFI_SMM_USB_UNREGISTER  UnRegister;
};

extern EFI_GUID gEfiSmmUsbDispatchProtocolGuid;

#endif
