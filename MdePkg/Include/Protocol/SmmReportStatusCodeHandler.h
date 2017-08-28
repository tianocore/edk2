/** @file
  This protocol provides registering and unregistering services to status code consumers while in DXE SMM.
  
  Copyright (c) 2007 - 2017, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __SMM_REPORT_STATUS_CODE_HANDLER_PROTOCOL_H__
#define __SMM_REPORT_STATUS_CODE_HANDLER_PROTOCOL_H__

#include <Protocol/MmReportStatusCodeHandler.h>

#define EFI_SMM_RSC_HANDLER_PROTOCOL_GUID EFI_MM_RSC_HANDLER_PROTOCOL_GUID

typedef EFI_MM_RSC_HANDLER_CALLBACK EFI_SMM_RSC_HANDLER_CALLBACK;

typedef EFI_MM_RSC_HANDLER_REGISTER EFI_SMM_RSC_HANDLER_REGISTER;

typedef EFI_MM_RSC_HANDLER_UNREGISTER EFI_SMM_RSC_HANDLER_UNREGISTER;

typedef EFI_MM_RSC_HANDLER_PROTOCOL EFI_SMM_RSC_HANDLER_PROTOCOL;

extern EFI_GUID gEfiSmmRscHandlerProtocolGuid;

#endif // __SMM_REPORT_STATUS_CODE_HANDLER_PROTOCOL_H__
