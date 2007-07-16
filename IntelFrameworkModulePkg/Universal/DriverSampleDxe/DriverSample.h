/*++

Copyright (c) 2006, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

  DriverSample.h

Abstract:


Revision History

--*/

#ifndef _DRIVER_SAMPLE_H
#define _DRIVER_SAMPLE_H


//
// The package level header files this module uses
//
#include <PiDxe.h>
//
// The protocols, PPI and GUID defintions for this module
//
#include <Protocol/FormCallbackFramework.h>
#include <Protocol/HiiFramework.h>
//
// The Library classes this module consumes
//
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/BaseLib.h>
#include <Library/PrintLib.h>
#include <Library/IfrSupportLibFramework.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/HiiLibFramework.h>

#include "NVDataStruc.h"

//
// This is the generated header file which includes whatever needs to be exported (strings + IFR)
//

extern UINT8  VfrBin[];
//
// extern UINT8 VfrStringsStr[];
//
extern UINT8  InventoryBin[];
//
// extern UINT8 InventoryStringsStr[];
//
extern UINT8  DriverSampleDxeStrings[];

#define SAMPLE_STRING               L"This is an error!"

#define EFI_CALLBACK_INFO_SIGNATURE EFI_SIGNATURE_32 ('C', 'l', 'b', 'k')

typedef struct {
  UINTN                       Signature;
  EFI_HANDLE                  CallbackHandle;
  EFI_FORM_CALLBACK_PROTOCOL  DriverCallback;
  UINT16                      *KeyList;
  VOID                        *FormBuffer;
  EFI_HII_HANDLE              RegisteredHandle;
  EFI_HII_PROTOCOL            *Hii;
} EFI_CALLBACK_INFO;

#define EFI_CALLBACK_INFO_FROM_THIS(a)  CR (a, EFI_CALLBACK_INFO, DriverCallback, EFI_CALLBACK_INFO_SIGNATURE)

#endif
