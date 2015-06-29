/** @file
Elf convert solution

Copyright (c) 2010 - 2014, Intel Corporation. All rights reserved.<BR>

This program and the accompanying materials are licensed and made available 
under the terms and conditions of the BSD License which accompanies this 
distribution.  The full text of the license may be found at 
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "WinNtInclude.h"

#ifndef __GNUC__
#include <windows.h>
#include <io.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>

#include <Common/UefiBaseTypes.h>
#include <IndustryStandard/PeImage.h>

#include "EfiUtilityMsgs.h"

#include "GenFw.h"
#include "ElfConvert.h"
#include "Elf32Convert.h"
#include "Elf64Convert.h"

//
// Result Coff file in memory.
//
UINT8 *mCoffFile = NULL;

//
// COFF relocation data
//
EFI_IMAGE_BASE_RELOCATION *mCoffBaseRel;
UINT16                    *mCoffEntryRel;

//
// Current offset in coff file.
//
UINT32 mCoffOffset;

//
// Offset in Coff file of headers and sections.
//
UINT32 mTableOffset;

//
//*****************************************************************************
// Common ELF Functions
//*****************************************************************************
//

VOID
CoffAddFixupEntry(
  UINT16 Val
  )
{
  *mCoffEntryRel = Val;
  mCoffEntryRel++;
  mCoffBaseRel->SizeOfBlock += 2;
  mCoffOffset += 2;
}

VOID
CoffAddFixup(
  UINT32 Offset,
  UINT8  Type
  )
{
  if (mCoffBaseRel == NULL
      || mCoffBaseRel->VirtualAddress != (Offset & ~0xfff)) {
    if (mCoffBaseRel != NULL) {
      //
      // Add a null entry (is it required ?)
      //
      CoffAddFixupEntry (0);
      
      //
      // Pad for alignment.
      //
      if (mCoffOffset % 4 != 0)
        CoffAddFixupEntry (0);
    }

    mCoffFile = realloc (
      mCoffFile,
      mCoffOffset + sizeof(EFI_IMAGE_BASE_RELOCATION) + 2 * MAX_COFF_ALIGNMENT
      );
    memset (
      mCoffFile + mCoffOffset, 0,
      sizeof(EFI_IMAGE_BASE_RELOCATION) + 2 * MAX_COFF_ALIGNMENT
      );

    mCoffBaseRel = (EFI_IMAGE_BASE_RELOCATION*)(mCoffFile + mCoffOffset);
    mCoffBaseRel->VirtualAddress = Offset & ~0xfff;
    mCoffBaseRel->SizeOfBlock = sizeof(EFI_IMAGE_BASE_RELOCATION);

    mCoffEntryRel = (UINT16 *)(mCoffBaseRel + 1);
    mCoffOffset += sizeof(EFI_IMAGE_BASE_RELOCATION);
  }

  //
  // Fill the entry.
  //
  CoffAddFixupEntry((UINT16) ((Type << 12) | (Offset & 0xfff)));
}

VOID
CreateSectionHeader (
  const CHAR8 *Name,
  UINT32      Offset,
  UINT32      Size,
  UINT32      Flags
  )
{
  EFI_IMAGE_SECTION_HEADER *Hdr;
  Hdr = (EFI_IMAGE_SECTION_HEADER*)(mCoffFile + mTableOffset);

  strcpy((char *)Hdr->Name, Name);
  Hdr->Misc.VirtualSize = Size;
  Hdr->VirtualAddress = Offset;
  Hdr->SizeOfRawData = Size;
  Hdr->PointerToRawData = Offset;
  Hdr->PointerToRelocations = 0;
  Hdr->PointerToLinenumbers = 0;
  Hdr->NumberOfRelocations = 0;
  Hdr->NumberOfLinenumbers = 0;
  Hdr->Characteristics = Flags;

  mTableOffset += sizeof (EFI_IMAGE_SECTION_HEADER);
}

//
//*****************************************************************************
// Functions called from GenFw main code.
//*****************************************************************************
//

INTN
IsElfHeader (
  UINT8  *FileBuffer
)
{
  return (FileBuffer[EI_MAG0] == ELFMAG0 && 
          FileBuffer[EI_MAG1] == ELFMAG1 &&
          FileBuffer[EI_MAG2] == ELFMAG2 &&
          FileBuffer[EI_MAG3] == ELFMAG3);
}

BOOLEAN
ConvertElf (
  UINT8  **FileBuffer,
  UINT32 *FileLength
  )
{
  ELF_FUNCTION_TABLE              ElfFunctions;
  UINT8                           EiClass;

  //
  // Determine ELF type and set function table pointer correctly.
  //
  VerboseMsg ("Check Elf Image Header");
  EiClass = (*FileBuffer)[EI_CLASS];
  if (EiClass == ELFCLASS32) {
    if (!InitializeElf32 (*FileBuffer, &ElfFunctions)) {
      return FALSE;
    }
  } else if (EiClass == ELFCLASS64) {
    if (!InitializeElf64 (*FileBuffer, &ElfFunctions)) {
      return FALSE;
    }
  } else {
    Error (NULL, 0, 3000, "Unsupported", "ELF EI_CLASS not supported.");
    return FALSE;
  }

  //
  // Compute sections new address.
  //  
  VerboseMsg ("Compute sections new address.");
  ElfFunctions.ScanSections ();

  //
  // Write and relocate sections.
  //
  VerboseMsg ("Write and relocate sections.");
  ElfFunctions.WriteSections (SECTION_TEXT);
  ElfFunctions.WriteSections (SECTION_DATA);
  ElfFunctions.WriteSections (SECTION_HII);

  //
  // Translate and write relocations.
  //
  VerboseMsg ("Translate and write relocations.");
  ElfFunctions.WriteRelocations ();

  //
  // Write debug info.
  //
  VerboseMsg ("Write debug info.");
  ElfFunctions.WriteDebug ();

  //
  // Make sure image size is correct before returning the new image.
  //
  VerboseMsg ("Set image size.");
  ElfFunctions.SetImageSize ();

  //
  // Replace.
  //
  free (*FileBuffer);
  *FileBuffer = mCoffFile;
  *FileLength = mCoffOffset;

  //
  // Free resources used by ELF functions.
  //
  ElfFunctions.CleanUp ();
  
  return TRUE;
}
