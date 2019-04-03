/** @file

Copyright (c) 2013-2015 Intel Corporation.

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef QNCX_SMM_HELPERS_H
#define QNCX_SMM_HELPERS_H

//
// Include common header file for this module.
//
#include "CommonHeader.h"

#include "QNCSmm.h"

EFI_STATUS
QNCSmmInitHardware (
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
;

EFI_STATUS
QNCSmmEnableGlobalSmiBit (
  VOID
  )
/*++

Routine Description:

  Enables the QNC to generate SMIs. Note that no SMIs will be generated
  if no SMI sources are enabled. Conversely, no enabled SMI source will
  generate SMIs if SMIs are not globally enabled. This is the main
  switchbox for SMI generation.

Arguments:

  None

Returns:

  EFI_SUCCESS.
  Asserts, otherwise.

--*/
;

EFI_STATUS
QNCSmmClearSmi (
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
;

BOOLEAN
QNCSmmSetAndCheckEos (
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
;

BOOLEAN
QNCSmmGetSciEn (
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
;

//
// ///////////////////////////////////////////////////////////////////////////
//
// These may or may not need to change w/ the QNC version;
// they're here because they're highly IA-32 dependent.
//
BOOLEAN
ReadBitDesc (
  CONST QNC_SMM_BIT_DESC *BitDesc
  )
/*++

Routine Description:

  GC_TODO: Add function description

Arguments:

  BitDesc - GC_TODO: add argument description

Returns:

  GC_TODO: add return values

--*/
;

VOID
WriteBitDesc (
  CONST QNC_SMM_BIT_DESC  *BitDesc,
  CONST BOOLEAN          ValueToWrite
  )
/*++

Routine Description:

  GC_TODO: Add function description

Arguments:

  BitDesc       - GC_TODO: add argument description
  ValueToWrite  - GC_TODO: add argument description

Returns:

  GC_TODO: add return values

--*/
;

#endif
