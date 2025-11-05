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
extern BOOLEAN mExportFlag;

//
// Common EFI specific data.
//
#define ELF_HII_SECTION_NAME ".hii"
#define ELF_STRTAB_SECTION_NAME ".strtab"
#define MAX_COFF_ALIGNMENT 0x10000
#define ELF_SYMBOL_SECTION_NAME ".symtab"

//
// Platform Runtime Mechanism (PRM) specific data.
//
#define PRM_MODULE_EXPORT_SYMBOL_NUM 256

// <to-do> to include PRM header directly once PrmPkg is in main repo
#define PRM_HANDLER_NAME_MAXIMUM_LENGTH 128

#define PRM_MODULE_EXPORT_DESCRIPTOR_NAME         "PrmModuleExportDescriptor"
#define PRM_MODULE_EXPORT_DESCRIPTOR_SIGNATURE    SIGNATURE_64 ('P', 'R', 'M', '_', 'M', 'E', 'D', 'T')
#define PRM_MODULE_EXPORT_REVISION                0x0

//
// Platform Runtime Mechanism (PRM) Export Descriptor Structures
//
#pragma pack(push, 1)

typedef struct {
  EFI_GUID                              PrmHandlerGuid;
  CHAR8                                 PrmHandlerName[PRM_HANDLER_NAME_MAXIMUM_LENGTH];
} PRM_HANDLER_EXPORT_DESCRIPTOR_STRUCT;

typedef struct {
  UINT64                                Signature;
  UINT16                                Revision;
  UINT16                                NumberPrmHandlers;
  EFI_GUID                              PlatformGuid;
  EFI_GUID                              ModuleGuid;
} PRM_MODULE_EXPORT_DESCRIPTOR_STRUCT_HEADER;

typedef struct {
  PRM_MODULE_EXPORT_DESCRIPTOR_STRUCT_HEADER  Header;
  PRM_HANDLER_EXPORT_DESCRIPTOR_STRUCT        PrmHandlerExportDescriptors[1];
} PRM_MODULE_EXPORT_DESCRIPTOR_STRUCT;

#pragma pack(pop)

//
// Filter Types
//
typedef enum {
  SECTION_TEXT,
  SECTION_HII,
  SECTION_DATA,
  SECTION_SYMBOL

} SECTION_FILTER_TYPES;

//
// FunctionTable
//
typedef struct {
  VOID    (*ScanSections) ();
  BOOLEAN (*WriteSections) (SECTION_FILTER_TYPES  FilterType);
  VOID    (*WriteRelocations) ();
  VOID    (*WriteDebug) ();
  VOID    (*WriteExport) ();
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
