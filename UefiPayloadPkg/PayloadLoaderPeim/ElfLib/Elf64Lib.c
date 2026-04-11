/** @file
  ELF library

  Copyright (c) 2019 - 2021, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "ElfLibInternal.h"

/**
  Return the section header specified by Index.

  @param ImageBase      The image base.
  @param Index          The section index.

  @return Pointer to the section header.
**/
Elf64_Shdr *
GetElf64SectionByIndex (
  IN  UINT8   *ImageBase,
  IN  UINT32  Index
  )
{
  Elf64_Ehdr  *Ehdr;

  Ehdr = (Elf64_Ehdr *)ImageBase;
  if (Index >= Ehdr->e_shnum) {
    return NULL;
  }

  return (Elf64_Shdr *)(ImageBase + Ehdr->e_shoff + Index * Ehdr->e_shentsize);
}

/**
  Return the segment header specified by Index.

  @param ImageBase      The image base.
  @param Index          The segment index.

  @return Pointer to the segment header.
**/
Elf64_Phdr *
GetElf64SegmentByIndex (
  IN  UINT8   *ImageBase,
  IN  UINT32  Index
  )
{
  Elf64_Ehdr  *Ehdr;

  Ehdr = (Elf64_Ehdr *)ImageBase;
  if (Index >= Ehdr->e_phnum) {
    return NULL;
  }

  return (Elf64_Phdr *)(ImageBase + Ehdr->e_phoff + Index * Ehdr->e_phentsize);
}

/**
  Return the section header specified by the range.

  @param ImageBase      The image base.
  @param Offset         The section offset.
  @param Size           The section size.

  @return Pointer to the section header.
**/
Elf64_Shdr *
GetElf64SectionByRange (
  IN  UINT8   *ImageBase,
  IN  UINT64  Offset,
  IN  UINT64  Size
  )
{
  UINT32      Index;
  Elf64_Ehdr  *Ehdr;
  Elf64_Shdr  *Shdr;

  Ehdr = (Elf64_Ehdr *)ImageBase;

  Shdr = (Elf64_Shdr *)(ImageBase + Ehdr->e_shoff);
  for (Index = 0; Index < Ehdr->e_shnum; Index++) {
    if ((Shdr->sh_offset == Offset) && (Shdr->sh_size == Size)) {
      return Shdr;
    }

    Shdr = ELF_NEXT_ENTRY (Elf64_Shdr, Shdr, Ehdr->e_shentsize);
  }

  return NULL;
}

