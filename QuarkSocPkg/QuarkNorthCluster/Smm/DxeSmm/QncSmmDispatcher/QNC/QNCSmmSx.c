/** @file
File to contain all the hardware specific stuff for the Smm Sx dispatch protocol.

Copyright (c) 2013-2015 Intel Corporation.

SPDX-License-Identifier: BSD-2-Clause-Patent


**/

//
// Include common header file for this module.
//
#include "CommonHeader.h"

#include "QNCSmmHelpers.h"

CONST QNC_SMM_SOURCE_DESC SX_SOURCE_DESC = {
  QNC_SMM_NO_FLAGS,
  {
    {
      {GPE_ADDR_TYPE, {R_QNC_GPE0BLK_SMIE}}, S_QNC_GPE0BLK_SMIE, N_QNC_GPE0BLK_SMIE_SLP
    },
    NULL_BIT_DESC_INITIALIZER
  },
  {
    {
      {GPE_ADDR_TYPE, {R_QNC_GPE0BLK_SMIS}}, S_QNC_GPE0BLK_SMIS, N_QNC_GPE0BLK_SMIS_SLP
    }
  }
};

VOID
SxGetContext(
  IN  DATABASE_RECORD    *Record,
  OUT QNC_SMM_CONTEXT    *Context
  )
{
  UINT32        Pm1Cnt;

  Pm1Cnt = IoRead32 (PcdGet16 (PcdPm1blkIoBaseAddress) + R_QNC_PM1BLK_PM1C);

  //
  // By design, the context phase will always be ENTRY
  //
  Context->Sx.Phase = SxEntry;

  //
  // Map the PM1_CNT register's SLP_TYP bits to the context type
  //
  switch (Pm1Cnt & B_QNC_PM1BLK_PM1C_SLPTP) {

  case V_S0:
    Context->Sx.Type = SxS0;
    break;

  case V_S3:
    Context->Sx.Type = SxS3;
    break;

  case V_S4:
    Context->Sx.Type = SxS4;
    break;

  case V_S5:
    Context->Sx.Type = SxS5;
    break;

  default:
    ASSERT (FALSE);
    break;
  };
}

BOOLEAN
SxCmpContext (
  IN QNC_SMM_CONTEXT     *Context1,
  IN QNC_SMM_CONTEXT     *Context2
  )
{
  return (BOOLEAN)(Context1->Sx.Type == Context2->Sx.Type);
}

VOID
QNCSmmSxGoToSleep(
  VOID
  )
/*++

Routine Description:

  When we get an SMI that indicates that we are transitioning to a sleep state,
  we need to actually transition to that state.  We do this by disabling the
  "SMI on sleep enable" feature, which generates an SMI when the operating system
  tries to put the system to sleep, and then physically putting the system to sleep.

Returns:

  None.

--*/
{
  UINT32        Pm1Cnt;

  //
  // Flush cache into memory before we go to sleep. It is necessary for S3 sleep
  // because we may update memory in SMM Sx sleep handlers -- the updates are in cache now
  //
  AsmWbinvd();

  //
  // Disable SMIs
  //
  QNCSmmClearSource (&SX_SOURCE_DESC );
  QNCSmmDisableSource (&SX_SOURCE_DESC);

  //
  // Clear Sleep Type Enable
  //
  IoAnd16 ((UINT16)(LpcPciCfg32 (R_QNC_LPC_GPE0BLK) & 0xFFFF) + R_QNC_GPE0BLK_SMIE, (UINT16)(~B_QNC_GPE0BLK_SMIE_SLP));

  // clear sleep SMI status
  IoAnd16 ((UINT16)(LpcPciCfg32 (R_QNC_LPC_GPE0BLK) & 0xFFFF) + R_QNC_GPE0BLK_SMIS, (UINT16)(S_QNC_GPE0BLK_SMIS));

  //
  // Now that SMIs are disabled, write to the SLP_EN bit again to trigger the sleep
  //
  Pm1Cnt = IoOr32 (PcdGet16 (PcdPm1blkIoBaseAddress) + R_QNC_PM1BLK_PM1C, B_QNC_PM1BLK_PM1C_SLPEN);

  //
  // The system just went to sleep. If the sleep state was S1, then code execution will resume
  // here when the system wakes up.
  //
  Pm1Cnt = IoRead32 (PcdGet16 (PcdPm1blkIoBaseAddress) + R_QNC_PM1BLK_PM1C);
  if ((Pm1Cnt & B_QNC_PM1BLK_PM1C_SCIEN) == 0) {
    //
    // An ACPI OS isn't present, clear the sleep information
    //
    Pm1Cnt &= ~B_QNC_PM1BLK_PM1C_SLPTP;
    Pm1Cnt |= V_S0;

    IoWrite32 (PcdGet16 (PcdPm1blkIoBaseAddress) + R_QNC_PM1BLK_PM1C, Pm1Cnt);
  }

  QNCSmmClearSource (&SX_SOURCE_DESC);
  QNCSmmEnableSource (&SX_SOURCE_DESC);
}
