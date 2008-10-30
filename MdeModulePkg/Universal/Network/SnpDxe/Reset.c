/** @file
Copyright (c) 2004 - 2007, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module name:
  reset.c

Abstract:

Revision history:
  2000-Feb-09 M(f)J   Genesis.

**/

#include "Snp.h"


/**
  This routine calls undi to reset the nic.

  @param  snp                   pointer to the snp driver structure

  @return EFI_SUCCESSFUL for a successful completion
  @return other for failed calls

**/
EFI_STATUS
pxe_reset (
  SNP_DRIVER *snp
  )
{
  snp->cdb.OpCode     = PXE_OPCODE_RESET;
  snp->cdb.OpFlags    = PXE_OPFLAGS_NOT_USED;
  snp->cdb.CPBsize    = PXE_CPBSIZE_NOT_USED;
  snp->cdb.DBsize     = PXE_DBSIZE_NOT_USED;
  snp->cdb.CPBaddr    = PXE_CPBADDR_NOT_USED;
  snp->cdb.DBaddr     = PXE_DBADDR_NOT_USED;
  snp->cdb.StatCode   = PXE_STATCODE_INITIALIZE;
  snp->cdb.StatFlags  = PXE_STATFLAGS_INITIALIZE;
  snp->cdb.IFnum      = snp->if_num;
  snp->cdb.Control    = PXE_CONTROL_LAST_CDB_IN_LIST;

  //
  // Issue UNDI command and check result.
  //
  DEBUG ((EFI_D_NET, "\nsnp->undi.reset()  "));

  (*snp->issue_undi32_command) ((UINT64)(UINTN) &snp->cdb);

  if (snp->cdb.StatCode != PXE_STATCODE_SUCCESS) {
    DEBUG (
      (EFI_D_WARN,
      "\nsnp->undi32.reset()  %xh:%xh\n",
      snp->cdb.StatFlags,
      snp->cdb.StatCode)
      );

    //
    // UNDI could not be reset. Return UNDI error.
    //
    return EFI_DEVICE_ERROR;
  }

  return EFI_SUCCESS;
}


/**
  This is the SNP interface routine for resetting the NIC
  This routine basically retrieves snp structure, checks the SNP state and
  calls the pxe_reset routine to actually do the reset!

  @param  this                  context pointer
  @param  ExtendedVerification  not implemented


**/
EFI_STATUS
EFIAPI
snp_undi32_reset (
  IN EFI_SIMPLE_NETWORK_PROTOCOL *this,
  IN BOOLEAN                     ExtendedVerification
  )
{
  SNP_DRIVER  *snp;
  EFI_TPL     OldTpl;
  EFI_STATUS  Status;

  //
  // Resolve Warning 4 unreferenced parameter problem
  //
  ExtendedVerification = 0;
  DEBUG ((EFI_D_WARN, "ExtendedVerification = %d is not implemented!\n", ExtendedVerification));

  if (this == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  snp = EFI_SIMPLE_NETWORK_DEV_FROM_THIS (this);

  OldTpl = gBS->RaiseTPL (TPL_CALLBACK);

  switch (snp->mode.State) {
  case EfiSimpleNetworkInitialized:
    break;

  case EfiSimpleNetworkStopped:
    Status = EFI_NOT_STARTED;
    goto ON_EXIT;

  default:
    Status = EFI_DEVICE_ERROR;
    goto ON_EXIT;
  }

  Status = pxe_reset (snp);

ON_EXIT:
  gBS->RestoreTPL (OldTpl);

  return Status;
}
