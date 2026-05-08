/** @file
  This module implements EFI CC Measurement Protocol for Arm CCA.

  Copyright (c) 2025, Arm Limited. All rights reserved.<BR>
  Copyright (c) 2020 - 2021, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Reference(s):
   - Realm Management Monitor (RMM) Specification, version 1.0-rel0
     (https://developer.arm.com/documentation/den0137/)
   - TCG PC Client Platform Firmware Profile Specification, Level 00
     Version 1.06 Revision 52 Family “2.0” - December 4, 2023
     (https://trustedcomputinggroup.org/resource/pc-client-specific-platform-
      firmware-profile-specification/)
**/

#include <PiDxe.h>
#include <IndustryStandard/Acpi.h>
#include <IndustryStandard/PeImage.h>
#include <IndustryStandard/TcpaAcpi.h>

#include <Guid/CcEventHob.h>
#include <Guid/EventExitBootServiceFailed.h>
#include <Guid/EventGroup.h>
#include <Guid/GlobalVariable.h>
#include <Guid/HobList.h>
#include <Guid/ImageAuthentication.h>

#include <Protocol/AcpiTable.h>
#include <Protocol/CcMeasurement.h>
#include <Protocol/DevicePath.h>
#include <Protocol/MpService.h>
#include <Protocol/Tcg2Protocol.h>
#include <Protocol/VariableWrite.h>

#include <Library/ArmCcaLib.h>
#include <Library/ArmCcaRsiLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/HashLib.h>
#include <Library/HobLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PcdLib.h>
#include <Library/PerformanceLib.h>
#include <Library/PrintLib.h>
#include <Library/ReportStatusCodeLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>

#include "MeasureBootPeCoff.h"

#define PERF_ID_CC_TCG2_DXE  0x3130

#define   CC_EVENT_LOG_AREA_COUNT_MAX  1

STATIC UINTN       mBootAttempts  = 0;
STATIC CHAR16      mBootVarName[] = L"BootOrder";
STATIC EFI_HANDLE  mImageHandle;

/** A structure defining the CC Event log area.
*/
typedef struct {
  /// The event log format.
  EFI_CC_EVENT_LOG_FORMAT    EventLogFormat;
  /// Physical address of the log area.
  EFI_PHYSICAL_ADDRESS       Lasa;
  /// Log area length.
  UINT64                     Laml;
  /// Event log size.
  UINTN                      EventLogSize;
  /// Pointer to the last event.
  UINT8                      *LastEvent;
  /// Flag to indicate that the Event log has started.
  BOOLEAN                    EventLogStarted;
  /// Flag to indicate that the Event log is truncated.
  BOOLEAN                    EventLogTruncated;
  /// Offset of the next SP 800-155 event.
  UINTN                      Next800155EventOffset;
} CC_EVENT_LOG_AREA_STRUCT;

/** A structure defining the Arm CCA TCG2 data.
*/
typedef struct ArmCcaTcg2DxeData {
  /// The CC boot service capabilities.
  EFI_CC_BOOT_SERVICE_CAPABILITY    BsCap;
  /// The Realm hash algorithm.
  UINT8                             RealmRemHashAlgorithm;
  /// The event log area.
  CC_EVENT_LOG_AREA_STRUCT          EventLogAreaStruct[CC_EVENT_LOG_AREA_COUNT_MAX];
  /// A flag indicating if GetEventLog() has been called.
  BOOLEAN                           GetEventLogCalled[CC_EVENT_LOG_AREA_COUNT_MAX];
  /// The Final event log area.
  CC_EVENT_LOG_AREA_STRUCT          FinalEventLogAreaStruct[CC_EVENT_LOG_AREA_COUNT_MAX];
  /// A pointer to the Final events table.
  EFI_CC_FINAL_EVENTS_TABLE         *FinalEventsTable[CC_EVENT_LOG_AREA_COUNT_MAX];
} ARMCCA_TCG2_DXE_DATA;

/* An instance of the Arm CCA TCG2 data.
*/
STATIC ARMCCA_TCG2_DXE_DATA  mArmCcaDxeData = {
  {
    sizeof (EFI_CC_BOOT_SERVICE_CAPABILITY), // Size
    { 1,                  1 },               // StructureVersion
    { 1,                  1 },               // ProtocolVersion
    EFI_CC_BOOT_HASH_ALG_SHA256,             // HashAlgorithmBitmap
    EFI_CC_EVENT_LOG_FORMAT_TCG_2,           // SupportedEventLogs
    { EFI_CC_TYPE_ARMCCA, 0 }                // {CC_TYPE, CC_SUBTYPE}
  },
};

/** A structure defining a mapping of a UEFI variable name and
    its correspondingGUID.
*/
typedef struct {
  /// Name of the variable.
  CHAR16      *VariableName;
  /// Corresponding GUID for the variable.
  EFI_GUID    *VendorGuid;
} VARIABLE_TYPE;

/* A list of UEFI variables that need to be measured and their
   corresponding GUIDs.
*/
STATIC VARIABLE_TYPE  mVariableType[] = {
  { EFI_SECURE_BOOT_MODE_NAME,    &gEfiGlobalVariableGuid        },
  { EFI_PLATFORM_KEY_NAME,        &gEfiGlobalVariableGuid        },
  { EFI_KEY_EXCHANGE_KEY_NAME,    &gEfiGlobalVariableGuid        },
  { EFI_IMAGE_SECURITY_DATABASE,  &gEfiImageSecurityDatabaseGuid },
  { EFI_IMAGE_SECURITY_DATABASE1, &gEfiImageSecurityDatabaseGuid },
};

/* A template instance of the Confidential Compute Event Log ACPI table.
  The event log area, event log length and other fields are populated
  at runtime before the table is installed.
*/
STATIC EFI_CC_EVENTLOG_ACPI_TABLE  mArmCcaEventlogAcpiTemplate = {
  {
    EFI_ACPI_6_5_CONFIDENTIAL_COMPUTE_EVENT_LOG_TABLE_SIGNATURE,
    sizeof (mArmCcaEventlogAcpiTemplate),
    EFI_CC_EVENTLOG_ACPI_TABLE_REVISION,
    //
    // Compiler initializes the remaining bytes to 0
    // These fields should be filled in production
    //
  },
  { EFI_CC_TYPE_ARMCCA, 0 }, // CcType
  0,                         // rsvd
  0,                         // laml
  0,                         // lasa
};

/**
 A structure mapping the Arm CCA hash information.
*/
typedef struct {
  /// The Realm hash algorithm.
  UINT8            RealmHashAlgorithm;
  /// The alogrithm ID for the hash function.
  TPMI_ALG_HASH    HashAlgo;
  /// The hash size.
  UINT16           HashSize;
  /// The hash mask.
  UINT32           HashMask;
} ARMCCA_HASH_INFO;

/**
  A map of the hash algorithms supported by a Realm.
*/
STATIC ARMCCA_HASH_INFO  mHashInfo[] = {
  { ARM_CCA_RSI_HASH_SHA_256, TPM_ALG_SHA256, SHA256_DIGEST_SIZE, HASH_ALG_SHA256 },
  { ARM_CCA_RSI_HASH_SHA_512, TPM_ALG_SHA512, SHA512_DIGEST_SIZE, HASH_ALG_SHA512 }
};

/**
  Get hash algorithm ID corresponding to the Realm Hash algorithm.

  @param[in]    RealmHashAlgorithm   The Realm Hash Algorithm.

  @return Hash Algorithm ID if success else TPM_ALG_ERROR.
**/
STATIC
TPM_ALG_ID
RealmHashAlgoToTpmAlgoId (
  UINT8  RealmHashAlgorithm
  )
{
  UINTN  Index;

  for (Index = 0; Index < ARRAY_SIZE (mHashInfo); Index++) {
    if (mHashInfo[Index].RealmHashAlgorithm == RealmHashAlgorithm) {
      return mHashInfo[Index].HashAlgo;
    }
  }

  // No suitable algorithm found return.
  return TPM_ALG_ERROR;
}

/**
  Get the hash information based on the Hash Algo.

  @param[in]     HashAlgo           Hash Algorithm Id.

  @return Pointer to the Hash information on success or NULL.
**/
STATIC
ARMCCA_HASH_INFO *
GetHashInfoFromAlgo (
  IN TPMI_ALG_HASH  HashAlgo
  )
{
  UINTN  Index;

  for (Index = 0; Index < ARRAY_SIZE (mHashInfo); Index++) {
    if (mHashInfo[Index].HashAlgo == HashAlgo) {
      return &mHashInfo[Index];
    }
  }

  ASSERT (FALSE);
  return NULL;
}

/**
  Get hash size based on Algo

  @param[in]     HashAlgo           Hash Algorithm Id.

  @return Size of the hash.
**/
STATIC
UINT16
GetHashSizeFromAlgo (
  IN TPMI_ALG_HASH  HashAlgo
  )
{
  ARMCCA_HASH_INFO  *HashInfo;

  HashInfo = GetHashInfoFromAlgo (HashAlgo);
  if (HashInfo != NULL) {
    return HashInfo->HashSize;
  }

  ASSERT (FALSE);
  return 0;
}

