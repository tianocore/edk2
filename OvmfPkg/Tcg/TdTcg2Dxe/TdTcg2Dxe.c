/** @file
  This module implements EFI TD Protocol.

  Copyright (c) 2020 - 2021, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiDxe.h>
#include <IndustryStandard/Acpi.h>
#include <IndustryStandard/PeImage.h>
#include <IndustryStandard/TcpaAcpi.h>

#include <Guid/GlobalVariable.h>
#include <Guid/HobList.h>
#include <Guid/EventGroup.h>
#include <Guid/EventExitBootServiceFailed.h>
#include <Guid/ImageAuthentication.h>
#include <Guid/TpmInstance.h>

#include <Protocol/DevicePath.h>
#include <Protocol/MpService.h>
#include <Protocol/VariableWrite.h>
#include <Protocol/Tcg2Protocol.h>
#include <Protocol/TrEEProtocol.h>
#include <Protocol/AcpiTable.h>

#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/HobLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/BaseLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PrintLib.h>
#include <Library/PcdLib.h>
#include <Library/UefiLib.h>
#include <Library/HashLib.h>
#include <Library/PerformanceLib.h>
#include <Library/ReportStatusCodeLib.h>
#include <Library/TpmMeasurementLib.h>

#include <Protocol/CcMeasurement.h>
#include <Guid/CcEventHob.h>
#include <Library/TdxLib.h>

#define PERF_ID_CC_TCG2_DXE  0x3130

#define   CC_EVENT_LOG_AREA_COUNT_MAX  1
#define   CC_MR_INDEX_0_MRTD           0
#define   CC_MR_INDEX_1_RTMR0          1
#define   CC_MR_INDEX_2_RTMR1          2
#define   CC_MR_INDEX_3_RTMR2          3
#define   CC_MR_INDEX_INVALID          4

typedef struct {
  CHAR16      *VariableName;
  EFI_GUID    *VendorGuid;
} VARIABLE_TYPE;

typedef struct {
  EFI_GUID                   *EventGuid;
  EFI_CC_EVENT_LOG_FORMAT    LogFormat;
} CC_EVENT_INFO_STRUCT;

typedef struct {
  EFI_CC_EVENT_LOG_FORMAT    EventLogFormat;
  EFI_PHYSICAL_ADDRESS       Lasa;
  UINT64                     Laml;
  UINTN                      EventLogSize;
  UINT8                      *LastEvent;
  BOOLEAN                    EventLogStarted;
  BOOLEAN                    EventLogTruncated;
  UINTN                      Next800155EventOffset;
} CC_EVENT_LOG_AREA_STRUCT;

typedef struct _TDX_DXE_DATA {
  EFI_CC_BOOT_SERVICE_CAPABILITY    BsCap;
  CC_EVENT_LOG_AREA_STRUCT          EventLogAreaStruct[CC_EVENT_LOG_AREA_COUNT_MAX];
  BOOLEAN                           GetEventLogCalled[CC_EVENT_LOG_AREA_COUNT_MAX];
  CC_EVENT_LOG_AREA_STRUCT          FinalEventLogAreaStruct[CC_EVENT_LOG_AREA_COUNT_MAX];
  EFI_CC_FINAL_EVENTS_TABLE         *FinalEventsTable[CC_EVENT_LOG_AREA_COUNT_MAX];
} TDX_DXE_DATA;

typedef struct {
  TPMI_ALG_HASH    HashAlgo;
  UINT16           HashSize;
  UINT32           HashMask;
} TDX_HASH_INFO;

//
//
CC_EVENT_INFO_STRUCT  mCcEventInfo[] = {
  { &gCcEventEntryHobGuid, EFI_CC_EVENT_LOG_FORMAT_TCG_2 },
};

TDX_DXE_DATA  mTdxDxeData = {
  {
    sizeof (EFI_CC_BOOT_SERVICE_CAPABILITY), // Size
    { 1, 1 },                                // StructureVersion
    { 1, 1 },                                // ProtocolVersion
    EFI_CC_BOOT_HASH_ALG_SHA384,             // HashAlgorithmBitmap
    EFI_CC_EVENT_LOG_FORMAT_TCG_2,           // SupportedEventLogs
    { 2, 0 }                                 // {CC_TYPE, CC_SUBTYPE}
  },
};

UINTN   mBootAttempts  = 0;
CHAR16  mBootVarName[] = L"BootOrder";

VARIABLE_TYPE  mVariableType[] = {
  { EFI_SECURE_BOOT_MODE_NAME,    &gEfiGlobalVariableGuid        },
  { EFI_PLATFORM_KEY_NAME,        &gEfiGlobalVariableGuid        },
  { EFI_KEY_EXCHANGE_KEY_NAME,    &gEfiGlobalVariableGuid        },
  { EFI_IMAGE_SECURITY_DATABASE,  &gEfiImageSecurityDatabaseGuid },
  { EFI_IMAGE_SECURITY_DATABASE1, &gEfiImageSecurityDatabaseGuid },
};

EFI_CC_EVENTLOG_ACPI_TABLE  mTdxEventlogAcpiTemplate = {
  {
    EFI_CC_EVENTLOG_ACPI_TABLE_SIGNATURE,
    sizeof (mTdxEventlogAcpiTemplate),
    EFI_CC_EVENTLOG_ACPI_TABLE_REVISION,
    //
    // Compiler initializes the remaining bytes to 0
    // These fields should be filled in production
    //
  },
  { EFI_CC_TYPE_TDX, 0 }, // CcType
  0,                      // rsvd
  0,                      // laml
  0,                      // lasa
};

//
// Supported Hash list in Td guest.
// Currently SHA384 is supported.
//
TDX_HASH_INFO  mHashInfo[] = {
  { TPM_ALG_SHA384, SHA384_DIGEST_SIZE, HASH_ALG_SHA384 }
};

/**
  Get hash size based on Algo

  @param[in]     HashAlgo           Hash Algorithm Id.

  @return Size of the hash.
**/
UINT16
GetHashSizeFromAlgo (
  IN TPMI_ALG_HASH  HashAlgo
  )
{
  UINTN  Index;

  for (Index = 0; Index < sizeof (mHashInfo)/sizeof (mHashInfo[0]); Index++) {
    if (mHashInfo[Index].HashAlgo == HashAlgo) {
      return mHashInfo[Index].HashSize;
    }
  }

  return 0;
}

/**
  Get hash mask based on Algo

  @param[in]     HashAlgo           Hash Algorithm Id.

  @return Hash mask.
**/
UINT32
GetHashMaskFromAlgo (
  IN TPMI_ALG_HASH  HashAlgo
  )
{
  UINTN  Index;

  for (Index = 0; Index < ARRAY_SIZE (mHashInfo); Index++) {
    if (mHashInfo[Index].HashAlgo == HashAlgo) {
      return mHashInfo[Index].HashMask;
    }
  }

  ASSERT (FALSE);
  return 0;
}

/**
  Copy TPML_DIGEST_VALUES into a buffer

  @param[in,out] Buffer             Buffer to hold copied TPML_DIGEST_VALUES compact binary.
  @param[in]     DigestList         TPML_DIGEST_VALUES to be copied.
  @param[in]     HashAlgorithmMask  HASH bits corresponding to the desired digests to copy.

  @return The end of buffer to hold TPML_DIGEST_VALUES.
**/
VOID *
CopyDigestListToBuffer (
  IN OUT VOID            *Buffer,
  IN TPML_DIGEST_VALUES  *DigestList,
  IN UINT32              HashAlgorithmMask
  )
{
  UINTN   Index;
  UINT16  DigestSize;
  UINT32  DigestListCount;
  UINT32  *DigestListCountPtr;

  DigestListCountPtr = (UINT32 *)Buffer;
  DigestListCount    = 0;
  Buffer             = (UINT8 *)Buffer + sizeof (DigestList->count);
  for (Index = 0; Index < DigestList->count; Index++) {
    if ((DigestList->digests[Index].hashAlg & HashAlgorithmMask) == 0) {
      DEBUG ((DEBUG_ERROR, "WARNING: TD Event log has HashAlg unsupported (0x%x)\n", DigestList->digests[Index].hashAlg));
      continue;
    }

    CopyMem (Buffer, &DigestList->digests[Index].hashAlg, sizeof (DigestList->digests[Index].hashAlg));
    Buffer     = (UINT8 *)Buffer + sizeof (DigestList->digests[Index].hashAlg);
    DigestSize = GetHashSizeFromAlgo (DigestList->digests[Index].hashAlg);
    CopyMem (Buffer, &DigestList->digests[Index].digest, DigestSize);
    Buffer = (UINT8 *)Buffer + DigestSize;
    DigestListCount++;
  }

  WriteUnaligned32 (DigestListCountPtr, DigestListCount);

  return Buffer;
}

EFI_HANDLE  mImageHandle;

/**
  Measure PE image into TPM log based on the authenticode image hashing in
  PE/COFF Specification 8.0 Appendix A.

  Caution: This function may receive untrusted input.
  PE/COFF image is external input, so this function will validate its data structure
  within this image buffer before use.

  Notes: PE/COFF image is checked by BasePeCoffLib PeCoffLoaderGetImageInfo().

  @param[in]  RtmrIndex        RTMR index
  @param[in]  ImageAddress   Start address of image buffer.
  @param[in]  ImageSize      Image size
  @param[out] DigestList     Digest list of this image.

  @retval EFI_SUCCESS            Successfully measure image.
  @retval EFI_OUT_OF_RESOURCES   No enough resource to measure image.
  @retval other error value
**/
EFI_STATUS
MeasurePeImageAndExtend (
  IN  UINT32                RtmrIndex,
  IN  EFI_PHYSICAL_ADDRESS  ImageAddress,
  IN  UINTN                 ImageSize,
  OUT TPML_DIGEST_VALUES    *DigestList
  );

#define COLUME_SIZE  (16 * 2)

/**

  This function dump raw data.

  @param  Data  raw data
  @param  Size  raw data size

**/
VOID
InternalDumpData (
  IN UINT8  *Data,
  IN UINTN  Size
  )
{
  UINTN  Index;

  for (Index = 0; Index < Size; Index++) {
    DEBUG ((DEBUG_INFO, Index == COLUME_SIZE/2 ? " | %02x" : " %02x", (UINTN)Data[Index]));
  }
}

/**

  This function dump raw data with colume format.

  @param  Data  raw data
  @param  Size  raw data size

**/
VOID
InternalDumpHex (
  IN UINT8  *Data,
  IN UINTN  Size
  )
{
  UINTN  Index;
  UINTN  Count;
  UINTN  Left;

  Count = Size / COLUME_SIZE;
  Left  = Size % COLUME_SIZE;
  for (Index = 0; Index < Count; Index++) {
    DEBUG ((DEBUG_INFO, "%04x: ", Index * COLUME_SIZE));
    InternalDumpData (Data + Index * COLUME_SIZE, COLUME_SIZE);
    DEBUG ((DEBUG_INFO, "\n"));
  }

  if (Left != 0) {
    DEBUG ((DEBUG_INFO, "%04x: ", Index * COLUME_SIZE));
    InternalDumpData (Data + Index * COLUME_SIZE, Left);
    DEBUG ((DEBUG_INFO, "\n"));
  }
}

/**

  This function initialize TD_EVENT_HDR for EV_NO_ACTION
  Event Type other than EFI Specification ID event. The behavior is defined
  by TCG PC Client PFP Spec. Section 9.3.4 EV_NO_ACTION Event Types

  @param[in, out]   NoActionEvent  Event Header of EV_NO_ACTION Event
  @param[in]        EventSize      Event Size of the EV_NO_ACTION Event

**/
VOID
InitNoActionEvent (
  IN OUT CC_EVENT_HDR  *NoActionEvent,
  IN UINT32            EventSize
  )
{
  UINT32         DigestListCount;
  TPMI_ALG_HASH  HashAlgId;
  UINT8          *DigestBuffer;

  DigestBuffer    = (UINT8 *)NoActionEvent->Digests.digests;
  DigestListCount = 0;

  NoActionEvent->MrIndex   = 0;
  NoActionEvent->EventType = EV_NO_ACTION;

  //
  // Set Hash count & hashAlg accordingly, while Digest.digests[n].digest to all 0
  //
  ZeroMem (&NoActionEvent->Digests, sizeof (NoActionEvent->Digests));

  if ((mTdxDxeData.BsCap.HashAlgorithmBitmap & EFI_CC_BOOT_HASH_ALG_SHA384) != 0) {
    HashAlgId = TPM_ALG_SHA384;
    CopyMem (DigestBuffer, &HashAlgId, sizeof (TPMI_ALG_HASH));
    DigestBuffer += sizeof (TPMI_ALG_HASH) + GetHashSizeFromAlgo (HashAlgId);
    DigestListCount++;
  }

  //
  // Set Digests Count
  //
  WriteUnaligned32 ((UINT32 *)&NoActionEvent->Digests.count, DigestListCount);

  //
  // Set Event Size
  //
  WriteUnaligned32 ((UINT32 *)DigestBuffer, EventSize);
}

