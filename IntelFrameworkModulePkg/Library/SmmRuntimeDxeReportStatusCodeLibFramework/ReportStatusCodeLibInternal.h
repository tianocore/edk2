/** @file
  Internal Header file of Report Status Code Library for RUNTIME
  DXE Phase.

  Copyright (c) 2006 - 2010, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/
#ifndef __REPORT_STATUS_CODE_LIB_INTERNAL__H__
#define __REPORT_STATUS_CODE_LIB_INTERNAL__H__

#include <FrameworkSmm.h>

#include <Library/ReportStatusCodeLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/PcdLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/DevicePathLib.h>
#include <Library/MemoryAllocationLib.h>

#include <Guid/StatusCodeDataTypeId.h>
#include <Guid/StatusCodeDataTypeDebug.h>
#include <Guid/EventGroup.h>

#include <Protocol/SmmStatusCode.h>
#include <Protocol/StatusCode.h>
#include <Protocol/SmmBase.h>


extern EFI_STATUS_CODE_DATA    *mStatusCodeData;

/**
  Internal worker function that reports a status code through the status code service.

  If status code service is not cached, then this function checks if status code service is
  available in system.  If status code service is not available, then EFI_UNSUPPORTED is
  returned.  If status code service is present, then it is cached in mReportStatusCode.
  Finally this function reports status code through the status code service.

  @param  Type              Status code type.
  @param  Value             Status code value.
  @param  Instance          Status code instance number.
  @param  CallerId          Pointer to a GUID that identifies the caller of this
                            function.  This is an optional parameter that may be
                            NULL.
  @param  Data              Pointer to the extended data buffer.  This is an
                            optional parameter that may be NULL.

  @retval EFI_SUCCESS       The status code was reported.
  @retval EFI_UNSUPPORTED   Status code service is not available.
  @retval EFI_UNSUPPORTED   Status code type is not supported.

**/
EFI_STATUS
InternalReportStatusCode (
  IN EFI_STATUS_CODE_TYPE     Type,
  IN EFI_STATUS_CODE_VALUE    Value,
  IN UINT32                   Instance,
  IN CONST EFI_GUID           *CallerId OPTIONAL,
  IN EFI_STATUS_CODE_DATA     *Data     OPTIONAL
  );

#endif

