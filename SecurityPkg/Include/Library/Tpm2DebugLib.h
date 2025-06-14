/** @file -- Tpm2DebugLib.h
This file contains helper functions to perform a detailed debugging of
TPM transactions as they go to and from the TPM device.

Copyright (c) Microsoft Corporation.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef TPM_2_DEBUG_LIB_H_
#define TPM_2_DEBUG_LIB_H_

#include <Protocol/Tcg2Protocol.h>

/**
  This function dumps as much information as possible about
  a command being sent to the TPM for maximum user-readability.

  @param[in]  InputBlockSize  Size of the input buffer.
  @param[in]  InputBlock      Pointer to the input buffer itself.

**/
VOID
EFIAPI
DumpTpmInputBlock (
  IN UINT32       InputBlockSize,
  IN CONST UINT8  *InputBlock
  );

/**
  This function dumps as much information as possible about
  a response from the TPM for maximum user-readability.

  @param[in]  OutputBlockSize  Size of the output buffer.
  @param[in]  OutputBlock      Pointer to the output buffer itself.

**/
VOID
EFIAPI
DumpTpmOutputBlock (
  IN UINT32       OutputBlockSize,
  IN CONST UINT8  *OutputBlock
  );

/**
  This function dumps the provided event log.

  @param[in]  EventLogFormat     The type of the event log for which the information is requested.
  @param[in]  EventLogLocation   A pointer to the memory address of the event log.
  @param[in]  EventLogLastEntry  If the Event Log contains more than one entry, this is a pointer to the
                                 address of the start of the last entry in the event log in memory.
  @param[in]  FinalEventsTable   A pointer to the memory address of the final event table.
**/
VOID
EFIAPI
DumpEventLog (
  IN EFI_TCG2_EVENT_LOG_FORMAT    EventLogFormat,
  IN EFI_PHYSICAL_ADDRESS         EventLogLocation,
  IN EFI_PHYSICAL_ADDRESS         EventLogLastEntry,
  IN EFI_TCG2_FINAL_EVENTS_TABLE  *FinalEventsTable
  );

/**
  This function dumps the provided PCR digest.

  @param[in]  PcrIndex   The index of the PCR.
  @param[in]  HashAlg    The hash algorithm used.
  @param[in]  PcrValues  The digest to be dumped.
**/
VOID
EFIAPI
DumpPcrDigest (
  IN UINT32         PcrIndex,
  IN TPMI_ALG_HASH  HashAlg,
  IN TPML_DIGEST    *PcrValues
  );

#endif // TPM_2_DEBUG_LIB_H_
