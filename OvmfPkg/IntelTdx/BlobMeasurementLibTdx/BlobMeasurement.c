/** @file

  Copyright (C) 2022, Intel Corporation. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/
#include <PiDxe.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Protocol/Tcg2Protocol.h>
#include <Protocol/CcMeasurement.h>
#include <Library/BlobVerifierLib.h>

EFI_CC_MEASUREMENT_PROTOCOL  *mCcProtocol = NULL;

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
  EFI_STATUS    Status;
  UINT32        MrIndex;
  EFI_CC_EVENT  *CcEvent;

  if ((BlobBase == 0) || (BlobSize == 0)) {
    ASSERT (FALSE);
    return EFI_INVALID_PARAMETER;
  }

  if (mCcProtocol == NULL) {
    Status = gBS->LocateProtocol (&gEfiCcMeasurementProtocolGuid, NULL, (VOID **)&mCcProtocol);
    if (EFI_ERROR (Status)) {
      //
      // EFI_CC_MEASUREMENT_PROTOCOL protocol is not installed.
      //
      DEBUG ((DEBUG_ERROR, "%a: EFI_CC_MEASUREMENT_PROTOCOL protocol is not installed.\n", __FUNCTION__));
      return EFI_NOT_FOUND;
    }
  }

  Status = mCcProtocol->MapPcrToMrIndex (mCcProtocol, 4, &MrIndex);
  if (EFI_ERROR (Status)) {
    return EFI_INVALID_PARAMETER;
  }

  CcEvent = AllocateZeroPool (BlobNameSize + sizeof (EFI_CC_EVENT) - sizeof (CcEvent->Event));
  if (CcEvent == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  CcEvent->Size                 = BlobNameSize + sizeof (EFI_CC_EVENT) - sizeof (CcEvent->Event);
  CcEvent->Header.EventType     = EV_PLATFORM_CONFIG_FLAGS;
  CcEvent->Header.MrIndex       = MrIndex;
  CcEvent->Header.HeaderSize    = sizeof (EFI_TCG2_EVENT_HEADER);
  CcEvent->Header.HeaderVersion = EFI_TCG2_EVENT_HEADER_VERSION;
  CopyMem (&CcEvent->Event[0], BlobName, BlobNameSize);

  Status = mCcProtocol->HashLogExtendEvent (
                          mCcProtocol,
                          0,
                          (EFI_PHYSICAL_ADDRESS)(UINTN)BlobBase,
                          BlobSize,
                          CcEvent
                          );

  FreePool (CcEvent);

  return Status;
}
