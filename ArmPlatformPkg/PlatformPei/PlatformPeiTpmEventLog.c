/** @file

  Copyright (c) 2025, ARM Limited. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiPei.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/ArmPlatformLib.h>
#include <Library/ArmTransferListLib.h>
#include <Library/HobLib.h>
#include <Library/PcdLib.h>

#include <IndustryStandard/Tpm20.h>
#include <IndustryStandard/UefiTcgPlatform.h>

typedef struct {
  TPMI_ALG_HASH    HashAlgo;
  UINT32           HashMask;
  UINT16           HashSize;
  CHAR8            *HashName;
} INTERNAL_HASH_INFO;

STATIC INTERNAL_HASH_INFO  mHashInfo[] = {
  { TPM_ALG_SHA1,   HASH_ALG_SHA1,   SHA1_DIGEST_SIZE,   "SHA1"   },
  { TPM_ALG_SHA256, HASH_ALG_SHA256, SHA256_DIGEST_SIZE, "SHA256" },
  { TPM_ALG_SHA384, HASH_ALG_SHA384, SHA384_DIGEST_SIZE, "SHA384" },
  { TPM_ALG_SHA512, HASH_ALG_SHA512, SHA384_DIGEST_SIZE, "SHA512" },
};

typedef struct {
  UINTN    EventLogBase;
  UINTN    EventLogSize;
  VOID     *Event;
  UINTN    EventSize;
} EVENT_LOG_ITERATOR;

/**
  Get hash size based on Algo

  @param[in]     HashAlgo           Hash Algorithm Id.
  @param[out]    HashInfo           Hash Algorithm Information.

  @return Size of the hash.

**/
STATIC
EFI_STATUS
EFIAPI
GetHashInfo (
  IN TPMI_ALG_HASH        HashAlgo,
  OUT INTERNAL_HASH_INFO  **HashInfo
  )
{
  UINTN  Idx;

  for (Idx = 0; Idx < ARRAY_SIZE (mHashInfo); Idx++) {
    if (mHashInfo[Idx].HashAlgo == HashAlgo) {
      *HashInfo = &mHashInfo[Idx];
      return EFI_SUCCESS;
    }
  }

  return EFI_UNSUPPORTED;
}

/**
  Check Event is SpecId Event.

  @param[in]   Event                  Event

  @return      TRUE             Event is SpecId event.
  @return      FALSE            Event isn't SpecId event.

 **/
STATIC
BOOLEAN
EFIAPI
IsSpecIdEvent (
  IN VOID  *Event
  )
{
  TCG_PCR_EVENT             *TcgPcrEvent;
  TCG_EfiSpecIDEventStruct  *TcgEfiSpecIdEventStruct;

  TcgPcrEvent             = (TCG_PCR_EVENT *)Event;
  TcgEfiSpecIdEventStruct = (TCG_EfiSpecIDEventStruct *)
                            (Event + OFFSET_OF (TCG_PCR_EVENT, Event));

  if (!((TcgPcrEvent->EventType == EV_NO_ACTION) &&
        ((CompareMem (
            TcgEfiSpecIdEventStruct->signature,
            TCG_EfiSpecIDEventStruct_SIGNATURE_02,
            sizeof (TcgEfiSpecIdEventStruct->signature)
            ) == 0) ||
         (CompareMem (
            TcgEfiSpecIdEventStruct->signature,
            TCG_EfiSpecIDEventStruct_SIGNATURE_03,
            sizeof (TcgEfiSpecIdEventStruct->signature)
            ) == 0))))
  {
    return FALSE;
  }

  return TRUE;
}

/**
  Get size of SpecId Event.

  @param[in]   SpecIdEvent                  SpecId event

  @return      Size of SpecId event.

 **/
STATIC
UINTN
EFIAPI
GetSpecIdEventSize (
  IN  VOID  *SpecIdEvent
  )
{
  TCG_PCR_EVENT  *TcgPcrEvent;

  TcgPcrEvent = (TCG_PCR_EVENT *)SpecIdEvent;

  return OFFSET_OF (TCG_PCR_EVENT, Event) + TcgPcrEvent->EventSize;
}

