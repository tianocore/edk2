/** @file

Copyright (c) 2010 - 2019, Intel Corporation. All rights reserved.<BR>

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiPei.h>
#include <PiDxe.h>
#include <PiSmm.h>
#include <Library/PeiServicesTablePointerLib.h>
#include <Library/PeiServicesLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/LockBoxLib.h>
#include <Library/HobLib.h>
#include <Library/DebugLib.h>
#include <Library/PcdLib.h>
#include <Protocol/SmmCommunication.h>
#include <Ppi/SmmCommunication.h>
#include <Ppi/SmmAccess.h>
#include <Guid/AcpiS3Context.h>
#include <Guid/SmmLockBox.h>

#include "SmmLockBoxLibPrivate.h"

#if defined (MDE_CPU_IA32)
typedef struct _LIST_ENTRY64 LIST_ENTRY64;
struct _LIST_ENTRY64 {
  LIST_ENTRY64    *ForwardLink;
  UINT32          Reserved1;
  LIST_ENTRY64    *BackLink;
  UINT32          Reserved2;
};

typedef struct {
  EFI_TABLE_HEADER    Hdr;
  UINT64              SmmFirmwareVendor;
  UINT64              SmmFirmwareRevision;
  UINT64              SmmInstallConfigurationTable;
  UINT64              SmmIoMemRead;
  UINT64              SmmIoMemWrite;
  UINT64              SmmIoIoRead;
  UINT64              SmmIoIoWrite;
  UINT64              SmmAllocatePool;
  UINT64              SmmFreePool;
  UINT64              SmmAllocatePages;
  UINT64              SmmFreePages;
  UINT64              SmmStartupThisAp;
  UINT64              CurrentlyExecutingCpu;
  UINT64              NumberOfCpus;
  UINT64              CpuSaveStateSize;
  UINT64              CpuSaveState;
  UINT64              NumberOfTableEntries;
  UINT64              SmmConfigurationTable;
} EFI_SMM_SYSTEM_TABLE2_64;

typedef struct {
  EFI_GUID    VendorGuid;
  UINT64      VendorTable;
} EFI_CONFIGURATION_TABLE64;
#endif

#if defined (MDE_CPU_X64)
typedef LIST_ENTRY              LIST_ENTRY64;
typedef EFI_SMM_SYSTEM_TABLE2   EFI_SMM_SYSTEM_TABLE2_64;
typedef EFI_CONFIGURATION_TABLE EFI_CONFIGURATION_TABLE64;
#endif

/**
  This function return first node of LinkList queue.

  @param LockBoxQueue  LinkList queue

  @return first node of LinkList queue
**/
LIST_ENTRY *
InternalInitLinkDxe (
  IN LIST_ENTRY  *LinkList
  )
{
  if ((sizeof (UINTN) == sizeof (UINT32)) && (FeaturePcdGet (PcdDxeIplSwitchToLongMode))) {
    //
    // 32 PEI + 64 DXE
    //
    return (LIST_ENTRY *)(((LIST_ENTRY64 *)LinkList)->ForwardLink);
  } else {
    return LinkList->ForwardLink;
  }
}

/**
  This function return next node of LinkList.

  @param Link  LinkList node

  @return next node of LinkList
**/
LIST_ENTRY *
InternalNextLinkDxe (
  IN LIST_ENTRY  *Link
  )
{
  if ((sizeof (UINTN) == sizeof (UINT32)) && (FeaturePcdGet (PcdDxeIplSwitchToLongMode))) {
    //
    // 32 PEI + 64 DXE
    //
    return (LIST_ENTRY *)(((LIST_ENTRY64 *)Link)->ForwardLink);
  } else {
    return Link->ForwardLink;
  }
}

/**
  This function find LockBox by GUID from SMRAM.

  @param LockBoxQueue The LockBox queue in SMRAM
  @param Guid         The guid to indentify the LockBox

  @return LockBoxData
**/
SMM_LOCK_BOX_DATA *
InternalFindLockBoxByGuidFromSmram (
  IN LIST_ENTRY  *LockBoxQueue,
  IN EFI_GUID    *Guid
  )
{
  LIST_ENTRY         *Link;
  SMM_LOCK_BOX_DATA  *LockBox;

  for (Link = InternalInitLinkDxe (LockBoxQueue);
       Link != LockBoxQueue;
       Link = InternalNextLinkDxe (Link))
  {
    LockBox = BASE_CR (
                Link,
                SMM_LOCK_BOX_DATA,
                Link
                );
    if (CompareGuid (&LockBox->Guid, Guid)) {
      return LockBox;
    }
  }

  return NULL;
}