/**
  Get All processors EFI_CPU_LOCATION in system. LocationBuf is allocated inside the function
  Caller is responsible to free LocationBuf.

  @param[out] LocationBuf          Returns Processor Location Buffer.
  @param[out] Num                  Returns processor number.

  @retval EFI_SUCCESS              Operation completed successfully.
  @retval EFI_UNSUPPORTED       MpService protocol not found.

**/
EFI_STATUS
GetProcessorsCpuLocation (
  OUT  EFI_CPU_PHYSICAL_LOCATION  **LocationBuf,
  OUT  UINTN                      *Num
  )
{
  EFI_STATUS                 Status;
  EFI_MP_SERVICES_PROTOCOL   *MpProtocol;
  UINTN                      ProcessorNum;
  UINTN                      EnabledProcessorNum;
  EFI_PROCESSOR_INFORMATION  ProcessorInfo;
  EFI_CPU_PHYSICAL_LOCATION  *ProcessorLocBuf;
  UINTN                      Index;

  Status = gBS->LocateProtocol (&gEfiMpServiceProtocolGuid, NULL, (VOID **)&MpProtocol);
  if (EFI_ERROR (Status)) {
    //
    // MP protocol is not installed
    //
    return EFI_UNSUPPORTED;
  }

  Status = MpProtocol->GetNumberOfProcessors (
                         MpProtocol,
                         &ProcessorNum,
                         &EnabledProcessorNum
                         );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = gBS->AllocatePool (
                  EfiBootServicesData,
                  sizeof (EFI_CPU_PHYSICAL_LOCATION) * ProcessorNum,
                  (VOID **)&ProcessorLocBuf
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Get each processor Location info
  //
  for (Index = 0; Index < ProcessorNum; Index++) {
    Status = MpProtocol->GetProcessorInfo (
                           MpProtocol,
                           Index,
                           &ProcessorInfo
                           );
    if (EFI_ERROR (Status)) {
      FreePool (ProcessorLocBuf);
      return Status;
    }

    //
    // Get all Processor Location info & measure
    //
    CopyMem (
      &ProcessorLocBuf[Index],
      &ProcessorInfo.Location,
      sizeof (EFI_CPU_PHYSICAL_LOCATION)
      );
  }

  *LocationBuf = ProcessorLocBuf;
  *Num         = ProcessorNum;

  return Status;
}

/**
  The EFI_CC_MEASUREMENT_PROTOCOL GetCapability function call provides protocol
  capability information and state information.

  @param[in]      This               Indicates the calling context
  @param[in, out] ProtocolCapability The caller allocates memory for a EFI_CC_BOOT_SERVICE_CAPABILITY
                                     structure and sets the size field to the size of the structure allocated.
                                     The callee fills in the fields with the EFI protocol capability information
                                     and the current EFI TCG2 state information up to the number of fields which
                                     fit within the size of the structure passed in.

  @retval EFI_SUCCESS            Operation completed successfully.
  @retval EFI_DEVICE_ERROR       The command was unsuccessful.
                                 The ProtocolCapability variable will not be populated.
  @retval EFI_INVALID_PARAMETER  One or more of the parameters are incorrect.
                                 The ProtocolCapability variable will not be populated.
  @retval EFI_BUFFER_TOO_SMALL   The ProtocolCapability variable is too small to hold the full response.
                                 It will be partially populated (required Size field will be set).
**/
EFI_STATUS
EFIAPI
TdGetCapability (
  IN EFI_CC_MEASUREMENT_PROTOCOL         *This,
  IN OUT EFI_CC_BOOT_SERVICE_CAPABILITY  *ProtocolCapability
  )
{
  DEBUG ((DEBUG_VERBOSE, "TdGetCapability\n"));

  if ((This == NULL) || (ProtocolCapability == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  CopyMem (ProtocolCapability, &mTdxDxeData.BsCap, sizeof (EFI_CC_BOOT_SERVICE_CAPABILITY));

  return EFI_SUCCESS;
}

/**
  This function dump PCR event.
  TD Event log reuse the TCG PCR Event spec.
  The first event in the event log is the SHA1 log format.
  There is only ONE TCG_PCR_EVENT in TD Event log.

  @param[in]  EventHdr     TCG PCR event structure.
**/
VOID
DumpPcrEvent (
  IN TCG_PCR_EVENT_HDR  *EventHdr
  )
{
  UINTN  Index;

  DEBUG ((DEBUG_INFO, "  Event:\n"));
  DEBUG ((DEBUG_INFO, "    MrIndex  - %d\n", EventHdr->PCRIndex));
  DEBUG ((DEBUG_INFO, "    EventType - 0x%08x\n", EventHdr->EventType));
  DEBUG ((DEBUG_INFO, "    Digest    - "));
  for (Index = 0; Index < sizeof (TCG_DIGEST); Index++) {
    DEBUG ((DEBUG_INFO, "%02x ", EventHdr->Digest.digest[Index]));
  }

  DEBUG ((DEBUG_INFO, "\n"));
  DEBUG ((DEBUG_INFO, "    EventSize - 0x%08x\n", EventHdr->EventSize));
  InternalDumpHex ((UINT8 *)(EventHdr + 1), EventHdr->EventSize);
}

/**
  This function dump TCG_EfiSpecIDEventStruct.

  @param[in]  TcgEfiSpecIdEventStruct     A pointer to TCG_EfiSpecIDEventStruct.
**/
VOID
DumpTcgEfiSpecIdEventStruct (
  IN TCG_EfiSpecIDEventStruct  *TcgEfiSpecIdEventStruct
  )
{
  TCG_EfiSpecIdEventAlgorithmSize  *DigestSize;
  UINTN                            Index;
  UINT8                            *VendorInfoSize;
  UINT8                            *VendorInfo;
  UINT32                           NumberOfAlgorithms;

  DEBUG ((DEBUG_INFO, "  TCG_EfiSpecIDEventStruct:\n"));
  DEBUG ((DEBUG_INFO, "    signature          - '"));
  for (Index = 0; Index < sizeof (TcgEfiSpecIdEventStruct->signature); Index++) {
    DEBUG ((DEBUG_INFO, "%c", TcgEfiSpecIdEventStruct->signature[Index]));
  }

  DEBUG ((DEBUG_INFO, "'\n"));
  DEBUG ((DEBUG_INFO, "    platformClass      - 0x%08x\n", TcgEfiSpecIdEventStruct->platformClass));
  DEBUG ((DEBUG_INFO, "    specVersion        - %d.%d%d\n", TcgEfiSpecIdEventStruct->specVersionMajor, TcgEfiSpecIdEventStruct->specVersionMinor, TcgEfiSpecIdEventStruct->specErrata));
  DEBUG ((DEBUG_INFO, "    uintnSize          - 0x%02x\n", TcgEfiSpecIdEventStruct->uintnSize));

  CopyMem (&NumberOfAlgorithms, TcgEfiSpecIdEventStruct + 1, sizeof (NumberOfAlgorithms));
  DEBUG ((DEBUG_INFO, "    NumberOfAlgorithms - 0x%08x\n", NumberOfAlgorithms));

  DigestSize = (TCG_EfiSpecIdEventAlgorithmSize *)((UINT8 *)TcgEfiSpecIdEventStruct + sizeof (*TcgEfiSpecIdEventStruct) + sizeof (NumberOfAlgorithms));
  for (Index = 0; Index < NumberOfAlgorithms; Index++) {
    DEBUG ((DEBUG_INFO, "    digest(%d)\n", Index));
    DEBUG ((DEBUG_INFO, "      algorithmId      - 0x%04x\n", DigestSize[Index].algorithmId));
    DEBUG ((DEBUG_INFO, "      digestSize       - 0x%04x\n", DigestSize[Index].digestSize));
  }

  VendorInfoSize = (UINT8 *)&DigestSize[NumberOfAlgorithms];
  DEBUG ((DEBUG_INFO, "    VendorInfoSize     - 0x%02x\n", *VendorInfoSize));
  VendorInfo = VendorInfoSize + 1;
  DEBUG ((DEBUG_INFO, "    VendorInfo         - "));
  for (Index = 0; Index < *VendorInfoSize; Index++) {
    DEBUG ((DEBUG_INFO, "%02x ", VendorInfo[Index]));
  }

  DEBUG ((DEBUG_INFO, "\n"));
}

/**
  This function get size of TCG_EfiSpecIDEventStruct.

  @param[in]  TcgEfiSpecIdEventStruct     A pointer to TCG_EfiSpecIDEventStruct.
**/
UINTN
GetTcgEfiSpecIdEventStructSize (
  IN TCG_EfiSpecIDEventStruct  *TcgEfiSpecIdEventStruct
  )
{
  TCG_EfiSpecIdEventAlgorithmSize  *DigestSize;
  UINT8                            *VendorInfoSize;
  UINT32                           NumberOfAlgorithms;

  CopyMem (&NumberOfAlgorithms, TcgEfiSpecIdEventStruct + 1, sizeof (NumberOfAlgorithms));

  DigestSize     = (TCG_EfiSpecIdEventAlgorithmSize *)((UINT8 *)TcgEfiSpecIdEventStruct + sizeof (*TcgEfiSpecIdEventStruct) + sizeof (NumberOfAlgorithms));
  VendorInfoSize = (UINT8 *)&DigestSize[NumberOfAlgorithms];
  return sizeof (TCG_EfiSpecIDEventStruct) + sizeof (UINT32) + (NumberOfAlgorithms * sizeof (TCG_EfiSpecIdEventAlgorithmSize)) + sizeof (UINT8) + (*VendorInfoSize);
}

/**
  This function dump TD Event (including the Digests).

  @param[in]  CcEvent     TD Event structure.
**/
VOID
DumpCcEvent (
  IN CC_EVENT  *CcEvent
  )
{
  UINT32         DigestIndex;
  UINT32         DigestCount;
  TPMI_ALG_HASH  HashAlgo;
  UINT32         DigestSize;
  UINT8          *DigestBuffer;
  UINT32         EventSize;
  UINT8          *EventBuffer;

  DEBUG ((DEBUG_INFO, "Cc Event:\n"));
  DEBUG ((DEBUG_INFO, "    MrIndex  - %d\n", CcEvent->MrIndex));
  DEBUG ((DEBUG_INFO, "    EventType - 0x%08x\n", CcEvent->EventType));
  DEBUG ((DEBUG_INFO, "    DigestCount: 0x%08x\n", CcEvent->Digests.count));

  DigestCount  = CcEvent->Digests.count;
  HashAlgo     = CcEvent->Digests.digests[0].hashAlg;
  DigestBuffer = (UINT8 *)&CcEvent->Digests.digests[0].digest;
  for (DigestIndex = 0; DigestIndex < DigestCount; DigestIndex++) {
    DEBUG ((DEBUG_INFO, "      HashAlgo : 0x%04x\n", HashAlgo));
    DEBUG ((DEBUG_INFO, "      Digest(%d): \n", DigestIndex));
    DigestSize = GetHashSizeFromAlgo (HashAlgo);
    InternalDumpHex (DigestBuffer, DigestSize);
    //
    // Prepare next
    //
    CopyMem (&HashAlgo, DigestBuffer + DigestSize, sizeof (TPMI_ALG_HASH));
    DigestBuffer = DigestBuffer + DigestSize + sizeof (TPMI_ALG_HASH);
  }

  DigestBuffer = DigestBuffer - sizeof (TPMI_ALG_HASH);

  CopyMem (&EventSize, DigestBuffer, sizeof (CcEvent->EventSize));
  DEBUG ((DEBUG_INFO, "    EventSize - 0x%08x\n", EventSize));
  EventBuffer = DigestBuffer + sizeof (CcEvent->EventSize);
  InternalDumpHex (EventBuffer, EventSize);
  DEBUG ((DEBUG_INFO, "\n"));
}

/**
  This function returns size of Td Table event.

  @param[in]  CcEvent     Td Table event structure.

  @return size of Td event.
**/
UINTN
GetCcEventSize (
  IN CC_EVENT  *CcEvent
  )
{
  UINT32         DigestIndex;
  UINT32         DigestCount;
  TPMI_ALG_HASH  HashAlgo;
  UINT32         DigestSize;
  UINT8          *DigestBuffer;
  UINT32         EventSize;
  UINT8          *EventBuffer;

  DigestCount  = CcEvent->Digests.count;
  HashAlgo     = CcEvent->Digests.digests[0].hashAlg;
  DigestBuffer = (UINT8 *)&CcEvent->Digests.digests[0].digest;
  for (DigestIndex = 0; DigestIndex < DigestCount; DigestIndex++) {
    DigestSize = GetHashSizeFromAlgo (HashAlgo);
    //
    // Prepare next
    //
    CopyMem (&HashAlgo, DigestBuffer + DigestSize, sizeof (TPMI_ALG_HASH));
    DigestBuffer = DigestBuffer + DigestSize + sizeof (TPMI_ALG_HASH);
  }

  DigestBuffer = DigestBuffer - sizeof (TPMI_ALG_HASH);

  CopyMem (&EventSize, DigestBuffer, sizeof (CcEvent->EventSize));
  EventBuffer = DigestBuffer + sizeof (CcEvent->EventSize);

  return (UINTN)EventBuffer + EventSize - (UINTN)CcEvent;
}

/**
  This function dump CC event log.
  TDVF only supports EFI_CC_EVENT_LOG_FORMAT_TCG_2

  @param[in]  EventLogFormat     The type of the event log for which the information is requested.
  @param[in]  EventLogLocation   A pointer to the memory address of the event log.
  @param[in]  EventLogLastEntry  If the Event Log contains more than one entry, this is a pointer to the
                                 address of the start of the last entry in the event log in memory.
  @param[in]  FinalEventsTable   A pointer to the memory address of the final event table.
**/
VOID
DumpCcEventLog (
  IN EFI_CC_EVENT_LOG_FORMAT    EventLogFormat,
  IN EFI_PHYSICAL_ADDRESS       EventLogLocation,
  IN EFI_PHYSICAL_ADDRESS       EventLogLastEntry,
  IN EFI_CC_FINAL_EVENTS_TABLE  *FinalEventsTable
  )
{
  TCG_PCR_EVENT_HDR         *EventHdr;
  CC_EVENT                  *CcEvent;
  TCG_EfiSpecIDEventStruct  *TcgEfiSpecIdEventStruct;
  UINTN                     NumberOfEvents;

  DEBUG ((DEBUG_INFO, "EventLogFormat: (0x%x)\n", EventLogFormat));
  ASSERT (EventLogFormat == EFI_CC_EVENT_LOG_FORMAT_TCG_2);

  //
  // Dump first event.
  // The first event is always the TCG_PCR_EVENT_HDR
  // After this event is a TCG_EfiSpecIDEventStruct
  //
  EventHdr = (TCG_PCR_EVENT_HDR *)(UINTN)EventLogLocation;
  DumpPcrEvent (EventHdr);

  TcgEfiSpecIdEventStruct = (TCG_EfiSpecIDEventStruct *)(EventHdr + 1);
  DumpTcgEfiSpecIdEventStruct (TcgEfiSpecIdEventStruct);

  //
  // Then the CcEvent (Its structure is similar to TCG_PCR_EVENT2)
  //
  CcEvent = (CC_EVENT *)((UINTN)TcgEfiSpecIdEventStruct + GetTcgEfiSpecIdEventStructSize (TcgEfiSpecIdEventStruct));
  while ((UINTN)CcEvent <= EventLogLastEntry) {
    DumpCcEvent (CcEvent);
    CcEvent = (CC_EVENT *)((UINTN)CcEvent + GetCcEventSize (CcEvent));
  }

  if (FinalEventsTable == NULL) {
    DEBUG ((DEBUG_INFO, "FinalEventsTable: NOT FOUND\n"));
  } else {
    DEBUG ((DEBUG_INFO, "FinalEventsTable:    (0x%x)\n", FinalEventsTable));
    DEBUG ((DEBUG_INFO, "  Version:           (0x%x)\n", FinalEventsTable->Version));
    DEBUG ((DEBUG_INFO, "  NumberOfEvents:    (0x%x)\n", FinalEventsTable->NumberOfEvents));

    CcEvent = (CC_EVENT *)(UINTN)(FinalEventsTable + 1);
    for (NumberOfEvents = 0; NumberOfEvents < FinalEventsTable->NumberOfEvents; NumberOfEvents++) {
      DumpCcEvent (CcEvent);
      CcEvent = (CC_EVENT *)((UINTN)CcEvent + GetCcEventSize (CcEvent));
    }
  }

  return;
}

/**
  The EFI_CC_MEASUREMENT_PROTOCOL Get Event Log function call allows a caller to
  retrieve the address of a given event log and its last entry.

  @param[in]  This               Indicates the calling context
  @param[in]  EventLogFormat     The type of the event log for which the information is requested.
  @param[out] EventLogLocation   A pointer to the memory address of the event log.
  @param[out] EventLogLastEntry  If the Event Log contains more than one entry, this is a pointer to the
                                 address of the start of the last entry in the event log in memory.
  @param[out] EventLogTruncated  If the Event Log is missing at least one entry because an event would
                                 have exceeded the area allocated for events, this value is set to TRUE.
                                 Otherwise, the value will be FALSE and the Event Log will be complete.

  @retval EFI_SUCCESS            Operation completed successfully.
  @retval EFI_INVALID_PARAMETER  One or more of the parameters are incorrect
                                 (e.g. asking for an event log whose format is not supported).
**/
EFI_STATUS
EFIAPI
TdGetEventLog (
  IN EFI_CC_MEASUREMENT_PROTOCOL  *This,
  IN EFI_CC_EVENT_LOG_FORMAT      EventLogFormat,
  OUT EFI_PHYSICAL_ADDRESS        *EventLogLocation,
  OUT EFI_PHYSICAL_ADDRESS        *EventLogLastEntry,
  OUT BOOLEAN                     *EventLogTruncated
  )
{
  UINTN  Index = 0;

  DEBUG ((DEBUG_INFO, "TdGetEventLog ... (0x%x)\n", EventLogFormat));
  ASSERT (EventLogFormat == EFI_CC_EVENT_LOG_FORMAT_TCG_2);

  if (EventLogLocation != NULL) {
    *EventLogLocation = mTdxDxeData.EventLogAreaStruct[Index].Lasa;
    DEBUG ((DEBUG_INFO, "TdGetEventLog (EventLogLocation - %x)\n", *EventLogLocation));
  }

  if (EventLogLastEntry != NULL) {
    if (!mTdxDxeData.EventLogAreaStruct[Index].EventLogStarted) {
      *EventLogLastEntry = (EFI_PHYSICAL_ADDRESS)(UINTN)0;
    } else {
      *EventLogLastEntry = (EFI_PHYSICAL_ADDRESS)(UINTN)mTdxDxeData.EventLogAreaStruct[Index].LastEvent;
    }

    DEBUG ((DEBUG_INFO, "TdGetEventLog (EventLogLastEntry - %x)\n", *EventLogLastEntry));
  }

  if (EventLogTruncated != NULL) {
    *EventLogTruncated = mTdxDxeData.EventLogAreaStruct[Index].EventLogTruncated;
    DEBUG ((DEBUG_INFO, "TdGetEventLog (EventLogTruncated - %x)\n", *EventLogTruncated));
  }

  DEBUG ((DEBUG_INFO, "TdGetEventLog - %r\n", EFI_SUCCESS));

  // Dump Event Log for debug purpose
  if ((EventLogLocation != NULL) && (EventLogLastEntry != NULL)) {
    DumpCcEventLog (EventLogFormat, *EventLogLocation, *EventLogLastEntry, mTdxDxeData.FinalEventsTable[Index]);
  }

  //
  // All events generated after the invocation of EFI_TCG2_GET_EVENT_LOG SHALL be stored
  // in an instance of an EFI_CONFIGURATION_TABLE named by the VendorGuid of EFI_TCG2_FINAL_EVENTS_TABLE_GUID.
  //
  mTdxDxeData.GetEventLogCalled[Index] = TRUE;

  return EFI_SUCCESS;
}

/**
  Return if this is a Tcg800155PlatformIdEvent.

  @param[in]      NewEventHdr         Pointer to a TCG_PCR_EVENT_HDR/TCG_PCR_EVENT_EX data structure.
  @param[in]      NewEventHdrSize     New event header size.
  @param[in]      NewEventData        Pointer to the new event data.
  @param[in]      NewEventSize        New event data size.

  @retval TRUE   This is a Tcg800155PlatformIdEvent.
  @retval FALSE  This is NOT a Tcg800155PlatformIdEvent.

**/
BOOLEAN
Is800155Event (
  IN      VOID    *NewEventHdr,
  IN      UINT32  NewEventHdrSize,
  IN      UINT8   *NewEventData,
  IN      UINT32  NewEventSize
  )
{
  if ((((TCG_PCR_EVENT2_HDR *)NewEventHdr)->EventType == EV_NO_ACTION) &&
      (NewEventSize >= sizeof (TCG_Sp800_155_PlatformId_Event2)) &&
      ((CompareMem (
          NewEventData,
          TCG_Sp800_155_PlatformId_Event2_SIGNATURE,
          sizeof (TCG_Sp800_155_PlatformId_Event2_SIGNATURE) - 1
          ) == 0) ||
       (CompareMem (
          NewEventData,
          TCG_Sp800_155_PlatformId_Event3_SIGNATURE,
          sizeof (TCG_Sp800_155_PlatformId_Event3_SIGNATURE) - 1
          ) == 0)))
  {
    return TRUE;
  }

  return FALSE;
}

/**
  Add a new entry to the Event Log.

  @param[in, out] EventLogAreaStruct  The event log area data structure
  @param[in]      NewEventHdr         Pointer to a TCG_PCR_EVENT_HDR/TCG_PCR_EVENT_EX data structure.
  @param[in]      NewEventHdrSize     New event header size.
  @param[in]      NewEventData        Pointer to the new event data.
  @param[in]      NewEventSize        New event data size.

  @retval EFI_SUCCESS           The new event log entry was added.
  @retval EFI_OUT_OF_RESOURCES  No enough memory to log the new event.

**/
EFI_STATUS
TcgCommLogEvent (
  IN OUT  CC_EVENT_LOG_AREA_STRUCT  *EventLogAreaStruct,
  IN      VOID                      *NewEventHdr,
  IN      UINT32                    NewEventHdrSize,
  IN      UINT8                     *NewEventData,
  IN      UINT32                    NewEventSize
  )
{
  UINTN         NewLogSize;
  BOOLEAN       Record800155Event;
  CC_EVENT_HDR  *CcEventHdr;

  CcEventHdr = (CC_EVENT_HDR *)NewEventHdr;
  DEBUG ((DEBUG_VERBOSE, "Td: Try to log event. Index = %d, EventType = 0x%x\n", CcEventHdr->MrIndex, CcEventHdr->EventType));

  if (NewEventSize > MAX_ADDRESS -  NewEventHdrSize) {
    return EFI_OUT_OF_RESOURCES;
  }

  NewLogSize = NewEventHdrSize + NewEventSize;

  if (NewLogSize > MAX_ADDRESS -  EventLogAreaStruct->EventLogSize) {
    return EFI_OUT_OF_RESOURCES;
  }

  if (NewLogSize + EventLogAreaStruct->EventLogSize > EventLogAreaStruct->Laml) {
    DEBUG ((DEBUG_INFO, "  Laml       - 0x%x\n", EventLogAreaStruct->Laml));
    DEBUG ((DEBUG_INFO, "  NewLogSize - 0x%x\n", NewLogSize));
    DEBUG ((DEBUG_INFO, "  LogSize    - 0x%x\n", EventLogAreaStruct->EventLogSize));
    DEBUG ((DEBUG_INFO, "TcgCommLogEvent - %r\n", EFI_OUT_OF_RESOURCES));
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Check 800-155 event
  // Record to 800-155 event offset only.
  // If the offset is 0, no need to record.
  //
  Record800155Event = Is800155Event (NewEventHdr, NewEventHdrSize, NewEventData, NewEventSize);
  if (Record800155Event) {
    DEBUG ((DEBUG_INFO, "It is 800155Event.\n"));

    if (EventLogAreaStruct->Next800155EventOffset != 0) {
      CopyMem (
        (UINT8 *)(UINTN)EventLogAreaStruct->Lasa + EventLogAreaStruct->Next800155EventOffset + NewLogSize,
        (UINT8 *)(UINTN)EventLogAreaStruct->Lasa + EventLogAreaStruct->Next800155EventOffset,
        EventLogAreaStruct->EventLogSize - EventLogAreaStruct->Next800155EventOffset
        );

      CopyMem (
        (UINT8 *)(UINTN)EventLogAreaStruct->Lasa + EventLogAreaStruct->Next800155EventOffset,
        NewEventHdr,
        NewEventHdrSize
        );
      CopyMem (
        (UINT8 *)(UINTN)EventLogAreaStruct->Lasa + EventLogAreaStruct->Next800155EventOffset + NewEventHdrSize,
        NewEventData,
        NewEventSize
        );

      EventLogAreaStruct->Next800155EventOffset += NewLogSize;
      EventLogAreaStruct->LastEvent             += NewLogSize;
      EventLogAreaStruct->EventLogSize          += NewLogSize;
    }

    return EFI_SUCCESS;
  }

  EventLogAreaStruct->LastEvent     = (UINT8 *)(UINTN)EventLogAreaStruct->Lasa + EventLogAreaStruct->EventLogSize;
  EventLogAreaStruct->EventLogSize += NewLogSize;

  CopyMem (EventLogAreaStruct->LastEvent, NewEventHdr, NewEventHdrSize);
  CopyMem (
    EventLogAreaStruct->LastEvent + NewEventHdrSize,
    NewEventData,
    NewEventSize
    );

  return EFI_SUCCESS;
}

/**
  According to UEFI Spec 2.10 Section 38.4.1:
    The following table shows the TPM PCR index mapping and CC event log measurement
  register index interpretation for Intel TDX, where MRTD means Trust Domain Measurement
   Register and RTMR means Runtime Measurement Register

    // TPM PCR Index | CC Measurement Register Index | TDX-measurement register
    //  ------------------------------------------------------------------------
    // 0             |   0                           |   MRTD
    // 1, 7          |   1                           |   RTMR[0]
    // 2~6           |   2                           |   RTMR[1]
    // 8~15          |   3                           |   RTMR[2]

  @param[in] PCRIndex Index of the TPM PCR

  @retval    UINT32               Index of the CC Event Log Measurement Register Index
  @retval    CC_MR_INDEX_INVALID  Invalid MR Index
**/
UINT32
EFIAPI
MapPcrToMrIndex (
  IN  UINT32  PCRIndex
  )
{
  UINT32  MrIndex;

  if (PCRIndex > 15) {
    ASSERT (FALSE);
    return CC_MR_INDEX_INVALID;
  }

  MrIndex = 0;
  if (PCRIndex == 0) {
    MrIndex = CC_MR_INDEX_0_MRTD;
  } else if ((PCRIndex == 1) || (PCRIndex == 7)) {
    MrIndex = CC_MR_INDEX_1_RTMR0;
  } else if ((PCRIndex >= 2) && (PCRIndex <= 6)) {
    MrIndex = CC_MR_INDEX_2_RTMR1;
  } else if ((PCRIndex >= 8) && (PCRIndex <= 15)) {
    MrIndex = CC_MR_INDEX_3_RTMR2;
  }

  return MrIndex;
}

EFI_STATUS
EFIAPI
TdMapPcrToMrIndex (
  IN  EFI_CC_MEASUREMENT_PROTOCOL  *This,
  IN  UINT32                       PCRIndex,
  OUT UINT32                       *MrIndex
  )
{
  if (MrIndex == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  *MrIndex = MapPcrToMrIndex (PCRIndex);

  return *MrIndex == CC_MR_INDEX_INVALID ? EFI_INVALID_PARAMETER : EFI_SUCCESS;
}

/**
  Add a new entry to the Event Log.

  @param[in] EventLogFormat  The type of the event log for which the information is requested.
  @param[in] NewEventHdr     Pointer to a TCG_PCR_EVENT_HDR/TCG_PCR_EVENT_EX data structure.
  @param[in] NewEventHdrSize New event header size.
  @param[in] NewEventData    Pointer to the new event data.
  @param[in] NewEventSize    New event data size.

  @retval EFI_SUCCESS           The new event log entry was added.
  @retval EFI_OUT_OF_RESOURCES  No enough memory to log the new event.

**/
EFI_STATUS
TdxDxeLogEvent (
  IN      EFI_CC_EVENT_LOG_FORMAT  EventLogFormat,
  IN      VOID                     *NewEventHdr,
  IN      UINT32                   NewEventHdrSize,
  IN      UINT8                    *NewEventData,
  IN      UINT32                   NewEventSize
  )
{
  EFI_STATUS                Status;
  UINTN                     Index;
  CC_EVENT_LOG_AREA_STRUCT  *EventLogAreaStruct;

  if (EventLogFormat != EFI_CC_EVENT_LOG_FORMAT_TCG_2) {
    ASSERT (FALSE);
    return EFI_INVALID_PARAMETER;
  }

  Index = 0;

  //
  // Record to normal event log
  //
  EventLogAreaStruct = &mTdxDxeData.EventLogAreaStruct[Index];

  if (EventLogAreaStruct->EventLogTruncated) {
    return EFI_VOLUME_FULL;
  }

  Status = TcgCommLogEvent (
             EventLogAreaStruct,
             NewEventHdr,
             NewEventHdrSize,
             NewEventData,
             NewEventSize
             );

  if (Status == EFI_OUT_OF_RESOURCES) {
    EventLogAreaStruct->EventLogTruncated = TRUE;
    return EFI_VOLUME_FULL;
  } else if (Status == EFI_SUCCESS) {
    EventLogAreaStruct->EventLogStarted = TRUE;
  }

  //
  // If GetEventLog is called, record to FinalEventsTable, too.
  //
  if (mTdxDxeData.GetEventLogCalled[Index]) {
    if (mTdxDxeData.FinalEventsTable[Index] == NULL) {
      //
      // no need for FinalEventsTable
      //
      return EFI_SUCCESS;
    }

    EventLogAreaStruct = &mTdxDxeData.FinalEventLogAreaStruct[Index];

    if (EventLogAreaStruct->EventLogTruncated) {
      return EFI_VOLUME_FULL;
    }

    Status = TcgCommLogEvent (
               EventLogAreaStruct,
               NewEventHdr,
               NewEventHdrSize,
               NewEventData,
               NewEventSize
               );
    if (Status == EFI_OUT_OF_RESOURCES) {
      EventLogAreaStruct->EventLogTruncated = TRUE;
      return EFI_VOLUME_FULL;
    } else if (Status == EFI_SUCCESS) {
      EventLogAreaStruct->EventLogStarted = TRUE;
      //
      // Increase the NumberOfEvents in FinalEventsTable
      //
      (mTdxDxeData.FinalEventsTable[Index])->NumberOfEvents++;
      DEBUG ((DEBUG_INFO, "FinalEventsTable->NumberOfEvents - 0x%x\n", (mTdxDxeData.FinalEventsTable[Index])->NumberOfEvents));
      DEBUG ((DEBUG_INFO, "  Size - 0x%x\n", (UINTN)EventLogAreaStruct->EventLogSize));
    }
  }

  return Status;
}

/**
  Get TPML_DIGEST_VALUES compact binary buffer size.

  @param[in]     DigestListBin    TPML_DIGEST_VALUES compact binary buffer.

  @return TPML_DIGEST_VALUES compact binary buffer size.
**/
UINT32
GetDigestListBinSize (
  IN VOID  *DigestListBin
  )
{
  UINTN          Index;
  UINT16         DigestSize;
  UINT32         TotalSize;
  UINT32         Count;
  TPMI_ALG_HASH  HashAlg;

  Count         = ReadUnaligned32 (DigestListBin);
  TotalSize     = sizeof (Count);
  DigestListBin = (UINT8 *)DigestListBin + sizeof (Count);
  for (Index = 0; Index < Count; Index++) {
    HashAlg       = ReadUnaligned16 (DigestListBin);
    TotalSize    += sizeof (HashAlg);
    DigestListBin = (UINT8 *)DigestListBin + sizeof (HashAlg);

    DigestSize    = GetHashSizeFromAlgo (HashAlg);
    TotalSize    += DigestSize;
    DigestListBin = (UINT8 *)DigestListBin + DigestSize;
  }

  return TotalSize;
}

/**
  Copy TPML_DIGEST_VALUES compact binary into a buffer

  @param[in,out]    Buffer                  Buffer to hold copied TPML_DIGEST_VALUES compact binary.
  @param[in]        DigestListBin           TPML_DIGEST_VALUES compact binary buffer.
  @param[in]        HashAlgorithmMask       HASH bits corresponding to the desired digests to copy.
  @param[out]       HashAlgorithmMaskCopied Pointer to HASH bits corresponding to the digests copied.

  @return The end of buffer to hold TPML_DIGEST_VALUES compact binary.
**/
VOID *
CopyDigestListBinToBuffer (
  IN OUT VOID  *Buffer,
  IN VOID      *DigestListBin,
  IN UINT32    HashAlgorithmMask,
  OUT UINT32   *HashAlgorithmMaskCopied
  )
{
  UINTN          Index;
  UINT16         DigestSize;
  UINT32         Count;
  TPMI_ALG_HASH  HashAlg;
  UINT32         DigestListCount;
  UINT32         *DigestListCountPtr;

  DigestListCountPtr       = (UINT32 *)Buffer;
  DigestListCount          = 0;
  *HashAlgorithmMaskCopied = 0;

  Count         = ReadUnaligned32 (DigestListBin);
  Buffer        = (UINT8 *)Buffer + sizeof (Count);
  DigestListBin = (UINT8 *)DigestListBin + sizeof (Count);
  for (Index = 0; Index < Count; Index++) {
    HashAlg       = ReadUnaligned16 (DigestListBin);
    DigestListBin = (UINT8 *)DigestListBin + sizeof (HashAlg);
    DigestSize    = GetHashSizeFromAlgo (HashAlg);

    if ((HashAlg & HashAlgorithmMask) != 0) {
      CopyMem (Buffer, &HashAlg, sizeof (HashAlg));
      Buffer = (UINT8 *)Buffer + sizeof (HashAlg);
      CopyMem (Buffer, DigestListBin, DigestSize);
      Buffer = (UINT8 *)Buffer + DigestSize;
      DigestListCount++;
      (*HashAlgorithmMaskCopied) |= GetHashMaskFromAlgo (HashAlg);
    } else {
      DEBUG ((DEBUG_ERROR, "WARNING: CopyDigestListBinToBuffer Event log has HashAlg unsupported by PCR bank (0x%x)\n", HashAlg));
    }

    DigestListBin = (UINT8 *)DigestListBin + DigestSize;
  }

  WriteUnaligned32 (DigestListCountPtr, DigestListCount);

  return Buffer;
}

/**
  Add a new entry to the Event Log. The call chain is like below:
  TdxDxeLogHashEvent -> TdxDxeLogEvent -> TcgCommonLogEvent

  Before this function is called, the event information (including the digest)
  is ready.

  @param[in]     DigestList    A list of digest.
  @param[in,out] NewEventHdr   Pointer to a TD_EVENT_HDR data structure.
  @param[in]     NewEventData  Pointer to the new event data.

  @retval EFI_SUCCESS           The new event log entry was added.
  @retval EFI_OUT_OF_RESOURCES  No enough memory to log the new event.
**/
EFI_STATUS
TdxDxeLogHashEvent (
  IN      TPML_DIGEST_VALUES  *DigestList,
  IN OUT  CC_EVENT_HDR        *NewEventHdr,
  IN      UINT8               *NewEventData
  )
{
  EFI_STATUS               Status;
  EFI_TPL                  OldTpl;
  EFI_STATUS               RetStatus;
  CC_EVENT                 CcEvent;
  UINT8                    *DigestBuffer;
  UINT32                   *EventSizePtr;
  EFI_CC_EVENT_LOG_FORMAT  LogFormat;

  RetStatus = EFI_SUCCESS;
  LogFormat = EFI_CC_EVENT_LOG_FORMAT_TCG_2;

  ZeroMem (&CcEvent, sizeof (CcEvent));
  CcEvent.MrIndex   = NewEventHdr->MrIndex;
  CcEvent.EventType = NewEventHdr->EventType;
  DigestBuffer      = (UINT8 *)&CcEvent.Digests;
  EventSizePtr      = CopyDigestListToBuffer (DigestBuffer, DigestList, HASH_ALG_SHA384);
  CopyMem (EventSizePtr, &NewEventHdr->EventSize, sizeof (NewEventHdr->EventSize));

  //
  // Enter critical region
  //
  OldTpl = gBS->RaiseTPL (TPL_HIGH_LEVEL);
  Status = TdxDxeLogEvent (
             LogFormat,
             &CcEvent,
             sizeof (CcEvent.MrIndex) + sizeof (CcEvent.EventType) + GetDigestListBinSize (DigestBuffer) + sizeof (CcEvent.EventSize),
             NewEventData,
             NewEventHdr->EventSize
             );
  if (Status != EFI_SUCCESS) {
    RetStatus = Status;
  }

  gBS->RestoreTPL (OldTpl);

  return RetStatus;
}

/**
  Do a hash operation on a data buffer, extend a specific RTMR with the hash result,
  and add an entry to the Event Log.

  @param[in]      Flags         Bitmap providing additional information.
  @param[in]      HashData      Physical address of the start of the data buffer
                                to be hashed, extended, and logged.
  @param[in]      HashDataLen   The length, in bytes, of the buffer referenced by HashData
  @param[in, out] NewEventHdr   Pointer to a TD_EVENT_HDR data structure.
  @param[in]      NewEventData  Pointer to the new event data.

  @retval EFI_SUCCESS           Operation completed successfully.
  @retval EFI_OUT_OF_RESOURCES  No enough memory to log the new event.
  @retval EFI_DEVICE_ERROR      The command was unsuccessful.

**/
EFI_STATUS
TdxDxeHashLogExtendEvent (
  IN      UINT64        Flags,
  IN      UINT8         *HashData,
  IN      UINT64        HashDataLen,
  IN OUT  CC_EVENT_HDR  *NewEventHdr,
  IN      UINT8         *NewEventData
  )
{
  EFI_STATUS          Status;
  TPML_DIGEST_VALUES  DigestList;
  CC_EVENT_HDR        NoActionEvent;

  if (NewEventHdr->EventType == EV_NO_ACTION) {
    //
    // Do not do RTMR extend for EV_NO_ACTION
    //
    Status = EFI_SUCCESS;
    InitNoActionEvent (&NoActionEvent, NewEventHdr->EventSize);
    if ((Flags & EFI_CC_FLAG_EXTEND_ONLY) == 0) {
      Status = TdxDxeLogHashEvent (&(NoActionEvent.Digests), NewEventHdr, NewEventData);
    }

    return Status;
  }

  //
  // According to UEFI Spec 2.10 Section 38.4.1 the mapping between MrIndex and Intel
  // TDX Measurement Register is:
  //    MrIndex 0   <--> MRTD
  //    MrIndex 1-3 <--> RTMR[0-2]
  // Only the RMTR registers can be extended in TDVF by HashAndExtend. So MrIndex will
  // decreased by 1 before it is sent to HashAndExtend.
  //
  Status = HashAndExtend (
             NewEventHdr->MrIndex - 1,
             HashData,
             (UINTN)HashDataLen,
             &DigestList
             );
  if (!EFI_ERROR (Status)) {
    if ((Flags & EFI_CC_FLAG_EXTEND_ONLY) == 0) {
      Status = TdxDxeLogHashEvent (&DigestList, NewEventHdr, NewEventData);
    }
  }

  return Status;
}

/**
  The EFI_CC_MEASUREMENT_PROTOCOL HashLogExtendEvent function call provides callers with
  an opportunity to extend and optionally log events without requiring
  knowledge of actual TPM commands.
  The extend operation will occur even if this function cannot create an event
  log entry (e.g. due to the event log being full).

  @param[in]  This               Indicates the calling context
  @param[in]  Flags              Bitmap providing additional information.
  @param[in]  DataToHash         Physical address of the start of the data buffer to be hashed.
  @param[in]  DataToHashLen      The length in bytes of the buffer referenced by DataToHash.
  @param[in]  Event              Pointer to data buffer containing information about the event.

  @retval EFI_SUCCESS            Operation completed successfully.
  @retval EFI_DEVICE_ERROR       The command was unsuccessful.
  @retval EFI_VOLUME_FULL        The extend operation occurred, but the event could not be written to one or more event logs.
  @retval EFI_INVALID_PARAMETER  One or more of the parameters are incorrect.
  @retval EFI_UNSUPPORTED        The PE/COFF image type is not supported.
**/
EFI_STATUS
EFIAPI
TdHashLogExtendEvent (
  IN EFI_CC_MEASUREMENT_PROTOCOL  *This,
  IN UINT64                       Flags,
  IN EFI_PHYSICAL_ADDRESS         DataToHash,
  IN UINT64                       DataToHashLen,
  IN EFI_CC_EVENT                 *CcEvent
  )
{
  EFI_STATUS          Status;
  CC_EVENT_HDR        NewEventHdr;
  TPML_DIGEST_VALUES  DigestList;

  DEBUG ((DEBUG_VERBOSE, "TdHashLogExtendEvent ...\n"));

  if ((This == NULL) || (CcEvent == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Do not check hash data size for EV_NO_ACTION event.
  //
  if ((CcEvent->Header.EventType != EV_NO_ACTION) && (DataToHash == 0)) {
    return EFI_INVALID_PARAMETER;
  }

  if (CcEvent->Size < CcEvent->Header.HeaderSize + sizeof (UINT32)) {
    return EFI_INVALID_PARAMETER;
  }

  if (CcEvent->Header.MrIndex == CC_MR_INDEX_0_MRTD) {
    DEBUG ((DEBUG_ERROR, "%a: MRTD cannot be extended in TDVF.\n", __func__));
    return EFI_INVALID_PARAMETER;
  }

  if (CcEvent->Header.MrIndex >= CC_MR_INDEX_INVALID) {
    DEBUG ((DEBUG_ERROR, "%a: MrIndex is invalid. (%d)\n", __func__, CcEvent->Header.MrIndex));
    return EFI_INVALID_PARAMETER;
  }

  NewEventHdr.MrIndex   = CcEvent->Header.MrIndex;
  NewEventHdr.EventType = CcEvent->Header.EventType;
  NewEventHdr.EventSize = CcEvent->Size - sizeof (UINT32) - CcEvent->Header.HeaderSize;
  if ((Flags & EFI_CC_FLAG_PE_COFF_IMAGE) != 0) {
    //
    // According to UEFI Spec 2.10 Section 38.4.1 the mapping between MrIndex and Intel
    // TDX Measurement Register is:
    //    MrIndex 0   <--> MRTD
    //    MrIndex 1-3 <--> RTMR[0-2]
    // Only the RMTR registers can be extended in TDVF by HashAndExtend. So MrIndex will
    // decreased by 1 before it is sent to MeasurePeImageAndExtend.
    //
    Status = MeasurePeImageAndExtend (
               NewEventHdr.MrIndex - 1,
               DataToHash,
               (UINTN)DataToHashLen,
               &DigestList
               );
    if (!EFI_ERROR (Status)) {
      if ((Flags & EFI_CC_FLAG_EXTEND_ONLY) == 0) {
        Status = TdxDxeLogHashEvent (&DigestList, &NewEventHdr, CcEvent->Event);
      }
    }
  } else {
    Status = TdxDxeHashLogExtendEvent (
               Flags,
               (UINT8 *)(UINTN)DataToHash,
               DataToHashLen,
               &NewEventHdr,
               CcEvent->Event
               );
  }

  DEBUG ((DEBUG_VERBOSE, "TdHashLogExtendEvent - %r\n", Status));
  return Status;
}

EFI_CC_MEASUREMENT_PROTOCOL  mTdProtocol = {
  TdGetCapability,
  TdGetEventLog,
  TdHashLogExtendEvent,
  TdMapPcrToMrIndex,
};

#define TD_HASH_COUNT  1
#define TEMP_BUF_LEN   (sizeof(TCG_EfiSpecIDEventStruct) +  sizeof(UINT32) \
                     + (TD_HASH_COUNT * sizeof(TCG_EfiSpecIdEventAlgorithmSize)) + sizeof(UINT8))

/**
  Initialize the TD Event Log and log events passed from the PEI phase.

  @retval EFI_SUCCESS           Operation completed successfully.
  @retval EFI_OUT_OF_RESOURCES  Out of memory.

**/
EFI_STATUS
SetupCcEventLog (
  VOID
  )
{
  EFI_STATUS                       Status;
  EFI_PHYSICAL_ADDRESS             Lasa;
  UINTN                            Index;
  TCG_EfiSpecIDEventStruct         *TcgEfiSpecIdEventStruct;
  UINT8                            TempBuf[TEMP_BUF_LEN];
  TCG_PCR_EVENT_HDR                SpecIdEvent;
  TCG_EfiSpecIdEventAlgorithmSize  *DigestSize;
  TCG_EfiSpecIdEventAlgorithmSize  *TempDigestSize;
  UINT8                            *VendorInfoSize;
  UINT32                           NumberOfAlgorithms;
  EFI_CC_EVENT_LOG_FORMAT          LogFormat;
  EFI_PEI_HOB_POINTERS             GuidHob;
  CC_EVENT_HDR                     NoActionEvent;

  Status = EFI_SUCCESS;
  DEBUG ((DEBUG_INFO, "SetupCcEventLog\n"));

  Index     = 0;
  LogFormat = EFI_CC_EVENT_LOG_FORMAT_TCG_2;

  //
  // 1. Create Log Area
  //
  mTdxDxeData.EventLogAreaStruct[Index].EventLogFormat = LogFormat;

  // allocate pages for TD Event log
  Status = gBS->AllocatePages (
                  AllocateAnyPages,
                  EfiACPIMemoryNVS,
                  EFI_SIZE_TO_PAGES (PcdGet32 (PcdTcgLogAreaMinLen)),
                  &Lasa
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  mTdxDxeData.EventLogAreaStruct[Index].Lasa                  = Lasa;
  mTdxDxeData.EventLogAreaStruct[Index].Laml                  = PcdGet32 (PcdTcgLogAreaMinLen);
  mTdxDxeData.EventLogAreaStruct[Index].Next800155EventOffset = 0;

  //
  // Report TD event log address and length, so that they can be reported in
  // TD ACPI table. Ignore the return status, because those fields are optional.
  //
  PcdSet32S (PcdCcEventlogAcpiTableLaml, (UINT32)mTdxDxeData.EventLogAreaStruct[Index].Laml);
  PcdSet64S (PcdCcEventlogAcpiTableLasa, mTdxDxeData.EventLogAreaStruct[Index].Lasa);

  //
  // To initialize them as 0xFF is recommended
  // because the OS can know the last entry for that.
  //
  SetMem ((VOID *)(UINTN)Lasa, PcdGet32 (PcdTcgLogAreaMinLen), 0xFF);

  //
  // Create first entry for Log Header Entry Data
  //

  //
  // TcgEfiSpecIdEventStruct
  //
  TcgEfiSpecIdEventStruct = (TCG_EfiSpecIDEventStruct *)TempBuf;
  CopyMem (TcgEfiSpecIdEventStruct->signature, TCG_EfiSpecIDEventStruct_SIGNATURE_03, sizeof (TcgEfiSpecIdEventStruct->signature));

  TcgEfiSpecIdEventStruct->platformClass = PcdGet8 (PcdTpmPlatformClass);

  TcgEfiSpecIdEventStruct->specVersionMajor = TCG_EfiSpecIDEventStruct_SPEC_VERSION_MAJOR_TPM2;
  TcgEfiSpecIdEventStruct->specVersionMinor = TCG_EfiSpecIDEventStruct_SPEC_VERSION_MINOR_TPM2;
  TcgEfiSpecIdEventStruct->specErrata       = TCG_EfiSpecIDEventStruct_SPEC_ERRATA_TPM2;
  TcgEfiSpecIdEventStruct->uintnSize        = sizeof (UINTN)/sizeof (UINT32);
  NumberOfAlgorithms                        = 0;
  DigestSize                                = (TCG_EfiSpecIdEventAlgorithmSize *)((UINT8 *)TcgEfiSpecIdEventStruct
                                                                                  + sizeof (*TcgEfiSpecIdEventStruct)
                                                                                  + sizeof (NumberOfAlgorithms));

  TempDigestSize              = DigestSize;
  TempDigestSize             += NumberOfAlgorithms;
  TempDigestSize->algorithmId = TPM_ALG_SHA384;
  TempDigestSize->digestSize  = SHA384_DIGEST_SIZE;
  NumberOfAlgorithms++;

  CopyMem (TcgEfiSpecIdEventStruct + 1, &NumberOfAlgorithms, sizeof (NumberOfAlgorithms));
  TempDigestSize  = DigestSize;
  TempDigestSize += NumberOfAlgorithms;
  VendorInfoSize  = (UINT8 *)TempDigestSize;
  *VendorInfoSize = 0;

  SpecIdEvent.PCRIndex  = 1; // PCRIndex 0 maps to MrIndex 1
  SpecIdEvent.EventType = EV_NO_ACTION;
  ZeroMem (&SpecIdEvent.Digest, sizeof (SpecIdEvent.Digest));
  SpecIdEvent.EventSize = (UINT32)GetTcgEfiSpecIdEventStructSize (TcgEfiSpecIdEventStruct);

  //
  // TD Event log re-use the spec of TCG2 Event log.
  // Log TcgEfiSpecIdEventStruct as the first Event. Event format is TCG_PCR_EVENT.
  //   TCG EFI Protocol Spec. Section 5.3 Event Log Header
  //   TCG PC Client PFP spec. Section 9.2 Measurement Event Entries and Log
  //
  Status = TdxDxeLogEvent (
             LogFormat,
             &SpecIdEvent,
             sizeof (SpecIdEvent),
             (UINT8 *)TcgEfiSpecIdEventStruct,
             SpecIdEvent.EventSize
             );
  //
  // record the offset at the end of 800-155 event.
  // the future 800-155 event can be inserted here.
  //
  mTdxDxeData.EventLogAreaStruct[Index].Next800155EventOffset = mTdxDxeData.EventLogAreaStruct[Index].EventLogSize;

  //
  // Tcg800155PlatformIdEvent. Event format is TCG_PCR_EVENT2
  //
  GuidHob.Guid = GetFirstGuidHob (&gTcg800155PlatformIdEventHobGuid);
  while (GuidHob.Guid != NULL) {
    InitNoActionEvent (&NoActionEvent, GET_GUID_HOB_DATA_SIZE (GuidHob.Guid));

    Status = TdxDxeLogEvent (
               LogFormat,
               &NoActionEvent,
               sizeof (NoActionEvent.MrIndex) + sizeof (NoActionEvent.EventType) + GetDigestListBinSize (&NoActionEvent.Digests) + sizeof (NoActionEvent.EventSize),
               GET_GUID_HOB_DATA (GuidHob.Guid),
               GET_GUID_HOB_DATA_SIZE (GuidHob.Guid)
               );

    GuidHob.Guid = GET_NEXT_HOB (GuidHob);
    GuidHob.Guid = GetNextGuidHob (&gTcg800155PlatformIdEventHobGuid, GuidHob.Guid);
  }

  //
  // 2. Create Final Log Area
  //
  Status = gBS->AllocatePages (
                  AllocateAnyPages,
                  EfiACPIMemoryNVS,
                  EFI_SIZE_TO_PAGES (PcdGet32 (PcdTcg2FinalLogAreaLen)),
                  &Lasa
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  SetMem ((VOID *)(UINTN)Lasa, PcdGet32 (PcdTcg2FinalLogAreaLen), 0xFF);

  //
  // Initialize
  //
  mTdxDxeData.FinalEventsTable[Index]                   = (VOID *)(UINTN)Lasa;
  (mTdxDxeData.FinalEventsTable[Index])->Version        = EFI_TCG2_FINAL_EVENTS_TABLE_VERSION;
  (mTdxDxeData.FinalEventsTable[Index])->NumberOfEvents = 0;

  mTdxDxeData.FinalEventLogAreaStruct[Index].EventLogFormat        = LogFormat;
  mTdxDxeData.FinalEventLogAreaStruct[Index].Lasa                  = Lasa + sizeof (EFI_CC_FINAL_EVENTS_TABLE);
  mTdxDxeData.FinalEventLogAreaStruct[Index].Laml                  = PcdGet32 (PcdTcg2FinalLogAreaLen) - sizeof (EFI_CC_FINAL_EVENTS_TABLE);
  mTdxDxeData.FinalEventLogAreaStruct[Index].EventLogSize          = 0;
  mTdxDxeData.FinalEventLogAreaStruct[Index].LastEvent             = (VOID *)(UINTN)mTdxDxeData.FinalEventLogAreaStruct[Index].Lasa;
  mTdxDxeData.FinalEventLogAreaStruct[Index].EventLogStarted       = FALSE;
  mTdxDxeData.FinalEventLogAreaStruct[Index].EventLogTruncated     = FALSE;
  mTdxDxeData.FinalEventLogAreaStruct[Index].Next800155EventOffset = 0;

  //
  // Install to configuration table for EFI_CC_EVENT_LOG_FORMAT_TCG_2
  //
  Status = gBS->InstallConfigurationTable (&gEfiCcFinalEventsTableGuid, (VOID *)mTdxDxeData.FinalEventsTable[Index]);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  return Status;
}

/**
  Measure and log an action string, and extend the measurement result into RTMR.

  @param[in] MrIndex        MrIndex to extend
  @param[in] String           A specific string that indicates an Action event.

  @retval EFI_SUCCESS         Operation completed successfully.
  @retval EFI_DEVICE_ERROR    The operation was unsuccessful.

**/
EFI_STATUS
TdMeasureAction (
  IN      UINT32  MrIndex,
  IN      CHAR8   *String
  )
{
  CC_EVENT_HDR  CcEvent;

  CcEvent.MrIndex   = MrIndex;
  CcEvent.EventType = EV_EFI_ACTION;
  CcEvent.EventSize = (UINT32)AsciiStrLen (String);
  return TdxDxeHashLogExtendEvent (
           0,
           (UINT8 *)String,
           CcEvent.EventSize,
           &CcEvent,
           (UINT8 *)String
           );
}

/**
  Measure and log EFI handoff tables, and extend the measurement result into PCR[1].

  @retval EFI_SUCCESS         Operation completed successfully.
  @retval EFI_DEVICE_ERROR    The operation was unsuccessful.

**/
EFI_STATUS
MeasureHandoffTables (
  VOID
  )
{
  EFI_STATUS                  Status;
  CC_EVENT_HDR                CcEvent;
  EFI_HANDOFF_TABLE_POINTERS  HandoffTables;
  UINTN                       ProcessorNum;
  EFI_CPU_PHYSICAL_LOCATION   *ProcessorLocBuf;

  ProcessorLocBuf = NULL;
  Status          = EFI_SUCCESS;

  if (PcdGet8 (PcdTpmPlatformClass) == TCG_PLATFORM_TYPE_SERVER) {
    //
    // Tcg Server spec.
    // Measure each processor EFI_CPU_PHYSICAL_LOCATION with EV_TABLE_OF_DEVICES to PCR[1]
    //
    Status = GetProcessorsCpuLocation (&ProcessorLocBuf, &ProcessorNum);

    if (!EFI_ERROR (Status)) {
      CcEvent.MrIndex   = MapPcrToMrIndex (1);
      CcEvent.EventType = EV_TABLE_OF_DEVICES;
      CcEvent.EventSize = sizeof (HandoffTables);

      HandoffTables.NumberOfTables            = 1;
      HandoffTables.TableEntry[0].VendorGuid  = gEfiMpServiceProtocolGuid;
      HandoffTables.TableEntry[0].VendorTable = ProcessorLocBuf;

      Status = TdxDxeHashLogExtendEvent (
                 0,
                 (UINT8 *)(UINTN)ProcessorLocBuf,
                 sizeof (EFI_CPU_PHYSICAL_LOCATION) * ProcessorNum,
                 &CcEvent,
                 (UINT8 *)&HandoffTables
                 );

      FreePool (ProcessorLocBuf);
    }
  }

  return Status;
}

/**
  Measure and log Separator event, and extend the measurement result into a specific PCR.

  @param[in] PCRIndex         PCR index.

  @retval EFI_SUCCESS         Operation completed successfully.
  @retval EFI_DEVICE_ERROR    The operation was unsuccessful.

**/
EFI_STATUS
MeasureSeparatorEvent (
  IN      UINT32  MrIndex
  )
{
  CC_EVENT_HDR  CcEvent;
  UINT32        EventData;

  DEBUG ((DEBUG_INFO, "MeasureSeparatorEvent to Rtmr - %d\n", MrIndex));

  EventData         = 0;
  CcEvent.MrIndex   = MrIndex;
  CcEvent.EventType = EV_SEPARATOR;
  CcEvent.EventSize = (UINT32)sizeof (EventData);

  return TdxDxeHashLogExtendEvent (
           0,
           (UINT8 *)&EventData,
           sizeof (EventData),
           &CcEvent,
           (UINT8 *)&EventData
           );
}

/**
  Measure and log an EFI variable, and extend the measurement result into a specific RTMR.

  @param[in]  MrIndex         RTMR Index.
  @param[in]  EventType         Event type.
  @param[in]  VarName           A Null-terminated string that is the name of the vendor's variable.
  @param[in]  VendorGuid        A unique identifier for the vendor.
  @param[in]  VarData           The content of the variable data.
  @param[in]  VarSize           The size of the variable data.

  @retval EFI_SUCCESS           Operation completed successfully.
  @retval EFI_OUT_OF_RESOURCES  Out of memory.
  @retval EFI_DEVICE_ERROR      The operation was unsuccessful.

**/
EFI_STATUS
MeasureVariable (
  IN      UINT32         MrIndex,
  IN      TCG_EVENTTYPE  EventType,
  IN      CHAR16         *VarName,
  IN      EFI_GUID       *VendorGuid,
  IN      VOID           *VarData,
  IN      UINTN          VarSize
  )
{
  EFI_STATUS          Status;
  CC_EVENT_HDR        CcEvent;
  UINTN               VarNameLength;
  UEFI_VARIABLE_DATA  *VarLog;

  DEBUG ((DEBUG_INFO, "TdTcg2Dxe: MeasureVariable (Rtmr - %x, EventType - %x, ", (UINTN)MrIndex, (UINTN)EventType));
  DEBUG ((DEBUG_INFO, "VariableName - %s, VendorGuid - %g)\n", VarName, VendorGuid));

  VarNameLength     = StrLen (VarName);
  CcEvent.MrIndex   = MrIndex;
  CcEvent.EventType = EventType;

  CcEvent.EventSize = (UINT32)(sizeof (*VarLog) + VarNameLength * sizeof (*VarName) + VarSize
                               - sizeof (VarLog->UnicodeName) - sizeof (VarLog->VariableData));

  VarLog = (UEFI_VARIABLE_DATA *)AllocatePool (CcEvent.EventSize);
  if (VarLog == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  VarLog->VariableName       = *VendorGuid;
  VarLog->UnicodeNameLength  = VarNameLength;
  VarLog->VariableDataLength = VarSize;
  CopyMem (
    VarLog->UnicodeName,
    VarName,
    VarNameLength * sizeof (*VarName)
    );
  if ((VarSize != 0) && (VarData != NULL)) {
    CopyMem (
      (CHAR16 *)VarLog->UnicodeName + VarNameLength,
      VarData,
      VarSize
      );
  }

  if (EventType == EV_EFI_VARIABLE_DRIVER_CONFIG) {
    //
    // Digest is the event data (UEFI_VARIABLE_DATA)
    //
    Status = TdxDxeHashLogExtendEvent (
               0,
               (UINT8 *)VarLog,
               CcEvent.EventSize,
               &CcEvent,
               (UINT8 *)VarLog
               );
  } else {
    ASSERT (VarData != NULL);
    Status = TdxDxeHashLogExtendEvent (
               0,
               (UINT8 *)VarData,
               VarSize,
               &CcEvent,
               (UINT8 *)VarLog
               );
  }

  FreePool (VarLog);
  return Status;
}

/**
  Read then Measure and log an EFI variable, and extend the measurement result into a specific RTMR.

  @param[in]  MrIndex           RTMR Index.
  @param[in]  EventType         Event type.
  @param[in]   VarName          A Null-terminated string that is the name of the vendor's variable.
  @param[in]   VendorGuid       A unique identifier for the vendor.
  @param[out]  VarSize          The size of the variable data.
  @param[out]  VarData          Pointer to the content of the variable.

  @retval EFI_SUCCESS           Operation completed successfully.
  @retval EFI_OUT_OF_RESOURCES  Out of memory.
  @retval EFI_DEVICE_ERROR      The operation was unsuccessful.

**/
EFI_STATUS
ReadAndMeasureVariable (
  IN      UINT32         MrIndex,
  IN      TCG_EVENTTYPE  EventType,
  IN      CHAR16         *VarName,
  IN      EFI_GUID       *VendorGuid,
  OUT     UINTN          *VarSize,
  OUT     VOID           **VarData
  )
{
  EFI_STATUS  Status;

  Status = GetVariable2 (VarName, VendorGuid, VarData, VarSize);
  if (EventType == EV_EFI_VARIABLE_DRIVER_CONFIG) {
    if (EFI_ERROR (Status)) {
      //
      // It is valid case, so we need handle it.
      //
      *VarData = NULL;
      *VarSize = 0;
    }
  } else {
    //
    // if status error, VarData is freed and set NULL by GetVariable2
    //
    if (EFI_ERROR (Status)) {
      return EFI_NOT_FOUND;
    }
  }

  Status = MeasureVariable (
             MrIndex,
             EventType,
             VarName,
             VendorGuid,
             *VarData,
             *VarSize
             );
  return Status;
}

/**
  Read then Measure and log an EFI boot variable, and extend the measurement result into PCR[1].
according to TCG PC Client PFP spec 0021 Section 2.4.4.2

  @param[in]   VarName          A Null-terminated string that is the name of the vendor's variable.
  @param[in]   VendorGuid       A unique identifier for the vendor.
  @param[out]  VarSize          The size of the variable data.
  @param[out]  VarData          Pointer to the content of the variable.

  @retval EFI_SUCCESS           Operation completed successfully.
  @retval EFI_OUT_OF_RESOURCES  Out of memory.
  @retval EFI_DEVICE_ERROR      The operation was unsuccessful.

**/
EFI_STATUS
ReadAndMeasureBootVariable (
  IN      CHAR16    *VarName,
  IN      EFI_GUID  *VendorGuid,
  OUT     UINTN     *VarSize,
  OUT     VOID      **VarData
  )
{
  return ReadAndMeasureVariable (
           MapPcrToMrIndex (1),
           EV_EFI_VARIABLE_BOOT,
           VarName,
           VendorGuid,
           VarSize,
           VarData
           );
}

/**
  Read then Measure and log an EFI Secure variable, and extend the measurement result into PCR[7].

  @param[in]   VarName          A Null-terminated string that is the name of the vendor's variable.
  @param[in]   VendorGuid       A unique identifier for the vendor.
  @param[out]  VarSize          The size of the variable data.
  @param[out]  VarData          Pointer to the content of the variable.

  @retval EFI_SUCCESS           Operation completed successfully.
  @retval EFI_OUT_OF_RESOURCES  Out of memory.
  @retval EFI_DEVICE_ERROR      The operation was unsuccessful.

**/
EFI_STATUS
ReadAndMeasureSecureVariable (
  IN      CHAR16    *VarName,
  IN      EFI_GUID  *VendorGuid,
  OUT     UINTN     *VarSize,
  OUT     VOID      **VarData
  )
{
  return ReadAndMeasureVariable (
           MapPcrToMrIndex (7),
           EV_EFI_VARIABLE_DRIVER_CONFIG,
           VarName,
           VendorGuid,
           VarSize,
           VarData
           );
}

/**
  Measure and log all EFI boot variables, and extend the measurement result into a specific PCR.

  The EFI boot variables are BootOrder and Boot#### variables.

  @retval EFI_SUCCESS           Operation completed successfully.
  @retval EFI_OUT_OF_RESOURCES  Out of memory.
  @retval EFI_DEVICE_ERROR      The operation was unsuccessful.

**/
EFI_STATUS
MeasureAllBootVariables (
  VOID
  )
{
  EFI_STATUS  Status;
  UINT16      *BootOrder;
  UINTN       BootCount;
  UINTN       Index;
  VOID        *BootVarData;
  UINTN       Size;

  Status = ReadAndMeasureBootVariable (
             mBootVarName,
             &gEfiGlobalVariableGuid,
             &BootCount,
             (VOID **)&BootOrder
             );
  if ((Status == EFI_NOT_FOUND) || (BootOrder == NULL)) {
    return EFI_SUCCESS;
  }

  if (EFI_ERROR (Status)) {
    //
    // BootOrder can't be NULL if status is not EFI_NOT_FOUND
    //
    FreePool (BootOrder);
    return Status;
  }

  BootCount /= sizeof (*BootOrder);
  for (Index = 0; Index < BootCount; Index++) {
    UnicodeSPrint (mBootVarName, sizeof (mBootVarName), L"Boot%04x", BootOrder[Index]);
    Status = ReadAndMeasureBootVariable (
               mBootVarName,
               &gEfiGlobalVariableGuid,
               &Size,
               &BootVarData
               );
    if (!EFI_ERROR (Status)) {
      FreePool (BootVarData);
    }
  }

  FreePool (BootOrder);
  return EFI_SUCCESS;
}

/**
  Measure and log all EFI Secure variables, and extend the measurement result into a specific PCR.

  The EFI boot variables are BootOrder and Boot#### variables.

  @retval EFI_SUCCESS           Operation completed successfully.
  @retval EFI_OUT_OF_RESOURCES  Out of memory.
  @retval EFI_DEVICE_ERROR      The operation was unsuccessful.

**/
EFI_STATUS
MeasureAllSecureVariables (
  VOID
  )
{
  EFI_STATUS  Status;
  VOID        *Data;
  UINTN       DataSize;
  UINTN       Index;

  Status = EFI_NOT_FOUND;
  for (Index = 0; Index < sizeof (mVariableType)/sizeof (mVariableType[0]); Index++) {
    Status = ReadAndMeasureSecureVariable (
               mVariableType[Index].VariableName,
               mVariableType[Index].VendorGuid,
               &DataSize,
               &Data
               );
    if (!EFI_ERROR (Status)) {
      if (Data != NULL) {
        FreePool (Data);
      }
    }
  }

  //
  // Measure DBT if present and not empty
  //
  Status = GetVariable2 (EFI_IMAGE_SECURITY_DATABASE2, &gEfiImageSecurityDatabaseGuid, &Data, &DataSize);
  if (!EFI_ERROR (Status)) {
    Status = MeasureVariable (
               MapPcrToMrIndex (7),
               EV_EFI_VARIABLE_DRIVER_CONFIG,
               EFI_IMAGE_SECURITY_DATABASE2,
               &gEfiImageSecurityDatabaseGuid,
               Data,
               DataSize
               );
    FreePool (Data);
  } else {
    DEBUG ((DEBUG_INFO, "Skip measuring variable %s since it's deleted\n", EFI_IMAGE_SECURITY_DATABASE2));
  }

  return EFI_SUCCESS;
}

/**
  Measure and log launch of FirmwareDebugger, and extend the measurement result into a specific PCR.

  @retval EFI_SUCCESS           Operation completed successfully.
  @retval EFI_OUT_OF_RESOURCES  Out of memory.
  @retval EFI_DEVICE_ERROR      The operation was unsuccessful.

**/
EFI_STATUS
MeasureLaunchOfFirmwareDebugger (
  VOID
  )
{
  CC_EVENT_HDR  CcEvent;

  CcEvent.MrIndex   = MapPcrToMrIndex (7);
  CcEvent.EventType = EV_EFI_ACTION;
  CcEvent.EventSize = sizeof (FIRMWARE_DEBUGGER_EVENT_STRING) - 1;
  return TdxDxeHashLogExtendEvent (
           0,
           (UINT8 *)FIRMWARE_DEBUGGER_EVENT_STRING,
           sizeof (FIRMWARE_DEBUGGER_EVENT_STRING) - 1,
           &CcEvent,
           (UINT8 *)FIRMWARE_DEBUGGER_EVENT_STRING
           );
}

/**
  Measure and log all Secure Boot Policy, and extend the measurement result into a specific PCR.

  Platform firmware adhering to the policy must therefore measure the following values into PCR[7]: (in order listed)
   - The contents of the SecureBoot variable
   - The contents of the PK variable
   - The contents of the KEK variable
   - The contents of the EFI_IMAGE_SECURITY_DATABASE variable
   - The contents of the EFI_IMAGE_SECURITY_DATABASE1 variable
   - Separator
   - Entries in the EFI_IMAGE_SECURITY_DATABASE that are used to validate EFI Drivers or EFI Boot Applications in the boot path

  NOTE: Because of the above, UEFI variables PK, KEK, EFI_IMAGE_SECURITY_DATABASE,
  EFI_IMAGE_SECURITY_DATABASE1 and SecureBoot SHALL NOT be measured into PCR[3].

  @param[in]  Event     Event whose notification function is being invoked
  @param[in]  Context   Pointer to the notification function's context
**/
VOID
EFIAPI
MeasureSecureBootPolicy (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  EFI_STATUS  Status;
  VOID        *Protocol;

  Status = gBS->LocateProtocol (&gEfiVariableWriteArchProtocolGuid, NULL, (VOID **)&Protocol);
  if (EFI_ERROR (Status)) {
    return;
  }

  if (PcdGetBool (PcdFirmwareDebuggerInitialized)) {
    Status = MeasureLaunchOfFirmwareDebugger ();
    DEBUG ((DEBUG_INFO, "MeasureLaunchOfFirmwareDebugger - %r\n", Status));
  }

  Status = MeasureAllSecureVariables ();
  DEBUG ((DEBUG_INFO, "MeasureAllSecureVariables - %r\n", Status));

  //
  // We need measure Separator(7) here, because this event must be between SecureBootPolicy (Configure)
  // and ImageVerification (Authority)
  // There might be a case that we need measure UEFI image from DriverOrder, besides BootOrder. So
  // the Authority measurement happen before ReadToBoot event.
  //
  Status = MeasureSeparatorEvent (MapPcrToMrIndex (7));
  DEBUG ((DEBUG_INFO, "MeasureSeparatorEvent - %r\n", Status));
  return;
}

/**
  Ready to Boot Event notification handler.

  Sequence of OS boot events is measured in this event notification handler.

  @param[in]  Event     Event whose notification function is being invoked
  @param[in]  Context   Pointer to the notification function's context

**/
VOID
EFIAPI
OnReadyToBoot (
  IN      EFI_EVENT  Event,
  IN      VOID       *Context
  )
{
  EFI_STATUS  Status;

  PERF_START_EX (mImageHandle, "EventRec", "TdTcg2Dxe", 0, PERF_ID_CC_TCG2_DXE);
  if (mBootAttempts == 0) {
    //
    // Measure handoff tables.
    //
    Status = MeasureHandoffTables ();
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "HOBs not Measured. Error!\n"));
    }

    //
    // Measure BootOrder & Boot#### variables.
    //
    Status = MeasureAllBootVariables ();
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "Boot Variables not Measured. Error!\n"));
    }

    //
    // 1. This is the first boot attempt.
    //
    Status = TdMeasureAction (
               MapPcrToMrIndex (4),
               EFI_CALLING_EFI_APPLICATION
               );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a not Measured. Error!\n", EFI_CALLING_EFI_APPLICATION));
    }

    //
    // 2. Draw a line between pre-boot env and entering post-boot env.
    //
    // According to UEFI Spec 2.10 Section 38.4.1 the mapping between MrIndex and Intel
    // TDX Measurement Register is:
    //    MrIndex 0   <--> MRTD
    //    MrIndex 1-3 <--> RTMR[0-2]
    // RTMR[0] (i.e. MrIndex 1) is already done. So SepartorEvent shall be extended to
    // RTMR[1] (i.e. MrIndex 2) as well.
    //
    Status = MeasureSeparatorEvent (CC_MR_INDEX_2_RTMR1);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "Separator Event not Measured to RTMR[1]. Error!\n"));
    }

    //
    // 3. Measure GPT. It would be done in SAP driver.
    //

    //
    // 4. Measure PE/COFF OS loader. It would be done in SAP driver.
    //

    //
    // 5. Read & Measure variable. BootOrder already measured.
    //
  } else {
    //
    // 6. Not first attempt, meaning a return from last attempt
    //
    Status = TdMeasureAction (
               MapPcrToMrIndex (4),
               EFI_RETURNING_FROM_EFI_APPLICATION
               );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a not Measured. Error!\n", EFI_RETURNING_FROM_EFI_APPLICATION));
    }

    //
    // 7. Next boot attempt, measure "Calling EFI Application from Boot Option" again
    // TCG PC Client PFP spec Section 2.4.4.5 Step 4
    //
    Status = TdMeasureAction (
               MapPcrToMrIndex (4),
               EFI_CALLING_EFI_APPLICATION
               );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a not Measured. Error!\n", EFI_CALLING_EFI_APPLICATION));
    }
  }

  DEBUG ((DEBUG_INFO, "TdTcg2Dxe Measure Data when ReadyToBoot\n"));
  //
  // Increase boot attempt counter.
  //
  mBootAttempts++;
  PERF_END_EX (mImageHandle, "EventRec", "Tcg2Dxe", 0, PERF_ID_CC_TCG2_DXE + 1);
}

