/** @file

Copyright (c) 2006 - 2018, Intel Corporation. All rights reserved.<BR>

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "LegacyBiosInterface.h"

#define PHYSICAL_ADDRESS_TO_POINTER(Address)  ((VOID *) ((UINTN) Address))

//
// define maximum number of HDD system supports
//
#define MAX_HDD_ENTRIES  0x30

//
// Module Global:
//  Since this driver will only ever produce one instance of the Private Data
//  protocol you are not required to dynamically allocate the PrivateData.
//
LEGACY_BIOS_INSTANCE  mPrivateData;

//
// The SMBIOS table in EfiRuntimeServicesData memory
//
VOID  *mRuntimeSmbiosEntryPoint = NULL;

//
// The SMBIOS table in EfiReservedMemoryType memory
//
EFI_PHYSICAL_ADDRESS  mReserveSmbiosEntryPoint = 0;
EFI_PHYSICAL_ADDRESS  mStructureTableAddress   = 0;
UINTN                 mStructureTablePages     = 0;
BOOLEAN               mEndOfDxe                = FALSE;

/**
  Allocate memory for legacy usage. The memory is executable.

  @param  AllocateType               The type of allocation to perform.
  @param  MemoryType                 The type of memory to allocate.
  @param  StartPageAddress           Start address of range
  @param  Pages                      Number of pages to allocate
  @param  Result                     Result of allocation

  @retval EFI_SUCCESS                Legacy memory is allocated successfully.
  @retval Other                      Legacy memory is not allocated.

**/
EFI_STATUS
AllocateLegacyMemory (
  IN  EFI_ALLOCATE_TYPE     AllocateType,
  IN  EFI_MEMORY_TYPE       MemoryType,
  IN  EFI_PHYSICAL_ADDRESS  StartPageAddress,
  IN  UINTN                 Pages,
  OUT EFI_PHYSICAL_ADDRESS  *Result
  )
{
  EFI_STATUS                       Status;
  EFI_PHYSICAL_ADDRESS             MemPage;
  EFI_GCD_MEMORY_SPACE_DESCRIPTOR  MemDesc;

  //
  // Allocate Pages of memory less <= StartPageAddress
  //
  MemPage = (EFI_PHYSICAL_ADDRESS)(UINTN)StartPageAddress;
  Status  = gBS->AllocatePages (
                   AllocateType,
                   MemoryType,
                   Pages,
                   &MemPage
                   );
  //
  // Do not ASSERT on Status error but let caller decide since some cases
  // memory is already taken but that is ok.
  //
  if (!EFI_ERROR (Status)) {
    if (MemoryType != EfiBootServicesCode) {
      //
      // Make sure that the buffer can be used to store code.
      //
      Status = gDS->GetMemorySpaceDescriptor (MemPage, &MemDesc);
      if (!EFI_ERROR (Status) && ((MemDesc.Attributes & EFI_MEMORY_XP) != 0)) {
        Status = gDS->SetMemorySpaceAttributes (
                        MemPage,
                        EFI_PAGES_TO_SIZE (Pages),
                        MemDesc.Attributes & (~EFI_MEMORY_XP)
                        );
      }

      if (EFI_ERROR (Status)) {
        gBS->FreePages (MemPage, Pages);
      }
    }
  }

  if (!EFI_ERROR (Status)) {
    *Result = (EFI_PHYSICAL_ADDRESS)(UINTN)MemPage;
  }

  return Status;
}

/**
  This function is called when EFI needs to reserve an area in the 0xE0000 or 0xF0000
  64 KB blocks.

  Note: inconsistency with the Framework CSM spec. Per the spec, this function may be
  invoked only once. This limitation is relaxed to allow multiple calls in this implementation.

  @param  This                       Protocol instance pointer.
  @param  LegacyMemorySize           Size of required region
  @param  Region                     Region to use. 00 = Either 0xE0000 or 0xF0000
                                     block Bit0 = 1 0xF0000 block Bit1 = 1 0xE0000
                                     block
  @param  Alignment                  Address alignment. Bit mapped. First non-zero
                                     bit from right is alignment.
  @param  LegacyMemoryAddress        Region Assigned

  @retval EFI_SUCCESS                Region assigned
  @retval EFI_ACCESS_DENIED          Procedure previously invoked
  @retval Other                      Region not assigned

**/
EFI_STATUS
EFIAPI
LegacyBiosGetLegacyRegion (
  IN    EFI_LEGACY_BIOS_PROTOCOL  *This,
  IN    UINTN                     LegacyMemorySize,
  IN    UINTN                     Region,
  IN    UINTN                     Alignment,
  OUT   VOID                      **LegacyMemoryAddress
  )
{
  LEGACY_BIOS_INSTANCE   *Private;
  EFI_IA32_REGISTER_SET  Regs;
  EFI_STATUS             Status;
  UINT32                 Granularity;

  Private = LEGACY_BIOS_INSTANCE_FROM_THIS (This);
  Private->LegacyRegion->UnLock (Private->LegacyRegion, 0xE0000, 0x20000, &Granularity);

  ZeroMem (&Regs, sizeof (EFI_IA32_REGISTER_SET));
  Regs.X.AX = Legacy16GetTableAddress;
  Regs.X.BX = (UINT16)Region;
  Regs.X.CX = (UINT16)LegacyMemorySize;
  Regs.X.DX = (UINT16)Alignment;
  Private->LegacyBios.FarCall86 (
                        &Private->LegacyBios,
                        Private->Legacy16CallSegment,
                        Private->Legacy16CallOffset,
                        &Regs,
                        NULL,
                        0
                        );

  if (Regs.X.AX == 0) {
    *LegacyMemoryAddress = (VOID *)(((UINTN)Regs.X.DS << 4) + Regs.X.BX);
    Status               = EFI_SUCCESS;
  } else {
    Status = EFI_OUT_OF_RESOURCES;
  }

  Private->Cpu->FlushDataCache (Private->Cpu, 0xE0000, 0x20000, EfiCpuFlushTypeWriteBackInvalidate);
  Private->LegacyRegion->Lock (Private->LegacyRegion, 0xE0000, 0x20000, &Granularity);

  return Status;
}