/**
  Get hash mask based on Algo

  @param[in]     HashAlgo           Hash Algorithm Id.

  @return Hash mask.
**/
STATIC
UINT32
GetHashMaskFromAlgo (
  IN TPMI_ALG_HASH  HashAlgo
  )
{
  ARMCCA_HASH_INFO  *HashInfo;

  HashInfo = GetHashInfoFromAlgo (HashAlgo);
  if (HashInfo != NULL) {
    return HashInfo->HashMask;
  }

  ASSERT (FALSE);
  return 0;
}

/**
  Read the Realm configuration to get the Realm Hash algorithm.

  @param[out] RealmHashAlgorithm  The Realm Hash algorithm

  @retval EFI_SUCCESS            Success, the Realm Hash algorithm is returned.
  @retval EFI_OUT_OF_RESOURCES   Out of resources.
  @retval EFI_UNSUPPORTED        Unsupported algorithm.
**/
STATIC
EFI_STATUS
EFIAPI
GetRealmHashAlgorithm (
  OUT UINT8  *RealmHashAlgorithm
  )
{
  EFI_STATUS            Status;
  ARM_CCA_REALM_CONFIG  *Config;
  UINT8                 HashAlgorithm;

  Config = AllocateAlignedPages (
             EFI_SIZE_TO_PAGES (sizeof (ARM_CCA_REALM_CONFIG)),
             ARM_CCA_REALM_GRANULE_SIZE
             );
  if (Config == NULL) {
    ASSERT (0);
    return EFI_OUT_OF_RESOURCES;
  }

  ZeroMem (Config, sizeof (ARM_CCA_REALM_CONFIG));

  Status = ArmCcaRsiGetRealmConfig (Config);
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    FreeAlignedPages (
      Config,
      EFI_SIZE_TO_PAGES (sizeof (ARM_CCA_REALM_CONFIG))
      );
    return Status;
  }

  HashAlgorithm = Config->HashAlgorithm;

  FreeAlignedPages (Config, EFI_SIZE_TO_PAGES (sizeof (ARM_CCA_REALM_CONFIG)));

  if ((HashAlgorithm != ARM_CCA_RSI_HASH_SHA_256) &&
      (HashAlgorithm != ARM_CCA_RSI_HASH_SHA_512))
  {
    return EFI_UNSUPPORTED;
  }

  *RealmHashAlgorithm = HashAlgorithm;
  return EFI_SUCCESS;
}

/**
  Copy TPML_DIGEST_VALUES into a buffer

  @param[in,out] Buffer             Buffer to hold copied TPML_DIGEST_VALUES
                                    compact binary.
  @param[in]     DigestList         TPML_DIGEST_VALUES to be copied.
  @param[in]     HashAlgorithmMask  HASH bits corresponding to the desired
                                    digests to copy.

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
      DEBUG ((
        DEBUG_WARN,
        "WARNING: ArmCCA Event log has HashAlg unsupported (0x%x)\n",
        DigestList->digests[Index].hashAlg
        ));
      continue;
    }

    CopyMem (
      Buffer,
      &DigestList->digests[Index].hashAlg,
      sizeof (DigestList->digests[Index].hashAlg)
      );
    Buffer = (UINT8 *)Buffer +
             sizeof (DigestList->digests[Index].hashAlg);
    DigestSize = GetHashSizeFromAlgo (DigestList->digests[Index].hashAlg);
    CopyMem (Buffer, &DigestList->digests[Index].digest, DigestSize);
    Buffer = (UINT8 *)Buffer + DigestSize;
    DigestListCount++;
  }

  WriteUnaligned32 (DigestListCountPtr, DigestListCount);

  return Buffer;
}

#ifdef DUMP_EVENT_LOGS

#define COLUMN_SIZE  (16 * 2)

/**

  This function dump raw data.

  @param[in]  Data  raw data
  @param[in]  Size  raw data size

**/
STATIC
VOID
EFIAPI
InternalDumpData (
  IN UINT8  *Data,
  IN UINTN  Size
  )
{
  UINTN  Index;

  for (Index = 0; Index < Size; Index++) {
    if (Index == (COLUMN_SIZE/2)) {
      DEBUG ((DEBUG_INFO, " | %02x", (UINTN)Data[Index]));
    } else {
      DEBUG ((DEBUG_INFO, " %02x", (UINTN)Data[Index]));
    }
  }
}

/**

  This function dump raw data with colume format.

  @param[in]  Data  raw data
  @param[in]  Size  raw data size

**/
STATIC
VOID
EFIAPI
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

#endif

