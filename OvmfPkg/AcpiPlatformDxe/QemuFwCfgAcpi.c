/** @file
  OVMF ACPI support using QEMU's fw-cfg interface

  Copyright (c) 2008 - 2014, Intel Corporation. All rights reserved.<BR>
  Copyright (C) 2012-2014, Red Hat, Inc.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <IndustryStandard/Acpi.h>            // EFI_ACPI_DESCRIPTION_HEADER
#include <IndustryStandard/QemuLoader.h>      // QEMU_LOADER_FNAME_SIZE
#include <Library/BaseLib.h>                  // AsciiStrCmp()
#include <Library/BaseMemoryLib.h>            // CopyMem()
#include <Library/DebugLib.h>                 // DEBUG()
#include <Library/MemoryAllocationLib.h>      // AllocatePool()
#include <Library/OrderedCollectionLib.h>     // OrderedCollectionMin()
#include <Library/QemuFwCfgLib.h>             // QemuFwCfgFindFile()
#include <Library/QemuFwCfgS3Lib.h>           // QemuFwCfgS3Enabled()
#include <Library/UefiBootServicesTableLib.h> // gBS

#include "AcpiPlatform.h"

//
// The user structure for the ordered collection that will track the fw_cfg
// blobs under processing.
//
typedef struct {
  UINT8      File[QEMU_LOADER_FNAME_SIZE]; // NUL-terminated name of the fw_cfg
                                           // blob. This is the ordering / search
                                           // key.
  UINTN      Size;                         // The number of bytes in this blob.
  UINT8      *Base;                        // Pointer to the blob data.
  BOOLEAN    HostsOnlyTableData;           // TRUE iff the blob has been found to
                                           // only contain data that is directly
                                           // part of ACPI tables.
} BLOB;

/**
  Compare a standalone key against a user structure containing an embedded key.

  @param[in] StandaloneKey  Pointer to the bare key.

  @param[in] UserStruct     Pointer to the user structure with the embedded
                            key.

  @retval <0  If StandaloneKey compares less than UserStruct's key.

  @retval  0  If StandaloneKey compares equal to UserStruct's key.

  @retval >0  If StandaloneKey compares greater than UserStruct's key.
**/
STATIC
INTN
EFIAPI
BlobKeyCompare (
  IN CONST VOID  *StandaloneKey,
  IN CONST VOID  *UserStruct
  )
{
  CONST BLOB  *Blob;

  Blob = UserStruct;
  return AsciiStrCmp (StandaloneKey, (CONST CHAR8 *)Blob->File);
}

/**
  Comparator function for two user structures.

  @param[in] UserStruct1  Pointer to the first user structure.

  @param[in] UserStruct2  Pointer to the second user structure.

  @retval <0  If UserStruct1 compares less than UserStruct2.

  @retval  0  If UserStruct1 compares equal to UserStruct2.

  @retval >0  If UserStruct1 compares greater than UserStruct2.
**/
STATIC
INTN
EFIAPI
BlobCompare (
  IN CONST VOID  *UserStruct1,
  IN CONST VOID  *UserStruct2
  )
{
  CONST BLOB  *Blob1;

  Blob1 = UserStruct1;
  return BlobKeyCompare (Blob1->File, UserStruct2);
}

/**
  Comparator function for two opaque pointers, ordering on (unsigned) pointer
  value itself.
  Can be used as both Key and UserStruct comparator.

  @param[in] Pointer1  First pointer.

  @param[in] Pointer2  Second pointer.

  @retval <0  If Pointer1 compares less than Pointer2.

  @retval  0  If Pointer1 compares equal to Pointer2.

  @retval >0  If Pointer1 compares greater than Pointer2.
**/
STATIC
INTN
EFIAPI
PointerCompare (
  IN CONST VOID  *Pointer1,
  IN CONST VOID  *Pointer2
  )
{
  if (Pointer1 == Pointer2) {
    return 0;
  }

  if ((UINTN)Pointer1 < (UINTN)Pointer2) {
    return -1;
  }

  return 1;
}

/**
  Comparator function for two ASCII strings. Can be used as both Key and
  UserStruct comparator.

  This function exists solely so we can avoid casting &AsciiStrCmp to
  ORDERED_COLLECTION_USER_COMPARE and ORDERED_COLLECTION_KEY_COMPARE.

  @param[in] AsciiString1  Pointer to the first ASCII string.

  @param[in] AsciiString2  Pointer to the second ASCII string.

  @return  The return value of AsciiStrCmp (AsciiString1, AsciiString2).
**/
STATIC
INTN
EFIAPI
AsciiStringCompare (
  IN CONST VOID  *AsciiString1,
  IN CONST VOID  *AsciiString2
  )
{
  return AsciiStrCmp (AsciiString1, AsciiString2);
}

/**
  Release the ORDERED_COLLECTION structure populated by
  CollectAllocationsRestrictedTo32Bit() (below).

  This function may be called by CollectAllocationsRestrictedTo32Bit() itself,
  on the error path.

  @param[in] AllocationsRestrictedTo32Bit  The ORDERED_COLLECTION structure to
                                           release.
**/
STATIC
VOID
ReleaseAllocationsRestrictedTo32Bit (
  IN ORDERED_COLLECTION  *AllocationsRestrictedTo32Bit
  )
{
  ORDERED_COLLECTION_ENTRY  *Entry, *Entry2;

  for (Entry = OrderedCollectionMin (AllocationsRestrictedTo32Bit);
       Entry != NULL;
       Entry = Entry2)
  {
    Entry2 = OrderedCollectionNext (Entry);
    OrderedCollectionDelete (AllocationsRestrictedTo32Bit, Entry, NULL);
  }

  OrderedCollectionUninit (AllocationsRestrictedTo32Bit);
}

