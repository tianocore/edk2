/** @file
  Internal include file for Status Code Handler Driver.

  Copyright (c) 2009 - 2018, Intel Corporation. All rights reserved.<BR>
  (C) Copyright 2016 Hewlett Packard Enterprise Development LP<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __STATUS_CODE_HANDLER_MM_H__
#define __STATUS_CODE_HANDLER_MM_H__

#include <Protocol/MmReportStatusCodeHandler.h>

#include <Guid/MemoryStatusCodeRecord.h>
#include <Guid/StatusCodeDataTypeId.h>
#include <Guid/StatusCodeDataTypeDebug.h>

#include <Library/SynchronizationLib.h>
#include <Library/DebugLib.h>
#include <Library/ReportStatusCodeLib.h>
#include <Library/PrintLib.h>
#include <Library/PcdLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/MmServicesTableLib.h>
#include <Library/SerialPortLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/BaseMemoryLib.h>

//
// Define the maximum message length
//
#define MAX_DEBUG_MESSAGE_LENGTH 0x100

extern RUNTIME_MEMORY_STATUSCODE_HEADER  *mMmMemoryStatusCodeTable;

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
EFIAPI
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
MemoryStatusCodeInitializeWorker (
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
  @param  CallerId                This optional parameter may be used to identify the caller.
                                  This parameter allows the status code driver to apply different rules to
                                  different callers.
  @param  Data                    This optional parameter may be used to pass additional data.

  @retval EFI_SUCCESS             Status code successfully recorded in runtime memory status code table.

**/
EFI_STATUS
EFIAPI
MemoryStatusCodeReportWorker (
  IN EFI_STATUS_CODE_TYPE               CodeType,
  IN EFI_STATUS_CODE_VALUE              Value,
  IN UINT32                             Instance,
  IN EFI_GUID                           *CallerId,
  IN EFI_STATUS_CODE_DATA               *Data OPTIONAL
  );

/**
  Entry point of Common MM Status Code Driver.

  This function is the entry point of MM Status Code Driver.

  @retval EFI_SUCCESS       The entry point is executed successfully.

**/
EFI_STATUS
StatusCodeHandlerCommonEntry (
  VOID
  );

#endif
