/** @file

  Copyright (c) 2021, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "UefiPayloadEntry.h"
#include <Library/ElfLib.h>
#include <Elf32.h>
#include <Elf64.h>

#define MEMORY_ATTRIBUTE_MASK  (EFI_RESOURCE_ATTRIBUTE_PRESENT             |        \
                                       EFI_RESOURCE_ATTRIBUTE_INITIALIZED         | \
                                       EFI_RESOURCE_ATTRIBUTE_TESTED              | \
                                       EFI_RESOURCE_ATTRIBUTE_READ_PROTECTED      | \
                                       EFI_RESOURCE_ATTRIBUTE_WRITE_PROTECTED     | \
                                       EFI_RESOURCE_ATTRIBUTE_EXECUTION_PROTECTED | \
                                       EFI_RESOURCE_ATTRIBUTE_READ_ONLY_PROTECTED | \
                                       EFI_RESOURCE_ATTRIBUTE_16_BIT_IO           | \
                                       EFI_RESOURCE_ATTRIBUTE_32_BIT_IO           | \
                                       EFI_RESOURCE_ATTRIBUTE_64_BIT_IO           | \
                                       EFI_RESOURCE_ATTRIBUTE_PERSISTENT          )

#define TESTED_MEMORY_ATTRIBUTES  (EFI_RESOURCE_ATTRIBUTE_PRESENT     |     \
                                       EFI_RESOURCE_ATTRIBUTE_INITIALIZED | \
                                       EFI_RESOURCE_ATTRIBUTE_TESTED      )

extern VOID  *mHobList;

EFI_HOB_RESOURCE_DESCRIPTOR *
FindResourceDescriptorByRange (
  IN VOID                  *HobList,
  IN EFI_PHYSICAL_ADDRESS  Base,
  IN EFI_PHYSICAL_ADDRESS  Top
  );

EFI_HOB_RESOURCE_DESCRIPTOR *
FindAnotherHighestBelow4GResourceDescriptor (
  IN VOID                         *HobList,
  IN UINTN                        MinimalNeededSize,
  IN EFI_HOB_RESOURCE_DESCRIPTOR  *ExceptResourceHob
  );

BOOLEAN
IsHobNeed (
  EFI_PEI_HOB_POINTERS  Hob
  );

VOID
AddNewHob (
  IN EFI_PEI_HOB_POINTERS  *Hob
  );

#if FixedPcdGet8(PcdHandOffFdtEnable) == FALSE
/**
  It will build HOBs based on information from bootloaders.
  @param[in]  BootloaderParameter   The starting memory address of bootloader parameter block.
  @retval EFI_SUCCESS        If it completed successfully.
  @retval Others             If it failed to build required HOBs.
**/
EFI_STATUS
BuildHobs (
  IN  UINTN                       BootloaderParameter
  );
#endif

#if FixedPcdGet8(PcdHandOffFdtEnable) == TRUE
/**
  It will initialize HOBs for UPL.
  @param[in]  FdtBase        Address of the Fdt data.
  @retval EFI_SUCCESS        If it completed successfully.
  @retval Others             If it failed to initialize HOBs.
**/
UINT64
UplInitHob (
  IN VOID                         *FdtBase
  );

/**
  It will parse FDT based on DTB from bootloaders.
  @param[in]  FdtBase               Address of the Fdt data.
  @retval EFI_SUCCESS               If it completed successfully.
  @retval Others                    If it failed to parse DTB.
**/
UINTN
ParseDtb (
  IN VOID                           *FdtBase
  );
#endif

/**
  Print all HOBs info from the HOB list.

  @return The pointer to the HOB list.
**/
VOID
PrintHob (
  IN CONST VOID  *HobStart
  );

/**
  Retrieve DXE FV from Hob.
  @return .
**/
EFI_STATUS
GetDxeFv (
  OUT EFI_FIRMWARE_VOLUME_HEADER  **DxeFv
  )
{
  UINT8                         *GuidHob;
  UNIVERSAL_PAYLOAD_EXTRA_DATA  *ExtraData;
  EFI_HOB_FIRMWARE_VOLUME       *FvHob;

  //
  // Get DXE FV location
  //
  GuidHob = GetFirstGuidHob (&gUniversalPayloadExtraDataGuid);
  ASSERT (GuidHob != NULL);
  ExtraData = (UNIVERSAL_PAYLOAD_EXTRA_DATA *) GET_GUID_HOB_DATA (GuidHob);
  ASSERT (AsciiStrCmp (ExtraData->Entry[0].Identifier, "uefi_fv") == 0);

  *DxeFv = (EFI_FIRMWARE_VOLUME_HEADER *) (UINTN) ExtraData->Entry[0].Base;
  ASSERT ((*DxeFv)->FvLength == ExtraData->Entry[0].Size);

  //
  // Update DXE FV information to first fv hob in the hob list, which
  // is the empty FvHob created before.
  //
  FvHob              = GetFirstHob (EFI_HOB_TYPE_FV);
  FvHob->BaseAddress = (EFI_PHYSICAL_ADDRESS) (UINTN) *DxeFv;
  FvHob->Length      = (*DxeFv)->FvLength;

  return EFI_SUCCESS;
}

