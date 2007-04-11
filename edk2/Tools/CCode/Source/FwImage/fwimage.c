/*++

Copyright (c) 2004 - 2007, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

    fwimage.c

Abstract:

    Converts a pe32+ image to an FW image type

--*/

#include "WinNtInclude.h"

//
// List of OS and CPU which support ELF to PE conversion
//
#if defined(linux)
#if defined (__i386__) || defined(__x86_64__)
#define HAVE_ELF
#endif
#endif

#ifndef __GNUC__
#include <windows.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef HAVE_ELF
#include <elf.h>
#endif

#include <Common/UefiBaseTypes.h>
#include <Common/EfiImage.h>

#include "CommonLib.h"
#include "EfiUtilityMsgs.c"

//
// Version of this utility
//
#define UTILITY_NAME "FwImage"
#define UTILITY_MAJOR_VERSION 1
#define UTILITY_MINOR_VERSION 0

#ifdef __GNUC__
typedef unsigned long ULONG;
typedef unsigned char UCHAR;
typedef unsigned char *PUCHAR;
typedef unsigned short USHORT;
#endif

PUCHAR            InImageName;

static
void
Version (
  VOID
  )
{
  printf ("%s v%d.%d -EDK Utility for Converting a pe32+ image to an FW image type.\n", UTILITY_NAME, UTILITY_MAJOR_VERSION, UTILITY_MINOR_VERSION);
  printf ("Copyright (c) 1999-2006 Intel Corporation. All rights reserved.\n");
}


VOID
Usage (
  VOID
  )
{
  Version();
  printf ("\nUsage: " UTILITY_NAME "  {-t time-date} {-h|--help|-?|/?|-V|--version} \n\
         [BASE|SEC|PEI_CORE|PEIM|DXE_CORE|DXE_DRIVER|DXE_RUNTIME_DRIVER|\n\
         DXE_SAL_DRIVER|DXE_SMM_DRIVER|TOOL|UEFI_DRIVER|UEFI_APPLICATION|\n\
         USER_DEFINED] peimage [outimage]\n");
}

static
STATUS
FCopyFile (
  FILE    *in,
  FILE    *out
  )
{
  ULONG filesize;
  ULONG offset;
  ULONG length;
  UCHAR Buffer[8 * 1024];

  fseek (in, 0, SEEK_END);
  filesize = ftell (in);

  fseek (in, 0, SEEK_SET);
  fseek (out, 0, SEEK_SET);

  offset = 0;
  while (offset < filesize) {
    length = sizeof (Buffer);
    if (filesize - offset < length) {
      length = filesize - offset;
    }

    fread (Buffer, length, 1, in);
    fwrite (Buffer, length, 1, out);
    offset += length;
  }

  if ((ULONG) ftell (out) != filesize) {
    Error (NULL, 0, 0, "write error", NULL);
    return STATUS_ERROR;
  }

  return STATUS_SUCCESS;
}

static
STATUS
FReadFile (
  FILE    *in,
  VOID    **Buffer,
  UINTN   *Length
  )
{
  fseek (in, 0, SEEK_END);
  *Length = ftell (in);
  *Buffer = malloc (*Length);
  fseek (in, 0, SEEK_SET);
  fread (*Buffer, *Length, 1, in);
  return STATUS_SUCCESS;
}

static
STATUS
FWriteFile (
  FILE    *out,
  VOID    *Buffer,
  UINTN   Length
  )
{
  fseek (out, 0, SEEK_SET);
  fwrite (Buffer, Length, 1, out);
  if ((ULONG) ftell (out) != Length) {
    Error (NULL, 0, 0, "write error", NULL);
    return STATUS_ERROR;
  }
  free (Buffer);
  return STATUS_SUCCESS;
}

#ifdef HAVE_ELF
INTN
IsElfHeader(
  UINT8	*FileBuffer
)
{
  return (FileBuffer[EI_MAG0] == ELFMAG0
	  && FileBuffer[EI_MAG1] == ELFMAG1
	  && FileBuffer[EI_MAG2] == ELFMAG2
	  && FileBuffer[EI_MAG3] == ELFMAG3);
}

typedef Elf32_Shdr Elf_Shdr;
typedef Elf32_Ehdr Elf_Ehdr;
typedef Elf32_Rel Elf_Rel;
typedef Elf32_Sym Elf_Sym;
#define ELFCLASS ELFCLASS32
#define ELF_R_TYPE(r) ELF32_R_TYPE(r)
#define ELF_R_SYM(r) ELF32_R_SYM(r)

//
// Well known ELF structures.
//
Elf_Ehdr *Ehdr;
Elf_Shdr *ShdrBase;

//
// PE section alignment.
//
const UINT32 CoffAlignment = 0x20;
const UINT32 CoffNbrSections = 4;

//
// Current offset in coff file.
//
UINT32 CoffOffset;

//
// Result Coff file in memory.
//
UINT8 *CoffFile;

//
// Offset in Coff file of headers and sections.
//
UINT32 NtHdrOffset;
UINT32 TableOffset;
UINT32 TextOffset;
UINT32 DataOffset;
UINT32 RelocOffset;

//
// ELF sections to offset in Coff file.
//
UINT32 *CoffSectionsOffset;

EFI_IMAGE_BASE_RELOCATION *CoffBaseRel;
UINT16 *CoffEntryRel;