/**
  This function initialize CC_EVENT_HDR for EV_NO_ACTION
  Event Type other than EFI Specification ID event. The behavior is defined
  by TCG PC Client PFP Spec. Section 9.3.4 EV_NO_ACTION Event Types

  @param[in, out]   NoActionEvent  Event Header of EV_NO_ACTION Event
  @param[in]        EventSize      Event Size of the EV_NO_ACTION Event

**/
STATIC
VOID
EFIAPI
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
  // Set Hash count & hashAlg accordingly,
  // while Digest.digests[n].digest to all 0.
  //
  ZeroMem (&NoActionEvent->Digests, sizeof (NoActionEvent->Digests));

  if ((mArmCcaDxeData.BsCap.HashAlgorithmBitmap &
       EFI_CC_BOOT_HASH_ALG_SHA256) != 0)
  {
    HashAlgId = TPM_ALG_SHA256;
    CopyMem (DigestBuffer, &HashAlgId, sizeof (TPMI_ALG_HASH));
    DigestBuffer += sizeof (TPMI_ALG_HASH) + GetHashSizeFromAlgo (HashAlgId);
    DigestListCount++;
  }

  if ((mArmCcaDxeData.BsCap.HashAlgorithmBitmap &
       EFI_CC_BOOT_HASH_ALG_SHA512) != 0)
  {
    HashAlgId = TPM_ALG_SHA512;
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
  Get All processors EFI_CPU_LOCATION in system. LocationBuf is allocated
  inside the function. Caller is responsible to free LocationBuf.

  @param[out] LocationBuf          Returns Processor Location Buffer.
  @param[out] Num                  Returns processor number.

  @retval EFI_SUCCESS              Operation completed successfully.
  @retval EFI_UNSUPPORTED       MpService protocol not found.

**/
STATIC
EFI_STATUS
EFIAPI
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

  Status = gBS->LocateProtocol (
                  &gEfiMpServiceProtocolGuid,
                  NULL,
                  (VOID **)&MpProtocol
                  );
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

  @param[in]      This                Indicates the calling context
  @param[in, out] ProtocolCapability  The caller allocates memory for a
                                      EFI_CC_BOOT_SERVICE_CAPABILITY structure
                                      and sets the size field to the size of
                                      the structure allocated.
                                      The callee fills in the fields with the
                                      EFI protocol capability information and
                                      the current EFI TCG2 state information
                                      up to the number of fields which  fit
                                      within the size of the structure passed
                                      in.

  @retval EFI_SUCCESS            Operation completed successfully.
  @retval EFI_DEVICE_ERROR       The command was unsuccessful.
                                 The ProtocolCapability variable will not be
                                 populated.
  @retval EFI_INVALID_PARAMETER  One or more of the parameters are incorrect.
                                 The ProtocolCapability variable will not be
                                 populated.
  @retval EFI_BUFFER_TOO_SMALL   The ProtocolCapability variable is too small
                                 to hold the full response.
                                 It will be partially populated (required Size
                                 field will be set).
**/
EFI_STATUS
EFIAPI
ArmCcaGetCapability (
  IN EFI_CC_MEASUREMENT_PROTOCOL         *This,
  IN OUT EFI_CC_BOOT_SERVICE_CAPABILITY  *ProtocolCapability
  )
{
  DEBUG ((DEBUG_VERBOSE, "ArmCcaGetCapability\n"));

  if ((This == NULL) || (ProtocolCapability == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if (ProtocolCapability->Size < sizeof (EFI_CC_BOOT_SERVICE_CAPABILITY)) {
    ProtocolCapability->Size = sizeof (EFI_CC_BOOT_SERVICE_CAPABILITY);
    return EFI_BUFFER_TOO_SMALL;
  }

  CopyMem (
    ProtocolCapability,
    &mArmCcaDxeData.BsCap,
    sizeof (EFI_CC_BOOT_SERVICE_CAPABILITY)
    );

  return EFI_SUCCESS;
}

#ifdef DUMP_EVENT_LOGS

/**
  This function dump PCR event.
  CC Event log reuse the TCG PCR Event spec.
  The first event in the event log is the SHA1 log format.
  There is only ONE TCG_PCR_EVENT in CC Event log.

  @param[in]  EventHdr     TCG PCR event structure.
**/
STATIC
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
STATIC
VOID
EFIAPI
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

#endif

/**
  This function get size of TCG_EfiSpecIDEventStruct.

  @param[in]  TcgEfiSpecIdEventStruct     A pointer to TCG_EfiSpecIDEventStruct.
**/
STATIC
UINTN
EFIAPI
GetTcgEfiSpecIdEventStructSize (
  IN TCG_EfiSpecIDEventStruct  *TcgEfiSpecIdEventStruct
  )
{
  TCG_EfiSpecIdEventAlgorithmSize  *DigestSize;
  UINT8                            *VendorInfoSize;
  UINT32                           NumberOfAlgorithms;

  CopyMem (
    &NumberOfAlgorithms,
    TcgEfiSpecIdEventStruct + 1,
    sizeof (NumberOfAlgorithms)
    );

  DigestSize = (TCG_EfiSpecIdEventAlgorithmSize *)
               ((UINT8 *)TcgEfiSpecIdEventStruct +
                sizeof (*TcgEfiSpecIdEventStruct) +
                sizeof (NumberOfAlgorithms));
  VendorInfoSize = (UINT8 *)&DigestSize[NumberOfAlgorithms];
  return sizeof (TCG_EfiSpecIDEventStruct) + sizeof (UINT32) +
         (NumberOfAlgorithms * sizeof (TCG_EfiSpecIdEventAlgorithmSize)) +
         sizeof (UINT8) + (*VendorInfoSize);
}

#ifdef DUMP_EVENT_LOGS

/**
  This function dumps CC Event (including the Digests).

  @param[in]  CcEvent     CC Event structure.
**/
STATIC
VOID
EFIAPI
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
  This function returns size of CC Table event.

  @param[in]  CcEvent     CC Table event structure.

  @return size of CC event.
**/
STATIC
UINTN
EFIAPI
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
  Arm CCA only supports EFI_CC_EVENT_LOG_FORMAT_TCG_2

  @param[in]  EventLogFormat     The type of the event log for which the
                                 information is requested.
  @param[in]  EventLogLocation   A pointer to the memory address of the
                                 event log.
  @param[in]  EventLogLastEntry  If the Event Log contains more than one
                                 entry, this is a pointer to the address
                                 of the start of the last entry in the
                                 event log in memory.
  @param[in]  FinalEventsTable   A pointer to the memory address of the
                                 final event table.
**/
STATIC
VOID
EFIAPI
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
  CcEvent = (CC_EVENT *)((UINTN)TcgEfiSpecIdEventStruct +
                         GetTcgEfiSpecIdEventStructSize (TcgEfiSpecIdEventStruct));
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
    for (NumberOfEvents = 0;
         NumberOfEvents < FinalEventsTable->NumberOfEvents;
         NumberOfEvents++)
    {
      DumpCcEvent (CcEvent);
      CcEvent = (CC_EVENT *)((UINTN)CcEvent + GetCcEventSize (CcEvent));
    }
  }

  return;
}

#endif

/**
  The EFI_CC_MEASUREMENT_PROTOCOL Get Event Log function call allows a caller to
  retrieve the address of a given event log and its last entry.

  @param[in]  This               Indicates the calling context
  @param[in]  EventLogFormat     The type of the event log for which the
                                 information is requested.
  @param[out] EventLogLocation   A pointer to the memory address of the
                                 event log.
  @param[out] EventLogLastEntry  If the Event Log contains more than one
                                 entry, this is a pointer to the address
                                 of the start of the last entry in the
                                 event log in memory.
  @param[out] EventLogTruncated  If the Event Log is missing at least one
                                 entry because an event would have exceeded
                                 the area allocated for events, this value
                                 is set to TRUE.
                                 Otherwise, the value will be FALSE and the
                                 Event Log will be complete.

  @retval EFI_SUCCESS            Operation completed successfully.
  @retval EFI_INVALID_PARAMETER  One or more of the parameters are incorrect
                                 (e.g. asking for an event log whose format
                                 is not supported).
**/
EFI_STATUS
EFIAPI
ArmCcaGetEventLog (
  IN EFI_CC_MEASUREMENT_PROTOCOL  *This,
  IN EFI_CC_EVENT_LOG_FORMAT      EventLogFormat,
  OUT EFI_PHYSICAL_ADDRESS        *EventLogLocation,
  OUT EFI_PHYSICAL_ADDRESS        *EventLogLastEntry,
  OUT BOOLEAN                     *EventLogTruncated
  )
{
  UINTN  Index;

  Index = 0;
  DEBUG ((DEBUG_INFO, "ArmCcaGetEventLog ... (0x%x)\n", EventLogFormat));
  ASSERT (EventLogFormat == EFI_CC_EVENT_LOG_FORMAT_TCG_2);

  if (EventLogLocation != NULL) {
    *EventLogLocation = mArmCcaDxeData.EventLogAreaStruct[Index].Lasa;
    DEBUG ((DEBUG_INFO, "ArmCcaGetEventLog (EventLogLocation - %x)\n", *EventLogLocation));
  }

  if (EventLogLastEntry != NULL) {
    if (!mArmCcaDxeData.EventLogAreaStruct[Index].EventLogStarted) {
      *EventLogLastEntry = (EFI_PHYSICAL_ADDRESS)(UINTN)0;
    } else {
      *EventLogLastEntry = (EFI_PHYSICAL_ADDRESS)(UINTN)mArmCcaDxeData.EventLogAreaStruct[Index].LastEvent;
    }

    DEBUG ((DEBUG_INFO, "ArmCcaGetEventLog (EventLogLastEntry - %x)\n", *EventLogLastEntry));
  }

  if (EventLogTruncated != NULL) {
    *EventLogTruncated = mArmCcaDxeData.EventLogAreaStruct[Index].EventLogTruncated;
    DEBUG ((DEBUG_INFO, "ArmCcaGetEventLog (EventLogTruncated - %x)\n", *EventLogTruncated));
  }

  DEBUG ((DEBUG_INFO, "ArmCcaGetEventLog - %r\n", EFI_SUCCESS));

  // Dump Event Log for debug purpose
 #ifdef DUMP_EVENT_LOGS
  if ((EventLogLocation != NULL) && (EventLogLastEntry != NULL)) {
    DumpCcEventLog (EventLogFormat, *EventLogLocation, *EventLogLastEntry, mArmCcaDxeData.FinalEventsTable[Index]);
  }

 #endif

  //
  // All events generated after the invocation of EFI_TCG2_GET_EVENT_LOG
  // SHALL be stored in an instance of an EFI_CONFIGURATION_TABLE named
  // by the VendorGuid of EFI_TCG2_FINAL_EVENTS_TABLE_GUID.
  //
  mArmCcaDxeData.GetEventLogCalled[Index] = TRUE;

  return EFI_SUCCESS;
}

/**
  Return if this is a Tcg800155PlatformIdEvent.

  @param[in]      NewEventHdr         Pointer to a TCG_PCR_EVENT_HDR/
                                      TCG_PCR_EVENT_EX data structure.
  @param[in]      NewEventHdrSize     New event header size.
  @param[in]      NewEventData        Pointer to the new event data.
  @param[in]      NewEventSize        New event data size.

  @retval TRUE   This is a Tcg800155PlatformIdEvent.
  @retval FALSE  This is NOT a Tcg800155PlatformIdEvent.

**/
STATIC
BOOLEAN
EFIAPI
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
  @param[in]      NewEventHdr         Pointer to a TCG_PCR_EVENT_HDR/
                                      TCG_PCR_EVENT_EX data structure.
  @param[in]      NewEventHdrSize     New event header size.
  @param[in]      NewEventData        Pointer to the new event data.
  @param[in]      NewEventSize        New event data size.

  @retval EFI_SUCCESS           The new event log entry was added.
  @retval EFI_OUT_OF_RESOURCES  No enough memory to log the new event.

**/
STATIC
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
  DEBUG ((DEBUG_VERBOSE, "CC: Try to log event. Index = %d, EventType = 0x%x\n", CcEventHdr->MrIndex, CcEventHdr->EventType));

  if (NewEventSize > MAX_ADDRESS -  NewEventHdrSize) {
    return EFI_OUT_OF_RESOURCES;
  }

  NewLogSize = NewEventHdrSize + NewEventSize;

  DEBUG ((DEBUG_INFO, "  NewLogSize - 0x%x\n", NewLogSize));

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
  According to UEFI Spec 2.10 Section 38.4.3:
  The following table shows the TPM PCR index mapping and CC event log
  measurement register index interpretation for Arm CCA where:
    - RIM means Realm Initial Measurement Register and
    - REM means Realm Extensible Measurement Register

  The table below depicts the mapping of TPM PCRs, CC Measurement Register
  indices and ARM CCA measurement registers. It also depicts the mapping
  of the TDX measurement registers for comparison.

   TPM PCR Index | CC Measurement  | TDX-measurement | Arm CCA-measurement
                 | Register Index  |  register       |  register
   ------------------------------------------------------------------------
   0             |   0             |   MRTD          |   RIM
   1, 7          |   1             |   RTMR[0]       |   REM[0]
   2~6           |   2             |   RTMR[1]       |   REM[1]
   8~15          |   3             |   RTMR[2]       |   REM[2]

  @param[in] PCRIndex   Index of the TPM PCR

  @retval    UINT32               Index of the CC Event Log Measurement
                                  Register Index
  @retval    ARMCCA_MR_INDEX_INVALID  Invalid MR Index
**/
STATIC
UINT32
EFIAPI
MapPcrToMrIndex (
  IN  UINT32  PCRIndex
  )
{
  UINT32  MrIndex;

  if (PCRIndex > 15) {
    ASSERT (FALSE);
    return ARMCCA_MR_INDEX_INVALID;
  }

  MrIndex = 0;
  if (PCRIndex == 0) {
    ASSERT (0);
    MrIndex = ARMCCA_MR_INDEX_0_RIM;
  } else if ((PCRIndex == 1) || (PCRIndex == 7)) {
    MrIndex = ARMCCA_MR_INDEX_1_REM0;
  } else if ((PCRIndex >= 2) && (PCRIndex <= 6)) {
    MrIndex = ARMCCA_MR_INDEX_2_REM1;
  } else if ((PCRIndex >= 8) && (PCRIndex <= 15)) {
    MrIndex = ARMCCA_MR_INDEX_3_REM2;
  } else {
    ASSERT (0);
    MrIndex = ARMCCA_MR_INDEX_INVALID;
  }

  return MrIndex;
}

/**
  Map TPM PCR to Measurement Register Index

  The EFI_CC_MEASUREMENT_PROTOCOL MapPcrToMrIndex function call provides callers
  the info on TPM PCR <-> CC MR mapping information.

  @param[in]  This               Pointer to the CC Measurement Protocol
  @param[in]  PCRIndex           TPM PCR index.
  @param[out] MrIndex            CC MR index.

  @retval EFI_SUCCESS            The MrIndex is returned.
  @retval EFI_INVALID_PARAMETER  The MrIndex is NULL.
  @retval EFI_UNSUPPORTED        The PcrIndex is invalid.

**/
EFI_STATUS
EFIAPI
ArmCcaMapPcrToMrIndex (
  IN  EFI_CC_MEASUREMENT_PROTOCOL  *This,
  IN  UINT32                       PCRIndex,
  OUT UINT32                       *MrIndex
  )
{
  if (MrIndex == NULL) {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  *MrIndex = MapPcrToMrIndex (PCRIndex);

  return *MrIndex == ARMCCA_MR_INDEX_INVALID ? EFI_UNSUPPORTED : EFI_SUCCESS;
}

/**
  Add a new entry to the Event Log.

  @param[in] EventLogFormat  The type of the event log for which the
                             information is requested.
  @param[in] NewEventHdr     Pointer to a TCG_PCR_EVENT_HDR/TCG_PCR_EVENT_EX
                             data structure.
  @param[in] NewEventHdrSize New event header size.
  @param[in] NewEventData    Pointer to the new event data.
  @param[in] NewEventSize    New event data size.

  @retval EFI_SUCCESS           The new event log entry was added.
  @retval EFI_OUT_OF_RESOURCES  No enough memory to log the new event.

**/
STATIC
EFI_STATUS
EFIAPI
ArmCcaDxeLogEvent (
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
  EventLogAreaStruct = &mArmCcaDxeData.EventLogAreaStruct[Index];

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
  if (mArmCcaDxeData.GetEventLogCalled[Index]) {
    if (mArmCcaDxeData.FinalEventsTable[Index] == NULL) {
      //
      // no need for FinalEventsTable
      //
      return EFI_SUCCESS;
    }

    EventLogAreaStruct = &mArmCcaDxeData.FinalEventLogAreaStruct[Index];

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
      (mArmCcaDxeData.FinalEventsTable[Index])->NumberOfEvents++;
      DEBUG ((DEBUG_INFO, "FinalEventsTable->NumberOfEvents - 0x%x\n", (mArmCcaDxeData.FinalEventsTable[Index])->NumberOfEvents));
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
STATIC
UINT32
EFIAPI
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
  Add a new entry to the Event Log. The call chain is like below:
  ArmCcaDxeLogHashEvent -> ArmCcaDxeLogEvent -> TcgCommonLogEvent

  Before this function is called, the event information (including the digest)
  is ready.

  @param[in]     DigestList    A list of digest.
  @param[in,out] NewEventHdr   Pointer to a CC_EVENT_HDR data structure.
  @param[in]     NewEventData  Pointer to the new event data.

  @retval EFI_SUCCESS           The new event log entry was added.
  @retval EFI_OUT_OF_RESOURCES  No enough memory to log the new event.
**/
STATIC
EFI_STATUS
EFIAPI
ArmCcaDxeLogHashEvent (
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
  TPM_ALG_ID               AlgoId;

  AlgoId = RealmHashAlgoToTpmAlgoId (mArmCcaDxeData.RealmRemHashAlgorithm);
  if (AlgoId == TPM_ALG_ERROR) {
    return EFI_UNSUPPORTED;
  }

  RetStatus = EFI_SUCCESS;
  LogFormat = EFI_CC_EVENT_LOG_FORMAT_TCG_2;

  ZeroMem (&CcEvent, sizeof (CcEvent));
  CcEvent.MrIndex   = NewEventHdr->MrIndex;
  CcEvent.EventType = NewEventHdr->EventType;
  DigestBuffer      = (UINT8 *)&CcEvent.Digests;
  EventSizePtr      = CopyDigestListToBuffer (
                        DigestBuffer,
                        DigestList,
                        GetHashMaskFromAlgo (AlgoId)
                        );
  CopyMem (EventSizePtr, &NewEventHdr->EventSize, sizeof (NewEventHdr->EventSize));

  //
  // Enter critical region
  //
  OldTpl = gBS->RaiseTPL (TPL_HIGH_LEVEL);
  Status = ArmCcaDxeLogEvent (
             LogFormat,
             &CcEvent,
             sizeof (CcEvent.MrIndex) + sizeof (CcEvent.EventType) +
             GetDigestListBinSize (DigestBuffer) + sizeof (CcEvent.EventSize),
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
  Do a hash operation on a data buffer, extend a specific REM with the hash
  result, and add an entry to the Event Log.

  @param[in]      Flags         Bitmap providing additional information.
  @param[in]      HashData      Physical address of the start of the data buffer
                                to be hashed, extended, and logged.
  @param[in]      HashDataLen   The length, in bytes, of the buffer referenced
                                by HashData
  @param[in, out] NewEventHdr   Pointer to a CC_EVENT_HDR data structure.
  @param[in]      NewEventData  Pointer to the new event data.

  @retval EFI_SUCCESS           Operation completed successfully.
  @retval EFI_OUT_OF_RESOURCES  No enough memory to log the new event.
  @retval EFI_DEVICE_ERROR      The command was unsuccessful.

**/
STATIC
EFI_STATUS
EFIAPI
ArmCcaDxeHashLogExtendEvent (
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

  //
  // The section '10.4.5 EV_NO_ACTION Event Types' of the "TCG PC Client
  // Platform Firmware Profile Specification, Level 00 Version 1.06
  // Revision 52 Family “2.0” - December 4, 2023" states the following
  // 'An EV_NO_ACTION event will not result in a digest being extended
  // into a PCR'
  //
  if (NewEventHdr->EventType == EV_NO_ACTION) {
    //
    // Do not do extend to REM for EV_NO_ACTION
    //
    Status = EFI_SUCCESS;
    InitNoActionEvent (&NoActionEvent, NewEventHdr->EventSize);
    if ((Flags & EFI_CC_FLAG_EXTEND_ONLY) == 0) {
      Status = ArmCcaDxeLogHashEvent (
                 &(NoActionEvent.Digests),
                 NewEventHdr,
                 NewEventData
                 );
    }

    return Status;
  }

  //
  // According to UEFI Spec 2.10 Section 38.4.3 the mapping between MrIndex and
  // Arm CCA Realm Measurement register is:
  //    MrIndex 0   <--> RIM
  //    MrIndex 1-3 <--> REM[0-2]
  //
  // This means MrIndex maps 1:1 with Arm CCA Realm Measuement registers.
  // Only the REM registers can be extended by a Arm CCA Guest by calling
  // HashAndExtend.
  // The RIM is read-only and cannot be extended by a Arm CCA Guest. So assert
  // if the MrIndex is not 0.
  //
  ASSERT (NewEventHdr->MrIndex != 0);
  Status = HashAndExtend (
             NewEventHdr->MrIndex,
             HashData,
             (UINTN)HashDataLen,
             &DigestList
             );
  if (!EFI_ERROR (Status)) {
    if ((Flags & EFI_CC_FLAG_EXTEND_ONLY) == 0) {
      Status = ArmCcaDxeLogHashEvent (&DigestList, NewEventHdr, NewEventData);
    }
  }

  return Status;
}

/**
  The EFI_CC_MEASUREMENT_PROTOCOL HashLogExtendEvent function call provides
  callers with an opportunity to extend and optionally log events without
  requiring knowledge of actual TPM commands.
  The extend operation will occur even if this function cannot create an event
  log entry (e.g. due to the event log being full).

  @param[in]  This               Indicates the calling context
  @param[in]  Flags              Bitmap providing additional information.
  @param[in]  DataToHash         Physical address of the start of the data
                                 buffer to be hashed.
  @param[in]  DataToHashLen      The length in bytes of the buffer referenced
                                 by DataToHash.
  @param[in]  CcEvent            Pointer to data buffer containing information
                                 about the event.

  @retval EFI_SUCCESS            Operation completed successfully.
  @retval EFI_DEVICE_ERROR       The command was unsuccessful.
  @retval EFI_VOLUME_FULL        The extend operation occurred, but the event
                                 could not be written to one or more event logs.
  @retval EFI_INVALID_PARAMETER  One or more of the parameters are incorrect.
  @retval EFI_UNSUPPORTED        The PE/COFF image type is not supported.
**/
EFI_STATUS
EFIAPI
ArmCcaHashLogExtendEvent (
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

  DEBUG ((DEBUG_VERBOSE, "ArmCcaHashLogExtendEvent ...\n"));

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

  if (CcEvent->Header.MrIndex == ARMCCA_MR_INDEX_0_RIM) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: RIM cannot be extended in Realm Guest Firmware.\n",
      __func__
      ));
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  if (CcEvent->Header.MrIndex >= ARMCCA_MR_INDEX_INVALID) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: MrIndex is invalid. (%d)\n",
      __func__,
      CcEvent->Header.MrIndex
      ));
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  NewEventHdr.MrIndex   = CcEvent->Header.MrIndex;
  NewEventHdr.EventType = CcEvent->Header.EventType;
  NewEventHdr.EventSize = CcEvent->Size - sizeof (UINT32)
                          - CcEvent->Header.HeaderSize;
  if ((Flags & EFI_CC_FLAG_PE_COFF_IMAGE) != 0) {
    //
    // According to UEFI Spec 2.10 Section 38.4.3 the mapping between MrIndex
    // and the Arm CCA Realm Measurement register is:
    //    MrIndex 0   <--> RIM
    //    MrIndex 1-3 <--> REM[0-2]
    //
    // This means MrIndex maps 1:1 with Arm CCA Realm Measuement registers.
    // Only the REM registers can be extended by a Arm CCA Guest by calling
    // HashAndExtend.
    // The RIM is read-only and cannot be extended by a Arm CCA Guest. So assert
    // if the MrIndex is not 0.
    //
    ASSERT (NewEventHdr.MrIndex != 0);
    Status = MeasurePeImageAndExtend (
               NewEventHdr.MrIndex,
               DataToHash,
               (UINTN)DataToHashLen,
               &DigestList
               );
    if (!EFI_ERROR (Status)) {
      if ((Flags & EFI_CC_FLAG_EXTEND_ONLY) == 0) {
        Status = ArmCcaDxeLogHashEvent (
                   &DigestList,
                   &NewEventHdr,
                   CcEvent->Event
                   );
      }
    }
  } else {
    Status = ArmCcaDxeHashLogExtendEvent (
               Flags,
               (UINT8 *)(UINTN)DataToHash,
               DataToHashLen,
               &NewEventHdr,
               CcEvent->Event
               );
  }

  DEBUG ((DEBUG_VERBOSE, "ArmCcaHashLogExtendEvent - %r\n", Status));
  return Status;
}

/* The instance of the CC Measurement Protocol.
*/
STATIC EFI_CC_MEASUREMENT_PROTOCOL  mArmCcaCcMeasurementProtocol = {
  ArmCcaGetCapability,
  ArmCcaGetEventLog,
  ArmCcaHashLogExtendEvent,
  ArmCcaMapPcrToMrIndex,
};

#define ARM_CCA_HASH_COUNT  1
#define TEMP_BUF_LEN        (sizeof(TCG_EfiSpecIDEventStruct)\
                        + sizeof(UINT32) \
                        + (ARM_CCA_HASH_COUNT  \
                            * sizeof(TCG_EfiSpecIdEventAlgorithmSize))  \
                        + sizeof(UINT8))

/**
  Initialize the CC Event Log and log events passed from the PEI phase.

  @retval EFI_SUCCESS           Operation completed successfully.
  @retval EFI_OUT_OF_RESOURCES  Out of memory.

**/
STATIC
EFI_STATUS
EFIAPI
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
  TPM_ALG_ID                       AlgoId;

  Status = EFI_SUCCESS;
  DEBUG ((DEBUG_INFO, "SetupCcEventLog\n"));

  AlgoId = RealmHashAlgoToTpmAlgoId (mArmCcaDxeData.RealmRemHashAlgorithm);
  if (AlgoId == TPM_ALG_ERROR) {
    return EFI_UNSUPPORTED;
  }

  Index     = 0;
  LogFormat = EFI_CC_EVENT_LOG_FORMAT_TCG_2;

  //
  // 1. Create Log Area
  //
  mArmCcaDxeData.EventLogAreaStruct[Index].EventLogFormat = LogFormat;

  // Allocate pages for Arm CCA Event log
  Status = gBS->AllocatePages (
                  AllocateAnyPages,
                  EfiACPIMemoryNVS,
                  EFI_SIZE_TO_PAGES (PcdGet32 (PcdTcgLogAreaMinLen)),
                  &Lasa
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  mArmCcaDxeData.EventLogAreaStruct[Index].Lasa = Lasa;
  mArmCcaDxeData.EventLogAreaStruct[Index].Laml =
    PcdGet32 (PcdTcgLogAreaMinLen);
  mArmCcaDxeData.EventLogAreaStruct[Index].Next800155EventOffset = 0;

  //
  // Report CC event log address and length, so that they can be reported in
  // the CCEL ACPI table. Ignore the return status, because those fields are
  // optional.
  //
  PcdSet32S (
    PcdCcEventlogAcpiTableLaml,
    (UINT32)mArmCcaDxeData.EventLogAreaStruct[Index].Laml
    );
  PcdSet64S (
    PcdCcEventlogAcpiTableLasa,
    mArmCcaDxeData.EventLogAreaStruct[Index].Lasa
    );

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
  CopyMem (
    TcgEfiSpecIdEventStruct->signature,
    TCG_EfiSpecIDEventStruct_SIGNATURE_03,
    sizeof (TcgEfiSpecIdEventStruct->signature)
    );

  TcgEfiSpecIdEventStruct->platformClass = PcdGet8 (PcdTpmPlatformClass);

  TcgEfiSpecIdEventStruct->specVersionMajor =
    TCG_EfiSpecIDEventStruct_SPEC_VERSION_MAJOR_TPM2;
  TcgEfiSpecIdEventStruct->specVersionMinor =
    TCG_EfiSpecIDEventStruct_SPEC_VERSION_MINOR_TPM2;
  TcgEfiSpecIdEventStruct->specErrata =
    TCG_EfiSpecIDEventStruct_SPEC_ERRATA_TPM2;
  TcgEfiSpecIdEventStruct->uintnSize =
    sizeof (UINTN)/sizeof (UINT32);
  NumberOfAlgorithms = 0;
  DigestSize         =
    (TCG_EfiSpecIdEventAlgorithmSize *)((UINT8 *)TcgEfiSpecIdEventStruct
                                        + sizeof (*TcgEfiSpecIdEventStruct)
                                        + sizeof (NumberOfAlgorithms));

  TempDigestSize              = DigestSize;
  TempDigestSize             += NumberOfAlgorithms;
  TempDigestSize->algorithmId = AlgoId;
  TempDigestSize->digestSize  = GetHashSizeFromAlgo (AlgoId);
  NumberOfAlgorithms++;

  CopyMem (
    TcgEfiSpecIdEventStruct + 1,
    &NumberOfAlgorithms,
    sizeof (NumberOfAlgorithms)
    );
  TempDigestSize  = DigestSize;
  TempDigestSize += NumberOfAlgorithms;
  VendorInfoSize  = (UINT8 *)TempDigestSize;
  *VendorInfoSize = 0;

  //
  // Referring to "TCG PC Client Platform Firmware Profile Specification,
  // Level 00 Version 1.06 Revision 52 Family “2.0” - December 4, 2023",
  // the 'Table 9 TCG_EfiSpecIdEvent Example' describes a TCG_EfiSpecIdEvent.
  // It also states "Information events have an event type of value
  // EV_NO_ACTION and PCR index of zero (See Section 10.4.5 EV_NO_ACTION
  // Event Types)".
  // Furthermore, section '10.4.5.1 Specification ID Version Event' states
  // "The first event in the event log is the Specification ID version. This
  // event is not extended to a PCR."
  SpecIdEvent.PCRIndex  = 0;
  SpecIdEvent.EventType = EV_NO_ACTION;
  ZeroMem (&SpecIdEvent.Digest, sizeof (SpecIdEvent.Digest));
  SpecIdEvent.EventSize = (UINT32)GetTcgEfiSpecIdEventStructSize (
                                    TcgEfiSpecIdEventStruct
                                    );

  //
  // Re-use the TCG2 Event log specification for Arm CCA Event Logs.
  // Log TcgEfiSpecIdEventStruct as the first Event.
  //  Event format is TCG_PCR_EVENT.
  //  TCG EFI Protocol Spec. Section 5.3 Event Log Header
  //  TCG PC Client PFP spec. Section 9.2 Measurement Event Entries and Log
  //
  Status = ArmCcaDxeLogEvent (
             LogFormat,
             &SpecIdEvent,
             sizeof (SpecIdEvent),
             (UINT8 *)TcgEfiSpecIdEventStruct,
             SpecIdEvent.EventSize
             );
  if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
    return Status;
  }

  //
  // record the offset at the end of 800-155 event.
  // the future 800-155 event can be inserted here.
  //
  mArmCcaDxeData.EventLogAreaStruct[Index].Next800155EventOffset =
    mArmCcaDxeData.EventLogAreaStruct[Index].EventLogSize;

  //
  // Tcg800155PlatformIdEvent. Event format is TCG_PCR_EVENT2
  //
  GuidHob.Guid = GetFirstGuidHob (&gTcg800155PlatformIdEventHobGuid);
  while (GuidHob.Guid != NULL) {
    InitNoActionEvent (&NoActionEvent, GET_GUID_HOB_DATA_SIZE (GuidHob.Guid));

    Status = ArmCcaDxeLogEvent (
               LogFormat,
               &NoActionEvent,
               sizeof (NoActionEvent.MrIndex) +
               sizeof (NoActionEvent.EventType) +
               GetDigestListBinSize (&NoActionEvent.Digests) +
               sizeof (NoActionEvent.EventSize),
               GET_GUID_HOB_DATA (GuidHob.Guid),
               GET_GUID_HOB_DATA_SIZE (GuidHob.Guid)
               );

    GuidHob.Guid = GET_NEXT_HOB (GuidHob);
    GuidHob.Guid = GetNextGuidHob (
                     &gTcg800155PlatformIdEventHobGuid,
                     GuidHob.Guid
                     );
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
  mArmCcaDxeData.FinalEventsTable[Index] =
    (VOID *)(UINTN)Lasa;
  (mArmCcaDxeData.FinalEventsTable[Index])->Version =
    EFI_TCG2_FINAL_EVENTS_TABLE_VERSION;
  (mArmCcaDxeData.FinalEventsTable[Index])->NumberOfEvents = 0;

  mArmCcaDxeData.FinalEventLogAreaStruct[Index].EventLogFormat =
    LogFormat;
  mArmCcaDxeData.FinalEventLogAreaStruct[Index].Lasa =
    Lasa + sizeof (EFI_CC_FINAL_EVENTS_TABLE);
  mArmCcaDxeData.FinalEventLogAreaStruct[Index].Laml =
    PcdGet32 (PcdTcg2FinalLogAreaLen) - sizeof (EFI_CC_FINAL_EVENTS_TABLE);
  mArmCcaDxeData.FinalEventLogAreaStruct[Index].EventLogSize = 0;
  mArmCcaDxeData.FinalEventLogAreaStruct[Index].LastEvent    =
    (VOID *)(UINTN)mArmCcaDxeData.FinalEventLogAreaStruct[Index].Lasa;
  mArmCcaDxeData.FinalEventLogAreaStruct[Index].EventLogStarted       = FALSE;
  mArmCcaDxeData.FinalEventLogAreaStruct[Index].EventLogTruncated     = FALSE;
  mArmCcaDxeData.FinalEventLogAreaStruct[Index].Next800155EventOffset = 0;

  //
  // Install to configuration table for EFI_CC_EVENT_LOG_FORMAT_TCG_2
  //
  Status = gBS->InstallConfigurationTable (
                  &gEfiCcFinalEventsTableGuid,
                  (VOID *)mArmCcaDxeData.FinalEventsTable[Index]
                  );
  ASSERT_EFI_ERROR (Status);

  DEBUG ((DEBUG_INFO, "SetupCcEventLog - END\n"));
  return Status;
}

/**
  Measure and log an action string, and extend the measurement result to a REM.

  @param[in] MrIndex          MrIndex to extend
  @param[in] String           A specific string that indicates an Action event.

  @retval EFI_SUCCESS         Operation completed successfully.
  @retval EFI_DEVICE_ERROR    The operation was unsuccessful.

**/
STATIC
EFI_STATUS
EFIAPI
ArmCcaMeasureAction (
  IN      UINT32  MrIndex,
  IN      CHAR8   *String
  )
{
  CC_EVENT_HDR  CcEvent;

  CcEvent.MrIndex   = MrIndex;
  CcEvent.EventType = EV_EFI_ACTION;
  CcEvent.EventSize = (UINT32)AsciiStrLen (String);
  return ArmCcaDxeHashLogExtendEvent (
           0,
           (UINT8 *)String,
           CcEvent.EventSize,
           &CcEvent,
           (UINT8 *)String
           );
}

/**
  Measure and log EFI handoff tables, and extend the measurement result
  to PCR[1].

  @retval EFI_SUCCESS         Operation completed successfully.
  @retval EFI_DEVICE_ERROR    The operation was unsuccessful.

**/
STATIC
EFI_STATUS
EFIAPI
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
    // Measure each processor EFI_CPU_PHYSICAL_LOCATION with
    // EV_TABLE_OF_DEVICES to PCR[1]
    //
    Status = GetProcessorsCpuLocation (&ProcessorLocBuf, &ProcessorNum);
    if (!EFI_ERROR (Status)) {
      CcEvent.MrIndex   = MapPcrToMrIndex (1);
      CcEvent.EventType = EV_TABLE_OF_DEVICES;
      CcEvent.EventSize = sizeof (HandoffTables);

      HandoffTables.NumberOfTables            = 1;
      HandoffTables.TableEntry[0].VendorGuid  = gEfiMpServiceProtocolGuid;
      HandoffTables.TableEntry[0].VendorTable = ProcessorLocBuf;

      Status = ArmCcaDxeHashLogExtendEvent (
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
  Measure and log Separator event, and extend the measurement result to
  a specific REM.

  @param[in] MrIndex         The measurement register index.

  @retval EFI_SUCCESS         Operation completed successfully.
  @retval EFI_DEVICE_ERROR    The operation was unsuccessful.

**/
STATIC
EFI_STATUS
EFIAPI
MeasureSeparatorEvent (
  IN      UINT32  MrIndex
  )
{
  CC_EVENT_HDR  CcEvent;
  UINT32        EventData;

  DEBUG ((DEBUG_INFO, "MeasureSeparatorEvent to REM - %d\n", MrIndex));

  EventData         = 0;
  CcEvent.MrIndex   = MrIndex;
  CcEvent.EventType = EV_SEPARATOR;
  CcEvent.EventSize = (UINT32)sizeof (EventData);

  return ArmCcaDxeHashLogExtendEvent (
           0,
           (UINT8 *)&EventData,
           sizeof (EventData),
           &CcEvent,
           (UINT8 *)&EventData
           );
}

/**
  Measure and log an EFI variable, and extend the measurement result to
  a specific REM.

  @param[in]  MrIndex           RTMR Index.
  @param[in]  EventType         Event type.
  @param[in]  VarName           A Null-terminated string that is the name
                                of the vendor's variable.
  @param[in]  VendorGuid        A unique identifier for the vendor.
  @param[in]  VarData           The content of the variable data.
  @param[in]  VarSize           The size of the variable data.

  @retval EFI_SUCCESS           Operation completed successfully.
  @retval EFI_OUT_OF_RESOURCES  Out of memory.
  @retval EFI_DEVICE_ERROR      The operation was unsuccessful.

**/
STATIC
EFI_STATUS
EFIAPI
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

  DEBUG ((
    DEBUG_INFO,
    "ArmCcaTcg2Dxe: MeasureVariable (REM - %x, EventType - %x, ",
    (UINTN)MrIndex,
    (UINTN)EventType
    ));
  DEBUG ((
    DEBUG_INFO,
    "VariableName - %s, VendorGuid - %g)\n",
    VarName,
    VendorGuid
    ));

  VarNameLength     = StrLen (VarName);
  CcEvent.MrIndex   = MrIndex;
  CcEvent.EventType = EventType;

  CcEvent.EventSize = (UINT32)(sizeof (*VarLog) +
                               VarNameLength * sizeof (*VarName) + VarSize
                               - sizeof (VarLog->UnicodeName)
                               - sizeof (VarLog->VariableData));

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
    Status = ArmCcaDxeHashLogExtendEvent (
               0,
               (UINT8 *)VarLog,
               CcEvent.EventSize,
               &CcEvent,
               (UINT8 *)VarLog
               );
  } else {
    ASSERT (VarData != NULL);
    Status = ArmCcaDxeHashLogExtendEvent (
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
  Read then Measure and log an EFI variable, and extend the measurement result
  to a specific REM.

  @param[in]  MrIndex           RTMR Index.
  @param[in]  EventType         Event type.
  @param[in]   VarName          A Null-terminated string that is the name
                                of the vendor's variable.
  @param[in]   VendorGuid       A unique identifier for the vendor.
  @param[out]  VarSize          The size of the variable data.
  @param[out]  VarData          Pointer to the content of the variable.

  @retval EFI_SUCCESS           Operation completed successfully.
  @retval EFI_OUT_OF_RESOURCES  Out of memory.
  @retval EFI_DEVICE_ERROR      The operation was unsuccessful.

**/
STATIC
EFI_STATUS
EFIAPI
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
  Read then Measure and log an EFI boot variable, and extend the measurement
  result into PCR[1] according to TCG PC Client PFP spec 0021 Section 2.4.4.2.

  @param[in]   VarName          A Null-terminated string that is the name
                                of the vendor's variable.
  @param[in]   VendorGuid       A unique identifier for the vendor.
  @param[out]  VarSize          The size of the variable data.
  @param[out]  VarData          Pointer to the content of the variable.

  @retval EFI_SUCCESS           Operation completed successfully.
  @retval EFI_OUT_OF_RESOURCES  Out of memory.
  @retval EFI_DEVICE_ERROR      The operation was unsuccessful.

**/
STATIC
EFI_STATUS
EFIAPI
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
  Read then Measure and log an EFI Secure variable, and extend the measurement
  result into PCR[7].

  @param[in]   VarName          A Null-terminated string that is the name
                                of the vendor's variable.
  @param[in]   VendorGuid       A unique identifier for the vendor.
  @param[out]  VarSize          The size of the variable data.
  @param[out]  VarData          Pointer to the content of the variable.

  @retval EFI_SUCCESS           Operation completed successfully.
  @retval EFI_OUT_OF_RESOURCES  Out of memory.
  @retval EFI_DEVICE_ERROR      The operation was unsuccessful.

**/
STATIC
EFI_STATUS
EFIAPI
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
  Measure and log all EFI boot variables, and extend the measurement result
  to a specific PCR.

  The EFI boot variables are BootOrder and Boot#### variables.

  @retval EFI_SUCCESS           Operation completed successfully.
  @retval EFI_OUT_OF_RESOURCES  Out of memory.
  @retval EFI_DEVICE_ERROR      The operation was unsuccessful.

**/
STATIC
EFI_STATUS
EFIAPI
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
    UnicodeSPrint (
      mBootVarName,
      sizeof (mBootVarName),
      L"Boot%04x",
      BootOrder[Index]
      );
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
  Measure and log all EFI Secure variables, and extend the measurement result
  to a specific PCR.

  The EFI boot variables are BootOrder and Boot#### variables.

  @retval EFI_SUCCESS           Operation completed successfully.
  @retval EFI_OUT_OF_RESOURCES  Out of memory.
  @retval EFI_DEVICE_ERROR      The operation was unsuccessful.

**/
STATIC
EFI_STATUS
EFIAPI
MeasureAllSecureVariables (
  VOID
  )
{
  EFI_STATUS  Status;
  VOID        *Data;
  UINTN       DataSize;
  UINTN       Index;

  Status = EFI_NOT_FOUND;
  for (Index = 0; Index < ARRAY_SIZE (mVariableType); Index++) {
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
  Status = GetVariable2 (
             EFI_IMAGE_SECURITY_DATABASE2,
             &gEfiImageSecurityDatabaseGuid,
             &Data,
             &DataSize
             );
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
    DEBUG ((
      DEBUG_WARN,
      "Skip measuring variable %s since it's deleted\n",
      EFI_IMAGE_SECURITY_DATABASE2
      ));
  }

  return EFI_SUCCESS;
}

/**
  Measure and log launch of FirmwareDebugger, and extend the measurement result
  to a specific PCR.

  @retval EFI_SUCCESS           Operation completed successfully.
  @retval EFI_OUT_OF_RESOURCES  Out of memory.
  @retval EFI_DEVICE_ERROR      The operation was unsuccessful.

**/
STATIC
EFI_STATUS
EFIAPI
MeasureLaunchOfFirmwareDebugger (
  VOID
  )
{
  CC_EVENT_HDR  CcEvent;

  CcEvent.MrIndex   = MapPcrToMrIndex (7);
  CcEvent.EventType = EV_EFI_ACTION;
  CcEvent.EventSize = sizeof (FIRMWARE_DEBUGGER_EVENT_STRING) - 1;
  return ArmCcaDxeHashLogExtendEvent (
           0,
           (UINT8 *)FIRMWARE_DEBUGGER_EVENT_STRING,
           sizeof (FIRMWARE_DEBUGGER_EVENT_STRING) - 1,
           &CcEvent,
           (UINT8 *)FIRMWARE_DEBUGGER_EVENT_STRING
           );
}

/**
  Measure and log all Secure Boot Policy, and extend the measurement result
  to a specific PCR.

  Platform firmware adhering to the policy must therefore measure the
  following values into PCR[7]: (in order listed)
   - The contents of the SecureBoot variable
   - The contents of the PK variable
   - The contents of the KEK variable
   - The contents of the EFI_IMAGE_SECURITY_DATABASE variable
   - The contents of the EFI_IMAGE_SECURITY_DATABASE1 variable
   - Separator
   - Entries in the EFI_IMAGE_SECURITY_DATABASE that are used to validate
     EFI Drivers or EFI Boot Applications in the boot path

  NOTE: Because of the above, UEFI variables PK, KEK,
  EFI_IMAGE_SECURITY_DATABASE, EFI_IMAGE_SECURITY_DATABASE1 and
  SecureBoot SHALL NOT be measured into PCR[3].

  @param[in]  Event     Event whose notification function is being invoked
  @param[in]  Context   Pointer to the notification function's context
**/
STATIC
VOID
EFIAPI
MeasureSecureBootPolicy (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  EFI_STATUS  Status;
  VOID        *Protocol;

  Status = gBS->LocateProtocol (
                  &gEfiVariableWriteArchProtocolGuid,
                  NULL,
                  (VOID **)&Protocol
                  );
  if (EFI_ERROR (Status)) {
    return;
  }

  if (PcdGetBool (PcdFirmwareDebuggerInitialized)) {
    Status = MeasureLaunchOfFirmwareDebugger ();
    DEBUG ((
      DEBUG_INFO,
      "MeasureLaunchOfFirmwareDebugger - %r\n",
      Status
      ));
  }

  Status = MeasureAllSecureVariables ();
  DEBUG ((DEBUG_INFO, "MeasureAllSecureVariables - %r\n", Status));

  //
  // We need measure Separator(7) here, because this event must be between
  // SecureBootPolicy (Configure) and ImageVerification (Authority)
  // There might be a case that we need measure UEFI image from DriverOrder,
  // besides BootOrder. So the Authority measurement happen before ReadToBoot
  // event.
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

  PERF_START_EX (
    mImageHandle,
    "EventRec",
    "ArmCcaTcg2Dxe",
    0,
    PERF_ID_CC_TCG2_DXE
    );

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
    Status = ArmCcaMeasureAction (
               MapPcrToMrIndex (4),
               EFI_CALLING_EFI_APPLICATION
               );
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "%a not Measured. Error!\n",
        EFI_CALLING_EFI_APPLICATION
        ));
    }

    //
    // 2. Draw a line between pre-boot env and entering post-boot env.
    //    i.e. PCR 0 to 7
    // According to UEFI Spec 2.10 Section 38.4.3 the mapping between
    // MrIndex and Arm CCA Measurement Register is:
    //    PCR[0]   <--> MrIndex 0   <--> RIM
    //    PCR[1,7] <--> MrIndex 1   <--> REM[0]    - PCR[7] already done.
    //    PCR[2,6] <--> MrIndex 2   <--> REM[1]
    // So SepartorEvent shall be extended to REM[1] (i.e. MrIndex 2).
    //
    Status = MeasureSeparatorEvent (ARMCCA_MR_INDEX_2_REM1);
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "Separator Event not Measured to REM[1]. Error!\n"
        ));
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
    Status = ArmCcaMeasureAction (
               MapPcrToMrIndex (4),
               EFI_RETURNING_FROM_EFI_APPLICATION
               );
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "%a not Measured. Error!\n",
        EFI_RETURNING_FROM_EFI_APPLICATION
        ));
    }

    //
    // 7. Next boot attempt, measure "Calling EFI Application from Boot Option"
    // again.
    // TCG PC Client PFP spec Section 2.4.4.5 Step 4
    //
    Status = ArmCcaMeasureAction (
               MapPcrToMrIndex (4),
               EFI_CALLING_EFI_APPLICATION
               );
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "%a not Measured. Error!\n",
        EFI_CALLING_EFI_APPLICATION
        ));
    }
  }

  DEBUG ((DEBUG_INFO, "ArmCcaTcg2Dxe Measure Data when ReadyToBoot\n"));
  //
  // Increase boot attempt counter.
  //
  mBootAttempts++;
  PERF_END_EX (
    mImageHandle,
    "EventRec",
    "ArmCcaTcg2Dxe",
    0,
    PERF_ID_CC_TCG2_DXE + 1
    );
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
  Status = ArmCcaMeasureAction (
             MapPcrToMrIndex (5),
             EFI_EXIT_BOOT_SERVICES_INVOCATION
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a not Measured. Error!\n",
      EFI_EXIT_BOOT_SERVICES_INVOCATION
      ));
  }

  //
  // Measure success of ExitBootServices
  //
  Status = ArmCcaMeasureAction (
             MapPcrToMrIndex (5),
             EFI_EXIT_BOOT_SERVICES_SUCCEEDED
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a not Measured. Error!\n",
      EFI_EXIT_BOOT_SERVICES_SUCCEEDED
      ));
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
  Status = ArmCcaMeasureAction (
             MapPcrToMrIndex (5),
             EFI_EXIT_BOOT_SERVICES_FAILED
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a not Measured. Error!\n",
      EFI_EXIT_BOOT_SERVICES_FAILED
      ));
  }
}