/**
  Get VendorTable by VendorGuid in Smst.

  @param Signature  Signature of SMM_S3_RESUME_STATE
  @param Smst       SMM system table
  @param VendorGuid vendor guid

  @return vendor table.
**/
VOID *
InternalSmstGetVendorTableByGuid (
  IN UINT64                 Signature,
  IN EFI_SMM_SYSTEM_TABLE2  *Smst,
  IN EFI_GUID               *VendorGuid
  )
{
  EFI_CONFIGURATION_TABLE    *SmmConfigurationTable;
  UINTN                      NumberOfTableEntries;
  UINTN                      Index;
  EFI_SMM_SYSTEM_TABLE2_64   *Smst64;
  EFI_CONFIGURATION_TABLE64  *SmmConfigurationTable64;

  if ((sizeof (UINTN) == sizeof (UINT32)) && (FeaturePcdGet (PcdDxeIplSwitchToLongMode))) {
    //
    // 32 PEI + 64 DXE
    //
    Smst64                  = (EFI_SMM_SYSTEM_TABLE2_64 *)Smst;
    SmmConfigurationTable64 = (EFI_CONFIGURATION_TABLE64 *)(UINTN)Smst64->SmmConfigurationTable;
    NumberOfTableEntries    = (UINTN)Smst64->NumberOfTableEntries;
    for (Index = 0; Index < NumberOfTableEntries; Index++) {
      if (CompareGuid (&SmmConfigurationTable64[Index].VendorGuid, VendorGuid)) {
        return (VOID *)(UINTN)SmmConfigurationTable64[Index].VendorTable;
      }
    }

    return NULL;
  } else {
    SmmConfigurationTable = Smst->SmmConfigurationTable;
    NumberOfTableEntries  = Smst->NumberOfTableEntries;
    for (Index = 0; Index < NumberOfTableEntries; Index++) {
      if (CompareGuid (&SmmConfigurationTable[Index].VendorGuid, VendorGuid)) {
        return (VOID *)SmmConfigurationTable[Index].VendorTable;
      }
    }

    return NULL;
  }
}

/**
  Get SMM LockBox context.

  @return SMM LockBox context.
**/
SMM_LOCK_BOX_CONTEXT *
InternalGetSmmLockBoxContext (
  VOID
  )
{
  EFI_SMRAM_DESCRIPTOR  *SmramDescriptor;
  SMM_S3_RESUME_STATE   *SmmS3ResumeState;
  VOID                  *GuidHob;
  SMM_LOCK_BOX_CONTEXT  *SmmLockBoxContext;

  GuidHob = GetFirstGuidHob (&gEfiAcpiVariableGuid);
  ASSERT (GuidHob != NULL);
  SmramDescriptor  = (EFI_SMRAM_DESCRIPTOR *)GET_GUID_HOB_DATA (GuidHob);
  SmmS3ResumeState = (SMM_S3_RESUME_STATE *)(UINTN)SmramDescriptor->CpuStart;

  SmmLockBoxContext = (SMM_LOCK_BOX_CONTEXT *)InternalSmstGetVendorTableByGuid (
                                                SmmS3ResumeState->Signature,
                                                (EFI_SMM_SYSTEM_TABLE2 *)(UINTN)SmmS3ResumeState->Smst,
                                                &gEfiSmmLockBoxCommunicationGuid
                                                );
  ASSERT (SmmLockBoxContext != NULL);

  return SmmLockBoxContext;
}

