/** @file

  Library implementing the LockBox interface for OVMF

  Copyright (C) 2013, Red Hat, Inc.
  Copyright (c) 2010 - 2019, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/LockBoxLib.h>
#include <Library/PcdLib.h>
#include <LockBoxLib.h>

#pragma pack(1)
typedef struct {
  EFI_GUID             Guid;
  EFI_PHYSICAL_ADDRESS OrigAddress;
  EFI_PHYSICAL_ADDRESS CopyAddress;
  UINT32               Size;
  UINT64               Attributes;
} LOCK_BOX_ENTRY;
#pragma pack()

LOCK_BOX_GLOBAL *mLockBoxGlobal = NULL;
STATIC LOCK_BOX_ENTRY *StartOfEntries = NULL;
STATIC LOCK_BOX_ENTRY *EndOfEntries = NULL;

RETURN_STATUS
EFIAPI
LockBoxLibInitialize (
  VOID
  )
{
  UINTN NumEntries;

  ASSERT (!FeaturePcdGet (PcdSmmSmramRequire));

  if (PcdGet32 (PcdOvmfLockBoxStorageSize) < sizeof (LOCK_BOX_GLOBAL)) {
    return RETURN_UNSUPPORTED;
  }

  mLockBoxGlobal = (LOCK_BOX_GLOBAL *)(UINTN) PcdGet32 (PcdOvmfLockBoxStorageBase);
  StartOfEntries = ((LOCK_BOX_ENTRY *) (mLockBoxGlobal + 1));
  NumEntries = ((PcdGet32 (PcdOvmfLockBoxStorageSize) - sizeof (LOCK_BOX_GLOBAL)) /
                sizeof (LOCK_BOX_ENTRY));
  EndOfEntries = StartOfEntries + NumEntries;
  if (mLockBoxGlobal->Signature != LOCK_BOX_GLOBAL_SIGNATURE) {
    //
    // Note: This code depends on the lock box being cleared in early
    // PEI before usage, so the SubPageBuffer and SubPageRemaining
    // fields don't need to be set to 0.
    //
    mLockBoxGlobal->Signature = LOCK_BOX_GLOBAL_SIGNATURE;
  }
  return RETURN_SUCCESS;
}


/**
  Find LockBox entry based on GUID.

  @param[in] Guid  The GUID to search for.

  @return  Address of the LOCK_BOX_ENTRY found.

           If NULL, then the item was not found, and there is no space
           left to store a new item.

           If non-NULL and LOCK_BOX_ENTRY.Size == 0, then the item was not
           found, but a new item can be inserted at the returned location.

           If non-NULL and LOCK_BOX_ENTRY.Size > 0, then the item was found.
**/
STATIC
LOCK_BOX_ENTRY *
EFIAPI
FindHeaderByGuid (
  IN CONST EFI_GUID *Guid
  )
{
  LOCK_BOX_ENTRY *Header;

  for (Header = StartOfEntries; Header < EndOfEntries; Header++) {
    if (Header->Size == 0 || CompareGuid (Guid, &Header->Guid)) {
      return Header;
    }
  }

  return NULL;
}