/**
  Synchronise the CC event logs from the SEC phase.

  @retval EFI_SUCCESS           Operation completed successfully.
  @retval EFI_OUT_OF_RESOURCES  Out of memory.

**/
STATIC
EFI_STATUS
EFIAPI
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
    CcEvent = AllocateCopyPool (
                GET_GUID_HOB_DATA_SIZE (GuidHob.Guid),
                GET_GUID_HOB_DATA (GuidHob.Guid)
                );
    if (CcEvent == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    GuidHob.Guid = GET_NEXT_HOB (GuidHob);
    GuidHob.Guid = GetNextGuidHob (&gCcEventEntryHobGuid, GuidHob.Guid);

    DigestListBin = (UINT8 *)CcEvent + sizeof (UINT32) +
                    sizeof (TCG_EVENTTYPE);
    DigestListBinSize = GetDigestListBinSize (DigestListBin);

    //
    // Event size.
    //
    EventSize = *(UINT32 *)((UINT8 *)DigestListBin + DigestListBinSize);
    Event     = (UINT8 *)DigestListBin + DigestListBinSize + sizeof (UINT32);

    //
    // Log the event
    //
    Status = ArmCcaDxeLogEvent (
               LogFormat,
               CcEvent,
               sizeof (UINT32) + sizeof (TCG_EVENTTYPE) + DigestListBinSize +
               sizeof (UINT32),
               Event,
               EventSize
               );

 #ifdef DUMP_EVENT_LOGS
    DumpCcEvent ((CC_EVENT *)CcEvent);
 #endif
    FreePool (CcEvent);
  } // while

  DEBUG ((DEBUG_INFO, "Sync Cc event from SEC - END\n"));
  return Status;
}

