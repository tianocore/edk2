/** @file
  This module implements TCG EFI Protocol.

Caution: This module requires additional review when modified.
This driver will have external input - TcgDxePassThroughToTpm
This external input must be validated carefully to avoid security issue like
buffer overflow, integer overflow.

TcgDxePassThroughToTpm() will receive untrusted input and do basic validation.

Copyright (c) 2005 - 2018, Intel Corporation. All rights reserved.<BR>
(C) Copyright 2016 Hewlett Packard Enterprise Development LP<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiDxe.h>
#include <IndustryStandard/Tpm12.h>
#include <IndustryStandard/Acpi.h>
#include <IndustryStandard/PeImage.h>
#include <IndustryStandard/TcpaAcpi.h>

#include <Guid/GlobalVariable.h>
#include <Guid/HobList.h>
#include <Guid/TcgEventHob.h>
#include <Guid/EventGroup.h>
#include <Guid/EventExitBootServiceFailed.h>
#include <Guid/TpmInstance.h>

#include <Protocol/DevicePath.h>
#include <Protocol/TcgService.h>
#include <Protocol/AcpiTable.h>
#include <Protocol/MpService.h>

#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/HobLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/BaseLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PrintLib.h>
#include <Library/Tpm12DeviceLib.h>
#include <Library/PcdLib.h>
#include <Library/UefiLib.h>
#include <Library/ReportStatusCodeLib.h>
#include <Library/Tpm12CommandLib.h>
#include <Library/BaseCryptLib.h>

#define TCG_DXE_DATA_FROM_THIS(this)  \
  BASE_CR (this, TCG_DXE_DATA, TcgProtocol)

typedef struct _TCG_DXE_DATA {
  EFI_TCG_PROTOCOL                   TcgProtocol;
  TCG_EFI_BOOT_SERVICE_CAPABILITY    BsCap;
  EFI_TCG_CLIENT_ACPI_TABLE          *TcgClientAcpiTable;
  EFI_TCG_SERVER_ACPI_TABLE          *TcgServerAcpiTable;
  UINTN                              EventLogSize;
  UINT8                              *LastEvent;
} TCG_DXE_DATA;

EFI_TCG_CLIENT_ACPI_TABLE  mTcgClientAcpiTemplate = {
  {
    EFI_ACPI_3_0_TRUSTED_COMPUTING_PLATFORM_ALLIANCE_CAPABILITIES_TABLE_SIGNATURE,
    sizeof (mTcgClientAcpiTemplate),
    0x02                      // Revision
    //
    // Compiler initializes the remaining bytes to 0
    // These fields should be filled in in production
    //
  },
  0,                                   // 0 for PC Client Platform Class
  0,                                   // Log Area Max Length
  (EFI_PHYSICAL_ADDRESS)(SIZE_4GB - 1) // Log Area Start Address
};

//
// The following EFI_TCG_SERVER_ACPI_TABLE default setting is just one example,
// the TPM device connects to LPC, and also defined the ACPI _UID as 0xFF,
// this _UID can be changed and should match with the _UID setting of the TPM
// ACPI device object
//
EFI_TCG_SERVER_ACPI_TABLE  mTcgServerAcpiTemplate = {
  {
    EFI_ACPI_3_0_TRUSTED_COMPUTING_PLATFORM_ALLIANCE_CAPABILITIES_TABLE_SIGNATURE,
    sizeof (mTcgServerAcpiTemplate),
    0x02                      // Revision
    //
    // Compiler initializes the remaining bytes to 0
    // These fields should be filled in in production
    //
  },
  1,                                    // 1 for Server Platform Class
  0,                                    // Reserved
  0,                                    // Log Area Max Length
  (EFI_PHYSICAL_ADDRESS)(SIZE_4GB - 1), // Log Area Start Address
  0x0120,                               // TCG Specification revision 1.2
  0,                                    // Device Flags
  0,                                    // Interrupt Flags
  0,                                    // GPE
  { 0 },                                // Reserved 3 bytes
  0,                                    // Global System Interrupt
  {
    EFI_ACPI_3_0_SYSTEM_MEMORY,
    0,
    0,
    EFI_ACPI_3_0_BYTE,
    0                         // Base Address
  },
  0,                          // Reserved
  { 0 },                      // Configuration Address
  0xFF,                       // ACPI _UID value of the device, can be changed for different platforms
  0,                          // ACPI _UID value of the device, can be changed for different platforms
  0,                          // ACPI _UID value of the device, can be changed for different platforms
  0                           // ACPI _UID value of the device, can be changed for different platforms
};

UINTN   mBootAttempts  = 0;
CHAR16  mBootVarName[] = L"BootOrder";

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
  This service provides EFI protocol capability information, state information
  about the TPM, and Event Log state information.

  @param[in]  This               Indicates the calling context
  @param[out] ProtocolCapability The callee allocates memory for a TCG_BOOT_SERVICE_CAPABILITY
                                 structure and fills in the fields with the EFI protocol
                                 capability information and the current TPM state information.
  @param[out] TCGFeatureFlags    This is a pointer to the feature flags. No feature
                                 flags are currently defined so this parameter
                                 MUST be set to 0. However, in the future,
                                 feature flags may be defined that, for example,
                                 enable hash algorithm agility.
  @param[out] EventLogLocation   This is a pointer to the address of the event log in memory.
  @param[out] EventLogLastEntry  If the Event Log contains more than one entry,
                                 this is a pointer to the address of the start of
                                 the last entry in the event log in memory.

  @retval EFI_SUCCESS            Operation completed successfully.
  @retval EFI_INVALID_PARAMETER  ProtocolCapability does not match TCG capability.

**/
EFI_STATUS
EFIAPI
TcgDxeStatusCheck (
  IN      EFI_TCG_PROTOCOL                 *This,
  OUT     TCG_EFI_BOOT_SERVICE_CAPABILITY  *ProtocolCapability,
  OUT     UINT32                           *TCGFeatureFlags,
  OUT     EFI_PHYSICAL_ADDRESS             *EventLogLocation,
  OUT     EFI_PHYSICAL_ADDRESS             *EventLogLastEntry
  )
{
  TCG_DXE_DATA  *TcgData;

  TcgData = TCG_DXE_DATA_FROM_THIS (This);

  if (ProtocolCapability != NULL) {
    *ProtocolCapability = TcgData->BsCap;
  }

  if (TCGFeatureFlags != NULL) {
    *TCGFeatureFlags = 0;
  }

  if (EventLogLocation != NULL) {
    if (PcdGet8 (PcdTpmPlatformClass) == TCG_PLATFORM_TYPE_CLIENT) {
      *EventLogLocation = TcgData->TcgClientAcpiTable->Lasa;
    } else {
      *EventLogLocation = TcgData->TcgServerAcpiTable->Lasa;
    }
  }

  if (EventLogLastEntry != NULL) {
    if (TcgData->BsCap.TPMDeactivatedFlag || (!TcgData->BsCap.TPMPresentFlag)) {
      *EventLogLastEntry = (EFI_PHYSICAL_ADDRESS)(UINTN)0;
    } else {
      *EventLogLastEntry = (EFI_PHYSICAL_ADDRESS)(UINTN)TcgData->LastEvent;
    }
  }

  return EFI_SUCCESS;
}

