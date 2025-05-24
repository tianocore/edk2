/** @file -- Tpm2DebugLibSimple.c
This file contains helper functions to perform a simple debugging of
TPM transactions as they go to and from the TPM device.

Copyright (c) Microsoft Corporation.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>
#include <Library/DebugLib.h>
#include <Library/Tpm2DebugLib.h>

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
  )
{
  UINTN  Index, DebugSize;

  DEBUG ((DEBUG_INFO, "TpmCommand Send - "));

  if (InputBlockSize > 0x100) {
    DebugSize = 0x40;
  } else {
    DebugSize = InputBlockSize;
  }

  for (Index = 0; Index < DebugSize; Index++) {
    DEBUG ((DEBUG_INFO, "%02x ", InputBlock[Index]));
  }

  if (DebugSize != InputBlockSize) {
    DEBUG ((DEBUG_INFO, "...... "));

    for (Index = InputBlockSize - 0x20; Index < InputBlockSize; Index++) {
      DEBUG ((DEBUG_INFO, "%02x ", InputBlock[Index]));
    }
  }

  DEBUG ((DEBUG_INFO, "\n"));

  return;
}

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
  )
{
  UINTN  Index;

  DEBUG ((DEBUG_INFO, "TpmCommand Receive - "));

  for (Index = 0; Index < OutputBlockSize; Index++) {
    DEBUG ((DEBUG_INFO, "%02x ", OutputBlock[Index]));
  }

  DEBUG ((DEBUG_INFO, "\n"));

  return;
}

/**
  This function dump event log.

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
  )
{
  // Dumping the entire event log is left for the verbose version of this library, because it can be thousands of
  // lines long.
  DEBUG ((DEBUG_INFO, "EventLogFormat - 0x%llx\n", EventLogFormat));
  DEBUG ((DEBUG_INFO, "EventLogLocation - 0x%llx\n", EventLogLocation));
  DEBUG ((DEBUG_INFO, "EventLogLastEntry - 0x%llx\n", EventLogLastEntry));
  DEBUG ((DEBUG_INFO, "FinalEventsTable - 0x%llx\n", FinalEventsTable));
}

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
  )
{
  // This function simply marks we have read these PCRs, the verbose version of this function will dump the contents.
  DEBUG ((DEBUG_INFO, "PcrIndex - %d\n", PcrIndex));
  DEBUG ((DEBUG_INFO, "HashAlg - 0x%04x\n", HashAlg));
}
