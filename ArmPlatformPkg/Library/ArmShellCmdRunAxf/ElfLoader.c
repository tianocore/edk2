/** @file
*
*  Copyright (c) 2014, ARM Limited. All rights reserved.
*
*  This program and the accompanying materials
*  are licensed and made available under the terms and conditions of the BSD License
*  which accompanies this distribution.  The full text of the license may be found at
*  http://opensource.org/licenses/bsd-license.php
*
*  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
*  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
*
**/

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiLib.h>

#include "ArmShellCmdRunAxf.h"
#include "ElfLoader.h"
#include "elf_common.h"
#include "elf32.h"
#include "elf64.h"


// Put the functions the #ifdef. We only use the appropriate one for the platform.
// This prevents 'defined but not used' compiler warning.
#ifdef MDE_CPU_ARM
STATIC
BOOLEAN
IsArmElf (
  IN  CONST VOID *Buf
  )
{
  Elf32_Ehdr *Hdr = (Elf32_Ehdr*)Buf;

  if (Hdr->e_ident[EI_CLASS] != ELFCLASS32) {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_RUNAXF_ELFWRONGCLASS_32), gRunAxfHiiHandle);
    return FALSE;
  }

  if (Hdr->e_machine != EM_ARM) {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_RUNAXF_ELFWRONGMACH_32), gRunAxfHiiHandle);
    return FALSE;
  }

  // We don't currently check endianness of ELF data (hdr->e_ident[EI_DATA])

  return TRUE;
}
#elif defined(MDE_CPU_AARCH64)
STATIC
BOOLEAN
IsAarch64Elf (
  IN  CONST VOID *Buf
  )
{
  Elf64_Ehdr *Hdr = (Elf64_Ehdr*)Buf;

  if (Hdr->e_ident[EI_CLASS] != ELFCLASS64) {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_RUNAXF_ELFWRONGCLASS_64), gRunAxfHiiHandle);
    return FALSE;
  }

  if (Hdr->e_machine != EM_AARCH64) {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_RUNAXF_ELFWRONGMACH_64), gRunAxfHiiHandle);
    return FALSE;
  }

  // We don't currently check endianness of ELF data (hdr->e_ident[EI_DATA])

  return TRUE;
}
#endif // MDE_CPU_ARM , MDE_CPU_AARCH64


/**
 Support checking 32 and 64bit as the header could be valid, we might just
 not support loading it.
**/
STATIC
EFI_STATUS
ElfCheckHeader (
  IN  CONST VOID *Buf
  )
{
  Elf32_Ehdr *Hdr32 = (Elf32_Ehdr*)Buf;

  if (!IS_ELF (*Hdr32)) {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_RUNAXF_ELFMAGIC), gRunAxfHiiHandle);
    return EFI_INVALID_PARAMETER;
  }

  if (Hdr32->e_type != ET_EXEC) {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_RUNAXF_ELFNOTEXEC), gRunAxfHiiHandle);
    return EFI_INVALID_PARAMETER;
  }

  if (Hdr32->e_ident[EI_CLASS] == ELFCLASS32) {
    if ((Hdr32->e_phoff == 0) || (Hdr32->e_phentsize == 0) || (Hdr32->e_phnum == 0)) {
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_RUNAXF_ELFNOPROG), gRunAxfHiiHandle);
      return EFI_INVALID_PARAMETER;
    }

    if (Hdr32->e_flags != 0) {
      DEBUG ((EFI_D_INFO, "Warning: Wrong processor-specific flags, expected 0.\n"));
    }

    DEBUG ((EFI_D_INFO, "Entry point addr: 0x%lx\n", Hdr32->e_entry));
    DEBUG ((EFI_D_INFO, "Start of program headers: 0x%lx\n", Hdr32->e_phoff));
    DEBUG ((EFI_D_INFO, "Size of 1 program header: %d\n", Hdr32->e_phentsize));
    DEBUG ((EFI_D_INFO, "Number of program headers: %d\n", Hdr32->e_phnum));
  } else if (Hdr32->e_ident[EI_CLASS] == ELFCLASS64) {
      Elf64_Ehdr *Hdr64 = (Elf64_Ehdr*)Buf;

    if ((Hdr64->e_phoff == 0) || (Hdr64->e_phentsize == 0) || (Hdr64->e_phnum == 0)) {
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_RUNAXF_ELFNOPROG), gRunAxfHiiHandle);
      return EFI_INVALID_PARAMETER;
    }

    if (Hdr64->e_flags != 0) {
      DEBUG ((EFI_D_INFO, "Warning: Wrong processor-specific flags, expected 0.\n"));
    }

    DEBUG ((EFI_D_INFO, "Entry point addr: 0x%lx\n", Hdr64->e_entry));
    DEBUG ((EFI_D_INFO, "Start of program headers: 0x%lx\n", Hdr64->e_phoff));
    DEBUG ((EFI_D_INFO, "Size of 1 program header: %d\n", Hdr64->e_phentsize));
    DEBUG ((EFI_D_INFO, "Number of program headers: %d\n", Hdr64->e_phnum));
  } else {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_RUNAXF_ELFWRONGCLASS), gRunAxfHiiHandle);
    return EFI_INVALID_PARAMETER;
  }

  return EFI_SUCCESS;
}


