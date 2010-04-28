/** @file
  Report Status Code Library that depends on a GUIDed HOB for report status
  code functionality.

  Copyright (c) 2009, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "ReportStatusCodeLibInternal.h"

//
// Handle to install report status code protocol interface
//
EFI_HANDLE               mHandle = NULL;

//
// Report status protocol interface.
//
EFI_STATUS_CODE_PROTOCOL mStatusProtocol = {
  NULL
};

/**
  Initializes report status code infrastructure for DXE phase.

  The constructor function assumes the PEI phase has published the GUIDed HOB
  tagged by gEfiStatusCodeRuntimeProtocolGuid and publish status code runtime
  protocol based on the GUID data. It will ASSERT() if one of these operations
  fails and it will always return EFI_SUCCESS.

  @param  ImageHandle   The firmware allocated handle for the EFI image.
  @param  SystemTable   A pointer to the EFI System Table.

  @retval EFI_SUCCESS   The constructor always returns EFI_SUCCESS.

**/
EFI_STATUS
EFIAPI
DxeCoreReportStatusCodeFromHobLibConstructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS           Status;
  EFI_HOB_GUID_TYPE    *GuidHob;

  GuidHob = GetFirstGuidHob(&gEfiStatusCodeRuntimeProtocolGuid);
  ASSERT (GuidHob != NULL);
  
  mStatusProtocol.ReportStatusCode = (EFI_REPORT_STATUS_CODE) (*(UINTN *) GET_GUID_HOB_DATA (GuidHob));
  ASSERT (mStatusProtocol.ReportStatusCode != NULL);

  //
  // Install report status protocol interface so that later DXE phase drivers can retrieve the protocol
  // interface to report status code.
  //
  Status = gBS->InstallProtocolInterface (
                  &mHandle,
                  &gEfiStatusCodeRuntimeProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  &mStatusProtocol
                  );
  ASSERT_EFI_ERROR (Status);

  return Status;
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

