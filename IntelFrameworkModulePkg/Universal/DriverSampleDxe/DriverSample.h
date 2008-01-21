/**@file

  This is an example of how a driver might export data to the HII protocol to be
  later utilized by the Setup Protocol
  
Copyright (c) 2006, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/
#ifndef _DRIVER_SAMPLE_H
#define _DRIVER_SAMPLE_H



#include <FrameworkDxe.h>

#include <Protocol/FrameworkFormCallback.h>
#include <Protocol/FrameworkHii.h>

#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/BaseLib.h>
#include <Library/PrintLib.h>
#include <Library/FrameworkIfrSupportLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/FrameworkHiiLib.h>

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
  FRAMEWORK_EFI_HII_HANDLE    RegisteredHandle;
  EFI_HII_PROTOCOL            *Hii;
} EFI_CALLBACK_INFO;

#define EFI_CALLBACK_INFO_FROM_THIS(a)  CR (a, EFI_CALLBACK_INFO, DriverCallback, EFI_CALLBACK_INFO_SIGNATURE)

#endif
