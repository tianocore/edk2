/*++

Copyright (c)  2004-2006 Intel Corporation. All rights reserved
This program and the accompanying materials are licensed and made available 
under the terms and conditions of the BSD License which accompanies this 
distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

  Symbol.c

Abstract:

  Class-like implementation for a symbol table.

--*/

// GC_TODO: fix comment to set correct module name: Symbols.c
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
//
// for isspace()
//
#include <ctype.h>

#include <Common/UefiBaseTypes.h>

#include "CommonLib.h"
#include "EfiUtilityMsgs.h"
#include "Symbols.h"

#define MAX_LINE_LEN  512

//
// Linked list to keep track of all symbols
//
typedef struct _SYMBOL {
  struct _SYMBOL  *Next;
  int             Type;
  char            *Name;
  char            *Value;
} SYMBOL;

static
SYMBOL        *
FreeSymbols (
  SYMBOL *Syms
  );

static
int
ExpandMacros (
  char  *SourceLine,
  char  *DestLine,
  int   LineLen
  );

static SYMBOL *mSymbolTable = NULL;

void
SymbolsConstructor (
  VOID
  )
/*++

Routine Description:

  GC_TODO: Add function description

Arguments:

  None

Returns:

  GC_TODO: add return values

--*/
{
  SymbolsDestructor ();
}

void
SymbolsDestructor (
  VOID
  )
/*++

Routine Description:

  GC_TODO: Add function description

Arguments:

  None

Returns:

  GC_TODO: add return values

--*/
{
  mSymbolTable = FreeSymbols (mSymbolTable);
}

char *
GetSymbolValue (
  char *SymbolName
  )
/*++

Routine Description:
  
  Look up a symbol in our symbol table.

Arguments:

  SymbolName

Returns:

  Pointer to the value of the symbol if found
  NULL if the symbol is not found

--*/
// GC_TODO:    SymbolName - add argument and description to function comment
{
  SYMBOL  *Symbol;
  //
  // Walk the symbol table
  //
  Symbol = mSymbolTable;
  while (Symbol) {
    if (stricmp (SymbolName, Symbol->Name) == 0) {
      return Symbol->Value;
    }

    Symbol = Symbol->Next;
  }

  return NULL;
}

int
SymbolAdd (
  char    *Name,
  char    *Value,
  int     Mode
  )
/*++

Routine Description:
  
  Add a symbol name/value to the symbol table

Arguments:

  Name  - name of symbol to add
  Value - value of symbol to add
  Mode  - currrently unused

Returns:

  Length of symbol added.

Notes:
  If Value == NULL, then this routine will assume that the Name field
  looks something like "MySymName = MySymValue", and will try to parse
  it that way and add the symbol name/pair from the string.

--*/
{
  SYMBOL  *Symbol;

  SYMBOL  *NewSymbol;
  int     Len;
  char    *Start;
  char    *Cptr;
  char    CSave;
  char    *SaveCptr;

  Len       = 0;
  SaveCptr  = NULL;
  CSave     = 0;
  //
  // If value pointer is null, then they passed us a line something like:
  //    varname = value, or simply var =
  //
  if (Value == NULL) {
    Start = Name;
    while (*Name && isspace (*Name)) {
      Name++;
    }

    if (Name == NULL) {
      return -1;
    }
    //
    // Find the end of the name. Either space or a '='.
    //
    for (Value = Name; *Value && !isspace (*Value) && (*Value != '='); Value++)
      ;
    if (Value == NULL) {
      return -1;
    }
    //
    // Look for the '='
    //
    Cptr = Value;
    while (*Value && (*Value != '=')) {
      Value++;
    }

    if (Value == NULL) {
      return -1;
    }
    //
    // Now truncate the name
    //
    *Cptr = 0;
    //
    // Skip over the = and then any spaces
    //
    Value++;
    while (*Value && isspace (*Value)) {
      Value++;

    }
    //
    // Find end of string, checking for quoted string
    //
    if (*Value == '\"') {
      Value++;
      for (Cptr = Value; *Cptr && *Cptr != '\"'; Cptr++)
        ;
    } else {
      for (Cptr = Value; *Cptr && !isspace (*Cptr); Cptr++)
        ;
    }
    //
    // Null terminate the value string
    //
    CSave     = *Cptr;
    SaveCptr  = Cptr;
    *Cptr     = 0;
    Len       = (int) (Cptr - Start);
  }
  //
  // We now have a symbol name and a value. Look for an existing variable
  // and overwrite it.
  //
  Symbol = mSymbolTable;
  while (Symbol) {
    //
    // Check for symbol name match
    //
    if (stricmp (Name, Symbol->Name) == 0) {
      _free (Symbol->Value);
      Symbol->Value = (char *) _malloc (strlen (Value) + 1);
      if (Symbol->Value == NULL) {
        Error (NULL, 0, 0, NULL, "failed to allocate memory");
        return -1;
      }

      strcpy (Symbol->Value, Value);
      //
      // If value == "NULL", then make it a 0-length string
      //
      if (stricmp (Symbol->Value, "NULL") == 0) {
        Symbol->Value[0] = 0;
      }

      return Len;
    }

    Symbol = Symbol->Next;
  }
  //
  // Does not exist, create a new one
  //
  NewSymbol = (SYMBOL *) _malloc (sizeof (SYMBOL));
  if (NewSymbol == NULL) {
    Error (NULL, 0, 0, NULL, "memory allocation failure");
    return -1;
  }

  memset ((char *) NewSymbol, 0, sizeof (SYMBOL));
  NewSymbol->Name = (char *) _malloc (strlen (Name) + 1);
  if (NewSymbol->Name == NULL) {
    Error (NULL, 0, 0, NULL, "memory allocation failure");
    _free (NewSymbol);
    return -1;
  }

  NewSymbol->Value = (char *) _malloc (strlen (Value) + 1);
  if (NewSymbol->Value == NULL) {
    Error (NULL, 0, 0, NULL, "memory allocation failure");
    _free (NewSymbol->Name);
    _free (NewSymbol);
    return -1;
  }

  strcpy (NewSymbol->Name, Name);
  strcpy (NewSymbol->Value, Value);
  //
  // Remove trailing spaces
  //
  Cptr = NewSymbol->Value + strlen (NewSymbol->Value) - 1;
  while (Cptr > NewSymbol->Value) {
    if (isspace (*Cptr)) {
      *Cptr = 0;
      Cptr--;
    } else {
      break;
    }
  }
  //
  // Add it to the head of the list.
  //
  NewSymbol->Next = mSymbolTable;
  mSymbolTable    = NewSymbol;
  //
  // If value == "NULL", then make it a 0-length string
  //
  if (stricmp (NewSymbol->Value, "NULL") == 0) {
    NewSymbol->Value[0] = 0;
  }
  //
  // Restore the terminator we inserted if they passed in var=value
  //
  if (SaveCptr != NULL) {
    *SaveCptr = CSave;
  }
  _free (NewSymbol->Value);
  _free (NewSymbol->Name);
  _free (NewSymbol);
  return Len;
}