/**
  This function will restore confidential information from lockbox in SMRAM directly.

  @param Guid   the guid to identify the confidential information
  @param Buffer the address of the restored confidential information
                NULL means restored to original address, Length MUST be NULL at same time.
  @param Length the length of the restored confidential information

  @retval RETURN_SUCCESS            the information is restored successfully.
  @retval RETURN_WRITE_PROTECTED    Buffer and Length are NULL, but the LockBox has no
                                    LOCK_BOX_ATTRIBUTE_RESTORE_IN_PLACE attribute.
  @retval RETURN_BUFFER_TOO_SMALL   the Length is too small to hold the confidential information.
  @retval RETURN_NOT_FOUND          the requested GUID not found.
**/
EFI_STATUS
InternalRestoreLockBoxFromSmram (
  IN  GUID       *Guid,
  IN  VOID       *Buffer  OPTIONAL,
  IN  OUT UINTN  *Length  OPTIONAL
  )
{
  PEI_SMM_ACCESS_PPI    *SmmAccess;
  UINTN                 Index;
  EFI_STATUS            Status;
  SMM_LOCK_BOX_CONTEXT  *SmmLockBoxContext;
  LIST_ENTRY            *LockBoxQueue;
  SMM_LOCK_BOX_DATA     *LockBox;
  VOID                  *RestoreBuffer;

  //
  // Get needed resource
  //
  Status = PeiServicesLocatePpi (
             &gPeiSmmAccessPpiGuid,
             0,
             NULL,
             (VOID **)&SmmAccess
             );
  if (!EFI_ERROR (Status)) {
    for (Index = 0; !EFI_ERROR (Status); Index++) {
      Status = SmmAccess->Open ((EFI_PEI_SERVICES **)GetPeiServicesTablePointer (), SmmAccess, Index);
    }
  }

  //
  // Get LockBox context
  //
  SmmLockBoxContext = InternalGetSmmLockBoxContext ();
  LockBoxQueue      = (LIST_ENTRY *)(UINTN)SmmLockBoxContext->LockBoxDataAddress;

  //
  // We do NOT check Buffer address in SMRAM, because if SMRAM not locked, we trust the caller.
  //

  //
  // Restore this, Buffer and Length MUST be both NULL or both non-NULL
  //

  //
  // Find LockBox
  //
  LockBox = InternalFindLockBoxByGuidFromSmram (LockBoxQueue, Guid);
  if (LockBox == NULL) {
    //
    // Not found
    //
    return EFI_NOT_FOUND;
  }

  //
  // Set RestoreBuffer
  //
  if (Buffer != NULL) {
    //
    // restore to new buffer
    //
    RestoreBuffer = Buffer;
  } else {
    //
    // restore to original buffer
    //
    if ((LockBox->Attributes & LOCK_BOX_ATTRIBUTE_RESTORE_IN_PLACE) == 0) {
      return EFI_WRITE_PROTECTED;
    }

    RestoreBuffer = (VOID *)(UINTN)LockBox->Buffer;
  }

  //
  // Set RestoreLength
  //
  if (Length != NULL) {
    if (*Length < (UINTN)LockBox->Length) {
      //
      // Input buffer is too small to hold all data.
      //
      *Length = (UINTN)LockBox->Length;
      return EFI_BUFFER_TOO_SMALL;
    }

    *Length = (UINTN)LockBox->Length;
  }

  //
  // Restore data
  //
  CopyMem (RestoreBuffer, (VOID *)(UINTN)LockBox->SmramBuffer, (UINTN)LockBox->Length);

  //
  // Done
  //
  return EFI_SUCCESS;
}

/**
  This function will restore confidential information from all lockbox which have RestoreInPlace attribute.

  @retval RETURN_SUCCESS            the information is restored successfully.
**/
EFI_STATUS
InternalRestoreAllLockBoxInPlaceFromSmram (
  VOID
  )
{
  PEI_SMM_ACCESS_PPI    *SmmAccess;
  UINTN                 Index;
  EFI_STATUS            Status;
  SMM_LOCK_BOX_CONTEXT  *SmmLockBoxContext;
  LIST_ENTRY            *LockBoxQueue;
  SMM_LOCK_BOX_DATA     *LockBox;
  LIST_ENTRY            *Link;

  //
  // Get needed resource
  //
  Status = PeiServicesLocatePpi (
             &gPeiSmmAccessPpiGuid,
             0,
             NULL,
             (VOID **)&SmmAccess
             );
  if (!EFI_ERROR (Status)) {
    for (Index = 0; !EFI_ERROR (Status); Index++) {
      Status = SmmAccess->Open ((EFI_PEI_SERVICES **)GetPeiServicesTablePointer (), SmmAccess, Index);
    }
  }

  //
  // Get LockBox context
  //
  SmmLockBoxContext = InternalGetSmmLockBoxContext ();
  LockBoxQueue      = (LIST_ENTRY *)(UINTN)SmmLockBoxContext->LockBoxDataAddress;

  //
  // We do NOT check Buffer address in SMRAM, because if SMRAM not locked, we trust the caller.
  //

  //
  // Restore all, Buffer and Length MUST be NULL
  //
  for (Link = InternalInitLinkDxe (LockBoxQueue);
       Link != LockBoxQueue;
       Link = InternalNextLinkDxe (Link))
  {
    LockBox = BASE_CR (
                Link,
                SMM_LOCK_BOX_DATA,
                Link
                );
    if ((LockBox->Attributes & LOCK_BOX_ATTRIBUTE_RESTORE_IN_PLACE) != 0) {
      //
      // Restore data
      //
      CopyMem ((VOID *)(UINTN)LockBox->Buffer, (VOID *)(UINTN)LockBox->SmramBuffer, (UINTN)LockBox->Length);
    }
  }

  //
  // Done
  //
  return EFI_SUCCESS;
}

