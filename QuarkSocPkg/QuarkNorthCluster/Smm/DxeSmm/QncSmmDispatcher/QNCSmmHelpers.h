/** @file

Copyright (c) 2013-2015 Intel Corporation.

This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef QNC_SMM_HELPERS_H
#define QNC_SMM_HELPERS_H

//
// Include common header file for this module.
//
#include "CommonHeader.h"

#include "QNCSmm.h"
#include "QNCxSmmHelpers.h"

//
// /////////////////////////////////////////////////////////////////////////////
// SUPPORT / HELPER FUNCTIONS (QNC version-independent)
//
VOID
QNCSmmPublishDispatchProtocols (
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
CompareEnables (
  CONST IN QNC_SMM_SOURCE_DESC *Src1,
  CONST IN QNC_SMM_SOURCE_DESC *Src2
  )
/*++

Routine Description:

  GC_TODO: Add function description

Arguments:

  Src1  - GC_TODO: add argument description
  Src2  - GC_TODO: add argument description

Returns:

  GC_TODO: add return values

--*/
;

BOOLEAN
CompareStatuses (
  CONST IN QNC_SMM_SOURCE_DESC *Src1,
  CONST IN QNC_SMM_SOURCE_DESC *Src2
  )
/*++

Routine Description:

  GC_TODO: Add function description

Arguments:

  Src1  - GC_TODO: add argument description
  Src2  - GC_TODO: add argument description

Returns:

  GC_TODO: add return values

--*/
;

BOOLEAN
CompareSources (
  CONST IN QNC_SMM_SOURCE_DESC *Src1,
  CONST IN QNC_SMM_SOURCE_DESC *Src2
  )
/*++

Routine Description:

  GC_TODO: Add function description

Arguments:

  Src1  - GC_TODO: add argument description
  Src2  - GC_TODO: add argument description

Returns:

  GC_TODO: add return values

--*/
;

BOOLEAN
SourceIsActive (
  CONST IN QNC_SMM_SOURCE_DESC *Src
  )
/*++

Routine Description:

  GC_TODO: Add function description

Arguments:

  Src - GC_TODO: add argument description

Returns:

  GC_TODO: add return values

--*/
;

VOID
QNCSmmEnableSource (
  CONST QNC_SMM_SOURCE_DESC *SrcDesc
  )
/*++

Routine Description:

  GC_TODO: Add function description

Arguments:

  SrcDesc - GC_TODO: add argument description

Returns:

  GC_TODO: add return values

--*/
;

VOID
QNCSmmDisableSource (
  CONST QNC_SMM_SOURCE_DESC *SrcDesc
  )
/*++

Routine Description:

  GC_TODO: Add function description

Arguments:

  SrcDesc - GC_TODO: add argument description

Returns:

  GC_TODO: add return values

--*/
;

VOID
QNCSmmClearSource (
  CONST QNC_SMM_SOURCE_DESC *SrcDesc
  )
/*++

Routine Description:

  GC_TODO: Add function description

Arguments:

  SrcDesc - GC_TODO: add argument description

Returns:

  GC_TODO: add return values

--*/
;

VOID
QNCSmmClearSourceAndBlock (
  CONST QNC_SMM_SOURCE_DESC *SrcDesc
  )
/*++

Routine Description:

  GC_TODO: Add function description

Arguments:

  SrcDesc - GC_TODO: add argument description

Returns:

  GC_TODO: add return values

--*/
;

#endif
