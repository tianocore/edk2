/** @file
  Report Status Code Library for DXE Phase.

  Copyright (c) 2006 - 2007, Intel Corporation<BR>
  All rights reserved. This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "ReportStatusCodeLibInternal.h"

/**
  Locate he report status code service.

  @return     EFI_REPORT_STATUS_CODE    function point to
              ReportStatusCode.
**/
EFI_REPORT_STATUS_CODE
InternalGetReportStatusCode (
  VOID
  )
{
  EFI_STATUS_CODE_PROTOCOL  *StatusCodeProtocol;
  EFI_STATUS                Status;

  if (gRT->Hdr.Revision < 0x20000) {
    return ((FRAMEWORK_EFI_RUNTIME_SERVICES*)gRT)->ReportStatusCode;
  } else if (gBS != NULL) {
    Status = gBS->LocateProtocol (&gEfiStatusCodeRuntimeProtocolGuid, NULL, (VOID**)&StatusCodeProtocol);
    if (!EFI_ERROR (Status) && StatusCodeProtocol != NULL) {
      return StatusCodeProtocol->ReportStatusCode;
    }
  }

  return NULL;
}

/**
  Reports a status code with full parameters.

  The function reports a status code.  If ExtendedData is NULL and ExtendedDataSize
  is 0, then an extended data buffer is not reported.  If ExtendedData is not
  NULL and ExtendedDataSize is not 0, then an extended data buffer is allocated.
  ExtendedData is assumed not have the standard status code header, so this function
  is responsible for allocating a buffer large enough for the standard header and
  the extended data passed into this function.  The standard header is filled in
  with a GUID specified by ExtendedDataGuid.  If ExtendedDataGuid is NULL, then a
  GUID of gEfiStatusCodeSpecificDatauid is used.  The status code is reported with
  an instance specified by Instance and a caller ID specified by CallerId.  If
  CallerId is NULL, then a caller ID of gEfiCallerIdGuid is used.

  ReportStatusCodeEx()must actively prevent recursion.  If ReportStatusCodeEx()
  is called while processing another any other Report Status Code Library function,
  then ReportStatusCodeEx() must return EFI_DEVICE_ERROR immediately.

  If ExtendedData is NULL and ExtendedDataSize is not zero, then ASSERT().
  If ExtendedData is not NULL and ExtendedDataSize is zero, then ASSERT().

  @param  Type              Status code type.
  @param  Value             Status code value.
  @param  Instance          Status code instance number.
  @param  CallerId          Pointer to a GUID that identifies the caller of this
                            function.  If this parameter is NULL, then a caller
                            ID of gEfiCallerIdGuid is used.
  @param  ExtendedDataGuid  Pointer to the GUID for the extended data buffer.
                            If this parameter is NULL, then a the status code
                            standard header is filled in with
                            gEfiStatusCodeSpecificDataGuid.
  @param  ExtendedData      Pointer to the extended data buffer.  This is an
                            optional parameter that may be NULL.
  @param  ExtendedDataSize  The size, in bytes, of the extended data buffer.

  @retval  EFI_SUCCESS           The status code was reported.
  @retval  EFI_OUT_OF_RESOURCES  There were not enough resources to allocate
                                 the extended data section if it was specified.
  @retval  EFI_UNSUPPORTED       Report status code is not supported

**/
EFI_STATUS
EFIAPI
InternalReportStatusCodeEx (
  IN EFI_STATUS_CODE_TYPE   Type,
  IN EFI_STATUS_CODE_VALUE  Value,
  IN UINT32                 Instance,
  IN CONST EFI_GUID         *CallerId          OPTIONAL,
  IN CONST EFI_GUID         *ExtendedDataGuid  OPTIONAL,
  IN CONST VOID             *ExtendedData      OPTIONAL,
  IN UINTN                  ExtendedDataSize
  )
{
  EFI_STATUS            Status;
  EFI_STATUS_CODE_DATA  *StatusCodeData;

  ASSERT (!((ExtendedData == NULL) && (ExtendedDataSize != 0)));
  ASSERT (!((ExtendedData != NULL) && (ExtendedDataSize == 0)));

  if (gBS == NULL) {
    return EFI_UNSUPPORTED;
  }

  //
  // Allocate space for the Status Code Header and its buffer
  //
  StatusCodeData = NULL;
  gBS->AllocatePool (EfiBootServicesData, sizeof (EFI_STATUS_CODE_DATA) + ExtendedDataSize, (VOID **)&StatusCodeData);
  if (StatusCodeData == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Fill in the extended data header
  //
  StatusCodeData->HeaderSize = sizeof (EFI_STATUS_CODE_DATA);
  StatusCodeData->Size = (UINT16)ExtendedDataSize;
  if (ExtendedDataGuid == NULL) {
    ExtendedDataGuid = &gEfiStatusCodeSpecificDataGuid;
  }
  CopyGuid (&StatusCodeData->Type, ExtendedDataGuid);

  //
  // Fill in the extended data buffer
  //
  CopyMem (StatusCodeData + 1, ExtendedData, ExtendedDataSize);

  //
  // Report the status code
  //
  if (CallerId == NULL) {
    CallerId = &gEfiCallerIdGuid;
  }
  Status = InternalReportStatusCode (Type, Value, Instance, CallerId, StatusCodeData);

  //
  // Free the allocated buffer
  //
  gBS->FreePool (StatusCodeData);

  return Status;
}