/**
  Fix up the image based on the relocation entries.

  @param Rela                Relocation entries.
  @param RelaSize            Total size of relocation entries.
  @param RelaEntrySize       Relocation entry size.
  @param RelaType            Type of relocation entry.
  @param Delta               The delta between preferred image base and the actual image base.
  @param DynamicLinking      TRUE when fixing up according to dynamic relocation.

  @retval EFI_SUCCESS   The image fix up is processed successfully.
**/
EFI_STATUS
ProcessRelocation64 (
  IN  Elf64_Rela  *Rela,
  IN  UINT64      RelaSize,
  IN  UINT64      RelaEntrySize,
  IN  UINT64      RelaType,
  IN  INT64       Delta,
  IN  BOOLEAN     DynamicLinking
  )
{
  UINTN   Index;
  UINT64  *Ptr;
  UINT32  Type;

  for ( Index = 0
        ; MultU64x64 (RelaEntrySize, Index) < RelaSize
        ; Index++, Rela = ELF_NEXT_ENTRY (Elf64_Rela, Rela, RelaEntrySize)
        )
  {
    //
    // r_offset is the virtual address of the storage unit affected by the relocation.
    //
    Ptr  = (UINT64 *)(UINTN)(Rela->r_offset + Delta);
    Type = ELF64_R_TYPE (Rela->r_info);
    switch (Type) {
      case R_X86_64_NONE:
      case R_X86_64_PC32:
      case R_X86_64_PLT32:
      case R_X86_64_GOTPCREL:
      case R_X86_64_GOTPCRELX:
      case R_X86_64_REX_GOTPCRELX:
        break;

      case R_X86_64_64:
        if (DynamicLinking) {
          //
          // Dynamic section doesn't contain entries of this type.
          //
          DEBUG ((DEBUG_INFO, "Unsupported relocation type %02X\n", Type));
          ASSERT (FALSE);
        } else {
          *Ptr += Delta;
        }

        break;

      case R_X86_64_32:
        //
        // Dynamic section doesn't contain entries of this type.
        //
        DEBUG ((DEBUG_INFO, "Unsupported relocation type %02X\n", Type));
        ASSERT (FALSE);
        break;

      case R_X86_64_RELATIVE:
        if (DynamicLinking) {
          //
          // A: Represents the addend used to compute the value of the relocatable field.
          // B: Represents the base address at which a shared object has been loaded into memory during execution.
          //    Generally, a shared object is built with a 0 base virtual address, but the execution address will be different.
          //
          // B (Base Address) in ELF spec is slightly different:
          //   An executable or shared object file's base address (on platforms that support the concept) is calculated during
          //   execution from three values: the virtual memory load address, the maximum page size, and the lowest virtual address
          //   of a program's loadable segment. To compute the base address, one determines the memory address associated with the
          //   lowest p_vaddr value for a PT_LOAD segment. This address is truncated to the nearest multiple of the maximum page size.
          //   The corresponding p_vaddr value itself is also truncated to the nearest multiple of the maximum page size.
          //
          //   *** The base address is the difference between the truncated memory address and the truncated p_vaddr value. ***
          //
          // Delta in this function is B.
          //
          // Calculation: B + A
          //
          if (RelaType == SHT_RELA) {
            *Ptr = Delta + Rela->r_addend;
          } else {
            //
            // A is stored in the field of relocation for REL type.
            //
            *Ptr = Delta + *Ptr;
          }
        } else {
          //
          // non-Dynamic section doesn't contain entries of this type.
          //
          DEBUG ((DEBUG_INFO, "Unsupported relocation type %02X\n", Type));
          ASSERT (FALSE);
        }

        break;

      default:
        DEBUG ((DEBUG_INFO, "Unsupported relocation type %02X\n", Type));
    }
  }

  return EFI_SUCCESS;
}

