/** @file
  Internal include file for Report Status Code Router Driver.

  Copyright (c) 2009, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __REPORT_STATUS_CODE_ROUTER_RUNTIME_DXE_H__
#define __REPORT_STATUS_CODE_ROUTER_RUNTIME_DXE_H__


#include <Protocol/ReportStatusCodeHandler.h>
#include <Protocol/StatusCode.h>

#include <Guid/EventGroup.h>

#include <Library/BaseLib.h>
#include <Library/SynchronizationLib.h>
#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiRuntimeLib.h>
#include "Library/UefiLib.h"

#define RSC_HANDLER_CALLBACK_ENTRY_SIGNATURE  SIGNATURE_32 ('r', 'h', 'c', 'e')

typedef struct {
  UINTN                     Signature;
  EFI_RSC_HANDLER_CALLBACK  RscHandlerCallback;
  EFI_TPL                   Tpl;
  EFI_EVENT                 Event;
  EFI_PHYSICAL_ADDRESS      StatusCodeDataBuffer;
  UINTN                     BufferSize;
  EFI_PHYSICAL_ADDRESS      EndPointer;
  LIST_ENTRY                Node;
} RSC_HANDLER_CALLBACK_ENTRY;

typedef struct {
  EFI_STATUS_CODE_TYPE      Type;
  EFI_STATUS_CODE_VALUE     Value;
  UINT32                    Instance;
  UINT32                    Reserved;
  EFI_GUID                  CallerId;
  EFI_STATUS_CODE_DATA      Data;
} RSC_DATA_ENTRY;

/**
  Register the callback function for ReportStatusCode() notification.
  
  When this function is called the function pointer is added to an internal list and any future calls to
  ReportStatusCode() will be forwarded to the Callback function. During the bootservices,
  this is the callback for which this service can be invoked. The report status code router
  will create an event such that the callback function is only invoked at the TPL for which it was
  registered. The entity that registers for the callback should also register for an event upon
  generation of exit boot services and invoke the unregister service.
  If the handler does not have a TPL dependency, it should register for a callback at TPL high. The
  router infrastructure will support making callbacks at runtime, but the caller for runtime invocation
  must meet the following criteria:
  1. must be a runtime driver type so that its memory is not reclaimed
  2. not unregister at exit boot services so that the router will still have its callback address
  3. the caller must be self-contained (eg. Not call out into any boot-service interfaces) and be
  runtime safe, in general.
  
  @param[in] Callback   A pointer to a function of type EFI_RSC_HANDLER_CALLBACK that is called when
                        a call to ReportStatusCode() occurs.
  @param[in] Tpl        TPL at which callback can be safely invoked.   
  
  @retval  EFI_SUCCESS              Function was successfully registered.
  @retval  EFI_INVALID_PARAMETER    The callback function was NULL.
  @retval  EFI_OUT_OF_RESOURCES     The internal buffer ran out of space. No more functions can be
                                    registered.
  @retval  EFI_ALREADY_STARTED      The function was already registered. It can't be registered again.

**/
EFI_STATUS
EFIAPI
Register (
  IN EFI_RSC_HANDLER_CALLBACK   Callback,
  IN EFI_TPL                    Tpl
  );

/**
  Remove a previously registered callback function from the notification list.
  
  A callback function must be unregistered before it is deallocated. It is important that any registered
  callbacks that are not runtime complaint be unregistered when ExitBootServices() is called.
  
  @param[in]  Callback  A pointer to a function of type EFI_RSC_HANDLER_CALLBACK that is to be
                        unregistered.
                        
  @retval EFI_SUCCESS           The function was successfully unregistered.
  @retval EFI_INVALID_PARAMETER The callback function was NULL.
  @retval EFI_NOT_FOUND         The callback function was not found to be unregistered.

**/
EFI_STATUS
EFIAPI
Unregister (
  IN EFI_RSC_HANDLER_CALLBACK Callback
  );

/**
  Provides an interface that a software module can call to report a status code.

  @param  Type             Indicates the type of status code being reported.
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
  IN EFI_STATUS_CODE_TYPE     Type,
  IN EFI_STATUS_CODE_VALUE    Value,
  IN UINT32                   Instance,
  IN EFI_GUID                 *CallerId  OPTIONAL,
  IN EFI_STATUS_CODE_DATA     *Data      OPTIONAL
  );

#endif


