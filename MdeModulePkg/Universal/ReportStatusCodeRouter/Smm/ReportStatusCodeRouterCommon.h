/** @file
  Internal include file for Report Status Code Router Driver.

  Copyright (c) 2009 - 2018, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __REPORT_STATUS_CODE_ROUTER_COMMON_H__
#define __REPORT_STATUS_CODE_ROUTER_COMMON_H__

#include <Protocol/MmReportStatusCodeHandler.h>
#include <Protocol/MmStatusCode.h>

#include <Library/BaseLib.h>
#include <Library/SynchronizationLib.h>
#include <Library/DebugLib.h>
#include <Library/PcdLib.h>
#include <Library/MmServicesTableLib.h>
#include <Library/MemoryAllocationLib.h>

#define MM_RSC_HANDLER_CALLBACK_ENTRY_SIGNATURE  SIGNATURE_32 ('s', 'h', 'c', 'e')

typedef struct {
  UINTN                          Signature;
  EFI_MM_RSC_HANDLER_CALLBACK    RscHandlerCallback;
  LIST_ENTRY                     Node;
} MM_RSC_HANDLER_CALLBACK_ENTRY;

/**
  Register the callback function for ReportStatusCode() notification.

  When this function is called the function pointer is added to an internal list and any future calls to
  ReportStatusCode() will be forwarded to the Callback function.

  @param[in] Callback           A pointer to a function of type EFI_PEI_RSC_HANDLER_CALLBACK that is called
                                when a call to ReportStatusCode() occurs.

  @retval EFI_SUCCESS           Function was successfully registered.
  @retval EFI_INVALID_PARAMETER The callback function was NULL.
  @retval EFI_OUT_OF_RESOURCES  The internal buffer ran out of space. No more functions can be
                                registered.
  @retval EFI_ALREADY_STARTED   The function was already registered. It can't be registered again.

**/
EFI_STATUS
EFIAPI
Register (
  IN EFI_MM_RSC_HANDLER_CALLBACK  Callback
  );

/**
  Remove a previously registered callback function from the notification list.

  ReportStatusCode() messages will no longer be forwarded to the Callback function.

  @param[in] Callback           A pointer to a function of type EFI_PEI_RSC_HANDLER_CALLBACK that is to be
                                unregistered.

  @retval EFI_SUCCESS           The function was successfully unregistered.
  @retval EFI_INVALID_PARAMETER The callback function was NULL.
  @retval EFI_NOT_FOUND         The callback function was not found to be unregistered.

**/
EFI_STATUS
EFIAPI
Unregister (
  IN EFI_MM_RSC_HANDLER_CALLBACK  Callback
  );

/**
  Provides an interface that a software module can call to report a status code.

  @param  This             EFI_MM_STATUS_CODE_PROTOCOL instance.
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

  @retval EFI_SUCCESS           The function completed successfully
  @retval EFI_DEVICE_ERROR      The function should not be completed due to a device error.

**/
EFI_STATUS
EFIAPI
ReportDispatcher (
  IN CONST EFI_MM_STATUS_CODE_PROTOCOL  *This,
  IN EFI_STATUS_CODE_TYPE               CodeType,
  IN EFI_STATUS_CODE_VALUE              Value,
  IN UINT32                             Instance,
  IN CONST EFI_GUID                     *CallerId,
  IN EFI_STATUS_CODE_DATA               *Data      OPTIONAL
  );

/**
  Entry point of Generic Status Code Driver.

  This function is the common entry point of MM Status Code Router.
  It produces MM Report Status Code Handler and Status Code protocol.

  @retval EFI_SUCCESS       The entry point is executed successfully.

**/
EFI_STATUS
GenericStatusCodeCommonEntry (
  VOID
  );

#endif