/**
  This function is called when copying data to the region assigned by
  EFI_LEGACY_BIOS_PROTOCOL.GetLegacyRegion().

  @param  This                       Protocol instance pointer.
  @param  LegacyMemorySize           Size of data to copy
  @param  LegacyMemoryAddress        Legacy Region destination address Note: must
                                     be in region assigned by
                                     LegacyBiosGetLegacyRegion
  @param  LegacyMemorySourceAddress  Source of data

  @retval EFI_SUCCESS                The data was copied successfully.
  @retval EFI_ACCESS_DENIED          Either the starting or ending address is out of bounds.
**/
EFI_STATUS
EFIAPI
LegacyBiosCopyLegacyRegion (
  IN EFI_LEGACY_BIOS_PROTOCOL  *This,
  IN    UINTN                  LegacyMemorySize,
  IN    VOID                   *LegacyMemoryAddress,
  IN    VOID                   *LegacyMemorySourceAddress
  )
{
  LEGACY_BIOS_INSTANCE  *Private;
  UINT32                Granularity;

  if ((LegacyMemoryAddress < (VOID *)(UINTN)0xE0000) ||
      ((UINTN)LegacyMemoryAddress + LegacyMemorySize > (UINTN)0x100000)
      )
  {
    return EFI_ACCESS_DENIED;
  }

  //
  // There is no protection from writes over lapping if this function is
  // called multiple times.
  //
  Private = LEGACY_BIOS_INSTANCE_FROM_THIS (This);
  Private->LegacyRegion->UnLock (Private->LegacyRegion, 0xE0000, 0x20000, &Granularity);
  CopyMem (LegacyMemoryAddress, LegacyMemorySourceAddress, LegacyMemorySize);

  Private->Cpu->FlushDataCache (Private->Cpu, 0xE0000, 0x20000, EfiCpuFlushTypeWriteBackInvalidate);
  Private->LegacyRegion->Lock (Private->LegacyRegion, 0xE0000, 0x20000, &Granularity);

  return EFI_SUCCESS;
}

