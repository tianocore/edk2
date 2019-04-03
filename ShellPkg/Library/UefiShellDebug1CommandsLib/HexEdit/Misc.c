/** @file
  Implementation of various string and line routines

  Copyright (c) 2005 - 2018, Intel Corporation. All rights reserved. <BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "HexEditor.h"

extern BOOLEAN  HEditorMouseAction;

/**
  Free a line and it's internal buffer.

  @param[in] Src    The line to be freed.
**/
VOID
HLineFree (
  IN  HEFI_EDITOR_LINE *Src
  )
{
  if (Src == NULL) {
    return ;
  }

  SHELL_FREE_NON_NULL (Src);

}

/**
  Advance to the next Count lines.

  @param[in] Count      The line number to advance.

  @retval NULL An error occured.
  @return A pointer to the line after advance.
**/
HEFI_EDITOR_LINE *
HLineAdvance (
  IN  UINTN Count
  )
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

/**
  Retreat to the previous Count lines.

  @param[in] Count    The line number to retreat.

  @retval NULL An error occured.
  @return A pointer to the line after retreat.
**/
HEFI_EDITOR_LINE *
HLineRetreat (
  IN  UINTN Count
  )
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

/**
  Advance/Retreat lines.

  @param[in] Count      The line number to advance/retreat.
                            >0 : advance
                            <0: retreat

  @retval NULL An error occured.
  @return A pointer to the line after move.
**/
HEFI_EDITOR_LINE *
HMoveLine (
  IN  INTN Count
  )
{
  HEFI_EDITOR_LINE  *Line;
  UINTN             AbsCount;

  //
  // difference with MoveCurrentLine
  //     just return Line
  //     do not set currentline to Line
  //
  if (Count <= 0) {
    AbsCount  = (UINTN)ABS(Count);
    Line      = HLineRetreat (AbsCount);
  } else {
    Line = HLineAdvance ((UINTN)Count);
  }

  return Line;
}

/**
  Advance/Retreat lines and set CurrentLine in BufferImage to it.

  @param[in] Count    The line number to advance/retreat.
                          >0 : advance
                          <0: retreat

  @retval NULL An error occured.
  @return A pointer to the line after move.
**/
HEFI_EDITOR_LINE *
HMoveCurrentLine (
  IN  INTN Count
  )
{
  HEFI_EDITOR_LINE  *Line;
  UINTN             AbsCount;

  //
  // <0: retreat
  // >0: advance
  //
  if (Count <= 0) {
    AbsCount  = (UINTN)ABS(Count);
    Line      = HLineRetreat (AbsCount);
  } else {
    Line = HLineAdvance ((UINTN)Count);
  }

  if (Line == NULL) {
    return NULL;
  }

  HMainEditor.BufferImage->CurrentLine = Line;

  return Line;
}


/**
  Free all the lines in HBufferImage.
    Fields affected:
    Lines
    CurrentLine
    NumLines
    ListHead

  @param[in] ListHead     The list head.
  @param[in] Lines        The lines.

  @retval EFI_SUCCESS     The operation was successful.
**/
EFI_STATUS
HFreeLines (
  IN LIST_ENTRY   *ListHead,
  IN HEFI_EDITOR_LINE *Lines
  )
{
  LIST_ENTRY        *Link;
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

/**
  Get the X information for the mouse.

  @param[in] GuidX      The change.

  @return the new information.
**/
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

/**
  Get the Y information for the mouse.

  @param[in] GuidY      The change.

  @return the new information.
**/
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