UINT32
CoffAlign(
  UINT32 Offset
  )
{
  return (Offset + CoffAlignment - 1) & ~(CoffAlignment - 1);
}

Elf_Shdr *
GetShdrByIndex(
  UINT32 Num
  )
{
  if (Num >= Ehdr->e_shnum)
    return NULL;
  return (Elf_Shdr*)((UINT8*)ShdrBase + Num * Ehdr->e_shentsize);
}

INTN
CheckElfHeader(
  VOID
  )
{
  //
  // Note: Magic has already been tested.
  //
  if (Ehdr->e_ident[EI_CLASS] != ELFCLASS)
    return 0;
  if (Ehdr->e_ident[EI_DATA] != ELFDATA2LSB)
    return 0;
  if (Ehdr->e_type != ET_EXEC)
    return 0;
  if (Ehdr->e_machine != EM_386)
    return 0;
  if (Ehdr->e_version != EV_CURRENT)
    return 0;

  //
  // Find the section header table
  // 
  ShdrBase = (Elf_Shdr *)((UINT8 *)Ehdr + Ehdr->e_shoff);

  CoffSectionsOffset = (UINT32 *)malloc(Ehdr->e_shnum * sizeof (UINT32));

  memset(CoffSectionsOffset, 0, Ehdr->e_shnum * sizeof(UINT32));
  return 1;
}

int
IsTextShdr(
  Elf_Shdr *Shdr
  )
{
  return (Shdr->sh_flags & (SHF_WRITE | SHF_ALLOC)) == SHF_ALLOC;
}

int
IsDataShdr(
  Elf_Shdr *Shdr
  )
{
  return (Shdr->sh_flags & (SHF_WRITE | SHF_ALLOC)) == (SHF_ALLOC | SHF_WRITE);
}

void
CreateSectionHeader(
  const char *Name,
  UINT32     Offset,
  UINT32     Size,
  UINT32     Flags
  )
{
  EFI_IMAGE_SECTION_HEADER *Hdr;
  Hdr = (EFI_IMAGE_SECTION_HEADER*)(CoffFile + TableOffset);

  strcpy(Hdr->Name, Name);
  Hdr->Misc.VirtualSize = Size;
  Hdr->VirtualAddress = Offset;
  Hdr->SizeOfRawData = Size;
  Hdr->PointerToRawData = Offset;
  Hdr->PointerToRelocations = 0;
  Hdr->PointerToLinenumbers = 0;
  Hdr->NumberOfRelocations = 0;
  Hdr->NumberOfLinenumbers = 0;
  Hdr->Characteristics = Flags;

  TableOffset += sizeof (EFI_IMAGE_SECTION_HEADER);
}

