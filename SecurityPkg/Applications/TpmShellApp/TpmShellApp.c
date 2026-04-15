/** @file

A UEFI shell application for testing TPM 2.0 Physical Presence Interface
operations, including querying and configuring PCR banks.

Copyright (C) Microsoft Corporation.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>

#include <Protocol/Tcg2Protocol.h>
#include <IndustryStandard/Tpm20.h>
#include <IndustryStandard/UefiTcgPlatform.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/BaseCryptLib.h>
#include <Library/ShellLib.h>
#include <Library/UefiApplicationEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>

// Largest supported digest size (SHA-512).
#define MAX_DIGEST_SIZE  SHA512_DIGEST_SIZE

// TPM2_PCR_Read command size (tag + size + code + pcrSelectionIn).
#define TPM2_PCR_READ_CMD_SIZE  20

// Offset to the first digest byte in a TPM2_PCR_Read response.
#define TPM2_PCR_READ_RSP_DIGEST_OFFSET  30

typedef struct {
  UINT32         HashMask;
  UINT16         AlgId;
  CONST CHAR8    *Name;
} HASH_ALG_INFO;

STATIC CONST HASH_ALG_INFO  mHashAlgTable[] = {
  { EFI_TCG2_BOOT_HASH_ALG_SHA1,    TPM_ALG_SHA1,    "SHA1"    },
  { EFI_TCG2_BOOT_HASH_ALG_SHA256,  TPM_ALG_SHA256,  "SHA256"  },
  { EFI_TCG2_BOOT_HASH_ALG_SHA384,  TPM_ALG_SHA384,  "SHA384"  },
  { EFI_TCG2_BOOT_HASH_ALG_SHA512,  TPM_ALG_SHA512,  "SHA512"  },
  { EFI_TCG2_BOOT_HASH_ALG_SM3_256, TPM_ALG_SM3_256, "SM3_256" },
};

// Event type name lookup table.
typedef struct {
  UINT32         EventType;
  CONST CHAR8    *Name;
} EVENT_TYPE_INFO;

STATIC CONST EVENT_TYPE_INFO  mEventTypeTable[] = {
  { EV_POST_CODE,                     "EV_POST_CODE"                     },
  { EV_NO_ACTION,                     "EV_NO_ACTION"                     },
  { EV_SEPARATOR,                     "EV_SEPARATOR"                     },
  { EV_ACTION,                        "EV_ACTION"                        },
  { EV_S_CRTM_CONTENTS,               "EV_S_CRTM_CONTENTS"               },
  { EV_S_CRTM_VERSION,                "EV_S_CRTM_VERSION"                },
  { EV_TABLE_OF_DEVICES,              "EV_TABLE_OF_DEVICES"              },
  { EV_EFI_VARIABLE_DRIVER_CONFIG,    "EV_EFI_VARIABLE_DRIVER_CONFIG"    },
  { EV_EFI_VARIABLE_BOOT,             "EV_EFI_VARIABLE_BOOT"             },
  { EV_EFI_BOOT_SERVICES_APPLICATION, "EV_EFI_BOOT_SERVICES_APPLICATION" },
  { EV_EFI_BOOT_SERVICES_DRIVER,      "EV_EFI_BOOT_SERVICES_DRIVER"      },
  { EV_EFI_RUNTIME_SERVICES_DRIVER,   "EV_EFI_RUNTIME_SERVICES_DRIVER"   },
  { EV_EFI_GPT_EVENT,                 "EV_EFI_GPT_EVENT"                 },
  { EV_EFI_ACTION,                    "EV_EFI_ACTION"                    },
  { EV_EFI_PLATFORM_FIRMWARE_BLOB,    "EV_EFI_PLATFORM_FIRMWARE_BLOB"    },
  { EV_EFI_HANDOFF_TABLES,            "EV_EFI_HANDOFF_TABLES"            },
  { EV_EFI_PLATFORM_FIRMWARE_BLOB2,   "EV_EFI_PLATFORM_FIRMWARE_BLOB2"   },
  { EV_EFI_HANDOFF_TABLES2,           "EV_EFI_HANDOFF_TABLES2"           },
  { EV_EFI_VARIABLE_AUTHORITY,        "EV_EFI_VARIABLE_AUTHORITY"        },
};

/**
  Print a bitmask of hash algorithms as human-readable names.

  @param[in] AlgBitmap  Bitmask of HASH_ALG_* values.
**/
STATIC
VOID
PrintAlgorithms (
  IN UINT32  AlgBitmap
  )
{
  UINTN  Index;

  for (Index = 0; Index < ARRAY_SIZE (mHashAlgTable); Index++) {
    if ((AlgBitmap & mHashAlgTable[Index].HashMask) != 0) {
      Print (L" * %a\n", mHashAlgTable[Index].Name);
    }
  }
}

