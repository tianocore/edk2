/** @file
  Functions to deal with an FMP payload in a transparent way.

  Copyright (c) 2026, 3mdeb Sp. z o.o. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#pragma once

/**
  Retrieves range of memory that corresponds to data within FMP payload.

  @param[in]   FmpPayload       Start of an FMP payload.
  @param[in]   FmpPayloadSize   Length of the FMP payload.
  @param[out]  PayloadData      Start of data within the payload.
  @param[out]  PayloadDataSize  Length of the data.

  @retval EFI_INVALID_PARAMETER  Any of pointer parameters is NULL.
                                 Payload is malformed.
  @retval EFI_SUCCESS            Output parameters were set successfully.

**/
EFI_STATUS
EFIAPI
FmpPayloadGetData (
  IN  VOID   *FmpPayload,
  IN  UINTN  FmpPayloadSize,
  OUT VOID   **PayloadData,
  OUT UINTN  *PayloadDataSize
  );
