/** @file
Elf32 Convert solution

Copyright (c) 2010 - 2021, Intel Corporation. All rights reserved.<BR>
Portions copyright (c) 2013, ARM Ltd. All rights reserved.<BR>
Portions Copyright (c) 2020, Hewlett Packard Enterprise Development LP. All rights reserved.<BR>

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "WinNtInclude.h"

#ifndef __GNUC__
#include <windows.h>
#include <io.h>
#endif
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>

#include <Common/UefiBaseTypes.h>
#include <IndustryStandard/PeImage.h>

#include "PeCoffLib.h"
#include "EfiUtilityMsgs.h"

#include "GenFw.h"
#include "ElfConvert.h"
#include "Elf32Convert.h"

STATIC
VOID
ScanSections32 (
  VOID
  );

STATIC
BOOLEAN
WriteSections32 (
  SECTION_FILTER_TYPES  FilterType
  );

STATIC
VOID
WriteRelocations32 (
  VOID
  );

STATIC
VOID
WriteDebug32 (
  VOID
  );

STATIC
VOID
SetImageSize32 (
  VOID
  );

STATIC
VOID
CleanUp32 (
  VOID
  );

//
// Rename ELF32 structures to common names to help when porting to ELF64.
//
typedef Elf32_Shdr Elf_Shdr;
typedef Elf32_Ehdr Elf_Ehdr;
typedef Elf32_Rel Elf_Rel;
typedef Elf32_Sym Elf_Sym;
typedef Elf32_Phdr Elf_Phdr;
typedef Elf32_Dyn Elf_Dyn;
#define ELFCLASS ELFCLASS32
#define ELF_R_TYPE(r) ELF32_R_TYPE(r)
#define ELF_R_SYM(r) ELF32_R_SYM(r)

//
// Well known ELF structures.
//
STATIC Elf_Ehdr *mEhdr;
STATIC Elf_Shdr *mShdrBase;
STATIC Elf_Phdr *mPhdrBase;

//
// Coff information
//
STATIC UINT32 mCoffAlignment = 0x20;

//
// PE section alignment.
//
STATIC const UINT16 mCoffNbrSections = 4;

//
// ELF sections to offset in Coff file.
//
STATIC UINT32 *mCoffSectionsOffset = NULL;

//
// Offsets in COFF file
//
STATIC UINT32 mNtHdrOffset;
STATIC UINT32 mTextOffset;
STATIC UINT32 mDataOffset;
STATIC UINT32 mHiiRsrcOffset;
STATIC UINT32 mRelocOffset;
STATIC UINT32 mDebugOffset;

//
// Initialization Function
//
BOOLEAN
InitializeElf32 (
  UINT8               *FileBuffer,
  ELF_FUNCTION_TABLE  *ElfFunctions
  )
{
  //
  // Initialize data pointer and structures.
  //
  mEhdr = (Elf_Ehdr*) FileBuffer;

  //
  // Check the ELF32 specific header information.
  //
  if (mEhdr->e_ident[EI_CLASS] != ELFCLASS32) {
    Error (NULL, 0, 3000, "Unsupported", "ELF EI_DATA not ELFCLASS32");
    return FALSE;
  }
  if (mEhdr->e_ident[EI_DATA] != ELFDATA2LSB) {
    Error (NULL, 0, 3000, "Unsupported", "ELF EI_DATA not ELFDATA2LSB");
    return FALSE;
  }
  if ((mEhdr->e_type != ET_EXEC) && (mEhdr->e_type != ET_DYN)) {
    Error (NULL, 0, 3000, "Unsupported", "ELF e_type not ET_EXEC or ET_DYN");
    return FALSE;
  }
  if (!((mEhdr->e_machine == EM_386) || (mEhdr->e_machine == EM_ARM) || (mEhdr->e_machine == EM_RISCV))) {
    Warning (NULL, 0, 3000, "Unsupported", "ELF e_machine is not Elf32 machine.");
  }
  if (mEhdr->e_version != EV_CURRENT) {
    Error (NULL, 0, 3000, "Unsupported", "ELF e_version (%u) not EV_CURRENT (%d)", (unsigned) mEhdr->e_version, EV_CURRENT);
    return FALSE;
  }

  //
  // Update section header pointers
  //
  mShdrBase  = (Elf_Shdr *)((UINT8 *)mEhdr + mEhdr->e_shoff);
  mPhdrBase = (Elf_Phdr *)((UINT8 *)mEhdr + mEhdr->e_phoff);

  //
  // Create COFF Section offset buffer and zero.
  //
  mCoffSectionsOffset = (UINT32 *)malloc(mEhdr->e_shnum * sizeof (UINT32));
  if (mCoffSectionsOffset == NULL) {
    Error (NULL, 0, 4001, "Resource", "memory cannot be allocated!");
    return FALSE;
  }
  memset(mCoffSectionsOffset, 0, mEhdr->e_shnum * sizeof(UINT32));

  //
  // Fill in function pointers.
  //
  ElfFunctions->ScanSections = ScanSections32;
  ElfFunctions->WriteSections = WriteSections32;
  ElfFunctions->WriteRelocations = WriteRelocations32;
  ElfFunctions->WriteDebug = WriteDebug32;
  ElfFunctions->SetImageSize = SetImageSize32;
  ElfFunctions->CleanUp = CleanUp32;

  return TRUE;
}


//
// Header by Index functions
//
STATIC
Elf_Shdr*
GetShdrByIndex (
  UINT32 Num
  )
{
  if (Num >= mEhdr->e_shnum) {
    Error (NULL, 0, 3000, "Invalid", "GetShdrByIndex: Index %u is too high.", Num);
    exit(EXIT_FAILURE);
  }

  return (Elf_Shdr*)((UINT8*)mShdrBase + Num * mEhdr->e_shentsize);
}

STATIC
Elf_Phdr*
GetPhdrByIndex (
  UINT32 num
  )
{
  if (num >= mEhdr->e_phnum) {
    Error (NULL, 0, 3000, "Invalid", "GetPhdrByIndex: Index %u is too high.", num);
    exit(EXIT_FAILURE);
  }

  return (Elf_Phdr *)((UINT8*)mPhdrBase + num * mEhdr->e_phentsize);
}

STATIC
UINT32
CoffAlign (
  UINT32 Offset
  )
{
  return (Offset + mCoffAlignment - 1) & ~(mCoffAlignment - 1);
}

STATIC
UINT32
DebugRvaAlign (
  UINT32 Offset
  )
{
  return (Offset + 3) & ~3;
}

//
// filter functions
//
STATIC
BOOLEAN
IsTextShdr (
  Elf_Shdr *Shdr
  )
{
  return (BOOLEAN) (((Shdr->sh_flags & (SHF_EXECINSTR | SHF_ALLOC)) == (SHF_EXECINSTR | SHF_ALLOC)) ||
                   ((Shdr->sh_flags & (SHF_WRITE | SHF_ALLOC)) == SHF_ALLOC));
}

STATIC
BOOLEAN
IsHiiRsrcShdr (
  Elf_Shdr *Shdr
  )
{
  Elf_Shdr *Namedr = GetShdrByIndex(mEhdr->e_shstrndx);

  return (BOOLEAN) (strcmp((CHAR8*)mEhdr + Namedr->sh_offset + Shdr->sh_name, ELF_HII_SECTION_NAME) == 0);
}

STATIC
BOOLEAN
IsDataShdr (
  Elf_Shdr *Shdr
  )
{
  if (IsHiiRsrcShdr(Shdr)) {
    return FALSE;
  }
  return (BOOLEAN) (Shdr->sh_flags & (SHF_EXECINSTR | SHF_WRITE | SHF_ALLOC)) == (SHF_ALLOC | SHF_WRITE);
}

STATIC
BOOLEAN
IsStrtabShdr (
  Elf_Shdr *Shdr
  )
{
  Elf_Shdr *Namedr = GetShdrByIndex(mEhdr->e_shstrndx);

  return (BOOLEAN) (strcmp((CHAR8*)mEhdr + Namedr->sh_offset + Shdr->sh_name, ELF_STRTAB_SECTION_NAME) == 0);
}

STATIC
Elf_Shdr *
FindStrtabShdr (
  VOID
  )
{
  UINT32 i;
  for (i = 0; i < mEhdr->e_shnum; i++) {
    Elf_Shdr *shdr = GetShdrByIndex(i);
    if (IsStrtabShdr(shdr)) {
      return shdr;
    }
  }
  return NULL;
}

STATIC
const UINT8 *
GetSymName (
  Elf_Sym *Sym
  )
{
  Elf_Shdr *StrtabShdr;
  UINT8    *StrtabContents;
  BOOLEAN  foundEnd;
  UINT32   i;

  if (Sym->st_name == 0) {
    return NULL;
  }

  StrtabShdr = FindStrtabShdr();
  if (StrtabShdr == NULL) {
    return NULL;
  }

  assert(Sym->st_name < StrtabShdr->sh_size);

  StrtabContents = (UINT8*)mEhdr + StrtabShdr->sh_offset;

  foundEnd = FALSE;
  for (i = Sym->st_name; (i < StrtabShdr->sh_size) && !foundEnd; i++) {
    foundEnd = (BOOLEAN)(StrtabContents[i] == 0);
  }
  assert(foundEnd);

  return StrtabContents + Sym->st_name;
}

//
// Elf functions interface implementation
//

STATIC
VOID
ScanSections32 (
  VOID
  )
{
  UINT32                          i;
  EFI_IMAGE_DOS_HEADER            *DosHdr;
  EFI_IMAGE_OPTIONAL_HEADER_UNION *NtHdr;
  UINT32                          CoffEntry;
  UINT32                          SectionCount;
  BOOLEAN                         FoundSection;

  CoffEntry = 0;
  mCoffOffset = 0;

  //
  // Coff file start with a DOS header.
  //
  mCoffOffset = sizeof(EFI_IMAGE_DOS_HEADER) + 0x40;
  mNtHdrOffset = mCoffOffset;
  switch (mEhdr->e_machine) {
  case EM_386:
  case EM_ARM:
    mCoffOffset += sizeof (EFI_IMAGE_NT_HEADERS32);
  break;
  default:
    VerboseMsg ("%u unknown e_machine type. Assume IA-32", (UINTN)mEhdr->e_machine);
    mCoffOffset += sizeof (EFI_IMAGE_NT_HEADERS32);
  break;
  }

  mTableOffset = mCoffOffset;
  mCoffOffset += mCoffNbrSections * sizeof(EFI_IMAGE_SECTION_HEADER);

  //
  // Set mCoffAlignment to the maximum alignment of the input sections
  // we care about
  //
  for (i = 0; i < mEhdr->e_shnum; i++) {
    Elf_Shdr *shdr = GetShdrByIndex(i);
    if (shdr->sh_addralign <= mCoffAlignment) {
      continue;
    }
    if (IsTextShdr(shdr) || IsDataShdr(shdr) || IsHiiRsrcShdr(shdr)) {
      mCoffAlignment = (UINT32)shdr->sh_addralign;
    }
  }

  //
  // Check if mCoffAlignment is larger than MAX_COFF_ALIGNMENT
  //
  if (mCoffAlignment > MAX_COFF_ALIGNMENT) {
    Error (NULL, 0, 3000, "Invalid", "Section alignment is larger than MAX_COFF_ALIGNMENT.");
    assert (FALSE);
  }

  //
  // Move the PE/COFF header right before the first section. This will help us
  // save space when converting to TE.
  //
  if (mCoffAlignment > mCoffOffset) {
    mNtHdrOffset += mCoffAlignment - mCoffOffset;
    mTableOffset += mCoffAlignment - mCoffOffset;
    mCoffOffset = mCoffAlignment;
  }

  //
  // First text sections.
  //
  mCoffOffset = CoffAlign(mCoffOffset);
  mTextOffset = mCoffOffset;
  FoundSection = FALSE;
  SectionCount = 0;
  for (i = 0; i < mEhdr->e_shnum; i++) {
    Elf_Shdr *shdr = GetShdrByIndex(i);
    if (IsTextShdr(shdr)) {
      if ((shdr->sh_addralign != 0) && (shdr->sh_addralign != 1)) {
        // the alignment field is valid
        if ((shdr->sh_addr & (shdr->sh_addralign - 1)) == 0) {
          // if the section address is aligned we must align PE/COFF
          mCoffOffset = (mCoffOffset + shdr->sh_addralign - 1) & ~(shdr->sh_addralign - 1);
        } else {
          Error (NULL, 0, 3000, "Invalid", "Section address not aligned to its own alignment.");
        }
      }

      /* Relocate entry.  */
      if ((mEhdr->e_entry >= shdr->sh_addr) &&
          (mEhdr->e_entry < shdr->sh_addr + shdr->sh_size)) {
        CoffEntry = mCoffOffset + mEhdr->e_entry - shdr->sh_addr;
      }

      //
      // Set mTextOffset with the offset of the first '.text' section
      //
      if (!FoundSection) {
        mTextOffset = mCoffOffset;
        FoundSection = TRUE;
      }

      mCoffSectionsOffset[i] = mCoffOffset;
      mCoffOffset += shdr->sh_size;
      SectionCount ++;
    }
  }

  if (!FoundSection && mOutImageType != FW_ACPI_IMAGE) {
    Error (NULL, 0, 3000, "Invalid", "Did not find any '.text' section.");
    assert (FALSE);
  }

  mDebugOffset = DebugRvaAlign(mCoffOffset);
  mCoffOffset = CoffAlign(mCoffOffset);

  if (SectionCount > 1 && mOutImageType == FW_EFI_IMAGE) {
    Warning (NULL, 0, 0, NULL, "Multiple sections in %s are merged into 1 text section. Source level debug might not work correctly.", mInImageName);
  }

  //
  //  Then data sections.
  //
  mDataOffset = mCoffOffset;
  FoundSection = FALSE;
  SectionCount = 0;
  for (i = 0; i < mEhdr->e_shnum; i++) {
    Elf_Shdr *shdr = GetShdrByIndex(i);
    if (IsDataShdr(shdr)) {
      if ((shdr->sh_addralign != 0) && (shdr->sh_addralign != 1)) {
        // the alignment field is valid
        if ((shdr->sh_addr & (shdr->sh_addralign - 1)) == 0) {
          // if the section address is aligned we must align PE/COFF
          mCoffOffset = (mCoffOffset + shdr->sh_addralign - 1) & ~(shdr->sh_addralign - 1);
        } else {
          Error (NULL, 0, 3000, "Invalid", "Section address not aligned to its own alignment.");
        }
      }

      //
      // Set mDataOffset with the offset of the first '.data' section
      //
      if (!FoundSection) {
        mDataOffset = mCoffOffset;
        FoundSection = TRUE;
      }

      mCoffSectionsOffset[i] = mCoffOffset;
      mCoffOffset += shdr->sh_size;
      SectionCount ++;
    }
  }

  if (SectionCount > 1 && mOutImageType == FW_EFI_IMAGE) {
    Warning (NULL, 0, 0, NULL, "Multiple sections in %s are merged into 1 data section. Source level debug might not work correctly.", mInImageName);
  }

  //
  // Make room for .debug data in .data (or .text if .data is empty) instead of
  // putting it in a section of its own. This is explicitly allowed by the
  // PE/COFF spec, and prevents bloat in the binary when using large values for
  // section alignment.
  //
  if (SectionCount > 0) {
    mDebugOffset = DebugRvaAlign(mCoffOffset);
  }
  mCoffOffset = mDebugOffset + sizeof(EFI_IMAGE_DEBUG_DIRECTORY_ENTRY) +
                sizeof(EFI_IMAGE_DEBUG_CODEVIEW_NB10_ENTRY) +
                strlen(mInImageName) + 1;

  mCoffOffset = CoffAlign(mCoffOffset);
  if (SectionCount == 0) {
    mDataOffset = mCoffOffset;
  }

  //
  //  The HII resource sections.
  //
  mHiiRsrcOffset = mCoffOffset;
  for (i = 0; i < mEhdr->e_shnum; i++) {
    Elf_Shdr *shdr = GetShdrByIndex(i);
    if (IsHiiRsrcShdr(shdr)) {
      if ((shdr->sh_addralign != 0) && (shdr->sh_addralign != 1)) {
        // the alignment field is valid
        if ((shdr->sh_addr & (shdr->sh_addralign - 1)) == 0) {
          // if the section address is aligned we must align PE/COFF
          mCoffOffset = (mCoffOffset + shdr->sh_addralign - 1) & ~(shdr->sh_addralign - 1);
        } else {
          Error (NULL, 0, 3000, "Invalid", "Section address not aligned to its own alignment.");
        }
      }
      if (shdr->sh_size != 0) {
        mHiiRsrcOffset = mCoffOffset;
        mCoffSectionsOffset[i] = mCoffOffset;
        mCoffOffset += shdr->sh_size;
        mCoffOffset = CoffAlign(mCoffOffset);
        SetHiiResourceHeader ((UINT8*) mEhdr + shdr->sh_offset, mHiiRsrcOffset);
      }
      break;
    }
  }

  mRelocOffset = mCoffOffset;

  //
  // Allocate base Coff file.  Will be expanded later for relocations.
  //
  mCoffFile = (UINT8 *)malloc(mCoffOffset);
  if (mCoffFile == NULL) {
    Error (NULL, 0, 4001, "Resource", "memory cannot be allocated!");
  }
  assert (mCoffFile != NULL);
  memset(mCoffFile, 0, mCoffOffset);

  //
  // Fill headers.
  //
  DosHdr = (EFI_IMAGE_DOS_HEADER *)mCoffFile;
  DosHdr->e_magic = EFI_IMAGE_DOS_SIGNATURE;
  DosHdr->e_lfanew = mNtHdrOffset;

  NtHdr = (EFI_IMAGE_OPTIONAL_HEADER_UNION*)(mCoffFile + mNtHdrOffset);

  NtHdr->Pe32.Signature = EFI_IMAGE_NT_SIGNATURE;

  switch (mEhdr->e_machine) {
  case EM_386:
    NtHdr->Pe32.FileHeader.Machine = EFI_IMAGE_MACHINE_IA32;
    NtHdr->Pe32.OptionalHeader.Magic = EFI_IMAGE_NT_OPTIONAL_HDR32_MAGIC;
    break;
  case EM_ARM:
    NtHdr->Pe32.FileHeader.Machine = EFI_IMAGE_MACHINE_ARMT;
    NtHdr->Pe32.OptionalHeader.Magic = EFI_IMAGE_NT_OPTIONAL_HDR32_MAGIC;
    break;
  default:
    VerboseMsg ("%s unknown e_machine type %hu. Assume IA-32", mInImageName, mEhdr->e_machine);
    NtHdr->Pe32.FileHeader.Machine = EFI_IMAGE_MACHINE_IA32;
    NtHdr->Pe32.OptionalHeader.Magic = EFI_IMAGE_NT_OPTIONAL_HDR32_MAGIC;
  }

  NtHdr->Pe32.FileHeader.NumberOfSections = mCoffNbrSections;
  NtHdr->Pe32.FileHeader.TimeDateStamp = (UINT32) time(NULL);
  mImageTimeStamp = NtHdr->Pe32.FileHeader.TimeDateStamp;
  NtHdr->Pe32.FileHeader.PointerToSymbolTable = 0;
  NtHdr->Pe32.FileHeader.NumberOfSymbols = 0;
  NtHdr->Pe32.FileHeader.SizeOfOptionalHeader = sizeof(NtHdr->Pe32.OptionalHeader);
  NtHdr->Pe32.FileHeader.Characteristics = EFI_IMAGE_FILE_EXECUTABLE_IMAGE
    | EFI_IMAGE_FILE_LINE_NUMS_STRIPPED
    | EFI_IMAGE_FILE_LOCAL_SYMS_STRIPPED
    | EFI_IMAGE_FILE_32BIT_MACHINE;

  NtHdr->Pe32.OptionalHeader.SizeOfCode = mDataOffset - mTextOffset;
  NtHdr->Pe32.OptionalHeader.SizeOfInitializedData = mRelocOffset - mDataOffset;
  NtHdr->Pe32.OptionalHeader.SizeOfUninitializedData = 0;
  NtHdr->Pe32.OptionalHeader.AddressOfEntryPoint = CoffEntry;

  NtHdr->Pe32.OptionalHeader.BaseOfCode = mTextOffset;

  NtHdr->Pe32.OptionalHeader.BaseOfData = mDataOffset;
  NtHdr->Pe32.OptionalHeader.ImageBase = 0;
  NtHdr->Pe32.OptionalHeader.SectionAlignment = mCoffAlignment;
  NtHdr->Pe32.OptionalHeader.FileAlignment = mCoffAlignment;
  NtHdr->Pe32.OptionalHeader.SizeOfImage = 0;

  NtHdr->Pe32.OptionalHeader.SizeOfHeaders = mTextOffset;
  NtHdr->Pe32.OptionalHeader.NumberOfRvaAndSizes = EFI_IMAGE_NUMBER_OF_DIRECTORY_ENTRIES;

  //
  // Section headers.
  //
  if ((mDataOffset - mTextOffset) > 0) {
    CreateSectionHeader (".text", mTextOffset, mDataOffset - mTextOffset,
            EFI_IMAGE_SCN_CNT_CODE
            | EFI_IMAGE_SCN_MEM_EXECUTE
            | EFI_IMAGE_SCN_MEM_READ);
  } else {
    // Don't make a section of size 0.
    NtHdr->Pe32.FileHeader.NumberOfSections--;
  }

  if ((mHiiRsrcOffset - mDataOffset) > 0) {
    CreateSectionHeader (".data", mDataOffset, mHiiRsrcOffset - mDataOffset,
            EFI_IMAGE_SCN_CNT_INITIALIZED_DATA
            | EFI_IMAGE_SCN_MEM_WRITE
            | EFI_IMAGE_SCN_MEM_READ);
  } else {
    // Don't make a section of size 0.
    NtHdr->Pe32.FileHeader.NumberOfSections--;
  }

  if ((mRelocOffset - mHiiRsrcOffset) > 0) {
    CreateSectionHeader (".rsrc", mHiiRsrcOffset, mRelocOffset - mHiiRsrcOffset,
            EFI_IMAGE_SCN_CNT_INITIALIZED_DATA
            | EFI_IMAGE_SCN_MEM_READ);

    NtHdr->Pe32.OptionalHeader.DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY_RESOURCE].Size = mRelocOffset - mHiiRsrcOffset;
    NtHdr->Pe32.OptionalHeader.DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY_RESOURCE].VirtualAddress = mHiiRsrcOffset;
  } else {
    // Don't make a section of size 0.
    NtHdr->Pe32.FileHeader.NumberOfSections--;
  }

}