/**
  Query and display TPM PCR bank information via TCG2 Protocol.

  Retrieves the supported and currently active PCR banks from the
  TCG2 Protocol capability structure.

  @param[in] Tcg2Protocol  Pointer to the TCG2 Protocol instance.

  @retval EFI_SUCCESS           Information retrieved and displayed.
  @retval Others                Error querying the protocol.
**/
STATIC
EFI_STATUS
ShowPcrBanks (
  IN EFI_TCG2_PROTOCOL  *Tcg2Protocol
  )
{
  EFI_STATUS                        Status;
  EFI_TCG2_BOOT_SERVICE_CAPABILITY  Capability;

  Capability.Size = sizeof (Capability);
  Status          = Tcg2Protocol->GetCapability (Tcg2Protocol, &Capability);
  if (EFI_ERROR (Status)) {
    Print (L"GetCapability failed - %r\n", Status);
    return Status;
  }

  Print (L"Supported PCR banks bitmap: 0x%x\n", Capability.HashAlgorithmBitmap);
  PrintAlgorithms (Capability.HashAlgorithmBitmap);
  Print (L"\n");

  Print (L"Active PCR banks: 0x%x\n", Capability.ActivePcrBanks);
  PrintAlgorithms (Capability.ActivePcrBanks);

  return EFI_SUCCESS;
}

/**
  Submit a request to set specific PCR banks via TCG2 Protocol.

  This calls TCG2 Protocol SetActivePcrBanks which submits a Physical
  Presence request internally. The change will take effect on the next
  reboot.

  @param[in] Tcg2Protocol  Pointer to the TCG2 Protocol instance.
  @param[in] DesiredBanks  Bitmask of desired PCR banks
                           (EFI_TCG2_BOOT_HASH_ALG_* values).

  @retval EFI_SUCCESS       Request submitted successfully.
  @retval Others            Error submitting the request.
**/
STATIC
EFI_STATUS
RequestSetPcrBanks (
  IN EFI_TCG2_PROTOCOL  *Tcg2Protocol,
  IN UINT32             DesiredBanks
  )
{
  EFI_STATUS  Status;

  Print (L"Submitting SetActivePcrBanks with parameter 0x%x\n", DesiredBanks);

  Status = Tcg2Protocol->SetActivePcrBanks (Tcg2Protocol, DesiredBanks);

  Print (L"Status: %r\n", Status);

  if (EFI_ERROR (Status)) {
    Print (L"SetActivePcrBanks failed.\n");
    return Status;
  }

  Print (L"Request submitted. Changes will take effect after reboot.\n");
  return EFI_SUCCESS;
}

/**
  Submit a request to enable all supported PCR banks via TCG2 Protocol.

  @param[in] Tcg2Protocol  Pointer to the TCG2 Protocol instance.

  @retval EFI_SUCCESS       Request submitted successfully.
  @retval Others            Error submitting the request.
**/
STATIC
EFI_STATUS
RequestEnableAllPcrBanks (
  IN EFI_TCG2_PROTOCOL  *Tcg2Protocol
  )
{
  EFI_STATUS                        Status;
  EFI_TCG2_BOOT_SERVICE_CAPABILITY  Capability;

  Capability.Size = sizeof (Capability);
  Status          = Tcg2Protocol->GetCapability (Tcg2Protocol, &Capability);
  if (EFI_ERROR (Status)) {
    Print (L"GetCapability failed - %r\n", Status);
    return Status;
  }

  Print (L"Requesting all supported PCR banks: 0x%x\n", Capability.HashAlgorithmBitmap);

  Status = Tcg2Protocol->SetActivePcrBanks (Tcg2Protocol, Capability.HashAlgorithmBitmap);

  Print (L"Status: %r\n", Status);

  if (EFI_ERROR (Status)) {
    Print (L"SetActivePcrBanks failed.\n");
    return Status;
  }

  Print (L"Request submitted. All supported PCR banks will be enabled after reboot.\n");
  return EFI_SUCCESS;
}

/**
  Return a human-readable name for a TPM algorithm ID.

  @param[in] AlgId  TPM_ALG_ID value.

  @return Static string name, or "UNKNOWN" if not recognized.
**/
STATIC
CONST CHAR8 *
GetAlgName (
  IN UINT16  AlgId
  )
{
  UINTN  Index;

  for (Index = 0; Index < ARRAY_SIZE (mHashAlgTable); Index++) {
    if (mHashAlgTable[Index].AlgId == AlgId) {
      return mHashAlgTable[Index].Name;
    }
  }

  return "UNKNOWN";
}

/**
  Return a human-readable name for a TCG event type.

  @param[in] EventType  TCG_EVENTTYPE value.

  @return Static string name, or "UNKNOWN" if not recognized.
**/
STATIC
CONST CHAR8 *
GetEventTypeName (
  IN UINT32  EventType
  )
{
  UINTN  Index;

  for (Index = 0; Index < ARRAY_SIZE (mEventTypeTable); Index++) {
    if (mEventTypeTable[Index].EventType == EventType) {
      return mEventTypeTable[Index].Name;
    }
  }

  return "UNKNOWN";
}

