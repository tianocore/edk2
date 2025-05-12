/** @file
  This module implements Tcg2 Protocol.

Copyright (c) 2015 - 2024, Intel Corporation. All rights reserved.<BR>
(C) Copyright 2016 Hewlett Packard Enterprise Development LP<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiDxe.h>
#include <IndustryStandard/Acpi.h>
#include <IndustryStandard/PeImage.h>
#include <IndustryStandard/TcpaAcpi.h>

#include <Guid/GlobalVariable.h>
#include <Guid/HobList.h>
#include <Guid/TcgEventHob.h>
#include <Guid/EventGroup.h>
#include <Guid/EventExitBootServiceFailed.h>
#include <Guid/ImageAuthentication.h>
#include <Guid/TpmInstance.h>
#include <Guid/DeviceAuthentication.h>

#include <Protocol/DevicePath.h>
#include <Protocol/MpService.h>
#include <Protocol/VariableWrite.h>
#include <Protocol/Tcg2Protocol.h>
#include <Protocol/TrEEProtocol.h>
#include <Protocol/ResetNotification.h>

#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/HobLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/BaseLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PrintLib.h>
#include <Library/Tpm2CommandLib.h>
#include <Library/PcdLib.h>
#include <Library/UefiLib.h>
#include <Library/Tpm2DeviceLib.h>
#include <Library/HashLib.h>
#include <Library/PerformanceLib.h>
#include <Library/ReportStatusCodeLib.h>
#include <Library/Tcg2PhysicalPresenceLib.h>

#define PERF_ID_TCG2_DXE  0x3120

typedef struct {
  CHAR16      *VariableName;
  EFI_GUID    *VendorGuid;
} VARIABLE_TYPE;

#define  TCG2_DEFAULT_MAX_COMMAND_SIZE   0x1000
#define  TCG2_DEFAULT_MAX_RESPONSE_SIZE  0x1000

typedef struct {
  EFI_GUID                     *EventGuid;
  EFI_TCG2_EVENT_LOG_FORMAT    LogFormat;
} TCG2_EVENT_INFO_STRUCT;

TCG2_EVENT_INFO_STRUCT  mTcg2EventInfo[] = {
  { &gTcgEventEntryHobGuid,  EFI_TCG2_EVENT_LOG_FORMAT_TCG_1_2 },
  { &gTcgEvent2EntryHobGuid, EFI_TCG2_EVENT_LOG_FORMAT_TCG_2   },
};

#define TCG_EVENT_LOG_AREA_COUNT_MAX  2

typedef struct {
  EFI_TCG2_EVENT_LOG_FORMAT    EventLogFormat;
  EFI_PHYSICAL_ADDRESS         Lasa;
  UINT64                       Laml;
  UINTN                        EventLogSize;
  UINT8                        *LastEvent;
  BOOLEAN                      EventLogStarted;
  BOOLEAN                      EventLogTruncated;
  UINTN                        Next800155EventOffset;
} TCG_EVENT_LOG_AREA_STRUCT;

typedef struct _TCG_DXE_DATA {
  EFI_TCG2_BOOT_SERVICE_CAPABILITY    BsCap;
  TCG_EVENT_LOG_AREA_STRUCT           EventLogAreaStruct[TCG_EVENT_LOG_AREA_COUNT_MAX];
  BOOLEAN                             GetEventLogCalled[TCG_EVENT_LOG_AREA_COUNT_MAX];
  TCG_EVENT_LOG_AREA_STRUCT           FinalEventLogAreaStruct[TCG_EVENT_LOG_AREA_COUNT_MAX];
  EFI_TCG2_FINAL_EVENTS_TABLE         *FinalEventsTable[TCG_EVENT_LOG_AREA_COUNT_MAX];
} TCG_DXE_DATA;

TCG_DXE_DATA  mTcgDxeData = {
  {
    sizeof (EFI_TCG2_BOOT_SERVICE_CAPABILITY), // Size
    { 1, 1 },                                  // StructureVersion
    { 1, 1 },                                  // ProtocolVersion
    EFI_TCG2_BOOT_HASH_ALG_SHA1,               // HashAlgorithmBitmap
    EFI_TCG2_EVENT_LOG_FORMAT_TCG_1_2,         // SupportedEventLogs
    TRUE,                                      // TPMPresentFlag
    TCG2_DEFAULT_MAX_COMMAND_SIZE,             // MaxCommandSize
    TCG2_DEFAULT_MAX_RESPONSE_SIZE,            // MaxResponseSize
    0,                                         // ManufacturerID
    0,                                         // NumberOfPCRBanks
    0,                                         // ActivePcrBanks
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

EFI_HANDLE  mImageHandle;

/**
  Measure PE image into TPM log based on the authenticode image hashing in
  PE/COFF Specification 8.0 Appendix A.

  Caution: This function may receive untrusted input.
  PE/COFF image is external input, so this function will validate its data structure
  within this image buffer before use.

  Notes: PE/COFF image is checked by BasePeCoffLib PeCoffLoaderGetImageInfo().

  @param[in]  PCRIndex       TPM PCR index
  @param[in]  ImageAddress   Start address of image buffer.
  @param[in]  ImageSize      Image size
  @param[out] DigestList     Digest list of this image.

  @retval EFI_SUCCESS            Successfully measure image.
  @retval EFI_OUT_OF_RESOURCES   No enough resource to measure image.
  @retval other error value
**/
EFI_STATUS
MeasurePeImageAndExtend (
  IN  UINT32                PCRIndex,
  IN  EFI_PHYSICAL_ADDRESS  ImageAddress,
  IN  UINTN                 ImageSize,
  OUT TPML_DIGEST_VALUES    *DigestList
  );