STATIC
BOOLEAN
WriteSections32 (
  SECTION_FILTER_TYPES  FilterType
  )
{
  UINT32      Idx;
  Elf_Shdr    *SecShdr;
  UINT32      SecOffset;
  BOOLEAN     (*Filter)(Elf_Shdr *);

  //
  // Initialize filter pointer
  //
  switch (FilterType) {
    case SECTION_TEXT:
      Filter = IsTextShdr;
      break;
    case SECTION_HII:
      Filter = IsHiiRsrcShdr;
      break;
    case SECTION_DATA:
      Filter = IsDataShdr;
      break;
    default:
      return FALSE;
  }

  //
  // First: copy sections.
  //
  for (Idx = 0; Idx < mEhdr->e_shnum; Idx++) {
    Elf_Shdr *Shdr = GetShdrByIndex(Idx);
    if ((*Filter)(Shdr)) {
      switch (Shdr->sh_type) {
      case SHT_PROGBITS:
        /* Copy.  */
        if (Shdr->sh_offset + Shdr->sh_size > mFileBufferSize) {
          return FALSE;
        }
        memcpy(mCoffFile + mCoffSectionsOffset[Idx],
              (UINT8*)mEhdr + Shdr->sh_offset,
              Shdr->sh_size);
        break;

      case SHT_NOBITS:
        memset(mCoffFile + mCoffSectionsOffset[Idx], 0, Shdr->sh_size);
        break;

      default:
        //
        //  Ignore for unknown section type.
        //
        VerboseMsg ("%s unknown section type %x. We ignore this unknown section type.", mInImageName, (unsigned)Shdr->sh_type);
        break;
      }
    }
  }

  //
  // Second: apply relocations.
  //
  for (Idx = 0; Idx < mEhdr->e_shnum; Idx++) {
    //
    // Determine if this is a relocation section.
    //
    Elf_Shdr *RelShdr = GetShdrByIndex(Idx);
    if ((RelShdr->sh_type != SHT_REL) && (RelShdr->sh_type != SHT_RELA)) {
      continue;
    }

    //
    // Relocation section found.  Now extract section information that the relocations
    // apply to in the ELF data and the new COFF data.
    //
    SecShdr = GetShdrByIndex(RelShdr->sh_info);
    SecOffset = mCoffSectionsOffset[RelShdr->sh_info];

    //
    // Only process relocations for the current filter type.
    //
    if (RelShdr->sh_type == SHT_REL && (*Filter)(SecShdr)) {
      UINT32 RelOffset;

      //
      // Determine the symbol table referenced by the relocation data.
      //
      Elf_Shdr *SymtabShdr = GetShdrByIndex(RelShdr->sh_link);
      UINT8 *Symtab = (UINT8*)mEhdr + SymtabShdr->sh_offset;

      //
      // Process all relocation entries for this section.
      //
      for (RelOffset = 0; RelOffset < RelShdr->sh_size; RelOffset += RelShdr->sh_entsize) {
        //
        // Set pointer to relocation entry
        //
        Elf_Rel *Rel = (Elf_Rel *)((UINT8*)mEhdr + RelShdr->sh_offset + RelOffset);

        //
        // Set pointer to symbol table entry associated with the relocation entry.
        //
        Elf_Sym *Sym = (Elf_Sym *)(Symtab + ELF_R_SYM(Rel->r_info) * SymtabShdr->sh_entsize);

        Elf_Shdr *SymShdr;
        UINT8 *Targ;
        UINT16 Address;

        //
        // Check section header index found in symbol table and get the section
        // header location.
        //
        if (Sym->st_shndx == SHN_UNDEF
            || Sym->st_shndx >= mEhdr->e_shnum) {
          const UINT8 *SymName = GetSymName(Sym);
          if (SymName == NULL) {
            SymName = (const UINT8 *)"<unknown>";
          }
          continue;
        }
        SymShdr = GetShdrByIndex(Sym->st_shndx);

        //
        // Convert the relocation data to a pointer into the coff file.
        //
        // Note:
        //   r_offset is the virtual address of the storage unit to be relocated.
        //   sh_addr is the virtual address for the base of the section.
        //
        Targ = mCoffFile + SecOffset + (Rel->r_offset - SecShdr->sh_addr);

        //
        // Determine how to handle each relocation type based on the machine type.
        //
        if (mEhdr->e_machine == EM_386) {
          switch (ELF_R_TYPE(Rel->r_info)) {
          case R_386_NONE:
            break;
          case R_386_32:
            //
            // Absolute relocation.
            //  Converts Targ from a absolute virtual address to the absolute
            //  COFF address.
            //
            *(UINT32 *)Targ = *(UINT32 *)Targ - SymShdr->sh_addr
              + mCoffSectionsOffset[Sym->st_shndx];
            break;
          case R_386_PC32:
            //
            // Relative relocation: Symbol - Ip + Addend
            //
            *(UINT32 *)Targ = *(UINT32 *)Targ
              + (mCoffSectionsOffset[Sym->st_shndx] - SymShdr->sh_addr)
              - (SecOffset - SecShdr->sh_addr);
            break;
          default:
            Error (NULL, 0, 3000, "Invalid", "%s unsupported ELF EM_386 relocation 0x%x.", mInImageName, (unsigned) ELF_R_TYPE(Rel->r_info));
          }
        } else if (mEhdr->e_machine == EM_ARM) {
          switch (ELF32_R_TYPE(Rel->r_info)) {
          case R_ARM_RBASE:
            // No relocation - no action required
            // break skipped

          case R_ARM_PC24:
          case R_ARM_REL32:
          case R_ARM_XPC25:
          case R_ARM_THM_PC22:
          case R_ARM_THM_JUMP19:
          case R_ARM_CALL:
          case R_ARM_JMP24:
          case R_ARM_THM_JUMP24:
          case R_ARM_PREL31:
          case R_ARM_MOVW_PREL_NC:
          case R_ARM_MOVT_PREL:
          case R_ARM_THM_MOVW_PREL_NC:
          case R_ARM_THM_MOVT_PREL:
          case R_ARM_THM_JMP6:
          case R_ARM_THM_ALU_PREL_11_0:
          case R_ARM_THM_PC12:
          case R_ARM_REL32_NOI:
          case R_ARM_ALU_PC_G0_NC:
          case R_ARM_ALU_PC_G0:
          case R_ARM_ALU_PC_G1_NC:
          case R_ARM_ALU_PC_G1:
          case R_ARM_ALU_PC_G2:
          case R_ARM_LDR_PC_G1:
          case R_ARM_LDR_PC_G2:
          case R_ARM_LDRS_PC_G0:
          case R_ARM_LDRS_PC_G1:
          case R_ARM_LDRS_PC_G2:
          case R_ARM_LDC_PC_G0:
          case R_ARM_LDC_PC_G1:
          case R_ARM_LDC_PC_G2:
          case R_ARM_THM_JUMP11:
          case R_ARM_THM_JUMP8:
          case R_ARM_TLS_GD32:
          case R_ARM_TLS_LDM32:
          case R_ARM_TLS_IE32:
            // Thease are all PC-relative relocations and don't require modification
            // GCC does not seem to have the concept of a application that just needs to get relocated.
            break;

          case R_ARM_THM_MOVW_ABS_NC:
            // MOVW is only lower 16-bits of the addres
            Address = (UINT16)(Sym->st_value - SymShdr->sh_addr + mCoffSectionsOffset[Sym->st_shndx]);
            ThumbMovtImmediatePatch ((UINT16 *)Targ, Address);
            break;

          case R_ARM_THM_MOVT_ABS:
            // MOVT is only upper 16-bits of the addres
            Address = (UINT16)((Sym->st_value - SymShdr->sh_addr + mCoffSectionsOffset[Sym->st_shndx]) >> 16);
            ThumbMovtImmediatePatch ((UINT16 *)Targ, Address);
            break;

          case R_ARM_ABS32:
          case R_ARM_RABS32:
            //
            // Absolute relocation.
            //
            *(UINT32 *)Targ = *(UINT32 *)Targ - SymShdr->sh_addr + mCoffSectionsOffset[Sym->st_shndx];
            break;

          default:
            Error (NULL, 0, 3000, "Invalid", "WriteSections (): %s unsupported ELF EM_ARM relocation 0x%x.", mInImageName, (unsigned) ELF32_R_TYPE(Rel->r_info));
          }
        }
      }
    }
  }

  return TRUE;
}

