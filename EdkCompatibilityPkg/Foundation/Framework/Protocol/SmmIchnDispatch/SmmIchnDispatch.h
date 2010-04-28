/*++

Copyright (c) 1999 - 2007, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.


Module Name:

    SmmIchnDispatch.h

Abstract:

    EFI Smm ICH [N] Specific Smi Child Protocol

Revision History

--*/

#ifndef _EFI_SMM_ICHN_DISPATCH_H_
#define _EFI_SMM_ICHN_DISPATCH_H_

//
// Global ID for the ICH SMI Protocol
//
#define EFI_SMM_ICHN_DISPATCH_PROTOCOL_GUID \
  { \
    0xc50b323e, 0x9075, 0x4f2a, {0xac, 0x8e, 0xd2, 0x59, 0x6a, 0x10, 0x85, 0xcc} \
  }

EFI_FORWARD_DECLARATION (EFI_SMM_ICHN_DISPATCH_PROTOCOL);

//
// Related Definitions
//
//
// ICHN Specific SMIs.  These are miscellaneous SMI sources that are supported by the
// ICHN specific SMI implementation.  These may change over time.  TrapNumber is only
// valid if the Type is Trap.
//
typedef enum {
  //
  // NOTE: NEVER delete items from this list/enumeration!  Doing so will prevent other versions
  // of the code from compiling.  If the ICH version your driver is written for doesn't support
  // some of these SMIs, then simply return EFI_UNSUPPORTED when a child/client tries to register
  // for them.
  //
  IchnMch,
  IchnPme,
  IchnRtcAlarm,
  IchnRingIndicate,
  IchnAc97Wake,
  IchnSerialIrq,
  IchnY2KRollover,
  IchnTcoTimeout,
  IchnOsTco,
  IchnNmi,
  IchnIntruderDetect,
  IchnBiosWp,
  IchnMcSmi,
  IchnPmeB0,
  IchnThrmSts,
  IchnSmBus,
  IchnIntelUsb2,
  IchnMonSmi7,
  IchnMonSmi6,
  IchnMonSmi5,
  IchnMonSmi4,
  IchnDevTrap13,
  IchnDevTrap12,
  IchnDevTrap11,
  IchnDevTrap10,
  IchnDevTrap9,
  IchnDevTrap8,
  IchnDevTrap7,
  IchnDevTrap6,
  IchnDevTrap5,
  IchnDevTrap3,
  IchnDevTrap2,
  IchnDevTrap1,
  IchnDevTrap0,
  IchnIoTrap3,
  IchnIoTrap2,
  IchnIoTrap1,
  IchnIoTrap0,
  IchnPciExpress,
  IchnMonitor,
  IchnSpi,
  IchnQRT,
  IchnGpioUnlock,
  //
  // INSERT NEW ITEMS JUST BEFORE THIS LINE
  //
  NUM_ICHN_TYPES  // the number of items in this enumeration
} EFI_SMM_ICHN_SMI_TYPE;

typedef struct {
  EFI_SMM_ICHN_SMI_TYPE Type;
} EFI_SMM_ICHN_DISPATCH_CONTEXT;

//
// Member functions
//
typedef
VOID
(EFIAPI *EFI_SMM_ICHN_DISPATCH) (
  IN  EFI_HANDLE                      DispatchHandle,
  IN  EFI_SMM_ICHN_DISPATCH_CONTEXT   * DispatchContext
  );

/*++

  Routine Description:
    Dispatch function for a ICH n specific SMI handler.

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
(EFIAPI *EFI_SMM_ICHN_REGISTER) (
  IN EFI_SMM_ICHN_DISPATCH_PROTOCOL             * This,
  IN  EFI_SMM_ICHN_DISPATCH                     DispatchFunction,
  IN  EFI_SMM_ICHN_DISPATCH_CONTEXT             * DispatchContext,
  OUT EFI_HANDLE                                * DispatchHandle
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
                            function the ICHN SMI source for which the dispatch
                            function should be invoked.
    DispatchHandle        - Handle of dispatch function, for when interfacing
                            with the parent Sx state SMM driver.

  Returns:
    EFI_SUCCESS           - The dispatch function has been successfully
                            registered and the SMI source has been enabled.
    EFI_DEVICE_ERROR      - The driver was unable to enable the SMI source.
    EFI_OUT_OF_RESOURCES  - Not enough memory (system or SMM) to manage this
                            child.
    EFI_INVALID_PARAMETER - DispatchContext is invalid. The ICHN input value
                            is not within valid range.

--*/
typedef
EFI_STATUS
(EFIAPI *EFI_SMM_ICHN_UNREGISTER) (
  IN EFI_SMM_ICHN_DISPATCH_PROTOCOL             * This,
  IN  EFI_HANDLE                                DispatchHandle
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
// Interface structure for the SMM Ich n specific SMI Dispatch Protocol
//
struct _EFI_SMM_ICHN_DISPATCH_PROTOCOL {
  EFI_SMM_ICHN_REGISTER   Register;
  EFI_SMM_ICHN_UNREGISTER UnRegister;
};

extern EFI_GUID gEfiSmmIchnDispatchProtocolGuid;

#endif