void
ScanSections(
  VOID
  )
{
  UINT32 i;
  EFI_IMAGE_DOS_HEADER *DosHdr;
  EFI_IMAGE_NT_HEADERS *NtHdr;
  UINT32 CoffEntry = 0;

  CoffOffset = 0;

  //
  // Coff file start with a DOS header.
  //
  CoffOffset = sizeof(EFI_IMAGE_DOS_HEADER) + 0x40;
  NtHdrOffset = CoffOffset;
  CoffOffset += sizeof(EFI_IMAGE_NT_HEADERS);
  TableOffset = CoffOffset;
  CoffOffset += CoffNbrSections * sizeof(EFI_IMAGE_SECTION_HEADER);

  //
  // First text sections.
  //
  CoffOffset = CoffAlign(CoffOffset);
  TextOffset = CoffOffset;
  for (i = 0; i < Ehdr->e_shnum; i++) {
    Elf_Shdr *shdr = GetShdrByIndex(i);
    if (IsTextShdr(shdr)) {
      //
      // Align the coff offset to meet with the alignment requirement of section
      // itself.
      // 
      if ((shdr->sh_addralign != 0) && (shdr->sh_addralign != 1)) {
        CoffOffset = (CoffOffset + shdr->sh_addralign - 1) & ~(shdr->sh_addralign - 1);
      }

      /* Relocate entry.  */
      if ((Ehdr->e_entry >= shdr->sh_addr) && 
          (Ehdr->e_entry < shdr->sh_addr + shdr->sh_size)) {
        CoffEntry = CoffOffset + Ehdr->e_entry - shdr->sh_addr;
      }
      CoffSectionsOffset[i] = CoffOffset;
      CoffOffset += shdr->sh_size;
    }
  }
  CoffOffset = CoffAlign(CoffOffset);

  //
  //  Then data sections.
  //
  DataOffset = CoffOffset;
  for (i = 0; i < Ehdr->e_shnum; i++) {
    Elf_Shdr *shdr = GetShdrByIndex(i);
    if (IsDataShdr(shdr)) {
      //
      // Align the coff offset to meet with the alignment requirement of section
      // itself.
      // 
      if ((shdr->sh_addralign != 0) && (shdr->sh_addralign != 1)) {
        CoffOffset = (CoffOffset + shdr->sh_addralign - 1) & ~(shdr->sh_addralign - 1);
      }
     
      CoffSectionsOffset[i] = CoffOffset;
      CoffOffset += shdr->sh_size;
    }
  }
  CoffOffset = CoffAlign(CoffOffset);

  RelocOffset = CoffOffset;  

  //
  // Allocate base Coff file.  Will be expanded later for relocations. 
  //
  CoffFile = (UINT8 *)malloc(CoffOffset);
  memset(CoffFile, 0, CoffOffset);

  //
  // Fill headers.
  //
  DosHdr = (EFI_IMAGE_DOS_HEADER *)CoffFile;
  DosHdr->e_magic = EFI_IMAGE_DOS_SIGNATURE;
  DosHdr->e_lfanew = NtHdrOffset;

  NtHdr = (EFI_IMAGE_NT_HEADERS*)(CoffFile + NtHdrOffset);

  NtHdr->Signature = EFI_IMAGE_NT_SIGNATURE;

  NtHdr->FileHeader.Machine = EFI_IMAGE_MACHINE_IA32;
  NtHdr->FileHeader.NumberOfSections = CoffNbrSections;
  NtHdr->FileHeader.TimeDateStamp = time(NULL);
  NtHdr->FileHeader.PointerToSymbolTable = 0;
  NtHdr->FileHeader.NumberOfSymbols = 0;
  NtHdr->FileHeader.SizeOfOptionalHeader = sizeof(NtHdr->OptionalHeader);
  NtHdr->FileHeader.Characteristics = EFI_IMAGE_FILE_EXECUTABLE_IMAGE
    | EFI_IMAGE_FILE_LINE_NUMS_STRIPPED
    | EFI_IMAGE_FILE_LOCAL_SYMS_STRIPPED
    | EFI_IMAGE_FILE_32BIT_MACHINE;
  
  NtHdr->OptionalHeader.Magic = EFI_IMAGE_NT_OPTIONAL_HDR32_MAGIC;
  NtHdr->OptionalHeader.SizeOfCode = DataOffset - TextOffset;
  NtHdr->OptionalHeader.SizeOfInitializedData = RelocOffset - DataOffset;
  NtHdr->OptionalHeader.SizeOfUninitializedData = 0;
  NtHdr->OptionalHeader.AddressOfEntryPoint = CoffEntry;
  NtHdr->OptionalHeader.BaseOfCode = TextOffset;

  NtHdr->OptionalHeader.BaseOfData = DataOffset;
  NtHdr->OptionalHeader.ImageBase = 0;
  NtHdr->OptionalHeader.SectionAlignment = CoffAlignment;
  NtHdr->OptionalHeader.FileAlignment = CoffAlignment;
  NtHdr->OptionalHeader.SizeOfImage = 0;

  NtHdr->OptionalHeader.SizeOfHeaders = TextOffset;
  NtHdr->OptionalHeader.NumberOfRvaAndSizes = EFI_IMAGE_NUMBER_OF_DIRECTORY_ENTRIES;

  //
  // Section headers.
  //
  CreateSectionHeader (".text", TextOffset, DataOffset - TextOffset,
		       EFI_IMAGE_SCN_CNT_CODE
		       | EFI_IMAGE_SCN_MEM_EXECUTE
		       | EFI_IMAGE_SCN_MEM_READ);
  CreateSectionHeader (".data", DataOffset, RelocOffset - DataOffset,
		       EFI_IMAGE_SCN_CNT_INITIALIZED_DATA
		       | EFI_IMAGE_SCN_MEM_WRITE
		       | EFI_IMAGE_SCN_MEM_READ);
}

void
WriteSections(
  int	 (*Filter)(Elf_Shdr *)
  )
{
  UINT32 Idx;

  //
  // First: copy sections.
  //
  for (Idx = 0; Idx < Ehdr->e_shnum; Idx++) {
    Elf_Shdr *Shdr = GetShdrByIndex(Idx);
    if ((*Filter)(Shdr)) {
      switch (Shdr->sh_type) {
      case SHT_PROGBITS:
	/* Copy.  */
	memcpy(CoffFile + CoffSectionsOffset[Idx],
	       (UINT8*)Ehdr + Shdr->sh_offset,
	       Shdr->sh_size);
	break;
      case SHT_NOBITS:
	memset(CoffFile + CoffSectionsOffset[Idx], 0, Shdr->sh_size);
	break;
      default:
	Error (NULL, 0, 0, InImageName, "unhandle section type %x",
	       (UINTN)Shdr->sh_type);
      }
    }
  }

  //
  // Second: apply relocations.
  //
  for (Idx = 0; Idx < Ehdr->e_shnum; Idx++) {
    Elf_Shdr *RelShdr = GetShdrByIndex(Idx);
    if (RelShdr->sh_type != SHT_REL)
      continue;
    Elf_Shdr *SecShdr = GetShdrByIndex(RelShdr->sh_info);
    UINT32 SecOffset = CoffSectionsOffset[RelShdr->sh_info];
    if (RelShdr->sh_type == SHT_REL && (*Filter)(SecShdr)) {
      UINT32 RelIdx;
      Elf_Shdr *SymtabShdr = GetShdrByIndex(RelShdr->sh_link);
      UINT8 *Symtab = (UINT8*)Ehdr + SymtabShdr->sh_offset;

      for (RelIdx = 0; RelIdx < RelShdr->sh_size; RelIdx += RelShdr->sh_entsize) {
	Elf_Rel *Rel = (Elf_Rel *)((UINT8*)Ehdr + RelShdr->sh_offset + RelIdx);
	Elf_Sym *Sym = (Elf_Sym *)
	  (Symtab + ELF_R_SYM(Rel->r_info) * SymtabShdr->sh_entsize);
	Elf_Shdr *SymShdr;
	UINT8 *Targ;

	if (Sym->st_shndx == SHN_UNDEF
	    || Sym->st_shndx == SHN_ABS
	    || Sym->st_shndx > Ehdr->e_shnum) {
	  Error (NULL, 0, 0, InImageName, "bad symbol definition");
	}
	SymShdr = GetShdrByIndex(Sym->st_shndx);

	//
	// Note: r_offset in a memory address.
	//  Convert it to a pointer in the coff file.
	//
	Targ = CoffFile + SecOffset + (Rel->r_offset - SecShdr->sh_addr);

	switch (ELF_R_TYPE(Rel->r_info)) {
	case R_386_NONE:
	  break;
	case R_386_32:
	  //
	  // Absolute relocation.
	  //
	  *(UINT32 *)Targ = *(UINT32 *)Targ - SymShdr->sh_addr
	    + CoffSectionsOffset[Sym->st_shndx];
	  break;
	case R_386_PC32:
	  //
	  // Relative relocation: Symbol - Ip + Addend
	  //
	  *(UINT32 *)Targ = *(UINT32 *)Targ
	    + (CoffSectionsOffset[Sym->st_shndx] - SymShdr->sh_addr)
	    - (SecOffset - SecShdr->sh_addr);
	  break;
	default:
	  Error (NULL, 0, 0, InImageName, "unhandled relocation type %x",
		 ELF_R_TYPE(Rel->r_info));
	}
      }
    }
  }
}

