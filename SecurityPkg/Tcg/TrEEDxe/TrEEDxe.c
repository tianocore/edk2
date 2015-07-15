/** @file
  This module implements TrEE Protocol.
  
Copyright (c) 2013 - 2015, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials 
are licensed and made available under the terms and conditions of the BSD License 
which accompanies this distribution.  The full text of the license may be found at 
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS, 
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <PiDxe.h>
#include <IndustryStandard/Acpi.h>
#include <IndustryStandard/PeImage.h>
#include <IndustryStandard/SmBios.h>
#include <IndustryStandard/TcpaAcpi.h>

#include <Guid/GlobalVariable.h>
#include <Guid/SmBios.h>
#include <Guid/HobList.h>
#include <Guid/TcgEventHob.h>
#include <Guid/EventGroup.h>
#include <Guid/EventExitBootServiceFailed.h>
#include <Guid/ImageAuthentication.h>
#include <Guid/TpmInstance.h>

#include <Protocol/DevicePath.h>
#include <Protocol/AcpiTable.h>
#include <Protocol/MpService.h>
#include <Protocol/VariableWrite.h>
#include <Protocol/TrEEProtocol.h>

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

#define PERF_ID_TREE_DXE  0x3120

typedef struct {
  CHAR16                                 *VariableName;
  EFI_GUID                               *VendorGuid;
} VARIABLE_TYPE;

#define  EFI_TCG_LOG_AREA_SIZE        0x10000

#define  TREE_DEFAULT_MAX_COMMAND_SIZE        0x1000
#define  TREE_DEFAULT_MAX_RESPONSE_SIZE       0x1000

typedef struct {
  EFI_GUID               *EventGuid;
  TREE_EVENT_LOG_FORMAT  LogFormat;
} TREE_EVENT_INFO_STRUCT;

TREE_EVENT_INFO_STRUCT mTreeEventInfo[] = {
  {&gTcgEventEntryHobGuid,             TREE_EVENT_LOG_FORMAT_TCG_1_2},
};

#define TCG_EVENT_LOG_AREA_COUNT_MAX   2

typedef struct {
  TREE_EVENT_LOG_FORMAT             EventLogFormat;
  EFI_PHYSICAL_ADDRESS              Lasa;
  UINT64                            Laml;
  UINTN                             EventLogSize;
  UINT8                             *LastEvent;
  BOOLEAN                           EventLogStarted;
  BOOLEAN                           EventLogTruncated;
} TCG_EVENT_LOG_AREA_STRUCT;

typedef struct _TCG_DXE_DATA {
  TREE_BOOT_SERVICE_CAPABILITY      BsCap;
  EFI_TCG_CLIENT_ACPI_TABLE         *TcgClientAcpiTable;
  EFI_TCG_SERVER_ACPI_TABLE         *TcgServerAcpiTable;
  TCG_EVENT_LOG_AREA_STRUCT         EventLogAreaStruct[TCG_EVENT_LOG_AREA_COUNT_MAX];
} TCG_DXE_DATA;

EFI_TCG_CLIENT_ACPI_TABLE           mTcgClientAcpiTemplate = {
  {
    EFI_ACPI_3_0_TRUSTED_COMPUTING_PLATFORM_ALLIANCE_CAPABILITIES_TABLE_SIGNATURE,
    sizeof (mTcgClientAcpiTemplate),
    0x02                      //Revision
    //
    // Compiler initializes the remaining bytes to 0
    // These fields should be filled in in production
    //
  },
  0,                          // 0 for PC Client Platform Class
  0,                          // Log Area Max Length
  (EFI_PHYSICAL_ADDRESS) (SIZE_4GB - 1)  // Log Area Start Address
};

//
// The following EFI_TCG_SERVER_ACPI_TABLE default setting is just one example,
// the TPM device connectes to LPC, and also defined the ACPI _UID as 0xFF,
// this _UID can be changed and should match with the _UID setting of the TPM 
// ACPI device object  
//
EFI_TCG_SERVER_ACPI_TABLE           mTcgServerAcpiTemplate = {
  {
    EFI_ACPI_3_0_TRUSTED_COMPUTING_PLATFORM_ALLIANCE_CAPABILITIES_TABLE_SIGNATURE,
    sizeof (mTcgServerAcpiTemplate),
    0x02                      //Revision
    //
    // Compiler initializes the remaining bytes to 0
    // These fields should be filled in in production
    //
  },
  1,                          // 1 for Server Platform Class
  0,                          // Reserved
  0,                          // Log Area Max Length
  (EFI_PHYSICAL_ADDRESS) (SIZE_4GB - 1), // Log Area Start Address
  0x0100,                     // TCG Specification revision 1.0
  2,                          // Device Flags
  0,                          // Interrupt Flags
  0,                          // GPE
  {0},                        // Reserved 3 bytes
  0,                          // Global System Interrupt
  {
    EFI_ACPI_3_0_SYSTEM_MEMORY,
    0,
    0,
    EFI_ACPI_3_0_BYTE,
    0x0          // Base Address
  },
  0,                          // Reserved
  {0},                        // Configuration Address
  0xFF,                       // ACPI _UID value of the device, can be changed for different platforms
  0,                          // ACPI _UID value of the device, can be changed for different platforms
  0,                          // ACPI _UID value of the device, can be changed for different platforms
  0                           // ACPI _UID value of the device, can be changed for different platforms
};

TCG_DXE_DATA                 mTcgDxeData = {
  {
    sizeof (TREE_BOOT_SERVICE_CAPABILITY_1_0),     // Size
    { 1, 0 },                       // StructureVersion
    { 1, 0 },                       // ProtocolVersion
    TREE_BOOT_HASH_ALG_SHA1,        // HashAlgorithmBitmap
    TREE_EVENT_LOG_FORMAT_TCG_1_2,  // SupportedEventLogs
    TRUE,                           // TrEEPresentFlag
    TREE_DEFAULT_MAX_COMMAND_SIZE,  // MaxCommandSize
    TREE_DEFAULT_MAX_RESPONSE_SIZE, // MaxResponseSize
    0                               // ManufacturerID
  },
  &mTcgClientAcpiTemplate,
  &mTcgServerAcpiTemplate,
};

UINTN  mBootAttempts  = 0;
CHAR16 mBootVarName[] = L"BootOrder";

VARIABLE_TYPE  mVariableType[] = {
  {EFI_SECURE_BOOT_MODE_NAME,    &gEfiGlobalVariableGuid},
  {EFI_PLATFORM_KEY_NAME,        &gEfiGlobalVariableGuid},
  {EFI_KEY_EXCHANGE_KEY_NAME,    &gEfiGlobalVariableGuid},
  {EFI_IMAGE_SECURITY_DATABASE,  &gEfiImageSecurityDatabaseGuid},
  {EFI_IMAGE_SECURITY_DATABASE1, &gEfiImageSecurityDatabaseGuid},
};

EFI_HANDLE mImageHandle;

/**
  Measure PE image into TPM log based on the authenticode image hashing in
  PE/COFF Specification 8.0 Appendix A.

  Caution: This function may receive untrusted input.
  PE/COFF image is external input, so this function will validate its data structure
  within this image buffer before use.

  @param[in]  PCRIndex       TPM PCR index
  @param[in]  ImageAddress   Start address of image buffer.
  @param[in]  ImageSize      Image size
  @param[out] DigestList     Digeest list of this image.

  @retval EFI_SUCCESS            Successfully measure image.
  @retval EFI_OUT_OF_RESOURCES   No enough resource to measure image.
  @retval other error value
**/
EFI_STATUS
MeasurePeImageAndExtend (
  IN  UINT32                    PCRIndex,
  IN  EFI_PHYSICAL_ADDRESS      ImageAddress,
  IN  UINTN                     ImageSize,
  OUT TPML_DIGEST_VALUES        *DigestList
  );

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
    DEBUG ((EFI_D_INFO, "%02x", (UINTN)Data[Index]));
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
  UINTN   Index;
  UINTN   Count;
  UINTN   Left;