/**
  This function will save confidential information to lockbox.

  @param Guid       the guid to identify the confidential information
  @param Buffer     the address of the confidential information
  @param Length     the length of the confidential information

  @retval RETURN_SUCCESS            the information is saved successfully.
  @retval RETURN_INVALID_PARAMETER  the Guid is NULL, or Buffer is NULL, or
                                    Length is 0
  @retval RETURN_ALREADY_STARTED    the requested GUID already exist.
  @retval RETURN_OUT_OF_RESOURCES   no enough resource to save the information.
  @retval RETURN_ACCESS_DENIED      it is too late to invoke this interface
  @retval RETURN_NOT_STARTED        it is too early to invoke this interface
  @retval RETURN_UNSUPPORTED        the service is not supported by
                                    implementaion.
**/
RETURN_STATUS
EFIAPI
SaveLockBox (
  IN  GUID                        *Guid,
  IN  VOID                        *Buffer,
  IN  UINTN                       Length
  )
{
  LOCK_BOX_ENTRY *Header;
  VOID            *CopyBuffer;

  DEBUG ((DEBUG_VERBOSE, "%a: Guid=%g Buffer=%p Length=0x%x\n", __FUNCTION__,
    Guid, Buffer, (UINT32) Length));

  if (Guid == NULL || Buffer == NULL || Length == 0) {
    return RETURN_INVALID_PARAMETER;
  }

  if (Length > 0xFFFFFFFF) {
    return RETURN_OUT_OF_RESOURCES;
  }

  Header = FindHeaderByGuid (Guid);
  if (Header == NULL) {
    return RETURN_OUT_OF_RESOURCES;
  }

  if (Header->Size > 0) {
    return RETURN_ALREADY_STARTED;
  }

  CopyBuffer = AllocateAcpiNvsPool (Length);
  if (CopyBuffer == NULL) {
    return RETURN_OUT_OF_RESOURCES;
  }

  //
  // overwrite the current terminator header with new metadata
  //
  CopyGuid (&Header->Guid, Guid);
  Header->OrigAddress = (UINTN) Buffer;
  Header->CopyAddress = (UINTN) CopyBuffer;
  Header->Size        = (UINT32) Length;
  Header->Attributes  = 0;

  //
  // copy contents
  //
  CopyMem (CopyBuffer, Buffer, Length);

  return RETURN_SUCCESS;
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
  @retval RETURN_UNSUPPORTED        the service is not supported by
                                    implementaion.
**/
RETURN_STATUS
EFIAPI
SetLockBoxAttributes (
  IN  GUID                        *Guid,
  IN  UINT64                      Attributes
  )
{
  LOCK_BOX_ENTRY *Header;

  DEBUG ((DEBUG_VERBOSE, "%a: Guid=%g Attributes=0x%Lx\n", __FUNCTION__, Guid,
    Attributes));

  if (Guid == NULL) {
    return RETURN_INVALID_PARAMETER;
  }

  Header = FindHeaderByGuid (Guid);
  if (!Header || Header->Size == 0) {
    return RETURN_NOT_FOUND;
  }
  Header->Attributes = Attributes;

  return RETURN_SUCCESS;
}


/**
  This function will update confidential information to lockbox.

  @param Guid   the guid to identify the original confidential information
  @param Offset the offset of the original confidential information
  @param Buffer the address of the updated confidential information
  @param Length the length of the updated confidential information

  @retval RETURN_SUCCESS            the information is saved successfully.
  @retval RETURN_INVALID_PARAMETER  the Guid is NULL, or Buffer is NULL, or
                                    Length is 0.
  @retval RETURN_NOT_FOUND          the requested GUID not found.
  @retval RETURN_BUFFER_TOO_SMALL   for lockbox without attribute
                                    LOCK_BOX_ATTRIBUTE_RESTORE_IN_S3_ONLY, the
                                    original buffer to too small to hold new
                                    information.
  @retval RETURN_OUT_OF_RESOURCES   for lockbox with attribute
                                    LOCK_BOX_ATTRIBUTE_RESTORE_IN_S3_ONLY, no
                                    enough resource to save the information.
  @retval RETURN_ACCESS_DENIED      it is too late to invoke this interface
  @retval RETURN_NOT_STARTED        it is too early to invoke this interface
  @retval RETURN_UNSUPPORTED        the service is not supported by
                                    implementaion.
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
  LOCK_BOX_ENTRY *Header;

  DEBUG ((DEBUG_VERBOSE, "%a: Guid=%g Offset=0x%x Length=0x%x\n", __FUNCTION__,
    Guid, (UINT32) Offset, (UINT32) Length));

  if (Guid == NULL || Buffer == NULL || Length == 0) {
    return RETURN_INVALID_PARAMETER;
  }

  Header = FindHeaderByGuid (Guid);
  if (!Header || Header->Size == 0) {
    return RETURN_NOT_FOUND;
  }

  if (Header->Size < Offset ||
      Length > Header->Size - Offset) {
    return RETURN_BUFFER_TOO_SMALL;
  }

  CopyMem ((UINT8 *)(UINTN) (Header->CopyAddress) + Offset, Buffer, Length);

  return RETURN_SUCCESS;
}


/**
  This function will restore confidential information from lockbox.

  @param Guid   the guid to identify the confidential information
  @param Buffer the address of the restored confidential information
                NULL means restored to original address, Length MUST be NULL at
                same time.
  @param Length the length of the restored confidential information

  @retval RETURN_SUCCESS            the information is restored successfully.
  @retval RETURN_INVALID_PARAMETER  the Guid is NULL, or one of Buffer and
                                    Length is NULL.
  @retval RETURN_WRITE_PROTECTED    Buffer and Length are NULL, but the LockBox
                                    has no LOCK_BOX_ATTRIBUTE_RESTORE_IN_PLACE
                                    attribute.
  @retval RETURN_BUFFER_TOO_SMALL   the Length is too small to hold the
                                    confidential information.
  @retval RETURN_NOT_FOUND          the requested GUID not found.
  @retval RETURN_NOT_STARTED        it is too early to invoke this interface
  @retval RETURN_ACCESS_DENIED      not allow to restore to the address
  @retval RETURN_UNSUPPORTED        the service is not supported by
                                    implementaion.
**/
RETURN_STATUS
EFIAPI
RestoreLockBox (
  IN  GUID                        *Guid,
  IN  VOID                        *Buffer, OPTIONAL
  IN  OUT UINTN                   *Length  OPTIONAL
  )
{
  LOCK_BOX_ENTRY *Header;

  DEBUG ((DEBUG_VERBOSE, "%a: Guid=%g Buffer=%p\n", __FUNCTION__, Guid,
    Buffer));

  if ((Guid == NULL) ||
      ((Buffer == NULL) && (Length != NULL)) ||
      ((Buffer != NULL) && (Length == NULL))) {
    return EFI_INVALID_PARAMETER;
  }

  Header = FindHeaderByGuid (Guid);
  if (!Header || Header->Size == 0) {
    return RETURN_NOT_FOUND;
  }

  if (Buffer == NULL) {
    if (!(Header->Attributes & LOCK_BOX_ATTRIBUTE_RESTORE_IN_PLACE)) {
      return RETURN_WRITE_PROTECTED;
    }
    if (Header->OrigAddress + (Header->Size - 1) > MAX_ADDRESS) {
      return RETURN_UNSUPPORTED;
    }
    Buffer = (VOID *)(UINTN) Header->OrigAddress;
  }

  //
  // Set RestoreLength
  //
  if (Length != NULL) {
    if (Header->Size > *Length) {
      //
      // Input buffer is too small to hold all data.
      //
      *Length = Header->Size;
      return EFI_BUFFER_TOO_SMALL;
    }
    *Length = Header->Size;
  }

  CopyMem (Buffer, (VOID*)(UINTN) Header->CopyAddress, Header->Size);

  return RETURN_SUCCESS;
}


/**
  This function will restore confidential information from all lockbox which
  have RestoreInPlace attribute.

  @retval RETURN_SUCCESS            the information is restored successfully.
  @retval RETURN_NOT_STARTED        it is too early to invoke this interface
  @retval RETURN_UNSUPPORTED        the service is not supported by
                                    implementaion.
**/
RETURN_STATUS
EFIAPI
RestoreAllLockBoxInPlace (
  VOID
  )
{
  LOCK_BOX_ENTRY *Header;

  for (Header = StartOfEntries;
       Header < EndOfEntries && Header->Size > 0;
       Header++) {
    if (Header->Attributes & LOCK_BOX_ATTRIBUTE_RESTORE_IN_PLACE) {
      VOID *Buffer;

      if (Header->OrigAddress + (Header->Size - 1) > MAX_ADDRESS) {
        return RETURN_UNSUPPORTED;
      }
      Buffer = (VOID *)(UINTN) Header->OrigAddress;
      CopyMem (Buffer, (VOID*)(UINTN)Header->CopyAddress, Header->Size);
      DEBUG ((DEBUG_VERBOSE, "%a: Guid=%g Buffer=%p\n", __FUNCTION__,
        &Header->Guid, Buffer));
    }
  }
  return RETURN_SUCCESS;
}