BOOLEAN
IfUPLFv( CHAR8 *SectionName)
{
   if (AsciiStrnCmp (SectionName, ".upld.uefi_fv", AsciiStrLen (".upld.uefi_fv")) == 0) {
      return TRUE;
   } else if (AsciiStrnCmp (SectionName, ".upld.network_fv", AsciiStrLen (".upld.network_fv")) == 0) {
      return TRUE;
   } else if (AsciiStrnCmp (SectionName, ".upld.bds_fv", AsciiStrLen (".upld.bds_fv")) == 0) {
      return TRUE;
   } else {
    return FALSE;
   }
}


/**
  Find extra FV sections from EFL image.

  @return .
**/
EFI_STATUS
FindExtraSection (
  IN  EFI_PHYSICAL_ADDRESS        ElfEntryPoint,
  OUT EFI_FIRMWARE_VOLUME_HEADER  **DxeFv
  )
{
  EFI_STATUS               Status;
  ELF_IMAGE_CONTEXT        ElfCt;
  Elf32_Ehdr               *Elf32Hdr;
  Elf64_Ehdr               *Elf64Hdr;
  Elf32_Shdr               *Elf32Shdr;
  Elf64_Shdr               *Elf64Shdr;
  UINT32                   Index;
  UINT32                   ExtraDataCount;
  CHAR8                    *SectionName;
  UINTN                    Offset;
  UINTN                    Size;
  EFI_HOB_FIRMWARE_VOLUME  *FvHob;
  UINT8                    *GuidHob;
  UNIVERSAL_PAYLOAD_BASE   *PayloadBase;
  EFI_FIRMWARE_VOLUME_HEADER *UplFV = NULL;
  EFI_PHYSICAL_ADDRESS      BaseAddress;
  UINT64                    Length;

  GuidHob = GetFirstGuidHob (&gUniversalPayloadBaseGuid);
  if (GuidHob != NULL) {
    PayloadBase = (UNIVERSAL_PAYLOAD_BASE *) GET_GUID_HOB_DATA (GuidHob);
    ElfCt.FileBase = (UINT8 *) PayloadBase->Entry;
  }
  else {
    Status = GetDxeFv (DxeFv);
    FvHob              = GetFirstHob (EFI_HOB_TYPE_FV);
    FvHob->BaseAddress = (EFI_PHYSICAL_ADDRESS) (UINTN) *DxeFv;
    FvHob->Length      = (*DxeFv)->FvLength;

    return Status;
  }

  Elf32Hdr       = (Elf32_Ehdr *) ElfCt.FileBase;
  ElfCt.EiClass = Elf32Hdr->e_ident[EI_CLASS];
  if (ElfCt.EiClass == ELFCLASS32) {
    if ((Elf32Hdr->e_type != ET_EXEC) && (Elf32Hdr->e_type != ET_DYN)) {
      return (ElfCt.ParseStatus = EFI_UNSUPPORTED);
    }

    Elf32Shdr = (Elf32_Shdr *) GetElf32SectionByIndex (ElfCt.FileBase, Elf32Hdr->e_shstrndx);
    if (Elf32Shdr == NULL) {
      return (ElfCt.ParseStatus = EFI_UNSUPPORTED);
    }

    ElfCt.EntryPoint = (UINTN) Elf32Hdr->e_entry;
    ElfCt.ShNum      = Elf32Hdr->e_shnum;
    ElfCt.PhNum      = Elf32Hdr->e_phnum;
    ElfCt.ShStrLen   = Elf32Shdr->sh_size;
    ElfCt.ShStrOff   = Elf32Shdr->sh_offset;
  } else {
    Elf64Hdr = (Elf64_Ehdr *) Elf32Hdr;
    if ((Elf64Hdr->e_type != ET_EXEC) && (Elf64Hdr->e_type != ET_DYN)) {
      return (ElfCt.ParseStatus = EFI_UNSUPPORTED);
    }

    Elf64Shdr = (Elf64_Shdr *) GetElf64SectionByIndex (ElfCt.FileBase, Elf64Hdr->e_shstrndx);
    if (Elf64Shdr == NULL) {
      return (ElfCt.ParseStatus = EFI_UNSUPPORTED);
    }

    ElfCt.EntryPoint = (UINTN) Elf64Hdr->e_entry;
    ElfCt.ShNum      = Elf64Hdr->e_shnum;
    ElfCt.PhNum      = Elf64Hdr->e_phnum;
    ElfCt.ShStrLen   = (UINT32) Elf64Shdr->sh_size;
    ElfCt.ShStrOff   = (UINT32) Elf64Shdr->sh_offset;
  }

  ExtraDataCount = 0;
  for (Index = 0; Index < ElfCt.ShNum; Index++) {
  	DEBUG ((DEBUG_INFO, "%a #3\n", __func__));
    Status = GetElfSectionName (&ElfCt, Index, &SectionName);
    if (EFI_ERROR (Status)) {
      continue;
    }

    DEBUG ((DEBUG_INFO, "Payload Section[%d]: %a\n", Index, SectionName));
    //if (AsciiStrnCmp (SectionName, ".upld.uefi_fv", AsciiStrLen (".upld.uefi_fv")) == 0) {
    if (IfUPLFv (SectionName)) {


      Status = GetElfSectionPos (&ElfCt, Index, &Offset, &Size);
      if (!EFI_ERROR (Status)) {
        DEBUG ((DEBUG_INFO, "  - Found upld_fv section %x\n", (UINTN) (ElfCt.FileBase + Offset)));
        if(UplFV == NULL){
         *DxeFv = (EFI_FIRMWARE_VOLUME_HEADER *) (UINTN) (ElfCt.FileBase + Offset);
        } 
        UplFV = (EFI_FIRMWARE_VOLUME_HEADER *) (UINTN) (ElfCt.FileBase + Offset);
        BaseAddress = (EFI_PHYSICAL_ADDRESS) (UINTN) UplFV;
        Length      = UplFV->FvLength;
        BuildFvHob (BaseAddress, Length);
        DEBUG ((
          DEBUG_INFO,
          "UPL Multiple fv[%d], name :%a , Base=0x%x, size=0x%x\n",
           Index,
           SectionName,
           BaseAddress,
           Length
         ));
      }
    }
  }

/*  if (ExtraDataCount != 0) {
    for (Index = 0; Index < ElfCt.ShNum; Index++) {
      Status = GetElfSectionName (&ElfCt, Index, &SectionName);
      if (EFI_ERROR (Status)) {
        continue;
      }

      if (AsciiStrnCmp (SectionName, UNIVERSAL_PAYLOAD_EXTRA_SEC_NAME_PREFIX, UNIVERSAL_PAYLOAD_EXTRA_SEC_NAME_PREFIX_LENGTH) == 0) {
        Status = GetElfSectionPos (&ElfCt, Index, &Offset, &Size);
        if (!EFI_ERROR (Status)) {
          *DxeFv = (EFI_FIRMWARE_VOLUME_HEADER *) (UINTN) (ElfCt.FileBase + Offset);;
          FvHob              = GetFirstHob (EFI_HOB_TYPE_FV);
          FvHob->BaseAddress = (EFI_PHYSICAL_ADDRESS) (UINTN) *DxeFv;
          FvHob->Length      = (*DxeFv)->FvLength;
        }
      }
    }
  }
*/

  return Status;
}

