/** @file
  Internal include file for Datahub Status Code Handler Driver.

  Copyright (c) 2010, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __DATAHUB_STATUS_CODE_HANDLER_DXE_H__
#define __DATAHUB_STATUS_CODE_HANDLER_DXE_H__

#include <Protocol/ReportStatusCodeHandler.h>
#include <Protocol/DataHub.h>
#include <Protocol/StatusCode.h>

#include <Guid/StatusCodeDataTypeId.h>
#include <Guid/StatusCodeDataTypeDebug.h>
#include <Guid/DataHubStatusCodeRecord.h>
#include <Guid/EventGroup.h>

#include <Library/BaseLib.h>
#include <Library/SynchronizationLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/ReportStatusCodeLib.h>
#include <Library/PrintLib.h>
#include <Library/PcdLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/MemoryAllocationLib.h>

//
// Data hub worker definition
//
#define DATAHUB_STATUS_CODE_SIGNATURE             SIGNATURE_32 ('B', 'D', 'H', 'S')

typedef struct {
  UINTN       Signature;
  LIST_ENTRY  Node;
  UINT8       Data[sizeof(DATA_HUB_STATUS_CODE_DATA_RECORD) + EFI_STATUS_CODE_DATA_MAX_SIZE];
} DATAHUB_STATUSCODE_RECORD;

/**
  Report status code into DataHub.

  @param  CodeType             Indicates the type of status code being reported.
  @param  Value                Describes the current status of a hardware or software entity.
                               This included information about the class and subclass that is used to
                               classify the entity as well as an operation.
  @param  Instance             The enumeration of a hardware or software entity within
                               the system. Valid instance numbers start with 1.
  @param  CallerId             This optional parameter may be used to identify the caller.
                               This parameter allows the status code driver to apply different rules to
                               different callers.
  @param  Data                 This optional parameter may be used to pass additional data.

  @retval EFI_SUCCESS          The function completed successfully.
  @retval EFI_DEVICE_ERROR     Function is reentered.
  @retval EFI_DEVICE_ERROR     Function is called at runtime.
  @retval EFI_OUT_OF_RESOURCES Fail to allocate memory for free record buffer.

**/
EFI_STATUS
EFIAPI
DataHubStatusCodeReportWorker (
  IN EFI_STATUS_CODE_TYPE     CodeType,
  IN EFI_STATUS_CODE_VALUE    Value,
  IN UINT32                   Instance,
  IN EFI_GUID                 *CallerId,
  IN EFI_STATUS_CODE_DATA     *Data OPTIONAL
  );

/**
  Locate Data Hub Protocol and create event for logging data
  as initialization for data hub status code worker.

  @retval EFI_SUCCESS  Initialization is successful.

**/
EFI_STATUS
DataHubStatusCodeInitializeWorker (
  VOID
  );

#endif
