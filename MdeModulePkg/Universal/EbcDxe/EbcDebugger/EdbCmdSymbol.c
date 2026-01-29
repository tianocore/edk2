/** @file

Copyright (c) 2007 - 2016, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent


**/

#include "Edb.h"

/**

  Get file name from full path.

  @param  FullPath        - full file path

  @return  file name

**/
CHAR16 *
GetFileNameFromFullPath (
  IN CHAR16  *FullPath
  )
{
  CHAR16  *FileName;
  CHAR16  *TempFileName;

  FileName     = FullPath;
  TempFileName = StrGetNewTokenLine (FullPath, L"\\");

  while (TempFileName != NULL) {
    FileName     = TempFileName;
    TempFileName = StrGetNextTokenLine (L"\\");
    PatchForStrTokenBefore (TempFileName, L'\\');
  }

  return FileName;
}

/**

  Get dir name from full path.

  @param  FullPath        - full file path

  @return dir name

**/
CHAR16 *
GetDirNameFromFullPath (
  IN CHAR16  *FullPath
  )
{
  CHAR16  *FileName;

  FileName = GetFileNameFromFullPath (FullPath);
  if (FileName != FullPath) {
    *(FileName - 1) = 0;
    return FullPath;
  }

  return L"";
}

/**

  Construct full path according to dir and file path.

  @param  DirPath         - dir path
  @param  FilePath        - file path
  @param  Size            - dir max size

  @return Full file name

**/
CHAR16 *
ConstructFullPath (
  IN CHAR16  *DirPath,
  IN CHAR16  *FilePath,
  IN UINTN   Size
  )
{
  UINTN  DirPathSize;

  DirPathSize              = StrLen (DirPath);
  *(DirPath + DirPathSize) = L'\\';
  StrnCatS (DirPath, DirPathSize + Size + 1, FilePath, Size);

  *(DirPath + DirPathSize + Size + 1) = 0;

  return DirPath;
}

CHAR16  *mSymbolTypeStr[] = {
  L"( F)",
  L"(SF)",
  L"(GV)",
  L"(SV)",
};

/**

  Comvert Symbol Type to string.

  @param  Type            - Symbol Type

  @return String

**/
CHAR16 *
EdbSymbolTypeToStr (
  IN EFI_DEBUGGER_SYMBOL_TYPE  Type
  )
{
  if ((Type < 0) || (Type >= EfiDebuggerSymbolTypeMax)) {
    return L"(?)";
  }

  return mSymbolTypeStr[Type];
}

/**

  Find the symbol according to address and display symbol.

  @param  Address         - SymbolAddress
  @param  DebuggerPrivate - EBC Debugger private data structure

  @retval EFI_DEBUG_CONTINUE - formal return value

**/
EFI_DEBUG_STATUS
DebuggerDisplaySymbolAccrodingToAddress (
  IN     UINTN                      Address,
  IN     EFI_DEBUGGER_PRIVATE_DATA  *DebuggerPrivate
  )
{
  EFI_DEBUGGER_SYMBOL_OBJECT  *Object;
  EFI_DEBUGGER_SYMBOL_ENTRY   *Entry;
  UINTN                       CandidateAddress;

  //
  // Find the nearest symbol address
  //
  CandidateAddress = EbdFindSymbolAddress (Address, EdbMatchSymbolTypeNearestAddress, &Object, &Entry);
  if ((CandidateAddress == 0) || (CandidateAddress == (UINTN)-1) || (Entry == NULL)) {
    EDBPrint (L"Symbole at Address not found!\n");
    return EFI_DEBUG_CONTINUE;
  } else if (Address != CandidateAddress) {
    EDBPrint (L"Symbole at Address not found, print nearest one!\n");
  }

  //
  // Display symbol
  //
  EDBPrint (L"Symbol File Name: %s\n", Object->Name);
  if (sizeof (UINTN) == sizeof (UINT64)) {
    EDBPrint (L"        Address      Type  Symbol\n");
    EDBPrint (L"  ================== ==== ========\n");
    //  EDBPrint (L"  0xFFFFFFFF00000000 ( F) TestMain\n");
    EDBPrint (
      L"  0x%016lx %s %a\n",
      (UINT64)Entry->Rva + Object->BaseAddress,
      EdbSymbolTypeToStr (Entry->Type),
      Entry->Name
      );
  } else {
    EDBPrint (L"   Address   Type  Symbol\n");
    EDBPrint (L"  ========== ==== ========\n");
    //  EDBPrint (L"  0xFFFF0000 ( F) TestMain\n");
    EDBPrint (
      L"  0x%08x %s %a\n",
      Entry->Rva + Object->BaseAddress,
      EdbSymbolTypeToStr (Entry->Type),
      Entry->Name
      );
  }

  //
  // Done
  //
  return EFI_DEBUG_CONTINUE;
}

