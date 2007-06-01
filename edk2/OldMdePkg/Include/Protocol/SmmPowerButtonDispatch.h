/** @file
  This file declares EFI Smm Power Button Smi Child Protocol

  Copyright (c) 2006, Intel Corporation                                                         
  All rights reserved. This program and the accompanying materials                          
  are licensed and made available under the terms and conditions of the BSD License         
  which accompanies this distribution.  The full text of the license may be found at        
  http://opensource.org/licenses/bsd-license.php                                            

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

  Module Name:  SmmPowerButtonDispatch.h

  @par Revision Reference:
  This Protocol is defined in Framework of EFI SMM Core Interface Spec
  Version 0.9.

**/

#ifndef _EFI_SMM_POWER_BUTTON_DISPATCH_H_
#define _EFI_SMM_POWER_BUTTON_DISPATCH_H_

//
// Global ID for the Power Button SMI Protocol
//
#define EFI_SMM_POWER_BUTTON_DISPATCH_PROTOCOL_GUID \
  { \
    0xb709efa0, 0x47a6, 0x4b41, {0xb9, 0x31, 0x12, 0xec, 0xe7, 0xa8, 0xee, 0x56 } \
  }

typedef struct _EFI_SMM_POWER_BUTTON_DISPATCH_PROTOCOL  EFI_SMM_POWER_BUTTON_DISPATCH_PROTOCOL;

//
// Related Definitions
//
//
// Power Button. Example, Use for changing LEDs before ACPI OS is on.
//    - DXE/BDS Phase
//    - OS Install Phase
//
typedef enum {
  PowerButtonEntry,
  PowerButtonExit
} EFI_POWER_BUTTON_PHASE;

typedef struct {
  EFI_POWER_BUTTON_PHASE  Phase;
} EFI_SMM_POWER_BUTTON_DISPATCH_CONTEXT;

//
// Member functions
//
/**
  Dispatch function for a Power Button SMI handler.

  @param  DispatchHandle        Handle of this dispatch function.
  @param  DispatchContext       Pointer to the dispatch function's context.
                                The DispatchContext fields are filled in
                                by the dispatching driver prior to
                                invoking this dispatch function.

  Nothing

**/
typedef
VOID
(EFIAPI *EFI_SMM_POWER_BUTTON_DISPATCH) (
  IN  EFI_HANDLE                             DispatchHandle,
  IN  EFI_SMM_POWER_BUTTON_DISPATCH_CONTEXT  *DispatchContext
  );

/**
  Register a child SMI source dispatch function with a parent SMM driver

  @param  This                  Protocol instance pointer.
  @param  DispatchFunction      Pointer to dispatch function to be invoked for
                                this SMI source
  @param  DispatchContext       Pointer to the dispatch function's context.
                                The caller fills this context in before calling
                                the register function to indicate to the register
                                function the Power Button SMI phase for which the dispatch
                                function should be invoked.
  @param  DispatchHandle        Handle of dispatch function, for when interfacing
                                with the parent Sx state SMM driver.

  @retval EFI_SUCCESS           The dispatch function has been successfully
                                registered and the SMI source has been enabled.
  @retval EFI_DEVICE_ERROR      The driver was unable to enable the SMI source.
  @retval EFI_OUT_OF_RESOURCES  Not enough memory (system or SMM) to manage this
                                child.
  @retval EFI_INVALID_PARAMETER DispatchContext is invalid. The Power Button SMI
                                phase is not within valid range.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_SMM_POWER_BUTTON_REGISTER) (
  IN EFI_SMM_POWER_BUTTON_DISPATCH_PROTOCOL            *This,
  IN  EFI_SMM_POWER_BUTTON_DISPATCH                    DispatchFunction,
  IN  EFI_SMM_POWER_BUTTON_DISPATCH_CONTEXT            *DispatchContext,
  OUT EFI_HANDLE                                       *DispatchHandle
  );

/**
  Unregister a child SMI source dispatch function with a parent SMM driver

  @param  This                  Protocol instance pointer.
  @param  DispatchHandle        Handle of dispatch function to deregister.

  @retval EFI_SUCCESS           The dispatch function has been successfully
                                unregistered and the SMI source has been disabled
                                if there are no other registered child dispatch
                                functions for this SMI source.
  @retval EFI_INVALID_PARAMETER Handle is invalid.
  @retval other                 TBD

**/
typedef
EFI_STATUS
(EFIAPI *EFI_SMM_POWER_BUTTON_UNREGISTER) (
  IN EFI_SMM_POWER_BUTTON_DISPATCH_PROTOCOL             *This,
  IN  EFI_HANDLE                                        DispatchHandle
  );

//
// Interface structure for the SMM Power Button SMI Dispatch Protocol
//
/**
  @par Protocol Description:
  Provides the parent dispatch service for the SMM power button SMI source generator.

  @param Register
  Installs a child service to be dispatched by this protocol.

  @param UnRegister
  Removes a child service dispatched by this protocol.

**/
struct _EFI_SMM_POWER_BUTTON_DISPATCH_PROTOCOL {
  EFI_SMM_POWER_BUTTON_REGISTER   Register;
  EFI_SMM_POWER_BUTTON_UNREGISTER UnRegister;
};

extern EFI_GUID gEfiSmmPowerButtonDispatchProtocolGuid;

#endif
