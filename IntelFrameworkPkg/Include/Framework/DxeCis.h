/** @file
  Include file that supportes Framework extension to the UEFI 2.0 spec.

  This include file must only contain things defined in the Framework
  specifications. If a code construct is defined in the Framework specification
  it must be included by this include file.

  Copyright (c) 2007, Intel Corporation
  All rights reserved. This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

  Module Name:    FrameworkDxeCis.h

**/

#ifndef _FRAMEWORK_DXE_CIS_H_
#define _FRAMEWORK_DXE_CIS_H_

#include <PiDxe.h>
#include <Framework/StatusCode.h>
#include <Protocol/StatusCode.h>

//
// Function prototype for invoking a function on an Application Processor
// Used by both the SMM infrastructure and the MP Services Protocol
//
typedef
VOID
(EFIAPI *EFI_AP_PROCEDURE) (
  IN  VOID                              *Buffer
  );

typedef struct {
  EFI_TABLE_HEADER              Hdr;

  //
  // Time services
  //
  EFI_GET_TIME                  GetTime;
  EFI_SET_TIME                  SetTime;
  EFI_GET_WAKEUP_TIME           GetWakeupTime;
  EFI_SET_WAKEUP_TIME           SetWakeupTime;

  //
  // Virtual memory services
  //
  EFI_SET_VIRTUAL_ADDRESS_MAP   SetVirtualAddressMap;
  EFI_CONVERT_POINTER           ConvertPointer;

  //
  // Variable services
  //
  EFI_GET_VARIABLE              GetVariable;
  EFI_GET_NEXT_VARIABLE_NAME    GetNextVariableName;
  EFI_SET_VARIABLE              SetVariable;

  //
  // Misc
  //
  EFI_GET_NEXT_HIGH_MONO_COUNT  GetNextHighMonotonicCount;
  EFI_RESET_SYSTEM              ResetSystem;

  //
  // Framework extension to UEFI 2.0 runtime table
  // It was moved to a protocol to not conflict with UEFI 2.0
  //
  EFI_REPORT_STATUS_CODE        ReportStatusCode;
} FRAMEWORK_EFI_RUNTIME_SERVICES;

#define EFI_EVENT_RUNTIME_CONTEXT       0x20000000
#define EFI_EVENT_NOTIFY_SIGNAL_ALL     0x00000400
#define EFI_EVENT_SIGNAL_READY_TO_BOOT  0x00000203
#define EFI_EVENT_SIGNAL_LEGACY_BOOT    0x00000204


typedef struct {
  EFI_DEVICE_PATH_PROTOCOL  Header;
  EFI_GUID                  TianoSpecificDevicePath;
  UINT32                    Type;
} TIANO_DEVICE_PATH;

#define TIANO_MEDIA_FW_VOL_FILEPATH_DEVICE_PATH_TYPE         0x01
typedef struct {
  TIANO_DEVICE_PATH     Tiano;
  EFI_GUID              NameGuid;
} FRAMEWORK_MEDIA_FW_VOL_FILEPATH_DEVICE_PATH;


#endif
