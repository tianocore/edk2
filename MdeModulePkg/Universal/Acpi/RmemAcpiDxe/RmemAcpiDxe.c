/** @file  // [CODE_FIRST] 13253
  Publishes the Reserved-Memory Reporting (RMEM) ACPI table.  // [CODE_FIRST] 13253
// [CODE_FIRST] 13253
  Copyright (c) Microsoft Corporation.  // [CODE_FIRST] 13253
  SPDX-License-Identifier: BSD-2-Clause-Patent  // [CODE_FIRST] 13253
**/  // [CODE_FIRST] 13253
// [CODE_FIRST] 13253
#include <PiDxe.h>  // [CODE_FIRST] 13253
// [CODE_FIRST] 13253
#include <Pi/PiHob.h>  // [CODE_FIRST] 13253
// [CODE_FIRST] 13253
#include <Guid/EventGroup.h>  // [CODE_FIRST] 13253
#include <Guid/ReservedMemoryReportingHob.h>  // [CODE_FIRST] 13253
#include <Guid/ReservedMemoryReportingTable.h>  // [CODE_FIRST] 13253
#include <Library/BaseLib.h>  // [CODE_FIRST] 13253
#include <Library/BaseMemoryLib.h>  // [CODE_FIRST] 13253
#include <Library/DebugLib.h>  // [CODE_FIRST] 13253
#include <Library/HobLib.h>  // [CODE_FIRST] 13253
#include <Library/MemoryAllocationLib.h>  // [CODE_FIRST] 13253
#include <Library/PcdLib.h>  // [CODE_FIRST] 13253
#include <Library/UefiBootServicesTableLib.h>  // [CODE_FIRST] 13253
#include <Protocol/AcpiTable.h>  // [CODE_FIRST] 13253
#include <Protocol/ReservedMemoryReporting.h>  // [CODE_FIRST] 13253
// [CODE_FIRST] 13253
#define RMEM_MAX_ENTRIES  64  // [CODE_FIRST] 13253
// [CODE_FIRST] 13253
STATIC_ASSERT (RMEM_MAX_ENTRIES <= MAX_UINT16, "RMEM entry count does not fit in the table header");  // [CODE_FIRST] 13253
// [CODE_FIRST] 13253
STATIC RMEM_ENTRY  mEntries[RMEM_MAX_ENTRIES];  // [CODE_FIRST] 13253
STATIC UINT16      mEntryCount;  // [CODE_FIRST] 13253
STATIC BOOLEAN     mFinalized;  // [CODE_FIRST] 13253
STATIC EFI_EVENT   mPublicationEvent;  // [CODE_FIRST] 13253
STATIC UINT64      mMaximumPhysicalAddress = MAX_UINT64;  // [CODE_FIRST] 13253
// [CODE_FIRST] 13253
/**  // [CODE_FIRST] 13253
  Sets the maximum physical address from the CPU HOB.  // [CODE_FIRST] 13253
**/  // [CODE_FIRST] 13253
STATIC  // [CODE_FIRST] 13253
EFI_STATUS  // [CODE_FIRST] 13253
RmemInitializeMaximumPhysicalAddress (  // [CODE_FIRST] 13253
  VOID  // [CODE_FIRST] 13253
  )  // [CODE_FIRST] 13253
{  // [CODE_FIRST] 13253
  EFI_HOB_CPU  *CpuHob;  // [CODE_FIRST] 13253
  UINT8        PhysicalAddressBits;  // [CODE_FIRST] 13253
// [CODE_FIRST] 13253
  CpuHob = (EFI_HOB_CPU *)GetFirstHob (EFI_HOB_TYPE_CPU);  // [CODE_FIRST] 13253
  if (CpuHob == NULL) {  // [CODE_FIRST] 13253
    DEBUG ((DEBUG_ERROR, "RMEM: CPU HOB not found\n"));  // [CODE_FIRST] 13253
    return EFI_NOT_FOUND;  // [CODE_FIRST] 13253
  }  // [CODE_FIRST] 13253
// [CODE_FIRST] 13253
  PhysicalAddressBits = CpuHob->SizeOfMemorySpace;  // [CODE_FIRST] 13253
  if ((PhysicalAddressBits == 0) || (PhysicalAddressBits > 64)) {  // [CODE_FIRST] 13253
    DEBUG ((DEBUG_ERROR, "RMEM: CPU HOB contains invalid physical-address width %u\n", PhysicalAddressBits));  // [CODE_FIRST] 13253
    ASSERT ((PhysicalAddressBits > 0) && (PhysicalAddressBits <= 64));  // [CODE_FIRST] 13253
    return EFI_COMPROMISED_DATA;  // [CODE_FIRST] 13253
  }  // [CODE_FIRST] 13253
// [CODE_FIRST] 13253
  if (PhysicalAddressBits < 64) {  // [CODE_FIRST] 13253
    mMaximumPhysicalAddress = LShiftU64 (1, PhysicalAddressBits) - 1;  // [CODE_FIRST] 13253
  } else {  // [CODE_FIRST] 13253
    mMaximumPhysicalAddress = MAX_UINT64;  // [CODE_FIRST] 13253
  }  // [CODE_FIRST] 13253
// [CODE_FIRST] 13253
  return EFI_SUCCESS;  // [CODE_FIRST] 13253
}  // [CODE_FIRST] 13253
// [CODE_FIRST] 13253
/**  // [CODE_FIRST] 13253
  Checks that a range is page-aligned, nonempty, and contained within the  // [CODE_FIRST] 13253
  platform physical address space.  // [CODE_FIRST] 13253
**/  // [CODE_FIRST] 13253
STATIC  // [CODE_FIRST] 13253
BOOLEAN  // [CODE_FIRST] 13253
RmemRangeIsValid (  // [CODE_FIRST] 13253
  IN EFI_PHYSICAL_ADDRESS  Base,  // [CODE_FIRST] 13253
  IN UINT64                Size  // [CODE_FIRST] 13253
  )  // [CODE_FIRST] 13253
{  // [CODE_FIRST] 13253
  if ((Size == 0) ||  // [CODE_FIRST] 13253
      ((Base & EFI_PAGE_MASK) != 0) ||  // [CODE_FIRST] 13253
      ((Size & EFI_PAGE_MASK) != 0) ||  // [CODE_FIRST] 13253
      ((Size - 1) > mMaximumPhysicalAddress))  // [CODE_FIRST] 13253
  {  // [CODE_FIRST] 13253
    return FALSE;  // [CODE_FIRST] 13253
  }  // [CODE_FIRST] 13253
// [CODE_FIRST] 13253
  return Base <= (mMaximumPhysicalAddress - (Size - 1));  // [CODE_FIRST] 13253
}  // [CODE_FIRST] 13253
// [CODE_FIRST] 13253
/**  // [CODE_FIRST] 13253
  Checks whether two valid ranges share at least one byte. Ranges that only  // [CODE_FIRST] 13253
  touch at adjacent endpoints do not overlap.  // [CODE_FIRST] 13253
// [CODE_FIRST] 13253
  Both ranges must first pass RmemRangeIsValid() so the inclusive end-address  // [CODE_FIRST] 13253
  calculations cannot overflow.  // [CODE_FIRST] 13253
**/  // [CODE_FIRST] 13253
STATIC  // [CODE_FIRST] 13253
BOOLEAN  // [CODE_FIRST] 13253
RmemRangesOverlap (  // [CODE_FIRST] 13253
  IN EFI_PHYSICAL_ADDRESS  FirstBase,  // [CODE_FIRST] 13253
  IN UINT64                FirstSize,  // [CODE_FIRST] 13253
  IN EFI_PHYSICAL_ADDRESS  SecondBase,  // [CODE_FIRST] 13253
  IN UINT64                SecondSize  // [CODE_FIRST] 13253
  )  // [CODE_FIRST] 13253
{  // [CODE_FIRST] 13253
  return (FirstBase <= (SecondBase + SecondSize - 1)) &&  // [CODE_FIRST] 13253
         (SecondBase <= (FirstBase + FirstSize - 1));  // [CODE_FIRST] 13253
}  // [CODE_FIRST] 13253
// [CODE_FIRST] 13253
STATIC  // [CODE_FIRST] 13253
EFI_STATUS  // [CODE_FIRST] 13253
EFIAPI  // [CODE_FIRST] 13253
RmemAddReservedRange (  // [CODE_FIRST] 13253
  IN EDKII_RMEM_REGISTRATION_PROTOCOL  *This,  // [CODE_FIRST] 13253
  IN EFI_PHYSICAL_ADDRESS              Base,  // [CODE_FIRST] 13253
  IN UINT64                            Size,  // [CODE_FIRST] 13253
  IN RMEM_CATEGORY                     Category,  // [CODE_FIRST] 13253
  IN UINT16                            Flags,  // [CODE_FIRST] 13253
  IN CONST CHAR8                       *Label OPTIONAL  // [CODE_FIRST] 13253
  )  // [CODE_FIRST] 13253
{  // [CODE_FIRST] 13253
  CONST CHAR8  *EffectiveLabel;  // [CODE_FIRST] 13253
  UINTN        LabelLength;  // [CODE_FIRST] 13253
  UINT32       Index;  // [CODE_FIRST] 13253
// [CODE_FIRST] 13253
  if (This == NULL) {  // [CODE_FIRST] 13253
    DEBUG ((DEBUG_ERROR, "RMEM: Registration protocol pointer is NULL\n"));  // [CODE_FIRST] 13253
    return EFI_INVALID_PARAMETER;  // [CODE_FIRST] 13253
  }  // [CODE_FIRST] 13253
// [CODE_FIRST] 13253
  if (This->Revision != EDKII_RMEM_REGISTRATION_PROTOCOL_REVISION) {  // [CODE_FIRST] 13253
    DEBUG ((  // [CODE_FIRST] 13253
      DEBUG_ERROR,  // [CODE_FIRST] 13253
      "RMEM: Unsupported registration protocol revision %u\n",  // [CODE_FIRST] 13253
      This->Revision  // [CODE_FIRST] 13253
      ));  // [CODE_FIRST] 13253
    return EFI_INVALID_PARAMETER;  // [CODE_FIRST] 13253
  }  // [CODE_FIRST] 13253
// [CODE_FIRST] 13253
  if (mFinalized) {  // [CODE_FIRST] 13253
    DEBUG ((DEBUG_WARN, "RMEM: Registration attempted after table finalization\n"));  // [CODE_FIRST] 13253
    return EFI_ACCESS_DENIED;  // [CODE_FIRST] 13253
  }  // [CODE_FIRST] 13253
// [CODE_FIRST] 13253
  if ((Flags & ~RMEM_ENTRY_FLAG_VALID_MASK) != 0) {  // [CODE_FIRST] 13253
    DEBUG ((DEBUG_ERROR, "RMEM: Unsupported flags 0x%04x\n", Flags));  // [CODE_FIRST] 13253
    return EFI_INVALID_PARAMETER;  // [CODE_FIRST] 13253
  }  // [CODE_FIRST] 13253
// [CODE_FIRST] 13253
  if (!RmemRangeIsValid (Base, Size)) {  // [CODE_FIRST] 13253
    if ((Flags & RMEM_ENTRY_FLAG_ADDRESS_HIDDEN) != 0) {  // [CODE_FIRST] 13253
      DEBUG ((  // [CODE_FIRST] 13253
        DEBUG_ERROR,  // [CODE_FIRST] 13253
        "RMEM: Invalid hidden range size=0x%lx maximum=0x%lx\n",  // [CODE_FIRST] 13253
        Size,  // [CODE_FIRST] 13253
        mMaximumPhysicalAddress  // [CODE_FIRST] 13253
        ));  // [CODE_FIRST] 13253
    } else {  // [CODE_FIRST] 13253
      DEBUG ((  // [CODE_FIRST] 13253
        DEBUG_ERROR,  // [CODE_FIRST] 13253
        "RMEM: Invalid range base=0x%lx size=0x%lx maximum=0x%lx\n",  // [CODE_FIRST] 13253
        Base,  // [CODE_FIRST] 13253
        Size,  // [CODE_FIRST] 13253
        mMaximumPhysicalAddress  // [CODE_FIRST] 13253
        ));  // [CODE_FIRST] 13253
    }  // [CODE_FIRST] 13253
// [CODE_FIRST] 13253
    return EFI_INVALID_PARAMETER;  // [CODE_FIRST] 13253
  }  // [CODE_FIRST] 13253
// [CODE_FIRST] 13253
  if ((Category == RmemCategoryUnknown) ||  // [CODE_FIRST] 13253
      ((UINT32)Category >= (UINT32)RmemCategoryMax))  // [CODE_FIRST] 13253
  {  // [CODE_FIRST] 13253
    DEBUG ((DEBUG_ERROR, "RMEM: Invalid category %u\n", (UINT32)Category));  // [CODE_FIRST] 13253
    return EFI_INVALID_PARAMETER;  // [CODE_FIRST] 13253
  }  // [CODE_FIRST] 13253
// [CODE_FIRST] 13253
  EffectiveLabel = (Label == NULL) ? "" : Label;  // [CODE_FIRST] 13253
  LabelLength    = AsciiStrnLenS (EffectiveLabel, RMEM_LABEL_MAX_LEN);  // [CODE_FIRST] 13253
  if (LabelLength >= RMEM_LABEL_MAX_LEN) {  // [CODE_FIRST] 13253
    DEBUG ((DEBUG_ERROR, "RMEM: Label exceeds %u bytes including its terminator\n", RMEM_LABEL_MAX_LEN));  // [CODE_FIRST] 13253
    return EFI_BAD_BUFFER_SIZE;  // [CODE_FIRST] 13253
  }  // [CODE_FIRST] 13253
// [CODE_FIRST] 13253
  for (Index = 0; Index < mEntryCount; Index++) {  // [CODE_FIRST] 13253
    if (RmemRangesOverlap (  // [CODE_FIRST] 13253
          mEntries[Index].Base,  // [CODE_FIRST] 13253
          mEntries[Index].Size,  // [CODE_FIRST] 13253
          Base,  // [CODE_FIRST] 13253
          Size  // [CODE_FIRST] 13253
          ))  // [CODE_FIRST] 13253
    {  // [CODE_FIRST] 13253
      if ((Flags & RMEM_ENTRY_FLAG_ADDRESS_HIDDEN) != 0) {  // [CODE_FIRST] 13253
        DEBUG ((  // [CODE_FIRST] 13253
          DEBUG_ERROR,  // [CODE_FIRST] 13253
          "RMEM: Hidden range size=0x%lx overlaps entry %u\n",  // [CODE_FIRST] 13253
          Size,  // [CODE_FIRST] 13253
          Index  // [CODE_FIRST] 13253
          ));  // [CODE_FIRST] 13253
      } else {  // [CODE_FIRST] 13253
        DEBUG ((  // [CODE_FIRST] 13253
          DEBUG_ERROR,  // [CODE_FIRST] 13253
          "RMEM: Range base=0x%lx size=0x%lx overlaps entry %u\n",  // [CODE_FIRST] 13253
          Base,  // [CODE_FIRST] 13253
          Size,  // [CODE_FIRST] 13253
          Index  // [CODE_FIRST] 13253
          ));  // [CODE_FIRST] 13253
      }  // [CODE_FIRST] 13253
// [CODE_FIRST] 13253
      return EFI_ACCESS_DENIED;  // [CODE_FIRST] 13253
    }  // [CODE_FIRST] 13253
  }  // [CODE_FIRST] 13253
// [CODE_FIRST] 13253
  if (mEntryCount >= RMEM_MAX_ENTRIES) {  // [CODE_FIRST] 13253
    DEBUG ((DEBUG_ERROR, "RMEM: Registration capacity of %u entries has been reached\n", RMEM_MAX_ENTRIES));  // [CODE_FIRST] 13253
    return EFI_OUT_OF_RESOURCES;  // [CODE_FIRST] 13253
  }  // [CODE_FIRST] 13253
// [CODE_FIRST] 13253
  ZeroMem (&mEntries[mEntryCount], sizeof (mEntries[mEntryCount]));  // [CODE_FIRST] 13253
  mEntries[mEntryCount].Base     = Base;  // [CODE_FIRST] 13253
  mEntries[mEntryCount].Size     = Size;  // [CODE_FIRST] 13253
  mEntries[mEntryCount].Category = (UINT16)Category;  // [CODE_FIRST] 13253
  mEntries[mEntryCount].Flags    = Flags;  // [CODE_FIRST] 13253
  CopyMem (  // [CODE_FIRST] 13253
    mEntries[mEntryCount].Label,  // [CODE_FIRST] 13253
    EffectiveLabel,  // [CODE_FIRST] 13253
    LabelLength + 1  // [CODE_FIRST] 13253
    );  // [CODE_FIRST] 13253
  mEntryCount++;  // [CODE_FIRST] 13253
// [CODE_FIRST] 13253
  return EFI_SUCCESS;  // [CODE_FIRST] 13253
}  // [CODE_FIRST] 13253
// [CODE_FIRST] 13253
STATIC EDKII_RMEM_REGISTRATION_PROTOCOL  mRmemProtocol = {  // [CODE_FIRST] 13253
  EDKII_RMEM_REGISTRATION_PROTOCOL_REVISION,  // [CODE_FIRST] 13253
  RmemAddReservedRange  // [CODE_FIRST] 13253
};  // [CODE_FIRST] 13253
// [CODE_FIRST] 13253
STATIC  // [CODE_FIRST] 13253
EFI_STATUS  // [CODE_FIRST] 13253
RmemImportHobs (  // [CODE_FIRST] 13253
  VOID  // [CODE_FIRST] 13253
  )  // [CODE_FIRST] 13253
{  // [CODE_FIRST] 13253
  EFI_HOB_GUID_TYPE  *GuidHob;  // [CODE_FIRST] 13253
  EFI_HOB_GUID_TYPE  *NextGuidHob;  // [CODE_FIRST] 13253
  RMEM_HOB_RECORD    *Record;  // [CODE_FIRST] 13253
  EFI_STATUS         Status;  // [CODE_FIRST] 13253
  UINT32             HobIndex;  // [CODE_FIRST] 13253
// [CODE_FIRST] 13253
  HobIndex = 0;  // [CODE_FIRST] 13253
  GuidHob  = GetFirstGuidHob (&gEdkiiRmemRecordHobGuid);  // [CODE_FIRST] 13253
  while (GuidHob != NULL) {  // [CODE_FIRST] 13253
    NextGuidHob = GetNextGuidHob (  // [CODE_FIRST] 13253
                    &gEdkiiRmemRecordHobGuid,  // [CODE_FIRST] 13253
                    GET_NEXT_HOB (GuidHob)  // [CODE_FIRST] 13253
                    );  // [CODE_FIRST] 13253
// [CODE_FIRST] 13253
    if (GET_GUID_HOB_DATA_SIZE (GuidHob) != sizeof (RMEM_HOB_RECORD)) {  // [CODE_FIRST] 13253
      DEBUG ((  // [CODE_FIRST] 13253
        DEBUG_ERROR,  // [CODE_FIRST] 13253
        "RMEM: HOB %u has invalid payload size %u\n",  // [CODE_FIRST] 13253
        HobIndex,  // [CODE_FIRST] 13253
        (UINT32)GET_GUID_HOB_DATA_SIZE (GuidHob)  // [CODE_FIRST] 13253
        ));  // [CODE_FIRST] 13253
      ASSERT (GET_GUID_HOB_DATA_SIZE (GuidHob) == sizeof (RMEM_HOB_RECORD));  // [CODE_FIRST] 13253
      GuidHob = NextGuidHob;  // [CODE_FIRST] 13253
      HobIndex++;  // [CODE_FIRST] 13253
      continue;  // [CODE_FIRST] 13253
    }  // [CODE_FIRST] 13253
// [CODE_FIRST] 13253
    Record = (RMEM_HOB_RECORD *)GET_GUID_HOB_DATA (GuidHob);  // [CODE_FIRST] 13253
    Status = RmemAddReservedRange (  // [CODE_FIRST] 13253
               &mRmemProtocol,  // [CODE_FIRST] 13253
               Record->Base,  // [CODE_FIRST] 13253
               Record->Size,  // [CODE_FIRST] 13253
               (RMEM_CATEGORY)Record->Category,  // [CODE_FIRST] 13253
               Record->Flags,  // [CODE_FIRST] 13253
               Record->Label  // [CODE_FIRST] 13253
               );  // [CODE_FIRST] 13253
    if (EFI_ERROR (Status)) {  // [CODE_FIRST] 13253
      DEBUG ((DEBUG_ERROR, "RMEM: HOB %u was rejected: %r\n", HobIndex, Status));  // [CODE_FIRST] 13253
      ASSERT_EFI_ERROR (Status);  // [CODE_FIRST] 13253
    }  // [CODE_FIRST] 13253
// [CODE_FIRST] 13253
    GuidHob = NextGuidHob;  // [CODE_FIRST] 13253
    HobIndex++;  // [CODE_FIRST] 13253
  }  // [CODE_FIRST] 13253
// [CODE_FIRST] 13253
  return EFI_SUCCESS;  // [CODE_FIRST] 13253
}  // [CODE_FIRST] 13253
// [CODE_FIRST] 13253
STATIC  // [CODE_FIRST] 13253
EFI_STATUS  // [CODE_FIRST] 13253
RmemPublishTable (  // [CODE_FIRST] 13253
  VOID  // [CODE_FIRST] 13253
  )  // [CODE_FIRST] 13253
{  // [CODE_FIRST] 13253
  EFI_ACPI_TABLE_PROTOCOL  *AcpiTableProtocol;  // [CODE_FIRST] 13253
  RMEM_TABLE_HEADER        *Table;  // [CODE_FIRST] 13253
  RMEM_ENTRY               *TableEntries;  // [CODE_FIRST] 13253
  EFI_STATUS               Status;  // [CODE_FIRST] 13253
  UINT32                   Index;  // [CODE_FIRST] 13253
  UINTN                    TableKey;  // [CODE_FIRST] 13253
  UINTN                    TableSize;  // [CODE_FIRST] 13253
// [CODE_FIRST] 13253
  if (mEntryCount == 0) {  // [CODE_FIRST] 13253
    return EFI_NOT_FOUND;  // [CODE_FIRST] 13253
  }  // [CODE_FIRST] 13253
// [CODE_FIRST] 13253
  TableSize = sizeof (RMEM_TABLE_HEADER) +  // [CODE_FIRST] 13253
              ((UINTN)mEntryCount * sizeof (RMEM_ENTRY));  // [CODE_FIRST] 13253
  Table = AllocateZeroPool (TableSize);  // [CODE_FIRST] 13253
  if (Table == NULL) {  // [CODE_FIRST] 13253
    return EFI_OUT_OF_RESOURCES;  // [CODE_FIRST] 13253
  }  // [CODE_FIRST] 13253
// [CODE_FIRST] 13253
  Table->Header.Signature = RMEM_TABLE_SIGNATURE;  // [CODE_FIRST] 13253
  Table->Header.Length    = (UINT32)TableSize;  // [CODE_FIRST] 13253
  Table->Header.Revision  = RMEM_TABLE_REVISION;  // [CODE_FIRST] 13253
  CopyMem (  // [CODE_FIRST] 13253
    Table->Header.OemId,  // [CODE_FIRST] 13253
    PcdGetPtr (PcdAcpiDefaultOemId),  // [CODE_FIRST] 13253
    MIN (PcdGetSize (PcdAcpiDefaultOemId), sizeof (Table->Header.OemId))  // [CODE_FIRST] 13253
    );  // [CODE_FIRST] 13253
  WriteUnaligned64 (  // [CODE_FIRST] 13253
    &Table->Header.OemTableId,  // [CODE_FIRST] 13253
    PcdGet64 (PcdAcpiDefaultOemTableId)  // [CODE_FIRST] 13253
    );  // [CODE_FIRST] 13253
  Table->Header.OemRevision     = PcdGet32 (PcdAcpiDefaultOemRevision);  // [CODE_FIRST] 13253
  Table->Header.CreatorId       = PcdGet32 (PcdAcpiDefaultCreatorId);  // [CODE_FIRST] 13253
  Table->Header.CreatorRevision = PcdGet32 (PcdAcpiDefaultCreatorRevision);  // [CODE_FIRST] 13253
  Table->EntryCount             = mEntryCount;  // [CODE_FIRST] 13253
  Table->EntryOffset            = (UINT16)sizeof (RMEM_TABLE_HEADER);  // [CODE_FIRST] 13253
// [CODE_FIRST] 13253
  CopyMem (  // [CODE_FIRST] 13253
    (UINT8 *)Table + Table->EntryOffset,  // [CODE_FIRST] 13253
    mEntries,  // [CODE_FIRST] 13253
    (UINTN)mEntryCount * sizeof (RMEM_ENTRY)  // [CODE_FIRST] 13253
    );  // [CODE_FIRST] 13253
  TableEntries = (RMEM_ENTRY *)((UINT8 *)Table + Table->EntryOffset);  // [CODE_FIRST] 13253
  for (Index = 0; Index < mEntryCount; Index++) {  // [CODE_FIRST] 13253
    if ((TableEntries[Index].Flags & RMEM_ENTRY_FLAG_ADDRESS_HIDDEN) != 0) {  // [CODE_FIRST] 13253
      TableEntries[Index].Base = 0;  // [CODE_FIRST] 13253
    }  // [CODE_FIRST] 13253
  }  // [CODE_FIRST] 13253
// [CODE_FIRST] 13253
  Table->Header.Checksum = CalculateCheckSum8 ((UINT8 *)Table, TableSize);  // [CODE_FIRST] 13253
// [CODE_FIRST] 13253
  Status = gBS->LocateProtocol (  // [CODE_FIRST] 13253
                  &gEfiAcpiTableProtocolGuid,  // [CODE_FIRST] 13253
                  NULL,  // [CODE_FIRST] 13253
                  (VOID **)&AcpiTableProtocol  // [CODE_FIRST] 13253
                  );  // [CODE_FIRST] 13253
  if (!EFI_ERROR (Status)) {  // [CODE_FIRST] 13253
    Status = AcpiTableProtocol->InstallAcpiTable (  // [CODE_FIRST] 13253
                                  AcpiTableProtocol,  // [CODE_FIRST] 13253
                                  Table,  // [CODE_FIRST] 13253
                                  TableSize,  // [CODE_FIRST] 13253
                                  &TableKey  // [CODE_FIRST] 13253
                                  );  // [CODE_FIRST] 13253
  }  // [CODE_FIRST] 13253
// [CODE_FIRST] 13253
  FreePool (Table);  // [CODE_FIRST] 13253
  return Status;  // [CODE_FIRST] 13253
}  // [CODE_FIRST] 13253
// [CODE_FIRST] 13253
STATIC  // [CODE_FIRST] 13253
VOID  // [CODE_FIRST] 13253
EFIAPI  // [CODE_FIRST] 13253
RmemOnPublicationEvent (  // [CODE_FIRST] 13253
  IN EFI_EVENT  Event,  // [CODE_FIRST] 13253
  IN VOID       *Context  // [CODE_FIRST] 13253
  )  // [CODE_FIRST] 13253
{  // [CODE_FIRST] 13253
  EFI_STATUS  Status;  // [CODE_FIRST] 13253
// [CODE_FIRST] 13253
  (VOID)Context;  // [CODE_FIRST] 13253
// [CODE_FIRST] 13253
  mFinalized = TRUE;  // [CODE_FIRST] 13253
  Status     = RmemPublishTable ();  // [CODE_FIRST] 13253
  if ((Status != EFI_NOT_FOUND) && EFI_ERROR (Status)) {  // [CODE_FIRST] 13253
    DEBUG ((DEBUG_ERROR, "RMEM: Failed to publish table: %r\n", Status));  // [CODE_FIRST] 13253
  }  // [CODE_FIRST] 13253
// [CODE_FIRST] 13253
  gBS->CloseEvent (Event);  // [CODE_FIRST] 13253
  mPublicationEvent = NULL;  // [CODE_FIRST] 13253
}  // [CODE_FIRST] 13253
// [CODE_FIRST] 13253
STATIC  // [CODE_FIRST] 13253
EFI_STATUS  // [CODE_FIRST] 13253
RmemInstallProtocolAndEvent (  // [CODE_FIRST] 13253
  VOID  // [CODE_FIRST] 13253
  )  // [CODE_FIRST] 13253
{  // [CODE_FIRST] 13253
  EFI_HANDLE  ProtocolHandle;  // [CODE_FIRST] 13253
  EFI_STATUS  Status;  // [CODE_FIRST] 13253
// [CODE_FIRST] 13253
  ProtocolHandle = NULL;  // [CODE_FIRST] 13253
  Status         = gBS->InstallProtocolInterface (  // [CODE_FIRST] 13253
                          &ProtocolHandle,  // [CODE_FIRST] 13253
                          &gEdkiiRmemRegistrationProtocolGuid,  // [CODE_FIRST] 13253
                          EFI_NATIVE_INTERFACE,  // [CODE_FIRST] 13253
                          &mRmemProtocol  // [CODE_FIRST] 13253
                          );  // [CODE_FIRST] 13253
  if (EFI_ERROR (Status)) {  // [CODE_FIRST] 13253
    return Status;  // [CODE_FIRST] 13253
  }  // [CODE_FIRST] 13253
// [CODE_FIRST] 13253
  Status = gBS->CreateEventEx (  // [CODE_FIRST] 13253
                  EVT_NOTIFY_SIGNAL,  // [CODE_FIRST] 13253
                  TPL_CALLBACK,  // [CODE_FIRST] 13253
                  RmemOnPublicationEvent,  // [CODE_FIRST] 13253
                  NULL,  // [CODE_FIRST] 13253
                  &gEfiEventReadyToBootGuid,  // [CODE_FIRST] 13253
                  &mPublicationEvent  // [CODE_FIRST] 13253
                  );  // [CODE_FIRST] 13253
  if (EFI_ERROR (Status)) {  // [CODE_FIRST] 13253
    gBS->UninstallProtocolInterface (  // [CODE_FIRST] 13253
           ProtocolHandle,  // [CODE_FIRST] 13253
           &gEdkiiRmemRegistrationProtocolGuid,  // [CODE_FIRST] 13253
           &mRmemProtocol  // [CODE_FIRST] 13253
           );  // [CODE_FIRST] 13253
  }  // [CODE_FIRST] 13253
// [CODE_FIRST] 13253
  return Status;  // [CODE_FIRST] 13253
}  // [CODE_FIRST] 13253
// [CODE_FIRST] 13253
EFI_STATUS  // [CODE_FIRST] 13253
EFIAPI  // [CODE_FIRST] 13253
RmemAcpiDxeEntryPoint (  // [CODE_FIRST] 13253
  IN EFI_HANDLE        ImageHandle,  // [CODE_FIRST] 13253
  IN EFI_SYSTEM_TABLE  *SystemTable  // [CODE_FIRST] 13253
  )  // [CODE_FIRST] 13253
{  // [CODE_FIRST] 13253
  EFI_STATUS  Status;  // [CODE_FIRST] 13253
// [CODE_FIRST] 13253
  // Required by the UEFI driver entry-point ABI but unused by this driver.  // [CODE_FIRST] 13253
  (VOID)ImageHandle;  // [CODE_FIRST] 13253
  (VOID)SystemTable;  // [CODE_FIRST] 13253
// [CODE_FIRST] 13253
  Status = RmemInitializeMaximumPhysicalAddress ();  // [CODE_FIRST] 13253
  if (EFI_ERROR (Status)) {  // [CODE_FIRST] 13253
    return Status;  // [CODE_FIRST] 13253
  }  // [CODE_FIRST] 13253
// [CODE_FIRST] 13253
  Status = RmemImportHobs ();  // [CODE_FIRST] 13253
  if (EFI_ERROR (Status)) {  // [CODE_FIRST] 13253
    return Status;  // [CODE_FIRST] 13253
  }  // [CODE_FIRST] 13253
// [CODE_FIRST] 13253
  return RmemInstallProtocolAndEvent ();  // [CODE_FIRST] 13253
}  // [CODE_FIRST] 13253