/**

  Find the symbol according to name and display symbol.

  @param  SymbolFileName  - The Symbol File Name, NULL means for all
  @param  SymbolName      - The Symbol Name, NULL means for all
  @param  DebuggerPrivate - EBC Debugger private data structure

  @retval EFI_DEBUG_CONTINUE - formal return value

**/
EFI_DEBUG_STATUS
DebuggerDisplaySymbolAccrodingToName (
  IN     CHAR16                     *SymbolFileName,
  IN     CHAR16                     *SymbolName,
  IN     EFI_DEBUGGER_PRIVATE_DATA  *DebuggerPrivate
  )
{
  UINTN                       Index;
  UINTN                       SubIndex;
  EFI_DEBUGGER_SYMBOL_OBJECT  *Object;
  EFI_DEBUGGER_SYMBOL_ENTRY   *Entry;

  if (DebuggerPrivate->DebuggerSymbolContext.ObjectCount == 0) {
    EDBPrint (L"No Symbol File!\n");
    return EFI_DEBUG_CONTINUE;
  }

  //
  // Go throuth each symbol file
  //
  Object = DebuggerPrivate->DebuggerSymbolContext.Object;
  for (Index = 0; Index < DebuggerPrivate->DebuggerSymbolContext.ObjectCount; Index++, Object++) {
    if ((SymbolFileName != NULL) &&
        (StriCmp (SymbolFileName, Object->Name) != 0))
    {
      continue;
    }

    //
    // Break each symbol file
    //
    if (Index != 0) {
      if (SetPageBreak ()) {
        break;
      }
    }

    EDBPrint (L"Symbol File Name: %s\n", Object->Name);
    if (Object->EntryCount == 0) {
      EDBPrint (L"No Symbol!\n");
      continue;
    }

    Entry = Object->Entry;
    if (sizeof (UINTN) == sizeof (UINT64)) {
      EDBPrint (L"        Address      Type  Symbol\n");
      EDBPrint (L"  ================== ==== ========\n");
      //    EDBPrint (L"  0xFFFFFFFF00000000 ( F) TestMain (EbcTest.obj)\n");
    } else {
      EDBPrint (L"   Address   Type  Symbol\n");
      EDBPrint (L"  ========== ==== ========\n");
      //    EDBPrint (L"  0xFFFF0000 ( F) TestMain (EbcTest.obj)\n");
    }

    //
    // Go through each symbol name
    //
    for (SubIndex = 0; SubIndex < Object->EntryCount; SubIndex++, Entry++) {
      if ((SymbolName != NULL) &&
          (StrCmpUnicodeAndAscii (SymbolName, Entry->Name) != 0))
      {
        continue;
      }

      //
      // Break symbol
      //
      if (((SubIndex % EFI_DEBUGGER_LINE_NUMBER_IN_PAGE) == 0) &&
          (SubIndex != 0))
      {
        if (SetPageBreak ()) {
          break;
        }
      }

      if (sizeof (UINTN) == sizeof (UINT64)) {
        EDBPrint (
          L"  0x%016lx %s %a (%a)\n",
          (UINT64)Entry->Rva + Object->BaseAddress,
          EdbSymbolTypeToStr (Entry->Type),
          Entry->Name,
          Entry->ObjName
          );
      } else {
        EDBPrint (
          L"  0x%08x %s %a (%a)\n",
          Entry->Rva + Object->BaseAddress,
          EdbSymbolTypeToStr (Entry->Type),
          Entry->Name,
          Entry->ObjName
          );
      }
    }
  }

  //
  // Done
  //
  return EFI_DEBUG_CONTINUE;
}