/**
  Some bootloader may pass a pcd database, and UPL also contain a PCD database.
  Dxe PCD driver has the assumption that the two PCD database can be catenated and
  the local token number should be successive.
  This function will fix up the UPL PCD database to meet that assumption.

  @param[in]   DxeFv         The FV where to find the Universal PCD database.

  @retval EFI_SUCCESS        If it completed successfully.
  @retval other              Failed to fix up.
**/
EFI_STATUS
FixUpPcdDatabase (
  IN  EFI_FIRMWARE_VOLUME_HEADER  *DxeFv
  )
{
  EFI_STATUS           Status;
  EFI_FFS_FILE_HEADER  *FileHeader;
  VOID                 *PcdRawData;
  PEI_PCD_DATABASE     *PeiDatabase;
  PEI_PCD_DATABASE     *UplDatabase;
  EFI_HOB_GUID_TYPE    *GuidHob;
  DYNAMICEX_MAPPING    *ExMapTable;
  UINTN                Index;

  GuidHob = GetFirstGuidHob (&gPcdDataBaseHobGuid);
  if (GuidHob == NULL) {
    //
    // No fix-up is needed.
    //
    return EFI_SUCCESS;
  }

  PeiDatabase = (PEI_PCD_DATABASE *)GET_GUID_HOB_DATA (GuidHob);
  DEBUG ((DEBUG_INFO, "Find the Pei PCD data base, the total local token number is %d\n", PeiDatabase->LocalTokenCount));

  Status = FvFindFileByTypeGuid (DxeFv, EFI_FV_FILETYPE_DRIVER, PcdGetPtr (PcdPcdDriverFile), &FileHeader);
  ASSERT_EFI_ERROR (Status);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = FileFindSection (FileHeader, EFI_SECTION_RAW, &PcdRawData);
  ASSERT_EFI_ERROR (Status);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  UplDatabase = (PEI_PCD_DATABASE *)PcdRawData;
  ExMapTable  = (DYNAMICEX_MAPPING *)(UINTN)((UINTN)PcdRawData + UplDatabase->ExMapTableOffset);

  for (Index = 0; Index < UplDatabase->ExTokenCount; Index++) {
    ExMapTable[Index].TokenNumber += PeiDatabase->LocalTokenCount;
  }

  DEBUG ((DEBUG_INFO, "Fix up UPL PCD database successfully\n"));
  return EFI_SUCCESS;
}