/**
  Iterate over the linker/loader script, and collect the names of the fw_cfg
  blobs that are referenced by QEMU_LOADER_ADD_POINTER.PointeeFile fields, such
  that QEMU_LOADER_ADD_POINTER.PointerSize is less than 8. This means that the
  pointee blob's address will have to be patched into a narrower-than-8 byte
  pointer field, hence the pointee blob must not be allocated from 64-bit
  address space.

  @param[out] AllocationsRestrictedTo32Bit  The ORDERED_COLLECTION structure
                                            linking (not copying / owning) such
                                            QEMU_LOADER_ADD_POINTER.PointeeFile
                                            fields that name the blobs
                                            restricted from 64-bit allocation.

  @param[in] LoaderStart                    Points to the first entry in the
                                            linker/loader script.

  @param[in] LoaderEnd                      Points one past the last entry in
                                            the linker/loader script.

  @retval EFI_SUCCESS           AllocationsRestrictedTo32Bit has been
                                populated.

  @retval EFI_OUT_OF_RESOURCES  Memory allocation failed.

  @retval EFI_PROTOCOL_ERROR    Invalid linker/loader script contents.
**/
STATIC
EFI_STATUS
CollectAllocationsRestrictedTo32Bit (
  OUT ORDERED_COLLECTION      **AllocationsRestrictedTo32Bit,
  IN CONST QEMU_LOADER_ENTRY  *LoaderStart,
  IN CONST QEMU_LOADER_ENTRY  *LoaderEnd
  )
{
  ORDERED_COLLECTION       *Collection;
  CONST QEMU_LOADER_ENTRY  *LoaderEntry;
  EFI_STATUS               Status;

  Collection = OrderedCollectionInit (AsciiStringCompare, AsciiStringCompare);
  if (Collection == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  for (LoaderEntry = LoaderStart; LoaderEntry < LoaderEnd; ++LoaderEntry) {
    CONST QEMU_LOADER_ADD_POINTER  *AddPointer;

    if (LoaderEntry->Type != QemuLoaderCmdAddPointer) {
      continue;
    }

    AddPointer = &LoaderEntry->Command.AddPointer;

    if (AddPointer->PointerSize >= 8) {
      continue;
    }

    if (AddPointer->PointeeFile[QEMU_LOADER_FNAME_SIZE - 1] != '\0') {
      DEBUG ((DEBUG_ERROR, "%a: malformed file name\n", __FUNCTION__));
      Status = EFI_PROTOCOL_ERROR;
      goto RollBack;
    }

    Status = OrderedCollectionInsert (
               Collection,
               NULL,                           // Entry
               (VOID *)AddPointer->PointeeFile
               );
    switch (Status) {
      case EFI_SUCCESS:
        DEBUG ((
          DEBUG_VERBOSE,
          "%a: restricting blob \"%a\" from 64-bit allocation\n",
          __FUNCTION__,
          AddPointer->PointeeFile
          ));
        break;
      case EFI_ALREADY_STARTED:
        //
        // The restriction has been recorded already.
        //
        break;
      case EFI_OUT_OF_RESOURCES:
        goto RollBack;
      default:
        ASSERT (FALSE);
    }
  }

  *AllocationsRestrictedTo32Bit = Collection;
  return EFI_SUCCESS;

RollBack:
  ReleaseAllocationsRestrictedTo32Bit (Collection);
  return Status;
}

/**
  Process a QEMU_LOADER_ALLOCATE command.

  @param[in] Allocate                      The QEMU_LOADER_ALLOCATE command to
                                           process.

  @param[in,out] Tracker                   The ORDERED_COLLECTION tracking the
                                           BLOB user structures created thus
                                           far.

  @param[in] AllocationsRestrictedTo32Bit  The ORDERED_COLLECTION populated by
                                           the function
                                           CollectAllocationsRestrictedTo32Bit,
                                           naming the fw_cfg blobs that must
                                           not be allocated from 64-bit address
                                           space.

  @retval EFI_SUCCESS           An area of whole AcpiNVS pages has been
                                allocated for the blob contents, and the
                                contents have been saved. A BLOB object (user
                                structure) has been allocated from pool memory,
                                referencing the blob contents. The BLOB user
                                structure has been linked into Tracker.

  @retval EFI_PROTOCOL_ERROR    Malformed fw_cfg file name has been found in
                                Allocate, or the Allocate command references a
                                file that is already known by Tracker.

  @retval EFI_UNSUPPORTED       Unsupported alignment request has been found in
                                Allocate.

  @retval EFI_OUT_OF_RESOURCES  Pool allocation failed.

  @return                       Error codes from QemuFwCfgFindFile() and
                                gBS->AllocatePages().
**/
STATIC
EFI_STATUS
EFIAPI
ProcessCmdAllocate (
  IN CONST QEMU_LOADER_ALLOCATE  *Allocate,
  IN OUT ORDERED_COLLECTION      *Tracker,
  IN ORDERED_COLLECTION          *AllocationsRestrictedTo32Bit
  )
{
  FIRMWARE_CONFIG_ITEM  FwCfgItem;
  UINTN                 FwCfgSize;
  EFI_STATUS            Status;
  UINTN                 NumPages;
  EFI_PHYSICAL_ADDRESS  Address;
  BLOB                  *Blob;

  if (Allocate->File[QEMU_LOADER_FNAME_SIZE - 1] != '\0') {
    DEBUG ((DEBUG_ERROR, "%a: malformed file name\n", __FUNCTION__));
    return EFI_PROTOCOL_ERROR;
  }

  if (Allocate->Alignment > EFI_PAGE_SIZE) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: unsupported alignment 0x%x\n",
      __FUNCTION__,
      Allocate->Alignment
      ));
    return EFI_UNSUPPORTED;
  }

  Status = QemuFwCfgFindFile ((CHAR8 *)Allocate->File, &FwCfgItem, &FwCfgSize);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: QemuFwCfgFindFile(\"%a\"): %r\n",
      __FUNCTION__,
      Allocate->File,
      Status
      ));
    return Status;
  }

  NumPages = EFI_SIZE_TO_PAGES (FwCfgSize);
  Address  = MAX_UINT64;
  if (OrderedCollectionFind (
        AllocationsRestrictedTo32Bit,
        Allocate->File
        ) != NULL)
  {
    Address = MAX_UINT32;
  }

  Status = gBS->AllocatePages (
                  AllocateMaxAddress,
                  EfiACPIMemoryNVS,
                  NumPages,
                  &Address
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Blob = AllocatePool (sizeof *Blob);
  if (Blob == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto FreePages;
  }

  CopyMem (Blob->File, Allocate->File, QEMU_LOADER_FNAME_SIZE);
  Blob->Size               = FwCfgSize;
  Blob->Base               = (VOID *)(UINTN)Address;
  Blob->HostsOnlyTableData = TRUE;

  Status = OrderedCollectionInsert (Tracker, NULL, Blob);
  if (Status == RETURN_ALREADY_STARTED) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: duplicated file \"%a\"\n",
      __FUNCTION__,
      Allocate->File
      ));
    Status = EFI_PROTOCOL_ERROR;
  }

  if (EFI_ERROR (Status)) {
    goto FreeBlob;
  }

  QemuFwCfgSelectItem (FwCfgItem);
  QemuFwCfgReadBytes (FwCfgSize, Blob->Base);
  ZeroMem (Blob->Base + Blob->Size, EFI_PAGES_TO_SIZE (NumPages) - Blob->Size);

  DEBUG ((
    DEBUG_VERBOSE,
    "%a: File=\"%a\" Alignment=0x%x Zone=%d Size=0x%Lx "
    "Address=0x%Lx\n",
    __FUNCTION__,
    Allocate->File,
    Allocate->Alignment,
    Allocate->Zone,
    (UINT64)Blob->Size,
    (UINT64)(UINTN)Blob->Base
    ));
  return EFI_SUCCESS;