/**
Single function calculates SHA1 digest value for all raw data. It
combines Sha1Init(), Sha1Update() and Sha1Final().

@param[in]  Data          Raw data to be digested.
@param[in]  DataLen       Size of the raw data.
@param[out] Digest        Pointer to a buffer that stores the final digest.

@retval     EFI_SUCCESS   Always successfully calculate the final digest.
**/
EFI_STATUS
EFIAPI
TpmCommHashAll (
  IN  CONST UINT8       *Data,
  IN        UINTN       DataLen,
  OUT       TPM_DIGEST  *Digest
  )
{
  VOID   *Sha1Ctx;
  UINTN  CtxSize;

  CtxSize = Sha1GetContextSize ();
  Sha1Ctx = AllocatePool (CtxSize);
  ASSERT (Sha1Ctx != NULL);

  Sha1Init (Sha1Ctx);
  Sha1Update (Sha1Ctx, Data, DataLen);
  Sha1Final (Sha1Ctx, (UINT8 *)Digest);

  FreePool (Sha1Ctx);

  return EFI_SUCCESS;
}

/**
  This service abstracts the capability to do a hash operation on a data buffer.

  @param[in]      This             Indicates the calling context
  @param[in]      HashData         Pointer to the data buffer to be hashed
  @param[in]      HashDataLen      Length of the data buffer to be hashed
  @param[in]      AlgorithmId      Identification of the Algorithm to use for the hashing operation
  @param[in, out] HashedDataLen    Resultant length of the hashed data
  @param[in, out] HashedDataResult Resultant buffer of the hashed data

  @retval EFI_SUCCESS              Operation completed successfully.
  @retval EFI_INVALID_PARAMETER    HashDataLen is NULL.
  @retval EFI_INVALID_PARAMETER    HashDataLenResult is NULL.
  @retval EFI_OUT_OF_RESOURCES     Cannot allocate buffer of size *HashedDataLen.
  @retval EFI_UNSUPPORTED          AlgorithmId not supported.
  @retval EFI_BUFFER_TOO_SMALL     *HashedDataLen < sizeof (TCG_DIGEST).

**/
EFI_STATUS
EFIAPI
TcgDxeHashAll (
  IN      EFI_TCG_PROTOCOL  *This,
  IN      UINT8             *HashData,
  IN      UINT64            HashDataLen,
  IN      TCG_ALGORITHM_ID  AlgorithmId,
  IN OUT  UINT64            *HashedDataLen,
  IN OUT  UINT8             **HashedDataResult
  )
{
  if ((HashedDataLen == NULL) || (HashedDataResult == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  switch (AlgorithmId) {
    case TPM_ALG_SHA:
      if (*HashedDataLen == 0) {
        *HashedDataLen    = sizeof (TPM_DIGEST);
        *HashedDataResult = AllocatePool ((UINTN)*HashedDataLen);
        if (*HashedDataResult == NULL) {
          return EFI_OUT_OF_RESOURCES;
        }
      }

      if (*HashedDataLen < sizeof (TPM_DIGEST)) {
        *HashedDataLen = sizeof (TPM_DIGEST);
        return EFI_BUFFER_TOO_SMALL;
      }

      *HashedDataLen = sizeof (TPM_DIGEST);

      if (*HashedDataResult == NULL) {
        *HashedDataResult = AllocatePool ((UINTN)*HashedDataLen);
      }

      return TpmCommHashAll (
               HashData,
               (UINTN)HashDataLen,
               (TPM_DIGEST *)*HashedDataResult
               );
    default:
      return EFI_UNSUPPORTED;
  }
}

/**
Add a new entry to the Event Log.

@param[in, out] EventLogPtr   Pointer to the Event Log data.
@param[in, out] LogSize       Size of the Event Log.
@param[in]      MaxSize       Maximum size of the Event Log.
@param[in]      NewEventHdr   Pointer to a TCG_PCR_EVENT_HDR data structure.
@param[in]      NewEventData  Pointer to the new event data.

@retval EFI_SUCCESS           The new event log entry was added.
@retval EFI_OUT_OF_RESOURCES  No enough memory to log the new event.

**/
EFI_STATUS
TpmCommLogEvent (
  IN OUT  UINT8              **EventLogPtr,
  IN OUT  UINTN              *LogSize,
  IN      UINTN              MaxSize,
  IN      TCG_PCR_EVENT_HDR  *NewEventHdr,
  IN      UINT8              *NewEventData
  )
{
  UINTN  NewLogSize;

  //
  // Prevent Event Overflow
  //
  if ((UINTN)NewEventHdr->EventSize > MAX_UINTN - sizeof (*NewEventHdr)) {
    return EFI_OUT_OF_RESOURCES;
  }

  NewLogSize = sizeof (*NewEventHdr) + NewEventHdr->EventSize;
  if (NewLogSize > MaxSize - *LogSize) {
    return EFI_OUT_OF_RESOURCES;
  }

  *EventLogPtr += *LogSize;
  *LogSize     += NewLogSize;
  CopyMem (*EventLogPtr, NewEventHdr, sizeof (*NewEventHdr));
  CopyMem (
    *EventLogPtr + sizeof (*NewEventHdr),
    NewEventData,
    NewEventHdr->EventSize
    );
  return EFI_SUCCESS;
}

/**
  Add a new entry to the Event Log.

  @param[in] TcgData       TCG_DXE_DATA structure.
  @param[in] NewEventHdr   Pointer to a TCG_PCR_EVENT_HDR data structure.
  @param[in] NewEventData  Pointer to the new event data.

  @retval EFI_SUCCESS           The new event log entry was added.
  @retval EFI_OUT_OF_RESOURCES  No enough memory to log the new event.

**/
EFI_STATUS
EFIAPI
TcgDxeLogEventI (
  IN      TCG_DXE_DATA       *TcgData,
  IN      TCG_PCR_EVENT_HDR  *NewEventHdr,
  IN      UINT8              *NewEventData
  )
{
  if (PcdGet8 (PcdTpmPlatformClass) == TCG_PLATFORM_TYPE_CLIENT) {
    TcgData->LastEvent = (UINT8 *)(UINTN)TcgData->TcgClientAcpiTable->Lasa;
    return TpmCommLogEvent (
             &TcgData->LastEvent,
             &TcgData->EventLogSize,
             (UINTN)TcgData->TcgClientAcpiTable->Laml,
             NewEventHdr,
             NewEventData
             );
  } else {
    TcgData->LastEvent = (UINT8 *)(UINTN)TcgData->TcgServerAcpiTable->Lasa;
    return TpmCommLogEvent (
             &TcgData->LastEvent,
             &TcgData->EventLogSize,
             (UINTN)TcgData->TcgServerAcpiTable->Laml,
             NewEventHdr,
             NewEventData
             );
  }
}

/**
  This service abstracts the capability to add an entry to the Event Log.

  @param[in]      This           Indicates the calling context
  @param[in]      TCGLogData     Pointer to the start of the data buffer containing
                                 the TCG_PCR_EVENT data structure. All fields in
                                 this structure are properly filled by the caller.
  @param[in, out] EventNumber    The event number of the event just logged
  @param[in]      Flags          Indicate additional flags. Only one flag has been
                                 defined at this time, which is 0x01 and means the
                                 extend operation should not be performed. All
                                 other bits are reserved.

  @retval EFI_SUCCESS            Operation completed successfully.
  @retval EFI_OUT_OF_RESOURCES   Insufficient memory in the event log to complete this action.

**/
EFI_STATUS
EFIAPI
TcgDxeLogEvent (
  IN      EFI_TCG_PROTOCOL  *This,
  IN      TCG_PCR_EVENT     *TCGLogData,
  IN OUT  UINT32            *EventNumber,
  IN      UINT32            Flags
  )
{
  TCG_DXE_DATA  *TcgData;

  if (TCGLogData == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  TcgData = TCG_DXE_DATA_FROM_THIS (This);

  if (TcgData->BsCap.TPMDeactivatedFlag || (!TcgData->BsCap.TPMPresentFlag)) {
    return EFI_DEVICE_ERROR;
  }

  return TcgDxeLogEventI (
           TcgData,
           (TCG_PCR_EVENT_HDR *)TCGLogData,
           TCGLogData->Event
           );
}

/**
  This service is a proxy for commands to the TPM.

  @param[in]  This                        Indicates the calling context
  @param[in]  TpmInputParameterBlockSize  Size of the TPM input parameter block
  @param[in]  TpmInputParameterBlock      Pointer to the TPM input parameter block
  @param[in]  TpmOutputParameterBlockSize Size of the TPM output parameter block
  @param[in]  TpmOutputParameterBlock     Pointer to the TPM output parameter block

  @retval     EFI_SUCCESS                 Operation completed successfully.
  @retval     EFI_INVALID_PARAMETER       Invalid ordinal.
  @retval     EFI_UNSUPPORTED             Current Task Priority Level  >= EFI_TPL_CALLBACK.
  @retval     EFI_TIMEOUT                 The TIS timed-out.

**/
EFI_STATUS
EFIAPI
TcgDxePassThroughToTpm (
  IN      EFI_TCG_PROTOCOL  *This,
  IN      UINT32            TpmInputParameterBlockSize,
  IN      UINT8             *TpmInputParameterBlock,
  IN      UINT32            TpmOutputParameterBlockSize,
  IN      UINT8             *TpmOutputParameterBlock
  )
{
  if ((TpmInputParameterBlock == NULL) ||
      (TpmOutputParameterBlock == NULL) ||
      (TpmInputParameterBlockSize == 0) ||
      (TpmOutputParameterBlockSize == 0))
  {
    return EFI_INVALID_PARAMETER;
  }

  return Tpm12SubmitCommand (
           TpmInputParameterBlockSize,
           TpmInputParameterBlock,
           &TpmOutputParameterBlockSize,
           TpmOutputParameterBlock
           );
}

/**
  Do a hash operation on a data buffer, extend a specific TPM PCR with the hash result,
  and add an entry to the Event Log.

  @param[in]      TcgData       TCG_DXE_DATA structure.
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
EFIAPI
TcgDxeHashLogExtendEventI (
  IN      TCG_DXE_DATA       *TcgData,
  IN      UINT8              *HashData,
  IN      UINT64             HashDataLen,
  IN OUT  TCG_PCR_EVENT_HDR  *NewEventHdr,
  IN      UINT8              *NewEventData
  )
{
  EFI_STATUS  Status;

  if (!TcgData->BsCap.TPMPresentFlag) {
    return EFI_DEVICE_ERROR;
  }

  if ((HashDataLen > 0) || (HashData != NULL)) {
    Status = TpmCommHashAll (
               HashData,
               (UINTN)HashDataLen,
               &NewEventHdr->Digest
               );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "TpmCommHashAll Failed. %x\n", Status));
      goto Done;
    }
  }

  Status = Tpm12Extend (
             &NewEventHdr->Digest,
             NewEventHdr->PCRIndex,
             NULL
             );
  if (!EFI_ERROR (Status)) {
    Status = TcgDxeLogEventI (TcgData, NewEventHdr, NewEventData);
  }

Done:
  if ((Status == EFI_DEVICE_ERROR) || (Status == EFI_TIMEOUT)) {
    DEBUG ((DEBUG_ERROR, "TcgDxeHashLogExtendEventI - %r. Disable TPM.\n", Status));
    TcgData->BsCap.TPMPresentFlag = FALSE;
    REPORT_STATUS_CODE (
      EFI_ERROR_CODE | EFI_ERROR_MINOR,
      (PcdGet32 (PcdStatusCodeSubClassTpmDevice) | EFI_P_EC_INTERFACE_ERROR)
      );
    Status = EFI_DEVICE_ERROR;
  }

  return Status;
}

/**
  This service abstracts the capability to do a hash operation on a data buffer,
  extend a specific TPM PCR with the hash result, and add an entry to the Event Log

  @param[in]      This               Indicates the calling context
  @param[in]      HashData           Physical address of the start of the data buffer
                                     to be hashed, extended, and logged.
  @param[in]      HashDataLen        The length, in bytes, of the buffer referenced by HashData
  @param[in]      AlgorithmId        Identification of the Algorithm to use for the hashing operation
  @param[in, out] TCGLogData         The physical address of the start of the data
                                     buffer containing the TCG_PCR_EVENT data structure.
  @param[in, out] EventNumber        The event number of the event just logged.
  @param[out]     EventLogLastEntry  Physical address of the first byte of the entry
                                     just placed in the Event Log. If the Event Log was
                                     empty when this function was called then this physical
                                     address will be the same as the physical address of
                                     the start of the Event Log.

  @retval EFI_SUCCESS                Operation completed successfully.
  @retval EFI_UNSUPPORTED            AlgorithmId != TPM_ALG_SHA.
  @retval EFI_UNSUPPORTED            Current TPL >= EFI_TPL_CALLBACK.
  @retval EFI_DEVICE_ERROR           The command was unsuccessful.

**/
EFI_STATUS
EFIAPI
TcgDxeHashLogExtendEvent (
  IN      EFI_TCG_PROTOCOL      *This,
  IN      EFI_PHYSICAL_ADDRESS  HashData,
  IN      UINT64                HashDataLen,
  IN      TPM_ALGORITHM_ID      AlgorithmId,
  IN OUT  TCG_PCR_EVENT         *TCGLogData,
  IN OUT  UINT32                *EventNumber,
  OUT  EFI_PHYSICAL_ADDRESS     *EventLogLastEntry
  )
{
  TCG_DXE_DATA  *TcgData;
  EFI_STATUS    Status;

  if ((TCGLogData == NULL) || (EventLogLastEntry == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  TcgData = TCG_DXE_DATA_FROM_THIS (This);

  if (TcgData->BsCap.TPMDeactivatedFlag || (!TcgData->BsCap.TPMPresentFlag)) {
    return EFI_DEVICE_ERROR;
  }

  if (AlgorithmId != TPM_ALG_SHA) {
    return EFI_UNSUPPORTED;
  }

  if ((HashData == 0) && (HashDataLen > 0)) {
    return EFI_INVALID_PARAMETER;
  }

  Status = TcgDxeHashLogExtendEventI (
             TcgData,
             (UINT8 *)(UINTN)HashData,
             HashDataLen,
             (TCG_PCR_EVENT_HDR *)TCGLogData,
             TCGLogData->Event
             );

  if (!EFI_ERROR (Status)) {
    *EventLogLastEntry = (EFI_PHYSICAL_ADDRESS)(UINTN)TcgData->LastEvent;
  }

  return Status;
}

TCG_DXE_DATA  mTcgDxeData = {
  {
    TcgDxeStatusCheck,
    TcgDxeHashAll,
    TcgDxeLogEvent,
    TcgDxePassThroughToTpm,
    TcgDxeHashLogExtendEvent
  },
  {
    sizeof (mTcgDxeData.BsCap),
    { 1, 2, 0, 0 },
    { 1, 2, 0, 0 },
    1,
    TRUE,
    FALSE
  },
  &mTcgClientAcpiTemplate,
  &mTcgServerAcpiTemplate,
  0,
  NULL
};

/**
  Initialize the Event Log and log events passed from the PEI phase.

  @retval EFI_SUCCESS           Operation completed successfully.
  @retval EFI_OUT_OF_RESOURCES  Out of memory.

**/
EFI_STATUS
EFIAPI
SetupEventLog (
  VOID
  )
{
  EFI_STATUS            Status;
  TCG_PCR_EVENT         *TcgEvent;
  EFI_PEI_HOB_POINTERS  GuidHob;
  EFI_PHYSICAL_ADDRESS  Lasa;

  if (PcdGet8 (PcdTpmPlatformClass) == TCG_PLATFORM_TYPE_CLIENT) {
    Lasa = mTcgClientAcpiTemplate.Lasa;

    Status = gBS->AllocatePages (
                    AllocateMaxAddress,
                    EfiACPIMemoryNVS,
                    EFI_SIZE_TO_PAGES (PcdGet32 (PcdTcgLogAreaMinLen)),
                    &Lasa
                    );
    if (EFI_ERROR (Status)) {
      return Status;
    }

    mTcgClientAcpiTemplate.Lasa = Lasa;
    //
    // To initialize them as 0xFF is recommended
    // because the OS can know the last entry for that.
    //
    SetMem ((VOID *)(UINTN)mTcgClientAcpiTemplate.Lasa, PcdGet32 (PcdTcgLogAreaMinLen), 0xFF);
    mTcgClientAcpiTemplate.Laml = PcdGet32 (PcdTcgLogAreaMinLen);
  } else {
    Lasa = mTcgServerAcpiTemplate.Lasa;

    Status = gBS->AllocatePages (
                    AllocateMaxAddress,
                    EfiACPIMemoryNVS,
                    EFI_SIZE_TO_PAGES (PcdGet32 (PcdTcgLogAreaMinLen)),
                    &Lasa
                    );
    if (EFI_ERROR (Status)) {
      return Status;
    }

    mTcgServerAcpiTemplate.Lasa = Lasa;
    //
    // To initialize them as 0xFF is recommended
    // because the OS can know the last entry for that.
    //
    SetMem ((VOID *)(UINTN)mTcgServerAcpiTemplate.Lasa, PcdGet32 (PcdTcgLogAreaMinLen), 0xFF);
    mTcgServerAcpiTemplate.Laml = PcdGet32 (PcdTcgLogAreaMinLen);
  }

  GuidHob.Raw = GetHobList ();
  while (!EFI_ERROR (Status) &&
         (GuidHob.Raw = GetNextGuidHob (&gTcgEventEntryHobGuid, GuidHob.Raw)) != NULL)
  {
    TcgEvent    = GET_GUID_HOB_DATA (GuidHob.Guid);
    GuidHob.Raw = GET_NEXT_HOB (GuidHob);
    Status      = TcgDxeLogEventI (
                    &mTcgDxeData,
                    (TCG_PCR_EVENT_HDR *)TcgEvent,
                    TcgEvent->Event
                    );
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
EFIAPI
TcgMeasureAction (
  IN      CHAR8  *String
  )
{
  TCG_PCR_EVENT_HDR  TcgEvent;

  TcgEvent.PCRIndex  = 5;
  TcgEvent.EventType = EV_EFI_ACTION;
  TcgEvent.EventSize = (UINT32)AsciiStrLen (String);
  return TcgDxeHashLogExtendEventI (
           &mTcgDxeData,
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
EFIAPI
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

      Status = TcgDxeHashLogExtendEventI (
                 &mTcgDxeData,
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
EFIAPI
MeasureSeparatorEvent (
  IN      TPM_PCRINDEX  PCRIndex
  )
{
  TCG_PCR_EVENT_HDR  TcgEvent;
  UINT32             EventData;

  EventData          = 0;
  TcgEvent.PCRIndex  = PCRIndex;
  TcgEvent.EventType = EV_SEPARATOR;
  TcgEvent.EventSize = (UINT32)sizeof (EventData);
  return TcgDxeHashLogExtendEventI (
           &mTcgDxeData,
           (UINT8 *)&EventData,
           sizeof (EventData),
           &TcgEvent,
           (UINT8 *)&EventData
           );
}

/**
  Read an EFI Variable.

  This function allocates a buffer to return the contents of the variable. The caller is
  responsible for freeing the buffer.

  @param[in]  VarName     A Null-terminated string that is the name of the vendor's variable.
  @param[in]  VendorGuid  A unique identifier for the vendor.
  @param[out] VarSize     The size of the variable data.

  @return A pointer to the buffer to return the contents of the variable.Otherwise NULL.

**/
VOID *
EFIAPI
ReadVariable (
  IN      CHAR16    *VarName,
  IN      EFI_GUID  *VendorGuid,
  OUT     UINTN     *VarSize
  )
{
  EFI_STATUS  Status;
  VOID        *VarData;

  *VarSize = 0;
  Status   = gRT->GetVariable (
                    VarName,
                    VendorGuid,
                    NULL,
                    VarSize,
                    NULL
                    );
  if (Status != EFI_BUFFER_TOO_SMALL) {
    return NULL;
  }

  VarData = AllocatePool (*VarSize);
  if (VarData != NULL) {
    Status = gRT->GetVariable (
                    VarName,
                    VendorGuid,
                    NULL,
                    VarSize,
                    VarData
                    );
    if (EFI_ERROR (Status)) {
      FreePool (VarData);
      VarData  = NULL;
      *VarSize = 0;
    }
  }

  return VarData;
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
EFIAPI
MeasureVariable (
  IN      TPM_PCRINDEX   PCRIndex,
  IN      TCG_EVENTTYPE  EventType,
  IN      CHAR16         *VarName,
  IN      EFI_GUID       *VendorGuid,
  IN      VOID           *VarData,
  IN      UINTN          VarSize
  )
{
  EFI_STATUS         Status;
  TCG_PCR_EVENT_HDR  TcgEvent;
  UINTN              VarNameLength;
  EFI_VARIABLE_DATA  *VarLog;

  VarNameLength      = StrLen (VarName);
  TcgEvent.PCRIndex  = PCRIndex;
  TcgEvent.EventType = EventType;
  TcgEvent.EventSize = (UINT32)(sizeof (*VarLog) + VarNameLength * sizeof (*VarName) + VarSize
                                - sizeof (VarLog->UnicodeName) - sizeof (VarLog->VariableData));

  VarLog = (EFI_VARIABLE_DATA *)AllocatePool (TcgEvent.EventSize);
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
  CopyMem (
    (CHAR16 *)VarLog->UnicodeName + VarNameLength,
    VarData,
    VarSize
    );

  Status = TcgDxeHashLogExtendEventI (
             &mTcgDxeData,
             (UINT8 *)VarLog,
             TcgEvent.EventSize,
             &TcgEvent,
             (UINT8 *)VarLog
             );
  FreePool (VarLog);
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
EFIAPI
ReadAndMeasureBootVariable (
  IN      CHAR16    *VarName,
  IN      EFI_GUID  *VendorGuid,
  OUT     UINTN     *VarSize,
  OUT     VOID      **VarData
  )
{
  EFI_STATUS  Status;

  *VarData = ReadVariable (VarName, VendorGuid, VarSize);
  if (*VarData == NULL) {
    return EFI_NOT_FOUND;
  }

  Status = MeasureVariable (
             5,
             EV_EFI_VARIABLE_BOOT,
             VarName,
             VendorGuid,
             *VarData,
             *VarSize
             );
  return Status;
}

/**
  Measure and log all EFI boot variables, and extend the measurement result into a specific PCR.

  The EFI boot variables are BootOrder and Boot#### variables.

  @retval EFI_SUCCESS           Operation completed successfully.
  @retval EFI_OUT_OF_RESOURCES  Out of memory.
  @retval EFI_DEVICE_ERROR      The operation was unsuccessful.

**/
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
               EFI_CALLING_EFI_APPLICATION
               );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a not Measured. Error!\n", EFI_CALLING_EFI_APPLICATION));
    }

    //
    // 2. Draw a line between pre-boot env and entering post-boot env.
    //
    for (PcrIndex = 0; PcrIndex < 8; PcrIndex++) {
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
               EFI_RETURNING_FROM_EFI_APPLICATION
               );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a not Measured. Error!\n", EFI_RETURNING_FROM_EFI_APPLICATION));
    }
  }

  DEBUG ((DEBUG_INFO, "TPM TcgDxe Measure Data when ReadyToBoot\n"));
  //
  // Increase boot attempt counter.
  //
  mBootAttempts++;
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
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  UINTN                    TableKey;
  EFI_STATUS               Status;
  EFI_ACPI_TABLE_PROTOCOL  *AcpiTable;
  UINT8                    Checksum;
  UINT64                   OemTableId;

  Status = gBS->LocateProtocol (&gEfiAcpiTableProtocolGuid, NULL, (VOID **)&AcpiTable);
  if (EFI_ERROR (Status)) {
    return;
  }

  if (PcdGet8 (PcdTpmPlatformClass) == TCG_PLATFORM_TYPE_CLIENT) {
    CopyMem (mTcgClientAcpiTemplate.Header.OemId, PcdGetPtr (PcdAcpiDefaultOemId), sizeof (mTcgClientAcpiTemplate.Header.OemId));
    OemTableId = PcdGet64 (PcdAcpiDefaultOemTableId);
    CopyMem (&mTcgClientAcpiTemplate.Header.OemTableId, &OemTableId, sizeof (UINT64));
    mTcgClientAcpiTemplate.Header.OemRevision     = PcdGet32 (PcdAcpiDefaultOemRevision);
    mTcgClientAcpiTemplate.Header.CreatorId       = PcdGet32 (PcdAcpiDefaultCreatorId);
    mTcgClientAcpiTemplate.Header.CreatorRevision = PcdGet32 (PcdAcpiDefaultCreatorRevision);
    //
    // The ACPI table must be checksummed before calling the InstallAcpiTable()
    // service of the ACPI table protocol to install it.
    //
    Checksum                               = CalculateCheckSum8 ((UINT8 *)&mTcgClientAcpiTemplate, sizeof (mTcgClientAcpiTemplate));
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
    mTcgServerAcpiTemplate.Header.OemRevision     = PcdGet32 (PcdAcpiDefaultOemRevision);
    mTcgServerAcpiTemplate.Header.CreatorId       = PcdGet32 (PcdAcpiDefaultCreatorId);
    mTcgServerAcpiTemplate.Header.CreatorRevision = PcdGet32 (PcdAcpiDefaultCreatorRevision);
    //
    // The ACPI table must be checksummed before calling the InstallAcpiTable()
    // service of the ACPI table protocol to install it.
    //
    Checksum                               = CalculateCheckSum8 ((UINT8 *)&mTcgServerAcpiTemplate, sizeof (mTcgServerAcpiTemplate));
    mTcgServerAcpiTemplate.Header.Checksum = Checksum;

    mTcgServerAcpiTemplate.BaseAddress.Address = PcdGet64 (PcdTpmBaseAddress);
    Status                                     = AcpiTable->InstallAcpiTable (
                                                              AcpiTable,
                                                              &mTcgServerAcpiTemplate,
                                                              sizeof (mTcgServerAcpiTemplate),
                                                              &TableKey
                                                              );
  }

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Tcg Acpi Table installation failure"));
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
  IN      EFI_EVENT  Event,
  IN      VOID       *Context
  )
{
  EFI_STATUS  Status;

  //
  // Measure invocation of ExitBootServices,
  //
  Status = TcgMeasureAction (
             EFI_EXIT_BOOT_SERVICES_INVOCATION
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a not Measured. Error!\n", EFI_EXIT_BOOT_SERVICES_INVOCATION));
  }

  //
  // Measure success of ExitBootServices
  //
  Status = TcgMeasureAction (
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
  Status = TcgMeasureAction (
             EFI_EXIT_BOOT_SERVICES_FAILED
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a not Measured. Error!\n", EFI_EXIT_BOOT_SERVICES_FAILED));
  }
}

/**
  Get TPM Deactivated state.

  @param[out] TPMDeactivatedFlag   Returns TPM Deactivated state.

  @retval EFI_SUCCESS              Operation completed successfully.
  @retval EFI_DEVICE_ERROR         The operation was unsuccessful.

**/
EFI_STATUS
GetTpmStatus (
  OUT BOOLEAN  *TPMDeactivatedFlag
  )
{
  EFI_STATUS         Status;
  TPM_STCLEAR_FLAGS  VolatileFlags;

  Status = Tpm12GetCapabilityFlagVolatile (&VolatileFlags);
  if (!EFI_ERROR (Status)) {
    *TPMDeactivatedFlag = VolatileFlags.deactivated;
  }

  return Status;
}

/**
  The driver's entry point.

  It publishes EFI TCG Protocol.

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

  if (!CompareGuid (PcdGetPtr (PcdTpmInstanceGuid), &gEfiTpmDeviceInstanceTpm12Guid)) {
    DEBUG ((DEBUG_ERROR, "No TPM12 instance required!\n"));
    return EFI_UNSUPPORTED;
  }

  if (GetFirstGuidHob (&gTpmErrorHobGuid) != NULL) {
    DEBUG ((DEBUG_ERROR, "TPM error!\n"));
    return EFI_DEVICE_ERROR;
  }

  Status = Tpm12RequestUseTpm ();
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "TPM not detected!\n"));
    return Status;
  }

  Status = GetTpmStatus (&mTcgDxeData.BsCap.TPMDeactivatedFlag);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "DriverEntry: TPM not working properly\n"
      ));
    return Status;
  }

  Status = gBS->InstallProtocolInterface (
                  &ImageHandle,
                  &gEfiTcgProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  &mTcgDxeData.TcgProtocol
                  );
  if (!EFI_ERROR (Status) && (!mTcgDxeData.BsCap.TPMDeactivatedFlag) && mTcgDxeData.BsCap.TPMPresentFlag) {
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
  }

  //
  // Install ACPI Table
  //
  EfiCreateProtocolNotifyEvent (&gEfiAcpiTableProtocolGuid, TPL_CALLBACK, InstallAcpiTable, NULL, &Registration);

  return Status;
}