void
CoffAddFixupEntry(
  UINT16 Val
  )
{
  *CoffEntryRel = Val;
  CoffEntryRel++;
  CoffBaseRel->SizeOfBlock += 2;
  CoffOffset += 2;
}

void
CoffAddFixup(
  UINT32 Offset,
  UINT8  Type
  )
{
  if (CoffBaseRel == NULL
      || CoffBaseRel->VirtualAddress != (Offset & ~0xfff)) {
    if (CoffBaseRel != NULL) {
      //
      // Add a null entry (is it required ?)
      //
      CoffAddFixupEntry (0);
      //
      // Pad for alignment.
      //
      if (CoffOffset % 4 != 0)
	CoffAddFixupEntry (0);
    }
      
    CoffFile = realloc
      (CoffFile,
       CoffOffset + sizeof(EFI_IMAGE_BASE_RELOCATION) + 2*0x1000);
    memset(CoffFile + CoffOffset, 0,
	   sizeof(EFI_IMAGE_BASE_RELOCATION) + 2*0x1000);

    CoffBaseRel = (EFI_IMAGE_BASE_RELOCATION*)(CoffFile + CoffOffset);
    CoffBaseRel->VirtualAddress = Offset & ~0xfff;
    CoffBaseRel->SizeOfBlock = sizeof(EFI_IMAGE_BASE_RELOCATION);

    CoffEntryRel = (UINT16 *)(CoffBaseRel + 1);
    CoffOffset += sizeof(EFI_IMAGE_BASE_RELOCATION);
  }

  //
  // Fill the entry.
  //
  CoffAddFixupEntry((Type << 12) | (Offset & 0xfff));
}

void
WriteRelocations(
  VOID
  )
{
  UINT32 Idx;
  EFI_IMAGE_NT_HEADERS *NtHdr;
  EFI_IMAGE_DATA_DIRECTORY *Dir;

  for (Idx = 0; Idx < Ehdr->e_shnum; Idx++) {
    Elf_Shdr *RelShdr = GetShdrByIndex(Idx);
    if (RelShdr->sh_type == SHT_REL) {
      Elf_Shdr *SecShdr = GetShdrByIndex(RelShdr->sh_info);
      if (IsTextShdr(SecShdr) || IsDataShdr(SecShdr)) {
	UINT32 RelIdx;
	for (RelIdx = 0; RelIdx < RelShdr->sh_size; RelIdx += RelShdr->sh_entsize) {
	  Elf_Rel *Rel = (Elf_Rel *)
	    ((UINT8*)Ehdr + RelShdr->sh_offset + RelIdx);
	  switch (ELF_R_TYPE(Rel->r_info)) {
	  case R_386_NONE:
	  case R_386_PC32:
	    break;
	  case R_386_32:
	    CoffAddFixup(CoffSectionsOffset[RelShdr->sh_info]
			 + (Rel->r_offset - SecShdr->sh_addr),
			 EFI_IMAGE_REL_BASED_HIGHLOW);
	    break;
	  default:
	    Error (NULL, 0, 0, InImageName, "unhandled relocation type %x",
		   ELF_R_TYPE(Rel->r_info));
	  }
	}
      }
    }
  }

  //
  // Pad by adding empty entries. 
  //
  while (CoffOffset & (CoffAlignment - 1)) {
    CoffAddFixupEntry(0);
  }

  CreateSectionHeader (".reloc", RelocOffset, CoffOffset - RelocOffset,
		       EFI_IMAGE_SCN_CNT_INITIALIZED_DATA
		       | EFI_IMAGE_SCN_MEM_DISCARDABLE
		       | EFI_IMAGE_SCN_MEM_READ);

  NtHdr = (EFI_IMAGE_NT_HEADERS *)(CoffFile + NtHdrOffset);
  Dir = &NtHdr->OptionalHeader.DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY_BASERELOC];
  Dir->VirtualAddress = RelocOffset;
  Dir->Size = CoffOffset - RelocOffset;
}

