/** @file
Header file for Elf convert solution

Copyright (c) 2010 - 2018, Intel Corporation. All rights reserved.<BR>

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _ELF_CONVERT_H_
#define _ELF_CONVERT_H_

#include "elf_common.h"
#include "elf32.h"
#include "elf64.h"

//
// Externally defined variables
//
extern UINT32 mCoffOffset;
extern CHAR8  *mInImageName;
extern UINT32 mImageTimeStamp;
extern UINT8  *mCoffFile;
extern UINT32 mTableOffset;
extern UINT32 mOutImageType;
extern UINT32 mFileBufferSize;

//
// Common EFI specific data.
//
#define ELF_HII_SECTION_NAME ".hii"
#define ELF_STRTAB_SECTION_NAME ".strtab"
#define MAX_COFF_ALIGNMENT 0x10000

//
// Filter Types
//
typedef enum {
  SECTION_TEXT,
  SECTION_HII,
  SECTION_DATA

} SECTION_FILTER_TYPES;

//
// FunctionTable
//
typedef struct {
  VOID    (*ScanSections) ();
  BOOLEAN (*WriteSections) (SECTION_FILTER_TYPES  FilterType);
  VOID    (*WriteRelocations) ();
  VOID    (*WriteDebug) ();
  VOID    (*SetImageSize) ();
  VOID    (*CleanUp) ();

} ELF_FUNCTION_TABLE;

//
// Common functions
//
VOID
CoffAddFixup (
  UINT32 Offset,
  UINT8  Type
  );

VOID
CoffAddFixupEntry (
  UINT16 Val
  );


VOID
CreateSectionHeader (
  const CHAR8 *Name,
  UINT32      Offset,
  UINT32      Size,
  UINT32      Flags
  );

#endif