static
STATUS
RemoveSymbol (
  char *Name,
  char SymbolType
  )
/*++

Routine Description:
  
  Remove a symbol name/value from the symbol table

Arguments:

  Name  - name of symbol to remove
  SymbolType - type of symbol to remove

Returns:

  STATUS_SUCCESS - matching symbol found and removed
  STATUS_ERROR   - matching symbol not found in symbol table

--*/
{
  SYMBOL  *Symbol;

  SYMBOL  *PrevSymbol;

  PrevSymbol  = NULL;
  Symbol      = mSymbolTable;
  //
  // Walk the linked list of symbols in the symbol table looking
  // for a match of both symbol name and type.
  //
  while (Symbol) {
    if ((stricmp (Name, Symbol->Name) == 0) && (Symbol->Type & SymbolType)) {
      //
      // If the symbol has a value associated with it, free the memory
      // allocated for the value.
      // Then free the memory allocated for the symbols string name.
      //
      if (Symbol->Value) {
        _free (Symbol->Value);
      }

      _free (Symbol->Name);
      //
      // Link the previous symbol to the next symbol to effectively
      // remove this symbol from the linked list.
      //
      if (PrevSymbol) {
        PrevSymbol->Next = Symbol->Next;
      } else {
        mSymbolTable = Symbol->Next;
      }

      _free (Symbol);
      return STATUS_SUCCESS;
    }

    PrevSymbol  = Symbol;
    Symbol      = Symbol->Next;
  }

  return STATUS_WARNING;
}

static
SYMBOL *
FreeSymbols (
  SYMBOL *Syms
  )
/*++

Routine Description:

  GC_TODO: Add function description

Arguments:

  Syms  - GC_TODO: add argument description

Returns:

  GC_TODO: add return values

--*/
{
  SYMBOL  *Next;
  while (Syms) {
    if (Syms->Name != NULL) {
      _free (Syms->Name);
    }

    if (Syms->Value != NULL) {
      _free (Syms->Value);
    }

    Next = Syms->Next;
    _free (Syms);
    Syms = Next;
  }

  return Syms;
}

static
int
ExpandMacros (
  char  *SourceLine,
  char  *DestLine,
  int   LineLen
  )