/**

  DebuggerCommand - ListSymbol.

  @param  CommandArg      - The argument for this command
  @param  DebuggerPrivate - EBC Debugger private data structure
  @param  ExceptionType   - Exception type.
  @param  SystemContext   - EBC system context.

  @retval EFI_DEBUG_CONTINUE - formal return value

**/
EFI_DEBUG_STATUS
DebuggerListSymbol (
  IN     CHAR16                     *CommandArg,
  IN     EFI_DEBUGGER_PRIVATE_DATA  *DebuggerPrivate,
  IN     EFI_EXCEPTION_TYPE         ExceptionType,
  IN OUT EFI_SYSTEM_CONTEXT         SystemContext
  )
{
  CHAR16  *SymbolFileName;
  CHAR16  *SymbolName;
  CHAR16  *CommandStr;
  UINTN   Address;

  SymbolFileName = NULL;
  SymbolName     = NULL;
  CommandStr     = CommandArg;

  //
  // display symbol according to address
  //
  if (CommandStr != NULL) {
    if ((StriCmp (CommandStr, L"F") != 0) &&
        (StriCmp (CommandStr, L"S") != 0))
    {
      Address = Xtoi (CommandStr);
      return DebuggerDisplaySymbolAccrodingToAddress (Address, DebuggerPrivate);
    }
  }

  //
  // Get SymbolFileName
  //
  if (CommandStr != NULL) {
    if (StriCmp (CommandStr, L"F") == 0) {
      CommandStr = StrGetNextTokenLine (L" ");
      if (CommandStr == NULL) {
        EDBPrint (L"Symbol File Name missing!\n");
        return EFI_DEBUG_CONTINUE;
      } else {
        SymbolFileName = CommandStr;
        CommandStr     = StrGetNextTokenLine (L" ");
      }
    }
  }

  //
  // Get SymbolName
  //
  if (CommandStr != NULL) {
    if (StriCmp (CommandStr, L"S") == 0) {
      CommandStr = StrGetNextTokenLine (L" ");
      if (CommandStr == NULL) {
        EDBPrint (L"Symbol Name missing!\n");
        return EFI_DEBUG_CONTINUE;
      } else {
        SymbolName = CommandStr;
        CommandStr = StrGetNextTokenLine (L" ");
      }
    }
  }

  if (CommandStr != NULL) {
    EDBPrint (L"Argument error!\n");
    return EFI_DEBUG_CONTINUE;
  }

  //
  // display symbol according to name
  //
  return DebuggerDisplaySymbolAccrodingToName (SymbolFileName, SymbolName, DebuggerPrivate);
}