/**
  Relocate the DYN type image.

  @param ElfCt                Point to image context.

  @retval EFI_SUCCESS      The relocation succeeds.
  @retval EFI_UNSUPPORTED  The image doesn't contain a dynamic section.
**/
EFI_STATUS
RelocateElf64Dynamic (
  IN    ELF_IMAGE_CONTEXT  *ElfCt
  )
{
  UINT32      Index;
  Elf64_Phdr  *Phdr;
  Elf64_Shdr  *DynShdr;
  Elf64_Shdr  *RelShdr;
  Elf64_Dyn   *Dyn;
  UINT64      RelaAddress;
  UINT64      RelaCount;
  UINT64      RelaSize;
  UINT64      RelaEntrySize;
  UINT64      RelaType;

  //
  // 1. Locate the dynamic section.
  //
  // If an object file participates in dynamic linking, its program header table
  // will have an element of type PT_DYNAMIC.
  // This ``segment'' contains the .dynamic section. A special symbol, _DYNAMIC,
  // labels the section, which contains an array of Elf32_Dyn or Elf64_Dyn.
  //
  DynShdr = NULL;
  for (Index = 0; Index < ElfCt->PhNum; Index++) {
    Phdr = GetElf64SegmentByIndex (ElfCt->FileBase, Index);
    ASSERT (Phdr != NULL);
    if (Phdr->p_type == PT_DYNAMIC) {
      //
      // Verify the existence of the dynamic section.
      //
      DynShdr = GetElf64SectionByRange (ElfCt->FileBase, Phdr->p_offset, Phdr->p_filesz);
      break;
    }
  }

  //
  // It's abnormal a DYN ELF doesn't contain a dynamic section.
  //
  ASSERT (DynShdr != NULL);
  if (DynShdr == NULL) {
    return EFI_UNSUPPORTED;
  }

  ASSERT (DynShdr->sh_type == SHT_DYNAMIC);
  ASSERT (DynShdr->sh_entsize >= sizeof (*Dyn));

  //
  // 2. Locate the relocation section from the dynamic section.
  //
  RelaAddress   = MAX_UINT64;
  RelaSize      = 0;
  RelaCount     = 0;
  RelaEntrySize = 0;
  RelaType      = 0;
  for ( Index = 0, Dyn = (Elf64_Dyn *)(ElfCt->FileBase + DynShdr->sh_offset)
        ; Index < DivU64x64Remainder (DynShdr->sh_size, DynShdr->sh_entsize, NULL)
        ; Index++, Dyn = ELF_NEXT_ENTRY (Elf64_Dyn, Dyn, DynShdr->sh_entsize)
        )
  {
    switch (Dyn->d_tag) {
      case DT_RELA:
      case DT_REL:
        //
        // DT_REL represent program virtual addresses.
        // A file's virtual addresses might not match the memory virtual addresses during execution.
        // When interpreting addresses contained in the dynamic structure, the dynamic linker computes actual addresses,
        // based on the original file value and the memory base address.
        // For consistency, files do not contain relocation entries to ``correct'' addresses in the dynamic structure.
        //
        RelaAddress = Dyn->d_un.d_ptr;
        RelaType    = (Dyn->d_tag == DT_RELA) ? SHT_RELA : SHT_REL;
        break;
      case DT_RELACOUNT:
      case DT_RELCOUNT:
        RelaCount = Dyn->d_un.d_val;
        break;
      case DT_RELENT:
      case DT_RELAENT:
        RelaEntrySize = Dyn->d_un.d_val;
        break;
      case DT_RELSZ:
      case DT_RELASZ:
        RelaSize = Dyn->d_un.d_val;
        break;
      default:
        break;
    }
  }

  if (RelaAddress == MAX_UINT64) {
    ASSERT (RelaCount     == 0);
    ASSERT (RelaEntrySize == 0);
    ASSERT (RelaSize      == 0);
    //
    // It's fine that a DYN ELF doesn't contain relocation section.
    //
    return EFI_SUCCESS;
  }

  //
  // Verify the existence of the relocation section.
  //
  RelShdr = NULL;
  for (Index = 0; Index < ElfCt->ShNum; Index++) {
    RelShdr = GetElf64SectionByIndex (ElfCt->FileBase, Index);
    ASSERT (RelShdr != NULL);
    if ((RelShdr->sh_addr == RelaAddress) && (RelShdr->sh_size == RelaSize)) {
      break;
    }

    RelShdr = NULL;
  }

  if (RelShdr == NULL) {
    return EFI_UNSUPPORTED;
  }

  ASSERT (RelShdr->sh_type == RelaType);
  ASSERT (RelShdr->sh_entsize == RelaEntrySize);

  //
  // 3. Process the relocation section.
  //
  ProcessRelocation64 (
    (Elf64_Rela *)(ElfCt->FileBase + RelShdr->sh_offset),
    RelShdr->sh_size,
    RelShdr->sh_entsize,
    RelShdr->sh_type,
    (UINTN)ElfCt->ImageAddress - (UINTN)ElfCt->PreferredImageAddress,
    TRUE
    );
  return EFI_SUCCESS;
}

