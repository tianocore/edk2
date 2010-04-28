/*++

Copyright (c) 1999 - 2002, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.


Module Name:

    SmmSxDispatch.h

Abstract:

    EFI Smm Sx Smi Child Protocol

Revision History

--*/

#ifndef _EFI_SMM_SX_DISPATCH_H_
#define _EFI_SMM_SX_DISPATCH_H_

//
// Global ID for the Sx SMI Protocol
//
#define EFI_SMM_SX_DISPATCH_PROTOCOL_GUID \
  { \
    0x14fc52be, 0x1dc, 0x426c, {0x91, 0xae, 0xa2, 0x3c, 0x3e, 0x22, 0xa, 0xe8} \
  }

EFI_FORWARD_DECLARATION (EFI_SMM_SX_DISPATCH_PROTOCOL);

//
// Related Definitions
//
typedef enum {
  SxS0,
  SxS1,
  SxS2,
  SxS3,
  SxS4,
  SxS5,
  EfiMaximumSleepType
} EFI_SLEEP_TYPE;

typedef enum {
  SxEntry,
  SxExit,
  EfiMaximumPhase
} EFI_SLEEP_PHASE;

typedef struct {
  EFI_SLEEP_TYPE  Type;
  EFI_SLEEP_PHASE Phase;
} EFI_SMM_SX_DISPATCH_CONTEXT;

//
// Member functions
//
typedef
VOID
(EFIAPI *EFI_SMM_SX_DISPATCH) (
  IN  EFI_HANDLE                    DispatchHandle,
  IN  EFI_SMM_SX_DISPATCH_CONTEXT   * DispatchContext
  );

/*++

  Routine Description:
    Dispatch function for a Sx state SMI handler.

  Arguments:
    DispatchHandle      - Handle of this dispatch function.
    DispatchContext     - Pointer to the dispatch function's context.
                          The Type and Phase fields are filled in
                          by the Sx dispatch driver prior to invoking
                          this dispatch function.
                          For this intertace, the Sx driver will call the
                          dispatch function for all Sx type and phases,
                          so the Sx state handler(s) must check the Type
                          and Phase field of the Dispatch context and act
                          accordingly.

  Returns:
    Nothing

--*/
typedef
EFI_STATUS
(EFIAPI *EFI_SMM_SX_REGISTER) (
  IN EFI_SMM_SX_DISPATCH_PROTOCOL           * This,
  IN  EFI_SMM_SX_DISPATCH                   DispatchFunction,
  IN  EFI_SMM_SX_DISPATCH_CONTEXT           * DispatchContext,
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
                            function which Sx state type and phase the caller
                            wishes to be called back on.  For this intertace,
                            the Sx driver will call the registered handlers for
                            all Sx type and phases, so the Sx state handler(s)
                            must check the Type and Phase field of the Dispatch
                            context and act accordingly.
    DispatchHandle        - Handle of dispatch function, for when interfacing
                            with the parent Sx state SMM driver.

  Returns:
    EFI_SUCCESS           - The dispatch function has been successfully
                            registered and the SMI source has been enabled.
    EFI_UNSUPPORTED       - The Sx driver or hardware does not support that
                            Sx Type/Phase.
    EFI_DEVICE_ERROR      - The Sx driver was unable to enable the SMI source.
    EFI_OUT_OF_RESOURCES  - Not enough memory (system or SMM) to manage this
                            child.
    EFI_INVALID_PARAMETER - DispatchContext is invalid. Type & Phase are not
                            within valid range.

--*/
typedef
EFI_STATUS
(EFIAPI *EFI_SMM_SX_UNREGISTER) (
  IN EFI_SMM_SX_DISPATCH_PROTOCOL           * This,
  IN  EFI_HANDLE                            DispatchHandle
  );

/*++

  Routine Description:
    Unregister a child SMI source dispatch function with a parent SMM driver

  Arguments:
    This                  - Protocol instance pointer.
    DispatchHandle        - Handle of dispatch function to deregister.

  Returns:
    EFI_SUCCESS           - The dispatch function has been successfully unregistered and the
                            SMI source has been disabled if there are no other registered child
                            dispatch functions for this SMI source.
    EFI_INVALID_PARAMETER - Handle is invalid.
    other                 - TBD

--*/

//
// Interface structure for the SMM Child Dispatch Protocol
//
struct _EFI_SMM_SX_DISPATCH_PROTOCOL {
  EFI_SMM_SX_REGISTER   Register;
  EFI_SMM_SX_UNREGISTER UnRegister;
};

extern EFI_GUID gEfiSmmSxDispatchProtocolGuid;

#endif