/*++

Routine Description:
  
  Given a line of text, replace all variables of format $(NAME) with values
  from our symbol table.

Arguments:

  SourceLine    - input line of text to do symbol replacements on
  DestLine      - on output, SourceLine with symbols replaced
  LineLen       - length of DestLine, so we don't exceed its allocated length

Returns:

  STATUS_SUCCESS - no problems encountered
  STATUS_WARNING - missing closing parenthesis on a symbol reference in SourceLine
  STATUS_ERROR   - memory allocation failure

--*/
{
  static int  NestDepth = 0;
  char        *FromPtr;
  char        *ToPtr;
  char        *SaveStart;
  char        *Cptr;
  char        *value;
  int         Expanded;
  int         ExpandedCount;
  INT8        *LocalDestLine;
  STATUS      Status;
  int         LocalLineLen;

  NestDepth++;
  Status        = STATUS_SUCCESS;
  LocalDestLine = (char *) _malloc (LineLen);
  if (LocalDestLine == NULL) {
    Error (__FILE__, __LINE__, 0, "memory allocation failed", NULL);
    return STATUS_ERROR;
  }

  FromPtr = SourceLine;
  ToPtr   = LocalDestLine;
  //
  // Walk the entire line, replacing $(MACRO_NAME).
  //
  LocalLineLen  = LineLen;
  ExpandedCount = 0;
  while (*FromPtr && (LocalLineLen > 0)) {
    if ((*FromPtr == '$') && (*(FromPtr + 1) == '(')) {
      //
      // Save the start in case it's undefined, in which case we copy it as-is.
      //
      SaveStart = FromPtr;
      Expanded  = 0;
      //
      // Macro expansion time. Find the end (no spaces allowed)
      //
      FromPtr += 2;
      for (Cptr = FromPtr; *Cptr && (*Cptr != ')'); Cptr++)
        ;
      if (*Cptr) {
        //
        // Truncate the string at the closing parenthesis for ease-of-use.
        // Then copy the string directly to the destination line in case we don't find
        // a definition for it.
        //
        *Cptr = 0;
        strcpy (ToPtr, SaveStart);
        if ((value = GetSymbolValue (FromPtr)) != NULL) {
          strcpy (ToPtr, value);
          LocalLineLen -= strlen (value);
          ToPtr += strlen (value);
          Expanded = 1;
          ExpandedCount++;
        }

        if (!Expanded) {
          //
          // Restore closing parenthesis, and advance to next character
          //
          *Cptr   = ')';
          FromPtr = SaveStart + 1;
          ToPtr++;
        } else {
          FromPtr = Cptr + 1;
        }
      } else {
        Error (NULL, 0, 0, SourceLine, "missing closing parenthesis on macro");
        strcpy (ToPtr, FromPtr);
        Status = STATUS_WARNING;
        goto Done;
      }
    } else {
      *ToPtr = *FromPtr;
      FromPtr++;
      ToPtr++;
      LocalLineLen--;
    }
  }

  if (*FromPtr == 0) {
    *ToPtr = 0;
  }

  //
  // If we expanded at least one string successfully, then make a recursive call to try again.
  //
  if ((ExpandedCount != 0) && (Status == STATUS_SUCCESS) && (NestDepth < 10)) {
    Status = ExpandMacros (LocalDestLine, DestLine, LineLen);
    _free (LocalDestLine);
    NestDepth = 0;
    return Status;
  }

Done:
  if (Status != STATUS_ERROR) {
    strcpy (DestLine, LocalDestLine);
  }

  NestDepth = 0;
  _free (LocalDestLine);
  return Status;
}

STATUS
SymbolsFileStringsReplace (
  char    *InFileName,
  char    *OutFileName
  )
/*++

Routine Description:
  
  Given input and output file names, read in the input file, replace variable
  references of format $(NAME) with appropriate values from our symbol table,
  and write the result out to the output file.

Arguments:

  InFileName  - name of input text file to replace variable references
  OutFileName - name of output text file to write results to

Returns:

  STATUS_SUCCESS - no problems encountered
  STATUS_ERROR   - failed to open input or output file

--*/
{
  STATUS  Status;
  FILE    *InFptr;
  FILE    *OutFptr;
  char    Line[MAX_LINE_LEN];
  char    OutLine[MAX_LINE_LEN];

  Status = STATUS_ERROR;
  //
  // Open input and output files
  //
  InFptr  = NULL;
  OutFptr = NULL;
  if ((InFptr = fopen (InFileName, "r")) == NULL) {
    Error (NULL, 0, 0, InFileName, "failed to open input file for reading");
    goto Done;
  }

  if ((OutFptr = fopen (OutFileName, "w")) == NULL) {
    Error (NULL, 0, 0, OutFileName, "failed to open output file for writing");
    goto Done;
  }
  //
  // Read lines from input file until done
  //
  while (fgets (Line, sizeof (Line), InFptr) != NULL) {
    ExpandMacros (Line, OutLine, sizeof (OutLine));
    fprintf (OutFptr, OutLine);
  }

  Status = STATUS_SUCCESS;
Done:
  if (InFptr != NULL) {
    fclose (InFptr);
  }

  if (OutFptr != NULL) {
    fclose (OutFptr);
  }

  return Status;
}