/**

  DebuggerCommand - LoadSymbol.

  @param  CommandArg      - The argument for this command
  @param  DebuggerPrivate - EBC Debugger private data structure
  @param  ExceptionType   - Exception type.
  @param  SystemContext   - EBC system context.

  @retval EFI_DEBUG_CONTINUE - formal return value

**/
EFI_DEBUG_STATUS
DebuggerLoadSymbol (
  IN     CHAR16                     *CommandArg,
  IN     EFI_DEBUGGER_PRIVATE_DATA  *DebuggerPrivate,
  IN     EFI_EXCEPTION_TYPE         ExceptionType,
  IN OUT EFI_SYSTEM_CONTEXT         SystemContext
  )
{
  UINTN       BufferSize;
  VOID        *Buffer;
  EFI_STATUS  Status;
  CHAR16      *FileName;
  CHAR16      *CommandArg2;
  BOOLEAN     IsLoadCode;
  CHAR16      *DirName;
  CHAR16      CodFile[EFI_DEBUGGER_SYMBOL_NAME_MAX];
  CHAR16      *CodFileName;
  UINTN       Index;

  //
  // Check the argument
  //
  if (CommandArg == NULL) {
    EDBPrint (L"SymbolFile not found!\n");
    return EFI_DEBUG_CONTINUE;
  }

  IsLoadCode  = FALSE;
  CommandArg2 = StrGetNextTokenLine (L" ");
  if (CommandArg2 != NULL) {
    if (StriCmp (CommandArg2, L"a") == 0) {
      IsLoadCode = TRUE;
    } else {
      EDBPrint (L"Argument error!\n");
      return EFI_DEBUG_CONTINUE;
    }
  }

  if (StrLen (CommandArg) <= 4) {
    EDBPrint (L"SymbolFile name error!\n");
    return EFI_DEBUG_CONTINUE;
  }

  if (StriCmp (CommandArg + (StrLen (CommandArg) - 4), L".map") != 0) {
    EDBPrint (L"SymbolFile name error!\n");
    return EFI_DEBUG_CONTINUE;
  }

  //
  // Read MAP file to memory
  //
  Status = ReadFileToBuffer (DebuggerPrivate, CommandArg, &BufferSize, &Buffer, TRUE);
  if (EFI_ERROR (Status)) {
    EDBPrint (L"SymbolFile read error!\n");
    return EFI_DEBUG_CONTINUE;
  }

  FileName = GetFileNameFromFullPath (CommandArg);
  //
  // Load Symbol
  //
  Status = EdbLoadSymbol (DebuggerPrivate, FileName, BufferSize, Buffer);
  if (EFI_ERROR (Status)) {
    EDBPrint (L"LoadSymbol error!\n");
    gBS->FreePool (Buffer);
    return EFI_DEBUG_CONTINUE;
  }

  gBS->FreePool (Buffer);

  //
  // Patch Symbol for RVA
  //
  Status = EdbPatchSymbolRVA (DebuggerPrivate, FileName, EdbEbcImageRvaSearchTypeLast);
  if (EFI_ERROR (Status)) {
    EDBPrint (L"PatchSymbol RVA  - %r! Using the RVA in symbol file.\n", Status);
  } else {
    DEBUG ((DEBUG_ERROR, "PatchSymbol RVA successfully!\n"));
  }

  if (!IsLoadCode) {
    return EFI_DEBUG_CONTINUE;
  }

  //
  // load each cod file
  //
  DirName = GetDirNameFromFullPath (CommandArg);
  ZeroMem (CodFile, sizeof (CodFile));
  if (StrCmp (DirName, L"") != 0) {
    StrCpyS (CodFile, sizeof (CodFile), DirName);
  } else {
    DirName = L"\\";
  }

  //
  // Go throuth each file under this dir
  //
  Index       = 0;
  CodFileName = GetFileNameUnderDir (DebuggerPrivate, DirName, L".cod", &Index);
  while (CodFileName != NULL) {
    ZeroMem (CodFile, sizeof (CodFile));
    if (StrCmp (DirName, L"\\") != 0) {
      StrCpyS (CodFile, sizeof (CodFile), DirName);
    }

    //
    // read cod file to memory
    //
    Status = ReadFileToBuffer (DebuggerPrivate, ConstructFullPath (CodFile, CodFileName, EFI_DEBUGGER_SYMBOL_NAME_MAX - StrLen (CodFile) - 2), &BufferSize, &Buffer, FALSE);
    if (EFI_ERROR (Status)) {
      EDBPrint (L"CodeFile read error!\n");
      CodFileName = GetFileNameUnderDir (DebuggerPrivate, DirName, L".cod", &Index);
      continue;
    }

    //
    // Load Code
    //
    Status = EdbLoadCode (DebuggerPrivate, FileName, CodFileName, BufferSize, Buffer);
    if (EFI_ERROR (Status)) {
      EDBPrint (L"LoadCode error!\n");
      gBS->FreePool (Buffer);
      CodFileName = GetFileNameUnderDir (DebuggerPrivate, DirName, L".cod", &Index);
      continue;
    }

    //
    // Record the buffer
    //
    Status = EdbAddCodeBuffer (DebuggerPrivate, FileName, CodFileName, BufferSize, Buffer);
    if (EFI_ERROR (Status)) {
      EDBPrint (L"AddCodeBuffer error!\n");
      gBS->FreePool (Buffer);
      CodFileName = GetFileNameUnderDir (DebuggerPrivate, DirName, L".cod", &Index);
      continue;
    }

    //
    // Get next file
    //
    CodFileName = GetFileNameUnderDir (DebuggerPrivate, DirName, L".cod", &Index);
  }

  //
  // Done
  //
  return EFI_DEBUG_CONTINUE;
}

