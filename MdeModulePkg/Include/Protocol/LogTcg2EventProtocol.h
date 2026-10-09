/** @file
  EDKII Log TCG2 Event Protocol definition.

  This protocol provides a service to add an entry to the TCG2 event log from
  a caller-supplied digest list, without measuring (hashing and extending) the
  event into a PCR. It is intended for callers that have already performed, or
  intentionally skipped, the PCR extend and only need the event recorded in the
  log.

  See the TCG PC Client Platform Firmware Profile Specification for the event
  log format. Refer to http://trustedcomputinggroup.org for the latest
  specifications.

Copyright (c) Qualcomm Technologies, Inc. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#pragma once

extern EFI_GUID  gEdkiiLogTcg2EventProtocolGuid;

//
// Protocol GUID.
//
#define EDKII_LOG_TCG2_EVENT_PROTOCOL_GUID \
  {0xf82300be, 0x6568, 0x46fa, { 0x96, 0xf2, 0xd5, 0xe5, 0x8d, 0x82, 0x07, 0xcd }}

//
// Protocol revision.
//
#define EDKII_LOG_TCG2_EVENT_PROTOCOL_REVISION  0x0000000000010000

//
// Forward declaration of the protocol structure.
//
typedef struct _EDKII_LOG_TCG2_EVENT_PROTOCOL EDKII_LOG_TCG2_EVENT_PROTOCOL;

/**
  Add an event to the TCG2 event log using the supplied digest list.

  This logs the event described by Tcg2Event together with the digests in
  DigestList. Unlike EFI_TCG2_PROTOCOL.HashLogExtendEvent, this service only
  records the event in the log; it does not compute any digest and does not
  extend a PCR. The caller is responsible for providing digests that are
  consistent with the platform's active PCR banks.

  @param[in]  This            Pointer to the EDKII_LOG_TCG2_EVENT_PROTOCOL
                              instance.
  @param[in]  DigestList      Pointer to a TPML_DIGEST_VALUES structure
                              containing the digests to record for the event.
  @param[in]  DigestListSize  Size, in bytes, of the buffer pointed to by
                              DigestList.
  @param[in]  Tcg2Event       Pointer to an EFI_TCG2_EVENT structure that
                              describes the event to log.
  @param[in]  Tcg2EventSize   Size, in bytes, of the buffer pointed to by
                              Tcg2Event.

  @retval EFI_SUCCESS            The event was added to the event log.
  @retval EFI_INVALID_PARAMETER  A parameter is NULL, zero, or otherwise
                                 inconsistent (for example, DigestListSize does
                                 not match the described digests).
  @retval EFI_BUFFER_TOO_SMALL   Tcg2EventSize is too small for the described
                                 event.
  @retval EFI_DEVICE_ERROR       The TPM is not present or the event could not
                                 be logged.
**/
typedef
EFI_STATUS
(EFIAPI *LOG_TCG2_EVENT)(
  IN EDKII_LOG_TCG2_EVENT_PROTOCOL  *This,
  IN VOID                           *DigestList,
  IN UINTN                          DigestListSize,
  IN VOID                           *Tcg2Event,
  IN UINTN                          Tcg2EventSize
  );

struct _EDKII_LOG_TCG2_EVENT_PROTOCOL {
  //
  // Protocol revision, EDKII_LOG_TCG2_EVENT_PROTOCOL_REVISION.
  //
  UINT64            Revision;
  //
  // Adds an event to the TCG2 event log from a caller-supplied digest list.
  //
  LOG_TCG2_EVENT    LogEvent;
};
