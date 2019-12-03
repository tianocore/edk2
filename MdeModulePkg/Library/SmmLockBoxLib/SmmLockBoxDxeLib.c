/** @file

Copyright (c) 2010 - 2019, Intel Corporation. All rights reserved.<BR>

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiDxe.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/LockBoxLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiLib.h>
#include <Protocol/SmmCommunication.h>
#include <Guid/SmmLockBox.h>
#include <Guid/PiSmmCommunicationRegionTable.h>

#include "SmmLockBoxLibPrivate.h"

EFI_SMM_COMMUNICATION_PROTOCOL  *mLockBoxSmmCommProtocol = NULL;
UINT8                           *mLockBoxSmmCommBuffer   = NULL;

/**
  Get smm communication protocol for lockbox.

  @return Pointer to smm communication protocol, NULL if not found.

**/
EFI_SMM_COMMUNICATION_PROTOCOL *
LockBoxGetSmmCommProtocol (
  VOID
  )
{
  EFI_STATUS    Status;

  //
  // If the protocol has been got previously, return it.
  //
  if (mLockBoxSmmCommProtocol != NULL) {
    return mLockBoxSmmCommProtocol;
  }

  Status = gBS->LocateProtocol (
                  &gEfiSmmCommunicationProtocolGuid,
                  NULL,
                  (VOID **)&mLockBoxSmmCommProtocol
                  );
  if (EFI_ERROR (Status)) {
    mLockBoxSmmCommProtocol = NULL;
  }
  return mLockBoxSmmCommProtocol;
}

