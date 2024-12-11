/** @file
   TdxMeasurement Functions which are used in DXE phase

Copyright (c) 2025, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent
**/
#include <Base.h>
#include <Library/DebugLib.h>
#include <Library/TdxMeasurementLib.h>

/**
 * Build GuidHob for Tdx CC measurement event.
 *
 * @param RtmrIndex     RTMR index
 * @param EventType     Event type
 * @param EventData     Event data
 * @param EventSize     Size of event data
 * @param HashValue     Hash value
 * @param HashSize      Size of hash
 *
 * @retval EFI_SUCCESS  Successfully build the GuidHobs
 * @retval Others       Other error as indicated
 */
EFI_STATUS
EFIAPI
TdxMeasurementBuildGuidHob (
  UINT32  RtmrIndex,
  UINT32  EventType,
  UINT8   *EventData,
  UINT32  EventSize,
  UINT8   *HashValue,
  UINT32  HashSize
  )
{
  return EFI_UNSUPPORTED;
}