/**
  This function will save confidential information to lockbox.

  @param Guid       the guid to identify the confidential information
  @param Buffer     the address of the confidential information
  @param Length     the length of the confidential information

  @retval RETURN_SUCCESS            the information is saved successfully.
  @retval RETURN_INVALID_PARAMETER  the Guid is NULL, or Buffer is NULL, or Length is 0
  @retval RETURN_ALREADY_STARTED    the requested GUID already exist.
  @retval RETURN_OUT_OF_RESOURCES   no enough resource to save the information.
  @retval RETURN_ACCESS_DENIED      it is too late to invoke this interface
  @retval RETURN_NOT_STARTED        it is too early to invoke this interface
  @retval RETURN_UNSUPPORTED        the service is not supported by implementaion.
**/
RETURN_STATUS
EFIAPI
SaveLockBox (
  IN  GUID   *Guid,
  IN  VOID   *Buffer,
  IN  UINTN  Length
  )
{
  ASSERT (FALSE);

  //
  // No support to save at PEI phase
  //
  return RETURN_UNSUPPORTED;
}

/**
  This function will set lockbox attributes.

  @param Guid       the guid to identify the confidential information
  @param Attributes the attributes of the lockbox

  @retval RETURN_SUCCESS            the information is saved successfully.
  @retval RETURN_INVALID_PARAMETER  attributes is invalid.
  @retval RETURN_NOT_FOUND          the requested GUID not found.
  @retval RETURN_ACCESS_DENIED      it is too late to invoke this interface
  @retval RETURN_NOT_STARTED        it is too early to invoke this interface
  @retval RETURN_UNSUPPORTED        the service is not supported by implementaion.
**/
RETURN_STATUS
EFIAPI
SetLockBoxAttributes (
  IN  GUID    *Guid,
  IN  UINT64  Attributes
  )
{
  ASSERT (FALSE);

  //
  // No support to save at PEI phase
  //
  return RETURN_UNSUPPORTED;
}

/**
  This function will update confidential information to lockbox.

  @param Guid   the guid to identify the original confidential information
  @param Offset the offset of the original confidential information
  @param Buffer the address of the updated confidential information
  @param Length the length of the updated confidential information

  @retval RETURN_SUCCESS            the information is saved successfully.
  @retval RETURN_INVALID_PARAMETER  the Guid is NULL, or Buffer is NULL, or Length is 0.
  @retval RETURN_NOT_FOUND          the requested GUID not found.
  @retval RETURN_BUFFER_TOO_SMALL   for lockbox without attribute LOCK_BOX_ATTRIBUTE_RESTORE_IN_S3_ONLY,
                                    the original buffer to too small to hold new information.
  @retval RETURN_OUT_OF_RESOURCES   for lockbox with attribute LOCK_BOX_ATTRIBUTE_RESTORE_IN_S3_ONLY,
                                    no enough resource to save the information.
  @retval RETURN_ACCESS_DENIED      it is too late to invoke this interface
  @retval RETURN_NOT_STARTED        it is too early to invoke this interface
  @retval RETURN_UNSUPPORTED        the service is not supported by implementaion.
**/
RETURN_STATUS
EFIAPI
UpdateLockBox (
  IN  GUID   *Guid,
  IN  UINTN  Offset,
  IN  VOID   *Buffer,
  IN  UINTN  Length
  )
{
  ASSERT (FALSE);

  //
  // No support to update at PEI phase
  //
  return RETURN_UNSUPPORTED;
}

