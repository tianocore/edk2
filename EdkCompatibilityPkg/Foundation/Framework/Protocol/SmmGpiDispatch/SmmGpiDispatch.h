/*++

Copyright (c) 1999 - 2002, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.


Module Name:

    SmmGpiDispatch.h

Abstract:

    EFI Smm Gpi Smi Child Protocol

Revision History

--*/

#ifndef _EFI_SMM_GPI_DISPATCH_H_
#define _EFI_SMM_GPI_DISPATCH_H_

//
// Global ID for the GPI SMI Protocol
//
#define EFI_SMM_GPI_DISPATCH_PROTOCOL_GUID \
  { \
    0xe0744b81, 0x9513, 0x49cd, {0x8c, 0xea, 0xe9, 0x24, 0x5e, 0x70, 0x39, 0xda} \
  }

EFI_FORWARD_DECLARATION (EFI_SMM_GPI_DISPATCH_PROTOCOL);

//
// Related Definitions
//
//
// GpiMask is a bit mask of 32 possible general purpose inputs that can generate a
// a SMI.  Bit 0 corresponds to logical GPI[0], 1 corresponds to logical GPI[1], etc.
//
// The logical GPI index to physical pin on device is described by the GPI device name
// found on the same handle as the GpiSmi child dispatch protocol.  The GPI device name
// is defined as protocol with a GUID name and NULL protocol pointer.
//
typedef struct {
  UINTN GpiNum;
} EFI_SMM_GPI_DISPATCH_CONTEXT;

//
// Member functions
//
typedef
VOID
(EFIAPI *EFI_SMM_GPI_DISPATCH) (
  IN  EFI_HANDLE                    DispatchHandle,
  IN  EFI_SMM_GPI_DISPATCH_CONTEXT  * DispatchContext
  );

/*++

  Routine Description:
    Dispatch function for a GPI SMI handler.

  Arguments:
    DispatchHandle      - Handle of this dispatch function.
    DispatchContext     - Pointer to the dispatch function's context.
                          The DispatchContext fields are filled in
                          by the dispatching driver prior to
                          invoking this dispatch function.

  Returns:
    Nothing

--*/
typedef
EFI_STATUS
(EFIAPI *EFI_SMM_GPI_REGISTER) (
  IN EFI_SMM_GPI_DISPATCH_PROTOCOL            * This,
  IN  EFI_SMM_GPI_DISPATCH                    DispatchFunction,
  IN  EFI_SMM_GPI_DISPATCH_CONTEXT            * DispatchContext,
  OUT EFI_HANDLE                              * DispatchHandle
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
                            function the GPI(s) for which the dispatch function
                            should be invoked.
    DispatchHandle        - Handle of dispatch function, for when interfacing
                            with the parent Sx state SMM driver.

  Returns:
    EFI_SUCCESS           - The dispatch function has been successfully
                            registered and the SMI source has been enabled.
    EFI_DEVICE_ERROR      - The driver was unable to enable the SMI source.
    EFI_OUT_OF_RESOURCES  - Not enough memory (system or SMM) to manage this
                            child.
    EFI_INVALID_PARAMETER - DispatchContext is invalid. The GPI input value
                            is not within valid range.

--*/
typedef
EFI_STATUS
(EFIAPI *EFI_SMM_GPI_UNREGISTER) (
  IN EFI_SMM_GPI_DISPATCH_PROTOCOL            * This,
  IN  EFI_HANDLE                              DispatchHandle
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
// Interface structure for the SMM GPI SMI Dispatch Protocol
//
struct _EFI_SMM_GPI_DISPATCH_PROTOCOL {
  EFI_SMM_GPI_REGISTER    Register;
  EFI_SMM_GPI_UNREGISTER  UnRegister;
  UINTN                   NumSupportedGpis;
};

extern EFI_GUID gEfiSmmGpiDispatchProtocolGuid;

#endif