/**
  Initialize event log iterator

  @param[out]    Iter               Iterator.
  @param[in]     EventLogBase       Event log base address.
  @param[in]     EventLogSize       Total event log size.

**/
STATIC
VOID
EFIAPI
EventLogIteratorInit (
  OUT EVENT_LOG_ITERATOR  *Iter,
  IN VOID                 *EventLogBase,
  IN UINTN                EventLogSize
  )
{
  Iter->EventLogBase = (UINTN)EventLogBase;
  Iter->EventLogSize = EventLogSize;
  Iter->Event        = NULL;
  Iter->EventSize    = 0;
}

/**
  Get next event form event log iterator

  @param[in,out]    Iter               Iterator
  @param[out]       NextEvent          Next event
  @param[out]       NextEventSize      Size of next event

  @return EFI_SUCCESS         Success to get next event.
  @return EFI_NOT_FOUND       No more event.

**/
STATIC
EFI_STATUS
EFIAPI
EventLogIteratorGetNext (
  IN OUT EVENT_LOG_ITERATOR  *Iter,
  OUT    VOID                **NextEvent,
  OUT    UINTN               *NextEventSize
  )
{
  EFI_STATUS          Status;
  TCG_PCR_EVENT2      *TcgPcrEvent2;
  UINTN               Idx;
  UINT32              DigestCount;
  TPMI_ALG_HASH       HashAlgo;
  INTERNAL_HASH_INFO  *HashInfo;
  VOID                *Event;
  UINT32              EventSize;

  if (Iter->Event == NULL) {
    Iter->Event = (VOID *)Iter->EventLogBase;
  } else {
    Iter->Event += Iter->EventSize;
  }

  if ((UINTN)Iter->Event >= (Iter->EventLogBase + Iter->EventLogSize)) {
    Status          = EFI_NOT_FOUND;
    Iter->Event     = NULL;
    Iter->EventSize = 0;
    goto ExitHandler;
  }

  if (IsSpecIdEvent (Iter->Event)) {
    Status          = EFI_SUCCESS;
    Iter->EventSize = GetSpecIdEventSize (Iter->Event);
    goto ExitHandler;
  }

  TcgPcrEvent2 = ((TCG_PCR_EVENT2 *)Iter->Event);
  DigestCount  = TcgPcrEvent2->Digest.count;
  HashAlgo     = TcgPcrEvent2->Digest.digests[0].hashAlg;
  Event        = (VOID *)&TcgPcrEvent2->Digest.digests[0].digest;

  for (Idx = 0; Idx < DigestCount; Idx++) {
    Status = GetHashInfo (HashAlgo, &HashInfo);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a: Can't get HashInfo for %d\n", __func__, HashAlgo));
      return Status;
    }

    Event += HashInfo->HashSize;
    CopyMem (&HashAlgo, Event, sizeof (TPMI_ALG_HASH));
    Event += sizeof (TPMI_ALG_HASH);
  }

  Event -= sizeof (TPMI_ALG_HASH);
  CopyMem (&EventSize, Event, sizeof (TcgPcrEvent2->EventSize));
  Event += sizeof (TcgPcrEvent2->EventSize);
  Event += EventSize;

  Iter->EventSize = (UINTN)Event - (UINTN)Iter->Event;

ExitHandler:
  *NextEvent     = Iter->Event;
  *NextEventSize = Iter->EventSize;
  return Status;
}