/**
  Find Legacy16 BIOS image in the FLASH device and shadow it into memory. Find
  the $EFI table in the shadow area. Thunk into the Legacy16 code after it had
  been shadowed.

  @param  Private                    Legacy BIOS context data

  @retval EFI_SUCCESS                Legacy16 code loaded
  @retval Other                      No protocol installed, unload driver.

**/
EFI_STATUS
ShadowAndStartLegacy16 (
  IN  LEGACY_BIOS_INSTANCE  *Private
  )
{
  EFI_STATUS                         Status;
  UINT8                              *Ptr;
  UINT8                              *PtrEnd;
  BOOLEAN                            Done;
  EFI_COMPATIBILITY16_TABLE          *Table;
  UINT8                              CheckSum;
  EFI_IA32_REGISTER_SET              Regs;
  EFI_TO_COMPATIBILITY16_INIT_TABLE  *EfiToLegacy16InitTable;
  EFI_TO_COMPATIBILITY16_BOOT_TABLE  *EfiToLegacy16BootTable;
  VOID                               *LegacyBiosImage;
  UINTN                              LegacyBiosImageSize;
  UINTN                              E820Size;
  UINT32                             *ClearPtr;
  BBS_TABLE                          *BbsTable;
  LEGACY_EFI_HDD_TABLE               *LegacyEfiHddTable;
  UINTN                              Index;
  UINT32                             TpmPointer;
  VOID                               *TpmBinaryImage;
  UINTN                              TpmBinaryImageSize;
  UINTN                              Location;
  UINTN                              Alignment;
  UINTN                              TempData;
  EFI_PHYSICAL_ADDRESS               Address;
  UINT16                             OldMask;
  UINT16                             NewMask;
  UINT32                             Granularity;
  EFI_GCD_MEMORY_SPACE_DESCRIPTOR    Descriptor;

  Location  = 0;
  Alignment = 0;

  //
  // we allocate the C/D/E/F segment as RT code so no one will use it any more.
  //
  Address = 0xC0000;
  gDS->GetMemorySpaceDescriptor (Address, &Descriptor);
  if (Descriptor.GcdMemoryType == EfiGcdMemoryTypeSystemMemory) {
    //
    // If it is already reserved, we should be safe, or else we allocate it.
    //
    Status = gBS->AllocatePages (
                    AllocateAddress,
                    EfiRuntimeServicesCode,
                    0x40000/EFI_PAGE_SIZE,
                    &Address
                    );
    if (EFI_ERROR (Status)) {
      //
      // Bugbug: need to figure out whether C/D/E/F segment should be marked as reserved memory.
      //
      DEBUG ((DEBUG_ERROR, "Failed to allocate the C/D/E/F segment Status = %r", Status));
    }
  }

  //
  // start testtest
  //    GetTimerValue (&Ticker);
  //
  //  gRT->SetVariable (L"StartLegacy",
  //                    &gEfiGlobalVariableGuid,
  //                    EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS,
  //                    sizeof (UINT64),
  //                    (VOID *)&Ticker
  //                    );
  // end testtest
  //
  EfiToLegacy16BootTable = &Private->IntThunk->EfiToLegacy16BootTable;
  Status                 = Private->LegacyBiosPlatform->GetPlatformInfo (
                                                          Private->LegacyBiosPlatform,
                                                          EfiGetPlatformBinarySystemRom,
                                                          &LegacyBiosImage,
                                                          &LegacyBiosImageSize,
                                                          &Location,
                                                          &Alignment,
                                                          0,
                                                          0
                                                          );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Private->BiosStart           = (UINT32)(0x100000 - LegacyBiosImageSize);
  Private->OptionRom           = 0xc0000;
  Private->LegacyBiosImageSize = (UINT32)LegacyBiosImageSize;

  //
  // Can only shadow into memory allocated for legacy usage.
  //
  ASSERT (Private->BiosStart > Private->OptionRom);

  //
  // Shadow Legacy BIOS. Turn on memory and copy image
  //
  Private->LegacyRegion->UnLock (Private->LegacyRegion, 0xc0000, 0x40000, &Granularity);

  ClearPtr = (VOID *)((UINTN)0xc0000);

  //
  // Initialize region from 0xc0000 to start of BIOS to all ffs. This allows unused
  // regions to be used by EMM386 etc.
  //
  SetMem ((VOID *)ClearPtr, (UINTN)(0x40000 - LegacyBiosImageSize), 0xff);

  TempData = Private->BiosStart;

  CopyMem (
    (VOID *)TempData,
    LegacyBiosImage,
    (UINTN)LegacyBiosImageSize
    );

  Private->Cpu->FlushDataCache (Private->Cpu, 0xc0000, 0x40000, EfiCpuFlushTypeWriteBackInvalidate);

  //
  // Search for Legacy16 table in Shadowed ROM
  //
  Done  = FALSE;
  Table = NULL;
  for (Ptr = (UINT8 *)TempData; Ptr < (UINT8 *)((UINTN)0x100000) && !Done; Ptr += 0x10) {
    if (*(UINT32 *)Ptr == SIGNATURE_32 ('I', 'F', 'E', '$')) {
      Table  = (EFI_COMPATIBILITY16_TABLE *)Ptr;
      PtrEnd = Ptr + Table->TableLength;
      for (CheckSum = 0; Ptr < PtrEnd; Ptr++) {
        CheckSum = (UINT8)(CheckSum +*Ptr);
      }

      Done = TRUE;
    }
  }

  if (Table == NULL) {
    DEBUG ((DEBUG_ERROR, "No Legacy16 table found\n"));
    return EFI_NOT_FOUND;
  }

  if (!Done) {
    //
    // Legacy16 table header checksum error.
    //
    DEBUG ((DEBUG_ERROR, "Legacy16 table found with bad talbe header checksum\n"));
  }

  //
  // Remember location of the Legacy16 table
  //
  Private->Legacy16Table           = Table;
  Private->Legacy16CallSegment     = Table->Compatibility16CallSegment;
  Private->Legacy16CallOffset      = Table->Compatibility16CallOffset;
  EfiToLegacy16InitTable           = &Private->IntThunk->EfiToLegacy16InitTable;
  Private->Legacy16InitPtr         = EfiToLegacy16InitTable;
  Private->Legacy16BootPtr         = &Private->IntThunk->EfiToLegacy16BootTable;
  Private->InternalIrqRoutingTable = NULL;
  Private->NumberIrqRoutingEntries = 0;
  Private->BbsTablePtr             = NULL;
  Private->LegacyEfiHddTable       = NULL;
  Private->DiskEnd                 = 0;
  Private->Disk4075                = 0;
  Private->HddTablePtr             = &Private->IntThunk->EfiToLegacy16BootTable.HddInfo;
  Private->NumberHddControllers    = MAX_IDE_CONTROLLER;
  Private->Dump[0]                 = 'D';
  Private->Dump[1]                 = 'U';
  Private->Dump[2]                 = 'M';
  Private->Dump[3]                 = 'P';

  ZeroMem (
    Private->Legacy16BootPtr,
    sizeof (EFI_TO_COMPATIBILITY16_BOOT_TABLE)
    );

  //
  // Store away a copy of the EFI System Table
  //
  Table->EfiSystemTable = (UINT32)(UINTN)gST;

  //
  // IPF CSM integration -Bug
  //
  // Construct the Legacy16 boot memory map. This sets up number of
  // E820 entries.
  //
  LegacyBiosBuildE820 (Private, &E820Size);
  //
  // Initialize BDA and EBDA standard values needed to load Legacy16 code
  //
  LegacyBiosInitBda (Private);
  LegacyBiosInitCmos (Private);

  //
  // All legacy interrupt should be masked when do initialization work from legacy 16 code.
  //
  Private->Legacy8259->GetMask (Private->Legacy8259, &OldMask, NULL, NULL, NULL);
  NewMask = 0xFFFF;
  Private->Legacy8259->SetMask (Private->Legacy8259, &NewMask, NULL, NULL, NULL);

  //
  // Call into Legacy16 code to do an INIT
  //
  ZeroMem (&Regs, sizeof (EFI_IA32_REGISTER_SET));
  Regs.X.AX = Legacy16InitializeYourself;
  Regs.X.ES = EFI_SEGMENT (*((UINT32 *)&EfiToLegacy16InitTable));
  Regs.X.BX = EFI_OFFSET (*((UINT32 *)&EfiToLegacy16InitTable));

  Private->LegacyBios.FarCall86 (
                        &Private->LegacyBios,
                        Table->Compatibility16CallSegment,
                        Table->Compatibility16CallOffset,
                        &Regs,
                        NULL,
                        0
                        );

  //
  // Restore original legacy interrupt mask value
  //
  Private->Legacy8259->SetMask (Private->Legacy8259, &OldMask, NULL, NULL, NULL);

  if (Regs.X.AX != 0) {
    return EFI_DEVICE_ERROR;
  }

  //
  // start testtest
  //  GetTimerValue (&Ticker);
  //
  //  gRT->SetVariable (L"BackFromInitYourself",
  //                    &gEfiGlobalVariableGuid,
  //                    EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS,
  //                    sizeof (UINT64),
  //                    (VOID *)&Ticker
  //                    );
  // end testtest
  //
  // Copy E820 table after InitializeYourself is completed
  //
  ZeroMem (&Regs, sizeof (EFI_IA32_REGISTER_SET));
  Regs.X.AX = Legacy16GetTableAddress;
  Regs.X.CX = (UINT16)E820Size;
  Regs.X.DX = 1;
  Private->LegacyBios.FarCall86 (
                        &Private->LegacyBios,
                        Table->Compatibility16CallSegment,
                        Table->Compatibility16CallOffset,
                        &Regs,
                        NULL,
                        0
                        );

  Table->E820Pointer = (UINT32)(Regs.X.DS * 16 + Regs.X.BX);
  Table->E820Length  = (UINT32)E820Size;
  if (Regs.X.AX != 0) {
    DEBUG ((DEBUG_ERROR, "Legacy16 E820 length insufficient\n"));
  } else {
    TempData = Table->E820Pointer;
    CopyMem ((VOID *)TempData, Private->E820Table, E820Size);
  }

  //
  // Get PnPInstallationCheck Info.
  //
  Private->PnPInstallationCheckSegment = Table->PnPInstallationCheckSegment;
  Private->PnPInstallationCheckOffset  = Table->PnPInstallationCheckOffset;

  //
  // Check if PCI Express is supported. If yes, Save base address.
  //
  Status = Private->LegacyBiosPlatform->GetPlatformInfo (
                                          Private->LegacyBiosPlatform,
                                          EfiGetPlatformPciExpressBase,
                                          NULL,
                                          NULL,
                                          &Location,
                                          &Alignment,
                                          0,
                                          0
                                          );
  if (!EFI_ERROR (Status)) {
    Private->Legacy16Table->PciExpressBase = (UINT32)Location;
    Location                               = 0;
  }

  //
  // Check if TPM is supported. If yes get a region in E0000,F0000 to copy it
  // into, copy it and update pointer to binary image. This needs to be
  // done prior to any OPROM for security purposes.
  //
  Status = Private->LegacyBiosPlatform->GetPlatformInfo (
                                          Private->LegacyBiosPlatform,
                                          EfiGetPlatformBinaryTpmBinary,
                                          &TpmBinaryImage,
                                          &TpmBinaryImageSize,
                                          &Location,
                                          &Alignment,
                                          0,
                                          0
                                          );
  if (!EFI_ERROR (Status)) {
    ZeroMem (&Regs, sizeof (EFI_IA32_REGISTER_SET));
    Regs.X.AX = Legacy16GetTableAddress;
    Regs.X.CX = (UINT16)TpmBinaryImageSize;
    Regs.X.DX = 1;
    Private->LegacyBios.FarCall86 (
                          &Private->LegacyBios,
                          Table->Compatibility16CallSegment,
                          Table->Compatibility16CallOffset,
                          &Regs,
                          NULL,
                          0
                          );

    TpmPointer = (UINT32)(Regs.X.DS * 16 + Regs.X.BX);
    if (Regs.X.AX != 0) {
      DEBUG ((DEBUG_ERROR, "TPM cannot be loaded\n"));
    } else {
      CopyMem ((VOID *)(UINTN)TpmPointer, TpmBinaryImage, TpmBinaryImageSize);
      Table->TpmSegment = Regs.X.DS;
      Table->TpmOffset  = Regs.X.BX;
    }
  }

  //
  // Lock the Legacy BIOS region
  //
  Private->Cpu->FlushDataCache (Private->Cpu, Private->BiosStart, (UINT32)LegacyBiosImageSize, EfiCpuFlushTypeWriteBackInvalidate);
  Private->LegacyRegion->Lock (Private->LegacyRegion, Private->BiosStart, (UINT32)LegacyBiosImageSize, &Granularity);

  //
  // Get the BbsTable from LOW_MEMORY_THUNK
  //
  BbsTable = (BBS_TABLE *)(UINTN)Private->IntThunk->BbsTable;
  ZeroMem ((VOID *)BbsTable, sizeof (Private->IntThunk->BbsTable));

  EfiToLegacy16BootTable->BbsTable = (UINT32)(UINTN)BbsTable;
  Private->BbsTablePtr             = (VOID *)BbsTable;

  //
  // Populate entire table with BBS_IGNORE_ENTRY
  //
  EfiToLegacy16BootTable->NumberBbsEntries = MAX_BBS_ENTRIES;

  for (Index = 0; Index < MAX_BBS_ENTRIES; Index++) {
    BbsTable[Index].BootPriority = BBS_IGNORE_ENTRY;
  }

  //
  // Allocate space for Legacy HDD table
  //
  LegacyEfiHddTable = (LEGACY_EFI_HDD_TABLE *)AllocateZeroPool ((UINTN)MAX_HDD_ENTRIES * sizeof (LEGACY_EFI_HDD_TABLE));
  ASSERT (LegacyEfiHddTable);

  Private->LegacyEfiHddTable      = LegacyEfiHddTable;
  Private->LegacyEfiHddTableIndex = 0x00;

  //
  // start testtest
  //  GetTimerValue (&Ticker);
  //
  //  gRT->SetVariable (L"EndOfLoadFv",
  //                    &gEfiGlobalVariableGuid,
  //                    EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS,
  //                    sizeof (UINT64),
  //                    (VOID *)&Ticker
  //                    );
  // end testtest
  //
  return EFI_SUCCESS;
}