/**
  Exit Boot Services Event notification handler.

  Measure invocation and success of ExitBootServices.

  @param[in]  Event     Event whose notification function is being invoked
  @param[in]  Context   Pointer to the notification function's context

**/
VOID
EFIAPI
OnExitBootServices (
  IN      EFI_EVENT  Event,
  IN      VOID       *Context
  )
{
  EFI_STATUS  Status;

  //
  // Measure invocation of ExitBootServices,
  //
  Status = TdMeasureAction (
             MapPcrToMrIndex (5),
             EFI_EXIT_BOOT_SERVICES_INVOCATION
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a not Measured. Error!\n", EFI_EXIT_BOOT_SERVICES_INVOCATION));
  }

  //
  // Measure success of ExitBootServices
  //
  Status = TdMeasureAction (
             MapPcrToMrIndex (5),
             EFI_EXIT_BOOT_SERVICES_SUCCEEDED
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a not Measured. Error!\n", EFI_EXIT_BOOT_SERVICES_SUCCEEDED));
  }
}

/**
  Exit Boot Services Failed Event notification handler.

  Measure Failure of ExitBootServices.

  @param[in]  Event     Event whose notification function is being invoked
  @param[in]  Context   Pointer to the notification function's context

**/
VOID
EFIAPI
OnExitBootServicesFailed (
  IN      EFI_EVENT  Event,
  IN      VOID       *Context
  )
{
  EFI_STATUS  Status;

  //
  // Measure Failure of ExitBootServices,
  //
  Status = TdMeasureAction (
             MapPcrToMrIndex (5),
             EFI_EXIT_BOOT_SERVICES_FAILED
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a not Measured. Error!\n", EFI_EXIT_BOOT_SERVICES_FAILED));
  }
}

