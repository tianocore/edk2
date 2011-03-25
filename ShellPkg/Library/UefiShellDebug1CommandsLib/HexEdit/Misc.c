/** @file
  Implementation of various string and line routines
  
  Copyright (c) 2005 - 2011, Intel Corporation. All rights reserved. <BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "HexEditor.h"

extern BOOLEAN  HEditorMouseAction;

VOID
HEditorClearLine (
  IN UINTN Row
  )
/*++

Routine Description: 

  Clear line at Row

Arguments:  

  Row -- row number to be cleared ( start from 1 )

Returns:  

  EFI_SUCCESS

--*/
{
  CHAR16  Line[200];
  UINTN   Index;
  UINTN   Limit;
  UINTN   StartCol;

  if (HEditorMouseAction) {
    Limit     = 3 * 0x10;
    StartCol  = 10;
  } else {
    Limit     = HMainEditor.ScreenSize.Column;
    StartCol  = 1;
  }
  //
  // prepare a blank line
  //
  for (Index = 0; Index < Limit; Index++) {
    Line[Index] = ' ';
  }

  if (Row == HMainEditor.ScreenSize.Row && Limit == HMainEditor.ScreenSize.Column) {
    //
    // if '\0' is still at position 80, it will cause first line error
    //
    Line[Limit - 1] = '\0';
  } else {
    Line[Limit] = '\0';
  }
  //
  // print out the blank line
  //
  ShellPrintEx ((INT32)StartCol - 1, (INT32)Row - 1, Line);
}

HEFI_EDITOR_LINE *
HLineDup (
  IN  HEFI_EDITOR_LINE *Src
  )
/*++

Routine Description: 

  Duplicate a line

Arguments:  

  Src -- line to be duplicated

Returns:  

  NULL -- wrong
  Not NULL -- line created

--*/
{
  HEFI_EDITOR_LINE  *Dest;

  //
  // allocate for the line structure
  //
  Dest = AllocateZeroPool (sizeof (HEFI_EDITOR_LINE));
  if (Dest == NULL) {
    return NULL;
  }

  Dest->Signature = EFI_EDITOR_LINE_LIST;
  Dest->Size      = Src->Size;

  CopyMem (Dest->Buffer, Src->Buffer, 0x10);

  Dest->Link = Src->Link;

  return Dest;
}

VOID
HLineFree (
  IN  HEFI_EDITOR_LINE *Src
  )
/*++

Routine Description: 

  Free a line and it's internal buffer

Arguments:  

  Src -- line to be freed

Returns:  

  None

--*/
{
  if (Src == NULL) {
    return ;
  }

  SHELL_FREE_NON_NULL (Src);

}

HEFI_EDITOR_LINE *
_HLineAdvance (
  IN  UINTN Count
  )
/*++

Routine Description: 

  Advance to the next Count lines

Arguments:  

  Count -- line number to advance

Returns:  

  NULL -- wrong
  Not NULL -- line after advance

--*/
{
  UINTN             Index;
  HEFI_EDITOR_LINE  *Line;

  Line = HMainEditor.BufferImage->CurrentLine;
  if (Line == NULL) {
    return NULL;
  }

  for (Index = 0; Index < Count; Index++) {
    //
    // if already last line
    //
    if (Line->Link.ForwardLink == HMainEditor.BufferImage->ListHead) {
      return NULL;
    }

    Line = CR (Line->Link.ForwardLink, HEFI_EDITOR_LINE, Link, EFI_EDITOR_LINE_LIST);
  }

  return Line;
}

HEFI_EDITOR_LINE *
_HLineRetreat (
  IN  UINTN Count
  )
/*++

Routine Description: 

  Retreat to the previous Count lines

Arguments:  

  Count -- line number to retreat

Returns:  

  NULL -- wrong
  Not NULL -- line after retreat

--*/
{
  UINTN             Index;
  HEFI_EDITOR_LINE  *Line;

  Line = HMainEditor.BufferImage->CurrentLine;
  if (Line == NULL) {
    return NULL;
  }

  for (Index = 0; Index < Count; Index++) {
    //
    // already the first line
    //
    if (Line->Link.BackLink == HMainEditor.BufferImage->ListHead) {
      return NULL;
    }

    Line = CR (Line->Link.BackLink, HEFI_EDITOR_LINE, Link, EFI_EDITOR_LINE_LIST);
  }

  return Line;
}

HEFI_EDITOR_LINE *
HMoveLine (
  IN  INTN Count
  )
/*++

Routine Description: 

  Advance/Retreat lines

Arguments:  

  Count -- line number to advance/retreat
    >0 : advance
    <0: retreat  

Returns:  

  NULL -- wrong
  Not NULL -- line after advance

--*/
{
  HEFI_EDITOR_LINE  *Line;
  UINTN             AbsCount;

  //
  // difference with MoveCurrentLine
  //     just return Line
  //     do not set currentline to Line
  //
  if (Count <= 0) {
    AbsCount  = -Count;
    Line      = _HLineRetreat (AbsCount);
  } else {
    Line = _HLineAdvance (Count);
  }

  return Line;
}

HEFI_EDITOR_LINE *
HMoveCurrentLine (
  IN  INTN Count
  )
/*++

Routine Description: 

  Advance/Retreat lines and set CurrentLine in BufferImage to it

Arguments:  

  Count -- line number to advance/retreat
    >0 : advance
    <0: retreat

Returns:  

  NULL -- wrong
  Not NULL -- line after advance


--*/
{
  HEFI_EDITOR_LINE  *Line;
  UINTN             AbsCount;

  //
  // <0: retreat
  // >0: advance
  //
  if (Count <= 0) {
    AbsCount  = -Count;
    Line      = _HLineRetreat (AbsCount);
  } else {
    Line = _HLineAdvance (Count);
  }

  if (Line == NULL) {
    return NULL;
  }

  HMainEditor.BufferImage->CurrentLine = Line;

  return Line;
}