/**

  DebuggerCommand - UnloadSymbol

  @param  CommandArg      - The argument for this command
  @param  DebuggerPrivate - EBC Debugger private data structure
  @param  ExceptionType   - Exception type.
  @param  SystemContext   - EBC system context.

  @retval EFI_DEBUG_CONTINUE - formal return value

**/
EFI_DEBUG_STATUS
DebuggerUnloadSymbol (
  IN     CHAR16                     *CommandArg,
  IN     EFI_DEBUGGER_PRIVATE_DATA  *DebuggerPrivate,
  IN     EFI_EXCEPTION_TYPE         ExceptionType,
  IN OUT EFI_SYSTEM_CONTEXT         SystemContext
  )
{
  EFI_STATUS  Status;
  CHAR16      *FileName;
  CHAR16      *DirName;
  CHAR16      CodFile[EFI_DEBUGGER_SYMBOL_NAME_MAX];
  CHAR16      *CodFileName;
  UINTN       Index;
  VOID        *BufferPtr;

  //
  // Check the argument
  //
  if (CommandArg == NULL) {
    EDBPrint (L"SymbolFile not found!\n");
    return EFI_DEBUG_CONTINUE;
  }

  FileName = GetFileNameFromFullPath (CommandArg);

  //
  // Unload Code
  //
  DirName = GetDirNameFromFullPath (CommandArg);
  ZeroMem (CodFile, sizeof (CodFile));
  if (StrCmp (DirName, L"") != 0) {
    StrCpyS (CodFile, sizeof (CodFile), DirName);
  } else {
    DirName = L"\\";
  }

  //
  // Go through each file under this dir
  //
  Index       = 0;
  CodFileName = GetFileNameUnderDir (DebuggerPrivate, DirName, L".cod", &Index);
  while (CodFileName != NULL) {
    ZeroMem (CodFile, sizeof (CodFile));
    if (StrCmp (DirName, L"\\") != 0) {
      StrCpyS (CodFile, sizeof (CodFile), DirName);
    }

    //
    // Unload Code
    //
    Status = EdbUnloadCode (DebuggerPrivate, FileName, CodFileName, &BufferPtr);
    if (EFI_ERROR (Status)) {
      EDBPrint (L"UnloadCode error!\n");
      CodFileName = GetFileNameUnderDir (DebuggerPrivate, DirName, L".cod", &Index);
      continue;
    }

    //
    // Delete the code buffer
    //
    Status = EdbDeleteCodeBuffer (DebuggerPrivate, FileName, CodFileName, BufferPtr);
    if (EFI_ERROR (Status)) {
      EDBPrint (L"DeleteCodeBuffer error!\n");
      CodFileName = GetFileNameUnderDir (DebuggerPrivate, DirName, L".cod", &Index);
      continue;
    }

    //
    // Get next file
    //
    CodFileName = GetFileNameUnderDir (DebuggerPrivate, DirName, L".cod", &Index);
  }

  //
  // Unload Symbol
  //
  Status = EdbUnloadSymbol (DebuggerPrivate, FileName);
  if (EFI_ERROR (Status)) {
    EDBPrint (L"UnloadSymbol error!\n");
  }

  //
  // Done
  //
  return EFI_DEBUG_CONTINUE;
}

