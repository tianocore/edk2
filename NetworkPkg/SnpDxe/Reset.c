/** @file
    Implementation of resetting a network adapter.

Copyright (c) 2004 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "Snp.h"


/**
  Call UNDI to reset the NIC.

  @param  Snp                 Pointer to the snp driver structure.

  @return EFI_SUCCESSFUL      The NIC was reset.
  @retval EFI_DEVICE_ERROR    The NIC cannot be reset.

**/
EFI_STATUS
PxeReset (
  SNP_DRIVER *Snp
  )
{
  Snp->Cdb.OpCode     = PXE_OPCODE_RESET;
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
  // Issue UNDI command and check result.
  //
  DEBUG ((EFI_D_NET, "\nsnp->undi.reset()  "));

  (*Snp->IssueUndi32Command) ((UINT64)(UINTN) &Snp->Cdb);

  if (Snp->Cdb.StatCode != PXE_STATCODE_SUCCESS) {
    DEBUG (
      (EFI_D_WARN,
      "\nsnp->undi32.reset()  %xh:%xh\n",
      Snp->Cdb.StatFlags,
      Snp->Cdb.StatCode)
      );

    //
    // UNDI could not be reset. Return UNDI error.
    //
    return EFI_DEVICE_ERROR;
  }

  return EFI_SUCCESS;
}


/**
  Resets a network adapter and reinitializes it with the parameters that were
  provided in the previous call to Initialize().

  This function resets a network adapter and reinitializes it with the parameters
  that were provided in the previous call to Initialize(). The transmit and
  receive queues are emptied and all pending interrupts are cleared.
  Receive filters, the station address, the statistics, and the multicast-IP-to-HW
  MAC addresses are not reset by this call. If the network interface was
  successfully reset, then EFI_SUCCESS will be returned. If the driver has not
  been initialized, EFI_DEVICE_ERROR will be returned.

  @param This                 A pointer to the EFI_SIMPLE_NETWORK_PROTOCOL instance.
  @param ExtendedVerification Indicates that the driver may perform a more
                              exhaustive verification operation of the device
                              during reset.

  @retval EFI_SUCCESS           The network interface was reset.
  @retval EFI_NOT_STARTED       The network interface has not been started.
  @retval EFI_INVALID_PARAMETER One or more of the parameters has an unsupported value.
  @retval EFI_DEVICE_ERROR      The command could not be sent to the network interface.
  @retval EFI_UNSUPPORTED       This function is not supported by the network interface.

**/
EFI_STATUS
EFIAPI
SnpUndi32Reset (
  IN EFI_SIMPLE_NETWORK_PROTOCOL *This,
  IN BOOLEAN                     ExtendedVerification
  )
{
  SNP_DRIVER  *Snp;
  EFI_TPL     OldTpl;
  EFI_STATUS  Status;

  //
  // Resolve Warning 4 unreferenced parameter problem
  //
  ExtendedVerification = 0;
  DEBUG ((EFI_D_WARN, "ExtendedVerification = %d is not implemented!\n", ExtendedVerification));

  if (This == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Snp = EFI_SIMPLE_NETWORK_DEV_FROM_THIS (This);

  OldTpl = gBS->RaiseTPL (TPL_CALLBACK);

  switch (Snp->Mode.State) {
  case EfiSimpleNetworkInitialized:
    break;

  case EfiSimpleNetworkStopped:
    Status = EFI_NOT_STARTED;
    goto ON_EXIT;

  default:
    Status = EFI_DEVICE_ERROR;
    goto ON_EXIT;
  }

  Status = PxeReset (Snp);

ON_EXIT:
  gBS->RestoreTPL (OldTpl);

  return Status;
}