EFI_STATUS
HFreeLines (
  IN LIST_ENTRY   *ListHead,
  IN HEFI_EDITOR_LINE *Lines
  )
/*++

Routine Description: 

  Free all the lines in HBufferImage
    Fields affected:
    Lines
    CurrentLine
    NumLines
    ListHead 

Arguments:  

  ListHead - The list head
  Lines - The lines

Returns:  

  EFI_SUCCESS

--*/
{
  LIST_ENTRY    *Link;
  HEFI_EDITOR_LINE  *Line;

  //
  // release all the lines
  //
  if (Lines != NULL) {

    Line  = Lines;
    Link  = &(Line->Link);
    do {
      Line  = CR (Link, HEFI_EDITOR_LINE, Link, EFI_EDITOR_LINE_LIST);
      Link  = Link->ForwardLink;
      HLineFree (Line);
    } while (Link != ListHead);
  }

  ListHead->ForwardLink = ListHead;
  ListHead->BackLink = ListHead;

  return EFI_SUCCESS;
}

UINTN
HStrStr (
  IN  CHAR16  *Str,
  IN  CHAR16  *Pat
  )
/*++

Routine Description: 

  Search Pat in Str

Arguments:  

  Str -- mother string
  Pat -- search pattern


Returns:  

  0 : not found
  >= 1 : found position + 1

--*/
{
  INTN  *Failure;
  INTN  i;
  INTN  j;
  INTN  Lenp;
  INTN  Lens;

  //
  // this function copies from some lib
  //
  Lenp        = StrLen (Pat);
  Lens        = StrLen (Str);

  Failure     = AllocateZeroPool (Lenp * sizeof (INTN));
  Failure[0]  = -1;
  for (j = 1; j < Lenp; j++) {
    i = Failure[j - 1];
    while ((Pat[j] != Pat[i + 1]) && (i >= 0)) {
      i = Failure[i];
    }

    if (Pat[j] == Pat[i + 1]) {
      Failure[j] = i + 1;
    } else {
      Failure[j] = -1;
    }
  }

  i = 0;
  j = 0;
  while (i < Lens && j < Lenp) {
    if (Str[i] == Pat[j]) {
      i++;
      j++;
    } else if (j == 0) {
      i++;
    } else {
      j = Failure[j - 1] + 1;
    }
  }

  FreePool (Failure);

  //
  // 0: not found
  // >=1 : found position + 1
  //
  return ((j == Lenp) ? (i - Lenp) : -1) + 1;

}

INT32
HGetTextX (
  IN INT32 GuidX
  )
{
  INT32 Gap;

  HMainEditor.MouseAccumulatorX += GuidX;
  Gap = (HMainEditor.MouseAccumulatorX * (INT32) HMainEditor.ScreenSize.Column) / (INT32) (50 * (INT32) HMainEditor.MouseInterface->Mode->ResolutionX);
  HMainEditor.MouseAccumulatorX = (HMainEditor.MouseAccumulatorX * (INT32) HMainEditor.ScreenSize.Column) % (INT32) (50 * (INT32) HMainEditor.MouseInterface->Mode->ResolutionX);
  HMainEditor.MouseAccumulatorX = HMainEditor.MouseAccumulatorX / (INT32) HMainEditor.ScreenSize.Column;
  return Gap;
}

INT32
HGetTextY (
  IN INT32 GuidY
  )
{
  INT32 Gap;

  HMainEditor.MouseAccumulatorY += GuidY;
  Gap = (HMainEditor.MouseAccumulatorY * (INT32) HMainEditor.ScreenSize.Row) / (INT32) (50 * (INT32) HMainEditor.MouseInterface->Mode->ResolutionY);
  HMainEditor.MouseAccumulatorY = (HMainEditor.MouseAccumulatorY * (INT32) HMainEditor.ScreenSize.Row) % (INT32) (50 * (INT32) HMainEditor.MouseInterface->Mode->ResolutionY);
  HMainEditor.MouseAccumulatorY = HMainEditor.MouseAccumulatorY / (INT32) HMainEditor.ScreenSize.Row;

  return Gap;
}

EFI_STATUS
HXtoi (
  IN  CHAR16  *Str,
  OUT UINTN   *Value
  )
/*++
Routine Description:

  convert hex string to uint
  
Arguments:

  Str   - The string
  Value - The value

Returns:


--*/
{
  UINT64  u;
  CHAR16  c;
  UINTN   Size;

  Size = sizeof (UINTN);

  //
  // skip leading white space
  //
  while (*Str && *Str == ' ') {
    Str += 1;
  }

  if (StrLen (Str) > Size * 2) {
    return EFI_LOAD_ERROR;
  }
  //
  // convert hex digits
  //
  u = 0;
  c = *Str;
  while (c) {
    c = *Str;
    Str++;

    if (c == 0) {
      break;
    }
    //
    // not valid char
    //
    if (!((c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F') || (c >= '0' && c <= '9') || (c == '\0'))) {
      return EFI_LOAD_ERROR;
    }

    if (c >= 'a' && c <= 'f') {
      c -= 'a' - 'A';
    }

    if ((c >= '0' && c <= '9') || (c >= 'A' && c <= 'F')) {
      u = LShiftU64 (u, 4) + (c - (c >= 'A' ? 'A' - 10 : '0'));
    } else {
      //
      // '\0'
      //
      break;
    }
  }

  *Value = (UINTN) u;

  return EFI_SUCCESS;
}
