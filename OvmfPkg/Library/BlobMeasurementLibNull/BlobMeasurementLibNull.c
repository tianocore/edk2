/** @file

  Null implementation of the blob measurement library.

  Copyright (C) 2022, Intel Corporation. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Library/BaseLib.h>
#include <Library/BlobMeasurementLib.h>

/**
  Measure blob from an external source.

  @param[in] BlobName           The name of the blob
  @param[in] BlobNameSize       Size of the blob name
  @param[in] BlobBase           The data of the blob
  @param[in] BlobSize           The size of the blob in bytes

  @retval EFI_SUCCESS           The blob was measured successfully.
  @retval Other errors
**/
EFI_STATUS
EFIAPI
MeasureKernelBlob (
  IN  CONST CHAR16  *BlobName,
  IN  UINT32        BlobNameSize,
  IN  CONST VOID    *BlobBase,
  IN  UINT32        BlobSize
  )
{
  return EFI_SUCCESS;
}
