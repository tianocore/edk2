/** @file
  Library constructor & destructor, event handlers, and other internal worker functions.

  Copyright (c) 2006 - 2009, Intel Corporation<BR>
  All rights reserved. This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "ReportStatusCodeLibInternal.h"

EFI_EVENT               mVirtualAddressChangeEvent;
EFI_EVENT               mExitBootServicesEvent;
EFI_STATUS_CODE_DATA    *mStatusCodeData;
EFI_RUNTIME_SERVICES    *mInternalRT;
BOOLEAN                 mHaveExitedBootServices = FALSE;
EFI_REPORT_STATUS_CODE  mReportStatusCode = NULL;

/**
  Locate the report status code service.

  It first tries to retrieve ReportStatusCode() in Runtime Services Table.
  If not found, it then tries to retrieve ReportStatusCode() API of Report Status Code Protocol.

  @return   Function pointer to the report status code service.
            NULL is returned if no status code service is available.

**/
EFI_REPORT_STATUS_CODE
InternalGetReportStatusCode (
  VOID
  )
{
  EFI_STATUS_CODE_PROTOCOL  *StatusCodeProtocol;
  EFI_STATUS                Status;

  if (!mHaveExitedBootServices) {
  	//
  	// Check gBS just in case. ReportStatusCode is called before gBS is initialized.
  	//
    if (gBS != NULL) {
      Status = gBS->LocateProtocol (&gEfiStatusCodeRuntimeProtocolGuid, NULL, (VOID**) &StatusCodeProtocol);
      if (!EFI_ERROR (Status) && StatusCodeProtocol != NULL) {
        return StatusCodeProtocol->ReportStatusCode;
      }
    }
  }

  return NULL;
}

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
  )
{
  if ((ReportProgressCodeEnabled() && ((Type) & EFI_STATUS_CODE_TYPE_MASK) == EFI_PROGRESS_CODE) ||
      (ReportErrorCodeEnabled() && ((Type) & EFI_STATUS_CODE_TYPE_MASK) == EFI_ERROR_CODE) ||
      (ReportDebugCodeEnabled() && ((Type) & EFI_STATUS_CODE_TYPE_MASK) == EFI_DEBUG_CODE)) {
    //
    // If mReportStatusCode is NULL, then check if status code service is available in system.
    //
    if (mReportStatusCode == NULL) {
      mReportStatusCode = InternalGetReportStatusCode ();
      if (mReportStatusCode == NULL) {
        return EFI_UNSUPPORTED;
      }
    }
  
    //
    // A status code service is present in system, so pass in all the parameters to the service.
    //
    return (*mReportStatusCode) (Type, Value, Instance, (EFI_GUID *)CallerId, Data);
  }
  
  return EFI_UNSUPPORTED;
}

/**
  Notification function of EVT_SIGNAL_VIRTUAL_ADDRESS_CHANGE.

  @param  Event        Event whose notification function is being invoked.
  @param  Context      Pointer to the notification function's context

**/
VOID
EFIAPI
ReportStatusCodeLibVirtualAddressChange (
  IN EFI_EVENT        Event,
  IN VOID             *Context
  )
{
  if (mReportStatusCode != NULL) {
    mInternalRT->ConvertPointer (0, (VOID **) &mReportStatusCode);
  }
  mInternalRT->ConvertPointer (0, (VOID **) &mStatusCodeData);
  mInternalRT->ConvertPointer (0, (VOID **) &mInternalRT);
}

/**
  Notification function of EVT_SIGNAL_EXIT_BOOT_SERVICES.

  @param  Event        Event whose notification function is being invoked.
  @param  Context      Pointer to the notification function's context

**/
VOID
EFIAPI
ReportStatusCodeLibExitBootServices (
  IN EFI_EVENT        Event,
  IN VOID             *Context
  )
{
  mHaveExitedBootServices = TRUE;
}

/**
  The constructor function of Runtime DXE Report Status Code Lib.

  This function allocates memory for extended status code data, caches
  the report status code service, and registers events.

  @param  ImageHandle   The firmware allocated handle for the EFI image.
  @param  SystemTable   A pointer to the EFI System Table.
  
  @retval EFI_SUCCESS   The constructor always returns EFI_SUCCESS.

**/
EFI_STATUS
EFIAPI
ReportStatusCodeLibConstruct (
  IN EFI_HANDLE           ImageHandle,
  IN EFI_SYSTEM_TABLE     *SystemTable
  )
{
  EFI_STATUS     Status;

  //
  // Library should not use the gRT directly, for it may be converted by other library instance.
  // 
  mInternalRT = gRT;

  mStatusCodeData = AllocateRuntimePool (sizeof (EFI_STATUS_CODE_DATA) + EFI_STATUS_CODE_DATA_MAX_SIZE);
  ASSERT (mStatusCodeData != NULL);
  //
  // Cache the report status code service
  // 
  mReportStatusCode = InternalGetReportStatusCode ();

  //
  // Register notify function for EVT_SIGNAL_VIRTUAL_ADDRESS_CHANGE
  // 
  Status = gBS->CreateEventEx (
                  EVT_NOTIFY_SIGNAL,
                  TPL_NOTIFY,
                  ReportStatusCodeLibVirtualAddressChange,
                  NULL,
                  &gEfiEventVirtualAddressChangeGuid,
                  &mVirtualAddressChangeEvent
                  );
  ASSERT_EFI_ERROR (Status);

  //
  // Register notify function for EVT_SIGNAL_EXIT_BOOT_SERVICES
  // 
  Status = gBS->CreateEventEx (
                  EVT_NOTIFY_SIGNAL,
                  TPL_NOTIFY,
                  ReportStatusCodeLibExitBootServices,
                  NULL,
                  &gEfiEventExitBootServicesGuid,
                  &mExitBootServicesEvent
                  );
  ASSERT_EFI_ERROR (Status);

  return EFI_SUCCESS;
}

/**
  The destructor function of Runtime DXE Report Status Code Lib.
  
  The destructor function frees memory allocated by constructor, and closes related events.
  It will ASSERT() if that related operation fails and it will always return EFI_SUCCESS. 

  @param  ImageHandle   The firmware allocated handle for the EFI image.
  @param  SystemTable   A pointer to the EFI System Table.
  
  @retval EFI_SUCCESS   The constructor always returns EFI_SUCCESS.

**/
EFI_STATUS
EFIAPI
ReportStatusCodeLibDestruct (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  ASSERT (gBS != NULL);
  Status = gBS->CloseEvent (mVirtualAddressChangeEvent);
  ASSERT_EFI_ERROR (Status);
  Status = gBS->CloseEvent (mExitBootServicesEvent);
  ASSERT_EFI_ERROR (Status);

  FreePool (mStatusCodeData);

  return EFI_SUCCESS;
}