EFI_STATUS
SyncCcEvent (
  VOID
  )
{
  EFI_STATUS               Status;
  EFI_PEI_HOB_POINTERS     GuidHob;
  VOID                     *CcEvent;
  VOID                     *DigestListBin;
  UINT32                   DigestListBinSize;
  UINT8                    *Event;
  UINT32                   EventSize;
  EFI_CC_EVENT_LOG_FORMAT  LogFormat;

  DEBUG ((DEBUG_INFO, "Sync Cc event from SEC\n"));

  Status       = EFI_SUCCESS;
  LogFormat    = EFI_CC_EVENT_LOG_FORMAT_TCG_2;
  GuidHob.Guid = GetFirstGuidHob (&gCcEventEntryHobGuid);

  while (!EFI_ERROR (Status) && GuidHob.Guid != NULL) {
    CcEvent = AllocateCopyPool (GET_GUID_HOB_DATA_SIZE (GuidHob.Guid), GET_GUID_HOB_DATA (GuidHob.Guid));
    if (CcEvent == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    GuidHob.Guid = GET_NEXT_HOB (GuidHob);
    GuidHob.Guid = GetNextGuidHob (&gCcEventEntryHobGuid, GuidHob.Guid);

    DigestListBin     = (UINT8 *)CcEvent + sizeof (UINT32) + sizeof (TCG_EVENTTYPE);
    DigestListBinSize = GetDigestListBinSize (DigestListBin);

    //
    // Event size.
    //
    EventSize = *(UINT32 *)((UINT8 *)DigestListBin + DigestListBinSize);
    Event     = (UINT8 *)DigestListBin + DigestListBinSize + sizeof (UINT32);

    //
    // Log the event
    //
    Status = TdxDxeLogEvent (
               LogFormat,
               CcEvent,
               sizeof (UINT32) + sizeof (TCG_EVENTTYPE) + DigestListBinSize + sizeof (UINT32),
               Event,
               EventSize
               );

    DumpCcEvent ((CC_EVENT *)CcEvent);
    FreePool (CcEvent);
  }

  return Status;
}

/**
  Install TDVF ACPI Table when ACPI Table Protocol is available.

  @param[in]  Event     Event whose notification function is being invoked
  @param[in]  Context   Pointer to the notification function's context
**/
VOID
EFIAPI
InstallAcpiTable (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  UINTN                    TableKey;
  EFI_STATUS               Status;
  EFI_ACPI_TABLE_PROTOCOL  *AcpiTable;

  Status = gBS->LocateProtocol (&gEfiAcpiTableProtocolGuid, NULL, (VOID **)&AcpiTable);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "TD: AcpiTableProtocol is not installed. %r\n", Status));
    return;
  }

  mTdxEventlogAcpiTemplate.Laml = (UINT64)PcdGet32 (PcdCcEventlogAcpiTableLaml);
  mTdxEventlogAcpiTemplate.Lasa = PcdGet64 (PcdCcEventlogAcpiTableLasa);
  CopyMem (mTdxEventlogAcpiTemplate.Header.OemId, PcdGetPtr (PcdAcpiDefaultOemId), sizeof (mTdxEventlogAcpiTemplate.Header.OemId));
  mTdxEventlogAcpiTemplate.Header.OemTableId      = PcdGet64 (PcdAcpiDefaultOemTableId);
  mTdxEventlogAcpiTemplate.Header.OemRevision     = PcdGet32 (PcdAcpiDefaultOemRevision);
  mTdxEventlogAcpiTemplate.Header.CreatorId       = PcdGet32 (PcdAcpiDefaultCreatorId);
  mTdxEventlogAcpiTemplate.Header.CreatorRevision = PcdGet32 (PcdAcpiDefaultCreatorRevision);

  //
  // Construct ACPI Table
  Status = AcpiTable->InstallAcpiTable (
                        AcpiTable,
                        &mTdxEventlogAcpiTemplate,
                        mTdxEventlogAcpiTemplate.Header.Length,
                        &TableKey
                        );
  ASSERT_EFI_ERROR (Status);

  DEBUG ((DEBUG_INFO, "TDVF Eventlog ACPI Table is installed.\n"));
}