FreeBlob:
  FreePool (Blob);

FreePages:
  gBS->FreePages (Address, NumPages);

  return Status;
}

/**
  Process a QEMU_LOADER_ADD_POINTER command.

  @param[in] AddPointer  The QEMU_LOADER_ADD_POINTER command to process.

  @param[in] Tracker     The ORDERED_COLLECTION tracking the BLOB user
                         structures created thus far.

  @retval EFI_PROTOCOL_ERROR  Malformed fw_cfg file name(s) have been found in
                              AddPointer, or the AddPointer command references
                              a file unknown to Tracker, or the pointer to
                              relocate has invalid location, size, or value, or
                              the relocated pointer value is not representable
                              in the given pointer size.

  @retval EFI_SUCCESS         The pointer field inside the pointer blob has
                              been relocated.
**/
STATIC
EFI_STATUS
EFIAPI
ProcessCmdAddPointer (
  IN CONST QEMU_LOADER_ADD_POINTER  *AddPointer,
  IN CONST ORDERED_COLLECTION       *Tracker
  )
{
  ORDERED_COLLECTION_ENTRY  *TrackerEntry, *TrackerEntry2;
  BLOB                      *Blob, *Blob2;
  UINT8                     *PointerField;
  UINT64                    PointerValue;

  if ((AddPointer->PointerFile[QEMU_LOADER_FNAME_SIZE - 1] != '\0') ||
      (AddPointer->PointeeFile[QEMU_LOADER_FNAME_SIZE - 1] != '\0'))
  {
    DEBUG ((DEBUG_ERROR, "%a: malformed file name\n", __FUNCTION__));
    return EFI_PROTOCOL_ERROR;
  }

  TrackerEntry  = OrderedCollectionFind (Tracker, AddPointer->PointerFile);
  TrackerEntry2 = OrderedCollectionFind (Tracker, AddPointer->PointeeFile);
  if ((TrackerEntry == NULL) || (TrackerEntry2 == NULL)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: invalid blob reference(s) \"%a\" / \"%a\"\n",
      __FUNCTION__,
      AddPointer->PointerFile,
      AddPointer->PointeeFile
      ));
    return EFI_PROTOCOL_ERROR;
  }

  Blob  = OrderedCollectionUserStruct (TrackerEntry);
  Blob2 = OrderedCollectionUserStruct (TrackerEntry2);
  if (((AddPointer->PointerSize != 1) && (AddPointer->PointerSize != 2) &&
       (AddPointer->PointerSize != 4) && (AddPointer->PointerSize != 8)) ||
      (Blob->Size < AddPointer->PointerSize) ||
      (Blob->Size - AddPointer->PointerSize < AddPointer->PointerOffset))
  {
    DEBUG ((
      DEBUG_ERROR,
      "%a: invalid pointer location or size in \"%a\"\n",
      __FUNCTION__,
      AddPointer->PointerFile
      ));
    return EFI_PROTOCOL_ERROR;
  }

  PointerField = Blob->Base + AddPointer->PointerOffset;
  PointerValue = 0;
  CopyMem (&PointerValue, PointerField, AddPointer->PointerSize);
  if (PointerValue >= Blob2->Size) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: invalid pointer value in \"%a\"\n",
      __FUNCTION__,
      AddPointer->PointerFile
      ));
    return EFI_PROTOCOL_ERROR;
  }

  //
  // The memory allocation system ensures that the address of the byte past the
  // last byte of any allocated object is expressible (no wraparound).
  //
  ASSERT ((UINTN)Blob2->Base <= MAX_ADDRESS - Blob2->Size);

  PointerValue += (UINT64)(UINTN)Blob2->Base;
  if ((AddPointer->PointerSize < 8) &&
      (RShiftU64 (PointerValue, AddPointer->PointerSize * 8) != 0))
  {
    DEBUG ((
      DEBUG_ERROR,
      "%a: relocated pointer value unrepresentable in "
      "\"%a\"\n",
      __FUNCTION__,
      AddPointer->PointerFile
      ));
    return EFI_PROTOCOL_ERROR;
  }

  CopyMem (PointerField, &PointerValue, AddPointer->PointerSize);

  DEBUG ((
    DEBUG_VERBOSE,
    "%a: PointerFile=\"%a\" PointeeFile=\"%a\" "
    "PointerOffset=0x%x PointerSize=%d\n",
    __FUNCTION__,
    AddPointer->PointerFile,
    AddPointer->PointeeFile,
    AddPointer->PointerOffset,
    AddPointer->PointerSize
    ));
  return EFI_SUCCESS;
}

