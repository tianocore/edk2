/** @file
  This file declares Smm Gpi Smi Child Protocol

  Copyright (c) 2006, Intel Corporation                                                         
  All rights reserved. This program and the accompanying materials                          
  are licensed and made available under the terms and conditions of the BSD License         
  which accompanies this distribution.  The full text of the license may be found at        
  http://opensource.org/licenses/bsd-license.php                                            

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

  Module Name:  SmmGpiDispatch.h

  @par Revision Reference:
  This Protocol is defined in Framework of EFI SMM Core Interface Spec
  Version 0.9.

**/

#ifndef _EFI_SMM_GPI_DISPATCH_H_
#define _EFI_SMM_GPI_DISPATCH_H_

//
// Global ID for the GPI SMI Protocol
//
#define EFI_SMM_GPI_DISPATCH_PROTOCOL_GUID \
  { \
    0xe0744b81, 0x9513, 0x49cd, {0x8c, 0xea, 0xe9, 0x24, 0x5e, 0x70, 0x39, 0xda } \
  }

typedef struct _EFI_SMM_GPI_DISPATCH_PROTOCOL  EFI_SMM_GPI_DISPATCH_PROTOCOL;

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
/**
  Dispatch function for a GPI SMI handler.

  @param  DispatchHandle        Handle of this dispatch function.
  @param  DispatchContext       Pointer to the dispatch function's context.
                                The DispatchContext fields are filled in by the dispatching driver prior to
                                invoking this dispatch function.

  @return None

**/
typedef
VOID
(EFIAPI *EFI_SMM_GPI_DISPATCH) (
  IN  EFI_HANDLE                    DispatchHandle,
  IN  EFI_SMM_GPI_DISPATCH_CONTEXT  *DispatchContext
  );

/**
  Register a child SMI source dispatch function with a parent SMM driver

  @param  This                  Protocol instance pointer.
  @param  DispatchFunction      Pointer to dispatch function to be invoked for
                                this SMI source
  @param  DispatchContext       Pointer to the dispatch function's context.
                                The caller fills this context in before calling
                                the register function to indicate to the register
                                function the GPI(s) for which the dispatch function
                                should be invoked.
  @param  DispatchHandle        Handle of dispatch function, for when interfacing
                                with the parent Sx state SMM driver.

  @retval EFI_SUCCESS           The dispatch function has been successfully
                                registered and the SMI source has been enabled.
  @retval EFI_DEVICE_ERROR      The driver was unable to enable the SMI source.
  @retval EFI_OUT_OF_RESOURCES  Not enough memory (system or SMM) to manage this
                                child.
  @retval EFI_INVALID_PARAMETER DispatchContext is invalid. The GPI input value
                                is not within valid range.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_SMM_GPI_REGISTER) (
  IN EFI_SMM_GPI_DISPATCH_PROTOCOL            *This,
  IN  EFI_SMM_GPI_DISPATCH                    DispatchFunction,
  IN  EFI_SMM_GPI_DISPATCH_CONTEXT            *DispatchContext,
  OUT EFI_HANDLE                              *DispatchHandle
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
(EFIAPI *EFI_SMM_GPI_UNREGISTER) (
  IN EFI_SMM_GPI_DISPATCH_PROTOCOL            *This,
  IN  EFI_HANDLE                              DispatchHandle
  );

//
// Interface structure for the SMM GPI SMI Dispatch Protocol
//
/**
  @par Protocol Description:
  Provides the parent dispatch service for the General Purpose Input 
  (GPI) SMI source generator.

  @param Register
  Installs a child service to be dispatched by this protocol.

  @param UnRegister
  Removes a child service dispatched by this protocol.

  @param NumSupportedGpis
  Denotes the maximum value of inputs that can have handlers attached.

**/
struct _EFI_SMM_GPI_DISPATCH_PROTOCOL {
  EFI_SMM_GPI_REGISTER    Register;
  EFI_SMM_GPI_UNREGISTER  UnRegister;
  UINTN                   NumSupportedGpis;
};

extern EFI_GUID gEfiSmmGpiDispatchProtocolGuid;

#endif