/**
  This function dump TCG_EfiSpecIDEventStruct.

  @param[in]   Event                  Event

**/
STATIC
VOID
EFIAPI
DumpTcgEfiSpecIdEvent (
  IN VOID  *Event
  )
{
  TCG_PCR_EVENT                    *TcgPcrEvent;
  TCG_EfiSpecIDEventStruct         *TcgEfiSpecIdEventStruct;
  TCG_EfiSpecIdEventAlgorithmSize  *DigestSize;
  UINTN                            Idx;
  UINT8                            *VendorInfoSize;
  UINT8                            *VendorInfo;
  UINT32                           NumberOfAlgorithms;

  TcgPcrEvent             = (TCG_PCR_EVENT *)Event;
  TcgEfiSpecIdEventStruct = (TCG_EfiSpecIDEventStruct *)
                            (Event + OFFSET_OF (TCG_PCR_EVENT, Event));

  DEBUG ((DEBUG_INFO, "  TCG_EfiSpecIDEvent:\n"));
  DEBUG ((DEBUG_INFO, "    PCRIndex  - %d\n", TcgPcrEvent->PCRIndex));
  DEBUG ((DEBUG_INFO, "    EventType - 0x%08x\n", TcgPcrEvent->EventType));

  DEBUG ((DEBUG_INFO, "    Digest: "));
  for (Idx = 0; Idx < TPM_SHA1_160_HASH_LEN; Idx++) {
    DEBUG ((DEBUG_INFO, "%02x ", TcgPcrEvent->Digest.digest[Idx]));
  }

  DEBUG ((DEBUG_INFO, "\n"));

  DEBUG ((DEBUG_INFO, "     Signature          - '"));
  for (Idx = 0; Idx < sizeof (TcgEfiSpecIdEventStruct->signature); Idx++) {
    DEBUG ((DEBUG_INFO, "%c", TcgEfiSpecIdEventStruct->signature[Idx]));
  }

  DEBUG ((DEBUG_INFO, "'\n"));

  DEBUG ((DEBUG_INFO, "     PlatformClass      - 0x%08x\n", TcgEfiSpecIdEventStruct->platformClass));
  DEBUG ((DEBUG_INFO, "     SpecVersion        - %d.%d%d\n", TcgEfiSpecIdEventStruct->specVersionMajor, TcgEfiSpecIdEventStruct->specVersionMinor, TcgEfiSpecIdEventStruct->specErrata));
  DEBUG ((DEBUG_INFO, "     UintnSize          - 0x%02x\n", TcgEfiSpecIdEventStruct->uintnSize));

  CopyMem (&NumberOfAlgorithms, TcgEfiSpecIdEventStruct + 1, sizeof (NumberOfAlgorithms));
  DEBUG ((DEBUG_INFO, "     NumberOfAlgorithms - 0x%08x\n", NumberOfAlgorithms));

  DigestSize = (TCG_EfiSpecIdEventAlgorithmSize *)((UINT8 *)TcgEfiSpecIdEventStruct + sizeof (*TcgEfiSpecIdEventStruct) + sizeof (NumberOfAlgorithms));
  for (Idx = 0; Idx < NumberOfAlgorithms; Idx++) {
    DEBUG ((DEBUG_INFO, "     Digest(%d)\n", Idx));
    DEBUG ((DEBUG_INFO, "       AlgorithmId      - 0x%04x\n", DigestSize[Idx].algorithmId));
    DEBUG ((DEBUG_INFO, "       DigestSize       - 0x%04x\n", DigestSize[Idx].digestSize));
  }

  VendorInfoSize = (UINT8 *)&DigestSize[NumberOfAlgorithms];
  DEBUG ((DEBUG_INFO, "    VendorInfoSize     - 0x%02x\n", *VendorInfoSize));
  VendorInfo = VendorInfoSize + 1;
  DEBUG ((DEBUG_INFO, "    VendorInfo         - "));
  for (Idx = 0; Idx < *VendorInfoSize; Idx++) {
    DEBUG ((DEBUG_INFO, "%02x ", VendorInfo[Idx]));
  }

  DEBUG ((DEBUG_INFO, "\n"));
}

