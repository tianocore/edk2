/** @file
Copyright (c) 2004 - 2007, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module name:
  nvdata.c

Abstract:

Revision history:
  2000-Feb-03 M(f)J   Genesis.

**/

#include "Snp.h"


/**
  This routine calls Undi to read the desired number of eeprom bytes.

  @param  snp          pointer to the snp driver structure
  @param  RegOffset    eeprom register value relative to the base address
  @param  NumBytes     number of bytes to read
  @param  BufferPtr    pointer where to read into


**/
EFI_STATUS
pxe_nvdata_read (
  IN SNP_DRIVER *snp,
  IN UINTN      RegOffset,
  IN UINTN      NumBytes,
  IN OUT VOID   *BufferPtr
  )
{
  PXE_DB_NVDATA *db;

  db                  = snp->db;
  snp->cdb.OpCode     = PXE_OPCODE_NVDATA;

  snp->cdb.OpFlags    = PXE_OPFLAGS_NVDATA_READ;

  snp->cdb.CPBsize    = PXE_CPBSIZE_NOT_USED;
  snp->cdb.CPBaddr    = PXE_CPBADDR_NOT_USED;

  snp->cdb.DBsize     = sizeof (PXE_DB_NVDATA);
  snp->cdb.DBaddr     = (UINT64)(UINTN) db;

  snp->cdb.StatCode   = PXE_STATCODE_INITIALIZE;
  snp->cdb.StatFlags  = PXE_STATFLAGS_INITIALIZE;
  snp->cdb.IFnum      = snp->if_num;
  snp->cdb.Control    = PXE_CONTROL_LAST_CDB_IN_LIST;

  //
  // Issue UNDI command and check result.
  //
  DEBUG ((EFI_D_NET, "\nsnp->undi.nvdata ()  "));

  (*snp->issue_undi32_command) ((UINT64)(UINTN) &snp->cdb);

  switch (snp->cdb.StatCode) {
  case PXE_STATCODE_SUCCESS:
    break;

  case PXE_STATCODE_UNSUPPORTED:
    DEBUG (
      (EFI_D_NET,
      "\nsnp->undi.nvdata()  %xh:%xh\n",
      snp->cdb.StatFlags,
      snp->cdb.StatCode)
      );

    return EFI_UNSUPPORTED;

  default:
    DEBUG (
      (EFI_D_NET,
      "\nsnp->undi.nvdata()  %xh:%xh\n",
      snp->cdb.StatFlags,
      snp->cdb.StatCode)
      );

    return EFI_DEVICE_ERROR;
  }

  CopyMem (BufferPtr, db->Data.Byte + RegOffset, NumBytes);

  return EFI_SUCCESS;
}


/**
  This is an interface call provided by SNP.
  It does the basic checking on the input parameters and retrieves snp structure
  and then calls the read_nvdata() call which does the actual reading

  @param  this         context pointer
  @param  ReadOrWrite  true for reading and false for writing
  @param  RegOffset    eeprom register relative to the base
  @param  NumBytes     how many bytes to read
  @param  BufferPtr    address of memory to read into


**/
EFI_STATUS
EFIAPI
snp_undi32_nvdata (
  IN EFI_SIMPLE_NETWORK_PROTOCOL *this,
  IN BOOLEAN                     ReadOrWrite,
  IN UINTN                       RegOffset,
  IN UINTN                       NumBytes,
  IN OUT VOID                    *BufferPtr
  )
{
  SNP_DRIVER  *snp;
  EFI_TPL     OldTpl;
  EFI_STATUS  Status;

  //
  // Get pointer to SNP driver instance for *this.
  //
  if (this == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  snp = EFI_SIMPLE_NETWORK_DEV_FROM_THIS (this);

  OldTpl = gBS->RaiseTPL (TPL_CALLBACK);

  //
  // Return error if the SNP is not initialized.
  //
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
  //
  // Return error if non-volatile memory variables are not valid.
  //
  if (snp->mode.NvRamSize == 0 || snp->mode.NvRamAccessSize == 0) {
    Status = EFI_UNSUPPORTED;
    goto ON_EXIT;
  }
  //
  // Check for invalid parameter combinations.
  //
  if ((NumBytes == 0) ||
      (BufferPtr == NULL) ||
      (RegOffset >= snp->mode.NvRamSize) ||
      (RegOffset + NumBytes > snp->mode.NvRamSize) ||
      (NumBytes % snp->mode.NvRamAccessSize != 0) ||
      (RegOffset % snp->mode.NvRamAccessSize != 0)
      ) {
    Status = EFI_INVALID_PARAMETER;
    goto ON_EXIT;
  }
  //
  // check the implementation flags of undi if we can write the nvdata!
  //
  if (!ReadOrWrite) {
    Status = EFI_UNSUPPORTED;
  } else {
    Status = pxe_nvdata_read (snp, RegOffset, NumBytes, BufferPtr);
  }

ON_EXIT:
  gBS->RestoreTPL (OldTpl);

  return Status;
}