void
WriteDebug(
  VOID
  )
{
  UINT32 Len = strlen(InImageName) + 1;
  UINT32 DebugOffset = CoffOffset;
  EFI_IMAGE_NT_HEADERS *NtHdr;
  EFI_IMAGE_DATA_DIRECTORY *DataDir;
  EFI_IMAGE_DEBUG_DIRECTORY_ENTRY *Dir;
  EFI_IMAGE_DEBUG_CODEVIEW_NB10_ENTRY *Nb10;

  CoffOffset += sizeof(EFI_IMAGE_DEBUG_DIRECTORY_ENTRY)
    + sizeof(EFI_IMAGE_DEBUG_CODEVIEW_NB10_ENTRY)
    + Len;
  CoffOffset = CoffAlign(CoffOffset);

  CoffFile = realloc
    (CoffFile, CoffOffset);
  memset(CoffFile + DebugOffset, 0, CoffOffset - DebugOffset);
  
  Dir = (EFI_IMAGE_DEBUG_DIRECTORY_ENTRY*)(CoffFile + DebugOffset);
  Dir->Type = EFI_IMAGE_DEBUG_TYPE_CODEVIEW;
  Dir->SizeOfData = sizeof(EFI_IMAGE_DEBUG_DIRECTORY_ENTRY) + Len;
  Dir->RVA = DebugOffset + sizeof(EFI_IMAGE_DEBUG_DIRECTORY_ENTRY);
  Dir->FileOffset = DebugOffset + sizeof(EFI_IMAGE_DEBUG_DIRECTORY_ENTRY);
  
  Nb10 = (EFI_IMAGE_DEBUG_CODEVIEW_NB10_ENTRY*)(Dir + 1);
  Nb10->Signature = CODEVIEW_SIGNATURE_NB10;
  strcpy ((PUCHAR)(Nb10 + 1), InImageName);

  CreateSectionHeader (".debug", DebugOffset, CoffOffset - DebugOffset,
		       EFI_IMAGE_SCN_CNT_INITIALIZED_DATA
		       | EFI_IMAGE_SCN_MEM_DISCARDABLE
		       | EFI_IMAGE_SCN_MEM_READ);

  NtHdr = (EFI_IMAGE_NT_HEADERS *)(CoffFile + NtHdrOffset);
  DataDir = &NtHdr->OptionalHeader.DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY_DEBUG];
  DataDir->VirtualAddress = DebugOffset;
  DataDir->Size = CoffOffset - DebugOffset;
}

void
ConvertElf (
  UINT8	**FileBuffer,
  UINTN *FileLength
  )
{
  EFI_IMAGE_NT_HEADERS *NtHdr;

  //
  // Check header, read section table.
  //
  Ehdr = (Elf32_Ehdr*)*FileBuffer;
  if (!CheckElfHeader())
    return;

  //
  // Compute sections new address.
  //
  ScanSections();

  //
  // Write and relocate sections.
  //
  WriteSections(IsTextShdr);
  WriteSections(IsDataShdr);

  //
  // Translate and write relocations.
  //
  WriteRelocations();

  //
  // Write debug info.
  //
  WriteDebug();

  NtHdr = (EFI_IMAGE_NT_HEADERS *)(CoffFile + NtHdrOffset);
  NtHdr->OptionalHeader.SizeOfImage = CoffOffset;

  //
  // Replace.
  //
  free(*FileBuffer);
  *FileBuffer = CoffFile;
  *FileLength = CoffOffset;
}
#endif // HAVE_ELF

int
main (
  int  argc,
  char *argv[]
  )