/**
  Get smm communication buffer for lockbox.

  @return Pointer to smm communication buffer, NULL if not found.

**/
UINT8 *
LockBoxGetSmmCommBuffer (
  VOID
  )
{
  EFI_STATUS                                Status;
  UINTN                                     MinimalSizeNeeded;
  EDKII_PI_SMM_COMMUNICATION_REGION_TABLE   *PiSmmCommunicationRegionTable;
  UINT32                                    Index;
  EFI_MEMORY_DESCRIPTOR                     *Entry;
  UINTN                                     Size;

  //
  // If the buffer has been got previously, return it.
  //
  if (mLockBoxSmmCommBuffer != NULL) {
    return mLockBoxSmmCommBuffer;
  }

  MinimalSizeNeeded = sizeof (EFI_GUID) +
                      sizeof (UINTN) +
                      MAX (sizeof (EFI_SMM_LOCK_BOX_PARAMETER_SAVE),
                           MAX (sizeof (EFI_SMM_LOCK_BOX_PARAMETER_SET_ATTRIBUTES),
                                MAX (sizeof (EFI_SMM_LOCK_BOX_PARAMETER_UPDATE),
                                     MAX (sizeof (EFI_SMM_LOCK_BOX_PARAMETER_RESTORE),
                                          sizeof (EFI_SMM_LOCK_BOX_PARAMETER_RESTORE_ALL_IN_PLACE)))));

  Status = EfiGetSystemConfigurationTable (
             &gEdkiiPiSmmCommunicationRegionTableGuid,
             (VOID **) &PiSmmCommunicationRegionTable
             );
  if (EFI_ERROR (Status)) {
    mLockBoxSmmCommBuffer = NULL;
    return mLockBoxSmmCommBuffer;
  }
  ASSERT (PiSmmCommunicationRegionTable != NULL);
  Entry = (EFI_MEMORY_DESCRIPTOR *) (PiSmmCommunicationRegionTable + 1);
  Size = 0;
  for (Index = 0; Index < PiSmmCommunicationRegionTable->NumberOfEntries; Index++) {
    if (Entry->Type == EfiConventionalMemory) {
      Size = EFI_PAGES_TO_SIZE ((UINTN) Entry->NumberOfPages);
      if (Size >= MinimalSizeNeeded) {
        break;
      }
    }
    Entry = (EFI_MEMORY_DESCRIPTOR *) ((UINT8 *) Entry + PiSmmCommunicationRegionTable->DescriptorSize);
  }
  if (Index >= PiSmmCommunicationRegionTable->NumberOfEntries) {
    mLockBoxSmmCommBuffer = NULL;
  } else {
    mLockBoxSmmCommBuffer = (UINT8 *) (UINTN) Entry->PhysicalStart;
  }
  return mLockBoxSmmCommBuffer;
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
  IN  GUID                        *Guid,
  IN  VOID                        *Buffer,
  IN  UINTN                       Length
  )
{
  EFI_STATUS                      Status;
  EFI_SMM_COMMUNICATION_PROTOCOL  *SmmCommunication;
  EFI_SMM_LOCK_BOX_PARAMETER_SAVE *LockBoxParameterSave;
  EFI_SMM_COMMUNICATE_HEADER      *CommHeader;
  UINT8                           TempCommBuffer[sizeof(EFI_GUID) + sizeof(UINTN) + sizeof(EFI_SMM_LOCK_BOX_PARAMETER_SAVE)];
  UINT8                           *CommBuffer;
  UINTN                           CommSize;

  DEBUG ((DEBUG_INFO, "SmmLockBoxDxeLib SaveLockBox - Enter\n"));

  //
  // Basic check
  //
  if ((Guid == NULL) || (Buffer == NULL) || (Length == 0)) {
    return EFI_INVALID_PARAMETER;
  }

  SmmCommunication = LockBoxGetSmmCommProtocol ();
  if (SmmCommunication == NULL) {
    return EFI_NOT_STARTED;
  }

  //
  // Prepare parameter
  //
  CommBuffer = LockBoxGetSmmCommBuffer ();
  if (CommBuffer == NULL) {
    CommBuffer = &TempCommBuffer[0];
  }
  CommHeader = (EFI_SMM_COMMUNICATE_HEADER *)&CommBuffer[0];
  CopyMem (&CommHeader->HeaderGuid, &gEfiSmmLockBoxCommunicationGuid, sizeof(gEfiSmmLockBoxCommunicationGuid));
  CommHeader->MessageLength = sizeof(*LockBoxParameterSave);

  LockBoxParameterSave = (EFI_SMM_LOCK_BOX_PARAMETER_SAVE *)&CommBuffer[OFFSET_OF (EFI_SMM_COMMUNICATE_HEADER, Data)];
  LockBoxParameterSave->Header.Command    = EFI_SMM_LOCK_BOX_COMMAND_SAVE;
  LockBoxParameterSave->Header.DataLength = sizeof(*LockBoxParameterSave);
  LockBoxParameterSave->Header.ReturnStatus = (UINT64)-1;
  CopyMem (&LockBoxParameterSave->Guid, Guid, sizeof(*Guid));
  LockBoxParameterSave->Buffer     = (EFI_PHYSICAL_ADDRESS)(UINTN)Buffer;
  LockBoxParameterSave->Length     = (UINT64)Length;

  //
  // Send command
  //
  CommSize = sizeof(EFI_GUID) + sizeof(UINTN) + sizeof(EFI_SMM_LOCK_BOX_PARAMETER_SAVE);
  Status = SmmCommunication->Communicate (
                               SmmCommunication,
                               &CommBuffer[0],
                               &CommSize
                               );
  ASSERT_EFI_ERROR (Status);

  Status = (EFI_STATUS)LockBoxParameterSave->Header.ReturnStatus;

  DEBUG ((DEBUG_INFO, "SmmLockBoxDxeLib SaveLockBox - Exit (%r)\n", Status));

  //
  // Done
  //
  return Status;
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
  IN  GUID                        *Guid,
  IN  UINT64                      Attributes
  )
{
  EFI_STATUS                                Status;
  EFI_SMM_COMMUNICATION_PROTOCOL            *SmmCommunication;
  EFI_SMM_LOCK_BOX_PARAMETER_SET_ATTRIBUTES *LockBoxParameterSetAttributes;
  EFI_SMM_COMMUNICATE_HEADER                *CommHeader;
  UINT8                                     TempCommBuffer[sizeof(EFI_GUID) + sizeof(UINTN) + sizeof(EFI_SMM_LOCK_BOX_PARAMETER_SET_ATTRIBUTES)];
  UINT8                                     *CommBuffer;
  UINTN                                     CommSize;

  DEBUG ((DEBUG_INFO, "SmmLockBoxDxeLib SetLockBoxAttributes - Enter\n"));

  //
  // Basic check
  //
  if ((Guid == NULL) ||
      ((Attributes & ~(LOCK_BOX_ATTRIBUTE_RESTORE_IN_PLACE | LOCK_BOX_ATTRIBUTE_RESTORE_IN_S3_ONLY)) != 0)) {
    return EFI_INVALID_PARAMETER;
  }

  SmmCommunication = LockBoxGetSmmCommProtocol ();
  if (SmmCommunication == NULL) {
    return EFI_NOT_STARTED;
  }

  //
  // Prepare parameter
  //
  CommBuffer = LockBoxGetSmmCommBuffer ();
  if (CommBuffer == NULL) {
    CommBuffer = &TempCommBuffer[0];
  }
  CommHeader = (EFI_SMM_COMMUNICATE_HEADER *)&CommBuffer[0];
  CopyMem (&CommHeader->HeaderGuid, &gEfiSmmLockBoxCommunicationGuid, sizeof(gEfiSmmLockBoxCommunicationGuid));
  CommHeader->MessageLength = sizeof(*LockBoxParameterSetAttributes);

  LockBoxParameterSetAttributes = (EFI_SMM_LOCK_BOX_PARAMETER_SET_ATTRIBUTES *)&CommBuffer[OFFSET_OF (EFI_SMM_COMMUNICATE_HEADER, Data)];
  LockBoxParameterSetAttributes->Header.Command    = EFI_SMM_LOCK_BOX_COMMAND_SET_ATTRIBUTES;
  LockBoxParameterSetAttributes->Header.DataLength = sizeof(*LockBoxParameterSetAttributes);
  LockBoxParameterSetAttributes->Header.ReturnStatus = (UINT64)-1;
  CopyMem (&LockBoxParameterSetAttributes->Guid, Guid, sizeof(*Guid));
  LockBoxParameterSetAttributes->Attributes = (UINT64)Attributes;

  //
  // Send command
  //
  CommSize = sizeof(EFI_GUID) + sizeof(UINTN) + sizeof(EFI_SMM_LOCK_BOX_PARAMETER_SET_ATTRIBUTES);
  Status = SmmCommunication->Communicate (
                               SmmCommunication,
                               &CommBuffer[0],
                               &CommSize
                               );
  ASSERT_EFI_ERROR (Status);

  Status = (EFI_STATUS)LockBoxParameterSetAttributes->Header.ReturnStatus;

  DEBUG ((DEBUG_INFO, "SmmLockBoxDxeLib SetLockBoxAttributes - Exit (%r)\n", Status));

  //
  // Done
  //
  return Status;
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
  IN  GUID                        *Guid,
  IN  UINTN                       Offset,
  IN  VOID                        *Buffer,
  IN  UINTN                       Length
  )
{
  EFI_STATUS                        Status;
  EFI_SMM_COMMUNICATION_PROTOCOL    *SmmCommunication;
  EFI_SMM_LOCK_BOX_PARAMETER_UPDATE *LockBoxParameterUpdate;
  EFI_SMM_COMMUNICATE_HEADER        *CommHeader;
  UINT8                             TempCommBuffer[sizeof(EFI_GUID) + sizeof(UINTN) + sizeof(EFI_SMM_LOCK_BOX_PARAMETER_UPDATE)];
  UINT8                             *CommBuffer;
  UINTN                             CommSize;

  DEBUG ((DEBUG_INFO, "SmmLockBoxDxeLib UpdateLockBox - Enter\n"));

  //
  // Basic check
  //
  if ((Guid == NULL) || (Buffer == NULL) || (Length == 0)) {
    return EFI_INVALID_PARAMETER;
  }

  SmmCommunication = LockBoxGetSmmCommProtocol ();
  if (SmmCommunication == NULL) {
    return EFI_NOT_STARTED;
  }

  //
  // Prepare parameter
  //
  CommBuffer = LockBoxGetSmmCommBuffer ();
  if (CommBuffer == NULL) {
    CommBuffer = &TempCommBuffer[0];
  }
  CommHeader = (EFI_SMM_COMMUNICATE_HEADER *)&CommBuffer[0];
  CopyMem (&CommHeader->HeaderGuid, &gEfiSmmLockBoxCommunicationGuid, sizeof(gEfiSmmLockBoxCommunicationGuid));
  CommHeader->MessageLength = sizeof(*LockBoxParameterUpdate);

  LockBoxParameterUpdate = (EFI_SMM_LOCK_BOX_PARAMETER_UPDATE *)(UINTN)&CommBuffer[OFFSET_OF (EFI_SMM_COMMUNICATE_HEADER, Data)];
  LockBoxParameterUpdate->Header.Command    = EFI_SMM_LOCK_BOX_COMMAND_UPDATE;
  LockBoxParameterUpdate->Header.DataLength = sizeof(*LockBoxParameterUpdate);
  LockBoxParameterUpdate->Header.ReturnStatus = (UINT64)-1;
  CopyMem (&LockBoxParameterUpdate->Guid, Guid, sizeof(*Guid));
  LockBoxParameterUpdate->Offset = (UINT64)Offset;
  LockBoxParameterUpdate->Buffer = (EFI_PHYSICAL_ADDRESS)(UINTN)Buffer;
  LockBoxParameterUpdate->Length = (UINT64)Length;

  //
  // Send command
  //
  CommSize = sizeof(EFI_GUID) + sizeof(UINTN) + sizeof(EFI_SMM_LOCK_BOX_PARAMETER_UPDATE);
  Status = SmmCommunication->Communicate (
                               SmmCommunication,
                               &CommBuffer[0],
                               &CommSize
                               );
  ASSERT_EFI_ERROR (Status);

  Status = (EFI_STATUS)LockBoxParameterUpdate->Header.ReturnStatus;

  DEBUG ((DEBUG_INFO, "SmmLockBoxDxeLib UpdateLockBox - Exit (%r)\n", Status));

  //
  // Done
  //
  return Status;
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
  IN  GUID                        *Guid,
  IN  VOID                        *Buffer, OPTIONAL
  IN  OUT UINTN                   *Length  OPTIONAL
  )
{
  EFI_STATUS                         Status;
  EFI_SMM_COMMUNICATION_PROTOCOL     *SmmCommunication;
  EFI_SMM_LOCK_BOX_PARAMETER_RESTORE *LockBoxParameterRestore;
  EFI_SMM_COMMUNICATE_HEADER         *CommHeader;
  UINT8                              TempCommBuffer[sizeof(EFI_GUID) + sizeof(UINTN) + sizeof(EFI_SMM_LOCK_BOX_PARAMETER_RESTORE)];
  UINT8                              *CommBuffer;
  UINTN                              CommSize;

  DEBUG ((DEBUG_INFO, "SmmLockBoxDxeLib RestoreLockBox - Enter\n"));

  //
  // Basic check
  //
  if ((Guid == NULL) ||
      ((Buffer == NULL) && (Length != NULL)) ||
      ((Buffer != NULL) && (Length == NULL))) {
    return EFI_INVALID_PARAMETER;
  }

  SmmCommunication = LockBoxGetSmmCommProtocol ();
  if (SmmCommunication == NULL) {
    return EFI_NOT_STARTED;
  }

  //
  // Prepare parameter
  //
  CommBuffer = LockBoxGetSmmCommBuffer ();
  if (CommBuffer == NULL) {
    CommBuffer = &TempCommBuffer[0];
  }
  CommHeader = (EFI_SMM_COMMUNICATE_HEADER *)&CommBuffer[0];
  CopyMem (&CommHeader->HeaderGuid, &gEfiSmmLockBoxCommunicationGuid, sizeof(gEfiSmmLockBoxCommunicationGuid));
  CommHeader->MessageLength = sizeof(*LockBoxParameterRestore);

  LockBoxParameterRestore = (EFI_SMM_LOCK_BOX_PARAMETER_RESTORE *)&CommBuffer[OFFSET_OF (EFI_SMM_COMMUNICATE_HEADER, Data)];
  LockBoxParameterRestore->Header.Command    = EFI_SMM_LOCK_BOX_COMMAND_RESTORE;
  LockBoxParameterRestore->Header.DataLength = sizeof(*LockBoxParameterRestore);
  LockBoxParameterRestore->Header.ReturnStatus = (UINT64)-1;
  CopyMem (&LockBoxParameterRestore->Guid, Guid, sizeof(*Guid));
  LockBoxParameterRestore->Buffer = (EFI_PHYSICAL_ADDRESS)(UINTN)Buffer;
  if (Length != NULL) {
    LockBoxParameterRestore->Length = (EFI_PHYSICAL_ADDRESS)*Length;
  } else {
    LockBoxParameterRestore->Length = 0;
  }

  //
  // Send command
  //
  CommSize = sizeof(EFI_GUID) + sizeof(UINTN) + sizeof(EFI_SMM_LOCK_BOX_PARAMETER_RESTORE);
  Status = SmmCommunication->Communicate (
                               SmmCommunication,
                               &CommBuffer[0],
                               &CommSize
                               );
  ASSERT_EFI_ERROR (Status);

  if (Length != NULL) {
    *Length = (UINTN)LockBoxParameterRestore->Length;
  }

  Status = (EFI_STATUS)LockBoxParameterRestore->Header.ReturnStatus;

  DEBUG ((DEBUG_INFO, "SmmLockBoxDxeLib RestoreLockBox - Exit (%r)\n", Status));

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
  EFI_STATUS                                      Status;
  EFI_SMM_COMMUNICATION_PROTOCOL                  *SmmCommunication;
  EFI_SMM_LOCK_BOX_PARAMETER_RESTORE_ALL_IN_PLACE *LockBoxParameterRestoreAllInPlace;
  EFI_SMM_COMMUNICATE_HEADER                      *CommHeader;
  UINT8                                           TempCommBuffer[sizeof(EFI_GUID) + sizeof(UINTN) + sizeof(EFI_SMM_LOCK_BOX_PARAMETER_RESTORE_ALL_IN_PLACE)];
  UINT8                                           *CommBuffer;
  UINTN                                           CommSize;

  DEBUG ((DEBUG_INFO, "SmmLockBoxDxeLib RestoreAllLockBoxInPlace - Enter\n"));

  SmmCommunication = LockBoxGetSmmCommProtocol ();
  if (SmmCommunication == NULL) {
    return EFI_NOT_STARTED;
  }

  //
  // Prepare parameter
  //
  CommBuffer = LockBoxGetSmmCommBuffer ();
  if (CommBuffer == NULL) {
    CommBuffer = &TempCommBuffer[0];
  }
  CommHeader = (EFI_SMM_COMMUNICATE_HEADER *)&CommBuffer[0];
  CopyMem (&CommHeader->HeaderGuid, &gEfiSmmLockBoxCommunicationGuid, sizeof(gEfiSmmLockBoxCommunicationGuid));
  CommHeader->MessageLength = sizeof(*LockBoxParameterRestoreAllInPlace);

  LockBoxParameterRestoreAllInPlace = (EFI_SMM_LOCK_BOX_PARAMETER_RESTORE_ALL_IN_PLACE *)&CommBuffer[OFFSET_OF (EFI_SMM_COMMUNICATE_HEADER, Data)];
  LockBoxParameterRestoreAllInPlace->Header.Command    = EFI_SMM_LOCK_BOX_COMMAND_RESTORE_ALL_IN_PLACE;
  LockBoxParameterRestoreAllInPlace->Header.DataLength = sizeof(*LockBoxParameterRestoreAllInPlace);
  LockBoxParameterRestoreAllInPlace->Header.ReturnStatus = (UINT64)-1;

  //
  // Send command
  //
  CommSize = sizeof(EFI_GUID) + sizeof(UINTN) + sizeof(EFI_SMM_LOCK_BOX_PARAMETER_RESTORE_ALL_IN_PLACE);
  Status = SmmCommunication->Communicate (
                               SmmCommunication,
                               &CommBuffer[0],
                               &CommSize
                               );
  ASSERT_EFI_ERROR (Status);

  Status = (EFI_STATUS)LockBoxParameterRestoreAllInPlace->Header.ReturnStatus;

  DEBUG ((DEBUG_INFO, "SmmLockBoxDxeLib RestoreAllLockBoxInPlace - Exit (%r)\n", Status));

  //
  // Done
  //
  return Status;
}

