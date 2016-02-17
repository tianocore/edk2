/** @file
    Implementation of stopping a network interface.
 
Copyright (c) 2004 - 2016, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials are licensed 
and made available under the terms and conditions of the BSD License which 
accompanies this distribution. The full text of the license may be found at 
http://opensource.org/licenses/bsd-license.php 

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "Snp.h"


/**
  Call UNDI to stop the interface and changes the snp state.

  @param  Snp   Pointer to snp driver structure
   
  @retval EFI_SUCCESS            The network interface was stopped.
  @retval EFI_DEVICE_ERROR       SNP is not initialized.

**/
EFI_STATUS
PxeStop (
  SNP_DRIVER *Snp
  )
{
  Snp->Cdb.OpCode     = PXE_OPCODE_STOP;
  Snp->Cdb.OpFlags    = PXE_OPFLAGS_NOT_USED;
  Snp->Cdb.CPBsize    = PXE_CPBSIZE_NOT_USED;
  Snp->Cdb.DBsize     = PXE_DBSIZE_NOT_USED;
  Snp->Cdb.CPBaddr    = PXE_CPBADDR_NOT_USED;
  Snp->Cdb.DBaddr     = PXE_DBADDR_NOT_USED;
  Snp->Cdb.StatCode   = PXE_STATCODE_INITIALIZE;
  Snp->Cdb.StatFlags  = PXE_STATFLAGS_INITIALIZE;
  Snp->Cdb.IFnum      = Snp->IfNum;
  Snp->Cdb.Control    = PXE_CONTROL_LAST_CDB_IN_LIST;

  //
  // Issue UNDI command
  //
  DEBUG ((EFI_D_NET, "\nsnp->undi.stop()  "));

  (*Snp->IssueUndi32Command) ((UINT64)(UINTN) &Snp->Cdb);

  if (Snp->Cdb.StatCode != PXE_STATCODE_SUCCESS) {
    DEBUG (
      (EFI_D_WARN,
      "\nsnp->undi.stop()  %xh:%xh\n",
      Snp->Cdb.StatFlags,
      Snp->Cdb.StatCode)
      );

    return EFI_DEVICE_ERROR;
  }
  //
  // Set simple network state to Started and return success.
  //
  Snp->Mode.State = EfiSimpleNetworkStopped;
  return EFI_SUCCESS;
}


/**
  Changes the state of a network interface from "started" to "stopped."
  
  This function stops a network interface. This call is only valid if the network
  interface is in the started state. If the network interface was successfully
  stopped, then EFI_SUCCESS will be returned.
  
  @param  This                    A pointer to the EFI_SIMPLE_NETWORK_PROTOCOL 
                                  instance.
  
  
  @retval EFI_SUCCESS             The network interface was stopped.
  @retval EFI_NOT_STARTED         The network interface has not been started.
  @retval EFI_INVALID_PARAMETER   This parameter was NULL or did not point to a 
                                  valid EFI_SIMPLE_NETWORK_PROTOCOL structure.
  @retval EFI_DEVICE_ERROR        The command could not be sent to the network 
                                  interface.
  @retval EFI_UNSUPPORTED         This function is not supported by the network 
                                  interface.

**/
EFI_STATUS
EFIAPI
SnpUndi32Stop (
  IN EFI_SIMPLE_NETWORK_PROTOCOL *This
  )
{
  SNP_DRIVER  *Snp;
  EFI_TPL     OldTpl;
  EFI_STATUS  Status;

  if (This == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Snp = EFI_SIMPLE_NETWORK_DEV_FROM_THIS (This);

  OldTpl = gBS->RaiseTPL (TPL_CALLBACK);

  switch (Snp->Mode.State) {
  case EfiSimpleNetworkStarted:
    break;

  case EfiSimpleNetworkStopped:
    Status = EFI_NOT_STARTED;
    goto ON_EXIT;

  default:
    Status = EFI_DEVICE_ERROR;
    goto ON_EXIT;
  }

  Status = PxeStop (Snp);

ON_EXIT:
  gBS->RestoreTPL (OldTpl);

  return Status;
}
