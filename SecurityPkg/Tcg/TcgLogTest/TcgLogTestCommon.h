/** @file
  TCG Log Test common function declarations shared by TcgLogTestDxe and
  TcgLogTestApp.

  Copyright (c), Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef TCG_LOG_TEST_COMMON_H_
#define TCG_LOG_TEST_COMMON_H_

#include <Uefi.h>
#include <Protocol/Tcg2Protocol.h>
#include <IndustryStandard/UefiTcgPlatform.h>

/**
  Advance one entry in the event log.

  @param[in,out] CurrentEvent  On entry, points to the start of the event (PCRIndex).
                               On success, updated to point to the next event.
  @param[in]     LogEnd        One byte past the end of the log buffer.
  @param[out]    PcrIndex      PCRIndex of the parsed event.
  @param[out]    EventType     EventType of the parsed event.
  @param[out]    EventSize     Size of the event data payload.
  @param[out]    EventData     Pointer to the event data payload.

  @retval TRUE   Event parsed successfully and CurrentEvent updated.
  @retval FALSE  Invalid pointers, buffer, or digest algorithm.
**/
BOOLEAN
TcgLogTestAdvanceEvent (
  IN OUT UINT8   **CurrentEvent,
  IN     UINT8   *LogEnd,
  OUT    UINT32  *PcrIndex   OPTIONAL,
  OUT    UINT32  *EventType  OPTIONAL,
  OUT    UINT32  *EventSize  OPTIONAL,
  OUT    UINT8   **EventData OPTIONAL
  );

/**
  Log events via TCG2 until the event log base address changes (dynamic
  scaling) or an error is returned.

  @param[in]  Tcg2    TCG2 protocol instance.
  @param[out] Scaled  TRUE if scaling was detected.

  @retval EFI_SUCCESS            Scaling detected.
  @retval EFI_INVALID_PARAMETER  NULL argument.
**/
EFI_STATUS
TcgLogTestLogEventsUntilScaled (
  IN  EFI_TCG2_PROTOCOL  *Tcg2,
  OUT BOOLEAN            *Scaled
  );

/**
  Log a single fixed test event via the TCG2 protocol.

  @param[in]  Tcg2  TCG2 protocol instance.

  @retval EFI_SUCCESS            Event logged.
  @retval EFI_INVALID_PARAMETER  NULL argument.
  @retval Other                  HashLogExtendEvent failure.
**/
EFI_STATUS
TcgLogTestLogSingleEvent (
  IN EFI_TCG2_PROTOCOL  *Tcg2
  );

#endif // TCG_LOG_TEST_COMMON_H_