/**

  DebuggerCommand - DisplaySymbol.

  @param  CommandArg      - The argument for this command
  @param  DebuggerPrivate - EBC Debugger private data structure
  @param  ExceptionType   - Exception type.
  @param  SystemContext   - EBC system context.

  @retval EFI_DEBUG_CONTINUE - formal return value

**/
EFI_DEBUG_STATUS
DebuggerDisplaySymbol (
  IN     CHAR16                     *CommandArg,
  IN     EFI_DEBUGGER_PRIVATE_DATA  *DebuggerPrivate,
  IN     EFI_EXCEPTION_TYPE         ExceptionType,
  IN OUT EFI_SYSTEM_CONTEXT         SystemContext
  )
{
  if (CommandArg == NULL) {
    DebuggerPrivate->DebuggerSymbolContext.DisplaySymbol = !DebuggerPrivate->DebuggerSymbolContext.DisplaySymbol;
    EdbShowDisasm (DebuggerPrivate, SystemContext);
  } else if (StriCmp (CommandArg, L"on") == 0) {
    DebuggerPrivate->DebuggerSymbolContext.DisplaySymbol = TRUE;
    EdbShowDisasm (DebuggerPrivate, SystemContext);
  } else if (StriCmp (CommandArg, L"off") == 0) {
    DebuggerPrivate->DebuggerSymbolContext.DisplaySymbol = FALSE;
    EdbShowDisasm (DebuggerPrivate, SystemContext);
  } else {
    EDBPrint (L"DisplaySymbol - argument error\n");
  }

  return EFI_DEBUG_CONTINUE;
}

/**

  DebuggerCommand - LoadCode.

  @param  CommandArg      - The argument for this command
  @param  DebuggerPrivate - EBC Debugger private data structure
  @param  ExceptionType   - Exception type.
  @param  SystemContext   - EBC system context.

  @retval EFI_DEBUG_CONTINUE - formal return value

**/
EFI_DEBUG_STATUS
DebuggerLoadCode (
  IN     CHAR16                     *CommandArg,
  IN     EFI_DEBUGGER_PRIVATE_DATA  *DebuggerPrivate,
  IN     EFI_EXCEPTION_TYPE         ExceptionType,
  IN OUT EFI_SYSTEM_CONTEXT         SystemContext
  )
{
  UINTN       BufferSize;
  VOID        *Buffer;
  EFI_STATUS  Status;
  CHAR16      *CommandArg2;
  CHAR16      *FileName;
  CHAR16      *MapFileName;

  //
  // Check the argument
  //
  if (CommandArg == NULL) {
    EDBPrint (L"CodeFile not found!\n");
    return EFI_DEBUG_CONTINUE;
  }

  CommandArg2 = StrGetNextTokenLine (L" ");
  if (CommandArg2 == NULL) {
    EDBPrint (L"SymbolFile not found!\n");
    return EFI_DEBUG_CONTINUE;
  }

  if (StrLen (CommandArg) <= 4) {
    EDBPrint (L"CodeFile name error!\n");
    return EFI_DEBUG_CONTINUE;
  }

  if (StriCmp (CommandArg + (StrLen (CommandArg) - 4), L".cod") != 0) {
    EDBPrint (L"CodeFile name error!\n");
    return EFI_DEBUG_CONTINUE;
  }

  if (StrLen (CommandArg2) <= 4) {
    EDBPrint (L"SymbolFile name error!\n");
    return EFI_DEBUG_CONTINUE;
  }

  if (StriCmp (CommandArg2 + (StrLen (CommandArg2) - 4), L".map") != 0) {
    EDBPrint (L"SymbolFile name error!\n");
    return EFI_DEBUG_CONTINUE;
  }

  //
  // read cod file to memory
  //
  Status = ReadFileToBuffer (DebuggerPrivate, CommandArg, &BufferSize, &Buffer, TRUE);
  if (EFI_ERROR (Status)) {
    EDBPrint (L"CodeFile read error!\n");
    return EFI_DEBUG_CONTINUE;
  }

  FileName    = GetFileNameFromFullPath (CommandArg);
  MapFileName = GetFileNameFromFullPath (CommandArg2);
  //
  // Load Code
  //
  Status = EdbLoadCode (DebuggerPrivate, MapFileName, FileName, BufferSize, Buffer);
  if (EFI_ERROR (Status)) {
    EDBPrint (L"LoadCode error!\n");
    gBS->FreePool (Buffer);
    return EFI_DEBUG_CONTINUE;
  }

  //
  // Record the buffer
  //
  Status = EdbAddCodeBuffer (DebuggerPrivate, MapFileName, FileName, BufferSize, Buffer);
  if (EFI_ERROR (Status)) {
    EDBPrint (L"AddCodeBuffer error!\n");
    gBS->FreePool (Buffer);
    return EFI_DEBUG_CONTINUE;
  }

  //
  // Done
  //
  return EFI_DEBUG_CONTINUE;
}