/**
  This function Dump PCR event 2.

  @param[in]   Event                  Event

**/
STATIC
VOID
EFIAPI
DumpEvent (
  IN VOID  *Event
  )
{
  EFI_STATUS                   Status;
  UINTN                        Idx;
  UINT32                       DigestIdx;
  UINT32                       DigestCount;
  TPMI_ALG_HASH                HashAlgo;
  INTERNAL_HASH_INFO           *HashInfo;
  UINT8                        *DigestBuffer;
  UINT8                        *EventBuffer;
  UINT32                       EventSize;
  TCG_EfiStartupLocalityEvent  *StartupLocalityEvent;
  TCG_PCR_EVENT2               *TcgPcrEvent2;
  UINT16                       DigestSize;

  TcgPcrEvent2 = ((TCG_PCR_EVENT2 *)Event);

  DEBUG ((DEBUG_INFO, "  Event:\n"));
  DEBUG ((DEBUG_INFO, "    PCRIndex  - %d\n", TcgPcrEvent2->PCRIndex));
  DEBUG ((DEBUG_INFO, "    EventType - 0x%08x\n", TcgPcrEvent2->EventType));

  DEBUG ((DEBUG_INFO, "    DigestCount: 0x%08x\n", TcgPcrEvent2->Digest.count));

  DigestCount  = TcgPcrEvent2->Digest.count;
  HashAlgo     = TcgPcrEvent2->Digest.digests[0].hashAlg;
  DigestBuffer = (UINT8 *)&TcgPcrEvent2->Digest.digests[0].digest;

  for (DigestIdx = 0; DigestIdx < DigestCount; DigestIdx++) {
    Status = GetHashInfo (HashAlgo, &HashInfo);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_INFO, "      HashAlgo : Unknown(0x%04x)\n", HashAlgo));
      return;
    } else {
      DEBUG ((DEBUG_INFO, "      HashAlgo : %a(0x%04x)\n", HashInfo->HashName, HashAlgo));
      DigestSize = HashInfo->HashSize;
    }

    DEBUG ((DEBUG_INFO, "      Digest(%d): ", DigestIdx));
    for (Idx = 0; Idx < DigestSize; Idx++) {
      DEBUG ((DEBUG_INFO, "%02x ", DigestBuffer[Idx]));
    }

    DEBUG ((DEBUG_INFO, "\n"));

    //
    // Prepare next
    //
    CopyMem (&HashAlgo, DigestBuffer + DigestSize, sizeof (TPMI_ALG_HASH));
    DigestBuffer = DigestBuffer + DigestSize + sizeof (TPMI_ALG_HASH);
  }

  DEBUG ((DEBUG_INFO, "\n"));

  DigestBuffer = DigestBuffer - sizeof (TPMI_ALG_HASH);
  CopyMem (&EventSize, DigestBuffer, sizeof (TcgPcrEvent2->EventSize));
  DEBUG ((DEBUG_INFO, "    EventSize - 0x%08x\n", EventSize));
  EventBuffer          = DigestBuffer + sizeof (TcgPcrEvent2->EventSize);
  StartupLocalityEvent = (TCG_EfiStartupLocalityEvent *)EventBuffer;

  if ((EventSize == sizeof (TCG_EfiStartupLocalityEvent)) &&
      (CompareMem (
         &StartupLocalityEvent->Signature,
         TCG_EfiStartupLocalityEvent_SIGNATURE,
         sizeof (StartupLocalityEvent->Signature)
         ) == 0))
  {
    DEBUG ((DEBUG_INFO, "    Signature - %a\n", StartupLocalityEvent->Signature));
    DEBUG ((DEBUG_INFO, "    StartupLocality - 0x%08x\n", StartupLocalityEvent->StartupLocality));
  } else {
    DEBUG ((DEBUG_INFO, "    Event     - %a\n", EventBuffer));
  }
}

/**
  Dump passed tpm event log from TF-A.

  @param[in]   EventLog          TPM eventlog base
  @param[in]   EventLogSize      Size of TPM event log

 **/
STATIC
VOID
EFIAPI
DumpTpmEventLog (
  IN UINT8  *EventLog,
  IN UINTN  EventLogSize
  )
{
  EVENT_LOG_ITERATOR  Iter;
  VOID                *NextEvent;
  UINTN               NextEventSize;

  EventLogIteratorInit (&Iter, EventLog, EventLogSize);

  while (!EFI_ERROR (EventLogIteratorGetNext (&Iter, &NextEvent, &NextEventSize))) {
    if (IsSpecIdEvent (NextEvent)) {
      DumpTcgEfiSpecIdEvent (NextEvent);
    } else {
      DumpEvent (NextEvent);
    }
  }
}

/**
  Build one Event Log Hob.

  @param[in]   Event                  Event
  @param[in]   EventSize              Size of Event

  @return EFI_SUCCESS
  @return EFI_UNSUPPORTED

 **/
