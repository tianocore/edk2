/**@file
  Common header file shared by all source files.

  This file includes package header files, library classes and protocol, PPI & GUID definitions.

Copyright (c) 2006 - 2007, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __COMMON_HEADER_H_
#define __COMMON_HEADER_H_


//
// The package level header files this module uses
//
#include <PiDxe.h>
#include <Framework/StatusCode.h>
//
// The protocols, PPI and GUID defintions for this module
//
#include <Protocol/SimplePointer.h>
#include <Protocol/IsaIo.h>
#include <Protocol/DevicePath.h>
//
// The Library classes this module consumes
//
#include <Library/DebugLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/ReportStatusCodeLib.h>
//
// Driver Binding Externs
//
extern EFI_DRIVER_BINDING_PROTOCOL gPS2MouseDriver;
extern EFI_COMPONENT_NAME_PROTOCOL gPs2MouseComponentName;

#endif