UINTN gMovwOffset = 0;

STATIC
VOID
WriteRelocations32 (
  VOID
  )
{
  UINT32                           Index;
  EFI_IMAGE_OPTIONAL_HEADER_UNION  *NtHdr;
  EFI_IMAGE_DATA_DIRECTORY         *Dir;
  BOOLEAN                          FoundRelocations;
  Elf_Dyn                          *Dyn;
  Elf_Rel                          *Rel;
  UINTN                            RelElementSize;
  UINTN                            RelSize;
  UINTN                            RelOffset;
  UINTN                            K;
  Elf32_Phdr                       *DynamicSegment;

  for (Index = 0, FoundRelocations = FALSE; Index < mEhdr->e_shnum; Index++) {
    Elf_Shdr *RelShdr = GetShdrByIndex(Index);
    if ((RelShdr->sh_type == SHT_REL) || (RelShdr->sh_type == SHT_RELA)) {
      Elf_Shdr *SecShdr = GetShdrByIndex (RelShdr->sh_info);
      if (IsTextShdr(SecShdr) || IsDataShdr(SecShdr)) {
        UINT32 RelIdx;

        FoundRelocations = TRUE;
        for (RelIdx = 0; RelIdx < RelShdr->sh_size; RelIdx += RelShdr->sh_entsize) {
          Rel = (Elf_Rel *)((UINT8*)mEhdr + RelShdr->sh_offset + RelIdx);

          if (mEhdr->e_machine == EM_386) {
            switch (ELF_R_TYPE(Rel->r_info)) {
            case R_386_NONE:
            case R_386_PC32:
              //
              // No fixup entry required.
              //
              break;
            case R_386_32:
              //
              // Creates a relative relocation entry from the absolute entry.
              //
              CoffAddFixup(mCoffSectionsOffset[RelShdr->sh_info]
              + (Rel->r_offset - SecShdr->sh_addr),
              EFI_IMAGE_REL_BASED_HIGHLOW);
              break;
            default:
              Error (NULL, 0, 3000, "Invalid", "%s unsupported ELF EM_386 relocation 0x%x.", mInImageName, (unsigned) ELF_R_TYPE(Rel->r_info));
            }
          } else if (mEhdr->e_machine == EM_ARM) {
            switch (ELF32_R_TYPE(Rel->r_info)) {
            case R_ARM_RBASE:
              // No relocation - no action required
              // break skipped

            case R_ARM_PC24:
            case R_ARM_REL32:
            case R_ARM_XPC25:
            case R_ARM_THM_PC22:
            case R_ARM_THM_JUMP19:
            case R_ARM_CALL:
            case R_ARM_JMP24:
            case R_ARM_THM_JUMP24:
            case R_ARM_PREL31:
            case R_ARM_MOVW_PREL_NC:
            case R_ARM_MOVT_PREL:
            case R_ARM_THM_MOVW_PREL_NC:
            case R_ARM_THM_MOVT_PREL:
            case R_ARM_THM_JMP6:
            case R_ARM_THM_ALU_PREL_11_0:
            case R_ARM_THM_PC12:
            case R_ARM_REL32_NOI:
            case R_ARM_ALU_PC_G0_NC:
            case R_ARM_ALU_PC_G0:
            case R_ARM_ALU_PC_G1_NC:
            case R_ARM_ALU_PC_G1:
            case R_ARM_ALU_PC_G2:
            case R_ARM_LDR_PC_G1:
            case R_ARM_LDR_PC_G2:
            case R_ARM_LDRS_PC_G0:
            case R_ARM_LDRS_PC_G1:
            case R_ARM_LDRS_PC_G2:
            case R_ARM_LDC_PC_G0:
            case R_ARM_LDC_PC_G1:
            case R_ARM_LDC_PC_G2:
            case R_ARM_THM_JUMP11:
            case R_ARM_THM_JUMP8:
            case R_ARM_TLS_GD32:
            case R_ARM_TLS_LDM32:
            case R_ARM_TLS_IE32:
              // Thease are all PC-relative relocations and don't require modification
              break;

            case R_ARM_THM_MOVW_ABS_NC:
              CoffAddFixup (
                mCoffSectionsOffset[RelShdr->sh_info]
                + (Rel->r_offset - SecShdr->sh_addr),
                EFI_IMAGE_REL_BASED_ARM_MOV32T
                );

              // PE/COFF treats MOVW/MOVT relocation as single 64-bit instruction
              // Track this address so we can log an error for unsupported sequence of MOVW/MOVT
              gMovwOffset = mCoffSectionsOffset[RelShdr->sh_info] + (Rel->r_offset - SecShdr->sh_addr);
              break;

            case R_ARM_THM_MOVT_ABS:
              if ((gMovwOffset + 4) !=  (mCoffSectionsOffset[RelShdr->sh_info] + (Rel->r_offset - SecShdr->sh_addr))) {
                Error (NULL, 0, 3000, "Not Supported", "PE/COFF requires MOVW+MOVT instruction sequence %x +4 != %x.", gMovwOffset, mCoffSectionsOffset[RelShdr->sh_info] + (Rel->r_offset - SecShdr->sh_addr));
              }
              break;

            case R_ARM_ABS32:
            case R_ARM_RABS32:
              CoffAddFixup (
                mCoffSectionsOffset[RelShdr->sh_info]
                + (Rel->r_offset - SecShdr->sh_addr),
                EFI_IMAGE_REL_BASED_HIGHLOW
                );
              break;

           default:
              Error (NULL, 0, 3000, "Invalid", "WriteRelocations(): %s unsupported ELF EM_ARM relocation 0x%x.", mInImageName, (unsigned) ELF32_R_TYPE(Rel->r_info));
            }
          } else {
            Error (NULL, 0, 3000, "Not Supported", "This tool does not support relocations for ELF with e_machine %u (processor type).", (unsigned) mEhdr->e_machine);
          }
        }
      }
    }
  }

  if (!FoundRelocations && (mEhdr->e_machine == EM_ARM)) {
    /* Try again, but look for PT_DYNAMIC instead of SHT_REL */

    for (Index = 0; Index < mEhdr->e_phnum; Index++) {
      RelElementSize = 0;
      RelSize = 0;
      RelOffset = 0;

      DynamicSegment = GetPhdrByIndex (Index);

      if (DynamicSegment->p_type == PT_DYNAMIC) {
        Dyn = (Elf32_Dyn *) ((UINT8 *)mEhdr + DynamicSegment->p_offset);

        while (Dyn->d_tag != DT_NULL) {
          switch (Dyn->d_tag) {
            case  DT_REL:
              RelOffset = Dyn->d_un.d_val;
              break;

            case  DT_RELSZ:
              RelSize = Dyn->d_un.d_val;
              break;

            case  DT_RELENT:
              RelElementSize = Dyn->d_un.d_val;
              break;

            default:
              break;
          }
          Dyn++;
        }
        if (( RelOffset == 0 ) || ( RelSize == 0 ) || ( RelElementSize == 0 )) {
          Error (NULL, 0, 3000, "Invalid", "%s bad ARM dynamic relocations.", mInImageName);
        }

        for (Index = 0; Index < mEhdr->e_shnum; Index++) {
          Elf_Shdr *shdr = GetShdrByIndex(Index);

          //
          // The PT_DYNAMIC section contains DT_REL relocations whose r_offset
          // field is relative to the base of a segment (or the entire image),
          // and not to the base of an ELF input section as is the case for
          // SHT_REL sections. This means that we cannot fix up such relocations
          // unless we cross-reference ELF sections and segments, considering
          // that the output placement recorded in mCoffSectionsOffset[] is
          // section based, not segment based.
          //
          // Fortunately, there is a simple way around this: we require that the
          // in-memory layout of the ELF and PE/COFF versions of the binary is
          // identical. That way, r_offset will retain its validity as a PE/COFF
          // image offset, and we can record it in the COFF fixup table
          // unmodified.
          //
          if (shdr->sh_addr != mCoffSectionsOffset[Index]) {
            Error (NULL, 0, 3000,
              "Invalid", "%s: PT_DYNAMIC relocations require identical ELF and PE/COFF section offsets.",
              mInImageName);
          }
        }

        for (K = 0; K < RelSize; K += RelElementSize) {

          if (DynamicSegment->p_paddr == 0) {
            // Older versions of the ARM ELF (SWS ESPC 0003 B-02) specification define DT_REL
            // as an offset in the dynamic segment. p_paddr is defined to be zero for ARM tools
            Rel = (Elf32_Rel *) ((UINT8 *) mEhdr + DynamicSegment->p_offset + RelOffset + K);
          } else {
            // This is how it reads in the generic ELF specification
            Rel = (Elf32_Rel *) ((UINT8 *) mEhdr + RelOffset + K);
          }

          switch (ELF32_R_TYPE (Rel->r_info)) {
          case  R_ARM_RBASE:
            break;

          case  R_ARM_RABS32:
            CoffAddFixup (Rel->r_offset, EFI_IMAGE_REL_BASED_HIGHLOW);
            break;

          default:
            Error (NULL, 0, 3000, "Invalid", "%s bad ARM dynamic relocations, unknown type %d.", mInImageName, ELF32_R_TYPE (Rel->r_info));
            break;
          }
        }
        break;
      }
    }
  }

  //
  // Pad by adding empty entries.
  //
  while (mCoffOffset & (mCoffAlignment - 1)) {
    CoffAddFixupEntry(0);
  }

  NtHdr = (EFI_IMAGE_OPTIONAL_HEADER_UNION *)(mCoffFile + mNtHdrOffset);
  Dir = &NtHdr->Pe32.OptionalHeader.DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY_BASERELOC];
  Dir->Size = mCoffOffset - mRelocOffset;
  if (Dir->Size == 0) {
    // If no relocations, null out the directory entry and don't add the .reloc section
    Dir->VirtualAddress = 0;
    NtHdr->Pe32.FileHeader.NumberOfSections--;
  } else {
    Dir->VirtualAddress = mRelocOffset;
    CreateSectionHeader (".reloc", mRelocOffset, mCoffOffset - mRelocOffset,
            EFI_IMAGE_SCN_CNT_INITIALIZED_DATA
            | EFI_IMAGE_SCN_MEM_DISCARDABLE
            | EFI_IMAGE_SCN_MEM_READ);
  }

}