/**
  The function install TdTcg2 protocol.

  @retval EFI_SUCCESS     TdTcg2 protocol is installed.
  @retval other           Some error occurs.
**/
EFI_STATUS
InstallCcMeasurementProtocol (
  VOID
  )
{
  EFI_STATUS  Status;
  EFI_HANDLE  Handle;

  Handle = NULL;
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &Handle,
                  &gEfiCcMeasurementProtocolGuid,
                  &mTdProtocol,
                  NULL
                  );
  DEBUG ((DEBUG_INFO, "CcProtocol: Install %r\n", Status));
  return Status;
}

/**
  The driver's entry point. It publishes EFI Tcg2 Protocol.

  @param[in] ImageHandle  The firmware allocated handle for the EFI image.
  @param[in] SystemTable  A pointer to the EFI System Table.

  @retval EFI_SUCCESS     The entry point is executed successfully.
  @retval other           Some error occurs when executing this entry point.
**/
EFI_STATUS
EFIAPI
DriverEntry (
  IN    EFI_HANDLE        ImageHandle,
  IN    EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;
  EFI_EVENT   Event;
  VOID        *Registration;

  if (!TdIsEnabled ()) {
    return EFI_UNSUPPORTED;
  }

  mImageHandle = ImageHandle;

  //
  // Fill information
  //
  //  ASSERT (TD_EVENT_LOG_AREA_COUNT_MAX == sizeof(mTEventInfo)/sizeof(mTcg2EventInfo[0]));

  mTdxDxeData.BsCap.Size                   = sizeof (EFI_CC_BOOT_SERVICE_CAPABILITY);
  mTdxDxeData.BsCap.ProtocolVersion.Major  = 1;
  mTdxDxeData.BsCap.ProtocolVersion.Minor  = 0;
  mTdxDxeData.BsCap.StructureVersion.Major = 1;
  mTdxDxeData.BsCap.StructureVersion.Minor = 0;

  //
  // Get supported PCR and current Active PCRs
  // For TD gueset HA384 is supported.
  //
  mTdxDxeData.BsCap.HashAlgorithmBitmap = HASH_ALG_SHA384;

  // TD guest only supports EFI_TCG2_EVENT_LOG_FORMAT_TCG_2
  mTdxDxeData.BsCap.SupportedEventLogs = EFI_CC_EVENT_LOG_FORMAT_TCG_2;

  //
  // Setup the log area and copy event log from hob list to it
  //
  Status = SetupCcEventLog ();
  ASSERT_EFI_ERROR (Status);

  if (!EFI_ERROR (Status)) {
    Status = SyncCcEvent ();
    ASSERT_EFI_ERROR (Status);
  }

  //
  // Measure handoff tables, Boot#### variables etc.
  //
  Status = EfiCreateEventReadyToBootEx (
             TPL_CALLBACK,
             OnReadyToBoot,
             NULL,
             &Event
             );

  Status = gBS->CreateEventEx (
                  EVT_NOTIFY_SIGNAL,
                  TPL_NOTIFY,
                  OnExitBootServices,
                  NULL,
                  &gEfiEventExitBootServicesGuid,
                  &Event
                  );

  //
  // Measure Exit Boot Service failed
  //
  Status = gBS->CreateEventEx (
                  EVT_NOTIFY_SIGNAL,
                  TPL_NOTIFY,
                  OnExitBootServicesFailed,
                  NULL,
                  &gEventExitBootServicesFailedGuid,
                  &Event
                  );

  //
  // Create event callback, because we need access variable on SecureBootPolicyVariable
  // We should use VariableWriteArch instead of VariableArch, because Variable driver
  // may update SecureBoot value based on last setting.
  //
  EfiCreateProtocolNotifyEvent (&gEfiVariableWriteArchProtocolGuid, TPL_CALLBACK, MeasureSecureBootPolicy, NULL, &Registration);

  //
  // Install CcMeasurementProtocol
  //
  Status = InstallCcMeasurementProtocol ();
  DEBUG ((DEBUG_INFO, "InstallCcMeasurementProtocol - %r\n", Status));

  if (Status == EFI_SUCCESS) {
    //
    // Create event callback to install CC EventLog ACPI Table
    EfiCreateProtocolNotifyEvent (&gEfiAcpiTableProtocolGuid, TPL_CALLBACK, InstallAcpiTable, NULL, &Registration);
  } else {
    //
    // Cc measurement feature is crucial to a td-guest and it shall stop running immediately
    // when it is failed to be installed.
    DEBUG ((DEBUG_ERROR, "%a: CcMeasurement protocol failed to be installed - %r\n", __func__, Status));
    CpuDeadLoop ();
  }

  return Status;
}
