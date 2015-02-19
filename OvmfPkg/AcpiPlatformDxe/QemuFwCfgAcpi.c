/** @file
  OVMF ACPI support using QEMU's fw-cfg interface

  Copyright (c) 2008 - 2014, Intel Corporation. All rights reserved.<BR>
  Copyright (C) 2012-2014, Red Hat, Inc.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "AcpiPlatform.h"
#include "QemuLoader.h"
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/QemuFwCfgLib.h>
#include <Library/DxeServicesTableLib.h>
#include <Library/PcdLib.h>
#include <Library/OrderedCollectionLib.h>
#include <IndustryStandard/Acpi.h>


//
// The user structure for the ordered collection that will track the fw_cfg
// blobs under processing.
//
typedef struct {
  UINT8   File[QEMU_LOADER_FNAME_SIZE]; // NUL-terminated name of the fw_cfg
                                        // blob. This is the ordering / search
                                        // key.
  UINTN   Size;                         // The number of bytes in this blob.
  UINT8   *Base;                        // Pointer to the blob data.
  BOOLEAN HostsOnlyTableData;           // TRUE iff the blob has been found to
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
  IN CONST VOID *StandaloneKey,
  IN CONST VOID *UserStruct
  )
{
  CONST BLOB *Blob;

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
  IN CONST VOID *UserStruct1,
  IN CONST VOID *UserStruct2
  )
{
  CONST BLOB *Blob1;

  Blob1 = UserStruct1;
  return BlobKeyCompare (Blob1->File, UserStruct2);
}


/**
  Process a QEMU_LOADER_ALLOCATE command.

  @param[in] Allocate     The QEMU_LOADER_ALLOCATE command to process.

  @param[in,out] Tracker  The ORDERED_COLLECTION tracking the BLOB user
                          structures created thus far.

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
  IN CONST QEMU_LOADER_ALLOCATE *Allocate,
  IN OUT ORDERED_COLLECTION     *Tracker
  )
{
  FIRMWARE_CONFIG_ITEM FwCfgItem;
  UINTN                FwCfgSize;
  EFI_STATUS           Status;
  UINTN                NumPages;
  EFI_PHYSICAL_ADDRESS Address;
  BLOB                 *Blob;

  if (Allocate->File[QEMU_LOADER_FNAME_SIZE - 1] != '\0') {
    DEBUG ((EFI_D_ERROR, "%a: malformed file name\n", __FUNCTION__));
    return EFI_PROTOCOL_ERROR;
  }

  if (Allocate->Alignment > EFI_PAGE_SIZE) {
    DEBUG ((EFI_D_ERROR, "%a: unsupported alignment 0x%x\n", __FUNCTION__,
      Allocate->Alignment));
    return EFI_UNSUPPORTED;
  }

  Status = QemuFwCfgFindFile ((CHAR8 *)Allocate->File, &FwCfgItem, &FwCfgSize);
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "%a: QemuFwCfgFindFile(\"%a\"): %r\n", __FUNCTION__,
      Allocate->File, Status));
    return Status;
  }

  NumPages = EFI_SIZE_TO_PAGES (FwCfgSize);
  Address = 0xFFFFFFFF;
  Status = gBS->AllocatePages (AllocateMaxAddress, EfiACPIMemoryNVS, NumPages,
                  &Address);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Blob = AllocatePool (sizeof *Blob);
  if (Blob == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto FreePages;
  }
  CopyMem (Blob->File, Allocate->File, QEMU_LOADER_FNAME_SIZE);
  Blob->Size = FwCfgSize;
  Blob->Base = (VOID *)(UINTN)Address;
  Blob->HostsOnlyTableData = TRUE;

  Status = OrderedCollectionInsert (Tracker, NULL, Blob);
  if (Status == RETURN_ALREADY_STARTED) {
    DEBUG ((EFI_D_ERROR, "%a: duplicated file \"%a\"\n", __FUNCTION__,
      Allocate->File));
    Status = EFI_PROTOCOL_ERROR;
  }
  if (EFI_ERROR (Status)) {
    goto FreeBlob;
  }

  QemuFwCfgSelectItem (FwCfgItem);
  QemuFwCfgReadBytes (FwCfgSize, Blob->Base);
  ZeroMem (Blob->Base + Blob->Size, EFI_PAGES_TO_SIZE (NumPages) - Blob->Size);

  DEBUG ((EFI_D_VERBOSE, "%a: File=\"%a\" Alignment=0x%x Zone=%d Size=0x%Lx "
    "Address=0x%Lx\n", __FUNCTION__, Allocate->File, Allocate->Alignment,
    Allocate->Zone, (UINT64)Blob->Size, (UINT64)(UINTN)Blob->Base));
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
  IN CONST QEMU_LOADER_ADD_POINTER *AddPointer,
  IN CONST ORDERED_COLLECTION      *Tracker
  )
{
  ORDERED_COLLECTION_ENTRY *TrackerEntry, *TrackerEntry2;
  BLOB                     *Blob, *Blob2;
  UINT8                    *PointerField;
  UINT64                   PointerValue;

  if (AddPointer->PointerFile[QEMU_LOADER_FNAME_SIZE - 1] != '\0' ||
      AddPointer->PointeeFile[QEMU_LOADER_FNAME_SIZE - 1] != '\0') {
    DEBUG ((EFI_D_ERROR, "%a: malformed file name\n", __FUNCTION__));
    return EFI_PROTOCOL_ERROR;
  }

  TrackerEntry = OrderedCollectionFind (Tracker, AddPointer->PointerFile);
  TrackerEntry2 = OrderedCollectionFind (Tracker, AddPointer->PointeeFile);
  if (TrackerEntry == NULL || TrackerEntry2 == NULL) {
    DEBUG ((EFI_D_ERROR, "%a: invalid blob reference(s) \"%a\" / \"%a\"\n",
      __FUNCTION__, AddPointer->PointerFile, AddPointer->PointeeFile));
    return EFI_PROTOCOL_ERROR;
  }

  Blob = OrderedCollectionUserStruct (TrackerEntry);
  Blob2 = OrderedCollectionUserStruct (TrackerEntry2);
  if ((AddPointer->PointerSize != 1 && AddPointer->PointerSize != 2 &&
       AddPointer->PointerSize != 4 && AddPointer->PointerSize != 8) ||
      Blob->Size < AddPointer->PointerSize ||
      Blob->Size - AddPointer->PointerSize < AddPointer->PointerOffset) {
    DEBUG ((EFI_D_ERROR, "%a: invalid pointer location or size in \"%a\"\n",
      __FUNCTION__, AddPointer->PointerFile));
    return EFI_PROTOCOL_ERROR;
  }

  PointerField = Blob->Base + AddPointer->PointerOffset;
  PointerValue = 0;
  CopyMem (&PointerValue, PointerField, AddPointer->PointerSize);
  if (PointerValue >= Blob2->Size) {
    DEBUG ((EFI_D_ERROR, "%a: invalid pointer value in \"%a\"\n", __FUNCTION__,
      AddPointer->PointerFile));
    return EFI_PROTOCOL_ERROR;
  }

  //
  // The memory allocation system ensures that the address of the byte past the
  // last byte of any allocated object is expressible (no wraparound).
  //
  ASSERT ((UINTN)Blob2->Base <= MAX_ADDRESS - Blob2->Size);

  PointerValue += (UINT64)(UINTN)Blob2->Base;
  if (RShiftU64 (
        RShiftU64 (PointerValue, AddPointer->PointerSize * 8 - 1), 1) != 0) {
    DEBUG ((EFI_D_ERROR, "%a: relocated pointer value unrepresentable in "
      "\"%a\"\n", __FUNCTION__, AddPointer->PointerFile));
    return EFI_PROTOCOL_ERROR;
  }

  CopyMem (PointerField, &PointerValue, AddPointer->PointerSize);

  DEBUG ((EFI_D_VERBOSE, "%a: PointerFile=\"%a\" PointeeFile=\"%a\" "
    "PointerOffset=0x%x PointerSize=%d\n", __FUNCTION__,
    AddPointer->PointerFile, AddPointer->PointeeFile,
    AddPointer->PointerOffset, AddPointer->PointerSize));
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
  IN CONST QEMU_LOADER_ADD_CHECKSUM *AddChecksum,
  IN CONST ORDERED_COLLECTION       *Tracker
  )
{
  ORDERED_COLLECTION_ENTRY *TrackerEntry;
  BLOB                     *Blob;

  if (AddChecksum->File[QEMU_LOADER_FNAME_SIZE - 1] != '\0') {
    DEBUG ((EFI_D_ERROR, "%a: malformed file name\n", __FUNCTION__));
    return EFI_PROTOCOL_ERROR;
  }

  TrackerEntry = OrderedCollectionFind (Tracker, AddChecksum->File);
  if (TrackerEntry == NULL) {
    DEBUG ((EFI_D_ERROR, "%a: invalid blob reference \"%a\"\n", __FUNCTION__,
      AddChecksum->File));
    return EFI_PROTOCOL_ERROR;
  }

  Blob = OrderedCollectionUserStruct (TrackerEntry);
  if (Blob->Size <= AddChecksum->ResultOffset ||
      Blob->Size < AddChecksum->Length ||
      Blob->Size - AddChecksum->Length < AddChecksum->Start) {
    DEBUG ((EFI_D_ERROR, "%a: invalid checksum range in \"%a\"\n",
      __FUNCTION__, AddChecksum->File));
    return EFI_PROTOCOL_ERROR;
  }

  Blob->Base[AddChecksum->ResultOffset] = CalculateCheckSum8 (
                                           Blob->Base + AddChecksum->Start,
                                           AddChecksum->Length
                                           );
  DEBUG ((EFI_D_VERBOSE, "%a: File=\"%a\" ResultOffset=0x%x Start=0x%x "
    "Length=0x%x\n", __FUNCTION__, AddChecksum->File,
    AddChecksum->ResultOffset, AddChecksum->Start, AddChecksum->Length));
  return EFI_SUCCESS;
}


//
// We'll be saving the keys of installed tables so that we can roll them back
// in case of failure. 128 tables should be enough for anyone (TM).
//
#define INSTALLED_TABLES_MAX 128

/**
  Process a QEMU_LOADER_ADD_POINTER command in order to see if its target byte
  array is an ACPI table, and if so, install it.

  This function assumes that the entire QEMU linker/loader command file has
  been processed successfuly in a prior first pass.

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

  @retval EFI_INVALID_PARAMETER  NumInstalled was outside the allowed range on
                                 input.

  @retval EFI_OUT_OF_RESOURCES   The AddPointer command identified an ACPI
                                 table different from RSDT and XSDT, but there
                                 was no more room in InstalledKey.

  @retval EFI_SUCCESS            AddPointer has been processed. Either an ACPI
                                 table different from RSDT and XSDT has been
                                 installed (reflected by InstalledKey and
                                 NumInstalled), or RSDT or XSDT has been
                                 identified but not installed, or the fw_cfg
                                 blob pointed-into by AddPointer has been
                                 marked as hosting something else than just
                                 direct ACPI table contents.

  @return                        Error codes returned by
                                 AcpiProtocol->InstallAcpiTable().
**/
STATIC
EFI_STATUS
EFIAPI
Process2ndPassCmdAddPointer (
  IN     CONST QEMU_LOADER_ADD_POINTER *AddPointer,
  IN     CONST ORDERED_COLLECTION      *Tracker,
  IN     EFI_ACPI_TABLE_PROTOCOL       *AcpiProtocol,
  IN OUT UINTN                         InstalledKey[INSTALLED_TABLES_MAX],
  IN OUT INT32                         *NumInstalled
  )
{
  CONST ORDERED_COLLECTION_ENTRY                     *TrackerEntry;
  CONST ORDERED_COLLECTION_ENTRY                     *TrackerEntry2;
  CONST BLOB                                         *Blob;
  BLOB                                               *Blob2;
  CONST UINT8                                        *PointerField;
  UINT64                                             PointerValue;
  UINTN                                              Blob2Remaining;
  UINTN                                              TableSize;
  CONST EFI_ACPI_1_0_FIRMWARE_ACPI_CONTROL_STRUCTURE *Facs;
  CONST EFI_ACPI_DESCRIPTION_HEADER                  *Header;
  EFI_STATUS                                         Status;

  if (*NumInstalled < 0 || *NumInstalled > INSTALLED_TABLES_MAX) {
    return EFI_INVALID_PARAMETER;
  }

  TrackerEntry = OrderedCollectionFind (Tracker, AddPointer->PointerFile);
  TrackerEntry2 = OrderedCollectionFind (Tracker, AddPointer->PointeeFile);
  Blob = OrderedCollectionUserStruct (TrackerEntry);
  Blob2 = OrderedCollectionUserStruct (TrackerEntry2);
  PointerField = Blob->Base + AddPointer->PointerOffset;
  PointerValue = 0;
  CopyMem (&PointerValue, PointerField, AddPointer->PointerSize);

  //
  // We assert that PointerValue falls inside Blob2's contents. This is ensured
  // by the Blob2->Size check and later checks in ProcessCmdAddPointer().
  //
  Blob2Remaining = (UINTN)Blob2->Base;
  ASSERT(PointerValue >= Blob2Remaining);
  Blob2Remaining += Blob2->Size;
  ASSERT (PointerValue < Blob2Remaining);

  Blob2Remaining -= (UINTN) PointerValue;
  DEBUG ((EFI_D_VERBOSE, "%a: checking for ACPI header in \"%a\" at 0x%Lx "
    "(remaining: 0x%Lx): ", __FUNCTION__, AddPointer->PointeeFile,
    PointerValue, (UINT64)Blob2Remaining));

  TableSize = 0;

  //
  // To make our job simple, the FACS has a custom header. Sigh.
  //
  if (sizeof *Facs <= Blob2Remaining) {
    Facs = (EFI_ACPI_1_0_FIRMWARE_ACPI_CONTROL_STRUCTURE *)(UINTN)PointerValue;

    if (Facs->Length >= sizeof *Facs &&
        Facs->Length <= Blob2Remaining &&
        Facs->Signature ==
                EFI_ACPI_1_0_FIRMWARE_ACPI_CONTROL_STRUCTURE_SIGNATURE) {
      DEBUG ((EFI_D_VERBOSE, "found \"%-4.4a\" size 0x%x\n",
        (CONST CHAR8 *)&Facs->Signature, Facs->Length));
      TableSize = Facs->Length;
    }
  }

  //
  // check for the uniform tables
  //
  if (TableSize == 0 && sizeof *Header <= Blob2Remaining) {
    Header = (EFI_ACPI_DESCRIPTION_HEADER *)(UINTN)PointerValue;

    if (Header->Length >= sizeof *Header &&
        Header->Length <= Blob2Remaining &&
        CalculateSum8 ((CONST UINT8 *)Header, Header->Length) == 0) {
      //
      // This looks very much like an ACPI table from QEMU:
      // - Length field consistent with both ACPI and containing blob size
      // - checksum is correct
      //
      DEBUG ((EFI_D_VERBOSE, "found \"%-4.4a\" size 0x%x\n",
        (CONST CHAR8 *)&Header->Signature, Header->Length));
      TableSize = Header->Length;

      //
      // Skip RSDT and XSDT because those are handled by
      // EFI_ACPI_TABLE_PROTOCOL automatically.
      if (Header->Signature ==
                    EFI_ACPI_1_0_ROOT_SYSTEM_DESCRIPTION_TABLE_SIGNATURE ||
          Header->Signature ==
                    EFI_ACPI_2_0_EXTENDED_SYSTEM_DESCRIPTION_TABLE_SIGNATURE) {
        return EFI_SUCCESS;
      }
    }
  }

  if (TableSize == 0) {
    DEBUG ((EFI_D_VERBOSE, "not found; marking fw_cfg blob as opaque\n"));
    Blob2->HostsOnlyTableData = FALSE;
    return EFI_SUCCESS;
  }

  if (*NumInstalled == INSTALLED_TABLES_MAX) {
    DEBUG ((EFI_D_ERROR, "%a: can't install more than %d tables\n",
      __FUNCTION__, INSTALLED_TABLES_MAX));
    return EFI_OUT_OF_RESOURCES;
  }

  Status = AcpiProtocol->InstallAcpiTable (AcpiProtocol,
                           (VOID *)(UINTN)PointerValue, TableSize,
                           &InstalledKey[*NumInstalled]);
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "%a: InstallAcpiTable(): %r\n", __FUNCTION__,
      Status));
    return Status;
  }
  ++*NumInstalled;
  return EFI_SUCCESS;
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
  IN   EFI_ACPI_TABLE_PROTOCOL       *AcpiProtocol
  )
{
  EFI_STATUS               Status;
  FIRMWARE_CONFIG_ITEM     FwCfgItem;
  UINTN                    FwCfgSize;
  QEMU_LOADER_ENTRY        *LoaderStart;
  CONST QEMU_LOADER_ENTRY  *LoaderEntry, *LoaderEnd;
  ORDERED_COLLECTION       *Tracker;
  UINTN                    *InstalledKey;
  INT32                    Installed;
  ORDERED_COLLECTION_ENTRY *TrackerEntry, *TrackerEntry2;

  Status = QemuFwCfgFindFile ("etc/table-loader", &FwCfgItem, &FwCfgSize);
  if (EFI_ERROR (Status)) {
    return Status;
  }
  if (FwCfgSize % sizeof *LoaderEntry != 0) {
    DEBUG ((EFI_D_ERROR, "%a: \"etc/table-loader\" has invalid size 0x%Lx\n",
      __FUNCTION__, (UINT64)FwCfgSize));
    return EFI_PROTOCOL_ERROR;
  }

  LoaderStart = AllocatePool (FwCfgSize);
  if (LoaderStart == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }
  QemuFwCfgSelectItem (FwCfgItem);
  QemuFwCfgReadBytes (FwCfgSize, LoaderStart);
  LoaderEnd = LoaderStart + FwCfgSize / sizeof *LoaderEntry;

  Tracker = OrderedCollectionInit (BlobCompare, BlobKeyCompare);
  if (Tracker == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto FreeLoader;
  }

  //
  // first pass: process the commands
  //
  for (LoaderEntry = LoaderStart; LoaderEntry < LoaderEnd; ++LoaderEntry) {
    switch (LoaderEntry->Type) {
    case QemuLoaderCmdAllocate:
      Status = ProcessCmdAllocate (&LoaderEntry->Command.Allocate, Tracker);
      break;

    case QemuLoaderCmdAddPointer:
      Status = ProcessCmdAddPointer (&LoaderEntry->Command.AddPointer,
                 Tracker);
      break;

    case QemuLoaderCmdAddChecksum:
      Status = ProcessCmdAddChecksum (&LoaderEntry->Command.AddChecksum,
                 Tracker);
      break;

    default:
      DEBUG ((EFI_D_VERBOSE, "%a: unknown loader command: 0x%x\n",
        __FUNCTION__, LoaderEntry->Type));
      break;
    }

    if (EFI_ERROR (Status)) {
      goto FreeTracker;
    }
  }

  InstalledKey = AllocatePool (INSTALLED_TABLES_MAX * sizeof *InstalledKey);
  if (InstalledKey == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto FreeTracker;
  }

  //
  // second pass: identify and install ACPI tables
  //
  Installed = 0;
  for (LoaderEntry = LoaderStart; LoaderEntry < LoaderEnd; ++LoaderEntry) {
    if (LoaderEntry->Type == QemuLoaderCmdAddPointer) {
      Status = Process2ndPassCmdAddPointer (&LoaderEntry->Command.AddPointer,
                 Tracker, AcpiProtocol, InstalledKey, &Installed);
      if (EFI_ERROR (Status)) {
        break;
      }
    }
  }

  if (EFI_ERROR (Status)) {
    //
    // roll back partial installation
    //
    while (Installed > 0) {
      --Installed;
      AcpiProtocol->UninstallAcpiTable (AcpiProtocol, InstalledKey[Installed]);
    }
  } else {
    DEBUG ((EFI_D_INFO, "%a: installed %d tables\n", __FUNCTION__, Installed));
  }

  FreePool (InstalledKey);

FreeTracker:
  //
  // Tear down the tracker infrastructure. Each fw_cfg blob will be left in
  // place only if we're exiting with success and the blob hosts data that is
  // not directly part of some ACPI table.
  //
  for (TrackerEntry = OrderedCollectionMin (Tracker); TrackerEntry != NULL;
       TrackerEntry = TrackerEntry2) {
    VOID *UserStruct;
    BLOB *Blob;

    TrackerEntry2 = OrderedCollectionNext (TrackerEntry);
    OrderedCollectionDelete (Tracker, TrackerEntry, &UserStruct);
    Blob = UserStruct;

    if (EFI_ERROR (Status) || Blob->HostsOnlyTableData) {
      DEBUG ((EFI_D_VERBOSE, "%a: freeing \"%a\"\n", __FUNCTION__,
        Blob->File));
      gBS->FreePages ((UINTN)Blob->Base, EFI_SIZE_TO_PAGES (Blob->Size));
    }
    FreePool (Blob);
  }
  OrderedCollectionUninit (Tracker);

FreeLoader:
  FreePool (LoaderStart);

  return Status;
}