/**

  This function dumps raw data.

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
    DEBUG ((DEBUG_SECURITY, "%02x", (UINTN)Data[Index]));
  }
}

/**

  This function initialize TCG_PCR_EVENT2_HDR for EV_NO_ACTION Event Type other than EFI Specification ID event
  The behavior is defined by TCG PC Client PFP Spec. Section 9.3.4 EV_NO_ACTION Event Types

  @param[in, out]   NoActionEvent  Event Header of EV_NO_ACTION Event
  @param[in]        EventSize      Event Size of the EV_NO_ACTION Event

**/
VOID
InitNoActionEvent (
  IN OUT TCG_PCR_EVENT2_HDR  *NoActionEvent,
  IN UINT32                  EventSize
  )
{
  UINT32         DigestListCount;
  TPMI_ALG_HASH  HashAlgId;
  UINT8          *DigestBuffer;

  DigestBuffer    = (UINT8 *)NoActionEvent->Digests.digests;
  DigestListCount = 0;

  NoActionEvent->PCRIndex  = 0;
  NoActionEvent->EventType = EV_NO_ACTION;

  //
  // Set Hash count & hashAlg accordingly, while Digest.digests[n].digest to all 0
  //
  ZeroMem (&NoActionEvent->Digests, sizeof (NoActionEvent->Digests));

  if ((mTcgDxeData.BsCap.ActivePcrBanks & EFI_TCG2_BOOT_HASH_ALG_SHA1) != 0) {
    HashAlgId = TPM_ALG_SHA1;
    CopyMem (DigestBuffer, &HashAlgId, sizeof (TPMI_ALG_HASH));
    DigestBuffer += sizeof (TPMI_ALG_HASH) + GetHashSizeFromAlgo (HashAlgId);
    DigestListCount++;
  }

  if ((mTcgDxeData.BsCap.ActivePcrBanks & EFI_TCG2_BOOT_HASH_ALG_SHA256) != 0) {
    HashAlgId = TPM_ALG_SHA256;
    CopyMem (DigestBuffer, &HashAlgId, sizeof (TPMI_ALG_HASH));
    DigestBuffer += sizeof (TPMI_ALG_HASH) + GetHashSizeFromAlgo (HashAlgId);
    DigestListCount++;
  }

  if ((mTcgDxeData.BsCap.ActivePcrBanks & EFI_TCG2_BOOT_HASH_ALG_SHA384) != 0) {
    HashAlgId = TPM_ALG_SHA384;
    CopyMem (DigestBuffer, &HashAlgId, sizeof (TPMI_ALG_HASH));
    DigestBuffer += sizeof (TPMI_ALG_HASH) + GetHashSizeFromAlgo (HashAlgId);
    DigestListCount++;
  }

  if ((mTcgDxeData.BsCap.ActivePcrBanks & EFI_TCG2_BOOT_HASH_ALG_SHA512) != 0) {
    HashAlgId = TPM_ALG_SHA512;
    CopyMem (DigestBuffer, &HashAlgId, sizeof (TPMI_ALG_HASH));
    DigestBuffer += sizeof (TPMI_ALG_HASH) + GetHashSizeFromAlgo (HashAlgId);
    DigestListCount++;
  }

  if ((mTcgDxeData.BsCap.ActivePcrBanks & EFI_TCG2_BOOT_HASH_ALG_SM3_256) != 0) {
    HashAlgId = TPM_ALG_SM3_256;
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

  This function dumps raw data in columns.

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

  #define COLUME_SIZE  (16 * 2)

  Count = Size / COLUME_SIZE;
  Left  = Size % COLUME_SIZE;
  for (Index = 0; Index < Count; Index++) {
    DEBUG ((DEBUG_SECURITY, "%04x: ", Index * COLUME_SIZE));
    InternalDumpData (Data + Index * COLUME_SIZE, COLUME_SIZE);
    DEBUG ((DEBUG_SECURITY, "\n"));
  }

  if (Left != 0) {
    DEBUG ((DEBUG_SECURITY, "%04x: ", Index * COLUME_SIZE));
    InternalDumpData (Data + Index * COLUME_SIZE, Left);
    DEBUG ((DEBUG_SECURITY, "\n"));
  }
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
  The EFI_TCG2_PROTOCOL GetCapability function call provides protocol
  capability information and state information.

  @param[in]      This               Indicates the calling context
  @param[in, out] ProtocolCapability The caller allocates memory for a EFI_TCG2_BOOT_SERVICE_CAPABILITY
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
Tcg2GetCapability (
  IN EFI_TCG2_PROTOCOL                     *This,
  IN OUT EFI_TCG2_BOOT_SERVICE_CAPABILITY  *ProtocolCapability
  )
{
  DEBUG ((DEBUG_VERBOSE, "Tcg2GetCapability ...\n"));

  if ((This == NULL) || (ProtocolCapability == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  DEBUG ((DEBUG_VERBOSE, "Size - 0x%x\n", ProtocolCapability->Size));
  DEBUG ((DEBUG_VERBOSE, " 1.1 - 0x%x, 1.0 - 0x%x\n", sizeof (EFI_TCG2_BOOT_SERVICE_CAPABILITY), sizeof (TREE_BOOT_SERVICE_CAPABILITY_1_0)));

  if (ProtocolCapability->Size < mTcgDxeData.BsCap.Size) {
    //
    // Handle the case that firmware support 1.1 but OS only support 1.0.
    //
    if ((mTcgDxeData.BsCap.ProtocolVersion.Major > 0x01) ||
        ((mTcgDxeData.BsCap.ProtocolVersion.Major == 0x01) && ((mTcgDxeData.BsCap.ProtocolVersion.Minor > 0x00))))
    {
      if (ProtocolCapability->Size >= sizeof (TREE_BOOT_SERVICE_CAPABILITY_1_0)) {
        CopyMem (ProtocolCapability, &mTcgDxeData.BsCap, sizeof (TREE_BOOT_SERVICE_CAPABILITY_1_0));
        ProtocolCapability->Size                   = sizeof (TREE_BOOT_SERVICE_CAPABILITY_1_0);
        ProtocolCapability->StructureVersion.Major = 1;
        ProtocolCapability->StructureVersion.Minor = 0;
        ProtocolCapability->ProtocolVersion.Major  = 1;
        ProtocolCapability->ProtocolVersion.Minor  = 0;
        DEBUG ((DEBUG_ERROR, "TreeGetCapability (Compatible) - %r\n", EFI_SUCCESS));
        return EFI_SUCCESS;
      }
    }

    ProtocolCapability->Size = mTcgDxeData.BsCap.Size;
    return EFI_BUFFER_TOO_SMALL;
  }

  CopyMem (ProtocolCapability, &mTcgDxeData.BsCap, mTcgDxeData.BsCap.Size);
  DEBUG ((DEBUG_VERBOSE, "Tcg2GetCapability - %r\n", EFI_SUCCESS));
  return EFI_SUCCESS;
}

/**
  This function dumps PCR events.

  @param[in]  EventHdr     TCG PCR event structure.
**/
VOID
DumpEvent (
  IN TCG_PCR_EVENT_HDR  *EventHdr
  )
{
  UINTN  Index;

  DEBUG ((DEBUG_SECURITY, "  Event:\n"));
  DEBUG ((DEBUG_SECURITY, "    PCRIndex  - %d\n", EventHdr->PCRIndex));
  DEBUG ((DEBUG_SECURITY, "    EventType - 0x%08x\n", EventHdr->EventType));
  DEBUG ((DEBUG_SECURITY, "    Digest    - "));
  for (Index = 0; Index < sizeof (TCG_DIGEST); Index++) {
    DEBUG ((DEBUG_SECURITY, "%02x ", EventHdr->Digest.digest[Index]));
  }

  DEBUG ((DEBUG_SECURITY, "\n"));
  DEBUG ((DEBUG_SECURITY, "    EventSize - 0x%08x\n", EventHdr->EventSize));
  InternalDumpHex ((UINT8 *)(EventHdr + 1), EventHdr->EventSize);
}

/**
  This function dumps the TCG_EfiSpecIDEventStruct.

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

  DEBUG ((DEBUG_SECURITY, "  TCG_EfiSpecIDEventStruct:\n"));
  DEBUG ((DEBUG_SECURITY, "    signature          - '"));
  for (Index = 0; Index < sizeof (TcgEfiSpecIdEventStruct->signature); Index++) {
    DEBUG ((DEBUG_SECURITY, "%c", TcgEfiSpecIdEventStruct->signature[Index]));
  }

  DEBUG ((DEBUG_SECURITY, "'\n"));
  DEBUG ((DEBUG_SECURITY, "    platformClass      - 0x%08x\n", TcgEfiSpecIdEventStruct->platformClass));
  DEBUG ((DEBUG_SECURITY, "    specVersion        - %d.%d%d\n", TcgEfiSpecIdEventStruct->specVersionMajor, TcgEfiSpecIdEventStruct->specVersionMinor, TcgEfiSpecIdEventStruct->specErrata));
  DEBUG ((DEBUG_SECURITY, "    uintnSize          - 0x%02x\n", TcgEfiSpecIdEventStruct->uintnSize));

  CopyMem (&NumberOfAlgorithms, TcgEfiSpecIdEventStruct + 1, sizeof (NumberOfAlgorithms));
  DEBUG ((DEBUG_SECURITY, "    NumberOfAlgorithms - 0x%08x\n", NumberOfAlgorithms));

  DigestSize = (TCG_EfiSpecIdEventAlgorithmSize *)((UINT8 *)TcgEfiSpecIdEventStruct + sizeof (*TcgEfiSpecIdEventStruct) + sizeof (NumberOfAlgorithms));
  for (Index = 0; Index < NumberOfAlgorithms; Index++) {
    DEBUG ((DEBUG_SECURITY, "    digest(%d)\n", Index));
    DEBUG ((DEBUG_SECURITY, "      algorithmId      - 0x%04x\n", DigestSize[Index].algorithmId));
    DEBUG ((DEBUG_SECURITY, "      digestSize       - 0x%04x\n", DigestSize[Index].digestSize));
  }

  VendorInfoSize = (UINT8 *)&DigestSize[NumberOfAlgorithms];
  DEBUG ((DEBUG_SECURITY, "    VendorInfoSize     - 0x%02x\n", *VendorInfoSize));
  VendorInfo = VendorInfoSize + 1;
  DEBUG ((DEBUG_SECURITY, "    VendorInfo         - "));
  for (Index = 0; Index < *VendorInfoSize; Index++) {
    DEBUG ((DEBUG_SECURITY, "%02x ", VendorInfo[Index]));
  }

  DEBUG ((DEBUG_SECURITY, "\n"));
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
  This function dumps PCR event 2 structures.

  @param[in]  TcgPcrEvent2     TCG PCR event 2 structure.
**/
VOID
DumpEvent2 (
  IN TCG_PCR_EVENT2  *TcgPcrEvent2
  )
{
  UINTN          Index;
  UINT32         DigestIndex;
  UINT32         DigestCount;
  TPMI_ALG_HASH  HashAlgo;
  UINT32         DigestSize;
  UINT8          *DigestBuffer;
  UINT32         EventSize;
  UINT8          *EventBuffer;

  DEBUG ((DEBUG_SECURITY, "  Event:\n"));
  DEBUG ((DEBUG_SECURITY, "    PCRIndex  - %d\n", TcgPcrEvent2->PCRIndex));
  DEBUG ((DEBUG_SECURITY, "    EventType - 0x%08x\n", TcgPcrEvent2->EventType));

  DEBUG ((DEBUG_SECURITY, "    DigestCount: 0x%08x\n", TcgPcrEvent2->Digest.count));

  DigestCount  = TcgPcrEvent2->Digest.count;
  HashAlgo     = TcgPcrEvent2->Digest.digests[0].hashAlg;
  DigestBuffer = (UINT8 *)&TcgPcrEvent2->Digest.digests[0].digest;
  for (DigestIndex = 0; DigestIndex < DigestCount; DigestIndex++) {
    DEBUG ((DEBUG_SECURITY, "      HashAlgo : 0x%04x\n", HashAlgo));
    DEBUG ((DEBUG_SECURITY, "      Digest(%d): ", DigestIndex));
    DigestSize = GetHashSizeFromAlgo (HashAlgo);
    for (Index = 0; Index < DigestSize; Index++) {
      DEBUG ((DEBUG_SECURITY, "%02x ", DigestBuffer[Index]));
    }

    DEBUG ((DEBUG_SECURITY, "\n"));
    //
    // Prepare next
    //
    CopyMem (&HashAlgo, DigestBuffer + DigestSize, sizeof (TPMI_ALG_HASH));
    DigestBuffer = DigestBuffer + DigestSize + sizeof (TPMI_ALG_HASH);
  }

  DEBUG ((DEBUG_SECURITY, "\n"));
  DigestBuffer = DigestBuffer - sizeof (TPMI_ALG_HASH);

  CopyMem (&EventSize, DigestBuffer, sizeof (TcgPcrEvent2->EventSize));
  DEBUG ((DEBUG_SECURITY, "    EventSize - 0x%08x\n", EventSize));
  EventBuffer = DigestBuffer + sizeof (TcgPcrEvent2->EventSize);
  InternalDumpHex (EventBuffer, EventSize);
}

/**
  This function returns size of TCG PCR event 2.

  @param[in]  TcgPcrEvent2     TCG PCR event 2 structure.

  @return size of TCG PCR event 2.
**/
UINTN
GetPcrEvent2Size (
  IN TCG_PCR_EVENT2  *TcgPcrEvent2
  )
{
  UINT32         DigestIndex;
  UINT32         DigestCount;
  TPMI_ALG_HASH  HashAlgo;
  UINT32         DigestSize;
  UINT8          *DigestBuffer;
  UINT32         EventSize;
  UINT8          *EventBuffer;

  DigestCount  = TcgPcrEvent2->Digest.count;
  HashAlgo     = TcgPcrEvent2->Digest.digests[0].hashAlg;
  DigestBuffer = (UINT8 *)&TcgPcrEvent2->Digest.digests[0].digest;
  for (DigestIndex = 0; DigestIndex < DigestCount; DigestIndex++) {
    DigestSize = GetHashSizeFromAlgo (HashAlgo);
    //
    // Prepare next
    //
    CopyMem (&HashAlgo, DigestBuffer + DigestSize, sizeof (TPMI_ALG_HASH));
    DigestBuffer = DigestBuffer + DigestSize + sizeof (TPMI_ALG_HASH);
  }

  DigestBuffer = DigestBuffer - sizeof (TPMI_ALG_HASH);

  CopyMem (&EventSize, DigestBuffer, sizeof (TcgPcrEvent2->EventSize));
  EventBuffer = DigestBuffer + sizeof (TcgPcrEvent2->EventSize);

  return (UINTN)EventBuffer + EventSize - (UINTN)TcgPcrEvent2;
}

/**
  This function dumps the event log.

  @param[in]  EventLogFormat     The type of the event log for which the information is requested.
  @param[in]  EventLogLocation   A pointer to the memory address of the event log.
  @param[in]  EventLogLastEntry  If the Event Log contains more than one entry, this is a pointer to the
                                 address of the start of the last entry in the event log in memory.
  @param[in]  FinalEventsTable   A pointer to the memory address of the final event table.
**/
VOID
DumpEventLog (
  IN EFI_TCG2_EVENT_LOG_FORMAT    EventLogFormat,
  IN EFI_PHYSICAL_ADDRESS         EventLogLocation,
  IN EFI_PHYSICAL_ADDRESS         EventLogLastEntry,
  IN EFI_TCG2_FINAL_EVENTS_TABLE  *FinalEventsTable
  )
{
  TCG_PCR_EVENT_HDR         *EventHdr;
  TCG_PCR_EVENT2            *TcgPcrEvent2;
  TCG_EfiSpecIDEventStruct  *TcgEfiSpecIdEventStruct;
  UINTN                     NumberOfEvents;

  if (!DebugPrintLevelEnabled (DEBUG_SECURITY)) {
    return;
  }

  DEBUG ((DEBUG_SECURITY, "EventLogFormat: (0x%x)\n", EventLogFormat));

  switch (EventLogFormat) {
    case EFI_TCG2_EVENT_LOG_FORMAT_TCG_1_2:
      EventHdr = (TCG_PCR_EVENT_HDR *)(UINTN)EventLogLocation;
      while ((UINTN)EventHdr <= EventLogLastEntry) {
        DumpEvent (EventHdr);
        EventHdr = (TCG_PCR_EVENT_HDR *)((UINTN)EventHdr + sizeof (TCG_PCR_EVENT_HDR) + EventHdr->EventSize);
      }

      if (FinalEventsTable == NULL) {
        DEBUG ((DEBUG_SECURITY, "FinalEventsTable: NOT FOUND\n"));
      } else {
        DEBUG ((DEBUG_SECURITY, "FinalEventsTable:    (0x%x)\n", FinalEventsTable));
        DEBUG ((DEBUG_SECURITY, "  Version:           (0x%x)\n", FinalEventsTable->Version));
        DEBUG ((DEBUG_SECURITY, "  NumberOfEvents:    (0x%x)\n", FinalEventsTable->NumberOfEvents));

        EventHdr = (TCG_PCR_EVENT_HDR *)(UINTN)(FinalEventsTable + 1);
        for (NumberOfEvents = 0; NumberOfEvents < FinalEventsTable->NumberOfEvents; NumberOfEvents++) {
          DumpEvent (EventHdr);
          EventHdr = (TCG_PCR_EVENT_HDR *)((UINTN)EventHdr + sizeof (TCG_PCR_EVENT_HDR) + EventHdr->EventSize);
        }
      }

      break;
    case EFI_TCG2_EVENT_LOG_FORMAT_TCG_2:
      //
      // Dump first event
      //
      EventHdr = (TCG_PCR_EVENT_HDR *)(UINTN)EventLogLocation;
      DumpEvent (EventHdr);

      TcgEfiSpecIdEventStruct = (TCG_EfiSpecIDEventStruct *)(EventHdr + 1);
      DumpTcgEfiSpecIdEventStruct (TcgEfiSpecIdEventStruct);

      TcgPcrEvent2 = (TCG_PCR_EVENT2 *)((UINTN)TcgEfiSpecIdEventStruct + GetTcgEfiSpecIdEventStructSize (TcgEfiSpecIdEventStruct));
      while ((UINTN)TcgPcrEvent2 <= EventLogLastEntry) {
        DumpEvent2 (TcgPcrEvent2);
        TcgPcrEvent2 = (TCG_PCR_EVENT2 *)((UINTN)TcgPcrEvent2 + GetPcrEvent2Size (TcgPcrEvent2));
      }

      if (FinalEventsTable == NULL) {
        DEBUG ((DEBUG_SECURITY, "FinalEventsTable: NOT FOUND\n"));
      } else {
        DEBUG ((DEBUG_SECURITY, "FinalEventsTable:    (0x%x)\n", FinalEventsTable));
        DEBUG ((DEBUG_SECURITY, "  Version:           (0x%x)\n", FinalEventsTable->Version));
        DEBUG ((DEBUG_SECURITY, "  NumberOfEvents:    (0x%x)\n", FinalEventsTable->NumberOfEvents));

        TcgPcrEvent2 = (TCG_PCR_EVENT2 *)(UINTN)(FinalEventsTable + 1);
        for (NumberOfEvents = 0; NumberOfEvents < FinalEventsTable->NumberOfEvents; NumberOfEvents++) {
          DumpEvent2 (TcgPcrEvent2);
          TcgPcrEvent2 = (TCG_PCR_EVENT2 *)((UINTN)TcgPcrEvent2 + GetPcrEvent2Size (TcgPcrEvent2));
        }
      }

      break;
  }

  return;
}

/**
  The EFI_TCG2_PROTOCOL Get Event Log function call allows a caller to
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
Tcg2GetEventLog (
  IN EFI_TCG2_PROTOCOL          *This,
  IN EFI_TCG2_EVENT_LOG_FORMAT  EventLogFormat,
  OUT EFI_PHYSICAL_ADDRESS      *EventLogLocation,
  OUT EFI_PHYSICAL_ADDRESS      *EventLogLastEntry,
  OUT BOOLEAN                   *EventLogTruncated
  )
{
  UINTN  Index;

  if (This == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  for (Index = 0; Index < sizeof (mTcg2EventInfo)/sizeof (mTcg2EventInfo[0]); Index++) {
    if (EventLogFormat == mTcg2EventInfo[Index].LogFormat) {
      break;
    }
  }

  if (Index == sizeof (mTcg2EventInfo)/sizeof (mTcg2EventInfo[0])) {
    return EFI_INVALID_PARAMETER;
  }

  if ((mTcg2EventInfo[Index].LogFormat & mTcgDxeData.BsCap.SupportedEventLogs) == 0) {
    return EFI_INVALID_PARAMETER;
  }

  if (!mTcgDxeData.BsCap.TPMPresentFlag) {
    if (EventLogLocation != NULL) {
      *EventLogLocation = 0;
    }

    if (EventLogLastEntry != NULL) {
      *EventLogLastEntry = 0;
    }

    if (EventLogTruncated != NULL) {
      *EventLogTruncated = FALSE;
    }

    return EFI_SUCCESS;
  }

  if (EventLogLocation != NULL) {
    *EventLogLocation = mTcgDxeData.EventLogAreaStruct[Index].Lasa;
  }

  if (EventLogLastEntry != NULL) {
    if (!mTcgDxeData.EventLogAreaStruct[Index].EventLogStarted) {
      *EventLogLastEntry = (EFI_PHYSICAL_ADDRESS)(UINTN)0;
    } else {
      *EventLogLastEntry = (EFI_PHYSICAL_ADDRESS)(UINTN)mTcgDxeData.EventLogAreaStruct[Index].LastEvent;
    }
  }

  if (EventLogTruncated != NULL) {
    *EventLogTruncated = mTcgDxeData.EventLogAreaStruct[Index].EventLogTruncated;
  }

  // Dump Event Log for debug purpose
  if ((EventLogLocation != NULL) && (EventLogLastEntry != NULL)) {
    DumpEventLog (EventLogFormat, *EventLogLocation, *EventLogLastEntry, mTcgDxeData.FinalEventsTable[Index]);
  }

  //
  // All events generated after the invocation of EFI_TCG2_GET_EVENT_LOG SHALL be stored
  // in an instance of an EFI_CONFIGURATION_TABLE named by the VendorGuid of EFI_TCG2_FINAL_EVENTS_TABLE_GUID.
  //
  mTcgDxeData.GetEventLogCalled[Index] = TRUE;

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
STATIC
BOOLEAN
Is800155Event (
  IN      VOID    *NewEventHdr,
  IN      UINT32  NewEventHdrSize,
  IN      UINT8   *NewEventData,
  IN      UINT32  NewEventSize
  )
{
  if (((TCG_PCR_EVENT2_HDR *)NewEventHdr)->EventType != EV_NO_ACTION) {
    return FALSE;
  }

  if ((NewEventSize >= sizeof (TCG_Sp800_155_PlatformId_Event2)) &&
      (CompareMem (
         NewEventData,
         TCG_Sp800_155_PlatformId_Event2_SIGNATURE,
         sizeof (TCG_Sp800_155_PlatformId_Event2_SIGNATURE) - 1
         ) == 0))
  {
    return TRUE;
  }

  if ((NewEventSize >= sizeof (TCG_Sp800_155_PlatformId_Event3)) &&
      (CompareMem (
         NewEventData,
         TCG_Sp800_155_PlatformId_Event3_SIGNATURE,
         sizeof (TCG_Sp800_155_PlatformId_Event3_SIGNATURE) - 1
         ) == 0))
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
  IN OUT  TCG_EVENT_LOG_AREA_STRUCT  *EventLogAreaStruct,
  IN      VOID                       *NewEventHdr,
  IN      UINT32                     NewEventHdrSize,
  IN      UINT8                      *NewEventData,
  IN      UINT32                     NewEventSize
  )
{
  UINTN    NewLogSize;
  BOOLEAN  Record800155Event;

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
TcgDxeLogEvent (
  IN      EFI_TCG2_EVENT_LOG_FORMAT  EventLogFormat,
  IN      VOID                       *NewEventHdr,
  IN      UINT32                     NewEventHdrSize,
  IN      UINT8                      *NewEventData,
  IN      UINT32                     NewEventSize
  )
{
  EFI_STATUS                 Status;
  UINTN                      Index;
  TCG_EVENT_LOG_AREA_STRUCT  *EventLogAreaStruct;

  for (Index = 0; Index < sizeof (mTcg2EventInfo)/sizeof (mTcg2EventInfo[0]); Index++) {
    if (EventLogFormat == mTcg2EventInfo[Index].LogFormat) {
      break;
    }
  }

  if (Index == sizeof (mTcg2EventInfo)/sizeof (mTcg2EventInfo[0])) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Record to normal event log
  //
  EventLogAreaStruct = &mTcgDxeData.EventLogAreaStruct[Index];

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
  if (mTcgDxeData.GetEventLogCalled[Index]) {
    if (mTcgDxeData.FinalEventsTable[Index] == NULL) {
      //
      // no need for FinalEventsTable
      //
      return EFI_SUCCESS;
    }

    EventLogAreaStruct = &mTcgDxeData.FinalEventLogAreaStruct[Index];

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
      (mTcgDxeData.FinalEventsTable[Index])->NumberOfEvents++;
      DEBUG ((DEBUG_INFO, "FinalEventsTable->NumberOfEvents - 0x%x\n", (mTcgDxeData.FinalEventsTable[Index])->NumberOfEvents));
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

  DigestListCountPtr         = (UINT32 *)Buffer;
  DigestListCount            = 0;
  (*HashAlgorithmMaskCopied) = 0;

  Count         = ReadUnaligned32 (DigestListBin);
  Buffer        = (UINT8 *)Buffer + sizeof (Count);
  DigestListBin = (UINT8 *)DigestListBin + sizeof (Count);
  for (Index = 0; Index < Count; Index++) {
    HashAlg       = ReadUnaligned16 (DigestListBin);
    DigestListBin = (UINT8 *)DigestListBin + sizeof (HashAlg);
    DigestSize    = GetHashSizeFromAlgo (HashAlg);

    if (IsHashAlgSupportedInHashAlgorithmMask (HashAlg, HashAlgorithmMask)) {
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
  Add a new entry to the Event Log.

  @param[in]     DigestList    A list of digest.
  @param[in,out] NewEventHdr   Pointer to a TCG_PCR_EVENT_HDR data structure.
  @param[in]     NewEventData  Pointer to the new event data.

  @retval EFI_SUCCESS           The new event log entry was added.
  @retval EFI_OUT_OF_RESOURCES  No enough memory to log the new event.
**/
EFI_STATUS
TcgDxeLogHashEvent (
  IN TPML_DIGEST_VALUES      *DigestList,
  IN OUT  TCG_PCR_EVENT_HDR  *NewEventHdr,
  IN      UINT8              *NewEventData
  )
{
  EFI_STATUS      Status;
  EFI_TPL         OldTpl;
  UINTN           Index;
  EFI_STATUS      RetStatus;
  TCG_PCR_EVENT2  TcgPcrEvent2;
  UINT8           *DigestBuffer;
  UINT32          *EventSizePtr;

  RetStatus = EFI_SUCCESS;
  for (Index = 0; Index < sizeof (mTcg2EventInfo)/sizeof (mTcg2EventInfo[0]); Index++) {
    if ((mTcgDxeData.BsCap.SupportedEventLogs & mTcg2EventInfo[Index].LogFormat) != 0) {
      switch (mTcg2EventInfo[Index].LogFormat) {
        case EFI_TCG2_EVENT_LOG_FORMAT_TCG_1_2:
          Status = GetDigestFromDigestList (TPM_ALG_SHA1, DigestList, &NewEventHdr->Digest);
          if (!EFI_ERROR (Status)) {
            //
            // Enter critical region
            //
            OldTpl = gBS->RaiseTPL (TPL_HIGH_LEVEL);
            Status = TcgDxeLogEvent (
                       mTcg2EventInfo[Index].LogFormat,
                       NewEventHdr,
                       sizeof (TCG_PCR_EVENT_HDR),
                       NewEventData,
                       NewEventHdr->EventSize
                       );
            if (Status != EFI_SUCCESS) {
              RetStatus = Status;
            }

            gBS->RestoreTPL (OldTpl);
            //
            // Exit critical region
            //
          }

          break;
        case EFI_TCG2_EVENT_LOG_FORMAT_TCG_2:
          ZeroMem (&TcgPcrEvent2, sizeof (TcgPcrEvent2));
          TcgPcrEvent2.PCRIndex  = NewEventHdr->PCRIndex;
          TcgPcrEvent2.EventType = NewEventHdr->EventType;
          DigestBuffer           = (UINT8 *)&TcgPcrEvent2.Digest;
          EventSizePtr           = CopyDigestListToBuffer (DigestBuffer, DigestList, mTcgDxeData.BsCap.ActivePcrBanks);
          CopyMem (EventSizePtr, &NewEventHdr->EventSize, sizeof (NewEventHdr->EventSize));

          //
          // Enter critical region
          //
          OldTpl = gBS->RaiseTPL (TPL_HIGH_LEVEL);
          Status = TcgDxeLogEvent (
                     mTcg2EventInfo[Index].LogFormat,
                     &TcgPcrEvent2,
                     sizeof (TcgPcrEvent2.PCRIndex) + sizeof (TcgPcrEvent2.EventType) + GetDigestListBinSize (DigestBuffer) + sizeof (TcgPcrEvent2.EventSize),
                     NewEventData,
                     NewEventHdr->EventSize
                     );
          if (Status != EFI_SUCCESS) {
            RetStatus = Status;
          }

          gBS->RestoreTPL (OldTpl);
          //
          // Exit critical region
          //
          break;
      }
    }
  }

  return RetStatus;
}

/**
  Do a hash operation on a data buffer, extend a specific TPM PCR with the hash result,
  and add an entry to the Event Log.

  @param[in]      Flags         Bitmap providing additional information.
  @param[in]      HashData      Physical address of the start of the data buffer
                                to be hashed, extended, and logged.
  @param[in]      HashDataLen   The length, in bytes, of the buffer referenced by HashData
  @param[in, out] NewEventHdr   Pointer to a TCG_PCR_EVENT_HDR data structure.
  @param[in]      NewEventData  Pointer to the new event data.

  @retval EFI_SUCCESS           Operation completed successfully.
  @retval EFI_OUT_OF_RESOURCES  No enough memory to log the new event.
  @retval EFI_DEVICE_ERROR      The command was unsuccessful.

**/
EFI_STATUS
TcgDxeHashLogExtendEvent (
  IN      UINT64             Flags,
  IN      UINT8              *HashData,
  IN      UINT64             HashDataLen,
  IN OUT  TCG_PCR_EVENT_HDR  *NewEventHdr,
  IN      UINT8              *NewEventData
  )
{
  EFI_STATUS          Status;
  TPML_DIGEST_VALUES  DigestList;
  TCG_PCR_EVENT2_HDR  NoActionEvent;

  if (!mTcgDxeData.BsCap.TPMPresentFlag) {
    return EFI_DEVICE_ERROR;
  }

  if (NewEventHdr->EventType == EV_NO_ACTION) {
    //
    // Do not do TPM extend for EV_NO_ACTION
    //
    if (NewEventHdr->PCRIndex <= MAX_PCR_INDEX) {
      Status = EFI_SUCCESS;
      InitNoActionEvent (&NoActionEvent, NewEventHdr->EventSize);
      if ((Flags & EFI_TCG2_EXTEND_ONLY) == 0) {
        Status = TcgDxeLogHashEvent (&(NoActionEvent.Digests), NewEventHdr, NewEventData);
      }
    } else {
      //
      // Extend to NvIndex
      //
      Status = HashAndExtend (
                 NewEventHdr->PCRIndex,
                 HashData,
                 (UINTN)HashDataLen,
                 &DigestList
                 );
      if (!EFI_ERROR (Status)) {
        Status = TcgDxeLogHashEvent (&DigestList, NewEventHdr, NewEventData);
      }
    }

    return Status;
  }

  Status = HashAndExtend (
             NewEventHdr->PCRIndex,
             HashData,
             (UINTN)HashDataLen,
             &DigestList
             );
  if (!EFI_ERROR (Status)) {
    if ((Flags & EFI_TCG2_EXTEND_ONLY) == 0) {
      Status = TcgDxeLogHashEvent (&DigestList, NewEventHdr, NewEventData);
    }
  }

  if (Status == EFI_DEVICE_ERROR) {
    DEBUG ((DEBUG_ERROR, "TcgDxeHashLogExtendEvent - %r. Disable TPM.\n", Status));
    mTcgDxeData.BsCap.TPMPresentFlag = FALSE;
    REPORT_STATUS_CODE (
      EFI_ERROR_CODE | EFI_ERROR_MINOR,
      (PcdGet32 (PcdStatusCodeSubClassTpmDevice) | EFI_P_EC_INTERFACE_ERROR)
      );
  }

  return Status;
}

/**
  The EFI_TCG2_PROTOCOL HashLogExtendEvent function call provides callers with
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
Tcg2HashLogExtendEvent (
  IN EFI_TCG2_PROTOCOL     *This,
  IN UINT64                Flags,
  IN EFI_PHYSICAL_ADDRESS  DataToHash,
  IN UINT64                DataToHashLen,
  IN EFI_TCG2_EVENT        *Event
  )
{
  EFI_STATUS          Status;
  TCG_PCR_EVENT_HDR   NewEventHdr;
  TPML_DIGEST_VALUES  DigestList;

  DEBUG ((DEBUG_VERBOSE, "Tcg2HashLogExtendEvent ...\n"));

  if ((This == NULL) || (Event == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Do not check hash data size for EV_NO_ACTION event.
  //
  if ((Event->Header.EventType != EV_NO_ACTION) && (DataToHash == 0)) {
    return EFI_INVALID_PARAMETER;
  }

  if (!mTcgDxeData.BsCap.TPMPresentFlag) {
    return EFI_DEVICE_ERROR;
  }

  if (Event->Size < Event->Header.HeaderSize + sizeof (UINT32)) {
    return EFI_INVALID_PARAMETER;
  }

  if ((Event->Header.EventType != EV_NO_ACTION) && (Event->Header.PCRIndex > MAX_PCR_INDEX)) {
    return EFI_INVALID_PARAMETER;
  }

  NewEventHdr.PCRIndex  = Event->Header.PCRIndex;
  NewEventHdr.EventType = Event->Header.EventType;
  NewEventHdr.EventSize = Event->Size - sizeof (UINT32) - Event->Header.HeaderSize;
  if ((Flags & PE_COFF_IMAGE) != 0) {
    Status = MeasurePeImageAndExtend (
               NewEventHdr.PCRIndex,
               DataToHash,
               (UINTN)DataToHashLen,
               &DigestList
               );
    if (!EFI_ERROR (Status)) {
      if ((Flags & EFI_TCG2_EXTEND_ONLY) == 0) {
        Status = TcgDxeLogHashEvent (&DigestList, &NewEventHdr, Event->Event);
      }
    }

    if (Status == EFI_DEVICE_ERROR) {
      DEBUG ((DEBUG_ERROR, "MeasurePeImageAndExtend - %r. Disable TPM.\n", Status));
      mTcgDxeData.BsCap.TPMPresentFlag = FALSE;
      REPORT_STATUS_CODE (
        EFI_ERROR_CODE | EFI_ERROR_MINOR,
        (PcdGet32 (PcdStatusCodeSubClassTpmDevice) | EFI_P_EC_INTERFACE_ERROR)
        );
    }
  } else {
    Status = TcgDxeHashLogExtendEvent (
               Flags,
               (UINT8 *)(UINTN)DataToHash,
               DataToHashLen,
               &NewEventHdr,
               Event->Event
               );
  }

  DEBUG ((DEBUG_VERBOSE, "Tcg2HashLogExtendEvent - %r\n", Status));
  return Status;
}

/**
  This service enables the sending of commands to the TPM.

  @param[in]  This                     Indicates the calling context
  @param[in]  InputParameterBlockSize  Size of the TPM input parameter block.
  @param[in]  InputParameterBlock      Pointer to the TPM input parameter block.
  @param[in]  OutputParameterBlockSize Size of the TPM output parameter block.
  @param[in]  OutputParameterBlock     Pointer to the TPM output parameter block.

  @retval EFI_SUCCESS            The command byte stream was successfully sent to the device and a response was successfully received.
  @retval EFI_DEVICE_ERROR       The command was not successfully sent to the device or a response was not successfully received from the device.
  @retval EFI_INVALID_PARAMETER  One or more of the parameters are incorrect.
  @retval EFI_BUFFER_TOO_SMALL   The output parameter block is too small.
**/
EFI_STATUS
EFIAPI
Tcg2SubmitCommand (
  IN EFI_TCG2_PROTOCOL  *This,
  IN UINT32             InputParameterBlockSize,
  IN UINT8              *InputParameterBlock,
  IN UINT32             OutputParameterBlockSize,
  IN UINT8              *OutputParameterBlock
  )
{
  EFI_STATUS  Status;

  if ((This == NULL) ||
      (InputParameterBlockSize == 0) || (InputParameterBlock == NULL) ||
      (OutputParameterBlockSize == 0) || (OutputParameterBlock == NULL))
  {
    return EFI_INVALID_PARAMETER;
  }

  if (!mTcgDxeData.BsCap.TPMPresentFlag) {
    return EFI_DEVICE_ERROR;
  }

  if (InputParameterBlockSize > mTcgDxeData.BsCap.MaxCommandSize) {
    return EFI_INVALID_PARAMETER;
  }

  if (OutputParameterBlockSize > mTcgDxeData.BsCap.MaxResponseSize) {
    return EFI_INVALID_PARAMETER;
  }

  Status = Tpm2SubmitCommand (
             InputParameterBlockSize,
             InputParameterBlock,
             &OutputParameterBlockSize,
             OutputParameterBlock
             );
  return Status;
}

/**
  This service returns the currently active PCR banks.

  @param[in]  This            Indicates the calling context
  @param[out] ActivePcrBanks  Pointer to the variable receiving the bitmap of currently active PCR banks.

  @retval EFI_SUCCESS           The bitmap of active PCR banks was stored in the ActivePcrBanks parameter.
  @retval EFI_INVALID_PARAMETER One or more of the parameters are incorrect.
**/
EFI_STATUS
EFIAPI
Tcg2GetActivePCRBanks (
  IN  EFI_TCG2_PROTOCOL  *This,
  OUT UINT32             *ActivePcrBanks
  )
{
  if (ActivePcrBanks == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  *ActivePcrBanks = mTcgDxeData.BsCap.ActivePcrBanks;
  return EFI_SUCCESS;
}

/**
  This service sets the currently active PCR banks.

  @param[in]  This            Indicates the calling context
  @param[in]  ActivePcrBanks  Bitmap of the requested active PCR banks. At least one bit SHALL be set.

  @retval EFI_SUCCESS           The bitmap in ActivePcrBank parameter is already active.
  @retval EFI_INVALID_PARAMETER One or more of the parameters are incorrect.
**/
EFI_STATUS
EFIAPI
Tcg2SetActivePCRBanks (
  IN EFI_TCG2_PROTOCOL  *This,
  IN UINT32             ActivePcrBanks
  )
{
  EFI_STATUS  Status;
  UINT32      ReturnCode;

  DEBUG ((DEBUG_INFO, "Tcg2SetActivePCRBanks ... (0x%x)\n", ActivePcrBanks));

  if (ActivePcrBanks == 0) {
    return EFI_INVALID_PARAMETER;
  }

  if ((ActivePcrBanks & (~mTcgDxeData.BsCap.HashAlgorithmBitmap)) != 0) {
    return EFI_INVALID_PARAMETER;
  }

  if (ActivePcrBanks == mTcgDxeData.BsCap.ActivePcrBanks) {
    //
    // Need clear previous SET_PCR_BANKS setting
    //
    ReturnCode = Tcg2PhysicalPresenceLibSubmitRequestToPreOSFunction (TCG2_PHYSICAL_PRESENCE_NO_ACTION, 0);
  } else {
    ReturnCode = Tcg2PhysicalPresenceLibSubmitRequestToPreOSFunction (TCG2_PHYSICAL_PRESENCE_SET_PCR_BANKS, ActivePcrBanks);
  }

  if (ReturnCode == TCG_PP_SUBMIT_REQUEST_TO_PREOS_SUCCESS) {
    Status = EFI_SUCCESS;
  } else if (ReturnCode == TCG_PP_SUBMIT_REQUEST_TO_PREOS_GENERAL_FAILURE) {
    Status = EFI_OUT_OF_RESOURCES;
  } else if (ReturnCode == TCG_PP_SUBMIT_REQUEST_TO_PREOS_NOT_IMPLEMENTED) {
    Status = EFI_UNSUPPORTED;
  } else {
    Status = EFI_DEVICE_ERROR;
  }

  DEBUG ((DEBUG_INFO, "Tcg2SetActivePCRBanks - %r\n", Status));

  return Status;
}

/**
  This service retrieves the result of a previous invocation of SetActivePcrBanks.

  @param[in]  This              Indicates the calling context
  @param[out] OperationPresent  Non-zero value to indicate a SetActivePcrBank operation was invoked during the last boot.
  @param[out] Response          The response from the SetActivePcrBank request.

  @retval EFI_SUCCESS           The result value could be returned.
  @retval EFI_INVALID_PARAMETER One or more of the parameters are incorrect.
**/
EFI_STATUS
EFIAPI
Tcg2GetResultOfSetActivePcrBanks (
  IN  EFI_TCG2_PROTOCOL  *This,
  OUT UINT32             *OperationPresent,
  OUT UINT32             *Response
  )
{
  UINT32  ReturnCode;

  if ((OperationPresent == NULL) || (Response == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  ReturnCode = Tcg2PhysicalPresenceLibReturnOperationResponseToOsFunction (OperationPresent, Response);
  if (ReturnCode == TCG_PP_RETURN_TPM_OPERATION_RESPONSE_SUCCESS) {
    return EFI_SUCCESS;
  } else {
    return EFI_UNSUPPORTED;
  }
}

EFI_TCG2_PROTOCOL  mTcg2Protocol = {
  Tcg2GetCapability,
  Tcg2GetEventLog,
  Tcg2HashLogExtendEvent,
  Tcg2SubmitCommand,
  Tcg2GetActivePCRBanks,
  Tcg2SetActivePCRBanks,
  Tcg2GetResultOfSetActivePcrBanks,
};

/**
  Initialize the Event Log and log events passed from the PEI phase.

  @retval EFI_SUCCESS           Operation completed successfully.
  @retval EFI_OUT_OF_RESOURCES  Out of memory.

**/
EFI_STATUS
SetupEventLog (
  VOID
  )
{
  EFI_STATUS                       Status;
  VOID                             *TcgEvent;
  EFI_PEI_HOB_POINTERS             GuidHob;
  EFI_PHYSICAL_ADDRESS             Lasa;
  UINTN                            Index;
  VOID                             *DigestListBin;
  TPML_DIGEST_VALUES               TempDigestListBin;
  UINT32                           DigestListBinSize;
  UINT8                            *Event;
  UINT32                           EventSize;
  UINT32                           *EventSizePtr;
  UINT32                           HashAlgorithmMaskCopied;
  TCG_EfiSpecIDEventStruct         *TcgEfiSpecIdEventStruct;
  UINT8                            TempBuf[sizeof (TCG_EfiSpecIDEventStruct) + sizeof (UINT32) + (HASH_COUNT * sizeof (TCG_EfiSpecIdEventAlgorithmSize)) + sizeof (UINT8)];
  TCG_PCR_EVENT_HDR                SpecIdEvent;
  TCG_PCR_EVENT2_HDR               NoActionEvent;
  TCG_EfiSpecIdEventAlgorithmSize  *DigestSize;
  TCG_EfiSpecIdEventAlgorithmSize  *TempDigestSize;
  UINT8                            *VendorInfoSize;
  UINT32                           NumberOfAlgorithms;
  TCG_EfiStartupLocalityEvent      StartupLocalityEvent;

  DEBUG ((DEBUG_INFO, "SetupEventLog\n"));

  //
  // 1. Create Log Area
  //
  for (Index = 0; Index < sizeof (mTcg2EventInfo)/sizeof (mTcg2EventInfo[0]); Index++) {
    if ((mTcgDxeData.BsCap.SupportedEventLogs & mTcg2EventInfo[Index].LogFormat) != 0) {
      mTcgDxeData.EventLogAreaStruct[Index].EventLogFormat = mTcg2EventInfo[Index].LogFormat;
      if (PcdGet8 (PcdTpm2AcpiTableRev) >= 4) {
        Status = gBS->AllocatePages (
                        AllocateAnyPages,
                        EfiACPIMemoryNVS,
                        EFI_SIZE_TO_PAGES (PcdGet32 (PcdTcgLogAreaMinLen)),
                        &Lasa
                        );
      } else {
        Status = gBS->AllocatePages (
                        AllocateAnyPages,
                        EfiBootServicesData,
                        EFI_SIZE_TO_PAGES (PcdGet32 (PcdTcgLogAreaMinLen)),
                        &Lasa
                        );
      }

      if (EFI_ERROR (Status)) {
        return Status;
      }

      mTcgDxeData.EventLogAreaStruct[Index].Lasa                  = Lasa;
      mTcgDxeData.EventLogAreaStruct[Index].Laml                  = PcdGet32 (PcdTcgLogAreaMinLen);
      mTcgDxeData.EventLogAreaStruct[Index].Next800155EventOffset = 0;

      if ((PcdGet8 (PcdTpm2AcpiTableRev) >= 4) ||
          (mTcg2EventInfo[Index].LogFormat == EFI_TCG2_EVENT_LOG_FORMAT_TCG_2))
      {
        //
        // Report TCG2 event log address and length, so that they can be reported in TPM2 ACPI table.
        // Ignore the return status, because those fields are optional.
        //
        PcdSet32S (PcdTpm2AcpiTableLaml, (UINT32)mTcgDxeData.EventLogAreaStruct[Index].Laml);
        PcdSet64S (PcdTpm2AcpiTableLasa, mTcgDxeData.EventLogAreaStruct[Index].Lasa);
      }

      //
      // To initialize them as 0xFF is recommended
      // because the OS can know the last entry for that.
      //
      SetMem ((VOID *)(UINTN)Lasa, PcdGet32 (PcdTcgLogAreaMinLen), 0xFF);
      //
      // Create first entry for Log Header Entry Data
      //
      if (mTcg2EventInfo[Index].LogFormat != EFI_TCG2_EVENT_LOG_FORMAT_TCG_1_2) {
        //
        // TcgEfiSpecIdEventStruct
        //
        TcgEfiSpecIdEventStruct = (TCG_EfiSpecIDEventStruct *)TempBuf;
        CopyMem (TcgEfiSpecIdEventStruct->signature, TCG_EfiSpecIDEventStruct_SIGNATURE_03, sizeof (TcgEfiSpecIdEventStruct->signature));
        TcgEfiSpecIdEventStruct->platformClass    = PcdGet8 (PcdTpmPlatformClass);
        TcgEfiSpecIdEventStruct->specVersionMajor = TCG_EfiSpecIDEventStruct_SPEC_VERSION_MAJOR_TPM2;
        TcgEfiSpecIdEventStruct->specVersionMinor = TCG_EfiSpecIDEventStruct_SPEC_VERSION_MINOR_TPM2;
        TcgEfiSpecIdEventStruct->specErrata       = (UINT8)PcdGet32 (PcdTcgPfpMeasurementRevision);
        TcgEfiSpecIdEventStruct->uintnSize        = sizeof (UINTN)/sizeof (UINT32);
        NumberOfAlgorithms                        = 0;
        DigestSize                                = (TCG_EfiSpecIdEventAlgorithmSize *)((UINT8 *)TcgEfiSpecIdEventStruct + sizeof (*TcgEfiSpecIdEventStruct) + sizeof (NumberOfAlgorithms));
        if ((mTcgDxeData.BsCap.ActivePcrBanks & EFI_TCG2_BOOT_HASH_ALG_SHA1) != 0) {
          TempDigestSize              = DigestSize;
          TempDigestSize             += NumberOfAlgorithms;
          TempDigestSize->algorithmId = TPM_ALG_SHA1;
          TempDigestSize->digestSize  = SHA1_DIGEST_SIZE;
          NumberOfAlgorithms++;
        }

        if ((mTcgDxeData.BsCap.ActivePcrBanks & EFI_TCG2_BOOT_HASH_ALG_SHA256) != 0) {
          TempDigestSize              = DigestSize;
          TempDigestSize             += NumberOfAlgorithms;
          TempDigestSize->algorithmId = TPM_ALG_SHA256;
          TempDigestSize->digestSize  = SHA256_DIGEST_SIZE;
          NumberOfAlgorithms++;
        }

        if ((mTcgDxeData.BsCap.ActivePcrBanks & EFI_TCG2_BOOT_HASH_ALG_SHA384) != 0) {
          TempDigestSize              = DigestSize;
          TempDigestSize             += NumberOfAlgorithms;
          TempDigestSize->algorithmId = TPM_ALG_SHA384;
          TempDigestSize->digestSize  = SHA384_DIGEST_SIZE;
          NumberOfAlgorithms++;
        }

        if ((mTcgDxeData.BsCap.ActivePcrBanks & EFI_TCG2_BOOT_HASH_ALG_SHA512) != 0) {
          TempDigestSize              = DigestSize;
          TempDigestSize             += NumberOfAlgorithms;
          TempDigestSize->algorithmId = TPM_ALG_SHA512;
          TempDigestSize->digestSize  = SHA512_DIGEST_SIZE;
          NumberOfAlgorithms++;
        }

        if ((mTcgDxeData.BsCap.ActivePcrBanks & EFI_TCG2_BOOT_HASH_ALG_SM3_256) != 0) {
          TempDigestSize              = DigestSize;
          TempDigestSize             += NumberOfAlgorithms;
          TempDigestSize->algorithmId = TPM_ALG_SM3_256;
          TempDigestSize->digestSize  = SM3_256_DIGEST_SIZE;
          NumberOfAlgorithms++;
        }

        CopyMem (TcgEfiSpecIdEventStruct + 1, &NumberOfAlgorithms, sizeof (NumberOfAlgorithms));
        TempDigestSize  = DigestSize;
        TempDigestSize += NumberOfAlgorithms;
        VendorInfoSize  = (UINT8 *)TempDigestSize;
        *VendorInfoSize = 0;

        SpecIdEvent.PCRIndex  = 0;
        SpecIdEvent.EventType = EV_NO_ACTION;
        ZeroMem (&SpecIdEvent.Digest, sizeof (SpecIdEvent.Digest));
        SpecIdEvent.EventSize = (UINT32)GetTcgEfiSpecIdEventStructSize (TcgEfiSpecIdEventStruct);

        //
        // Log TcgEfiSpecIdEventStruct as the first Event. Event format is TCG_PCR_EVENT.
        //   TCG EFI Protocol Spec. Section 5.3 Event Log Header
        //   TCG PC Client PFP spec. Section 9.2 Measurement Event Entries and Log
        //
        Status = TcgDxeLogEvent (
                   mTcg2EventInfo[Index].LogFormat,
                   &SpecIdEvent,
                   sizeof (SpecIdEvent),
                   (UINT8 *)TcgEfiSpecIdEventStruct,
                   SpecIdEvent.EventSize
                   );
        //
        // record the offset at the end of 800-155 event.
        // the future 800-155 event can be inserted here.
        //
        mTcgDxeData.EventLogAreaStruct[Index].Next800155EventOffset = \
          mTcgDxeData.EventLogAreaStruct[Index].EventLogSize;

        //
        // Tcg800155PlatformIdEvent. Event format is TCG_PCR_EVENT2
        //
        GuidHob.Guid = GetFirstGuidHob (&gTcg800155PlatformIdEventHobGuid);
        while (GuidHob.Guid != NULL) {
          InitNoActionEvent (&NoActionEvent, GET_GUID_HOB_DATA_SIZE (GuidHob.Guid));

          Status = TcgDxeLogEvent (
                     mTcg2EventInfo[Index].LogFormat,
                     &NoActionEvent,
                     sizeof (NoActionEvent.PCRIndex) + sizeof (NoActionEvent.EventType) + GetDigestListBinSize (&NoActionEvent.Digests) + sizeof (NoActionEvent.EventSize),
                     GET_GUID_HOB_DATA (GuidHob.Guid),
                     GET_GUID_HOB_DATA_SIZE (GuidHob.Guid)
                     );

          GuidHob.Guid = GET_NEXT_HOB (GuidHob);
          GuidHob.Guid = GetNextGuidHob (&gTcg800155PlatformIdEventHobGuid, GuidHob.Guid);
        }

        //
        // EfiStartupLocalityEvent. Event format is TCG_PCR_EVENT2
        //
        GuidHob.Guid = GetFirstGuidHob (&gTpm2StartupLocalityHobGuid);
        if (GuidHob.Guid != NULL) {
          //
          // Get Locality Indicator from StartupLocality HOB
          //
          StartupLocalityEvent.StartupLocality = *(UINT8 *)(GET_GUID_HOB_DATA (GuidHob.Guid));
          CopyMem (StartupLocalityEvent.Signature, TCG_EfiStartupLocalityEvent_SIGNATURE, sizeof (StartupLocalityEvent.Signature));
          DEBUG ((DEBUG_INFO, "SetupEventLog: Set Locality from HOB into StartupLocalityEvent 0x%02x\n", StartupLocalityEvent.StartupLocality));

          //
          // Initialize StartupLocalityEvent
          //
          InitNoActionEvent (&NoActionEvent, sizeof (StartupLocalityEvent));

          //
          // Log EfiStartupLocalityEvent as the second Event
          //   TCG PC Client PFP spec. Section 9.3.4.3 Startup Locality Event
          //
          Status = TcgDxeLogEvent (
                     mTcg2EventInfo[Index].LogFormat,
                     &NoActionEvent,
                     sizeof (NoActionEvent.PCRIndex) + sizeof (NoActionEvent.EventType) + GetDigestListBinSize (&NoActionEvent.Digests) + sizeof (NoActionEvent.EventSize),
                     (UINT8 *)&StartupLocalityEvent,
                     sizeof (StartupLocalityEvent)
                     );
        }
      }
    }
  }

  //
  // 2. Create Final Log Area
  //
  for (Index = 0; Index < sizeof (mTcg2EventInfo)/sizeof (mTcg2EventInfo[0]); Index++) {
    if ((mTcgDxeData.BsCap.SupportedEventLogs & mTcg2EventInfo[Index].LogFormat) != 0) {
      if (mTcg2EventInfo[Index].LogFormat == EFI_TCG2_EVENT_LOG_FORMAT_TCG_2) {
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
        mTcgDxeData.FinalEventsTable[Index]                   = (VOID *)(UINTN)Lasa;
        (mTcgDxeData.FinalEventsTable[Index])->Version        = EFI_TCG2_FINAL_EVENTS_TABLE_VERSION;
        (mTcgDxeData.FinalEventsTable[Index])->NumberOfEvents = 0;

        mTcgDxeData.FinalEventLogAreaStruct[Index].EventLogFormat        = mTcg2EventInfo[Index].LogFormat;
        mTcgDxeData.FinalEventLogAreaStruct[Index].Lasa                  = Lasa + sizeof (EFI_TCG2_FINAL_EVENTS_TABLE);
        mTcgDxeData.FinalEventLogAreaStruct[Index].Laml                  = PcdGet32 (PcdTcg2FinalLogAreaLen) - sizeof (EFI_TCG2_FINAL_EVENTS_TABLE);
        mTcgDxeData.FinalEventLogAreaStruct[Index].EventLogSize          = 0;
        mTcgDxeData.FinalEventLogAreaStruct[Index].LastEvent             = (VOID *)(UINTN)mTcgDxeData.FinalEventLogAreaStruct[Index].Lasa;
        mTcgDxeData.FinalEventLogAreaStruct[Index].EventLogStarted       = FALSE;
        mTcgDxeData.FinalEventLogAreaStruct[Index].EventLogTruncated     = FALSE;
        mTcgDxeData.FinalEventLogAreaStruct[Index].Next800155EventOffset = 0;

        //
        // Install to configuration table for EFI_TCG2_EVENT_LOG_FORMAT_TCG_2
        //
        Status = gBS->InstallConfigurationTable (&gEfiTcg2FinalEventsTableGuid, (VOID *)mTcgDxeData.FinalEventsTable[Index]);
        if (EFI_ERROR (Status)) {
          return Status;
        }
      } else {
        //
        // No need to handle EFI_TCG2_EVENT_LOG_FORMAT_TCG_1_2
        //
        mTcgDxeData.FinalEventsTable[Index]                              = NULL;
        mTcgDxeData.FinalEventLogAreaStruct[Index].EventLogFormat        = mTcg2EventInfo[Index].LogFormat;
        mTcgDxeData.FinalEventLogAreaStruct[Index].Lasa                  = 0;
        mTcgDxeData.FinalEventLogAreaStruct[Index].Laml                  = 0;
        mTcgDxeData.FinalEventLogAreaStruct[Index].EventLogSize          = 0;
        mTcgDxeData.FinalEventLogAreaStruct[Index].LastEvent             = 0;
        mTcgDxeData.FinalEventLogAreaStruct[Index].EventLogStarted       = FALSE;
        mTcgDxeData.FinalEventLogAreaStruct[Index].EventLogTruncated     = FALSE;
        mTcgDxeData.FinalEventLogAreaStruct[Index].Next800155EventOffset = 0;
      }
    }
  }

  //
  // 3. Sync data from PEI to DXE
  //
  Status = EFI_SUCCESS;
  for (Index = 0; Index < sizeof (mTcg2EventInfo)/sizeof (mTcg2EventInfo[0]); Index++) {
    if ((mTcgDxeData.BsCap.SupportedEventLogs & mTcg2EventInfo[Index].LogFormat) != 0) {
      GuidHob.Raw = GetHobList ();
      Status      = EFI_SUCCESS;
      while (!EFI_ERROR (Status) &&
             (GuidHob.Raw = GetNextGuidHob (mTcg2EventInfo[Index].EventGuid, GuidHob.Raw)) != NULL)
      {
        TcgEvent = AllocateCopyPool (GET_GUID_HOB_DATA_SIZE (GuidHob.Guid), GET_GUID_HOB_DATA (GuidHob.Guid));
        ASSERT (TcgEvent != NULL);
        GuidHob.Raw = GET_NEXT_HOB (GuidHob);
        switch (mTcg2EventInfo[Index].LogFormat) {
          case EFI_TCG2_EVENT_LOG_FORMAT_TCG_1_2:
            Status = TcgDxeLogEvent (
                       mTcg2EventInfo[Index].LogFormat,
                       TcgEvent,
                       sizeof (TCG_PCR_EVENT_HDR),
                       ((TCG_PCR_EVENT *)TcgEvent)->Event,
                       ((TCG_PCR_EVENT_HDR *)TcgEvent)->EventSize
                       );
            break;
          case EFI_TCG2_EVENT_LOG_FORMAT_TCG_2:
            DigestListBin     = (UINT8 *)TcgEvent + sizeof (TCG_PCRINDEX) + sizeof (TCG_EVENTTYPE);
            DigestListBinSize = GetDigestListBinSize (DigestListBin);
            //
            // Save event size.
            //
            CopyMem (&EventSize, (UINT8 *)DigestListBin + DigestListBinSize, sizeof (UINT32));
            Event = (UINT8 *)DigestListBin + DigestListBinSize + sizeof (UINT32);
            //
            // Filter inactive digest in the event2 log from PEI HOB.
            //
            CopyMem (&TempDigestListBin, DigestListBin, GetDigestListBinSize (DigestListBin));
            EventSizePtr = CopyDigestListBinToBuffer (
                             DigestListBin,
                             &TempDigestListBin,
                             mTcgDxeData.BsCap.ActivePcrBanks,
                             &HashAlgorithmMaskCopied
                             );
            if (HashAlgorithmMaskCopied != mTcgDxeData.BsCap.ActivePcrBanks) {
              DEBUG ((
                DEBUG_ERROR,
                "ERROR: The event2 log includes digest hash mask 0x%x, but required digest hash mask is 0x%x\n",
                HashAlgorithmMaskCopied,
                mTcgDxeData.BsCap.ActivePcrBanks
                ));
            }

            //
            // Restore event size.
            //
            CopyMem (EventSizePtr, &EventSize, sizeof (UINT32));
            DigestListBinSize = GetDigestListBinSize (DigestListBin);

            Status = TcgDxeLogEvent (
                       mTcg2EventInfo[Index].LogFormat,
                       TcgEvent,
                       sizeof (TCG_PCRINDEX) + sizeof (TCG_EVENTTYPE) + DigestListBinSize + sizeof (UINT32),
                       Event,
                       EventSize
                       );
            break;
        }

        FreePool (TcgEvent);
      }
    }
  }

  return Status;
}

/**
  Measure and log an action string, and extend the measurement result into PCR[PCRIndex].

  @param[in] PCRIndex         PCRIndex to extend
  @param[in] String           A specific string that indicates an Action event.

  @retval EFI_SUCCESS         Operation completed successfully.
  @retval EFI_DEVICE_ERROR    The operation was unsuccessful.

**/
EFI_STATUS
TcgMeasureAction (
  IN      TPM_PCRINDEX  PCRIndex,
  IN      CHAR8         *String
  )
{
  TCG_PCR_EVENT_HDR  TcgEvent;

  TcgEvent.PCRIndex  = PCRIndex;
  TcgEvent.EventType = EV_EFI_ACTION;
  TcgEvent.EventSize = (UINT32)AsciiStrLen (String);
  return TcgDxeHashLogExtendEvent (
           0,
           (UINT8 *)String,
           TcgEvent.EventSize,
           &TcgEvent,
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
  TCG_PCR_EVENT_HDR           TcgEvent;
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
      TcgEvent.PCRIndex  = 1;
      TcgEvent.EventType = EV_TABLE_OF_DEVICES;
      TcgEvent.EventSize = sizeof (HandoffTables);

      HandoffTables.NumberOfTables            = 1;
      HandoffTables.TableEntry[0].VendorGuid  = gEfiMpServiceProtocolGuid;
      HandoffTables.TableEntry[0].VendorTable = ProcessorLocBuf;

      Status = TcgDxeHashLogExtendEvent (
                 0,
                 (UINT8 *)(UINTN)ProcessorLocBuf,
                 sizeof (EFI_CPU_PHYSICAL_LOCATION) * ProcessorNum,
                 &TcgEvent,
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
  IN      TPM_PCRINDEX  PCRIndex
  )
{
  TCG_PCR_EVENT_HDR  TcgEvent;
  UINT32             EventData;

  DEBUG ((DEBUG_INFO, "MeasureSeparatorEvent Pcr - %x\n", PCRIndex));

  EventData          = 0;
  TcgEvent.PCRIndex  = PCRIndex;
  TcgEvent.EventType = EV_SEPARATOR;
  TcgEvent.EventSize = (UINT32)sizeof (EventData);
  return TcgDxeHashLogExtendEvent (
           0,
           (UINT8 *)&EventData,
           sizeof (EventData),
           &TcgEvent,
           (UINT8 *)&EventData
           );
}

/**
  Measure and log an EFI variable, and extend the measurement result into a specific PCR.

  @param[in]  PCRIndex          PCR Index.
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
  IN      TPM_PCRINDEX   PCRIndex,
  IN      TCG_EVENTTYPE  EventType,
  IN      CHAR16         *VarName,
  IN      EFI_GUID       *VendorGuid,
  IN      VOID           *VarData,
  IN      UINTN          VarSize
  )
{
  EFI_STATUS          Status;
  TCG_PCR_EVENT_HDR   TcgEvent;
  UINTN               VarNameLength;
  UEFI_VARIABLE_DATA  *VarLog;

  DEBUG ((DEBUG_INFO, "Tcg2Dxe: MeasureVariable (Pcr - %x, EventType - %x, ", (UINTN)PCRIndex, (UINTN)EventType));
  DEBUG ((DEBUG_INFO, "VariableName - %s, VendorGuid - %g)\n", VarName, VendorGuid));

  VarNameLength      = StrLen (VarName);
  TcgEvent.PCRIndex  = PCRIndex;
  TcgEvent.EventType = EventType;

  TcgEvent.EventSize = (UINT32)(sizeof (*VarLog) + VarNameLength * sizeof (*VarName) + VarSize
                                - sizeof (VarLog->UnicodeName) - sizeof (VarLog->VariableData));

  VarLog = (UEFI_VARIABLE_DATA *)AllocatePool (TcgEvent.EventSize);
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

  if ((EventType == EV_EFI_VARIABLE_DRIVER_CONFIG) || (EventType == EV_EFI_SPDM_DEVICE_POLICY)) {
    //
    // Digest is the event data (UEFI_VARIABLE_DATA)
    //
    Status = TcgDxeHashLogExtendEvent (
               0,
               (UINT8 *)VarLog,
               TcgEvent.EventSize,
               &TcgEvent,
               (UINT8 *)VarLog
               );
  } else {
    ASSERT (VarData != NULL);
    Status = TcgDxeHashLogExtendEvent (
               0,
               (UINT8 *)VarData,
               VarSize,
               &TcgEvent,
               (UINT8 *)VarLog
               );
  }

  FreePool (VarLog);
  return Status;
}

/**
  Read then Measure and log an EFI variable, and extend the measurement result into a specific PCR.

  @param[in]  PCRIndex          PCR Index.
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
  IN      TPM_PCRINDEX   PCRIndex,
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
             PCRIndex,
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
           1,
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
           7,
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
               7,
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

  //
  // Meaurement UEFI device signature database
  //
  if ((PcdGet32 (PcdTcgPfpMeasurementRevision) >= TCG_EfiSpecIDEventStruct_SPEC_ERRATA_TPM2_REV_106) &&
      (PcdGet8 (PcdEnableSpdmDeviceAuthentication) != 0))
  {
    Status = GetVariable2 (EFI_DEVICE_SECURITY_DATABASE, &gEfiDeviceSignatureDatabaseGuid, &Data, &DataSize);
    if (Status == EFI_SUCCESS) {
      Status = MeasureVariable (
                 PCR_INDEX_FOR_SIGNATURE_DB,
                 EV_EFI_SPDM_DEVICE_POLICY,
                 EFI_DEVICE_SECURITY_DATABASE,
                 &gEfiDeviceSignatureDatabaseGuid,
                 Data,
                 DataSize
                 );
      FreePool (Data);
    } else if (Status == EFI_NOT_FOUND) {
      Data     = NULL;
      DataSize = 0;
      Status   = MeasureVariable (
                   PCR_INDEX_FOR_SIGNATURE_DB,
                   EV_EFI_SPDM_DEVICE_POLICY,
                   EFI_DEVICE_SECURITY_DATABASE,
                   &gEfiDeviceSignatureDatabaseGuid,
                   Data,
                   DataSize
                   );
    }
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
  TCG_PCR_EVENT_HDR  TcgEvent;

  TcgEvent.PCRIndex  = 7;
  TcgEvent.EventType = EV_EFI_ACTION;
  TcgEvent.EventSize = sizeof (FIRMWARE_DEBUGGER_EVENT_STRING) - 1;
  return TcgDxeHashLogExtendEvent (
           0,
           (UINT8 *)FIRMWARE_DEBUGGER_EVENT_STRING,
           sizeof (FIRMWARE_DEBUGGER_EVENT_STRING) - 1,
           &TcgEvent,
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
  Status = MeasureSeparatorEvent (7);
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
  EFI_STATUS    Status;
  TPM_PCRINDEX  PcrIndex;

  PERF_START_EX (mImageHandle, "EventRec", "Tcg2Dxe", 0, PERF_ID_TCG2_DXE);
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
    Status = TcgMeasureAction (
               4,
               EFI_CALLING_EFI_APPLICATION
               );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a not Measured. Error!\n", EFI_CALLING_EFI_APPLICATION));
    }

    //
    // 2. Draw a line between pre-boot env and entering post-boot env.
    // PCR[7] is already done.
    //
    for (PcrIndex = 0; PcrIndex < 7; PcrIndex++) {
      Status = MeasureSeparatorEvent (PcrIndex);
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_ERROR, "Separator Event not Measured. Error!\n"));
      }
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
    Status = TcgMeasureAction (
               4,
               EFI_RETURNING_FROM_EFI_APPLICATION
               );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a not Measured. Error!\n", EFI_RETURNING_FROM_EFI_APPLICATION));
    }

    //
    // 7. Next boot attempt, measure "Calling EFI Application from Boot Option" again
    // TCG PC Client PFP spec Section 2.4.4.5 Step 4
    //
    Status = TcgMeasureAction (
               4,
               EFI_CALLING_EFI_APPLICATION
               );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a not Measured. Error!\n", EFI_CALLING_EFI_APPLICATION));
    }
  }

  DEBUG ((DEBUG_INFO, "TPM2 Tcg2Dxe Measure Data when ReadyToBoot\n"));
  //
  // Increase boot attempt counter.
  //
  mBootAttempts++;
  PERF_END_EX (mImageHandle, "EventRec", "Tcg2Dxe", 0, PERF_ID_TCG2_DXE + 1);
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
  Status = TcgMeasureAction (
             5,
             EFI_EXIT_BOOT_SERVICES_INVOCATION
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a not Measured. Error!\n", EFI_EXIT_BOOT_SERVICES_INVOCATION));
  }

  //
  // Measure success of ExitBootServices
  //
  Status = TcgMeasureAction (
             5,
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
  // Measure invocation of ExitBootServices,
  //
  Status = TcgMeasureAction (
             5,
             EFI_EXIT_BOOT_SERVICES_INVOCATION
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a not Measured. Error!\n", EFI_EXIT_BOOT_SERVICES_INVOCATION));
  }

  //
  // Measure Failure of ExitBootServices,
  //
  Status = TcgMeasureAction (
             5,
             EFI_EXIT_BOOT_SERVICES_FAILED
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a not Measured. Error!\n", EFI_EXIT_BOOT_SERVICES_FAILED));
  }
}

/**
  This routine is called to properly shutdown the TPM before system reset.
  It follow chapter "12.2.3 Startup State" in Trusted Platform Module Library
  Part 1: Architecture, Revision 01.16.

  @param[in]  ResetType         The type of reset to perform.
  @param[in]  ResetStatus       The status code for the reset.
  @param[in]  DataSize          The size, in bytes, of ResetData.
  @param[in]  ResetData         For a ResetType of EfiResetCold, EfiResetWarm, or
                                EfiResetShutdown the data buffer starts with a Null-terminated
                                string, optionally followed by additional binary data.
                                The string is a description that the caller may use to further
                                indicate the reason for the system reset.
                                For a ResetType of EfiResetPlatformSpecific the data buffer
                                also starts with a Null-terminated string that is followed
                                by an EFI_GUID that describes the specific type of reset to perform.
**/
VOID
EFIAPI
ShutdownTpmOnReset (
  IN EFI_RESET_TYPE  ResetType,
  IN EFI_STATUS      ResetStatus,
  IN UINTN           DataSize,
  IN VOID            *ResetData OPTIONAL
  )
{
  EFI_STATUS  Status;

  Status = Tpm2Shutdown (TPM_SU_CLEAR);
  DEBUG ((DEBUG_VERBOSE, "Tpm2Shutdown (SU_CLEAR) - %r\n", Status));
}

/**
  Hook the system reset to properly shutdown TPM.
  It follow chapter "12.2.3 Startup State" in Trusted Platform Module Library
  Part 1: Architecture, Revision 01.16.

  @param[in]  Event     Event whose notification function is being invoked
  @param[in]  Context   Pointer to the notification function's context
**/
VOID
EFIAPI
OnResetNotificationInstall (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  EFI_STATUS                       Status;
  EFI_RESET_NOTIFICATION_PROTOCOL  *ResetNotify;

  Status = gBS->LocateProtocol (&gEfiResetNotificationProtocolGuid, NULL, (VOID **)&ResetNotify);
  if (!EFI_ERROR (Status)) {
    Status = ResetNotify->RegisterResetNotify (ResetNotify, ShutdownTpmOnReset);
    ASSERT_EFI_ERROR (Status);
    DEBUG ((DEBUG_VERBOSE, "TCG2: Hook system reset to properly shutdown TPM.\n"));

    gBS->CloseEvent (Event);
  }
}

/**
  The function install Tcg2 protocol.

  @retval EFI_SUCCESS     Tcg2 protocol is installed.
  @retval other           Some error occurs.
**/
EFI_STATUS
InstallTcg2 (
  VOID
  )
{
  EFI_STATUS  Status;
  EFI_HANDLE  Handle;

  Handle = NULL;
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &Handle,
                  &gEfiTcg2ProtocolGuid,
                  &mTcg2Protocol,
                  NULL
                  );
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
  EFI_STATUS                       Status;
  EFI_EVENT                        Event;
  VOID                             *Registration;
  UINT32                           MaxCommandSize;
  UINT32                           MaxResponseSize;
  UINTN                            Index;
  EFI_TCG2_EVENT_ALGORITHM_BITMAP  TpmHashAlgorithmBitmap;
  UINT32                           ActivePCRBanks;
  UINT32                           NumberOfPCRBanks;

  mImageHandle = ImageHandle;

  if (CompareGuid (PcdGetPtr (PcdTpmInstanceGuid), &gEfiTpmDeviceInstanceNoneGuid) ||
      CompareGuid (PcdGetPtr (PcdTpmInstanceGuid), &gEfiTpmDeviceInstanceTpm12Guid))
  {
    DEBUG ((DEBUG_INFO, "No TPM2 instance required!\n"));
    return EFI_UNSUPPORTED;
  }

  if (GetFirstGuidHob (&gTpmErrorHobGuid) != NULL) {
    DEBUG ((DEBUG_ERROR, "TPM2 error!\n"));
    return EFI_DEVICE_ERROR;
  }

  Status = Tpm2RequestUseTpm ();
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "TPM2 not detected!\n"));
    return Status;
  }

  //
  // Fill information
  //
  ASSERT (TCG_EVENT_LOG_AREA_COUNT_MAX == sizeof (mTcg2EventInfo)/sizeof (mTcg2EventInfo[0]));

  mTcgDxeData.BsCap.Size                   = sizeof (EFI_TCG2_BOOT_SERVICE_CAPABILITY);
  mTcgDxeData.BsCap.ProtocolVersion.Major  = 1;
  mTcgDxeData.BsCap.ProtocolVersion.Minor  = 1;
  mTcgDxeData.BsCap.StructureVersion.Major = 1;
  mTcgDxeData.BsCap.StructureVersion.Minor = 1;

  DEBUG ((DEBUG_INFO, "Tcg2.ProtocolVersion  - %02x.%02x\n", mTcgDxeData.BsCap.ProtocolVersion.Major, mTcgDxeData.BsCap.ProtocolVersion.Minor));
  DEBUG ((DEBUG_INFO, "Tcg2.StructureVersion - %02x.%02x\n", mTcgDxeData.BsCap.StructureVersion.Major, mTcgDxeData.BsCap.StructureVersion.Minor));

  Status = Tpm2GetCapabilityManufactureID (&mTcgDxeData.BsCap.ManufacturerID);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Tpm2GetCapabilityManufactureID fail!\n"));
  } else {
    DEBUG ((DEBUG_INFO, "Tpm2GetCapabilityManufactureID - %08x\n", mTcgDxeData.BsCap.ManufacturerID));
  }

  DEBUG_CODE_BEGIN ();
  UINT32  FirmwareVersion1;
  UINT32  FirmwareVersion2;

  Status = Tpm2GetCapabilityFirmwareVersion (&FirmwareVersion1, &FirmwareVersion2);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Tpm2GetCapabilityFirmwareVersion fail!\n"));
  } else {
    DEBUG ((DEBUG_INFO, "Tpm2GetCapabilityFirmwareVersion - %08x %08x\n", FirmwareVersion1, FirmwareVersion2));
  }

  DEBUG_CODE_END ();

  Status = Tpm2GetCapabilityMaxCommandResponseSize (&MaxCommandSize, &MaxResponseSize);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Tpm2GetCapabilityMaxCommandResponseSize fail!\n"));
  } else {
    mTcgDxeData.BsCap.MaxCommandSize  = (UINT16)MaxCommandSize;
    mTcgDxeData.BsCap.MaxResponseSize = (UINT16)MaxResponseSize;
    DEBUG ((DEBUG_INFO, "Tpm2GetCapabilityMaxCommandResponseSize - %08x, %08x\n", MaxCommandSize, MaxResponseSize));
  }

  //
  // Get supported PCR and current Active PCRs
  //
  Status = Tpm2GetCapabilitySupportedAndActivePcrs (&TpmHashAlgorithmBitmap, &ActivePCRBanks);
  ASSERT_EFI_ERROR (Status);

  mTcgDxeData.BsCap.HashAlgorithmBitmap = TpmHashAlgorithmBitmap & PcdGet32 (PcdTcg2HashAlgorithmBitmap);
  mTcgDxeData.BsCap.ActivePcrBanks      = ActivePCRBanks & PcdGet32 (PcdTcg2HashAlgorithmBitmap);

  //
  // Need calculate NumberOfPCRBanks here, because HashAlgorithmBitmap might be removed by PCD.
  //
  NumberOfPCRBanks = 0;
  for (Index = 0; Index < 32; Index++) {
    if ((mTcgDxeData.BsCap.HashAlgorithmBitmap & (1u << Index)) != 0) {
      NumberOfPCRBanks++;
    }
  }

  if (PcdGet32 (PcdTcg2NumberOfPCRBanks) == 0) {
    mTcgDxeData.BsCap.NumberOfPCRBanks = NumberOfPCRBanks;
  } else {
    mTcgDxeData.BsCap.NumberOfPCRBanks = PcdGet32 (PcdTcg2NumberOfPCRBanks);
    if (PcdGet32 (PcdTcg2NumberOfPCRBanks) > NumberOfPCRBanks) {
      DEBUG ((DEBUG_ERROR, "ERROR: PcdTcg2NumberOfPCRBanks(0x%x) > NumberOfPCRBanks(0x%x)\n", PcdGet32 (PcdTcg2NumberOfPCRBanks), NumberOfPCRBanks));
      mTcgDxeData.BsCap.NumberOfPCRBanks = NumberOfPCRBanks;
    }
  }

  mTcgDxeData.BsCap.SupportedEventLogs = EFI_TCG2_EVENT_LOG_FORMAT_TCG_1_2 | EFI_TCG2_EVENT_LOG_FORMAT_TCG_2;
  if ((mTcgDxeData.BsCap.ActivePcrBanks & EFI_TCG2_BOOT_HASH_ALG_SHA1) == 0) {
    //
    // No need to expose TCG1.2 event log if SHA1 bank does not exist.
    //
    mTcgDxeData.BsCap.SupportedEventLogs &= ~EFI_TCG2_EVENT_LOG_FORMAT_TCG_1_2;
  }

  DEBUG ((DEBUG_INFO, "Tcg2.SupportedEventLogs - 0x%08x\n", mTcgDxeData.BsCap.SupportedEventLogs));
  DEBUG ((DEBUG_INFO, "Tcg2.HashAlgorithmBitmap - 0x%08x\n", mTcgDxeData.BsCap.HashAlgorithmBitmap));
  DEBUG ((DEBUG_INFO, "Tcg2.NumberOfPCRBanks      - 0x%08x\n", mTcgDxeData.BsCap.NumberOfPCRBanks));
  DEBUG ((DEBUG_INFO, "Tcg2.ActivePcrBanks        - 0x%08x\n", mTcgDxeData.BsCap.ActivePcrBanks));

  if (mTcgDxeData.BsCap.TPMPresentFlag) {
    //
    // Setup the log area and copy event log from hob list to it
    //
    Status = SetupEventLog ();
    ASSERT_EFI_ERROR (Status);

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
    // Hook the system reset to properly shutdown TPM.
    //
    EfiCreateProtocolNotifyEvent (&gEfiResetNotificationProtocolGuid, TPL_CALLBACK, OnResetNotificationInstall, NULL, &Registration);
  }

  //
  // Install Tcg2Protocol
  //
  Status = InstallTcg2 ();
  DEBUG ((DEBUG_INFO, "InstallTcg2 - %r\n", Status));

  return Status;
}