/**
  Install CCEL ACPI Table when ACPI Table Protocol is available.

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

  Status = gBS->LocateProtocol (
                  &gEfiAcpiTableProtocolGuid,
                  NULL,
                  (VOID **)&AcpiTable
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "CC: AcpiTableProtocol is not installed. %r\n",
      Status
      ));
    return;
  }

  mArmCcaEventlogAcpiTemplate.Laml = (UINT64)PcdGet32 (
                                               PcdCcEventlogAcpiTableLaml
                                               );
  mArmCcaEventlogAcpiTemplate.Lasa = PcdGet64 (PcdCcEventlogAcpiTableLasa);
  CopyMem (
    mArmCcaEventlogAcpiTemplate.Header.OemId,
    PcdGetPtr (PcdAcpiDefaultOemId),
    sizeof (mArmCcaEventlogAcpiTemplate.Header.OemId)
    );
  mArmCcaEventlogAcpiTemplate.Header.OemTableId =
    PcdGet64 (PcdAcpiDefaultOemTableId);
  mArmCcaEventlogAcpiTemplate.Header.OemRevision =
    PcdGet32 (PcdAcpiDefaultOemRevision);
  mArmCcaEventlogAcpiTemplate.Header.CreatorId =
    PcdGet32 (PcdAcpiDefaultCreatorId);
  mArmCcaEventlogAcpiTemplate.Header.CreatorRevision =
    PcdGet32 (PcdAcpiDefaultCreatorRevision);

  //
  // Construct ACPI Table
  Status = AcpiTable->InstallAcpiTable (
                        AcpiTable,
                        &mArmCcaEventlogAcpiTemplate,
                        mArmCcaEventlogAcpiTemplate.Header.Length,
                        &TableKey
                        );
  ASSERT_EFI_ERROR (Status);

  DEBUG ((DEBUG_INFO, "CC Event Log ACPI Table is installed.\n"));
}

/**
  The function installs the CC Measurement protocol.

  @retval EFI_SUCCESS     CC Measurement protocol is installed.
  @retval other           Some error occurs.
**/
STATIC
EFI_STATUS
EFIAPI
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
                  &mArmCcaCcMeasurementProtocol,
                  NULL
                  );
  DEBUG ((DEBUG_INFO, "CcProtocol: Install %r\n", Status));
  return Status;
}