/**
  Shadow all legacy16 OPROMs that haven't been shadowed.
  Warning: Use this with caution. This routine disconnects all EFI
  drivers. If used externally then caller must re-connect EFI
  drivers.

  @param  This                    Protocol instance pointer.

  @retval EFI_SUCCESS             OPROMs shadowed

**/
EFI_STATUS
EFIAPI
LegacyBiosShadowAllLegacyOproms (
  IN EFI_LEGACY_BIOS_PROTOCOL  *This
  )
{
  LEGACY_BIOS_INSTANCE  *Private;

  //
  //  EFI_LEGACY_BIOS_PLATFORM_PROTOCOL    *LegacyBiosPlatform;
  //  EFI_LEGACY16_TABLE                   *Legacy16Table;
  //
  Private = LEGACY_BIOS_INSTANCE_FROM_THIS (This);

  //
  //  LegacyBiosPlatform       = Private->LegacyBiosPlatform;
  //  Legacy16Table            = Private->Legacy16Table;
  //
  // Shadow PCI ROMs. We must do this near the end since this will kick
  // of Native EFI drivers that may be needed to collect info for Legacy16
  //
  //  WARNING: PciIo is gone after this call.
  //
  PciProgramAllInterruptLineRegisters (Private);

  PciShadowRoms (Private);

  //
  // Shadow PXE base code, BIS etc.
  //
  //  LegacyBiosPlatform->ShadowServiceRoms (LegacyBiosPlatform,
  //                       &Private->OptionRom,
  //                       Legacy16Table);
  //
  return EFI_SUCCESS;
}