#define COLUME_SIZE  (16 * 2)

  Count = Size / COLUME_SIZE;
  Left  = Size % COLUME_SIZE;
  for (Index = 0; Index < Count; Index++) {
    DEBUG ((EFI_D_INFO, "%04x: ", Index * COLUME_SIZE));
    InternalDumpData (Data + Index * COLUME_SIZE, COLUME_SIZE);
    DEBUG ((EFI_D_INFO, "\n"));
  }

  if (Left != 0) {
    DEBUG ((EFI_D_INFO, "%04x: ", Index * COLUME_SIZE));
    InternalDumpData (Data + Index * COLUME_SIZE, Left);
    DEBUG ((EFI_D_INFO, "\n"));
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
    OUT  EFI_CPU_PHYSICAL_LOCATION   **LocationBuf,
    OUT  UINTN                       *Num
  )
{
  EFI_STATUS                        Status;
  EFI_MP_SERVICES_PROTOCOL          *MpProtocol;
  UINTN                             ProcessorNum;
  UINTN                             EnabledProcessorNum;
  EFI_PROCESSOR_INFORMATION         ProcessorInfo;
  EFI_CPU_PHYSICAL_LOCATION         *ProcessorLocBuf;
  UINTN                             Index;

  Status = gBS->LocateProtocol (&gEfiMpServiceProtocolGuid, NULL, (VOID **) &MpProtocol);
  if (EFI_ERROR (Status)) {
    //
    // MP protocol is not installed
    //
    return EFI_UNSUPPORTED;
  }

  Status = MpProtocol->GetNumberOfProcessors(
                         MpProtocol,
                         &ProcessorNum,
                         &EnabledProcessorNum
                         );
  if (EFI_ERROR(Status)){
    return Status;
  }

  Status = gBS->AllocatePool(
                  EfiBootServicesData,
                  sizeof(EFI_CPU_PHYSICAL_LOCATION) * ProcessorNum,
                  (VOID **) &ProcessorLocBuf
                  );
  if (EFI_ERROR(Status)){
    return Status;
  }

  //
  // Get each processor Location info
  //
  for (Index = 0; Index < ProcessorNum; Index++) {
    Status = MpProtocol->GetProcessorInfo(
                           MpProtocol,
                           Index,
                           &ProcessorInfo
                           );
    if (EFI_ERROR(Status)){
      FreePool(ProcessorLocBuf);
      return Status;
    }

    //
    // Get all Processor Location info & measure
    //
    CopyMem(
      &ProcessorLocBuf[Index],
      &ProcessorInfo.Location,
      sizeof(EFI_CPU_PHYSICAL_LOCATION)
      );
  }

  *LocationBuf = ProcessorLocBuf;
  *Num = ProcessorNum;

  return Status;
}

/**
  The EFI_TREE_PROTOCOL GetCapability function call provides protocol
  capability information and state information about the TrEE.

  @param[in]      This               Indicates the calling context
  @param[in, out] ProtocolCapability The caller allocates memory for a TREE_BOOT_SERVICE_CAPABILITY
                                     structure and sets the size field to the size of the structure allocated.
                                     The callee fills in the fields with the EFI protocol capability information
                                     and the current TrEE state information up to the number of fields which
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
TreeGetCapability (
  IN EFI_TREE_PROTOCOL                *This,
  IN OUT TREE_BOOT_SERVICE_CAPABILITY *ProtocolCapability
  )
{
  DEBUG ((EFI_D_INFO, "TreeGetCapability ...\n"));

  if ((This == NULL) || (ProtocolCapability == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if (ProtocolCapability->Size < mTcgDxeData.BsCap.Size) {
    ProtocolCapability->Size = mTcgDxeData.BsCap.Size;
    return EFI_BUFFER_TOO_SMALL;
  }

  CopyMem (ProtocolCapability, &mTcgDxeData.BsCap, mTcgDxeData.BsCap.Size);
  DEBUG ((EFI_D_INFO, "TreeGetCapability - %r\n", EFI_SUCCESS));
  return EFI_SUCCESS;
}

/**
  This function dump event log.

  @param[in]  EventLogFormat     The type of the event log for which the information is requested.
  @param[in]  EventLogLocation   A pointer to the memory address of the event log.
  @param[in]  EventLogLastEntry  If the Event Log contains more than one entry, this is a pointer to the
                                 address of the start of the last entry in the event log in memory.
**/
VOID
DumpEventLog (
  IN TREE_EVENT_LOG_FORMAT EventLogFormat,
  IN EFI_PHYSICAL_ADDRESS  EventLogLocation,
  IN EFI_PHYSICAL_ADDRESS  EventLogLastEntry
  )
{
  TCG_PCR_EVENT_HDR         *EventHdr;
  UINTN                     Index;

  DEBUG ((EFI_D_INFO, "EventLogFormat: (0x%x)\n", EventLogFormat));
  
  switch (EventLogFormat) {
  case TREE_EVENT_LOG_FORMAT_TCG_1_2:
    EventHdr = (TCG_PCR_EVENT_HDR *)(UINTN)EventLogLocation;
    while ((UINTN)EventHdr <= EventLogLastEntry) {
      DEBUG ((EFI_D_INFO, "  Event:\n"));
      DEBUG ((EFI_D_INFO, "    PCRIndex  - %d\n", EventHdr->PCRIndex));
      DEBUG ((EFI_D_INFO, "    EventType - 0x%08x\n", EventHdr->EventType));
      DEBUG ((EFI_D_INFO, "    Digest    - "));
      for (Index = 0; Index < sizeof(TCG_DIGEST); Index++) {
        DEBUG ((EFI_D_INFO, "%02x ", EventHdr->Digest.digest[Index]));
      }
      DEBUG ((EFI_D_INFO, "\n"));
      DEBUG ((EFI_D_INFO, "    EventSize - 0x%08x\n", EventHdr->EventSize));
      InternalDumpHex ((UINT8 *)(EventHdr + 1), EventHdr->EventSize);
      EventHdr = (TCG_PCR_EVENT_HDR *)((UINTN)EventHdr + sizeof(TCG_PCR_EVENT_HDR) + EventHdr->EventSize);
    }
    break;
  }

  return ;
}