/**
 Load an ELF segment into memory.

 This function assumes the ELF file is valid.
 This function is meant to be called for PT_LOAD type segments only.
**/
STATIC
EFI_STATUS
ElfLoadSegment (
  IN  CONST VOID  *ElfImage,
  IN  CONST VOID  *PHdr,
  IN  LIST_ENTRY  *LoadList
  )
{
  VOID             *FileSegment;
  VOID             *MemSegment;
  UINTN             ExtraZeroes;
  UINTN             ExtraZeroesCount;
  RUNAXF_LOAD_LIST *LoadNode;

#ifdef MDE_CPU_ARM
  Elf32_Phdr  *ProgramHdr;
  ProgramHdr = (Elf32_Phdr *)PHdr;
#elif defined(MDE_CPU_AARCH64)
  Elf64_Phdr  *ProgramHdr;
  ProgramHdr = (Elf64_Phdr *)PHdr;
#endif

  ASSERT (ElfImage != NULL);
  ASSERT (ProgramHdr != NULL);

  FileSegment = (VOID *)((UINTN)ElfImage + ProgramHdr->p_offset);
  MemSegment = (VOID *)ProgramHdr->p_vaddr;

  // If the segment's memory size p_memsz is larger than the file size p_filesz,
  // the "extra" bytes are defined to hold the value 0 and to follow the
  // segment's initialised area.
  // This is typically the case for the .bss segment.
  // The file size may not be larger than the memory size.
  if (ProgramHdr->p_filesz > ProgramHdr->p_memsz) {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_RUNAXF_ELFBADFORMAT), gRunAxfHiiHandle);
    return EFI_INVALID_PARAMETER;
  }

  // Load the segment in memory.
  if (ProgramHdr->p_filesz != 0) {
    DEBUG ((EFI_D_INFO, "Loading segment from 0x%lx to 0x%lx (size = %ld)\n",
                 FileSegment, MemSegment, ProgramHdr->p_filesz));

    LoadNode = AllocateRuntimeZeroPool (sizeof (RUNAXF_LOAD_LIST));
    if (LoadNode == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }
    LoadNode->MemOffset  = (UINTN)MemSegment;
    LoadNode->FileOffset = (UINTN)FileSegment;
    LoadNode->Length     = (UINTN)ProgramHdr->p_filesz;
    InsertTailList (LoadList, &LoadNode->Link);
  }

  ExtraZeroes = ((UINTN)MemSegment + ProgramHdr->p_filesz);
  ExtraZeroesCount = ProgramHdr->p_memsz - ProgramHdr->p_filesz;
  DEBUG ((EFI_D_INFO, "Completing segment with %d zero bytes.\n", ExtraZeroesCount));
  if (ExtraZeroesCount > 0) {
    // Extra Node to add the Zeroes.
    LoadNode = AllocateRuntimeZeroPool (sizeof (RUNAXF_LOAD_LIST));
    if (LoadNode == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }
    LoadNode->MemOffset  = (UINTN)ExtraZeroes;
    LoadNode->Zeroes     = TRUE;
    LoadNode->Length     = ExtraZeroesCount;
    InsertTailList (LoadList, &LoadNode->Link);
  }

  return EFI_SUCCESS;
}


