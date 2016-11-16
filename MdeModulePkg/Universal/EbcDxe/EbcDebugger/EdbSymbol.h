/*++

Copyright (c) 2007, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

  EdbSymbol.h

Abstract:


--*/

#ifndef _EFI_EDB_SYMBOL_H_
#define _EFI_EDB_SYMBOL_H_

#include <Uefi.h>

//
// The default base address is 0x10000000
//
#define EFI_DEBUGGER_DEFAULT_LINK_IMAGEBASE  0x10000000

#define EFI_DEBUGGER_MAX_SYMBOL_ADDRESS_DELTA_VALUE  0x100000 // 1 M delta

typedef enum {
  EdbMatchSymbolTypeSameAdderss,
  EdbMatchSymbolTypeNearestAddress,
  EdbMatchSymbolTypeLowerAddress,
  EdbMatchSymbolTypeUpperAddress,
  EdbMatchSymbolTypeMax,
} EDB_MATCH_SYMBOL_TYPE;

typedef enum {
  EdbEbcImageRvaSearchTypeAny,
  EdbEbcImageRvaSearchTypeFirst,
  EdbEbcImageRvaSearchTypeLast,
  EdbEbcImageRvaSearchTypeMax,
} EDB_EBC_IMAGE_RVA_SEARCH_TYPE;

UINTN
EbdFindSymbolAddress (
  IN UINTN                       Address,
  IN EDB_MATCH_SYMBOL_TYPE       Type,
  OUT EFI_DEBUGGER_SYMBOL_OBJECT **Object,
  OUT EFI_DEBUGGER_SYMBOL_ENTRY  **Entry
  );

EFI_STATUS
EdbLoadSymbol (
  IN EFI_DEBUGGER_PRIVATE_DATA   *DebuggerPrivate,
  IN CHAR16                      *FileName,
  IN UINTN                       BufferSize,
  IN VOID                        *Buffer
  );

EFI_STATUS
EdbUnloadSymbol (
  IN EFI_DEBUGGER_PRIVATE_DATA   *DebuggerPrivate,
  IN CHAR16                      *FileName
  );

EFI_STATUS
EdbPatchSymbolRVA (
  IN EFI_DEBUGGER_PRIVATE_DATA     *DebuggerPrivate,
  IN CHAR16                        *FileName,
  IN EDB_EBC_IMAGE_RVA_SEARCH_TYPE SearchType
  );

EFI_STATUS
EdbLoadCode (
  IN EFI_DEBUGGER_PRIVATE_DATA   *DebuggerPrivate,
  IN CHAR16                      *MapFileName,
  IN CHAR16                      *FileName,
  IN UINTN                       BufferSize,
  IN VOID                        *Buffer
  );

EFI_STATUS
EdbUnloadCode (
  IN EFI_DEBUGGER_PRIVATE_DATA   *DebuggerPrivate,
  IN CHAR16                      *MapFileName,
  IN CHAR16                      *FileName,
  OUT VOID                       **Buffer
  );

EFI_STATUS
EdbAddCodeBuffer (
  IN     EFI_DEBUGGER_PRIVATE_DATA *DebuggerPrivate,
  IN     CHAR16                    *MapFileName,
  IN     CHAR16                    *CodeFileName,
  IN     UINTN                     SourceBufferSize,
  IN     VOID                      *SourceBuffer
  );

EFI_STATUS
EdbDeleteCodeBuffer (
  IN     EFI_DEBUGGER_PRIVATE_DATA *DebuggerPrivate,
  IN     CHAR16                    *MapFileName,
  IN     CHAR16                    *CodeFileName,
  IN     VOID                      *SourceBuffer
  );

CHAR8 *
FindSymbolStr (
  IN UINTN Address
  );

UINTN
EdbPrintSource (
  IN UINTN     Address,
  IN BOOLEAN   IsPrint
  );

EFI_STATUS
Symboltoi (
  IN CHAR16   *Symbol,
  OUT UINTN   *Address
  );

#endif