/**
  Get the PCI BIOS interface version.

  @param  Private  Driver private data.

  @return The PCI interface version number in Binary Coded Decimal (BCD) format.
          E.g.: 0x0210 indicates 2.10, 0x0300 indicates 3.00

**/
UINT16
GetPciInterfaceVersion (
  IN LEGACY_BIOS_INSTANCE  *Private
  )
{
  EFI_IA32_REGISTER_SET  Reg;
  BOOLEAN                ThunkFailed;
  UINT16                 PciInterfaceVersion;

  PciInterfaceVersion = 0;

  Reg.X.AX  = 0xB101;
  Reg.E.EDI = 0;

  ThunkFailed = Private->LegacyBios.Int86 (&Private->LegacyBios, 0x1A, &Reg);
  if (!ThunkFailed) {
    //
    // From PCI Firmware 3.0 Specification:
    //   If the CARRY FLAG [CF] is cleared and AH is set to 00h, it is still necessary to examine the
    //   contents of [EDX] for the presence of the string "PCI" + (trailing space) to fully validate the
    //   presence of the PCI function set. [BX] will further indicate the version level, with enough
    //   granularity to allow for incremental changes in the code that don't affect the function interface.
    //   Version numbers are stored as Binary Coded Decimal (BCD) values. For example, Version 2.10
    //   would be returned as a 02h in the [BH] registers and 10h in the [BL] registers.
    //
    if ((Reg.X.Flags.CF == 0) && (Reg.H.AH == 0) && (Reg.E.EDX == SIGNATURE_32 ('P', 'C', 'I', ' '))) {
      PciInterfaceVersion = Reg.X.BX;
    }
  }

  return PciInterfaceVersion;
}

/**
  Callback function to calculate SMBIOS table size, and allocate memory for SMBIOS table.
  SMBIOS table will be copied into EfiReservedMemoryType memory in legacy boot path.

  @param  Event                 Event whose notification function is being invoked.
  @param  Context               The pointer to the notification function's context,
                                which is implementation-dependent.

**/
VOID
EFIAPI
InstallSmbiosEventCallback (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  EFI_STATUS                Status;
  SMBIOS_TABLE_ENTRY_POINT  *EntryPointStructure;

  //
  // Get SMBIOS table from EFI configuration table
  //
  Status = EfiGetSystemConfigurationTable (
             &gEfiSmbiosTableGuid,
             &mRuntimeSmbiosEntryPoint
             );
  if ((EFI_ERROR (Status)) || (mRuntimeSmbiosEntryPoint == NULL)) {
    return;
  }

  EntryPointStructure = (SMBIOS_TABLE_ENTRY_POINT *)mRuntimeSmbiosEntryPoint;

  //
  // Allocate memory for SMBIOS Entry Point Structure.
  // CSM framework spec requires SMBIOS table below 4GB in EFI_TO_COMPATIBILITY16_BOOT_TABLE.
  //
  if (mReserveSmbiosEntryPoint == 0) {
    //
    // Entrypoint structure with fixed size is allocated only once.
    //
    mReserveSmbiosEntryPoint = SIZE_4GB - 1;
    Status                   = gBS->AllocatePages (
                                      AllocateMaxAddress,
                                      EfiReservedMemoryType,
                                      EFI_SIZE_TO_PAGES ((UINTN)(EntryPointStructure->EntryPointLength)),
                                      &mReserveSmbiosEntryPoint
                                      );
    if (EFI_ERROR (Status)) {
      mReserveSmbiosEntryPoint = 0;
      return;
    }

    DEBUG ((DEBUG_INFO, "Allocate memory for Smbios Entry Point Structure\n"));
  }

  if ((mStructureTableAddress != 0) &&
      (mStructureTablePages < EFI_SIZE_TO_PAGES ((UINT32)EntryPointStructure->TableLength)))
  {
    //
    // If original buffer is not enough for the new SMBIOS table, free original buffer and re-allocate
    //
    gBS->FreePages (mStructureTableAddress, mStructureTablePages);
    mStructureTableAddress = 0;
    mStructureTablePages   = 0;
    DEBUG ((DEBUG_INFO, "Original size is not enough. Re-allocate the memory.\n"));
  }

  if (mStructureTableAddress == 0) {
    //
    // Allocate reserved memory below 4GB.
    // Smbios spec requires the structure table is below 4GB.
    //
    mStructureTableAddress = SIZE_4GB - 1;
    mStructureTablePages   = EFI_SIZE_TO_PAGES (EntryPointStructure->TableLength);
    Status                 = gBS->AllocatePages (
                                    AllocateMaxAddress,
                                    EfiReservedMemoryType,
                                    mStructureTablePages,
                                    &mStructureTableAddress
                                    );
    if (EFI_ERROR (Status)) {
      gBS->FreePages (
             mReserveSmbiosEntryPoint,
             EFI_SIZE_TO_PAGES ((UINTN)(EntryPointStructure->EntryPointLength))
             );
      mReserveSmbiosEntryPoint = 0;
      mStructureTableAddress   = 0;
      mStructureTablePages     = 0;
      return;
    }

    DEBUG ((DEBUG_INFO, "Allocate memory for Smbios Structure Table\n"));
  }
}

