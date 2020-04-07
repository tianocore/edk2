/** @file

  Definitions specific to the Platform Runtime Mechanism (PRM) loader.x

  Copyright (c) Microsoft Corporation
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef PRM_LOADER_H_
#define PRM_LOADER_H_

#include <IndustryStandard/PeImage.h>
#include <Library/PeCoffLib.h>

#include <PrmExportDescriptor.h>

#define _DBGMSGID_                                    "[PRMLOADER]"
#define PRM_MODULE_IMAGE_CONTEXT_LIST_ENTRY_SIGNATURE SIGNATURE_32('P','R','M','E')

#pragma pack(push, 1)

typedef struct {
  PE_COFF_LOADER_IMAGE_CONTEXT          PeCoffImageContext;
  EFI_IMAGE_EXPORT_DIRECTORY            *ExportDirectory;
  PRM_MODULE_EXPORT_DESCRIPTOR_STRUCT   *ExportDescriptor;
} PRM_MODULE_IMAGE_CONTEXT;

typedef struct {
  UINTN                                 Signature;
  LIST_ENTRY                            Link;
  PRM_MODULE_IMAGE_CONTEXT              *Context;
} PRM_MODULE_IMAGE_CONTEXT_LIST_ENTRY;

#pragma pack(pop)

//
// Iterate through the double linked list. NOT delete safe.
//
#define EFI_LIST_FOR_EACH(Entry, ListHead)    \
  for(Entry = (ListHead)->ForwardLink; Entry != (ListHead); Entry = Entry->ForwardLink)

//
// Iterate through the double linked list. This is delete-safe.
// Don't touch NextEntry.
//
#define EFI_LIST_FOR_EACH_SAFE(Entry, NextEntry, ListHead)            \
  for(Entry = (ListHead)->ForwardLink, NextEntry = Entry->ForwardLink;\
      Entry != (ListHead); Entry = NextEntry, NextEntry = Entry->ForwardLin

#endif