/**
  Process a QEMU_LOADER_ADD_CHECKSUM command.

  @param[in] AddChecksum  The QEMU_LOADER_ADD_CHECKSUM command to process.

  @param[in] Tracker      The ORDERED_COLLECTION tracking the BLOB user
                          structures created thus far.

  @retval EFI_PROTOCOL_ERROR  Malformed fw_cfg file name has been found in
                              AddChecksum, or the AddChecksum command
                              references a file unknown to Tracker, or the
                              range to checksum is invalid.

  @retval EFI_SUCCESS         The requested range has been checksummed.
**/
STATIC
EFI_STATUS
EFIAPI
ProcessCmdAddChecksum (
  IN CONST QEMU_LOADER_ADD_CHECKSUM  *AddChecksum,
  IN CONST ORDERED_COLLECTION        *Tracker
  )
{
  ORDERED_COLLECTION_ENTRY  *TrackerEntry;
  BLOB                      *Blob;

  if (AddChecksum->File[QEMU_LOADER_FNAME_SIZE - 1] != '\0') {
    DEBUG ((DEBUG_ERROR, "%a: malformed file name\n", __FUNCTION__));
    return EFI_PROTOCOL_ERROR;
  }

  TrackerEntry = OrderedCollectionFind (Tracker, AddChecksum->File);
  if (TrackerEntry == NULL) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: invalid blob reference \"%a\"\n",
      __FUNCTION__,
      AddChecksum->File
      ));
    return EFI_PROTOCOL_ERROR;
  }

  Blob = OrderedCollectionUserStruct (TrackerEntry);
  if ((Blob->Size <= AddChecksum->ResultOffset) ||
      (Blob->Size < AddChecksum->Length) ||
      (Blob->Size - AddChecksum->Length < AddChecksum->Start))
  {
    DEBUG ((
      DEBUG_ERROR,
      "%a: invalid checksum range in \"%a\"\n",
      __FUNCTION__,
      AddChecksum->File
      ));
    return EFI_PROTOCOL_ERROR;
  }

  Blob->Base[AddChecksum->ResultOffset] = CalculateCheckSum8 (
                                            Blob->Base + AddChecksum->Start,
                                            AddChecksum->Length
                                            );
  DEBUG ((
    DEBUG_VERBOSE,
    "%a: File=\"%a\" ResultOffset=0x%x Start=0x%x "
    "Length=0x%x\n",
    __FUNCTION__,
    AddChecksum->File,
    AddChecksum->ResultOffset,
    AddChecksum->Start,
    AddChecksum->Length
    ));
  return EFI_SUCCESS;
}