/**
  Relocate all sections in a ELF image.

  @param[in]  ElfCt               ELF image context pointer.

  @retval EFI_UNSUPPORTED         Relocation is not supported.
  @retval EFI_SUCCESS             ELF image was relocated successfully.
**/
EFI_STATUS
RelocateElf64Sections  (
  IN    ELF_IMAGE_CONTEXT  *ElfCt
  )
{
  EFI_STATUS  Status;
  Elf64_Ehdr  *Ehdr;
  Elf64_Shdr  *RelShdr;
  Elf64_Shdr  *Shdr;
  UINT32      Index;
  UINTN       Delta;

  Ehdr = (Elf64_Ehdr *)ElfCt->FileBase;
  if (Ehdr->e_machine != EM_X86_64) {
    return EFI_UNSUPPORTED;
  }

  Delta             = (UINTN)ElfCt->ImageAddress - (UINTN)ElfCt->PreferredImageAddress;
  ElfCt->EntryPoint = (UINTN)(Ehdr->e_entry + Delta);

  //
  // 1. Relocate dynamic ELF using the relocation section pointed by dynamic section
  //
  if (Ehdr->e_type == ET_DYN) {
    DEBUG ((DEBUG_INFO, "DYN ELF: Relocate using dynamic sections...\n"));
    Status = RelocateElf64Dynamic (ElfCt);
    ASSERT_EFI_ERROR (Status);
    return Status;
  }

  //
  // 2. Executable ELF: Fix up the delta between actual image address and preferred image address.
  //
  //  Linker already fixed up EXEC ELF based on the preferred image address.
  //  A ELF loader in modern OS only loads it into the preferred image address.
  //  The below relocation is unneeded in that case.
  //  But the ELF loader in firmware supports to load the image to a different address.
  //  The below relocation is needed in this case.
  //
  DEBUG ((DEBUG_INFO, "EXEC ELF: Fix actual/preferred base address delta ...\n"));
  for ( Index = 0, RelShdr = (Elf64_Shdr *)(ElfCt->FileBase + Ehdr->e_shoff)
        ; Index < Ehdr->e_shnum
        ; Index++, RelShdr = ELF_NEXT_ENTRY (Elf64_Shdr, RelShdr, Ehdr->e_shentsize)
        )
  {
    if ((RelShdr->sh_type != SHT_REL) && (RelShdr->sh_type != SHT_RELA)) {
      continue;
    }

    Shdr = GetElf64SectionByIndex (ElfCt->FileBase, RelShdr->sh_info);
    if ((Shdr->sh_flags & SHF_ALLOC) == SHF_ALLOC) {
      //
      // Only fix up sections that occupy memory during process execution.
      //
      ProcessRelocation64 (
        (Elf64_Rela *)((UINT8 *)Ehdr + RelShdr->sh_offset),
        RelShdr->sh_size,
        RelShdr->sh_entsize,
        RelShdr->sh_type,
        Delta,
        FALSE
        );
    }
  }

  return EFI_SUCCESS;
}

/**
  Load ELF image which has 64-bit architecture.

  Caller should set Context.ImageAddress to a proper value, either pointing to
  a new allocated memory whose size equal to Context.ImageSize, or pointing
  to Context.PreferredImageAddress.

  @param[in]  ElfCt               ELF image context pointer.

  @retval EFI_SUCCESS         ELF binary is loaded successfully.
  @retval Others              Loading ELF binary fails.

**/
EFI_STATUS
LoadElf64Image (
  IN    ELF_IMAGE_CONTEXT  *ElfCt
  )
{
  Elf64_Ehdr  *Ehdr;
  Elf64_Phdr  *Phdr;
  UINT16      Index;
  UINTN       Delta;

  ASSERT (ElfCt != NULL);

  //
  // Per the sprit of ELF, loading to memory only consumes info from program headers.
  //
  Ehdr = (Elf64_Ehdr *)ElfCt->FileBase;

  for ( Index = 0, Phdr = (Elf64_Phdr *)(ElfCt->FileBase + Ehdr->e_phoff)
        ; Index < Ehdr->e_phnum
        ; Index++, Phdr = ELF_NEXT_ENTRY (Elf64_Phdr, Phdr, Ehdr->e_phentsize)
        )
  {
    //
    // Skip segments that don't require load (type tells, or size is 0)
    //
    if ((Phdr->p_type != PT_LOAD) ||
        (Phdr->p_memsz == 0))
    {
      continue;
    }

    //
    // The memory offset of segment relative to the image base
    // Note: CopyMem() does nothing when the dst equals to src.
    //
    Delta = (UINTN)Phdr->p_paddr - (UINTN)ElfCt->PreferredImageAddress;
    CopyMem (ElfCt->ImageAddress + Delta, ElfCt->FileBase + (UINTN)Phdr->p_offset, (UINTN)Phdr->p_filesz);
    ZeroMem (ElfCt->ImageAddress + Delta + (UINTN)Phdr->p_filesz, (UINTN)(Phdr->p_memsz - Phdr->p_filesz));
  }

  //
  // Relocate when new new image base is not the preferred image base.
  //
  if (ElfCt->ImageAddress != ElfCt->PreferredImageAddress) {
    RelocateElf64Sections (ElfCt);
  }

  return EFI_SUCCESS;
}