/**
  The driver's entry point. It publishes EFI CC Measurement Protocol.

  @param[in] ImageHandle  The firmware allocated handle for the EFI image.
  @param[in] SystemTable  A pointer to the EFI System Table.

  @retval EFI_SUCCESS     The entry point is executed successfully.
  @retval EFI_UNSUPPORTED The execution context is not a Realm VM.
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
  UINT8       RealmHashAlgorithm;
  TPM_ALG_ID  TpmAlgoId;

  if (!ArmCcaIsRealm ()) {
    // Nothing to do as the execution context is not a Realm.
    return EFI_SUCCESS;
  }

  Status = GetRealmHashAlgorithm (&RealmHashAlgorithm);
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    goto ErrorHandler;
  }

  TpmAlgoId = RealmHashAlgoToTpmAlgoId (RealmHashAlgorithm);
  if (TpmAlgoId == TPM_ALG_ERROR) {
    Status = EFI_UNSUPPORTED;
    goto ErrorHandler;
  }

  mImageHandle = ImageHandle;

  //
  // Fill information
  //
  mArmCcaDxeData.BsCap.Size =
    sizeof (EFI_CC_BOOT_SERVICE_CAPABILITY);
  mArmCcaDxeData.BsCap.ProtocolVersion.Major  = 1;
  mArmCcaDxeData.BsCap.ProtocolVersion.Minor  = 0;
  mArmCcaDxeData.BsCap.StructureVersion.Major = 1;
  mArmCcaDxeData.BsCap.StructureVersion.Minor = 0;

  //
  // Get supported PCR and current Active PCRs
  // Arm recommends NIST SP 800-107 SHA-256 as a minimum
  // requirement to be used for all current implementations
  // of CCA. Arm strongly recommends that all current CCA
  // implementations maintain a migration path to SHA-384 or
  // SHA-512.
  //
  mArmCcaDxeData.BsCap.HashAlgorithmBitmap = GetHashMaskFromAlgo (TpmAlgoId);

  // Arm CCA guests only support EFI_TCG2_EVENT_LOG_FORMAT_TCG_2
  mArmCcaDxeData.BsCap.SupportedEventLogs = EFI_CC_EVENT_LOG_FORMAT_TCG_2;

  mArmCcaDxeData.RealmRemHashAlgorithm = RealmHashAlgorithm;

  //
  // Setup the log area and copy event log from hob list to it
  //
  Status = SetupCcEventLog ();
  if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
    goto ErrorHandler;
  }

  Status = SyncCcEvent ();
  if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
    goto ErrorHandler;
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
  if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
    goto ErrorHandler;
  }

  Status = gBS->CreateEventEx (
                  EVT_NOTIFY_SIGNAL,
                  TPL_NOTIFY,
                  OnExitBootServices,
                  NULL,
                  &gEfiEventExitBootServicesGuid,
                  &Event
                  );
  if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
    goto ErrorHandler;
  }

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
  if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
    goto ErrorHandler;
  }

  //
  // Create event callback, because we need access to SecureBootPolicyVariable.
  // We should use VariableWriteArch instead of VariableArch, because Variable
  // driver may update SecureBoot value based on last setting.
  //
  EfiCreateProtocolNotifyEvent (
    &gEfiVariableWriteArchProtocolGuid,
    TPL_CALLBACK,
    MeasureSecureBootPolicy,
    NULL,
    &Registration
    );

  //
  // Install CcMeasurementProtocol
  //
  Status = InstallCcMeasurementProtocol ();
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: CC Measurement protocol failed to be installed - %r\n",
      __func__,
      Status
      ));
    goto ErrorHandler;
  }

  //
  // Create event callback to install CC EventLog ACPI Table
  EfiCreateProtocolNotifyEvent (
    &gEfiAcpiTableProtocolGuid,
    TPL_CALLBACK,
    InstallAcpiTable,
    NULL,
    &Registration
    );

  return Status;

ErrorHandler:
  //
  // CC measurement feature is crucial to a Realm-guest and it must stop
  // running immediately if the CC Measurement protocol initialisation fails.
  //
  DEBUG ((
    DEBUG_ERROR,
    "Error: ArmCcaTcg2Dxe - %a: initialisation failed - %r\n",
    __func__,
    Status
    ));
  CpuDeadLoop ();

  return Status;
}
