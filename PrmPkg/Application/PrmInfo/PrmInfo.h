/** @file
  Prints information about the PRM configuration loaded by the system firmware.

  Copyright (C) Microsoft Corporation. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef PRM_INFO_H_
#define PRM_INFO_H_

#include <Base.h>
#include <Prm.h>
#include <PrmDataBuffer.h>
#include <Uefi.h>

#define  APPLICATION_NAME  L"PrmInfo"

#define PRM_HANDLER_CONTEXT_LIST_ENTRY_SIGNATURE  SIGNATURE_32('P','R','H','E')

#pragma pack(push, 1)

typedef struct {
  CHAR8              *Name;
  EFI_GUID           *Guid;
  PRM_DATA_BUFFER    *StaticDataBuffer;
  CHAR8              *ModuleName;
  PRM_HANDLER        *Handler;
} PRM_HANDLER_CONTEXT;

typedef struct {
  UINTN                  Signature;
  LIST_ENTRY             Link;
  PRM_HANDLER_CONTEXT    Context;
} PRM_HANDLER_CONTEXT_LIST_ENTRY;

#pragma pack(pop)

//
// Iterate through the double linked list. NOT delete safe.
//
#define EFI_LIST_FOR_EACH(Entry, ListHead)    \
  for(Entry = (ListHead)->ForwardLink; Entry != (ListHead); Entry = Entry->ForwardLink)

#define ONE_MICROSECOND  (1000)
#define ONE_MILLISECOND  (1000 * ONE_MICROSECOND)
#define ONE_SECOND       (1000 * ONE_MILLISECOND)

#endif