/**
  Callback function to toggle EndOfDxe status. NULL pointer detection needs
  this status to decide if it's necessary to change attributes of page 0.

  @param  Event            Event whose notification function is being invoked.
  @param  Context          The pointer to the notification function's context,
                           which is implementation-dependent.

**/
VOID
EFIAPI
ToggleEndOfDxeStatus (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  mEndOfDxe = TRUE;
  return;
}

/**
  Install Driver to produce Legacy BIOS protocol.

  @param  ImageHandle  Handle of driver image.
  @param  SystemTable  Pointer to system table.

  @retval EFI_SUCCESS  Legacy BIOS protocol installed
  @retval No protocol installed, unload driver.

**/
EFI_STATUS
EFIAPI
LegacyBiosInstall (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS                         Status;
  LEGACY_BIOS_INSTANCE               *Private;
  EFI_TO_COMPATIBILITY16_INIT_TABLE  *EfiToLegacy16InitTable;
  EFI_PHYSICAL_ADDRESS               MemoryAddress;
  EFI_PHYSICAL_ADDRESS               EbdaReservedBaseAddress;
  VOID                               *MemoryPtr;
  EFI_PHYSICAL_ADDRESS               MemoryAddressUnder1MB;
  UINTN                              Index;
  UINT32                             *BaseVectorMaster;
  EFI_PHYSICAL_ADDRESS               StartAddress;
  UINT32                             *ClearPtr;
  EFI_PHYSICAL_ADDRESS               MemStart;
  UINT32                             IntRedirCode;
  UINT32                             Granularity;
  BOOLEAN                            DecodeOn;
  UINT32                             MemorySize;
  EFI_GCD_MEMORY_SPACE_DESCRIPTOR    Descriptor;
  UINT64                             Length;
  UINT8                              *SecureBoot;
  EFI_EVENT                          InstallSmbiosEvent;
  EFI_EVENT                          EndOfDxeEvent;

  //
  // Load this driver's image to memory
  //
  Status = RelocateImageUnder4GIfNeeded (ImageHandle, SystemTable);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // When UEFI Secure Boot is enabled, CSM module will not start any more.
  //
  SecureBoot = NULL;
  GetEfiGlobalVariable2 (EFI_SECURE_BOOT_MODE_NAME, (VOID **)&SecureBoot, NULL);
  if ((SecureBoot != NULL) && (*SecureBoot == SECURE_BOOT_MODE_ENABLE)) {
    FreePool (SecureBoot);
    return EFI_SECURITY_VIOLATION;
  }

  if (SecureBoot != NULL) {
    FreePool (SecureBoot);
  }

  Private = &mPrivateData;
  ZeroMem (Private, sizeof (LEGACY_BIOS_INSTANCE));

  //
  // Grab a copy of all the protocols we depend on. Any error would
  // be a dispatcher bug!.
  //
  Status = gBS->LocateProtocol (&gEfiCpuArchProtocolGuid, NULL, (VOID **)&Private->Cpu);
  ASSERT_EFI_ERROR (Status);

  Status = gBS->LocateProtocol (&gEfiTimerArchProtocolGuid, NULL, (VOID **)&Private->Timer);
  ASSERT_EFI_ERROR (Status);

  Status = gBS->LocateProtocol (&gEfiLegacyRegion2ProtocolGuid, NULL, (VOID **)&Private->LegacyRegion);
  ASSERT_EFI_ERROR (Status);

  Status = gBS->LocateProtocol (&gEfiLegacyBiosPlatformProtocolGuid, NULL, (VOID **)&Private->LegacyBiosPlatform);
  ASSERT_EFI_ERROR (Status);

  Status = gBS->LocateProtocol (&gEfiLegacy8259ProtocolGuid, NULL, (VOID **)&Private->Legacy8259);
  ASSERT_EFI_ERROR (Status);

  Status = gBS->LocateProtocol (&gEfiLegacyInterruptProtocolGuid, NULL, (VOID **)&Private->LegacyInterrupt);
  ASSERT_EFI_ERROR (Status);

  //
  // Locate Memory Test Protocol if exists
  //
  Status = gBS->LocateProtocol (
                  &gEfiGenericMemTestProtocolGuid,
                  NULL,
                  (VOID **)&Private->GenericMemoryTest
                  );
  ASSERT_EFI_ERROR (Status);

  //
  // Make sure all memory from 0-640K is tested
  //
  for (StartAddress = 0; StartAddress < 0xa0000; ) {
    gDS->GetMemorySpaceDescriptor (StartAddress, &Descriptor);
    if (Descriptor.GcdMemoryType != EfiGcdMemoryTypeReserved) {
      StartAddress = Descriptor.BaseAddress + Descriptor.Length;
      continue;
    }

    Length = MIN (Descriptor.Length, 0xa0000 - StartAddress);
    Private->GenericMemoryTest->CompatibleRangeTest (
                                  Private->GenericMemoryTest,
                                  StartAddress,
                                  Length
                                  );
    StartAddress = StartAddress + Length;
  }

  //
  // Make sure all memory from 1MB to 16MB is tested and added to memory map
  //
  for (StartAddress = BASE_1MB; StartAddress < BASE_16MB; ) {
    gDS->GetMemorySpaceDescriptor (StartAddress, &Descriptor);
    if (Descriptor.GcdMemoryType != EfiGcdMemoryTypeReserved) {
      StartAddress = Descriptor.BaseAddress + Descriptor.Length;
      continue;
    }

    Length = MIN (Descriptor.Length, BASE_16MB - StartAddress);
    Private->GenericMemoryTest->CompatibleRangeTest (
                                  Private->GenericMemoryTest,
                                  StartAddress,
                                  Length
                                  );
    StartAddress = StartAddress + Length;
  }

  Private->Signature = LEGACY_BIOS_INSTANCE_SIGNATURE;

  Private->LegacyBios.Int86                    = LegacyBiosInt86;
  Private->LegacyBios.FarCall86                = LegacyBiosFarCall86;
  Private->LegacyBios.CheckPciRom              = LegacyBiosCheckPciRom;
  Private->LegacyBios.InstallPciRom            = LegacyBiosInstallPciRom;
  Private->LegacyBios.LegacyBoot               = LegacyBiosLegacyBoot;
  Private->LegacyBios.UpdateKeyboardLedStatus  = LegacyBiosUpdateKeyboardLedStatus;
  Private->LegacyBios.GetBbsInfo               = LegacyBiosGetBbsInfo;
  Private->LegacyBios.ShadowAllLegacyOproms    = LegacyBiosShadowAllLegacyOproms;
  Private->LegacyBios.PrepareToBootEfi         = LegacyBiosPrepareToBootEfi;
  Private->LegacyBios.GetLegacyRegion          = LegacyBiosGetLegacyRegion;
  Private->LegacyBios.CopyLegacyRegion         = LegacyBiosCopyLegacyRegion;
  Private->LegacyBios.BootUnconventionalDevice = LegacyBiosBootUnconventionalDevice;

  Private->ImageHandle = ImageHandle;

  //
  // Enable read attribute of legacy region.
  //
  DecodeOn = TRUE;
  Private->LegacyRegion->Decode (
                           Private->LegacyRegion,
                           0xc0000,
                           0x40000,
                           &Granularity,
                           &DecodeOn
                           );
  //
  // Set Cachebility for legacy region
  // BUGBUG: Comments about this legacy region cacheability setting
  //         This setting will make D865GCHProduction CSM Unhappy
  //
  if (PcdGetBool (PcdLegacyBiosCacheLegacyRegion)) {
    gDS->SetMemorySpaceAttributes (
           0x0,
           0xA0000,
           EFI_MEMORY_WB
           );
    gDS->SetMemorySpaceAttributes (
           0xc0000,
           0x40000,
           EFI_MEMORY_WB
           );
  }

  gDS->SetMemorySpaceAttributes (
         0xA0000,
         0x20000,
         EFI_MEMORY_UC
         );

  //
  // Allocate 0 - 4K for real mode interrupt vectors and BDA.
  //
  AllocateLegacyMemory (
    AllocateAddress,
    EfiReservedMemoryType,
    0,
    1,
    &MemoryAddress
    );
  ASSERT (MemoryAddress == 0x000000000);

  ClearPtr = (VOID *)((UINTN)0x0000);

  //
  // Initialize region from 0x0000 to 4k. This initializes interrupt vector
  // range.
  //
  ACCESS_PAGE0_CODE (
    gBS->SetMem ((VOID *)ClearPtr, 0x400, INITIAL_VALUE_BELOW_1K);
    ZeroMem ((VOID *)((UINTN)ClearPtr + 0x400), 0xC00);
    );

  //
  // Allocate pages for OPROM usage
  //
  MemorySize = PcdGet32 (PcdEbdaReservedMemorySize);
  ASSERT ((MemorySize & 0xFFF) == 0);

  Status = AllocateLegacyMemory (
             AllocateAddress,
             EfiReservedMemoryType,
             CONVENTIONAL_MEMORY_TOP - MemorySize,
             EFI_SIZE_TO_PAGES (MemorySize),
             &MemoryAddress
             );
  ASSERT_EFI_ERROR (Status);

  ZeroMem ((VOID *)((UINTN)MemoryAddress), MemorySize);

  //
  // Allocate all 32k chunks from 0x60000 ~ 0x88000 for Legacy OPROMs that
  // don't use PMM but look for zeroed memory. Note that various non-BBS
  // OpROMs expect different areas to be free
  //
  EbdaReservedBaseAddress = MemoryAddress;
  MemoryAddress           = PcdGet32 (PcdOpromReservedMemoryBase);
  MemorySize              = PcdGet32 (PcdOpromReservedMemorySize);
  //
  // Check if base address and size for reserved memory are 4KB aligned.
  //
  ASSERT ((MemoryAddress & 0xFFF) == 0);
  ASSERT ((MemorySize & 0xFFF) == 0);
  //
  // Check if the reserved memory is below EBDA reserved range.
  //
  ASSERT ((MemoryAddress < EbdaReservedBaseAddress) && ((MemoryAddress + MemorySize - 1) < EbdaReservedBaseAddress));
  for (MemStart = MemoryAddress; MemStart < MemoryAddress + MemorySize; MemStart += 0x1000) {
    Status = AllocateLegacyMemory (
               AllocateAddress,
               EfiBootServicesCode,
               MemStart,
               1,
               &StartAddress
               );
    if (!EFI_ERROR (Status)) {
      MemoryPtr = (VOID *)((UINTN)StartAddress);
      ZeroMem (MemoryPtr, 0x1000);
    } else {
      DEBUG ((DEBUG_ERROR, "WARNING: Allocate legacy memory fail for SCSI card - %x\n", MemStart));
    }
  }

  //
  // Allocate low PMM memory and zero it out
  //
  MemorySize = PcdGet32 (PcdLowPmmMemorySize);
  ASSERT ((MemorySize & 0xFFF) == 0);
  Status = AllocateLegacyMemory (
             AllocateMaxAddress,
             EfiBootServicesCode,
             CONVENTIONAL_MEMORY_TOP,
             EFI_SIZE_TO_PAGES (MemorySize),
             &MemoryAddressUnder1MB
             );
  ASSERT_EFI_ERROR (Status);

  ZeroMem ((VOID *)((UINTN)MemoryAddressUnder1MB), MemorySize);

  //
  // Allocate space for thunker and Init Thunker
  //
  Status = AllocateLegacyMemory (
             AllocateMaxAddress,
             EfiReservedMemoryType,
             CONVENTIONAL_MEMORY_TOP,
             (sizeof (LOW_MEMORY_THUNK) / EFI_PAGE_SIZE) + 2,
             &MemoryAddress
             );
  ASSERT_EFI_ERROR (Status);
  Private->IntThunk                        = (LOW_MEMORY_THUNK *)(UINTN)MemoryAddress;
  EfiToLegacy16InitTable                   = &Private->IntThunk->EfiToLegacy16InitTable;
  EfiToLegacy16InitTable->ThunkStart       = (UINT32)(EFI_PHYSICAL_ADDRESS)(UINTN)MemoryAddress;
  EfiToLegacy16InitTable->ThunkSizeInBytes = (UINT32)(sizeof (LOW_MEMORY_THUNK));

  Status = LegacyBiosInitializeThunk (Private);
  ASSERT_EFI_ERROR (Status);

  //
  // Init the legacy memory map in memory < 1 MB.
  //
  EfiToLegacy16InitTable->BiosLessThan1MB         = (UINT32)MemoryAddressUnder1MB;
  EfiToLegacy16InitTable->LowPmmMemory            = (UINT32)MemoryAddressUnder1MB;
  EfiToLegacy16InitTable->LowPmmMemorySizeInBytes = MemorySize;

  MemorySize = PcdGet32 (PcdHighPmmMemorySize);
  ASSERT ((MemorySize & 0xFFF) == 0);
  //
  // Allocate high PMM Memory under 16 MB
  //
  Status = AllocateLegacyMemory (
             AllocateMaxAddress,
             EfiBootServicesCode,
             0x1000000,
             EFI_SIZE_TO_PAGES (MemorySize),
             &MemoryAddress
             );
  if (EFI_ERROR (Status)) {
    //
    // If it fails, allocate high PMM Memory under 4GB
    //
    Status = AllocateLegacyMemory (
               AllocateMaxAddress,
               EfiBootServicesCode,
               0xFFFFFFFF,
               EFI_SIZE_TO_PAGES (MemorySize),
               &MemoryAddress
               );
  }

  if (!EFI_ERROR (Status)) {
    EfiToLegacy16InitTable->HiPmmMemory            = (UINT32)(EFI_PHYSICAL_ADDRESS)(UINTN)MemoryAddress;
    EfiToLegacy16InitTable->HiPmmMemorySizeInBytes = MemorySize;
  }

  //
  //  ShutdownAPs();
  //
  // Start the Legacy BIOS;
  //
  Status = ShadowAndStartLegacy16 (Private);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Initialize interrupt redirection code and entries;
  // IDT Vectors 0x68-0x6f must be redirected to IDT Vectors 0x08-0x0f.
  //
  CopyMem (
    Private->IntThunk->InterruptRedirectionCode,
    (VOID *)(UINTN)InterruptRedirectionTemplate,
    sizeof (Private->IntThunk->InterruptRedirectionCode)
    );

  //
  // Save Unexpected interrupt vector so can restore it just prior to boot
  //
  ACCESS_PAGE0_CODE (
    BaseVectorMaster           = (UINT32 *)(sizeof (UINT32) * PROTECTED_MODE_BASE_VECTOR_MASTER);
    Private->BiosUnexpectedInt = BaseVectorMaster[0];
    IntRedirCode               = (UINT32)(UINTN)Private->IntThunk->InterruptRedirectionCode;
    for (Index = 0; Index < 8; Index++) {
    BaseVectorMaster[Index] = (EFI_SEGMENT (IntRedirCode + Index * 4) << 16) | EFI_OFFSET (IntRedirCode + Index * 4);
  }

    );

  //
  // Save EFI value
  //
  Private->ThunkSeg = (UINT16)(EFI_SEGMENT (IntRedirCode));

  //
  // Allocate reserved memory for SMBIOS table used in legacy boot if SMBIOS table exists
  //
  InstallSmbiosEventCallback (NULL, NULL);

  //
  // Create callback function to update the size of reserved memory after LegacyBiosDxe starts
  //
  Status = gBS->CreateEventEx (
                  EVT_NOTIFY_SIGNAL,
                  TPL_NOTIFY,
                  InstallSmbiosEventCallback,
                  NULL,
                  &gEfiSmbiosTableGuid,
                  &InstallSmbiosEvent
                  );
  ASSERT_EFI_ERROR (Status);

  //
  // Create callback to update status of EndOfDxe, which is needed by NULL
  // pointer detection
  //
  Status = gBS->CreateEventEx (
                  EVT_NOTIFY_SIGNAL,
                  TPL_NOTIFY,
                  ToggleEndOfDxeStatus,
                  NULL,
                  &gEfiEndOfDxeEventGroupGuid,
                  &EndOfDxeEvent
                  );
  ASSERT_EFI_ERROR (Status);

  //
  // Make a new handle and install the protocol
  //
  Private->Handle = NULL;
  Status          = gBS->InstallProtocolInterface (
                           &Private->Handle,
                           &gEfiLegacyBiosProtocolGuid,
                           EFI_NATIVE_INTERFACE,
                           &Private->LegacyBios
                           );
  Private->Csm16PciInterfaceVersion = GetPciInterfaceVersion (Private);

  DEBUG ((
    DEBUG_INFO,
    "CSM16 PCI BIOS Interface Version: %02x.%02x\n",
    (UINT8)(Private->Csm16PciInterfaceVersion >> 8),
    (UINT8)Private->Csm16PciInterfaceVersion
    ));
  ASSERT (Private->Csm16PciInterfaceVersion != 0);
  return Status;
}
