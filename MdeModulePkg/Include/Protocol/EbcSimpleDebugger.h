/** @file
  EBC Simple Debugger protocol for debug EBC code.

Copyright (c) 2011, Intel Corporation. All rights reserved.<BR>

This program and the accompanying materials
are licensed and made available under the terms and conditions
of the BSD License which accompanies this distribution.  The
full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _EBC_SIMPLE_DEBUGGER_PROTOCOL_H_
#define _EBC_SIMPLE_DEBUGGER_PROTOCOL_H_

#include <Protocol/DebugSupport.h>
#include <Protocol/EbcVmTest.h>

#define EFI_EBC_SIMPLE_DEBUGGER_PROTOCOL_GUID \
  { \
    0x2a72d11e, 0x7376, 0x40f6, { 0x9c, 0x68, 0x23, 0xfa, 0x2f, 0xe3, 0x63, 0xf1 } \
  }

//
// Defines for a simple EBC debugger interface
//
typedef struct _EFI_EBC_SIMPLE_DEBUGGER_PROTOCOL EFI_EBC_SIMPLE_DEBUGGER_PROTOCOL;

/**
  Trig Exception on EBC VM.

  @param[in] This           A pointer to the EFI_EBC_SIMPLE_DEBUGGER_PROTOCOL structure.
  @param[in] VmPtr          A pointer to a VM context.
  @param[in] ExceptionType  Exception to be trigged.

  @retval EFI_UNSUPPORTED       No support for it.
  @retval EFI_SUCCESS           Exception is trigged.

**/
typedef
EFI_STATUS
(EFIAPI *EBC_DEBUGGER_SIGNAL_EXCEPTION) (
  IN EFI_EBC_SIMPLE_DEBUGGER_PROTOCOL           *This,
  IN VM_CONTEXT                                 *VmPtr,
  IN EFI_EXCEPTION_TYPE                         ExceptionType
  );

/**
  Given a pointer to a new VM context, debug one or more instructions.

  @param[in] This           A pointer to the EFI_EBC_SIMPLE_DEBUGGER_PROTOCOL structure.
  @param[in] VmPtr          A pointer to a VM context.

  @retval EFI_UNSUPPORTED       No support for it.
  @retval EFI_SUCCESS           Debug one or more instructions.

**/
typedef
VOID
(EFIAPI *EBC_DEBUGGER_DEBUG) (
  IN EFI_EBC_SIMPLE_DEBUGGER_PROTOCOL           *This,
  IN VM_CONTEXT                                 *VmPtr
  );

/**
  Given a pointer to a new VM context, dump one or more instructions.

  @param[in] This           A pointer to the EFI_EBC_SIMPLE_DEBUGGER_PROTOCOL structure.
  @param[in] VmPtr          A pointer to a VM context.
  @param[in] DasmString     Dump string buffer. 
  @param[in] DasmStringSize Dump string size.

  @retval EFI_UNSUPPORTED       No support for it.
  @retval EFI_SUCCESS           Dump one or more instructions.

**/
typedef
UINT32
(EFIAPI *EBC_DEBUGGER_DASM) (
  IN EFI_EBC_SIMPLE_DEBUGGER_PROTOCOL           *This,
  IN VM_CONTEXT                                 *VmPtr,
  IN UINT16                                     *DasmString OPTIONAL,
  IN UINT32                                     DasmStringSize
  );

/**
  This interface allows you to configure the EBC debug support
  driver. For example, turn on or off saving and printing of
  delta VM even if called. Or to even disable the entire interface,
  in which case all functions become no-ops.

  @param[in] This           A pointer to the EFI_EBC_SIMPLE_DEBUGGER_PROTOCOL structure.
  @param[in] ConfigId       ID to be configured.
  @param[in] ConfigValue    Value to be set.

  @retval EFI_UNSUPPORTED       No support for it.
  @retval EFI_SUCCESS           Configure EBC debug.

**/
typedef
EFI_STATUS
(EFIAPI *EBC_DEBUGGER_CONFIGURE) (
  IN EFI_EBC_SIMPLE_DEBUGGER_PROTOCOL           *This,
  IN UINT32                                     ConfigId,
  IN UINTN                                      ConfigValue
  );

//
// Prototype for the actual EBC debug support protocol interface
//
struct _EFI_EBC_SIMPLE_DEBUGGER_PROTOCOL {
  EBC_DEBUGGER_DEBUG            Debugger;
  EBC_DEBUGGER_SIGNAL_EXCEPTION SignalException;
  EBC_DEBUGGER_DASM             Dasm;
  EBC_DEBUGGER_CONFIGURE        Configure;
};

extern EFI_GUID gEfiEbcSimpleDebuggerProtocolGuid;

#endif
