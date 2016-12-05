/** @file

Copyright (c) 2007, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.


**/

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

/**

  Find symbol by address.

  @param  Address         - Symbol address
  @param  Type            - Search type
  @param  RetObject       - Symbol object
  @param  RetEntry        - Symbol entry

  @return Nearest symbol address

**/
UINTN
EbdFindSymbolAddress (
  IN UINTN                       Address,
  IN EDB_MATCH_SYMBOL_TYPE       Type,
  OUT EFI_DEBUGGER_SYMBOL_OBJECT **Object,
  OUT EFI_DEBUGGER_SYMBOL_ENTRY  **Entry
  );

/**

  Load symbol file by name.

  @param  DebuggerPrivate - EBC Debugger private data structure
  @param  FileName        - Symbol file name
  @param  BufferSize      - Symbol file buffer size
  @param  Buffer          - Symbol file buffer

  @retval EFI_SUCCESS - load symbol successfully

**/
EFI_STATUS
EdbLoadSymbol (
  IN EFI_DEBUGGER_PRIVATE_DATA   *DebuggerPrivate,
  IN CHAR16                      *FileName,
  IN UINTN                       BufferSize,
  IN VOID                        *Buffer
  );

/**

  Unload symbol file by name.

  @param  DebuggerPrivate - EBC Debugger private data structure
  @param  FileName        - Symbol file name

  @retval EFI_SUCCESS - unload symbol successfully

**/
EFI_STATUS
EdbUnloadSymbol (
  IN EFI_DEBUGGER_PRIVATE_DATA   *DebuggerPrivate,
  IN CHAR16                      *FileName
  );

/**

  Patch symbol RVA.

  @param  DebuggerPrivate - EBC Debugger private data structure
  @param  FileName        - Symbol file name
  @param  SearchType      - Search type for Object

  @retval EFI_SUCCESS   - Patch symbol RVA successfully
  @retval EFI_NOT_FOUND - Symbol RVA base not found

**/
EFI_STATUS
EdbPatchSymbolRVA (
  IN EFI_DEBUGGER_PRIVATE_DATA     *DebuggerPrivate,
  IN CHAR16                        *FileName,
  IN EDB_EBC_IMAGE_RVA_SEARCH_TYPE SearchType
  );

/**

  Load code.

  @param  DebuggerPrivate - EBC Debugger private data structure
  @param  MapFileName     - Symbol file name
  @param  FileName        - Code file name
  @param  BufferSize      - Code file buffer size
  @param  Buffer          - Code file buffer

  @retval EFI_SUCCESS - Code loaded successfully

**/
EFI_STATUS
EdbLoadCode (
  IN EFI_DEBUGGER_PRIVATE_DATA   *DebuggerPrivate,
  IN CHAR16                      *MapFileName,
  IN CHAR16                      *FileName,
  IN UINTN                       BufferSize,
  IN VOID                        *Buffer
  );

/**

  Unload code.

  @param  DebuggerPrivate - EBC Debugger private data structure
  @param  MapFileName     - Symbol file name
  @param  FileName        - Code file name
  @param  Buffer          - Code file buffer

  @retval EFI_SUCCESS - Code unloaded successfully

**/
EFI_STATUS
EdbUnloadCode (
  IN EFI_DEBUGGER_PRIVATE_DATA   *DebuggerPrivate,
  IN CHAR16                      *MapFileName,
  IN CHAR16                      *FileName,
  OUT VOID                       **Buffer
  );

/**

  Add code buffer.

  @param  DebuggerPrivate - EBC Debugger private data structure
  @param  MapFileName     - Symbol file name
  @param  CodeFileName    - Code file name
  @param  SourceBufferSize- Code buffer size
  @param  SourceBuffer    - Code buffer

  @retval EFI_SUCCESS - CodeBuffer added successfully

**/
EFI_STATUS
EdbAddCodeBuffer (
  IN     EFI_DEBUGGER_PRIVATE_DATA *DebuggerPrivate,
  IN     CHAR16                    *MapFileName,
  IN     CHAR16                    *CodeFileName,
  IN     UINTN                     SourceBufferSize,
  IN     VOID                      *SourceBuffer
  );

/**

  Delete code buffer.

  @param  DebuggerPrivate - EBC Debugger private data structure
  @param  MapFileName     - Symbol file name
  @param  CodeFileName    - Code file name
  @param  SourceBuffer    - Code buffer

  @retval EFI_SUCCESS - CodeBuffer deleted successfully

**/
EFI_STATUS
EdbDeleteCodeBuffer (
  IN     EFI_DEBUGGER_PRIVATE_DATA *DebuggerPrivate,
  IN     CHAR16                    *MapFileName,
  IN     CHAR16                    *CodeFileName,
  IN     VOID                      *SourceBuffer
  );

/**

  Find the symbol string according to address.

  @param  Address         - Symbol address

  @return Symbol string

**/
CHAR8 *
FindSymbolStr (
  IN UINTN Address
  );

/**

  Print source.

  @param  Address         - Instruction address
  @param  IsPrint         - Whether need to print

  @retval 1 - find the source
  @retval 0 - not find the source

**/
UINTN
EdbPrintSource (
  IN UINTN     Address,
  IN BOOLEAN   IsPrint
  );

/**

  Convert a symbol to an address.

  @param  Symbol          - Symbol name
  @param  Address         - Symbol address

  @retval EFI_SUCCESS    - symbol found and address returned.
  @retval EFI_NOT_FOUND  - symbol not found
  @retval EFI_NO_MAPPING - duplicated symbol not found

**/
EFI_STATUS
Symboltoi (
  IN CHAR16   *Symbol,
  OUT UINTN   *Address
  );

#endif