/**
  Entry point to the C language phase of UEFI payload.

  @param[in]   BootloaderParameter    The starting address of bootloader parameter block.

  @retval      It will not return if SUCCESS, and return error when passing bootloader parameter.
**/
EFI_STATUS
EFIAPI
_ModuleEntryPoint (
  IN UINTN  BootloaderParameter
  )
{
  EFI_STATUS                     Status;
  PHYSICAL_ADDRESS               DxeCoreEntryPoint;
  EFI_PEI_HOB_POINTERS           Hob;
  EFI_FIRMWARE_VOLUME_HEADER     *DxeFv;
#if FixedPcdGet8 (PcdHandOffFdtEnable) TRUE
  PHYSICAL_ADDRESS               HobListPtr;
  UINT8                          *GuidHob;
  UNIVERSAL_PAYLOAD_DEVICE_TREE  *FdtHob;
  VOID                           *Fdt;
#endif   

  DxeFv    = NULL;
  // Call constructor with default values for all libraries
  ProcessLibraryConstructorList ();
  // Initialize floating point operating environment to be compliant with UEFI spec.
  InitializeFloatingPointUnits ();
  DEBUG ((DEBUG_INFO, "Entering Universal Payload...\n"));
  DEBUG ((DEBUG_INFO, "sizeof(UINTN) = 0x%x\n", sizeof (UINTN)));
  DEBUG ((DEBUG_INFO, "BootloaderParameter = 0x%x\n", BootloaderParameter));

#if FixedPcdGet8 (PcdHandOffFdtEnable) == FALSE
  mHobList = (VOID *) BootloaderParameter;

  DEBUG ((DEBUG_INFO, "Start build HOB...\n"));

  // Build HOB based on information from Bootloader
  Status = BuildHobs (BootloaderParameter);
  ASSERT_EFI_ERROR (Status);
#endif

#if FixedPcdGet8 (PcdHandOffFdtEnable) == TRUE

  DEBUG ((DEBUG_INFO, "Start parsing FDT...\n"));
  HobListPtr = UplInitHob ((VOID *) BootloaderParameter);
  mHobList = (VOID *)HobListPtr;

  GuidHob = GetFirstGuidHob (&gUniversalPayloadDeviceTreeGuid);
  ASSERT (GuidHob != NULL);
  FdtHob = (UNIVERSAL_PAYLOAD_DEVICE_TREE *) GET_GUID_HOB_DATA (GuidHob);

  Fdt = (VOID *) (FdtHob->DeviceTreeAddress);

  //
  // Found hob list node
  //
  if (HobListPtr != 0) {
    Status = BuildHobs (HobListPtr);
    ParseDtb ((VOID *) Fdt);
  }
#endif

  // Call constructor for all libraries again since hobs were built
  ProcessLibraryConstructorList ();

  FindExtraSection ((EFI_PHYSICAL_ADDRESS) (UINTN) _ModuleEntryPoint, &DxeFv);

  DEBUG_CODE (
    //
    // Dump the Hobs from boot loader
    //
    PrintHob (mHobList);
    );

  FixUpPcdDatabase (DxeFv);
  Status = UniversalLoadDxeCore (DxeFv, &DxeCoreEntryPoint);
  ASSERT_EFI_ERROR (Status);

  //
  // Mask off all legacy 8259 interrupt sources
  //
  IoWrite8 (LEGACY_8259_MASK_REGISTER_MASTER, 0xFF);
  IoWrite8 (LEGACY_8259_MASK_REGISTER_SLAVE, 0xFF);

  Hob.HandoffInformationTable = (EFI_HOB_HANDOFF_INFO_TABLE *) GetFirstHob (EFI_HOB_TYPE_HANDOFF);
  HandOffToDxeCore (DxeCoreEntryPoint, Hob);

  // Should not get here
  CpuDeadLoop ();
  return EFI_SUCCESS;
}