/**
  Process a QEMU_LOADER_WRITE_POINTER command.

  @param[in] WritePointer   The QEMU_LOADER_WRITE_POINTER command to process.

  @param[in] Tracker        The ORDERED_COLLECTION tracking the BLOB user
                            structures created thus far.

  @param[in,out] S3Context  The S3_CONTEXT object capturing the fw_cfg actions
                            of successfully processed QEMU_LOADER_WRITE_POINTER
                            commands, to be replayed at S3 resume. S3Context
                            may be NULL if S3 is disabled.

  @retval EFI_PROTOCOL_ERROR  Malformed fw_cfg file name(s) have been found in
                              WritePointer. Or, the WritePointer command
                              references a file unknown to Tracker or the
                              fw_cfg directory. Or, the pointer object to
                              rewrite has invalid location, size, or initial
                              relative value. Or, the pointer value to store
                              does not fit in the given pointer size.

  @retval EFI_SUCCESS         The pointer object inside the writeable fw_cfg
                              file has been written. If S3Context is not NULL,
                              then WritePointer has been condensed into
                              S3Context.

  @return                     Error codes propagated from
                              SaveCondensedWritePointerToS3Context(). The
                              pointer object inside the writeable fw_cfg file
                              has not been written.
**/
STATIC
EFI_STATUS
ProcessCmdWritePointer (
  IN     CONST QEMU_LOADER_WRITE_POINTER  *WritePointer,
  IN     CONST ORDERED_COLLECTION         *Tracker,
  IN OUT       S3_CONTEXT                 *S3Context OPTIONAL
  )
{
  RETURN_STATUS             Status;
  FIRMWARE_CONFIG_ITEM      PointerItem;
  UINTN                     PointerItemSize;
  ORDERED_COLLECTION_ENTRY  *PointeeEntry;
  BLOB                      *PointeeBlob;
  UINT64                    PointerValue;

  if ((WritePointer->PointerFile[QEMU_LOADER_FNAME_SIZE - 1] != '\0') ||
      (WritePointer->PointeeFile[QEMU_LOADER_FNAME_SIZE - 1] != '\0'))
  {
    DEBUG ((DEBUG_ERROR, "%a: malformed file name\n", __FUNCTION__));
    return EFI_PROTOCOL_ERROR;
  }

  Status = QemuFwCfgFindFile (
             (CONST CHAR8 *)WritePointer->PointerFile,
             &PointerItem,
             &PointerItemSize
             );
  PointeeEntry = OrderedCollectionFind (Tracker, WritePointer->PointeeFile);
  if (RETURN_ERROR (Status) || (PointeeEntry == NULL)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: invalid fw_cfg file or blob reference \"%a\" / \"%a\"\n",
      __FUNCTION__,
      WritePointer->PointerFile,
      WritePointer->PointeeFile
      ));
    return EFI_PROTOCOL_ERROR;
  }

  if (((WritePointer->PointerSize != 1) && (WritePointer->PointerSize != 2) &&
       (WritePointer->PointerSize != 4) && (WritePointer->PointerSize != 8)) ||
      (PointerItemSize < WritePointer->PointerSize) ||
      (PointerItemSize - WritePointer->PointerSize <
       WritePointer->PointerOffset))
  {
    DEBUG ((
      DEBUG_ERROR,
      "%a: invalid pointer location or size in \"%a\"\n",
      __FUNCTION__,
      WritePointer->PointerFile
      ));
    return EFI_PROTOCOL_ERROR;
  }

  PointeeBlob  = OrderedCollectionUserStruct (PointeeEntry);
  PointerValue = WritePointer->PointeeOffset;
  if (PointerValue >= PointeeBlob->Size) {
    DEBUG ((DEBUG_ERROR, "%a: invalid PointeeOffset\n", __FUNCTION__));
    return EFI_PROTOCOL_ERROR;
  }

  //
  // The memory allocation system ensures that the address of the byte past the
  // last byte of any allocated object is expressible (no wraparound).
  //
  ASSERT ((UINTN)PointeeBlob->Base <= MAX_ADDRESS - PointeeBlob->Size);

  PointerValue += (UINT64)(UINTN)PointeeBlob->Base;
  if ((WritePointer->PointerSize < 8) &&
      (RShiftU64 (PointerValue, WritePointer->PointerSize * 8) != 0))
  {
    DEBUG ((
      DEBUG_ERROR,
      "%a: pointer value unrepresentable in \"%a\"\n",
      __FUNCTION__,
      WritePointer->PointerFile
      ));
    return EFI_PROTOCOL_ERROR;
  }

  //
  // If S3 is enabled, we have to capture the below fw_cfg actions in condensed
  // form, to be replayed during S3 resume.
  //
  if (S3Context != NULL) {
    EFI_STATUS  SaveStatus;

    SaveStatus = SaveCondensedWritePointerToS3Context (
                   S3Context,
                   (UINT16)PointerItem,
                   WritePointer->PointerSize,
                   WritePointer->PointerOffset,
                   PointerValue
                   );
    if (EFI_ERROR (SaveStatus)) {
      return SaveStatus;
    }
  }

  QemuFwCfgSelectItem (PointerItem);
  QemuFwCfgSkipBytes (WritePointer->PointerOffset);
  QemuFwCfgWriteBytes (WritePointer->PointerSize, &PointerValue);

  //
  // Because QEMU has now learned PointeeBlob->Base, we must mark PointeeBlob
  // as unreleasable, for the case when the whole linker/loader script is
  // handled successfully.
  //
  PointeeBlob->HostsOnlyTableData = FALSE;

  DEBUG ((
    DEBUG_VERBOSE,
    "%a: PointerFile=\"%a\" PointeeFile=\"%a\" "
    "PointerOffset=0x%x PointeeOffset=0x%x PointerSize=%d\n",
    __FUNCTION__,
    WritePointer->PointerFile,
    WritePointer->PointeeFile,
    WritePointer->PointerOffset,
    WritePointer->PointeeOffset,
    WritePointer->PointerSize
    ));
  return EFI_SUCCESS;
}

/**
  Undo a QEMU_LOADER_WRITE_POINTER command.

  This function revokes (zeroes out) a guest memory reference communicated to
  QEMU earlier. The caller is responsible for invoking this function only on
  such QEMU_LOADER_WRITE_POINTER commands that have been successfully processed
  by ProcessCmdWritePointer().

  @param[in] WritePointer  The QEMU_LOADER_WRITE_POINTER command to undo.
**/
STATIC
VOID
UndoCmdWritePointer (
  IN CONST QEMU_LOADER_WRITE_POINTER  *WritePointer
  )
{
  RETURN_STATUS         Status;
  FIRMWARE_CONFIG_ITEM  PointerItem;
  UINTN                 PointerItemSize;
  UINT64                PointerValue;

  Status = QemuFwCfgFindFile (
             (CONST CHAR8 *)WritePointer->PointerFile,
             &PointerItem,
             &PointerItemSize
             );
  ASSERT_RETURN_ERROR (Status);

  PointerValue = 0;
  QemuFwCfgSelectItem (PointerItem);
  QemuFwCfgSkipBytes (WritePointer->PointerOffset);
  QemuFwCfgWriteBytes (WritePointer->PointerSize, &PointerValue);

  DEBUG ((
    DEBUG_VERBOSE,
    "%a: PointerFile=\"%a\" PointerOffset=0x%x PointerSize=%d\n",
    __FUNCTION__,
    WritePointer->PointerFile,
    WritePointer->PointerOffset,
    WritePointer->PointerSize
    ));
}

//
// We'll be saving the keys of installed tables so that we can roll them back
// in case of failure. 128 tables should be enough for anyone (TM).
//
#define INSTALLED_TABLES_MAX  128