/**
  The EFI_TREE_PROTOCOL Get Event Log function call allows a caller to
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
TreeGetEventLog (
  IN EFI_TREE_PROTOCOL     *This,
  IN TREE_EVENT_LOG_FORMAT EventLogFormat,
  OUT EFI_PHYSICAL_ADDRESS *EventLogLocation,
  OUT EFI_PHYSICAL_ADDRESS *EventLogLastEntry,
  OUT BOOLEAN              *EventLogTruncated
  )
{
  UINTN  Index;

  DEBUG ((EFI_D_INFO, "TreeGetEventLog ...\n"));

  if (This == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  for (Index = 0; Index < sizeof(mTreeEventInfo)/sizeof(mTreeEventInfo[0]); Index++) {
    if (EventLogFormat == mTreeEventInfo[Index].LogFormat) {
      break;
    }
  }

  if (Index == sizeof(mTreeEventInfo)/sizeof(mTreeEventInfo[0])) {
    return EFI_INVALID_PARAMETER;
  }

  if (!mTcgDxeData.BsCap.TrEEPresentFlag) {
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
    DEBUG ((EFI_D_INFO, "TreeGetEventLog (EventLogLocation - %x)\n", *EventLogLocation));
  }

  if (EventLogLastEntry != NULL) {
    if (!mTcgDxeData.EventLogAreaStruct[Index].EventLogStarted) {
      *EventLogLastEntry = (EFI_PHYSICAL_ADDRESS)(UINTN)0;
    } else {
      *EventLogLastEntry = (EFI_PHYSICAL_ADDRESS)(UINTN)mTcgDxeData.EventLogAreaStruct[Index].LastEvent;
    }
    DEBUG ((EFI_D_INFO, "TreeGetEventLog (EventLogLastEntry - %x)\n", *EventLogLastEntry));
  }

  if (EventLogTruncated != NULL) {
    *EventLogTruncated = mTcgDxeData.EventLogAreaStruct[Index].EventLogTruncated;
    DEBUG ((EFI_D_INFO, "TreeGetEventLog (EventLogTruncated - %x)\n", *EventLogTruncated));
  }

  DEBUG ((EFI_D_INFO, "TreeGetEventLog - %r\n", EFI_SUCCESS));

  // Dump Event Log for debug purpose
  if ((EventLogLocation != NULL) && (EventLogLastEntry != NULL)) {
    DumpEventLog (EventLogFormat, *EventLogLocation, *EventLogLastEntry);
  }

  return EFI_SUCCESS;
}

/**
  Add a new entry to the Event Log.

  @param[in, out] EventLogPtr     Pointer to the Event Log data.  
  @param[in, out] LogSize         Size of the Event Log.  
  @param[in]      MaxSize         Maximum size of the Event Log.
  @param[in]      NewEventHdr     Pointer to a TCG_PCR_EVENT_HDR/TCG_PCR_EVENT_EX data structure.  
  @param[in]      NewEventHdrSize New event header size.
  @param[in]      NewEventData    Pointer to the new event data.  
  @param[in]      NewEventSize    New event data size.
  
  @retval EFI_SUCCESS           The new event log entry was added.
  @retval EFI_OUT_OF_RESOURCES  No enough memory to log the new event.

**/
EFI_STATUS
TcgCommLogEvent (
  IN OUT  UINT8                     **EventLogPtr,
  IN OUT  UINTN                     *LogSize,
  IN      UINTN                     MaxSize,
  IN      VOID                      *NewEventHdr,
  IN      UINT32                    NewEventHdrSize,
  IN      UINT8                     *NewEventData,
  IN      UINT32                    NewEventSize
  )
{
  UINTN                            NewLogSize;

  if (NewEventSize > MAX_ADDRESS -  NewEventHdrSize) {
    return EFI_OUT_OF_RESOURCES;
  }

  NewLogSize = NewEventHdrSize + NewEventSize;

  if (NewLogSize > MAX_ADDRESS -  *LogSize) {
    return EFI_OUT_OF_RESOURCES;
  }

  if (NewLogSize + *LogSize > MaxSize) {
    DEBUG ((EFI_D_INFO, "  MaxSize    - 0x%x\n", MaxSize));
    DEBUG ((EFI_D_INFO, "  NewLogSize - 0x%x\n", NewLogSize));
    DEBUG ((EFI_D_INFO, "  LogSize    - 0x%x\n", *LogSize));
    DEBUG ((EFI_D_INFO, "TcgCommLogEvent - %r\n", EFI_OUT_OF_RESOURCES));
    return EFI_OUT_OF_RESOURCES;
  }

  *EventLogPtr += *LogSize;
  *LogSize += NewLogSize;
  CopyMem (*EventLogPtr, NewEventHdr, NewEventHdrSize);
  CopyMem (
    *EventLogPtr + NewEventHdrSize,
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
  IN      TREE_EVENT_LOG_FORMAT     EventLogFormat,
  IN      VOID                      *NewEventHdr,
  IN      UINT32                    NewEventHdrSize,
  IN      UINT8                     *NewEventData,
  IN      UINT32                    NewEventSize
  )
{
  EFI_STATUS   Status;
  UINTN        Index;

  for (Index = 0; Index < sizeof(mTreeEventInfo)/sizeof(mTreeEventInfo[0]); Index++) {
    if (EventLogFormat == mTreeEventInfo[Index].LogFormat) {
      break;
    }
  }

  if (Index == sizeof(mTreeEventInfo)/sizeof(mTreeEventInfo[0])) {
    return EFI_INVALID_PARAMETER;
  }

  if (mTcgDxeData.EventLogAreaStruct[Index].EventLogTruncated) {
    return EFI_VOLUME_FULL;
  }

  mTcgDxeData.EventLogAreaStruct[Index].LastEvent = (UINT8*)(UINTN)mTcgDxeData.EventLogAreaStruct[Index].Lasa;
  Status = TcgCommLogEvent (
             &mTcgDxeData.EventLogAreaStruct[Index].LastEvent,
             &mTcgDxeData.EventLogAreaStruct[Index].EventLogSize,
             (UINTN)mTcgDxeData.EventLogAreaStruct[Index].Laml,
             NewEventHdr,
             NewEventHdrSize,
             NewEventData,
             NewEventSize
             );
  
  if (Status == EFI_DEVICE_ERROR) {
    return EFI_DEVICE_ERROR;
  } else if (Status == EFI_OUT_OF_RESOURCES) {
    mTcgDxeData.EventLogAreaStruct[Index].EventLogTruncated = TRUE;
    return EFI_VOLUME_FULL;
  } else if (Status == EFI_SUCCESS) {
    mTcgDxeData.EventLogAreaStruct[Index].EventLogStarted = TRUE;
  }

  return Status;
}

/**
  This function get digest from digest list.

  @param HashAlg    digest algorithm
  @param DigestList digest list
  @param Digest     digest

  @retval EFI_SUCCESS   Sha1Digest is found and returned.
  @retval EFI_NOT_FOUND Sha1Digest is not found.
**/
EFI_STATUS
Tpm2GetDigestFromDigestList (
  IN TPMI_ALG_HASH      HashAlg,
  IN TPML_DIGEST_VALUES *DigestList,
  IN VOID               *Digest
  )
{
  UINTN  Index;
  UINT16 DigestSize;

  DigestSize = GetHashSizeFromAlgo (HashAlg);
  for (Index = 0; Index < DigestList->count; Index++) {
    if (DigestList->digests[Index].hashAlg == HashAlg) {
      CopyMem (
        Digest,
        &DigestList->digests[Index].digest,
        DigestSize
        );
      return EFI_SUCCESS;
    }
  }

  return EFI_NOT_FOUND;
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
  IN TPML_DIGEST_VALUES             *DigestList,
  IN OUT  TCG_PCR_EVENT_HDR         *NewEventHdr,
  IN      UINT8                     *NewEventData
  )
{
  EFI_STATUS                        Status;
  EFI_TPL                           OldTpl;
  UINTN                             Index;
  EFI_STATUS                        RetStatus;

  RetStatus = EFI_SUCCESS;
  for (Index = 0; Index < sizeof(mTreeEventInfo)/sizeof(mTreeEventInfo[0]); Index++) {
      DEBUG ((EFI_D_INFO, "  LogFormat - 0x%08x\n", mTreeEventInfo[Index].LogFormat));
      switch (mTreeEventInfo[Index].LogFormat) {
      case TREE_EVENT_LOG_FORMAT_TCG_1_2:
        Status = Tpm2GetDigestFromDigestList (TPM_ALG_SHA1, DigestList, &NewEventHdr->Digest);
        if (!EFI_ERROR (Status)) {
          //
          // Enter critical region
          //
          OldTpl = gBS->RaiseTPL (TPL_HIGH_LEVEL);
          Status = TcgDxeLogEvent (
                     mTreeEventInfo[Index].LogFormat,
                     NewEventHdr,
                     sizeof(TCG_PCR_EVENT_HDR),
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
  IN      UINT64                    Flags,
  IN      UINT8                     *HashData,
  IN      UINT64                    HashDataLen,
  IN OUT  TCG_PCR_EVENT_HDR         *NewEventHdr,
  IN      UINT8                     *NewEventData
  )
{
  EFI_STATUS                        Status;
  TPML_DIGEST_VALUES                DigestList;
  
  if (!mTcgDxeData.BsCap.TrEEPresentFlag) {
    return EFI_DEVICE_ERROR;
  }

  Status = HashAndExtend (
             NewEventHdr->PCRIndex,
             HashData,
             (UINTN)HashDataLen,
             &DigestList
             );
  if (!EFI_ERROR (Status)) {
    if ((Flags & TREE_EXTEND_ONLY) == 0) {
      Status = TcgDxeLogHashEvent (&DigestList, NewEventHdr, NewEventData);
    }
  }

  if (Status == EFI_DEVICE_ERROR) {
    DEBUG ((EFI_D_ERROR, "TcgDxeHashLogExtendEvent - %r. Disable TPM.\n", Status));
    mTcgDxeData.BsCap.TrEEPresentFlag = FALSE;
    REPORT_STATUS_CODE (
      EFI_ERROR_CODE | EFI_ERROR_MINOR,
      (PcdGet32 (PcdStatusCodeSubClassTpmDevice) | EFI_P_EC_INTERFACE_ERROR)
      );
  }

  return Status;
}

/**
  The EFI_TREE_PROTOCOL HashLogExtendEvent function call provides callers with
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
TreeHashLogExtendEvent (
  IN EFI_TREE_PROTOCOL    *This,
  IN UINT64               Flags,
  IN EFI_PHYSICAL_ADDRESS DataToHash,
  IN UINT64               DataToHashLen,
  IN TrEE_EVENT           *Event
  )
{
  EFI_STATUS         Status;
  TCG_PCR_EVENT_HDR  NewEventHdr;
  TPML_DIGEST_VALUES DigestList;

  DEBUG ((EFI_D_INFO, "TreeHashLogExtendEvent ...\n"));

  if ((This == NULL) || (DataToHash == 0) || (Event == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if (!mTcgDxeData.BsCap.TrEEPresentFlag) {
    return EFI_UNSUPPORTED;
  }

  if (Event->Size < Event->Header.HeaderSize + sizeof(UINT32)) {
    return EFI_INVALID_PARAMETER;
  }

  if (Event->Header.PCRIndex > MAX_PCR_INDEX) {
    return EFI_INVALID_PARAMETER;
  }

  NewEventHdr.PCRIndex  = Event->Header.PCRIndex;
  NewEventHdr.EventType = Event->Header.EventType;
  NewEventHdr.EventSize = Event->Size - sizeof(UINT32) - Event->Header.HeaderSize;
  if ((Flags & PE_COFF_IMAGE) != 0) {
    Status = MeasurePeImageAndExtend (
               NewEventHdr.PCRIndex,
               DataToHash,
               (UINTN)DataToHashLen,
               &DigestList
               );
    if (!EFI_ERROR (Status)) {
      if ((Flags & TREE_EXTEND_ONLY) == 0) {
        Status = TcgDxeLogHashEvent (&DigestList, &NewEventHdr, Event->Event);
      }
    }
    if (Status == EFI_DEVICE_ERROR) {
      DEBUG ((EFI_D_ERROR, "MeasurePeImageAndExtend - %r. Disable TPM.\n", Status));
      mTcgDxeData.BsCap.TrEEPresentFlag = FALSE;
      REPORT_STATUS_CODE (
        EFI_ERROR_CODE | EFI_ERROR_MINOR,
        (PcdGet32 (PcdStatusCodeSubClassTpmDevice) | EFI_P_EC_INTERFACE_ERROR)
        );
    }
  } else {
    Status = TcgDxeHashLogExtendEvent (
               Flags,
               (UINT8 *) (UINTN) DataToHash,
               DataToHashLen,
               &NewEventHdr,
               Event->Event
               );
  }
  DEBUG ((EFI_D_INFO, "TreeHashLogExtendEvent - %r\n", Status));
  return Status;
}

/**
  This service enables the sending of commands to the TrEE.

  @param[in]  This                     Indicates the calling context
  @param[in]  InputParameterBlockSize  Size of the TrEE input parameter block.
  @param[in]  InputParameterBlock      Pointer to the TrEE input parameter block.
  @param[in]  OutputParameterBlockSize Size of the TrEE output parameter block.
  @param[in]  OutputParameterBlock     Pointer to the TrEE output parameter block.

  @retval EFI_SUCCESS            The command byte stream was successfully sent to the device and a response was successfully received.
  @retval EFI_DEVICE_ERROR       The command was not successfully sent to the device or a response was not successfully received from the device.
  @retval EFI_INVALID_PARAMETER  One or more of the parameters are incorrect.
  @retval EFI_BUFFER_TOO_SMALL   The output parameter block is too small. 
**/
EFI_STATUS
EFIAPI
TreeSubmitCommand (
  IN EFI_TREE_PROTOCOL *This,
  IN UINT32            InputParameterBlockSize,
  IN UINT8             *InputParameterBlock,
  IN UINT32            OutputParameterBlockSize,
  IN UINT8             *OutputParameterBlock
  )
{
  EFI_STATUS    Status;

  DEBUG ((EFI_D_INFO, "TreeSubmitCommand ...\n"));

  if ((This == NULL) ||
      (InputParameterBlockSize == 0) || (InputParameterBlock == NULL) ||
      (OutputParameterBlockSize == 0) || (OutputParameterBlock == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if (!mTcgDxeData.BsCap.TrEEPresentFlag) {
    return EFI_UNSUPPORTED;
  }

  if (InputParameterBlockSize >= mTcgDxeData.BsCap.MaxCommandSize) {
    return EFI_INVALID_PARAMETER;
  }
  if (OutputParameterBlockSize >= mTcgDxeData.BsCap.MaxResponseSize) {
    return EFI_INVALID_PARAMETER;
  }

  Status = Tpm2SubmitCommand (
             InputParameterBlockSize,
             InputParameterBlock,
             &OutputParameterBlockSize,
             OutputParameterBlock
             );
  DEBUG ((EFI_D_INFO, "TreeSubmitCommand - %r\n", Status));
  return Status;
}


EFI_TREE_PROTOCOL mTreeProtocol = {
    TreeGetCapability,
    TreeGetEventLog,
    TreeHashLogExtendEvent,
    TreeSubmitCommand
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
  EFI_STATUS              Status;
  VOID                    *TcgEvent;
  EFI_PEI_HOB_POINTERS    GuidHob;
  EFI_PHYSICAL_ADDRESS    Lasa;
  UINTN                   Index;

  DEBUG ((EFI_D_INFO, "SetupEventLog\n"));

  //
  // 1. Create Log Area
  //
  for (Index = 0; Index < sizeof(mTreeEventInfo)/sizeof(mTreeEventInfo[0]); Index++) {
      mTcgDxeData.EventLogAreaStruct[Index].EventLogFormat = mTreeEventInfo[Index].LogFormat;
      Lasa = (EFI_PHYSICAL_ADDRESS) (SIZE_4GB - 1);
      Status = gBS->AllocatePages (
                      AllocateMaxAddress,
                      EfiACPIMemoryNVS,
                      EFI_SIZE_TO_PAGES (EFI_TCG_LOG_AREA_SIZE),
                      &Lasa
                      );
      if (EFI_ERROR (Status)) {
        return Status;
      }
      mTcgDxeData.EventLogAreaStruct[Index].Lasa = Lasa;
      mTcgDxeData.EventLogAreaStruct[Index].Laml = EFI_TCG_LOG_AREA_SIZE;
      //
      // To initialize them as 0xFF is recommended 
      // because the OS can know the last entry for that.
      //
      SetMem ((VOID *)(UINTN)Lasa, EFI_TCG_LOG_AREA_SIZE, 0xFF);
  }

  //
  // 2. Create ACPI table for TCG1.2 only
  //
    if (PcdGet8 (PcdTpmPlatformClass) == TCG_PLATFORM_TYPE_CLIENT) {
      mTcgClientAcpiTemplate.Lasa = mTcgDxeData.EventLogAreaStruct[0].Lasa;
      mTcgClientAcpiTemplate.Laml = EFI_TCG_LOG_AREA_SIZE;
    } else {
      mTcgServerAcpiTemplate.Lasa = mTcgDxeData.EventLogAreaStruct[0].Lasa;
      mTcgServerAcpiTemplate.Laml = EFI_TCG_LOG_AREA_SIZE;
    }

  //
  // 3. Sync data from PEI to DXE
  //
  Status = EFI_SUCCESS;
  for (Index = 0; Index < sizeof(mTreeEventInfo)/sizeof(mTreeEventInfo[0]); Index++) {
      GuidHob.Raw = GetHobList ();
      Status = EFI_SUCCESS;
      while (!EFI_ERROR (Status) && 
             (GuidHob.Raw = GetNextGuidHob (mTreeEventInfo[Index].EventGuid, GuidHob.Raw)) != NULL) {
        TcgEvent    = GET_GUID_HOB_DATA (GuidHob.Guid);
        GuidHob.Raw = GET_NEXT_HOB (GuidHob);
        switch (mTreeEventInfo[Index].LogFormat) {
        case TREE_EVENT_LOG_FORMAT_TCG_1_2:
          Status = TcgDxeLogEvent (
                     mTreeEventInfo[Index].LogFormat,
                     TcgEvent,
                     sizeof(TCG_PCR_EVENT_HDR),
                     ((TCG_PCR_EVENT*)TcgEvent)->Event,
                     ((TCG_PCR_EVENT_HDR*)TcgEvent)->EventSize
                     );
          break;
        }
      }
  }

  return Status;
}

/**
  Measure and log an action string, and extend the measurement result into PCR[5].

  @param[in] String           A specific string that indicates an Action event.  
  
  @retval EFI_SUCCESS         Operation completed successfully.
  @retval EFI_DEVICE_ERROR    The operation was unsuccessful.

**/
EFI_STATUS
TcgMeasureAction (
  IN      CHAR8                     *String
  )
{
  TCG_PCR_EVENT_HDR                 TcgEvent;

  TcgEvent.PCRIndex  = 5;
  TcgEvent.EventType = EV_EFI_ACTION;
  TcgEvent.EventSize = (UINT32)AsciiStrLen (String);
  return TcgDxeHashLogExtendEvent (
           0,
           (UINT8*)String,
           TcgEvent.EventSize,
           &TcgEvent,
           (UINT8 *) String
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
  EFI_STATUS                        Status;
  SMBIOS_TABLE_ENTRY_POINT          *SmbiosTable;
  TCG_PCR_EVENT_HDR                 TcgEvent;
  EFI_HANDOFF_TABLE_POINTERS        HandoffTables;
  UINTN                             ProcessorNum;
  EFI_CPU_PHYSICAL_LOCATION         *ProcessorLocBuf;

  ProcessorLocBuf = NULL;

  //
  // Measure SMBIOS with EV_EFI_HANDOFF_TABLES to PCR[1]
  //
  Status = EfiGetSystemConfigurationTable (
             &gEfiSmbiosTableGuid,
             (VOID **) &SmbiosTable
             );

  if (!EFI_ERROR (Status) && SmbiosTable != NULL) {
    TcgEvent.PCRIndex  = 1;
    TcgEvent.EventType = EV_EFI_HANDOFF_TABLES;
    TcgEvent.EventSize = sizeof (HandoffTables);

    HandoffTables.NumberOfTables = 1;
    HandoffTables.TableEntry[0].VendorGuid  = gEfiSmbiosTableGuid;
    HandoffTables.TableEntry[0].VendorTable = SmbiosTable;

    DEBUG ((DEBUG_INFO, "The Smbios Table starts at: 0x%x\n", SmbiosTable->TableAddress));
    DEBUG ((DEBUG_INFO, "The Smbios Table size: 0x%x\n", SmbiosTable->TableLength));

    Status = TcgDxeHashLogExtendEvent (
               0,
               (UINT8*)(UINTN)SmbiosTable->TableAddress,
               SmbiosTable->TableLength,
               &TcgEvent,
               (UINT8*)&HandoffTables
               );
  }

  if (PcdGet8 (PcdTpmPlatformClass) == TCG_PLATFORM_TYPE_SERVER) {
    //
    // Tcg Server spec. 
    // Measure each processor EFI_CPU_PHYSICAL_LOCATION with EV_TABLE_OF_DEVICES to PCR[1]
    //
    Status = GetProcessorsCpuLocation(&ProcessorLocBuf, &ProcessorNum);

    if (!EFI_ERROR(Status)){
      TcgEvent.PCRIndex  = 1;
      TcgEvent.EventType = EV_TABLE_OF_DEVICES;
      TcgEvent.EventSize = sizeof (HandoffTables);

      HandoffTables.NumberOfTables = 1;
      HandoffTables.TableEntry[0].VendorGuid  = gEfiMpServiceProtocolGuid;
      HandoffTables.TableEntry[0].VendorTable = ProcessorLocBuf;

      Status = TcgDxeHashLogExtendEvent (
                 0,
                 (UINT8*)(UINTN)ProcessorLocBuf,
                 sizeof(EFI_CPU_PHYSICAL_LOCATION) * ProcessorNum,
                 &TcgEvent,
                 (UINT8*)&HandoffTables
                 );

      FreePool(ProcessorLocBuf);
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
  IN      TPM_PCRINDEX              PCRIndex
  )
{
  TCG_PCR_EVENT_HDR                 TcgEvent;
  UINT32                            EventData;

  DEBUG ((EFI_D_INFO, "MeasureSeparatorEvent Pcr - %x\n", PCRIndex));

  EventData = 0;
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
  IN      TPM_PCRINDEX              PCRIndex,
  IN      TCG_EVENTTYPE             EventType,
  IN      CHAR16                    *VarName,
  IN      EFI_GUID                  *VendorGuid,
  IN      VOID                      *VarData,
  IN      UINTN                     VarSize
  )
{
  EFI_STATUS                        Status;
  TCG_PCR_EVENT_HDR                 TcgEvent;
  UINTN                             VarNameLength;
  EFI_VARIABLE_DATA_TREE            *VarLog;

  DEBUG ((EFI_D_INFO, "TrEEDxe: MeasureVariable (Pcr - %x, EventType - %x, ", (UINTN)PCRIndex, (UINTN)EventType));
  DEBUG ((EFI_D_INFO, "VariableName - %s, VendorGuid - %g)\n", VarName, VendorGuid));

  VarNameLength      = StrLen (VarName);
  TcgEvent.PCRIndex  = PCRIndex;
  TcgEvent.EventType = EventType;
  TcgEvent.EventSize = (UINT32)(sizeof (*VarLog) + VarNameLength * sizeof (*VarName) + VarSize
                        - sizeof (VarLog->UnicodeName) - sizeof (VarLog->VariableData));

  VarLog = (EFI_VARIABLE_DATA_TREE*)AllocatePool (TcgEvent.EventSize);
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
  if (VarSize != 0 && VarData != NULL) {
    CopyMem (
       (CHAR16 *)VarLog->UnicodeName + VarNameLength,
       VarData,
       VarSize
       );
  }

  Status = TcgDxeHashLogExtendEvent (
             0,
             (UINT8*)VarLog,
             TcgEvent.EventSize,
             &TcgEvent,
             (UINT8*)VarLog
             );

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
  IN      TPM_PCRINDEX              PCRIndex,
  IN      TCG_EVENTTYPE             EventType,
  IN      CHAR16                    *VarName,
  IN      EFI_GUID                  *VendorGuid,
  OUT     UINTN                     *VarSize,
  OUT     VOID                      **VarData
  )
{
  EFI_STATUS                        Status;

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
  Read then Measure and log an EFI boot variable, and extend the measurement result into PCR[5].

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
  IN      CHAR16                    *VarName,
  IN      EFI_GUID                  *VendorGuid,
  OUT     UINTN                     *VarSize,
  OUT     VOID                      **VarData
  )
{
  return ReadAndMeasureVariable (
           5,
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
  IN      CHAR16                    *VarName,
  IN      EFI_GUID                  *VendorGuid,
  OUT     UINTN                     *VarSize,
  OUT     VOID                      **VarData
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
  EFI_STATUS                        Status;
  UINT16                            *BootOrder;
  UINTN                             BootCount;
  UINTN                             Index;
  VOID                              *BootVarData;
  UINTN                             Size;

  Status = ReadAndMeasureBootVariable (
             mBootVarName,
             &gEfiGlobalVariableGuid,
             &BootCount,
             (VOID **) &BootOrder
             );
  if (Status == EFI_NOT_FOUND || BootOrder == NULL) {
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
  EFI_STATUS                        Status;
  VOID                              *Data;
  UINTN                             DataSize;
  UINTN                             Index;

  Status = EFI_NOT_FOUND;
  for (Index = 0; Index < sizeof(mVariableType)/sizeof(mVariableType[0]); Index++) {
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
  TCG_PCR_EVENT_HDR                 TcgEvent;

  TcgEvent.PCRIndex  = 7;
  TcgEvent.EventType = EV_EFI_ACTION;
  TcgEvent.EventSize = sizeof(FIRMWARE_DEBUGGER_EVENT_STRING) - 1;
  return TcgDxeHashLogExtendEvent (
           0,
           (UINT8 *)FIRMWARE_DEBUGGER_EVENT_STRING,
           sizeof(FIRMWARE_DEBUGGER_EVENT_STRING) - 1,
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
  IN EFI_EVENT                      Event,
  IN VOID                           *Context
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
    DEBUG ((EFI_D_INFO, "MeasureLaunchOfFirmwareDebugger - %r\n", Status));
  }

  Status = MeasureAllSecureVariables ();
  DEBUG ((EFI_D_INFO, "MeasureAllSecureVariables - %r\n", Status));

  //
  // We need measure Separator(7) here, because this event must be between SecureBootPolicy (Configure)
  // and ImageVerification (Authority)
  // There might be a case that we need measure UEFI image from DriverOrder, besides BootOrder. So
  // the Authority measurement happen before ReadToBoot event.
  //
  Status = MeasureSeparatorEvent (7);
  DEBUG ((EFI_D_INFO, "MeasureSeparatorEvent - %r\n", Status));
  return ;
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
  IN      EFI_EVENT                 Event,
  IN      VOID                      *Context
  )
{
  EFI_STATUS                        Status;
  TPM_PCRINDEX                      PcrIndex;

  PERF_START_EX (mImageHandle, "EventRec", "TrEEDxe", 0, PERF_ID_TREE_DXE);
  if (mBootAttempts == 0) {

    //
    // Measure handoff tables.
    //
    Status = MeasureHandoffTables ();
    if (EFI_ERROR (Status)) {
      DEBUG ((EFI_D_ERROR, "HOBs not Measured. Error!\n"));
    }

    //
    // Measure BootOrder & Boot#### variables.
    //
    Status = MeasureAllBootVariables ();
    if (EFI_ERROR (Status)) {
      DEBUG ((EFI_D_ERROR, "Boot Variables not Measured. Error!\n"));
    }

    //
    // 1. This is the first boot attempt.
    //
    Status = TcgMeasureAction (
               EFI_CALLING_EFI_APPLICATION
               );
    if (EFI_ERROR (Status)) {
      DEBUG ((EFI_D_ERROR, "%s not Measured. Error!\n", EFI_CALLING_EFI_APPLICATION));
    }

    //
    // 2. Draw a line between pre-boot env and entering post-boot env.
    // PCR[7] is already done.
    //
    for (PcrIndex = 0; PcrIndex < 7; PcrIndex++) {
      Status = MeasureSeparatorEvent (PcrIndex);
      if (EFI_ERROR (Status)) {
        DEBUG ((EFI_D_ERROR, "Seperator Event not Measured. Error!\n"));
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
               EFI_RETURNING_FROM_EFI_APPLICATOIN
               );
    if (EFI_ERROR (Status)) {
      DEBUG ((EFI_D_ERROR, "%s not Measured. Error!\n", EFI_RETURNING_FROM_EFI_APPLICATOIN));
    }
  }

  DEBUG ((EFI_D_INFO, "TPM2 TrEEDxe Measure Data when ReadyToBoot\n"));
  //
  // Increase boot attempt counter.
  //
  mBootAttempts++;
  PERF_END_EX (mImageHandle, "EventRec", "TrEEDxe", 0, PERF_ID_TREE_DXE + 1);
}

/**
  Install TCG ACPI Table when ACPI Table Protocol is available.

  A system's firmware uses an ACPI table to identify the system's TCG capabilities 
  to the Post-Boot environment. The information in this ACPI table is not guaranteed 
  to be valid until the Host Platform transitions from pre-boot state to post-boot state.  

  @param[in]  Event     Event whose notification function is being invoked
  @param[in]  Context   Pointer to the notification function's context
**/
VOID
EFIAPI
InstallAcpiTable (
  IN EFI_EVENT                      Event,
  IN VOID                           *Context
  )
{
  UINTN                             TableKey;
  EFI_STATUS                        Status;
  EFI_ACPI_TABLE_PROTOCOL           *AcpiTable;
  UINT8                             Checksum;
  UINT64                            OemTableId;

  Status = gBS->LocateProtocol (&gEfiAcpiTableProtocolGuid, NULL, (VOID **)&AcpiTable);
  if (EFI_ERROR (Status)) {
    return;
  }

  if (PcdGet8 (PcdTpmPlatformClass) == TCG_PLATFORM_TYPE_CLIENT) {
    CopyMem (mTcgClientAcpiTemplate.Header.OemId, PcdGetPtr (PcdAcpiDefaultOemId), sizeof (mTcgClientAcpiTemplate.Header.OemId));
    OemTableId = PcdGet64 (PcdAcpiDefaultOemTableId);
    CopyMem (&mTcgClientAcpiTemplate.Header.OemTableId, &OemTableId, sizeof (UINT64));
    mTcgClientAcpiTemplate.Header.OemRevision      = PcdGet32 (PcdAcpiDefaultOemRevision);
    mTcgClientAcpiTemplate.Header.CreatorId        = PcdGet32 (PcdAcpiDefaultCreatorId);
    mTcgClientAcpiTemplate.Header.CreatorRevision  = PcdGet32 (PcdAcpiDefaultCreatorRevision);
    //
    // The ACPI table must be checksumed before calling the InstallAcpiTable() 
    // service of the ACPI table protocol to install it.
    //
    Checksum = CalculateCheckSum8 ((UINT8 *)&mTcgClientAcpiTemplate, sizeof (mTcgClientAcpiTemplate));
    mTcgClientAcpiTemplate.Header.Checksum = Checksum;

    Status = AcpiTable->InstallAcpiTable (
                            AcpiTable,
                            &mTcgClientAcpiTemplate,
                            sizeof (mTcgClientAcpiTemplate),
                            &TableKey
                            );
  } else {
    CopyMem (mTcgServerAcpiTemplate.Header.OemId, PcdGetPtr (PcdAcpiDefaultOemId), sizeof (mTcgServerAcpiTemplate.Header.OemId));
    OemTableId = PcdGet64 (PcdAcpiDefaultOemTableId);
    CopyMem (&mTcgServerAcpiTemplate.Header.OemTableId, &OemTableId, sizeof (UINT64));
    mTcgServerAcpiTemplate.Header.OemRevision      = PcdGet32 (PcdAcpiDefaultOemRevision);
    mTcgServerAcpiTemplate.Header.CreatorId        = PcdGet32 (PcdAcpiDefaultCreatorId);
    mTcgServerAcpiTemplate.Header.CreatorRevision  = PcdGet32 (PcdAcpiDefaultCreatorRevision);
    //
    // The ACPI table must be checksumed before calling the InstallAcpiTable() 
    // service of the ACPI table protocol to install it.
    //
    Checksum = CalculateCheckSum8 ((UINT8 *)&mTcgServerAcpiTemplate, sizeof (mTcgServerAcpiTemplate));
    mTcgServerAcpiTemplate.Header.Checksum = Checksum;

    mTcgServerAcpiTemplate.BaseAddress.Address = PcdGet64 (PcdTpmBaseAddress);
    Status = AcpiTable->InstallAcpiTable (
                            AcpiTable,
                            &mTcgServerAcpiTemplate,
                            sizeof (mTcgServerAcpiTemplate),
                            &TableKey
                            );
  }

  if (EFI_ERROR (Status)) {
    DEBUG((EFI_D_ERROR, "Tcg Acpi Table installation failure"));
  }
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
  IN      EFI_EVENT                 Event,
  IN      VOID                      *Context
  )
{
  EFI_STATUS    Status;

  //
  // Measure invocation of ExitBootServices,
  //
  Status = TcgMeasureAction (
             EFI_EXIT_BOOT_SERVICES_INVOCATION
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "%s not Measured. Error!\n", EFI_EXIT_BOOT_SERVICES_INVOCATION));
  }

  //
  // Measure success of ExitBootServices
  //
  Status = TcgMeasureAction (
             EFI_EXIT_BOOT_SERVICES_SUCCEEDED
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "%s not Measured. Error!\n", EFI_EXIT_BOOT_SERVICES_SUCCEEDED));
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
  IN      EFI_EVENT                 Event,
  IN      VOID                      *Context
  )
{
  EFI_STATUS    Status;

  //
  // Measure Failure of ExitBootServices,
  //
  Status = TcgMeasureAction (
             EFI_EXIT_BOOT_SERVICES_FAILED
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "%s not Measured. Error!\n", EFI_EXIT_BOOT_SERVICES_FAILED));
  }

}

/**
  The function install TrEE protocol.
  
  @retval EFI_SUCCESS     TrEE protocol is installed.
  @retval other           Some error occurs.
**/
EFI_STATUS
InstallTrEE (
  VOID
  )
{
  EFI_STATUS        Status;
  EFI_HANDLE        Handle;

  Handle = NULL;
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &Handle,
                  &gEfiTrEEProtocolGuid,
                  &mTreeProtocol,
                  NULL
                  );
  return Status;
}

/**
  The driver's entry point. It publishes EFI TrEE Protocol.

  @param[in] ImageHandle  The firmware allocated handle for the EFI image.  
  @param[in] SystemTable  A pointer to the EFI System Table.
  
  @retval EFI_SUCCESS     The entry point is executed successfully.
  @retval other           Some error occurs when executing this entry point.
**/
EFI_STATUS
EFIAPI
DriverEntry (
  IN    EFI_HANDLE                  ImageHandle,
  IN    EFI_SYSTEM_TABLE            *SystemTable
  )
{
  EFI_STATUS                        Status;
  EFI_EVENT                         Event;
  VOID                              *Registration;
  UINT32                            MaxCommandSize;
  UINT32                            MaxResponseSize;
  TPML_PCR_SELECTION                Pcrs;
  UINTN                             Index;
  UINT32                            TpmHashAlgorithmBitmap;

  mImageHandle = ImageHandle;

  if (CompareGuid (PcdGetPtr(PcdTpmInstanceGuid), &gEfiTpmDeviceInstanceNoneGuid) ||
      CompareGuid (PcdGetPtr(PcdTpmInstanceGuid), &gEfiTpmDeviceInstanceTpm12Guid)){
    DEBUG ((EFI_D_ERROR, "No TPM2 instance required!\n"));
    return EFI_UNSUPPORTED;
  }

  if (GetFirstGuidHob (&gTpmErrorHobGuid) != NULL) {
    DEBUG ((EFI_D_ERROR, "TPM2 error!\n"));
    return EFI_DEVICE_ERROR;
  }
  
  Status = Tpm2RequestUseTpm ();
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "TPM2 not detected!\n"));
    return Status;
  }
  
  //
  // Fill information
  //
  DEBUG ((EFI_D_INFO, "TrEE.ProtocolVersion  - %02x.%02x\n", mTcgDxeData.BsCap.ProtocolVersion.Major, mTcgDxeData.BsCap.ProtocolVersion.Minor));
  DEBUG ((EFI_D_INFO, "TrEE.StructureVersion - %02x.%02x\n", mTcgDxeData.BsCap.StructureVersion.Major, mTcgDxeData.BsCap.StructureVersion.Minor));

  Status = Tpm2GetCapabilityManufactureID (&mTcgDxeData.BsCap.ManufacturerID);
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "Tpm2GetCapabilityManufactureID fail!\n"));
  } else {
    DEBUG ((EFI_D_INFO, "Tpm2GetCapabilityManufactureID - %08x\n", mTcgDxeData.BsCap.ManufacturerID));
  }

  DEBUG_CODE (
    UINT32                    FirmwareVersion1;
    UINT32                    FirmwareVersion2;

    Status = Tpm2GetCapabilityFirmwareVersion (&FirmwareVersion1, &FirmwareVersion2);
    if (EFI_ERROR (Status)) {
      DEBUG ((EFI_D_ERROR, "Tpm2GetCapabilityFirmwareVersion fail!\n"));
    } else {
      DEBUG ((EFI_D_INFO, "Tpm2GetCapabilityFirmwareVersion - %08x %08x\n", FirmwareVersion1, FirmwareVersion2));
    }
  );

  Status = Tpm2GetCapabilityMaxCommandResponseSize (&MaxCommandSize, &MaxResponseSize);
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "Tpm2GetCapabilityMaxCommandResponseSize fail!\n"));
  } else {
    mTcgDxeData.BsCap.MaxCommandSize  = (UINT16)MaxCommandSize;
    mTcgDxeData.BsCap.MaxResponseSize = (UINT16)MaxResponseSize;
    DEBUG ((EFI_D_INFO, "Tpm2GetCapabilityMaxCommandResponseSize - %08x, %08x\n", MaxCommandSize, MaxResponseSize));
  }

  Status = Tpm2GetCapabilityPcrs (&Pcrs);
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "Tpm2GetCapabilityPcrs fail!\n"));
    TpmHashAlgorithmBitmap = TREE_BOOT_HASH_ALG_SHA1;
  } else {
    DEBUG ((EFI_D_INFO, "Tpm2GetCapabilityPcrs Count - %08x\n", Pcrs.count));
    TpmHashAlgorithmBitmap = 0;
    for (Index = 0; Index < Pcrs.count; Index++) {
      DEBUG ((EFI_D_INFO, "hash - %x\n", Pcrs.pcrSelections[Index].hash));
      switch (Pcrs.pcrSelections[Index].hash) {
      case TPM_ALG_SHA1:
        TpmHashAlgorithmBitmap |= TREE_BOOT_HASH_ALG_SHA1;
        break;
      case TPM_ALG_SHA256:
        TpmHashAlgorithmBitmap |= TREE_BOOT_HASH_ALG_SHA256;
        break;
      case TPM_ALG_SHA384:
        TpmHashAlgorithmBitmap |= TREE_BOOT_HASH_ALG_SHA384;
        break;
      case TPM_ALG_SHA512:
        TpmHashAlgorithmBitmap |= TREE_BOOT_HASH_ALG_SHA512;
        break;
      case TPM_ALG_SM3_256:
        // TBD: Spec not define TREE_BOOT_HASH_ALG_SM3_256 yet
        break;
      }
    }
  }
  DEBUG ((EFI_D_INFO, "TPM.HashAlgorithmBitmap - 0x%08x\n", TpmHashAlgorithmBitmap));

  DEBUG ((EFI_D_INFO, "TrEE.SupportedEventLogs - 0x%08x\n", mTcgDxeData.BsCap.SupportedEventLogs));
  mTcgDxeData.BsCap.HashAlgorithmBitmap = TpmHashAlgorithmBitmap;
  DEBUG ((EFI_D_INFO, "TrEE.HashAlgorithmBitmap - 0x%08x\n", mTcgDxeData.BsCap.HashAlgorithmBitmap));

  if (mTcgDxeData.BsCap.TrEEPresentFlag) {
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
  }

  //
  // Install ACPI Table
  //
  EfiCreateProtocolNotifyEvent (&gEfiAcpiTableProtocolGuid, TPL_CALLBACK, InstallAcpiTable, NULL, &Registration);

  //
  // Install TrEEProtocol
  //
  Status = InstallTrEE ();
  DEBUG ((EFI_D_INFO, "InstallTrEE - %r\n", Status));

  return Status;
}
