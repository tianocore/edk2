/** @file
  Provides services to retrieve values from a capsule's FMP Payload Header.
  The structure is not included in the library class.  Instead, services are
  provided to retrieve information from the FMP Payload Header.  If information
  is added to the FMP Payload Header, then new services may be added to this
  library class to retrieve the new information.

  Copyright (c) 2016, Microsoft Corporation. All rights reserved.<BR>
  Copyright (c) 2018, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _FMP_PAYLOAD_HEADER_LIB_H__
#define _FMP_PAYLOAD_HEADER_LIB_H__

/**
  Returns the FMP Payload Header size in bytes.

  @param[in]  Header          FMP Payload Header to evaluate
  @param[in]  FmpPayloadSize  Size of FMP payload
  @param[out] Size            The size, in bytes, of the FMP Payload Header.

  @retval EFI_SUCCESS            The firmware version was returned.
  @retval EFI_INVALID_PARAMETER  Header is NULL.
  @retval EFI_INVALID_PARAMETER  Size is NULL.
  @retval EFI_INVALID_PARAMETER  Header is not a valid FMP Payload Header.

**/
EFI_STATUS
EFIAPI
GetFmpPayloadHeaderSize (
  IN  CONST VOID   *Header,
  IN  CONST UINTN  FmpPayloadSize,
  OUT UINT32       *Size
  );

/**
  Returns the version described in the FMP Payload Header.

  @param[in]  Header          FMP Payload Header to evaluate
  @param[in]  FmpPayloadSize  Size of FMP payload
  @param[out] Version         The firmware version described in the FMP Payload
                              Header.

  @retval EFI_SUCCESS            The firmware version was returned.
  @retval EFI_INVALID_PARAMETER  Header is NULL.
  @retval EFI_INVALID_PARAMETER  Version is NULL.
  @retval EFI_INVALID_PARAMETER  Header is not a valid FMP Payload Header.

**/
EFI_STATUS
EFIAPI
GetFmpPayloadHeaderVersion (
  IN  CONST VOID   *Header,
  IN  CONST UINTN  FmpPayloadSize,
  OUT UINT32       *Version
  );

/**
  Returns the lowest supported version described in the FMP Payload Header.

  @param[in]  Header                  FMP Payload Header to evaluate
  @param[in]  FmpPayloadSize          Size of FMP payload
  @param[out] LowestSupportedVersion  The lowest supported version described in
                                      the FMP Payload Header.

  @retval EFI_SUCCESS            The lowest support version was returned.
  @retval EFI_INVALID_PARAMETER  Header is NULL.
  @retval EFI_INVALID_PARAMETER  LowestSupportedVersion is NULL.
  @retval EFI_INVALID_PARAMETER  Header is not a valid FMP Payload Header.

**/
EFI_STATUS
EFIAPI
GetFmpPayloadHeaderLowestSupportedVersion (
  IN  CONST VOID   *Header,
  IN  CONST UINTN  FmpPayloadSize,
  OUT UINT32       *LowestSupportedVersion
  );

#endif