/**
  This function will restore confidential information from lockbox.

  @param Guid   the guid to identify the confidential information
  @param Buffer the address of the restored confidential information
                NULL means restored to original address, Length MUST be NULL at same time.
  @param Length the length of the restored confidential information

  @retval RETURN_SUCCESS            the information is restored successfully.
  @retval RETURN_INVALID_PARAMETER  the Guid is NULL, or one of Buffer and Length is NULL.
  @retval RETURN_WRITE_PROTECTED    Buffer and Length are NULL, but the LockBox has no
                                    LOCK_BOX_ATTRIBUTE_RESTORE_IN_PLACE attribute.
  @retval RETURN_BUFFER_TOO_SMALL   the Length is too small to hold the confidential information.
  @retval RETURN_NOT_FOUND          the requested GUID not found.
  @retval RETURN_NOT_STARTED        it is too early to invoke this interface
  @retval RETURN_ACCESS_DENIED      not allow to restore to the address
  @retval RETURN_UNSUPPORTED        the service is not supported by implementaion.
**/
RETURN_STATUS
EFIAPI
RestoreLockBox (
  IN  GUID       *Guid,
  IN  VOID       *Buffer  OPTIONAL,
  IN  OUT UINTN  *Length  OPTIONAL
  )
{
  EFI_STATUS                          Status;
  EFI_PEI_SMM_COMMUNICATION_PPI       *SmmCommunicationPpi;
  EFI_SMM_LOCK_BOX_PARAMETER_RESTORE  *LockBoxParameterRestore;
  EFI_SMM_COMMUNICATE_HEADER          *CommHeader;
  UINT8                               CommBuffer[sizeof (EFI_GUID) + sizeof (UINT64) + sizeof (EFI_SMM_LOCK_BOX_PARAMETER_RESTORE)];
  UINTN                               CommSize;
  UINT64                              MessageLength;

  //
  // Please aware that there is UINTN in EFI_SMM_COMMUNICATE_HEADER. It might be UINT64 in DXE, while it is UINT32 in PEI.
  // typedef struct {
  //   EFI_GUID  HeaderGuid;
  //   UINTN     MessageLength;
  //   UINT8     Data[1];
  // } EFI_SMM_COMMUNICATE_HEADER;
  //

  DEBUG ((DEBUG_INFO, "SmmLockBoxPeiLib RestoreLockBox - Enter\n"));

  //
  // Basic check
  //
  if ((Guid == NULL) ||
      ((Buffer == NULL) && (Length != NULL)) ||
      ((Buffer != NULL) && (Length == NULL)))
  {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Get needed resource
  //
  Status = PeiServicesLocatePpi (
             &gEfiPeiSmmCommunicationPpiGuid,
             0,
             NULL,
             (VOID **)&SmmCommunicationPpi
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "SmmLockBoxPeiLib LocatePpi - (%r)\n", Status));
    Status = InternalRestoreLockBoxFromSmram (Guid, Buffer, Length);
    DEBUG ((DEBUG_INFO, "SmmLockBoxPeiLib RestoreLockBox - Exit (%r)\n", Status));
    return Status;
  }

  //
  // Prepare parameter
  //
  CommHeader = (EFI_SMM_COMMUNICATE_HEADER *)&CommBuffer[0];
  CopyMem (&CommHeader->HeaderGuid, &gEfiSmmLockBoxCommunicationGuid, sizeof (gEfiSmmLockBoxCommunicationGuid));
  if ((sizeof (UINTN) == sizeof (UINT32)) && (FeaturePcdGet (PcdDxeIplSwitchToLongMode))) {
    MessageLength = sizeof (*LockBoxParameterRestore);
    CopyMem (&CommBuffer[OFFSET_OF (EFI_SMM_COMMUNICATE_HEADER, MessageLength)], &MessageLength, sizeof (MessageLength));
  } else {
    CommHeader->MessageLength = sizeof (*LockBoxParameterRestore);
  }

  DEBUG ((DEBUG_INFO, "SmmLockBoxPeiLib CommBuffer - %x\n", &CommBuffer[0]));
  if ((sizeof (UINTN) == sizeof (UINT32)) && (FeaturePcdGet (PcdDxeIplSwitchToLongMode))) {
    LockBoxParameterRestore = (EFI_SMM_LOCK_BOX_PARAMETER_RESTORE *)&CommBuffer[OFFSET_OF (EFI_SMM_COMMUNICATE_HEADER, MessageLength) + sizeof (UINT64)];
  } else {
    LockBoxParameterRestore = (EFI_SMM_LOCK_BOX_PARAMETER_RESTORE *)&CommBuffer[OFFSET_OF (EFI_SMM_COMMUNICATE_HEADER, MessageLength) + sizeof (UINTN)];
  }

  DEBUG ((DEBUG_INFO, "SmmLockBoxPeiLib LockBoxParameterRestore - %x\n", LockBoxParameterRestore));
  LockBoxParameterRestore->Header.Command      = EFI_SMM_LOCK_BOX_COMMAND_RESTORE;
  LockBoxParameterRestore->Header.DataLength   = sizeof (*LockBoxParameterRestore);
  LockBoxParameterRestore->Header.ReturnStatus = (UINT64)-1;
  if (Guid != 0) {
    CopyMem (&LockBoxParameterRestore->Guid, Guid, sizeof (*Guid));
  } else {
    ZeroMem (&LockBoxParameterRestore->Guid, sizeof (*Guid));
  }

  LockBoxParameterRestore->Buffer = (EFI_PHYSICAL_ADDRESS)(UINTN)Buffer;
  if (Length != NULL) {
    LockBoxParameterRestore->Length = (EFI_PHYSICAL_ADDRESS)*Length;
  } else {
    LockBoxParameterRestore->Length = 0;
  }

  //
  // Send command
  //
  CommSize = sizeof (CommBuffer);
  Status   = SmmCommunicationPpi->Communicate (
                                    SmmCommunicationPpi,
                                    &CommBuffer[0],
                                    &CommSize
                                    );
  if (Status == EFI_NOT_STARTED) {
    //
    // Pei SMM communication not ready yet, so we access SMRAM directly
    //
    DEBUG ((DEBUG_INFO, "SmmLockBoxPeiLib Communicate - (%r)\n", Status));
    Status                                       = InternalRestoreLockBoxFromSmram (Guid, Buffer, Length);
    LockBoxParameterRestore->Header.ReturnStatus = (UINT64)Status;
    if (Length != NULL) {
      LockBoxParameterRestore->Length = (UINT64)*Length;
    }
  }

  if (Length != NULL) {
    *Length = (UINTN)LockBoxParameterRestore->Length;
  }

  Status = (EFI_STATUS)LockBoxParameterRestore->Header.ReturnStatus;
  if (Status != EFI_SUCCESS) {
    // Need or MAX_BIT, because there might be case that SMM is X64 while PEI is IA32.
    Status |= MAX_BIT;
  }

  DEBUG ((DEBUG_INFO, "SmmLockBoxPeiLib RestoreLockBox - Exit (%r)\n", Status));

  //
  // Done
  //
  return Status;
}