/**
  Print a byte buffer as a hexadecimal string.

  @param[in] Data  Pointer to the byte buffer.
  @param[in] Size  Number of bytes to print.
**/
STATIC
VOID
PrintHexDump (
  IN UINT8   *Data,
  IN UINT16  Size
  )
{
  UINT16  Index;

  for (Index = 0; Index < Size; Index++) {
    Print (L"%02x ", Data[Index]);
  }
}

/**
  Retrieve and parse the TCG2 crypto-agile event log header.

  Calls GetEventLog(), validates the Spec ID Event, and extracts the
  algorithm list. On success the caller receives pointers to the first
  TCG_PCR_EVENT2 entry and the last entry, plus the algorithm table.

  @param[in]  Tcg2Protocol       Pointer to the TCG2 Protocol instance.
  @param[out] FirstEvent         Receives pointer to the first TCG_PCR_EVENT2.
  @param[out] LastEvent          Receives pointer to the last  TCG_PCR_EVENT2.
  @param[out] AlgList            Receives the algorithm/digest-size table.
  @param[out] NumberOfAlgorithms Receives the number of algorithms.
  @param[out] WasTruncated       Receives TRUE if the log was truncated.

  @retval EFI_SUCCESS      Event log parsed successfully.
  @retval EFI_NOT_FOUND    Event log is empty.
  @retval EFI_UNSUPPORTED  Event log format is not recognized.
  @retval Others           Error from GetEventLog.
**/
STATIC
EFI_STATUS
GetParsedEventLog (
  IN  EFI_TCG2_PROTOCOL                *Tcg2Protocol,
  OUT UINT8                            **FirstEvent,
  OUT UINT8                            **LastEvent,
  OUT TCG_EfiSpecIdEventAlgorithmSize  *AlgList,
  OUT UINT32                           *NumberOfAlgorithms,
  OUT BOOLEAN                          *WasTruncated
  )
{
  EFI_STATUS                       Status;
  EFI_PHYSICAL_ADDRESS             LogLocation;
  EFI_PHYSICAL_ADDRESS             LogLastEntry;
  TCG_PCR_EVENT_HDR                *SpecIdHdr;
  TCG_EfiSpecIDEventStruct         *SpecId;
  UINT8                            *LogData;
  UINT32                           AlgIndex;
  TCG_EfiSpecIdEventAlgorithmSize  *AlgEntry;

  Status = Tcg2Protocol->GetEventLog (
                           Tcg2Protocol,
                           EFI_TCG2_EVENT_LOG_FORMAT_TCG_2,
                           &LogLocation,
                           &LogLastEntry,
                           WasTruncated
                           );
  if (EFI_ERROR (Status)) {
    Print (L"GetEventLog failed - %r\n", Status);
    return Status;
  }

  if ((LogLocation == 0) || (LogLastEntry == 0)) {
    Print (L"Event log is empty.\n");
    return EFI_NOT_FOUND;
  }

  // The first entry is a v1 TCG_PCR_EVENT containing the Spec ID Event.
  SpecIdHdr = (TCG_PCR_EVENT_HDR *)(UINTN)LogLocation;
  if (SpecIdHdr->EventType != EV_NO_ACTION) {
    Print (L"First event is not a Spec ID Event (type 0x%x).\n", SpecIdHdr->EventType);
    return EFI_UNSUPPORTED;
  }

  SpecId = (TCG_EfiSpecIDEventStruct *)((UINT8 *)SpecIdHdr + sizeof (TCG_PCR_EVENT_HDR));

  if (AsciiStrCmp (
        (CONST CHAR8 *)SpecId->signature,
        TCG_EfiSpecIDEventStruct_SIGNATURE_03
        ) != 0)
  {
    Print (L"Unsupported event log format (expected crypto-agile).\n");
    return EFI_UNSUPPORTED;
  }

  // Read the algorithm list from the variable-length portion of the Spec ID Event.
  LogData             = (UINT8 *)SpecId + sizeof (TCG_EfiSpecIDEventStruct);
  *NumberOfAlgorithms = *(UINT32 *)LogData;
  LogData            += sizeof (UINT32);

  if ((*NumberOfAlgorithms == 0) || (*NumberOfAlgorithms > ARRAY_SIZE (mHashAlgTable))) {
    Print (L"Invalid algorithm count in Spec ID Event (%u).\n", *NumberOfAlgorithms);
    return EFI_UNSUPPORTED;
  }

  for (AlgIndex = 0; AlgIndex < *NumberOfAlgorithms; AlgIndex++) {
    AlgEntry          = (TCG_EfiSpecIdEventAlgorithmSize *)LogData;
    AlgList[AlgIndex] = *AlgEntry;
    LogData          += sizeof (TCG_EfiSpecIdEventAlgorithmSize);
  }

  *FirstEvent = (UINT8 *)(UINTN)LogLocation + sizeof (TCG_PCR_EVENT_HDR) + SpecIdHdr->EventSize;
  *LastEvent  = (UINT8 *)(UINTN)LogLastEntry;

  return EFI_SUCCESS;
}

