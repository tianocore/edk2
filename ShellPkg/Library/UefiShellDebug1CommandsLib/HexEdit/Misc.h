/** @file
    Definitions for various line and string routines

  Copyright (c) 2005 - 2011, Intel Corporation. All rights reserved. <BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _LIB_MISC_H_
#define _LIB_MISC_H_

#include "HexEditor.h"

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
  );

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
  );

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
  );

/**
  Get the X information for the mouse.

  @param[in] GuidX      The change.

  @return the new information.
**/
INT32
HGetTextX (
  IN INT32 GuidX
  );

/**
  Get the Y information for the mouse.

  @param[in] GuidY      The change.

  @return the new information.
**/
INT32
HGetTextY (
  IN INT32 GuidY
  );

#endif