/**
  Process a QEMU_LOADER_ADD_POINTER command in order to see if its target byte
  array is an ACPI table, and if so, install it.

  This function assumes that the entire QEMU linker/loader command file has
  been processed successfully in a prior first pass.

  @param[in] AddPointer        The QEMU_LOADER_ADD_POINTER command to process.

  @param[in] Tracker           The ORDERED_COLLECTION tracking the BLOB user
                               structures.

  @param[in] AcpiProtocol      The ACPI table protocol used to install tables.

  @param[in,out] InstalledKey  On input, an array of INSTALLED_TABLES_MAX UINTN
                               elements, allocated by the caller. On output,
                               the function will have stored (appended) the
                               AcpiProtocol-internal key of the ACPI table that
                               the function has installed, if the AddPointer
                               command identified an ACPI table that is
                               different from RSDT and XSDT.

  @param[in,out] NumInstalled  On input, the number of entries already used in
                               InstalledKey; it must be in [0,
                               INSTALLED_TABLES_MAX] inclusive. On output, the
                               parameter is incremented if the AddPointer
                               command identified an ACPI table that is
                               different from RSDT and XSDT.

  @param[in,out] SeenPointers  The ORDERED_COLLECTION tracking the absolute
                               target addresses that have been pointed-to by
                               QEMU_LOADER_ADD_POINTER commands thus far. If a
                               target address is encountered for the first
                               time, and it identifies an ACPI table that is
                               different from RDST and XSDT, the table is
                               installed. If a target address is seen for the
                               second or later times, it is skipped without
                               taking any action.

  @retval EFI_INVALID_PARAMETER  NumInstalled was outside the allowed range on
                                 input.

  @retval EFI_OUT_OF_RESOURCES   The AddPointer command identified an ACPI
                                 table different from RSDT and XSDT, but there
                                 was no more room in InstalledKey.

  @retval EFI_SUCCESS            AddPointer has been processed. Either its
                                 absolute target address has been encountered
                                 before, or an ACPI table different from RSDT
                                 and XSDT has been installed (reflected by
                                 InstalledKey and NumInstalled), or RSDT or
                                 XSDT has been identified but not installed, or
                                 the fw_cfg blob pointed-into by AddPointer has
                                 been marked as hosting something else than
                                 just direct ACPI table contents.

  @return                        Error codes returned by
                                 AcpiProtocol->InstallAcpiTable().
**/
STATIC
EFI_STATUS
EFIAPI
Process2ndPassCmdAddPointer (
  IN     CONST QEMU_LOADER_ADD_POINTER  *AddPointer,
  IN     CONST ORDERED_COLLECTION       *Tracker,
  IN     EFI_ACPI_TABLE_PROTOCOL        *AcpiProtocol,
  IN OUT UINTN                          InstalledKey[INSTALLED_TABLES_MAX],
  IN OUT INT32                          *NumInstalled,
  IN OUT ORDERED_COLLECTION             *SeenPointers
  )
{
  CONST ORDERED_COLLECTION_ENTRY                      *TrackerEntry;
  CONST ORDERED_COLLECTION_ENTRY                      *TrackerEntry2;
  ORDERED_COLLECTION_ENTRY                            *SeenPointerEntry;
  CONST BLOB                                          *Blob;
  BLOB                                                *Blob2;
  CONST UINT8                                         *PointerField;
  UINT64                                              PointerValue;
  UINTN                                               Blob2Remaining;
  UINTN                                               TableSize;
  CONST EFI_ACPI_1_0_FIRMWARE_ACPI_CONTROL_STRUCTURE  *Facs;
  CONST EFI_ACPI_DESCRIPTION_HEADER                   *Header;
  EFI_STATUS                                          Status;

  if ((*NumInstalled < 0) || (*NumInstalled > INSTALLED_TABLES_MAX)) {
    return EFI_INVALID_PARAMETER;
  }

  TrackerEntry  = OrderedCollectionFind (Tracker, AddPointer->PointerFile);
  TrackerEntry2 = OrderedCollectionFind (Tracker, AddPointer->PointeeFile);
  Blob          = OrderedCollectionUserStruct (TrackerEntry);
  Blob2         = OrderedCollectionUserStruct (TrackerEntry2);
  PointerField  = Blob->Base + AddPointer->PointerOffset;
  PointerValue  = 0;
  CopyMem (&PointerValue, PointerField, AddPointer->PointerSize);

  //
  // We assert that PointerValue falls inside Blob2's contents. This is ensured
  // by the Blob2->Size check and later checks in ProcessCmdAddPointer().
  //
  Blob2Remaining = (UINTN)Blob2->Base;
  ASSERT (PointerValue >= Blob2Remaining);
  Blob2Remaining += Blob2->Size;
  ASSERT (PointerValue < Blob2Remaining);

  Status = OrderedCollectionInsert (
             SeenPointers,
             &SeenPointerEntry, // for reverting insertion in error case
             (VOID *)(UINTN)PointerValue
             );
  if (EFI_ERROR (Status)) {
    if (Status == RETURN_ALREADY_STARTED) {
      //
      // Already seen this pointer, don't try to process it again.
      //
      DEBUG ((
        DEBUG_VERBOSE,
        "%a: PointerValue=0x%Lx already processed, skipping.\n",
        __FUNCTION__,
        PointerValue
        ));
      Status = EFI_SUCCESS;
    }

    return Status;
  }

  Blob2Remaining -= (UINTN)PointerValue;
  DEBUG ((
    DEBUG_VERBOSE,
    "%a: checking for ACPI header in \"%a\" at 0x%Lx "
    "(remaining: 0x%Lx): ",
    __FUNCTION__,
    AddPointer->PointeeFile,
    PointerValue,
    (UINT64)Blob2Remaining
    ));

  TableSize = 0;

  //
  // To make our job simple, the FACS has a custom header. Sigh.
  //
  if (sizeof *Facs <= Blob2Remaining) {
    Facs = (EFI_ACPI_1_0_FIRMWARE_ACPI_CONTROL_STRUCTURE *)(UINTN)PointerValue;

    if ((Facs->Length >= sizeof *Facs) &&
        (Facs->Length <= Blob2Remaining) &&
        (Facs->Signature ==
         EFI_ACPI_1_0_FIRMWARE_ACPI_CONTROL_STRUCTURE_SIGNATURE))
    {
      DEBUG ((
        DEBUG_VERBOSE,
        "found \"%-4.4a\" size 0x%x\n",
        (CONST CHAR8 *)&Facs->Signature,
        Facs->Length
        ));
      TableSize = Facs->Length;
    }
  }

  //
  // check for the uniform tables
  //
  if ((TableSize == 0) && (sizeof *Header <= Blob2Remaining)) {
    Header = (EFI_ACPI_DESCRIPTION_HEADER *)(UINTN)PointerValue;

    if ((Header->Length >= sizeof *Header) &&
        (Header->Length <= Blob2Remaining) &&
        (CalculateSum8 ((CONST UINT8 *)Header, Header->Length) == 0))
    {
      //
      // This looks very much like an ACPI table from QEMU:
      // - Length field consistent with both ACPI and containing blob size
      // - checksum is correct
      //
      DEBUG ((
        DEBUG_VERBOSE,
        "found \"%-4.4a\" size 0x%x\n",
        (CONST CHAR8 *)&Header->Signature,
        Header->Length
        ));
      TableSize = Header->Length;

      //
      // Skip RSDT and XSDT because those are handled by
      // EFI_ACPI_TABLE_PROTOCOL automatically.
      if ((Header->Signature ==
           EFI_ACPI_1_0_ROOT_SYSTEM_DESCRIPTION_TABLE_SIGNATURE) ||
          (Header->Signature ==
           EFI_ACPI_2_0_EXTENDED_SYSTEM_DESCRIPTION_TABLE_SIGNATURE))
      {
        return EFI_SUCCESS;
      }
    }
  }

  if (TableSize == 0) {
    DEBUG ((DEBUG_VERBOSE, "not found; marking fw_cfg blob as opaque\n"));
    Blob2->HostsOnlyTableData = FALSE;
    return EFI_SUCCESS;
  }

  if (*NumInstalled == INSTALLED_TABLES_MAX) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: can't install more than %d tables\n",
      __FUNCTION__,
      INSTALLED_TABLES_MAX
      ));
    Status = EFI_OUT_OF_RESOURCES;
    goto RollbackSeenPointer;
  }

  Status = AcpiProtocol->InstallAcpiTable (
                           AcpiProtocol,
                           (VOID *)(UINTN)PointerValue,
                           TableSize,
                           &InstalledKey[*NumInstalled]
                           );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: InstallAcpiTable(): %r\n",
      __FUNCTION__,
      Status
      ));
    goto RollbackSeenPointer;
  }

  ++*NumInstalled;
  return EFI_SUCCESS;

