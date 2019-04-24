/** @file
  Library constructor & destructor, event handlers, and other internal worker functions.

  Copyright (c) 2006 - 2019, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "ReportStatusCodeLibInternal.h"

EFI_EVENT                     mVirtualAddressChangeEvent;
static EFI_EVENT              mExitBootServicesEvent;
EFI_STATUS_CODE_DATA          *mStatusCodeData;
BOOLEAN                       mInSmm;
EFI_SMM_BASE_PROTOCOL         *mSmmBase;
EFI_RUNTIME_SERVICES          *mInternalRT;
BOOLEAN                       mHaveExitedBootServices = FALSE;
EFI_REPORT_STATUS_CODE        mReportStatusCode = NULL;
EFI_SMM_STATUS_CODE_PROTOCOL  *mSmmStatusCodeProtocol;

/**
  Locates and caches SMM Status Code Protocol.

**/
VOID
SmmStatusCodeInitialize (
  VOID
  )
{
  EFI_STATUS Status;

  Status = gBS->LocateProtocol (&gEfiSmmStatusCodeProtocolGuid, NULL, (VOID **) &mSmmStatusCodeProtocol);
  if (EFI_ERROR (Status)) {
    mSmmStatusCodeProtocol = NULL;
  }
}

/**
  Report status code via SMM Status Code Protocol.

  @param  Type          Indicates the type of status code being reported.
  @param  Value         Describes the current status of a hardware or software entity.
                        This included information about the class and subclass that is used to classify the entity
                        as well as an operation.  For progress codes, the operation is the current activity.
                        For error codes, it is the exception.  For debug codes, it is not defined at this time.
  @param  Instance      The enumeration of a hardware or software entity within the system.
                        A system may contain multiple entities that match a class/subclass pairing.
                        The instance differentiates between them.  An instance of 0 indicates that instance information is unavailable,
                        not meaningful, or not relevant.  Valid instance numbers start with 1.
  @param  CallerId      This optional parameter may be used to identify the caller.
                        This parameter allows the status code driver to apply different rules to different callers.
  @param  Data          This optional parameter may be used to pass additional data

  @retval EFI_SUCCESS   Always return EFI_SUCCESS.

**/
EFI_STATUS
SmmStatusCodeReport (
  IN EFI_STATUS_CODE_TYPE     Type,
  IN EFI_STATUS_CODE_VALUE    Value,
  IN UINT32                   Instance,
  IN EFI_GUID                 *CallerId OPTIONAL,
  IN EFI_STATUS_CODE_DATA     *Data     OPTIONAL
  )
{
  if (mSmmStatusCodeProtocol != NULL) {
    (mSmmStatusCodeProtocol->ReportStatusCode) (mSmmStatusCodeProtocol, Type, Value, Instance, CallerId, Data);
  }
  return EFI_SUCCESS;
}

/**
  Locate the report status code service.

  In SMM, it tries to retrieve SMM Status Code Protocol.
  Otherwise, it first tries to retrieve ReportStatusCode() in Runtime Services Table.
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

  if (mInSmm) {
    return (EFI_REPORT_STATUS_CODE) SmmStatusCodeReport;
  } else if (mInternalRT != NULL && mInternalRT->Hdr.Revision < 0x20000) {
    return ((FRAMEWORK_EFI_RUNTIME_SERVICES*)mInternalRT)->ReportStatusCode;
  } else if (!mHaveExitedBootServices) {
    //
    // Check gBS just in case. ReportStatusCode is called before gBS is initialized.
    //
    if (gBS != NULL) {
      Status = gBS->LocateProtocol (&gEfiStatusCodeRuntimeProtocolGuid, NULL, (VOID**)&StatusCodeProtocol);
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
  //
  // If mReportStatusCode is NULL, then see if a Status Code Protocol instance is present
  // in the handle database.
  //
  if (mReportStatusCode == NULL) {
    mReportStatusCode = InternalGetReportStatusCode ();
  }

  mHaveExitedBootServices = TRUE;
}

/**
  The constructor function of SMM Runtime DXE Report Status Code Lib.

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
  // If in SMM mode, then allocates memory from SMRAM for extended status code data.
  //
  Status = gBS->LocateProtocol (&gEfiSmmBaseProtocolGuid, NULL, (VOID **) &mSmmBase);
  if (!EFI_ERROR (Status)) {
    mSmmBase->InSmm (mSmmBase, &mInSmm);
    if (mInSmm) {
      Status = mSmmBase->SmmAllocatePool (
                           mSmmBase,
                           EfiRuntimeServicesData,
                           sizeof (EFI_STATUS_CODE_DATA) + EFI_STATUS_CODE_DATA_MAX_SIZE,
                           (VOID **) &mStatusCodeData
                           );
      ASSERT_EFI_ERROR (Status);
      SmmStatusCodeInitialize ();
      return EFI_SUCCESS;
    }
  }


  //
  // If not in SMM mode, then allocate runtime memory for extended status code data.
  //
  // Library should not use the gRT directly, for it may be converted by other library instance.
  //
  mInternalRT = gRT;
  mInSmm      = FALSE;

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
  The destructor function of SMM Runtime DXE Report Status Code Lib.

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

  if (!mInSmm) {
    ASSERT (gBS != NULL);
    Status = gBS->CloseEvent (mVirtualAddressChangeEvent);
    ASSERT_EFI_ERROR (Status);
    Status = gBS->CloseEvent (mExitBootServicesEvent);
    ASSERT_EFI_ERROR (Status);

    FreePool (mStatusCodeData);
  } else {
    mSmmBase->SmmFreePool (mSmmBase, mStatusCodeData);
  }

  return EFI_SUCCESS;
}