STATIC
EFI_STATUS
EFIAPI
BuildOnePeiEventLogHob (
  IN VOID   *Event,
  IN UINTN  EventSize
  )
{
  EFI_STATUS                   Status;
  TCG_PCR_EVENT2               *TcgPcrEvent2;
  UINTN                        Idx;
  UINT32                       DigestCount;
  TPMI_ALG_HASH                HashAlgo;
  INTERNAL_HASH_INFO           *HashInfo;
  VOID                         *GuidHob;
  UINT32                       EventDataSize;
  TCG_EfiStartupLocalityEvent  *StartupLocalityEvent;

  TcgPcrEvent2 = ((TCG_PCR_EVENT2 *)Event);
  DigestCount  = TcgPcrEvent2->Digest.count;
  HashAlgo     = TcgPcrEvent2->Digest.digests[0].hashAlg;
  Event        = (VOID *)&TcgPcrEvent2->Digest.digests[0].digest;

  for (Idx = 0; Idx < DigestCount; Idx++) {
    Status = GetHashInfo (HashAlgo, &HashInfo);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a: Can't get HashInfo for %d\n", __func__, HashAlgo));
      return Status;
    }

    Event += HashInfo->HashSize;
    CopyMem (&HashAlgo, Event, sizeof (TPMI_ALG_HASH));
    Event += sizeof (TPMI_ALG_HASH);
  }

  Event -= sizeof (TPMI_ALG_HASH);
  CopyMem (&EventDataSize, Event, sizeof (TcgPcrEvent2->EventSize));
  Event               += sizeof (TcgPcrEvent2->EventSize);
  StartupLocalityEvent = (TCG_EfiStartupLocalityEvent *)Event;

  if ((EventDataSize == sizeof (TCG_EfiStartupLocalityEvent)) &&
      (CompareMem (
         &StartupLocalityEvent->Signature,
         TCG_EfiStartupLocalityEvent_SIGNATURE,
         sizeof (StartupLocalityEvent->Signature)
         ) == 0))
  {
    /*
     *   StartupLocalityEvent is "EV_NO_ACTION" so it doesn't extend PCR.
     *   However, some version of tpm2_eventlog application extends
     *   StartupLocalityEvent, So it would be mismatched when it tries to
     *   replay with tpm2_eventlog and /sys/class/tpm/{tpm}/pcr{hash}/{NUM}.
     *
     *   StartupLocalityEvent is optional event not mandatory.
     *   To sustain integration with some version of tpm2_eventlog too,
     *   Skip the StartupLocality Event.
     */
    DEBUG ((DEBUG_INFO, "%a: Skip StartupLocality Event\n", __func__));
  } else {
    GuidHob = BuildGuidDataHob (&gTcgEvent2EntryHobGuid, TcgPcrEvent2, EventSize);
    if (GuidHob == NULL) {
      DEBUG ((DEBUG_ERROR, "%a: Failed to create Event Hob...\n", __func__));
      return EFI_OUT_OF_RESOURCES;
    }
  }

  return EFI_SUCCESS;
}

/**
  Build event log hob consumed by Tcg2Dxe.

  @param[in]  TransferList  Transfer list header

  @return EFI_SUCCESS               Success to generate HOBs from passed event log
  @return EFI_INVALID_PARAMETER     Invalid event log
  @return Others                    Failed to create event log hob

**/
EFI_STATUS
EFIAPI
BuildPeiEventLogHobs (
  IN TRANSFER_LIST_HEADER  *TransferList
  )
{
  EFI_STATUS          Status;
  VOID                *EventLog;
  UINTN               EventLogSize;
  EVENT_LOG_ITERATOR  Iter;
  VOID                *NextEvent;
  UINTN               NextEventSize;

  Status = TransferListGetEventLog (TransferList, &EventLog, &EventLogSize, NULL);
  if (EFI_ERROR (Status)) {
    /*
     * If there is no event log, skip creating EventLogHobs from TransferList.
     */
    return EFI_SUCCESS;
  }

  DumpTpmEventLog (EventLog, EventLogSize);

  EventLogIteratorInit (&Iter, EventLog, EventLogSize);

  Status = EventLogIteratorGetNext (&Iter, &NextEvent, &NextEventSize);
  if (EFI_ERROR (Status)) {
    /*
     * If event log is invalid, skip creating EventLogHobs from TransferList.
     */
    DEBUG ((DEBUG_WARN, "%a: Invalid TPM event log...", __func__));
    return EFI_SUCCESS;
  }

  if (!IsSpecIdEvent (NextEvent)) {
    /*
     * The first event must be a SpecId event otherwise
     * the event log is invalid.
     */
    DEBUG ((DEBUG_WARN, "%a: Invalid TPM event log...", __func__));
    return EFI_INVALID_PARAMETER;
  }

  /*
   * Tcg2Dxe generates a SpecId Event when it generates the final event log.
   * Therefore, skip creating a HOB for the TF-A SpecIdEvent log.
   */
  while (!EFI_ERROR (EventLogIteratorGetNext (&Iter, &NextEvent, &NextEventSize))) {
    Status = BuildOnePeiEventLogHob (NextEvent, NextEventSize);
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  return EFI_SUCCESS;
}
