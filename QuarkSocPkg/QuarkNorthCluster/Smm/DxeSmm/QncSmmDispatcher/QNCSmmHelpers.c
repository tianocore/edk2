/** @file

Copyright (c) 2013-2015 Intel Corporation.

SPDX-License-Identifier: BSD-2-Clause-Patent


**/

//
// Include common header file for this module.
//
#include "CommonHeader.h"

#include "QNCSmm.h"
#include "QNCSmmHelpers.h"

//
// #define BIT_ZERO 0x00000001
//
CONST UINT32  BIT_ZERO = 0x00000001;

//
// /////////////////////////////////////////////////////////////////////////////
// SUPPORT / HELPER FUNCTIONS (QNC version-independent)
//
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
{
  BOOLEAN IsEqual;
  UINTN   loopvar;

  IsEqual = TRUE;
  for (loopvar = 0; loopvar < NUM_EN_BITS; loopvar++) {
    //
    // It's okay to compare a NULL bit description to a non-NULL bit description.
    // They are unequal and these tests will generate the correct result.
    //
    if (Src1->En[loopvar].Bit != Src2->En[loopvar].Bit ||
        Src1->En[loopvar].Reg.Type != Src2->En[loopvar].Reg.Type ||
        Src1->En[loopvar].Reg.Data.raw != Src2->En[loopvar].Reg.Data.raw
        ) {
      IsEqual = FALSE;
      break;
      //
      // out of for loop
      //
    }
  }

  return IsEqual;
}

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
{
  BOOLEAN IsEqual;
  UINTN   loopvar;

  IsEqual = TRUE;

  for (loopvar = 0; loopvar < NUM_STS_BITS; loopvar++) {
    //
    // It's okay to compare a NULL bit description to a non-NULL bit description.
    // They are unequal and these tests will generate the correct result.
    //
    if (Src1->Sts[loopvar].Bit != Src2->Sts[loopvar].Bit ||
        Src1->Sts[loopvar].Reg.Type != Src2->Sts[loopvar].Reg.Type ||
        Src1->Sts[loopvar].Reg.Data.raw != Src2->Sts[loopvar].Reg.Data.raw
        ) {
      IsEqual = FALSE;
      break;
      //
      // out of for loop
      //
    }
  }

  return IsEqual;
}

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
{
  return (BOOLEAN) (CompareEnables (Src1, Src2) && CompareStatuses (Src1, Src2));
}

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
{
  BOOLEAN IsActive;
  UINTN   loopvar;

  BOOLEAN SciEn;

  IsActive  = TRUE;

  SciEn     = QNCSmmGetSciEn ();

  if ((Src->Flags & QNC_SMM_SCI_EN_DEPENDENT) && (SciEn)) {
    //
    // This source is dependent on SciEn, and SciEn == 1.  An ACPI OS is present,
    // so we shouldn't do anything w/ this source until SciEn == 0.
    //
    IsActive = FALSE;

  } else {
    //
    // Read each bit desc from hardware and make sure it's a one
    //
    for (loopvar = 0; loopvar < NUM_EN_BITS; loopvar++) {

      if (!IS_BIT_DESC_NULL (Src->En[loopvar])) {

        if (ReadBitDesc (&Src->En[loopvar]) == 0) {
          IsActive = FALSE;
          break;
          //
          // out of for loop
          //
        }

      }
    }

    if (IsActive) {
      //
      // Read each bit desc from hardware and make sure it's a one
      //
      for (loopvar = 0; loopvar < NUM_STS_BITS; loopvar++) {

        if (!IS_BIT_DESC_NULL (Src->Sts[loopvar])) {

          if (ReadBitDesc (&Src->Sts[loopvar]) == 0) {
            IsActive = FALSE;
            break;
            //
            // out of for loop
            //
          }

        }
      }
    }
  }

  return IsActive;
}

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
{
  UINTN loopvar;

  //
  // Set enables to 1 by writing a 1
  //
  for (loopvar = 0; loopvar < NUM_EN_BITS; loopvar++) {
    if (!IS_BIT_DESC_NULL (SrcDesc->En[loopvar])) {
      WriteBitDesc (&SrcDesc->En[loopvar], 1);
    }
  }

  QNCSmmClearSource (SrcDesc);

}

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
{
  UINTN loopvar;

  for (loopvar = 0; loopvar < NUM_EN_BITS; loopvar++) {
    if (!IS_BIT_DESC_NULL (SrcDesc->En[loopvar])) {
      WriteBitDesc (&SrcDesc->En[loopvar], 0);
    }
  }
}

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
{
  UINTN loopvar;
  BOOLEAN ValueToWrite;

  ValueToWrite =
    ((SrcDesc->Flags & QNC_SMM_CLEAR_WITH_ZERO) == 0) ? TRUE : FALSE;

  for (loopvar = 0; loopvar < NUM_STS_BITS; loopvar++) {
    if (!IS_BIT_DESC_NULL (SrcDesc->Sts[loopvar])) {
      WriteBitDesc (&SrcDesc->Sts[loopvar], ValueToWrite);
    }
  }
}

VOID
QNCSmmClearSourceAndBlock (
  CONST QNC_SMM_SOURCE_DESC *SrcDesc
  )
// GC_TODO: function comment should start with '/*++'
/*
  Sets the source to a 1 or 0 and then waits for it to clear.
  Be very careful when calling this function -- it will not
  ASSERT.  An acceptable case to call the function is when
  waiting for the NEWCENTURY_STS bit to clear (which takes
  3 RTCCLKs).
*/
// GC_TODO: function comment should end with '--*/'
// GC_TODO: function comment is missing 'Routine Description:'
// GC_TODO: function comment is missing 'Arguments:'
// GC_TODO: function comment is missing 'Returns:'
// GC_TODO:    SrcDesc - add argument and description to function comment
{
  UINTN   loopvar;
  BOOLEAN IsSet;
  BOOLEAN ValueToWrite;

  ValueToWrite =
    ((SrcDesc->Flags & QNC_SMM_CLEAR_WITH_ZERO) == 0) ? TRUE : FALSE;

  for (loopvar = 0; loopvar < NUM_STS_BITS; loopvar++) {

    if (!IS_BIT_DESC_NULL (SrcDesc->Sts[loopvar])) {
      //
      // Write the bit
      //
      WriteBitDesc (&SrcDesc->Sts[loopvar], ValueToWrite);

      //
      // Don't return until the bit actually clears.
      //
      IsSet = TRUE;
      while (IsSet) {
        IsSet = ReadBitDesc (&SrcDesc->Sts[loopvar]);
        //
        // IsSet will eventually clear -- or else we'll have
        // an infinite loop.
        //
      }
    }
  }
}
