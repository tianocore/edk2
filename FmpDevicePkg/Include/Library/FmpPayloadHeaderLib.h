/**  @file
  Provides services to retrieve values from a capsule's FMP Payload Header.
  The structure is not included in the library class.  Instead, services are
  provided to retrieve information from the FMP Payload Header.  If information
  is added to the FMP Payload Header, then new services may be added to this
  library class to retrieve the new information.

  Copyright (c) 2016, Microsoft Corporation. All rights reserved.<BR>
  Copyright (c) 2018, Intel Corporation. All rights reserved.<BR>

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are met:
  1. Redistributions of source code must retain the above copyright notice,
  this list of conditions and the following disclaimer.
  2. Redistributions in binary form must reproduce the above copyright notice,
  this list of conditions and the following disclaimer in the documentation
  and/or other materials provided with the distribution.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
  ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
  IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
  INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
  LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
  OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
  ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

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
  IN     CONST VOID   *Header,
  IN     CONST UINTN  FmpPayloadSize,
  IN OUT UINT32       *LowestSupportedVersion
  );

#endif
