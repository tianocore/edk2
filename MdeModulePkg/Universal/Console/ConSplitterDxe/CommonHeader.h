/**@file
  Common header file shared by all source files.

  This file includes package header files, library classes and protocol, PPI & GUID definitions.

  Copyright (c) 2006 - 2007, Intel Corporation
  All rights reserved. This program and the accompanying materials
   are licensed and made available under the terms and conditions of the BSD License
   which accompanies this distribution. The full text of the license may be found at
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
//
// The protocols, PPI and GUID defintions for this module
//
#include <Guid/PrimaryStandardErrorDevice.h>
#include <Guid/PrimaryConsoleOutDevice.h>
#include <Protocol/GraphicsOutput.h>
#include <Guid/PrimaryConsoleInDevice.h>
#include <Protocol/SimplePointer.h>
#include <Protocol/SimpleTextOut.h>
#include <Guid/ConsoleInDevice.h>
#include <Protocol/SimpleTextIn.h>
#include <Protocol/ConsoleControl.h>
#include <Guid/StandardErrorDevice.h>
#include <Guid/ConsoleOutDevice.h>
#include <Protocol/UgaDraw.h>
//
// The Library classes this module consumes
//
#include <Library/DebugLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
//
// Driver Binding Externs
//
extern EFI_DRIVER_BINDING_PROTOCOL gConSplitterConInDriverBinding;
extern EFI_COMPONENT_NAME_PROTOCOL gConSplitterConInComponentName;
extern EFI_DRIVER_BINDING_PROTOCOL gConSplitterSimplePointerDriverBinding;
extern EFI_COMPONENT_NAME_PROTOCOL gConSplitterSimplePointerComponentName;
extern EFI_DRIVER_BINDING_PROTOCOL gConSplitterConOutDriverBinding;
extern EFI_COMPONENT_NAME_PROTOCOL gConSplitterConOutComponentName;
extern EFI_DRIVER_BINDING_PROTOCOL gConSplitterStdErrDriverBinding;
extern EFI_COMPONENT_NAME_PROTOCOL gConSplitterStdErrComponentName;


#endif
