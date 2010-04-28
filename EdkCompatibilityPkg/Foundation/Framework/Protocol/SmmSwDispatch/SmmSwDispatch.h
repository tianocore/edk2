/*++

Copyright (c) 2006, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.


Module Name:

    SmmSwDispatch.h

Abstract:

    EFI Smm Software Smi Child Protocol

Revision History

--*/

#ifndef _EFI_SMM_SW_DISPATCH_H_
#define _EFI_SMM_SW_DISPATCH_H_

//
// Global ID for the SW SMI Protocol
//
#define EFI_SMM_SW_DISPATCH_PROTOCOL_GUID \
  { \
    0xe541b773, 0xdd11, 0x420c, {0xb0, 0x26, 0xdf, 0x99, 0x36, 0x53, 0xf8, 0xbf} \
  }

EFI_FORWARD_DECLARATION (EFI_SMM_SW_DISPATCH_PROTOCOL);

//
// Related Definitions
//
//
// A particular chipset may not support all possible software SMI input values.
// For example, the ICH supports only values 00h to 0FFh.  The parent only allows a single
// child registration for each SwSmiInputValue.
//
typedef struct {
  UINTN SwSmiInputValue;
} EFI_SMM_SW_DISPATCH_CONTEXT;

//
// Member functions
//
typedef
VOID
(EFIAPI *EFI_SMM_SW_DISPATCH) (
  IN  EFI_HANDLE                    DispatchHandle,
  IN  EFI_SMM_SW_DISPATCH_CONTEXT   * DispatchContext
  );

/*++

  Routine Description:
    Dispatch function for a Software SMI handler.

  Arguments:
    DispatchHandle      - Handle of this dispatch function.
    DispatchContext     - Pointer to the dispatch function's context.
                          The SwSmiInputValue field is filled in
                          by the software dispatch driver prior to
                          invoking this dispatch function.
                          The dispatch function will only be called
                          for input values for which it is registered.

  Returns:
    Nothing

--*/
typedef
EFI_STATUS
(EFIAPI *EFI_SMM_SW_REGISTER) (
  IN EFI_SMM_SW_DISPATCH_PROTOCOL           * This,
  IN  EFI_SMM_SW_DISPATCH                   DispatchFunction,
  IN  EFI_SMM_SW_DISPATCH_CONTEXT           * DispatchContext,
  OUT EFI_HANDLE                            * DispatchHandle
  );

/*++

  Routine Description:
    Register a child SMI source dispatch function with a parent SMM driver

  Arguments:
    This                  - Protocol instance pointer.
    DispatchFunction      - Pointer to dispatch function to be invoked for
                            this SMI source
    DispatchContext       - Pointer to the dispatch function's context.
                            The caller fills this context in before calling
                            the register function to indicate to the register
                            function which Software SMI input value the
                            dispatch function should be invoked for.
    DispatchHandle        - Handle of dispatch function, for when interfacing
                            with the parent Sx state SMM driver.

  Returns:
    EFI_SUCCESS           - The dispatch function has been successfully
                            registered and the SMI source has been enabled.
    EFI_DEVICE_ERROR      - The SW driver was unable to enable the SMI source.
    EFI_OUT_OF_RESOURCES  - Not enough memory (system or SMM) to manage this
                            child.
    EFI_INVALID_PARAMETER - DispatchContext is invalid. The SW SMI input value
                            is not within valid range.

--*/
typedef
EFI_STATUS
(EFIAPI *EFI_SMM_SW_UNREGISTER) (
  IN EFI_SMM_SW_DISPATCH_PROTOCOL           * This,
  IN  EFI_HANDLE                            DispatchHandle
  );

/*++

  Routine Description:
    Unregister a child SMI source dispatch function with a parent SMM driver

  Arguments:
    This                  - Protocol instance pointer.
    DispatchHandle        - Handle of dispatch function to deregister.

  Returns:
    EFI_SUCCESS           - The dispatch function has been successfully 
                            unregistered and the SMI source has been disabled
                            if there are no other registered child dispatch
                            functions for this SMI source.
    EFI_INVALID_PARAMETER - Handle is invalid.
    other                 - TBD

--*/

//
// Interface structure for the SMM Software SMI Dispatch Protocol
//
struct _EFI_SMM_SW_DISPATCH_PROTOCOL {
  EFI_SMM_SW_REGISTER   Register;
  EFI_SMM_SW_UNREGISTER UnRegister;
  UINTN                 MaximumSwiValue;
};

extern EFI_GUID gEfiSmmSwDispatchProtocolGuid;

#endif