/**

  DebuggerCommand - UnloadCode.

  @param  CommandArg      - The argument for this command
  @param  DebuggerPrivate - EBC Debugger private data structure
  @param  ExceptionType   - Exception type.
  @param  SystemContext   - EBC system context.

  @retval EFI_DEBUG_CONTINUE - formal return value

**/
EFI_DEBUG_STATUS
DebuggerUnloadCode (
  IN     CHAR16                     *CommandArg,
  IN     EFI_DEBUGGER_PRIVATE_DATA  *DebuggerPrivate,
  IN     EFI_EXCEPTION_TYPE         ExceptionType,
  IN OUT EFI_SYSTEM_CONTEXT         SystemContext
  )
{
  CHAR16      *CommandArg2;
  CHAR16      *FileName;
  CHAR16      *MapFileName;
  EFI_STATUS  Status;
  VOID        *BufferPtr;

  //
  // Check the argument
  //
  if (CommandArg == NULL) {
    EDBPrint (L"CodeFile not found!\n");
    return EFI_DEBUG_CONTINUE;
  }

  CommandArg2 = StrGetNextTokenLine (L" ");
  if (CommandArg2 == NULL) {
    EDBPrint (L"SymbolFile not found!\n");
    return EFI_DEBUG_CONTINUE;
  }

  FileName    = GetFileNameFromFullPath (CommandArg);
  MapFileName = GetFileNameFromFullPath (CommandArg2);

  //
  // Unload Code
  //
  Status = EdbUnloadCode (DebuggerPrivate, MapFileName, FileName, &BufferPtr);
  if (EFI_ERROR (Status)) {
    EDBPrint (L"UnloadCode error!\n");
    return EFI_DEBUG_CONTINUE;
  }

  //
  // Delete Code buffer
  //
  Status = EdbDeleteCodeBuffer (DebuggerPrivate, MapFileName, FileName, BufferPtr);
  if (EFI_ERROR (Status)) {
    EDBPrint (L"DeleteCodeBuffer error!\n");
  }

  //
  // Done
  //
  return EFI_DEBUG_CONTINUE;
}

/**

  DebuggerCommand - DisplayCode.

  @param  CommandArg      - The argument for this command
  @param  DebuggerPrivate - EBC Debugger private data structure
  @param  ExceptionType   - Exception type.
  @param  SystemContext   - EBC system context.

  @retval EFI_DEBUG_CONTINUE - formal return value

**/
EFI_DEBUG_STATUS
DebuggerDisplayCode (
  IN     CHAR16                     *CommandArg,
  IN     EFI_DEBUGGER_PRIVATE_DATA  *DebuggerPrivate,
  IN     EFI_EXCEPTION_TYPE         ExceptionType,
  IN OUT EFI_SYSTEM_CONTEXT         SystemContext
  )
{
  if (CommandArg == NULL) {
    DebuggerPrivate->DebuggerSymbolContext.DisplayCodeOnly = !DebuggerPrivate->DebuggerSymbolContext.DisplayCodeOnly;
    EdbShowDisasm (DebuggerPrivate, SystemContext);
  } else if (StriCmp (CommandArg, L"on") == 0) {
    DebuggerPrivate->DebuggerSymbolContext.DisplayCodeOnly = TRUE;
    EdbShowDisasm (DebuggerPrivate, SystemContext);
  } else if (StriCmp (CommandArg, L"off") == 0) {
    DebuggerPrivate->DebuggerSymbolContext.DisplayCodeOnly = FALSE;
    EdbShowDisasm (DebuggerPrivate, SystemContext);
  } else {
    EDBPrint (L"DisplayCode - argument error\n");
  }

  return EFI_DEBUG_CONTINUE;
}
