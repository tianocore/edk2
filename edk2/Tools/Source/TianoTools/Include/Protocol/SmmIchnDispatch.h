/** @file
  This file declares EFI Smm ICH [N] Specific Smi Child Protocol

  Copyright (c) 2006, Intel Corporation                                                         
  All rights reserved. This program and the accompanying materials                          
  are licensed and made available under the terms and conditions of the BSD License         
  which accompanies this distribution.  The full text of the license may be found at        
  http://opensource.org/licenses/bsd-license.php                                            

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

  Module Name:  SmmIchnDispatch.h

  @par Revision Reference:
  This Protocol is defined in Framework of EFI SMM Core Interface Spec
  Version 0.9.

**/

#ifndef _EFI_SMM_ICHN_DISPATCH_H_
#define _EFI_SMM_ICHN_DISPATCH_H_

//
// Global ID for the ICH SMI Protocol
//
#define EFI_SMM_ICHN_DISPATCH_PROTOCOL_GUID \
  { \
    0xc50b323e, 0x9075, 0x4f2a, {0xac, 0x8e, 0xd2, 0x59, 0x6a, 0x10, 0x85, 0xcc } \
  }

typedef struct _EFI_SMM_ICHN_DISPATCH_PROTOCOL  EFI_SMM_ICHN_DISPATCH_PROTOCOL;

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
/**
  Dispatch function for a ICH n specific SMI handler.

  @param  DispatchHandle Handle of this dispatch function.
  @param  DispatchContext Pointer to the dispatch function's context.
  The DispatchContext fields are filled in
  by the dispatching driver prior to
  invoking this dispatch function.

  Nothing

**/
typedef
VOID
(EFIAPI *EFI_SMM_ICHN_DISPATCH) (
  IN  EFI_HANDLE                      DispatchHandle,
  IN  EFI_SMM_ICHN_DISPATCH_CONTEXT   *DispatchContext
  );

/**
  Register a child SMI source dispatch function with a parent SMM driver

  @param  This Protocol instance pointer.
  @param  DispatchFunction Pointer to dispatch function to be invoked for
  this SMI source
  @param  DispatchContext Pointer to the dispatch function's context.
  The caller fills this context in before calling
  the register function to indicate to the register
  function the ICHN SMI source for which the dispatch
  function should be invoked.
  @param  DispatchHandle Handle of dispatch function, for when interfacing
  with the parent Sx state SMM driver.

  @retval  EFI_SUCCESS The dispatch function has been successfully
  registered and the SMI source has been enabled.
  @retval  EFI_DEVICE_ERROR The driver was unable to enable the SMI source.
  @retval  EFI_OUT_OF_RESOURCES Not enough memory (system or SMM) to manage this
  child.
  @retval  EFI_INVALID_PARAMETER DispatchContext is invalid. The ICHN input value
  is not within valid range.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_SMM_ICHN_REGISTER) (
  IN EFI_SMM_ICHN_DISPATCH_PROTOCOL             *This,
  IN  EFI_SMM_ICHN_DISPATCH                     DispatchFunction,
  IN  EFI_SMM_ICHN_DISPATCH_CONTEXT             *DispatchContext,
  OUT EFI_HANDLE                                *DispatchHandle
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
  @retval  other TBD

**/
typedef
EFI_STATUS
(EFIAPI *EFI_SMM_ICHN_UNREGISTER) (
  IN EFI_SMM_ICHN_DISPATCH_PROTOCOL             *This,
  IN  EFI_HANDLE                                DispatchHandle
  );

//
// Interface structure for the SMM Ich n specific SMI Dispatch Protocol
//
/**
  @par Protocol Description:
  Provides the parent dispatch service for a given SMI source generator.

  @param Register
  Installs a child service to be dispatched by this protocol.

  @param UnRegister
  Removes a child service dispatched by this protocol.

**/
struct _EFI_SMM_ICHN_DISPATCH_PROTOCOL {
  EFI_SMM_ICHN_REGISTER   Register;
  EFI_SMM_ICHN_UNREGISTER UnRegister;
};

extern EFI_GUID gEfiSmmIchnDispatchProtocolGuid;

#endif
