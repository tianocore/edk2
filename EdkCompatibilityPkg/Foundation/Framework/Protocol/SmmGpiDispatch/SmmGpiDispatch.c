/*++

Copyright (c) 1999 - 2002, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.


Module Name:

    SmmGpiDispatch.c
    
Abstract:

    EFI Smm Gpi Smi Child Protocol

Revision History

--*/

#include "Tiano.h"
#include EFI_PROTOCOL_DEFINITION (SmmGpiDispatch)

EFI_GUID  gEfiSmmGpiDispatchProtocolGuid = EFI_SMM_GPI_DISPATCH_PROTOCOL_GUID;

EFI_GUID_STRING
  (&gEfiSmmGpiDispatchProtocolGuid, "SMM GPI SMI Dispatch Protocol", "EFI 2.0 SMM GPI SMI Dispatch Protocol");
