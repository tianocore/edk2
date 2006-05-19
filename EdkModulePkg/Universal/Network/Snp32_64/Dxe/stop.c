/*++
Copyright (c) 2006, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module name:
  stop.c

Abstract:

Revision history:
  2000-Feb-09 M(f)J   Genesis.
--*/


#include "Snp.h"

EFI_STATUS
pxe_stop (
  SNP_DRIVER *snp
  )
/*++

Routine Description:
 this routine calls undi to stop the interface and changes the snp state

Arguments:
  snp  - pointer to snp driver structure

Returns:

--*/
{
  snp->cdb.OpCode     = PXE_OPCODE_STOP;
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
  // Issue UNDI command
  //
  DEBUG ((EFI_D_NET, "\nsnp->undi.stop()  "));

  (*snp->issue_undi32_command) ((UINT64) (UINTN) &snp->cdb);

  if (snp->cdb.StatCode != PXE_STATCODE_SUCCESS) {
    DEBUG (
      (EFI_D_WARN,
      "\nsnp->undi.stop()  %xh:%xh\n",
      snp->cdb.StatCode,
      snp->cdb.StatFlags)
      );

    return EFI_DEVICE_ERROR;
  }
  //
  // Set simple network state to Started and return success.
  //
  snp->mode.State = EfiSimpleNetworkStopped;
  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
snp_undi32_stop (
  IN EFI_SIMPLE_NETWORK_PROTOCOL *this
  )
/*++

Routine Description:
 This is the SNP interface routine for stopping the interface.
 This routine basically retrieves snp structure, checks the SNP state and
 calls the pxe_stop routine to actually stop the undi interface

Arguments:
  this  - context pointer

Returns:

--*/
{
  SNP_DRIVER  *snp;

  if (this == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  snp = EFI_SIMPLE_NETWORK_DEV_FROM_THIS (this);

  if (snp == NULL) {
    return EFI_DEVICE_ERROR;
  }

  switch (snp->mode.State) {
  case EfiSimpleNetworkStarted:
    break;

  case EfiSimpleNetworkStopped:
    return EFI_NOT_STARTED;

  case EfiSimpleNetworkInitialized:
    return EFI_DEVICE_ERROR;

  default:
    return EFI_DEVICE_ERROR;
  }

  return pxe_stop (snp);
}
