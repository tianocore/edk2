/**@file
  Common header file shared by all source files.

  This file includes package header files, library classes and protocol, PPI & GUID definitions.

  Copyright (c) 2006 - 2007, Intel Corporation.
  All rights reserved.
   This software and associated documentation (if any) is furnished
   under a license and may only be used or copied in accordance
   with the terms of the license. Except as permitted by such
   license, no part of this software or documentation may be
   reproduced, stored in a retrieval system, or transmitted in any
   form or by any means without the express written consent of
   Intel Corporation.
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
#include <Protocol/SimpleTextIn.h>
#include <Protocol/IsaIo.h>
#include <Protocol/DevicePath.h>
#include <Protocol/Ps2Policy.h>

//
// The Library classes this module consumes
//
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/ReportStatusCodeLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/MemoryAllocationLib.h>
//
// Driver Binding Externs
//
extern EFI_DRIVER_BINDING_PROTOCOL gKeyboardControllerDriver;
extern EFI_COMPONENT_NAME_PROTOCOL gPs2KeyboardComponentName;

#endif