/*++

Routine Description:

  Main function.

Arguments:

  argc - Number of command line parameters.
  argv - Array of pointers to command line parameter strings.

Returns:
  STATUS_SUCCESS - Utility exits successfully.
  STATUS_ERROR   - Some error occurred during execution.

--*/
{
  ULONG             Type;
  PUCHAR            Ext;
  PUCHAR            p;
  PUCHAR            pe;
  PUCHAR            OutImageName;
  UCHAR             outname[500];
  FILE              *fpIn;
  FILE              *fpOut;
  VOID              *ZeroBuffer;
  EFI_IMAGE_DOS_HEADER  *DosHdr;
  EFI_IMAGE_NT_HEADERS  *PeHdr;
  EFI_IMAGE_OPTIONAL_HEADER32  *Optional32;
  EFI_IMAGE_OPTIONAL_HEADER64  *Optional64;
  time_t            TimeStamp;
  struct tm         TimeStruct;
  EFI_IMAGE_DOS_HEADER  BackupDosHdr;
  ULONG             Index;
  ULONG             Index1;
  ULONG             Index2;
  ULONG             Index3;
  BOOLEAN           TimeStampPresent;
  UINTN                                 AllignedRelocSize;
  UINTN                                 Delta;
  EFI_IMAGE_SECTION_HEADER              *SectionHeader;
  UINT8      *FileBuffer;
  UINTN      FileLength;
  RUNTIME_FUNCTION  *RuntimeFunction;
  UNWIND_INFO       *UnwindInfo;

  SetUtilityName (UTILITY_NAME);
  //
  // Assign to fix compile warning
  //
  OutImageName      = NULL;
  Type              = 0;
  Ext               = 0;
  TimeStamp         = 0;
  TimeStampPresent  = FALSE;

  if (argc == 1) {
    Usage();
    return STATUS_ERROR;
  }
  
  if ((strcmp(argv[1], "-h") == 0) || (strcmp(argv[1], "--help") == 0) ||
      (strcmp(argv[1], "-?") == 0) || (strcmp(argv[1], "/?") == 0)) {
    Usage();
    return STATUS_ERROR;
  }
  
  if ((strcmp(argv[1], "-V") == 0) || (strcmp(argv[1], "--version") == 0)) {
    Version();
    return STATUS_ERROR;
  }
 
  //
  // Look for -t time-date option first. If the time is "0", then
  // skip it.
  //
  if ((argc > 2) && !strcmp (argv[1], "-t")) {
    TimeStampPresent = TRUE;
    if (strcmp (argv[2], "0") != 0) {
      //
      // Convert the string to a value
      //
      memset ((char *) &TimeStruct, 0, sizeof (TimeStruct));
      if (sscanf(
          argv[2], "%d/%d/%d,%d:%d:%d",
          &TimeStruct.tm_mon,   /* months since January - [0,11] */
          &TimeStruct.tm_mday,  /* day of the month - [1,31] */
          &TimeStruct.tm_year,  /* years since 1900 */
          &TimeStruct.tm_hour,  /* hours since midnight - [0,23] */
          &TimeStruct.tm_min,   /* minutes after the hour - [0,59] */
          &TimeStruct.tm_sec    /* seconds after the minute - [0,59] */
            ) != 6) {
        Error (NULL, 0, 0, argv[2], "failed to convert to mm/dd/yyyy,hh:mm:ss format");
        return STATUS_ERROR;
      }
      //
      // Now fixup some of the fields
      //
      TimeStruct.tm_mon--;
      TimeStruct.tm_year -= 1900;
      //
      // Sanity-check values?
      // Convert
      //
      TimeStamp = mktime (&TimeStruct);
      if (TimeStamp == (time_t) - 1) {
        Error (NULL, 0, 0, argv[2], "failed to convert time");
        return STATUS_ERROR;
      }
    }
    //
    // Skip over the args
    //
    argc -= 2;
    argv += 2;
  }
  //
  // Check for enough args
  //
  if (argc < 3) {
    Usage ();
    return STATUS_ERROR;
  }

  InImageName = argv[2];

  if (argc == 4) {
    OutImageName = argv[3];
  }
  //
  // Get new image type
  //
  p = argv[1];
  if (*p == '/' || *p == '\\') {
    p += 1;
  }

  if (stricmp (p, "app") == 0 || stricmp (p, "UEFI_APPLICATION") == 0) {
    Type  = EFI_IMAGE_SUBSYSTEM_EFI_APPLICATION;
    Ext   = ".efi";

  } else if (stricmp (p, "bsdrv") == 0 || stricmp (p, "DXE_DRIVER") == 0) {
    Type  = EFI_IMAGE_SUBSYSTEM_EFI_BOOT_SERVICE_DRIVER;
    Ext   = ".efi";

  } else if (stricmp (p, "rtdrv") == 0 || stricmp (p, "DXE_RUNTIME_DRIVER") == 0) {
    Type  = EFI_IMAGE_SUBSYSTEM_EFI_RUNTIME_DRIVER;
    Ext   = ".efi";

  } else if (stricmp (p, "rtdrv") == 0 || stricmp (p, "DXE_SAL_DRIVER") == 0) {
    Type  = EFI_IMAGE_SUBSYSTEM_SAL_RUNTIME_DRIVER;
    Ext   = ".efi";
  } else if (stricmp (p, "SEC") == 0) {
    Type  = EFI_IMAGE_SUBSYSTEM_EFI_BOOT_SERVICE_DRIVER;
    Ext   = ".sec";
  } else if (stricmp (p, "peim") == 0 ||
           stricmp (p, "BASE") == 0 ||
           stricmp (p, "PEI_CORE") == 0 ||
           stricmp (p, "PEIM") == 0 ||
           stricmp (p, "DXE_SMM_DRIVER") == 0 ||
           stricmp (p, "TOOL") == 0 ||
           stricmp (p, "UEFI_APPLICATION") == 0 ||
           stricmp (p, "USER_DEFINED") == 0 ||
           stricmp (p, "UEFI_DRIVER") == 0 ||
           stricmp (p, "DXE_CORE") == 0
          ) {
    Type  = EFI_IMAGE_SUBSYSTEM_EFI_BOOT_SERVICE_DRIVER;
    Ext   = ".pei";
  } else {
  	printf ("%s", p);
    Usage ();
    return STATUS_ERROR;
  }
  //
  // open source file
  //
  fpIn = fopen (InImageName, "rb");
  if (!fpIn) {
    Error (NULL, 0, 0, InImageName, "failed to open input file for reading");
    return STATUS_ERROR;
  }

  FReadFile (fpIn, (VOID **)&FileBuffer, &FileLength);

#ifdef HAVE_ELF
  if (IsElfHeader(FileBuffer)) {
    ConvertElf(&FileBuffer, &FileLength);
  }
#endif
  //
  // Read the dos & pe hdrs of the image
  //
  DosHdr = (EFI_IMAGE_DOS_HEADER *)FileBuffer;
  if (DosHdr->e_magic != EFI_IMAGE_DOS_SIGNATURE) {
    Error (NULL, 0, 0, InImageName, "DOS header signature not found in source image");
    fclose (fpIn);
    return STATUS_ERROR;
  }

  PeHdr = (EFI_IMAGE_NT_HEADERS *)(FileBuffer + DosHdr->e_lfanew);
  if (PeHdr->Signature != EFI_IMAGE_NT_SIGNATURE) {
    Error (NULL, 0, 0, InImageName, "PE header signature not found in source image");
    fclose (fpIn);
    return STATUS_ERROR;
  }

  //
  // open output file
  //
  strcpy (outname, InImageName);
  pe = NULL;
  for (p = outname; *p; p++) {
    if (*p == '.') {
      pe = p;
    }
  }

  if (!pe) {
    pe = p;
  }

  strcpy (pe, Ext);

  if (!OutImageName) {
    OutImageName = outname;
  }

  fpOut = fopen (OutImageName, "w+b");
  if (!fpOut) {
    Error (NULL, 0, 0, OutImageName, "could not open output file for writing");
    fclose (fpIn);
    return STATUS_ERROR;
  }

  //
  // Zero all unused fields of the DOS header
  //
  memcpy (&BackupDosHdr, DosHdr, sizeof (EFI_IMAGE_DOS_HEADER));
  memset (DosHdr, 0, sizeof (EFI_IMAGE_DOS_HEADER));
  DosHdr->e_magic  = BackupDosHdr.e_magic;
  DosHdr->e_lfanew = BackupDosHdr.e_lfanew;

  for (Index = sizeof (EFI_IMAGE_DOS_HEADER); Index < (ULONG) DosHdr->e_lfanew; Index++) {
    FileBuffer[Index] = DosHdr->e_cp;
  }

  //
  // Patch the PE header
  //
  PeHdr->OptionalHeader.Subsystem = (USHORT) Type;
  if (TimeStampPresent) {
    PeHdr->FileHeader.TimeDateStamp = (UINT32) TimeStamp;
  }

  if (PeHdr->OptionalHeader.Magic == EFI_IMAGE_NT_OPTIONAL_HDR32_MAGIC) {
    Optional32 = (EFI_IMAGE_OPTIONAL_HEADER32 *)&PeHdr->OptionalHeader;
    Optional32->MajorLinkerVersion          = 0;
    Optional32->MinorLinkerVersion          = 0;
    Optional32->MajorOperatingSystemVersion = 0;
    Optional32->MinorOperatingSystemVersion = 0;
    Optional32->MajorImageVersion           = 0;
    Optional32->MinorImageVersion           = 0;
    Optional32->MajorSubsystemVersion       = 0;
    Optional32->MinorSubsystemVersion       = 0;
    Optional32->Win32VersionValue           = 0;
    Optional32->CheckSum                    = 0;
    Optional32->SizeOfStackReserve = 0;
    Optional32->SizeOfStackCommit  = 0;
    Optional32->SizeOfHeapReserve  = 0;
    Optional32->SizeOfHeapCommit   = 0;

    //
    // Strip zero padding at the end of the .reloc section 
    //
    if (Optional32->NumberOfRvaAndSizes >= 6) {
      if (Optional32->DataDirectory[5].Size != 0) {
        SectionHeader = (EFI_IMAGE_SECTION_HEADER *)(FileBuffer + DosHdr->e_lfanew + sizeof(UINT32) + sizeof (EFI_IMAGE_FILE_HEADER) + PeHdr->FileHeader.SizeOfOptionalHeader);
        for (Index = 0; Index < PeHdr->FileHeader.NumberOfSections; Index++, SectionHeader++) {
          //
          // Look for the Section Header that starts as the same virtual address as the Base Relocation Data Directory
          //
          if (SectionHeader->VirtualAddress == Optional32->DataDirectory[5].VirtualAddress) {
            SectionHeader->Misc.VirtualSize = Optional32->DataDirectory[5].Size;
            AllignedRelocSize = (Optional32->DataDirectory[5].Size + Optional32->FileAlignment - 1) & (~(Optional32->FileAlignment - 1));
            //
            // Check to see if there is zero padding at the end of the base relocations
            //
            if (AllignedRelocSize < SectionHeader->SizeOfRawData) {
              //
              // Check to see if the base relocations are at the end of the file
              //
              if (SectionHeader->PointerToRawData + SectionHeader->SizeOfRawData == Optional32->SizeOfImage) {
                //
                // All the required conditions are met to strip the zero padding of the end of the base relocations section
                //
                Optional32->SizeOfImage -= (SectionHeader->SizeOfRawData - AllignedRelocSize);
                Optional32->SizeOfInitializedData -= (SectionHeader->SizeOfRawData - AllignedRelocSize);
                SectionHeader->SizeOfRawData = AllignedRelocSize;
                FileLength = Optional32->SizeOfImage;
              }
            }
          }
        }
      }
    }
  } 
  if (PeHdr->OptionalHeader.Magic == EFI_IMAGE_NT_OPTIONAL_HDR64_MAGIC) {
    Optional64 = (EFI_IMAGE_OPTIONAL_HEADER64 *)&PeHdr->OptionalHeader;
    Optional64->MajorLinkerVersion          = 0;
    Optional64->MinorLinkerVersion          = 0;
    Optional64->MajorOperatingSystemVersion = 0;
    Optional64->MinorOperatingSystemVersion = 0;
    Optional64->MajorImageVersion           = 0;
    Optional64->MinorImageVersion           = 0;
    Optional64->MajorSubsystemVersion       = 0;
    Optional64->MinorSubsystemVersion       = 0;
    Optional64->Win32VersionValue           = 0;
    Optional64->CheckSum                    = 0;
    Optional64->SizeOfStackReserve = 0;
    Optional64->SizeOfStackCommit  = 0;
    Optional64->SizeOfHeapReserve  = 0;
    Optional64->SizeOfHeapCommit   = 0;

    //
    // Zero the .pdata section if the machine type is X64 and the Debug Directory is empty
    //
    if (PeHdr->FileHeader.Machine == 0x8664) { // X64
      if (Optional64->NumberOfRvaAndSizes >= 4) {
        if (Optional64->NumberOfRvaAndSizes < 7 || (Optional64->NumberOfRvaAndSizes >= 7 && Optional64->DataDirectory[6].Size == 0)) {
          SectionHeader = (EFI_IMAGE_SECTION_HEADER *)(FileBuffer + DosHdr->e_lfanew + sizeof(UINT32) + sizeof (EFI_IMAGE_FILE_HEADER) + PeHdr->FileHeader.SizeOfOptionalHeader);
          for (Index = 0; Index < PeHdr->FileHeader.NumberOfSections; Index++, SectionHeader++) {
            if (SectionHeader->VirtualAddress == Optional64->DataDirectory[3].VirtualAddress) {
              RuntimeFunction = (RUNTIME_FUNCTION *)(FileBuffer + SectionHeader->PointerToRawData);
              for (Index1 = 0; Index1 < Optional64->DataDirectory[3].Size / sizeof (RUNTIME_FUNCTION); Index1++, RuntimeFunction++) {
                SectionHeader = (EFI_IMAGE_SECTION_HEADER *)(FileBuffer + DosHdr->e_lfanew + sizeof(UINT32) + sizeof (EFI_IMAGE_FILE_HEADER) + PeHdr->FileHeader.SizeOfOptionalHeader);
                for (Index2 = 0; Index2 < PeHdr->FileHeader.NumberOfSections; Index2++, SectionHeader++) {
                  if (RuntimeFunction->UnwindInfoAddress > SectionHeader->VirtualAddress && RuntimeFunction->UnwindInfoAddress < (SectionHeader->VirtualAddress + SectionHeader->SizeOfRawData)) {
                    UnwindInfo = (UNWIND_INFO *)(FileBuffer + SectionHeader->PointerToRawData + (RuntimeFunction->UnwindInfoAddress - SectionHeader->VirtualAddress));
                    if (UnwindInfo->Version == 1) {
                      memset (UnwindInfo + 1, 0, UnwindInfo->CountOfUnwindCodes * sizeof (UINT16));
                      memset (UnwindInfo, 0, sizeof (UNWIND_INFO));
                    }
                  }
                }
                memset (RuntimeFunction, 0, sizeof (RUNTIME_FUNCTION));
              }

              break;
            }
          }
          Optional64->DataDirectory[3].Size = 0;
          Optional64->DataDirectory[3].VirtualAddress = 0;
        }
      }
    }

    //
    // Strip zero padding at the end of the .reloc section 
    //
    if (Optional64->NumberOfRvaAndSizes >= 6) {
      if (Optional64->DataDirectory[5].Size != 0) {
        SectionHeader = (EFI_IMAGE_SECTION_HEADER *)(FileBuffer + DosHdr->e_lfanew + sizeof(UINT32) + sizeof (EFI_IMAGE_FILE_HEADER) + PeHdr->FileHeader.SizeOfOptionalHeader);
        for (Index = 0; Index < PeHdr->FileHeader.NumberOfSections; Index++, SectionHeader++) {
          //
          // Look for the Section Header that starts as the same virtual address as the Base Relocation Data Directory
          //
          if (SectionHeader->VirtualAddress == Optional64->DataDirectory[5].VirtualAddress) {
            SectionHeader->Misc.VirtualSize = Optional64->DataDirectory[5].Size;
            AllignedRelocSize = (Optional64->DataDirectory[5].Size + Optional64->FileAlignment - 1) & (~(Optional64->FileAlignment - 1));
            //
            // Check to see if there is zero padding at the end of the base relocations
            //
            if (AllignedRelocSize < SectionHeader->SizeOfRawData) {
              //
              // Check to see if the base relocations are at the end of the file
              //
              if (SectionHeader->PointerToRawData + SectionHeader->SizeOfRawData == Optional64->SizeOfImage) {
                //
                // All the required conditions are met to strip the zero padding of the end of the base relocations section
                //
                Optional64->SizeOfImage -= (SectionHeader->SizeOfRawData - AllignedRelocSize);
                Optional64->SizeOfInitializedData -= (SectionHeader->SizeOfRawData - AllignedRelocSize);
                SectionHeader->SizeOfRawData = AllignedRelocSize;
                FileLength = Optional64->SizeOfImage;
              }
            }
          }
        }
      }
    }
  }

  FWriteFile (fpOut, FileBuffer, FileLength);

  //
  // Done
  //
  fclose (fpIn);
  fclose (fpOut);
  //
  // printf ("Created %s\n", OutImageName);
  //
  return STATUS_SUCCESS;
}
