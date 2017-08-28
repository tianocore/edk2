/** @file
  SMM General Purpose Input (GPI) Dispatch2 Protocol as defined in PI 1.1 Specification
  Volume 4 System Management Mode Core Interface.

  This protocol provides the parent dispatch service for the General Purpose Input 
  (GPI) SMI source generator.

  The EFI_SMM_GPI_DISPATCH2_PROTOCOL provides the ability to install child handlers for the 
  given event types.  Several inputs can be enabled.  This purpose of this interface is to generate an 
  SMI in response to any of these inputs having a true value provided.

  Copyright (c) 2009 - 2017, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

  @par Revision Reference:
  This protocol is from PI Version 1.1.

**/

#ifndef _SMM_GPI_DISPATCH2_H_
#define _SMM_GPI_DISPATCH2_H_

#include <Protocol/MmGpiDispatch.h>
#include <Pi/PiSmmCis.h>

#define EFI_SMM_GPI_DISPATCH2_PROTOCOL_GUID    EFI_MM_GPI_DISPATCH_PROTOCOL_GUID
///
/// The dispatch function's context.
///
typedef EFI_MM_GPI_REGISTER_CONTEXT  EFI_SMM_GPI_REGISTER_CONTEXT;

typedef EFI_MM_GPI_REGISTER EFI_SMM_GPI_REGISTER2;

typedef EFI_MM_GPI_UNREGISTER EFI_SMM_GPI_UNREGISTER2;

typedef EFI_MM_GPI_DISPATCH_PROTOCOL EFI_SMM_GPI_DISPATCH2_PROTOCOL;



extern EFI_GUID gEfiSmmGpiDispatch2ProtocolGuid;

#endif