/**
  This function will restore confidential information from all lockbox which have RestoreInPlace attribute.

  @retval RETURN_SUCCESS            the information is restored successfully.
  @retval RETURN_NOT_STARTED        it is too early to invoke this interface
  @retval RETURN_UNSUPPORTED        the service is not supported by implementaion.
**/
RETURN_STATUS
EFIAPI
RestoreAllLockBoxInPlace (
  VOID
  )
{
  EFI_STATUS                                       Status;
  EFI_PEI_SMM_COMMUNICATION_PPI                    *SmmCommunicationPpi;
  EFI_SMM_LOCK_BOX_PARAMETER_RESTORE_ALL_IN_PLACE  *LockBoxParameterRestoreAllInPlace;
  EFI_SMM_COMMUNICATE_HEADER                       *CommHeader;
  UINT8                                            CommBuffer[sizeof (EFI_GUID) + sizeof (UINT64) + sizeof (EFI_SMM_LOCK_BOX_PARAMETER_RESTORE_ALL_IN_PLACE)];
  UINTN                                            CommSize;
  UINT64                                           MessageLength;

  //
  // Please aware that there is UINTN in EFI_SMM_COMMUNICATE_HEADER. It might be UINT64 in DXE, while it is UINT32 in PEI.
  // typedef struct {
  //   EFI_GUID  HeaderGuid;
  //   UINTN     MessageLength;
  //   UINT8     Data[1];
  // } EFI_SMM_COMMUNICATE_HEADER;
  //

  DEBUG ((DEBUG_INFO, "SmmLockBoxPeiLib RestoreAllLockBoxInPlace - Enter\n"));

  //
  // Get needed resource
  //
  Status = PeiServicesLocatePpi (
             &gEfiPeiSmmCommunicationPpiGuid,
             0,
             NULL,
             (VOID **)&SmmCommunicationPpi
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "SmmLockBoxPeiLib LocatePpi - (%r)\n", Status));
    Status = InternalRestoreAllLockBoxInPlaceFromSmram ();
    DEBUG ((DEBUG_INFO, "SmmLockBoxPeiLib RestoreAllLockBoxInPlace - Exit (%r)\n", Status));
    return Status;
  }

  //
  // Prepare parameter
  //
  CommHeader = (EFI_SMM_COMMUNICATE_HEADER *)&CommBuffer[0];
  CopyMem (&CommHeader->HeaderGuid, &gEfiSmmLockBoxCommunicationGuid, sizeof (gEfiSmmLockBoxCommunicationGuid));
  if ((sizeof (UINTN) == sizeof (UINT32)) && (FeaturePcdGet (PcdDxeIplSwitchToLongMode))) {
    MessageLength = sizeof (*LockBoxParameterRestoreAllInPlace);
    CopyMem (&CommBuffer[OFFSET_OF (EFI_SMM_COMMUNICATE_HEADER, MessageLength)], &MessageLength, sizeof (MessageLength));
  } else {
    CommHeader->MessageLength = sizeof (*LockBoxParameterRestoreAllInPlace);
  }

  if ((sizeof (UINTN) == sizeof (UINT32)) && (FeaturePcdGet (PcdDxeIplSwitchToLongMode))) {
    LockBoxParameterRestoreAllInPlace = (EFI_SMM_LOCK_BOX_PARAMETER_RESTORE_ALL_IN_PLACE *)&CommBuffer[OFFSET_OF (EFI_SMM_COMMUNICATE_HEADER, MessageLength) + sizeof (UINT64)];
  } else {
    LockBoxParameterRestoreAllInPlace = (EFI_SMM_LOCK_BOX_PARAMETER_RESTORE_ALL_IN_PLACE *)&CommBuffer[OFFSET_OF (EFI_SMM_COMMUNICATE_HEADER, MessageLength) + sizeof (UINTN)];
  }

  LockBoxParameterRestoreAllInPlace->Header.Command      = EFI_SMM_LOCK_BOX_COMMAND_RESTORE_ALL_IN_PLACE;
  LockBoxParameterRestoreAllInPlace->Header.DataLength   = sizeof (*LockBoxParameterRestoreAllInPlace);
  LockBoxParameterRestoreAllInPlace->Header.ReturnStatus = (UINT64)-1;

  //
  // Send command
  //
  CommSize = sizeof (CommBuffer);
  Status   = SmmCommunicationPpi->Communicate (
                                    SmmCommunicationPpi,
                                    &CommBuffer[0],
                                    &CommSize
                                    );
  if (Status == EFI_NOT_STARTED) {
    //
    // Pei SMM communication not ready yet, so we access SMRAM directly
    //
    DEBUG ((DEBUG_INFO, "SmmLockBoxPeiLib Communicate - (%r)\n", Status));
    Status                                                 = InternalRestoreAllLockBoxInPlaceFromSmram ();
    LockBoxParameterRestoreAllInPlace->Header.ReturnStatus = (UINT64)Status;
  }

  ASSERT_EFI_ERROR (Status);

  Status = (EFI_STATUS)LockBoxParameterRestoreAllInPlace->Header.ReturnStatus;
  if (Status != EFI_SUCCESS) {
    // Need or MAX_BIT, because there might be case that SMM is X64 while PEI is IA32.
    Status |= MAX_BIT;
  }

  DEBUG ((DEBUG_INFO, "SmmLockBoxPeiLib RestoreAllLockBoxInPlace - Exit (%r)\n", Status));

  //
  // Done
  //
  return Status;
}