/**
  Retrieve, parse, and display the TCG2 crypto-agile event log.

  Retrieves the event log via GetParsedEventLog(), then walks each
  subsequent TCG_PCR_EVENT2 entry displaying the PCR index, event
  type, and digest values.

  @param[in] Tcg2Protocol  Pointer to the TCG2 Protocol instance.

  @retval EFI_SUCCESS      Event log retrieved and displayed.
  @retval EFI_NOT_FOUND    Event log is empty.
  @retval EFI_UNSUPPORTED  Event log format is not recognized.
  @retval EFI_ABORTED      Event log entry is malformed.
  @retval Others           Error from GetEventLog.
**/
STATIC
EFI_STATUS
DumpEventLog (
  IN EFI_TCG2_PROTOCOL  *Tcg2Protocol
  )
{
  EFI_STATUS                       Status;
  BOOLEAN                          Truncated;
  UINT32                           NumberOfAlgorithms;
  TCG_EfiSpecIdEventAlgorithmSize  AlgList[ARRAY_SIZE (mHashAlgTable)];
  UINT32                           AlgIndex;
  UINT8                            *CurrentEvent;
  UINT8                            *LastEvent;
  UINTN                            EventNum;
  UINT32                           PcrIndex;
  UINT32                           EventType;
  UINT32                           DigestCount;
  UINT16                           AlgId;
  UINT16                           DigestSize;
  UINT32                           EventDataSize;
  UINT32                           DigestIndex;
  UINT32                           MatchIndex;
  UINT8                            *Digest;

  Status = GetParsedEventLog (
             Tcg2Protocol,
             &CurrentEvent,
             &LastEvent,
             AlgList,
             &NumberOfAlgorithms,
             &Truncated
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (Truncated) {
    Print (L"Warning: Event log was truncated.\n\n");
  }

  Print (L"Spec ID Event: %u algorithm(s)\n", NumberOfAlgorithms);
  for (AlgIndex = 0; AlgIndex < NumberOfAlgorithms; AlgIndex++) {
    Print (
      L"  %a (alg 0x%x, digest %u bytes)\n",
      GetAlgName (AlgList[AlgIndex].algorithmId),
      AlgList[AlgIndex].algorithmId,
      AlgList[AlgIndex].digestSize
      );
  }

  Print (L"\n");
  EventNum = 0;

  while (CurrentEvent <= LastEvent) {
    EventNum++;

    // Read fixed fields: PCRIndex, EventType, DigestCount.
    PcrIndex    = *(UINT32 *)CurrentEvent;
    EventType   = *(UINT32 *)(CurrentEvent + sizeof (UINT32));
    DigestCount = *(UINT32 *)(CurrentEvent + 2 * sizeof (UINT32));

    Print (
      L"Event %u: PCR %u %a (0x%x)\n",
      EventNum,
      PcrIndex,
      GetEventTypeName (EventType),
      EventType
      );

    if (DigestCount > NumberOfAlgorithms) {
      Print (L"  ERROR: DigestCount (%u) exceeds algorithm count. Stopping.\n", DigestCount);
      return EFI_ABORTED;
    }

    // Walk the digest list: each digest has a UINT16 AlgId followed by digest bytes.
    Digest = CurrentEvent + 3 * sizeof (UINT32);
    for (DigestIndex = 0; DigestIndex < DigestCount; DigestIndex++) {
      AlgId   = *(UINT16 *)Digest;
      Digest += sizeof (UINT16);

      // Look up the digest size from the Spec ID Event algorithm list.
      DigestSize = 0;
      for (MatchIndex = 0; MatchIndex < NumberOfAlgorithms; MatchIndex++) {
        if (AlgList[MatchIndex].algorithmId == AlgId) {
          DigestSize = AlgList[MatchIndex].digestSize;
          break;
        }
      }

      if (DigestSize == 0) {
        Print (L"  ERROR: Unknown algorithm 0x%x. Stopping.\n", AlgId);
        return EFI_ABORTED;
      }

      Print (L"  %a: ", GetAlgName (AlgId));
      PrintHexDump (Digest, DigestSize);
      Print (L"\n");

      Digest += DigestSize;
    }

    // Read EventSize and advance past the event data.
    EventDataSize = *(UINT32 *)Digest;
    Print (L"  Event data: %u bytes\n", EventDataSize);

    CurrentEvent = Digest + sizeof (UINT32) + EventDataSize;
  }

  Print (L"\nTotal: %u event(s)\n", EventNum);

  return EFI_SUCCESS;
}

/**
  Extend a PCR accumulator with a new digest.

  Computes NewPcr = Hash(CurrentPcr || Digest) using the hash algorithm
  identified by AlgId. CurrentPcr and NewPcr may point to the same buffer.

  @param[in]  AlgId       TPM_ALG_ID identifying the hash algorithm.
  @param[in]  CurrentPcr  Current PCR value (DigestSize bytes).
  @param[in]  Digest      Digest to extend with (DigestSize bytes).
  @param[in]  DigestSize  Size of CurrentPcr and Digest in bytes.
  @param[out] NewPcr      Receives the extended PCR value (DigestSize bytes).

  @retval EFI_SUCCESS      Extension computed.
  @retval EFI_UNSUPPORTED  Algorithm not supported for replay.
  @retval EFI_DEVICE_ERROR Hash computation failed.
**/
STATIC
EFI_STATUS
HashExtend (
  IN     UINT16  AlgId,
  IN     UINT8   *CurrentPcr,
  IN     UINT8   *Digest,
  IN     UINT16  DigestSize,
  OUT    UINT8   *NewPcr
  )
{
  UINT8    Buffer[MAX_DIGEST_SIZE * 2];
  BOOLEAN  Result;

  CopyMem (Buffer, CurrentPcr, DigestSize);
  CopyMem (Buffer + DigestSize, Digest, DigestSize);

  switch (AlgId) {
    case TPM_ALG_SHA1:
      Result = Sha1HashAll (Buffer, (UINTN)DigestSize * 2, NewPcr);
      break;
    case TPM_ALG_SHA256:
      Result = Sha256HashAll (Buffer, (UINTN)DigestSize * 2, NewPcr);
      break;
    case TPM_ALG_SHA384:
      Result = Sha384HashAll (Buffer, (UINTN)DigestSize * 2, NewPcr);
      break;
    case TPM_ALG_SHA512:
      Result = Sha512HashAll (Buffer, (UINTN)DigestSize * 2, NewPcr);
      break;
    default:
      return EFI_UNSUPPORTED;
  }

  return Result ? EFI_SUCCESS : EFI_DEVICE_ERROR;
}

/**
  Read a single PCR value from the TPM via SubmitCommand.

  Builds a TPM2_PCR_Read command and sends it through
  Tcg2Protocol->SubmitCommand(), avoiding any dependency on
  Tpm2CommandLib.

  @param[in]  Tcg2Protocol  Pointer to the TCG2 Protocol instance.
  @param[in]  AlgId         TPM_ALG_ID for the PCR bank to read.
  @param[in]  PcrIndex      PCR index (0..23).
  @param[in]  DigestSize    Expected digest size in bytes.
  @param[out] PcrValue      Receives the PCR digest (DigestSize bytes).

  @retval EFI_SUCCESS            PCR value read successfully.
  @retval EFI_INVALID_PARAMETER  PcrIndex out of range.
  @retval EFI_DEVICE_ERROR       TPM returned a non-zero response code.
  @retval Others                 Error from SubmitCommand.
**/
STATIC
EFI_STATUS
ReadPcrValue (
  IN  EFI_TCG2_PROTOCOL  *Tcg2Protocol,
  IN  UINT16             AlgId,
  IN  UINT32             PcrIndex,
  IN  UINT16             DigestSize,
  OUT UINT8              *PcrValue
  )
{
  EFI_STATUS  Status;
  UINT8       CmdBuffer[TPM2_PCR_READ_CMD_SIZE];
  UINT8       RspBuffer[TPM2_PCR_READ_RSP_DIGEST_OFFSET + MAX_DIGEST_SIZE];
  UINT32      RspSize;
  UINT32      ResponseCode;
  UINT8       *Ptr;

  if (PcrIndex > MAX_PCR_INDEX) {
    return EFI_INVALID_PARAMETER;
  }

  // Build TPM2_PCR_Read command (big-endian wire format).
  // Layout (20 bytes total):
  //   [0-1]   tag            = TPM_ST_NO_SESSIONS
  //   [2-5]   commandSize    = 20
  //   [6-9]   commandCode    = TPM_CC_PCR_Read
  //   [10-13] count          = 1
  //   [14-15] hash           = AlgId
  //   [16]    sizeofSelect   = 3
  //   [17-19] pcrSelect[3]   = bit for PcrIndex
  Ptr = CmdBuffer;

  *(UINT16 *)Ptr = SwapBytes16 (TPM_ST_NO_SESSIONS);
  Ptr           += sizeof (UINT16);

  *(UINT32 *)Ptr = SwapBytes32 ((UINT32)TPM2_PCR_READ_CMD_SIZE);
  Ptr           += sizeof (UINT32);

  *(UINT32 *)Ptr = SwapBytes32 (TPM_CC_PCR_Read);
  Ptr           += sizeof (UINT32);

  *(UINT32 *)Ptr = SwapBytes32 (1);
  Ptr           += sizeof (UINT32);

  *(UINT16 *)Ptr = SwapBytes16 (AlgId);
  Ptr           += sizeof (UINT16);

  *Ptr = 3;
  Ptr++;

  Ptr[0]            = 0;
  Ptr[1]            = 0;
  Ptr[2]            = 0;
  Ptr[PcrIndex / 8] = (UINT8)(1 << (PcrIndex % 8));

  // Send command and read response.
  RspSize = sizeof (RspBuffer);
  Status  = Tcg2Protocol->SubmitCommand (
                            Tcg2Protocol,
                            TPM2_PCR_READ_CMD_SIZE,
                            CmdBuffer,
                            RspSize,
                            RspBuffer
                            );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  // Response layout (for a single PCR):
  //   [0-1]   tag
  //   [2-5]   responseSize
  //   [6-9]   responseCode
  //   [10-13] pcrUpdateCounter
  //   [14-17] pcrSelectionOut.count
  //   [18-19] hash
  //   [20]    sizeofSelect
  //   [21-23] pcrSelect[3]
  //   [24-27] digests.count
  //   [28-29] digests[0].size
  //   [30+]   digests[0].buffer
  ResponseCode = SwapBytes32 (*(UINT32 *)(RspBuffer + 6));
  if (ResponseCode != 0) {
    return EFI_DEVICE_ERROR;
  }

  CopyMem (PcrValue, RspBuffer + TPM2_PCR_READ_RSP_DIGEST_OFFSET, DigestSize);

  return EFI_SUCCESS;
}

/**
  Replay the TCG2 event log and verify against actual TPM PCR values.

  Retrieves the event log, replays all extend operations locally to
  compute expected PCR values, reads the actual PCR values from the
  TPM via SubmitCommand, and compares them.

  @param[in] Tcg2Protocol  Pointer to the TCG2 Protocol instance.

  @retval EFI_SUCCESS      All replayed PCRs match the TPM.
  @retval EFI_NOT_FOUND    Event log is empty.
  @retval EFI_UNSUPPORTED  Event log format not recognized.
  @retval EFI_ABORTED      Replay completed but one or more PCRs did not match.
  @retval Others           Error from GetEventLog or SubmitCommand.
**/
STATIC
EFI_STATUS
ReplayAndVerifyEventLog (
  IN EFI_TCG2_PROTOCOL  *Tcg2Protocol
  )
{
  EFI_STATUS                       Status;
  BOOLEAN                          Truncated;
  UINT32                           NumberOfAlgorithms;
  TCG_EfiSpecIdEventAlgorithmSize  AlgList[ARRAY_SIZE (mHashAlgTable)];
  BOOLEAN                          AlgReplayable[ARRAY_SIZE (mHashAlgTable)];
  UINT32                           AlgIndex;
  UINT8                            *CurrentEvent;
  UINT8                            *LastEvent;
  UINTN                            EventNum;
  UINT32                           PcrIndex;
  UINT32                           DigestCount;
  UINT16                           AlgId;
  UINT16                           DigestSize;
  UINT32                           EventDataSize;
  UINT32                           DigestIndex;
  UINT32                           MatchIndex;
  UINT8                            ReplayedPcrs[ARRAY_SIZE (mHashAlgTable)][MAX_PCR_INDEX + 1][MAX_DIGEST_SIZE];
  BOOLEAN                          PcrUsed[MAX_PCR_INDEX + 1];
  UINT8                            TpmPcrValue[MAX_DIGEST_SIZE];
  UINTN                            PcrIdx;
  UINTN                            PassCount;
  UINTN                            FailCount;
  UINTN                            VerifiedCount;
  BOOLEAN                          Match;
  UINT8                            *Digest;

  Status = GetParsedEventLog (
             Tcg2Protocol,
             &CurrentEvent,
             &LastEvent,
             AlgList,
             &NumberOfAlgorithms,
             &Truncated
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (Truncated) {
    Print (L"Warning: Event log was truncated. Replay may not match.\n\n");
  }

  for (AlgIndex = 0; AlgIndex < NumberOfAlgorithms; AlgIndex++) {
    AlgReplayable[AlgIndex] = TRUE;
  }

  // Initialize PCR accumulators to zero.
  ZeroMem (ReplayedPcrs, sizeof (ReplayedPcrs));
  ZeroMem (PcrUsed, sizeof (PcrUsed));

  // Walk events and replay PCR extensions.
  EventNum = 0;

  while (CurrentEvent <= LastEvent) {
    EventNum++;

    PcrIndex    = *(UINT32 *)CurrentEvent;
    DigestCount = *(UINT32 *)(CurrentEvent + 2 * sizeof (UINT32));

    if (DigestCount > NumberOfAlgorithms) {
      Print (L"Event %u: DigestCount (%u) exceeds algorithm count. Stopping.\n", EventNum, DigestCount);
      return EFI_ABORTED;
    }

    if (PcrIndex <= MAX_PCR_INDEX) {
      PcrUsed[PcrIndex] = TRUE;
    }

    Digest = CurrentEvent + 3 * sizeof (UINT32);
    for (DigestIndex = 0; DigestIndex < DigestCount; DigestIndex++) {
      AlgId   = *(UINT16 *)Digest;
      Digest += sizeof (UINT16);

      DigestSize = 0;
      MatchIndex = 0;
      for (MatchIndex = 0; MatchIndex < NumberOfAlgorithms; MatchIndex++) {
        if (AlgList[MatchIndex].algorithmId == AlgId) {
          DigestSize = AlgList[MatchIndex].digestSize;
          break;
        }
      }

      if (DigestSize == 0) {
        Print (L"Event %u: Unknown algorithm 0x%x. Stopping.\n", EventNum, AlgId);
        return EFI_ABORTED;
      }

      if ((PcrIndex <= MAX_PCR_INDEX) && AlgReplayable[MatchIndex]) {
        Status = HashExtend (
                   AlgId,
                   ReplayedPcrs[MatchIndex][PcrIndex],
                   Digest,
                   DigestSize,
                   ReplayedPcrs[MatchIndex][PcrIndex]
                   );
        if (Status == EFI_UNSUPPORTED) {
          AlgReplayable[MatchIndex] = FALSE;
        } else if (EFI_ERROR (Status)) {
          Print (L"Event %u: Hash extend failed for %a - %r\n", EventNum, GetAlgName (AlgId), Status);
          return Status;
        }
      }

      Digest += DigestSize;
    }

    EventDataSize = *(UINT32 *)Digest;
    CurrentEvent  = Digest + sizeof (UINT32) + EventDataSize;
  }

  Print (L"Replayed %u event(s) across %u algorithm(s).\n\n", EventNum, NumberOfAlgorithms);

  // Verify replayed PCRs against actual TPM values.
  PassCount     = 0;
  FailCount     = 0;
  VerifiedCount = 0;

  for (PcrIdx = 0; PcrIdx <= MAX_PCR_INDEX; PcrIdx++) {
    if (!PcrUsed[PcrIdx]) {
      continue;
    }

    Print (L"PCR %u:\n", PcrIdx);

    for (AlgIndex = 0; AlgIndex < NumberOfAlgorithms; AlgIndex++) {
      AlgId      = AlgList[AlgIndex].algorithmId;
      DigestSize = AlgList[AlgIndex].digestSize;

      if (!AlgReplayable[AlgIndex]) {
        Print (L"  %a: (replay not supported, skipped)\n", GetAlgName (AlgId));
        continue;
      }

      Print (L"  %a Replayed: ", GetAlgName (AlgId));
      PrintHexDump (ReplayedPcrs[AlgIndex][PcrIdx], DigestSize);
      Print (L"\n");

      Status = ReadPcrValue (Tcg2Protocol, AlgId, (UINT32)PcrIdx, DigestSize, TpmPcrValue);
      if (EFI_ERROR (Status)) {
        Print (L"  %a Actual:   (read failed - %r)\n", GetAlgName (AlgId), Status);
        FailCount++;
        VerifiedCount++;
        continue;
      }

      Print (L"  %a Actual:   ", GetAlgName (AlgId));
      PrintHexDump (TpmPcrValue, DigestSize);
      Print (L"\n");

      Match = (CompareMem (ReplayedPcrs[AlgIndex][PcrIdx], TpmPcrValue, DigestSize) == 0);
      Print (L"  Result: %a\n", Match ? "PASS" : "FAIL");

      if (Match) {
        PassCount++;
      } else {
        FailCount++;
      }

      VerifiedCount++;
    }

    Print (L"\n");
  }

  Print (L"Summary: %u PCR bank(s) verified, %u PASS, %u FAIL\n", VerifiedCount, PassCount, FailCount);

  return (FailCount > 0) ? EFI_ABORTED : EFI_SUCCESS;
}

/**
  Print usage information.
**/
STATIC
VOID
PrintUsage (
  VOID
  )
{
  Print (L"TpmShellApp - TPM 2.0 Physical Presence Test Utility\n");
  Print (L"\n");
  Print (L"Usage:\n");
  Print (L"  TpmShellApp help             - Show usage\n");
  Print (L"  TpmShellApp getpcr           - Show supported PCR banks\n");
  Print (L"  TpmShellApp setpcr <mask>    - Request PCR bank change (hex bitmask)\n");
  Print (L"  TpmShellApp enableall        - Enable all supported PCR banks\n");
  Print (L"  TpmShellApp dumplog          - Dump the TCG2 event log\n");
  Print (L"  TpmShellApp replay           - Replay event log and verify PCRs\n");
  Print (L"  TpmShellApp getresult        - Show last SetActivePcrBanks result\n");
  Print (L"\n");
  Print (L"PCR bank bitmask values:\n");
  Print (L"  0x00000001 = SHA1\n");
  Print (L"  0x00000002 = SHA256\n");
  Print (L"  0x00000004 = SHA384\n");
  Print (L"  0x00000008 = SHA512\n");
  Print (L"  0x00000010 = SM3_256\n");
  Print (L"\n");
  Print (L"Example: TpmShellApp setpcr 0x2   (enable SHA256 only)\n");
  Print (L"Example: TpmShellApp setpcr 0x6   (enable SHA256 + SHA384)\n");
}

/**
  Entry point for the TPM Test UEFI Shell Application.

  @param[in] ImageHandle  The firmware allocated handle for the EFI image.
  @param[in] SystemTable  A pointer to the EFI System Table.

  @retval EFI_SUCCESS     Application executed successfully.
  @retval Others          An error occurred.
**/
EFI_STATUS
EFIAPI
TpmShellAppEntry (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS         Status;
  UINTN              Argc;
  LIST_ENTRY         *ParamPackage;
  CONST CHAR16       *Command;
  CONST CHAR16       *PcrMaskStr;
  UINT64             PcrMaskVal;
  EFI_TCG2_PROTOCOL  *Tcg2Protocol;
  UINT32             OperationPresent;
  UINT32             Response;

  // Initialize the Shell library.
  Status = ShellInitialize ();
  if (EFI_ERROR (Status)) {
    Print (L"ShellInitialize failed - %r\n", Status);
    return Status;
  }

  // Parse command line.
  Status = ShellCommandLineParse (EmptyParamList, &ParamPackage, NULL, FALSE);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  // Validate number of input parameters.
  Argc = ShellCommandLineGetCount (ParamPackage);
  if (Argc < 2) {
    Print (L"\n[Invalid Usage]\n");
    Print (L"  Use 'TpmShellApp help' for usage.\n");
    Status = EFI_INVALID_PARAMETER;
    goto Exit;
  }

  // Acquire the command value.
  Command = ShellCommandLineGetRawValue (ParamPackage, 1);
  if (Command == NULL) {
    Print (L"\n[Invalid Usage]\n");
    Print (L"  Use 'TpmShellApp help' for usage.\n");
    Status = EFI_INVALID_PARAMETER;
    goto Exit;
  }

  // Locate TCG2 Protocol it is required for this test app to function
  Status = gBS->LocateProtocol (&gEfiTcg2ProtocolGuid, NULL, (VOID **)&Tcg2Protocol);
  if (EFI_ERROR (Status)) {
    Print (L"TCG2 Protocol not found - %r\n", Status);
    Print (L"  TPM 2.0 may not be enabled on this platform.\n");
    goto Exit;
  }

  // Dispatch based on command.
  if (StrCmp (Command, L"help") == 0) {
    Print (L"\n[TPM 2.0 Help]\n\n");
    PrintUsage ();
    goto Exit;
  } else if (StrCmp (Command, L"getpcr") == 0) {
    Print (L"\n[TPM 2.0 Information]\n\n");
    ShowPcrBanks (Tcg2Protocol);
  } else if (StrCmp (Command, L"setpcr") == 0) {
    Print (L"\n[Set PCR Banks]\n\n");
    if (Argc < 3) {
      Print (L"setpcr requires a hex bitmask parameter.\n");
      Print (L"  Example: TpmShellApp setpcr 0x2\n");
      Status = EFI_INVALID_PARAMETER;
      goto Exit;
    }

    PcrMaskStr = ShellCommandLineGetRawValue (ParamPackage, 2);
    Status     = ShellConvertStringToUint64 (
                   PcrMaskStr,
                   &PcrMaskVal,
                   TRUE,
                   FALSE
                   );
    if (EFI_ERROR (Status)) {
      Print (L"Invalid hex value.\n");
      goto Exit;
    }

    Status = RequestSetPcrBanks (Tcg2Protocol, (UINT32)PcrMaskVal);
  } else if (StrCmp (Command, L"enableall") == 0) {
    Print (L"\n[Enable All PCR Banks]\n\n");
    Status = RequestEnableAllPcrBanks (Tcg2Protocol);
  } else if (StrCmp (Command, L"dumplog") == 0) {
    Print (L"\n[TCG2 Event Log]\n\n");
    Status = DumpEventLog (Tcg2Protocol);
  } else if (StrCmp (Command, L"replay") == 0) {
    Print (L"\n[Event Log Replay]\n\n");
    Status = ReplayAndVerifyEventLog (Tcg2Protocol);
  } else if (StrCmp (Command, L"getresult") == 0) {
    Print (L"\n[Last SetActivePcrBanks Result]\n\n");
    Status = Tcg2Protocol->GetResultOfSetActivePcrBanks (Tcg2Protocol, &OperationPresent, &Response);
    if (EFI_ERROR (Status)) {
      Print (L"  GetResultOfSetActivePcrBanks failed - %r\n", Status);
      goto Exit;
    }

    Print (L"  Operation present: %a\n", OperationPresent ? "YES" : "NO");
    Print (L"  Response code:     %u\n", Response);
  } else {
    Print (L"\n[Invalid Input Command]\n\n");
    Print (L"  Unknown command '%s'\n", Command);
    Print (L"  Use 'TpmShellApp help' for usage.\n");
    Status = EFI_INVALID_PARAMETER;
  }

Exit:
  Print (L"\n");
  ShellCommandLineFreeVarList (ParamPackage);
  return Status;
}
