/** @file
    Implementation of collecting the statistics on a network interface.

Copyright (c) 2004 - 2010, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials are licensed
and made available under the terms and conditions of the BSD License which
accompanies this distribution. The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/


#include "Snp.h"


/**
  Resets or collects the statistics on a network interface.

  This function resets or collects the statistics on a network interface. If the
  size of the statistics table specified by StatisticsSize is not big enough for
  all the statistics that are collected by the network interface, then a partial
  buffer of statistics is returned in StatisticsTable, StatisticsSize is set to
  the size required to collect all the available statistics, and
  EFI_BUFFER_TOO_SMALL is returned.
  If StatisticsSize is big enough for all the statistics, then StatisticsTable
  will be filled, StatisticsSize will be set to the size of the returned
  StatisticsTable structure, and EFI_SUCCESS is returned.
  If the driver has not been initialized, EFI_DEVICE_ERROR will be returned.
  If Reset is FALSE, and both StatisticsSize and StatisticsTable are NULL, then
  no operations will be performed, and EFI_SUCCESS will be returned.
  If Reset is TRUE, then all of the supported statistics counters on this network
  interface will be reset to zero.

  @param This            A pointer to the EFI_SIMPLE_NETWORK_PROTOCOL instance.
  @param Reset           Set to TRUE to reset the statistics for the network interface.
  @param StatisticsSize  On input the size, in bytes, of StatisticsTable. On output
                         the size, in bytes, of the resulting table of statistics.
  @param StatisticsTable A pointer to the EFI_NETWORK_STATISTICS structure that
                         contains the statistics. Type EFI_NETWORK_STATISTICS is
                         defined in "Related Definitions" below.

  @retval EFI_SUCCESS           The requested operation succeeded.
  @retval EFI_NOT_STARTED       The Simple Network Protocol interface has not been
                                started by calling Start().
  @retval EFI_BUFFER_TOO_SMALL  StatisticsSize is not NULL and StatisticsTable is
                                NULL. The current buffer size that is needed to
                                hold all the statistics is returned in StatisticsSize.
  @retval EFI_BUFFER_TOO_SMALL  StatisticsSize is not NULL and StatisticsTable is
                                not NULL. The current buffer size that is needed
                                to hold all the statistics is returned in
                                StatisticsSize. A partial set of statistics is
                                returned in StatisticsTable.
  @retval EFI_INVALID_PARAMETER StatisticsSize is NULL and StatisticsTable is not
                                NULL.
  @retval EFI_DEVICE_ERROR      The Simple Network Protocol interface has not
                                been initialized by calling Initialize().
  @retval EFI_DEVICE_ERROR      An error was encountered collecting statistics
                                from the NIC.
  @retval EFI_UNSUPPORTED       The NIC does not support collecting statistics
                                from the network interface.

**/
EFI_STATUS
EFIAPI
SnpUndi32Statistics (
  IN EFI_SIMPLE_NETWORK_PROTOCOL *This,
  IN BOOLEAN                     Reset,
  IN OUT UINTN                   *StatisticsSize, OPTIONAL
  IN OUT EFI_NETWORK_STATISTICS  *StatisticsTable OPTIONAL
  )
{
  SNP_DRIVER        *Snp;
  PXE_DB_STATISTICS *Db;
  UINT64            *Stp;
  UINT64            Mask;
  UINTN             Size;
  UINTN             Index;
  EFI_TPL           OldTpl;
  EFI_STATUS        Status;

  //
  // Get pointer to SNP driver instance for *This.
  //
  if (This == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Snp = EFI_SIMPLE_NETWORK_DEV_FROM_THIS (This);

  OldTpl = gBS->RaiseTPL (TPL_CALLBACK);

  //
  // Return error if the SNP is not initialized.
  //
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
  //
  // if we are not resetting the counters, we have to have a valid stat table
  // with >0 size. if no reset, no table and no size, return success.
  //
  if (!Reset && StatisticsSize == NULL) {
    Status = (StatisticsTable != NULL) ? EFI_INVALID_PARAMETER : EFI_SUCCESS;
    goto ON_EXIT;
  }
  //
  // Initialize UNDI Statistics CDB
  //
  Snp->Cdb.OpCode     = PXE_OPCODE_STATISTICS;
  Snp->Cdb.CPBsize    = PXE_CPBSIZE_NOT_USED;
  Snp->Cdb.CPBaddr    = PXE_CPBADDR_NOT_USED;
  Snp->Cdb.StatCode   = PXE_STATCODE_INITIALIZE;
  Snp->Cdb.StatFlags  = PXE_STATFLAGS_INITIALIZE;
  Snp->Cdb.IFnum      = Snp->IfNum;
  Snp->Cdb.Control    = PXE_CONTROL_LAST_CDB_IN_LIST;

  if (Reset) {
    Snp->Cdb.OpFlags  = PXE_OPFLAGS_STATISTICS_RESET;
    Snp->Cdb.DBsize   = PXE_DBSIZE_NOT_USED;
    Snp->Cdb.DBaddr   = PXE_DBADDR_NOT_USED;
    Db                = Snp->Db;
  } else {
    Snp->Cdb.OpFlags                = PXE_OPFLAGS_STATISTICS_READ;
    Snp->Cdb.DBsize                 = (UINT16) sizeof (PXE_DB_STATISTICS);
    Snp->Cdb.DBaddr                 = (UINT64)(UINTN) (Db = Snp->Db);
  }
  //
  // Issue UNDI command and check result.
  //
  DEBUG ((EFI_D_NET, "\nsnp->undi.statistics()  "));

  (*Snp->IssueUndi32Command) ((UINT64)(UINTN) &Snp->Cdb);

  switch (Snp->Cdb.StatCode) {
  case PXE_STATCODE_SUCCESS:
    break;

  case PXE_STATCODE_UNSUPPORTED:
    DEBUG (
      (EFI_D_ERROR,
      "\nsnp->undi.statistics()  %xh:%xh\n",
      Snp->Cdb.StatFlags,
      Snp->Cdb.StatCode)
      );

    Status = EFI_UNSUPPORTED;
    goto ON_EXIT;

  default:
    DEBUG (
      (EFI_D_ERROR,
      "\nsnp->undi.statistics()  %xh:%xh\n",
      Snp->Cdb.StatFlags,
      Snp->Cdb.StatCode)
      );

    Status = EFI_DEVICE_ERROR;
    goto ON_EXIT;
  }

  if (Reset) {
    Status = EFI_SUCCESS;
    goto ON_EXIT;
  }

  if (StatisticsTable == NULL) {
    *StatisticsSize = sizeof (EFI_NETWORK_STATISTICS);
    Status = EFI_BUFFER_TOO_SMALL;
    goto ON_EXIT;
  }
  //
  // Convert the UNDI statistics information to SNP statistics
  // information.
  //
  ZeroMem (StatisticsTable, *StatisticsSize);
  Stp   = (UINT64 *) StatisticsTable;
  Size  = 0;

  for (Index = 0, Mask = 1; Index < 64; Index++, Mask = LShiftU64 (Mask, 1), Stp++) {
    //
    // There must be room for a full UINT64.  Partial
    // numbers will not be stored.
    //
    if ((Index + 1) * sizeof (UINT64) > *StatisticsSize) {
      break;
    }

    if ((Db->Supported & Mask) != 0) {
      *Stp  = Db->Data[Index];
      Size  = Index + 1;
    } else {
      SetMem (Stp, sizeof (UINT64), 0xFF);
    }
  }
  //
  // Compute size up to last supported statistic.
  //
  while (++Index < 64) {
    if ((Db->Supported & (Mask = LShiftU64 (Mask, 1))) != 0) {
      Size = Index;
    }
  }

  Size *= sizeof (UINT64);

  if (*StatisticsSize >= Size) {
    *StatisticsSize = Size;
    Status = EFI_SUCCESS;
  } else {
    *StatisticsSize = Size;
    Status = EFI_BUFFER_TOO_SMALL;
  }

ON_EXIT:
  gBS->RestoreTPL (OldTpl);

  return Status;
}