/**
 Check that the ELF File Header is valid and Machine type supported.

 Not all information is checked in the ELF header, only the stuff that
 matters to us in our simplified ELF loader.

 @param[in] ElfImage  Address of the ELF file to check.

 @retval EFI_SUCCESS on success.
 @retval EFI_INVALID_PARAMETER if the header is invalid.
 @retval EFI_UNSUPPORTED if the file type/platform is not supported.
**/
EFI_STATUS
ElfCheckFile (
  IN  CONST VOID *ElfImage
  )
{
  EFI_STATUS Status;

  ASSERT (ElfImage != NULL);

  // Check that the ELF header is valid.
  Status = ElfCheckHeader (ElfImage);
  if (EFI_ERROR(Status)) {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_RUNAXF_ELFBADHEADER), gRunAxfHiiHandle);
    return EFI_INVALID_PARAMETER;
  }

#ifdef MDE_CPU_ARM
  if (IsArmElf (ElfImage)) {
    return EFI_SUCCESS;
  }
#elif defined(MDE_CPU_AARCH64)
  if (IsAarch64Elf (ElfImage)) {
    return EFI_SUCCESS;
  }
#endif

  ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_RUNAXF_BAD_ARCH), gRunAxfHiiHandle);
  return EFI_UNSUPPORTED;
}


/**
  Load a ELF file.

  @param[in] ElfImage       Address of the ELF file in memory.

  @param[out] EntryPoint    Will be filled with the ELF entry point address.

  @param[out] ImageSize     Will be filled with the ELF size in memory. This will
                            effectively be equal to the sum of the segments sizes.

  This functon assumes the header is valid and supported as checked with
  ElfCheckFile().

  @retval EFI_SUCCESS on success.
  @retval EFI_INVALID_PARAMETER if the ELF file is invalid.
**/
EFI_STATUS
ElfLoadFile (
  IN  CONST VOID   *ElfImage,
  OUT VOID        **EntryPoint,
  OUT LIST_ENTRY   *LoadList
  )
{
  EFI_STATUS    Status;
  UINT8        *ProgramHdr;
  UINTN         Index;
  UINTN         ImageSize;

#ifdef MDE_CPU_ARM
  Elf32_Ehdr   *ElfHdr;
  Elf32_Phdr   *ProgramHdrPtr;

  ElfHdr = (Elf32_Ehdr*)ElfImage;
#elif defined(MDE_CPU_AARCH64)
  Elf64_Ehdr   *ElfHdr;
  Elf64_Phdr   *ProgramHdrPtr;

  ElfHdr = (Elf64_Ehdr*)ElfImage;
#endif

  ASSERT (ElfImage   != NULL);
  ASSERT (EntryPoint != NULL);
  ASSERT (LoadList   != NULL);

  ProgramHdr = (UINT8*)ElfImage + ElfHdr->e_phoff;
  DEBUG ((EFI_D_INFO, "ELF program header entry : 0x%lx\n", ProgramHdr));

  ImageSize = 0;

  // Load every loadable ELF segment into memory.
  for (Index = 0; Index < ElfHdr->e_phnum; ++Index) {

#ifdef MDE_CPU_ARM
    ProgramHdrPtr = (Elf32_Phdr*)ProgramHdr;
#elif defined(MDE_CPU_AARCH64)
    ProgramHdrPtr = (Elf64_Phdr*)ProgramHdr;
#endif

    // Only consider PT_LOAD type segments, ignore others.
    if (ProgramHdrPtr->p_type == PT_LOAD) {
      Status = ElfLoadSegment (ElfImage, (VOID *)ProgramHdrPtr, LoadList);
      if (EFI_ERROR (Status)) {
        ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_RUNAXF_ELFFAILSEG), gRunAxfHiiHandle);
        return EFI_INVALID_PARAMETER;
      }
      ImageSize += ProgramHdrPtr->p_memsz;
    }
    ProgramHdr += ElfHdr->e_phentsize;
  }

  if (ImageSize == 0) {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_RUNAXF_ELFNOSEG), gRunAxfHiiHandle);
    return EFI_INVALID_PARAMETER;
  }

  // Return the entry point specified in the ELF header.
  *EntryPoint = (void*)ElfHdr->e_entry;

  return EFI_SUCCESS;
}