RollbackSeenPointer:
  OrderedCollectionDelete (SeenPointers, SeenPointerEntry, NULL);
  return Status;
}

/**
  Download, process, and install ACPI table data from the QEMU loader
  interface.

  @param[in] AcpiProtocol  The ACPI table protocol used to install tables.

  @retval  EFI_UNSUPPORTED       Firmware configuration is unavailable, or QEMU
                                 loader command with unsupported parameters
                                 has been found.

  @retval  EFI_NOT_FOUND         The host doesn't export the required fw_cfg
                                 files.

  @retval  EFI_OUT_OF_RESOURCES  Memory allocation failed, or more than
                                 INSTALLED_TABLES_MAX tables found.

  @retval  EFI_PROTOCOL_ERROR    Found invalid fw_cfg contents.

  @return                        Status codes returned by
                                 AcpiProtocol->InstallAcpiTable().

**/
EFI_STATUS
EFIAPI
InstallQemuFwCfgTables (
  IN   EFI_ACPI_TABLE_PROTOCOL  *AcpiProtocol
  )
{
  EFI_STATUS                Status;
  FIRMWARE_CONFIG_ITEM      FwCfgItem;
  UINTN                     FwCfgSize;
  QEMU_LOADER_ENTRY         *LoaderStart;
  CONST QEMU_LOADER_ENTRY   *LoaderEntry, *LoaderEnd;
  CONST QEMU_LOADER_ENTRY   *WritePointerSubsetEnd;
  ORIGINAL_ATTRIBUTES       *OriginalPciAttributes;
  UINTN                     OriginalPciAttributesCount;
  ORDERED_COLLECTION        *AllocationsRestrictedTo32Bit;
  S3_CONTEXT                *S3Context;
  ORDERED_COLLECTION        *Tracker;
  UINTN                     *InstalledKey;
  INT32                     Installed;
  ORDERED_COLLECTION_ENTRY  *TrackerEntry, *TrackerEntry2;
  ORDERED_COLLECTION        *SeenPointers;
  ORDERED_COLLECTION_ENTRY  *SeenPointerEntry, *SeenPointerEntry2;

  Status = QemuFwCfgFindFile ("etc/table-loader", &FwCfgItem, &FwCfgSize);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (FwCfgSize % sizeof *LoaderEntry != 0) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: \"etc/table-loader\" has invalid size 0x%Lx\n",
      __FUNCTION__,
      (UINT64)FwCfgSize
      ));
    return EFI_PROTOCOL_ERROR;
  }

  LoaderStart = AllocatePool (FwCfgSize);
  if (LoaderStart == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  EnablePciDecoding (&OriginalPciAttributes, &OriginalPciAttributesCount);
  QemuFwCfgSelectItem (FwCfgItem);
  QemuFwCfgReadBytes (FwCfgSize, LoaderStart);
  RestorePciDecoding (OriginalPciAttributes, OriginalPciAttributesCount);
  LoaderEnd = LoaderStart + FwCfgSize / sizeof *LoaderEntry;

  AllocationsRestrictedTo32Bit = NULL;
  Status                       = CollectAllocationsRestrictedTo32Bit (
                                   &AllocationsRestrictedTo32Bit,
                                   LoaderStart,
                                   LoaderEnd
                                   );
  if (EFI_ERROR (Status)) {
    goto FreeLoader;
  }

  S3Context = NULL;
  if (QemuFwCfgS3Enabled ()) {
    //
    // Size the allocation pessimistically, assuming that all commands in the
    // script are QEMU_LOADER_WRITE_POINTER commands.
    //
    Status = AllocateS3Context (&S3Context, LoaderEnd - LoaderStart);
    if (EFI_ERROR (Status)) {
      goto FreeAllocationsRestrictedTo32Bit;
    }
  }

  Tracker = OrderedCollectionInit (BlobCompare, BlobKeyCompare);
  if (Tracker == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto FreeS3Context;
  }

  //
  // first pass: process the commands
  //
  // "WritePointerSubsetEnd" points one past the last successful
  // QEMU_LOADER_WRITE_POINTER command. Now when we're about to start the first
  // pass, no such command has been encountered yet.
  //
  WritePointerSubsetEnd = LoaderStart;
  for (LoaderEntry = LoaderStart; LoaderEntry < LoaderEnd; ++LoaderEntry) {
    switch (LoaderEntry->Type) {
      case QemuLoaderCmdAllocate:
        Status = ProcessCmdAllocate (
                   &LoaderEntry->Command.Allocate,
                   Tracker,
                   AllocationsRestrictedTo32Bit
                   );
        break;

      case QemuLoaderCmdAddPointer:
        Status = ProcessCmdAddPointer (
                   &LoaderEntry->Command.AddPointer,
                   Tracker
                   );
        break;

      case QemuLoaderCmdAddChecksum:
        Status = ProcessCmdAddChecksum (
                   &LoaderEntry->Command.AddChecksum,
                   Tracker
                   );
        break;

      case QemuLoaderCmdWritePointer:
        Status = ProcessCmdWritePointer (
                   &LoaderEntry->Command.WritePointer,
                   Tracker,
                   S3Context
                   );
        if (!EFI_ERROR (Status)) {
          WritePointerSubsetEnd = LoaderEntry + 1;
        }

        break;

      default:
        DEBUG ((
          DEBUG_VERBOSE,
          "%a: unknown loader command: 0x%x\n",
          __FUNCTION__,
          LoaderEntry->Type
          ));
        break;
    }

    if (EFI_ERROR (Status)) {
      goto RollbackWritePointersAndFreeTracker;
    }
  }

  InstalledKey = AllocatePool (INSTALLED_TABLES_MAX * sizeof *InstalledKey);
  if (InstalledKey == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto RollbackWritePointersAndFreeTracker;
  }

  SeenPointers = OrderedCollectionInit (PointerCompare, PointerCompare);
  if (SeenPointers == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto FreeKeys;
  }

  //
  // second pass: identify and install ACPI tables
  //
  Installed = 0;
  for (LoaderEntry = LoaderStart; LoaderEntry < LoaderEnd; ++LoaderEntry) {
    if (LoaderEntry->Type == QemuLoaderCmdAddPointer) {
      Status = Process2ndPassCmdAddPointer (
                 &LoaderEntry->Command.AddPointer,
                 Tracker,
                 AcpiProtocol,
                 InstalledKey,
                 &Installed,
                 SeenPointers
                 );
      if (EFI_ERROR (Status)) {
        goto UninstallAcpiTables;
      }
    }
  }

  //
  // Translating the condensed QEMU_LOADER_WRITE_POINTER commands to ACPI S3
  // Boot Script opcodes has to be the last operation in this function, because
  // if it succeeds, it cannot be undone.
  //
  if (S3Context != NULL) {
    Status = TransferS3ContextToBootScript (S3Context);
    if (EFI_ERROR (Status)) {
      goto UninstallAcpiTables;
    }

    //
    // Ownership of S3Context has been transferred.
    //
    S3Context = NULL;
  }

UninstallAcpiTables:
  if (EFI_ERROR (Status)) {
    //
    // roll back partial installation
    //
    while (Installed > 0) {
      --Installed;
      AcpiProtocol->UninstallAcpiTable (AcpiProtocol, InstalledKey[Installed]);
    }
  } else {
    DEBUG ((DEBUG_INFO, "%a: installed %d tables\n", __FUNCTION__, Installed));
  }

  for (SeenPointerEntry = OrderedCollectionMin (SeenPointers);
       SeenPointerEntry != NULL;
       SeenPointerEntry = SeenPointerEntry2)
  {
    SeenPointerEntry2 = OrderedCollectionNext (SeenPointerEntry);
    OrderedCollectionDelete (SeenPointers, SeenPointerEntry, NULL);
  }

  OrderedCollectionUninit (SeenPointers);

FreeKeys:
  FreePool (InstalledKey);

RollbackWritePointersAndFreeTracker:
  //
  // In case of failure, revoke any allocation addresses that were communicated
  // to QEMU previously, before we release all the blobs.
  //
  if (EFI_ERROR (Status)) {
    LoaderEntry = WritePointerSubsetEnd;
    while (LoaderEntry > LoaderStart) {
      --LoaderEntry;
      if (LoaderEntry->Type == QemuLoaderCmdWritePointer) {
        UndoCmdWritePointer (&LoaderEntry->Command.WritePointer);
      }
    }
  }

  //
  // Tear down the tracker infrastructure. Each fw_cfg blob will be left in
  // place only if we're exiting with success and the blob hosts data that is
  // not directly part of some ACPI table.
  //
  for (TrackerEntry = OrderedCollectionMin (Tracker); TrackerEntry != NULL;
       TrackerEntry = TrackerEntry2)
  {
    VOID  *UserStruct;
    BLOB  *Blob;

    TrackerEntry2 = OrderedCollectionNext (TrackerEntry);
    OrderedCollectionDelete (Tracker, TrackerEntry, &UserStruct);
    Blob = UserStruct;

    if (EFI_ERROR (Status) || Blob->HostsOnlyTableData) {
      DEBUG ((
        DEBUG_VERBOSE,
        "%a: freeing \"%a\"\n",
        __FUNCTION__,
        Blob->File
        ));
      gBS->FreePages ((UINTN)Blob->Base, EFI_SIZE_TO_PAGES (Blob->Size));
    }

    FreePool (Blob);
  }

  OrderedCollectionUninit (Tracker);

FreeS3Context:
  if (S3Context != NULL) {
    ReleaseS3Context (S3Context);
  }

FreeAllocationsRestrictedTo32Bit:
  ReleaseAllocationsRestrictedTo32Bit (AllocationsRestrictedTo32Bit);

FreeLoader:
  FreePool (LoaderStart);

  return Status;
}
