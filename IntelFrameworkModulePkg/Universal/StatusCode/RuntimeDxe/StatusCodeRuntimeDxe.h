/** @file
  Internal include file of Status Code Runtime DXE Driver.

  Copyright (c) 2006 - 2009, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __STATUS_CODE_RUNTIME_DXE_H__
#define __STATUS_CODE_RUNTIME_DXE_H__


#include <FrameworkDxe.h>
#include <Guid/DataHubStatusCodeRecord.h>
#include <Protocol/DataHub.h>
#include <Guid/MemoryStatusCodeRecord.h>
#include <Protocol/StatusCode.h>
#include <Guid/StatusCodeDataTypeId.h>
#include <Guid/StatusCodeDataTypeDebug.h>
#include <Guid/EventGroup.h>

#include <Library/BaseLib.h>
#include <Library/SynchronizationLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/ReportStatusCodeLib.h>
#include <Library/PrintLib.h>
#include <Library/PcdLib.h>
#include <Library/HobLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiRuntimeLib.h>
#include <Library/SerialPortLib.h>
#include <Library/OemHookStatusCodeLib.h>

//
// Data hub worker definition
//
#define DATAHUB_STATUS_CODE_SIGNATURE             SIGNATURE_32 ('B', 'D', 'H', 'S')

typedef struct {
  UINTN       Signature;
  LIST_ENTRY  Node;
  UINT8       Data[sizeof(DATA_HUB_STATUS_CODE_DATA_RECORD) + EFI_STATUS_CODE_DATA_MAX_SIZE];
} DATAHUB_STATUSCODE_RECORD;


//
// Runtime memory status code worker definition
//
typedef struct {
  UINT32   RecordIndex;
  UINT32   NumberOfRecords;
  UINT32   MaxRecordsNumber;
} RUNTIME_MEMORY_STATUSCODE_HEADER;

extern RUNTIME_MEMORY_STATUSCODE_HEADER  *mRtMemoryStatusCodeTable;

/**
  Report status code to all supported device.

  This function implements EFI_STATUS_CODE_PROTOCOL.ReportStatusCode().
  It calls into the workers which dispatches the platform specific listeners.

  @param  CodeType         Indicates the type of status code being reported.
  @param  Value            Describes the current status of a hardware or software entity.
                           This included information about the class and subclass that is used to
                           classify the entity as well as an operation.
  @param  Instance         The enumeration of a hardware or software entity within
                           the system. Valid instance numbers start with 1.
  @param  CallerId         This optional parameter may be used to identify the caller.
                           This parameter allows the status code driver to apply different rules to
                           different callers.
  @param  Data             This optional parameter may be used to pass additional data.

  @retval EFI_SUCCESS      The function completed successfully
  @retval EFI_DEVICE_ERROR The function should not be completed due to a device error.

**/
EFI_STATUS
EFIAPI
ReportDispatcher (
  IN EFI_STATUS_CODE_TYPE     CodeType,
  IN EFI_STATUS_CODE_VALUE    Value,
  IN UINT32                   Instance,
  IN EFI_GUID                 *CallerId  OPTIONAL,
  IN EFI_STATUS_CODE_DATA     *Data      OPTIONAL
  );

/**
  Dispatch initialization request to sub status code devices based on 
  customized feature flags.
 
**/
VOID
InitializationDispatcherWorker (
  VOID
  );


/**
  Locates Serial I/O Protocol as initialization for serial status code worker.
 
  @retval EFI_SUCCESS  Serial I/O Protocol is successfully located.

**/
EFI_STATUS
EfiSerialStatusCodeInitializeWorker (
  VOID
  );


/**
  Convert status code value and extended data to readable ASCII string, send string to serial I/O device.
 
  @param  CodeType         Indicates the type of status code being reported.
  @param  Value            Describes the current status of a hardware or software entity.
                           This included information about the class and subclass that is used to
                           classify the entity as well as an operation.
  @param  Instance         The enumeration of a hardware or software entity within
                           the system. Valid instance numbers start with 1.
  @param  CallerId         This optional parameter may be used to identify the caller.
                           This parameter allows the status code driver to apply different rules to
                           different callers.
  @param  Data             This optional parameter may be used to pass additional data.

  @retval EFI_SUCCESS      Status code reported to serial I/O successfully.
  @retval EFI_DEVICE_ERROR EFI serial device cannot work after ExitBootService() is called.
  @retval EFI_DEVICE_ERROR EFI serial device cannot work with TPL higher than TPL_CALLBACK.

**/
EFI_STATUS
SerialStatusCodeReportWorker (
  IN EFI_STATUS_CODE_TYPE     CodeType,
  IN EFI_STATUS_CODE_VALUE    Value,
  IN UINT32                   Instance,
  IN EFI_GUID                 *CallerId,
  IN EFI_STATUS_CODE_DATA     *Data OPTIONAL
  );

/**
  Initialize runtime memory status code table as initialization for runtime memory status code worker
 
  @retval EFI_SUCCESS  Runtime memory status code table successfully initialized.

**/
EFI_STATUS
RtMemoryStatusCodeInitializeWorker (
  VOID
  );

/**
  Report status code into runtime memory. If the runtime pool is full, roll back to the 
  first record and overwrite it.
 
  @param  CodeType                Indicates the type of status code being reported.
  @param  Value                   Describes the current status of a hardware or software entity.
                                  This included information about the class and subclass that is used to
                                  classify the entity as well as an operation.
  @param  Instance                The enumeration of a hardware or software entity within
                                  the system. Valid instance numbers start with 1.
 
  @retval EFI_SUCCESS             Status code successfully recorded in runtime memory status code table.

**/
EFI_STATUS
RtMemoryStatusCodeReportWorker (
  IN EFI_STATUS_CODE_TYPE               CodeType,
  IN EFI_STATUS_CODE_VALUE              Value,
  IN UINT32                             Instance
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
DataHubStatusCodeReportWorker (
  IN EFI_STATUS_CODE_TYPE     CodeType,
  IN EFI_STATUS_CODE_VALUE    Value,
  IN UINT32                   Instance,
  IN EFI_GUID                 *CallerId,
  IN EFI_STATUS_CODE_DATA     *Data OPTIONAL
  );


/**
  Virtual address change notification call back. It converts global pointer
  to virtual address.

  @param  Event         Event whose notification function is being invoked.
  @param  Context       Pointer to the notification function's context, which is
                        always zero in current implementation.

**/
VOID
EFIAPI
VirtualAddressChangeCallBack (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  );

#endif
