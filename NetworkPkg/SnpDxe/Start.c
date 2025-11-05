/** @file
    Implementation of starting a network adapter.

Copyright (c) 2004 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "Snp.h"

/**
  Call UNDI to start the interface and changes the snp state.

  @param  Snp                    pointer to snp driver structure.

  @retval EFI_SUCCESS            UNDI is started successfully.
  @retval EFI_DEVICE_ERROR       UNDI could not be started.

**/
EFI_STATUS
PxeStart (
  IN SNP_DRIVER  *Snp
  )
{
  PXE_CPB_START_31  *Cpb31;

  if (Snp->Cdb == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: Snp->Cdb is NULL\n", __func__));
    return EFI_DEVICE_ERROR;
  }

  Cpb31 = Snp->Cpb;
  //
  // Initialize UNDI Start CDB for H/W UNDI
  //
  Snp->Cdb->OpCode    = PXE_OPCODE_START;
  Snp->Cdb->OpFlags   = PXE_OPFLAGS_NOT_USED;
  Snp->Cdb->CPBsize   = PXE_CPBSIZE_NOT_USED;
  Snp->Cdb->DBsize    = PXE_DBSIZE_NOT_USED;
  Snp->Cdb->CPBaddr   = PXE_CPBADDR_NOT_USED;
  Snp->Cdb->DBaddr    = PXE_DBADDR_NOT_USED;
  Snp->Cdb->StatCode  = PXE_STATCODE_INITIALIZE;
  Snp->Cdb->StatFlags = PXE_STATFLAGS_INITIALIZE;
  Snp->Cdb->IFnum     = Snp->IfNum;
  Snp->Cdb->Control   = PXE_CONTROL_LAST_CDB_IN_LIST;

  //
  // Make changes to H/W UNDI Start CDB if this is
  // a S/W UNDI.
  //
  if (Snp->IsSwUndi) {
    Snp->Cdb->CPBsize = (UINT16)sizeof (PXE_CPB_START_31);
    Snp->Cdb->CPBaddr = (UINT64)(UINTN)Cpb31;

    Cpb31->Delay = (UINT64)(UINTN)&SnpUndi32CallbackDelay;
    Cpb31->Block = (UINT64)(UINTN)&SnpUndi32CallbackBlock;

    //
    // Virtual == Physical.  This can be set to zero.
    //
    Cpb31->Virt2Phys = (UINT64)(UINTN)0;
    Cpb31->Mem_IO    = (UINT64)(UINTN)&SnpUndi32CallbackMemio;

    Cpb31->Map_Mem   = (UINT64)(UINTN)&SnpUndi32CallbackMap;
    Cpb31->UnMap_Mem = (UINT64)(UINTN)&SnpUndi32CallbackUnmap;
    Cpb31->Sync_Mem  = (UINT64)(UINTN)&SnpUndi32CallbackSync;

    Cpb31->Unique_ID = (UINT64)(UINTN)Snp;
  }

  //
  // Issue UNDI command and check result.
  //
  DEBUG ((DEBUG_NET, "\nsnp->undi.start()  "));

  (*Snp->IssueUndi32Command)((UINT64)(UINTN)Snp->Cdb);

  if (Snp->Cdb->StatCode != PXE_STATCODE_SUCCESS) {
    //
    // UNDI could not be started. Return UNDI error.
    //
    DEBUG (
      (DEBUG_ERROR,
       "\nsnp->undi.start()  %xh:%xh\n",
       Snp->Cdb->StatCode,
       Snp->Cdb->StatFlags)
      );

    return EFI_DEVICE_ERROR;
  }

  //
  // Set simple network state to Started and return success.
  //
  Snp->Mode.State = EfiSimpleNetworkStarted;

  return EFI_SUCCESS;
}

/**
  Change the state of a network interface from "stopped" to "started."

  This function starts a network interface. If the network interface successfully
  starts, then EFI_SUCCESS will be returned.

  @param  This                   A pointer to the EFI_SIMPLE_NETWORK_PROTOCOL instance.

  @retval EFI_SUCCESS            The network interface was started.
  @retval EFI_ALREADY_STARTED    The network interface is already in the started state.
  @retval EFI_INVALID_PARAMETER  This parameter was NULL or did not point to a valid
                                 EFI_SIMPLE_NETWORK_PROTOCOL structure.
  @retval EFI_DEVICE_ERROR       The command could not be sent to the network interface.
  @retval EFI_UNSUPPORTED        This function is not supported by the network interface.

**/
EFI_STATUS
EFIAPI
SnpUndi32Start (
  IN EFI_SIMPLE_NETWORK_PROTOCOL  *This
  )
{
  SNP_DRIVER  *Snp;
  EFI_STATUS  Status;
  UINTN       Index;
  EFI_TPL     OldTpl;

  if (This == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Snp = EFI_SIMPLE_NETWORK_DEV_FROM_THIS (This);

  OldTpl = gBS->RaiseTPL (TPL_CALLBACK);

  switch (Snp->Mode.State) {
    case EfiSimpleNetworkStopped:
      break;

    case EfiSimpleNetworkStarted:
    case EfiSimpleNetworkInitialized:
      Status = EFI_ALREADY_STARTED;
      goto ON_EXIT;

    default:
      Status = EFI_DEVICE_ERROR;
      goto ON_EXIT;
  }

  Status = PxeStart (Snp);
  if (EFI_ERROR (Status)) {
    goto ON_EXIT;
  }

  //
  // clear the map_list in SNP structure
  //
  for (Index = 0; Index < MAX_MAP_LENGTH; Index++) {
    Snp->MapList[Index].VirtualAddress = 0;
    Snp->MapList[Index].MapCookie      = 0;
  }

  Snp->Mode.MCastFilterCount = 0;

ON_EXIT:
  gBS->RestoreTPL (OldTpl);

  return Status;
}