STATIC
VOID
WriteDebug32 (
  VOID
  )
{
  UINT32                              Len;
  EFI_IMAGE_OPTIONAL_HEADER_UNION     *NtHdr;
  EFI_IMAGE_DATA_DIRECTORY            *DataDir;
  EFI_IMAGE_DEBUG_DIRECTORY_ENTRY     *Dir;
  EFI_IMAGE_DEBUG_CODEVIEW_NB10_ENTRY *Nb10;

  Len = strlen(mInImageName) + 1;

  Dir = (EFI_IMAGE_DEBUG_DIRECTORY_ENTRY*)(mCoffFile + mDebugOffset);
  Dir->Type = EFI_IMAGE_DEBUG_TYPE_CODEVIEW;
  Dir->SizeOfData = sizeof(EFI_IMAGE_DEBUG_CODEVIEW_NB10_ENTRY) + Len;
  Dir->RVA = mDebugOffset + sizeof(EFI_IMAGE_DEBUG_DIRECTORY_ENTRY);
  Dir->FileOffset = mDebugOffset + sizeof(EFI_IMAGE_DEBUG_DIRECTORY_ENTRY);

  Nb10 = (EFI_IMAGE_DEBUG_CODEVIEW_NB10_ENTRY*)(Dir + 1);
  Nb10->Signature = CODEVIEW_SIGNATURE_NB10;
  strcpy ((char *)(Nb10 + 1), mInImageName);


  NtHdr = (EFI_IMAGE_OPTIONAL_HEADER_UNION *)(mCoffFile + mNtHdrOffset);
  DataDir = &NtHdr->Pe32.OptionalHeader.DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY_DEBUG];
  DataDir->VirtualAddress = mDebugOffset;
  DataDir->Size = sizeof(EFI_IMAGE_DEBUG_DIRECTORY_ENTRY);
}

STATIC
VOID
SetImageSize32 (
  VOID
  )
{
  EFI_IMAGE_OPTIONAL_HEADER_UNION *NtHdr;

  //
  // Set image size
  //
  NtHdr = (EFI_IMAGE_OPTIONAL_HEADER_UNION *)(mCoffFile + mNtHdrOffset);
  NtHdr->Pe32.OptionalHeader.SizeOfImage = mCoffOffset;
}

STATIC
VOID
CleanUp32 (
  VOID
  )
{
  if (mCoffSectionsOffset != NULL) {
    free (mCoffSectionsOffset);
  }
}


