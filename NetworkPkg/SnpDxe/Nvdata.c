/** @file
   Implementation of reading and writing operations on the NVRAM device
   attached to a network interface.

Copyright (c) 2004 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "Snp.h"

/**
  This routine calls Undi to read the desired number of eeprom bytes.

  @param  Snp          pointer to the snp driver structure
  @param  Offset       eeprom register value relative to the base address
  @param  BufferSize   number of bytes to read
  @param  Buffer       pointer where to read into

  @retval EFI_SUCCESS           The NVRAM access was performed.
  @retval EFI_INVALID_PARAMETER Invalid UNDI command.
  @retval EFI_UNSUPPORTED       Command is not supported by UNDI.
  @retval EFI_DEVICE_ERROR      Fail to execute UNDI command.

**/
EFI_STATUS
PxeNvDataRead (
  IN SNP_DRIVER  *Snp,
  IN UINTN       Offset,
  IN UINTN       BufferSize,
  IN OUT VOID    *Buffer
  )
{
  PXE_DB_NVDATA  *Db;

  if (Snp->Cdb == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: Snp->Cdb is NULL\n", __func__));
    return EFI_DEVICE_ERROR;
  }

  Db               = Snp->Db;
  Snp->Cdb->OpCode = PXE_OPCODE_NVDATA;

  Snp->Cdb->OpFlags = PXE_OPFLAGS_NVDATA_READ;

  Snp->Cdb->CPBsize = PXE_CPBSIZE_NOT_USED;
  Snp->Cdb->CPBaddr = PXE_CPBADDR_NOT_USED;

  Snp->Cdb->DBsize = (UINT16)sizeof (PXE_DB_NVDATA);
  Snp->Cdb->DBaddr = (UINT64)(UINTN)Db;

  Snp->Cdb->StatCode  = PXE_STATCODE_INITIALIZE;
  Snp->Cdb->StatFlags = PXE_STATFLAGS_INITIALIZE;
  Snp->Cdb->IFnum     = Snp->IfNum;
  Snp->Cdb->Control   = PXE_CONTROL_LAST_CDB_IN_LIST;

  //
  // Issue UNDI command and check result.
  //
  DEBUG ((DEBUG_NET, "\nsnp->undi.nvdata ()  "));

  (*Snp->IssueUndi32Command)((UINT64)(UINTN)Snp->Cdb);

  switch (Snp->Cdb->StatCode) {
    case PXE_STATCODE_SUCCESS:
      break;

    case PXE_STATCODE_UNSUPPORTED:
      DEBUG (
        (DEBUG_NET,
         "\nsnp->undi.nvdata()  %xh:%xh\n",
         Snp->Cdb->StatFlags,
         Snp->Cdb->StatCode)
        );

      return EFI_UNSUPPORTED;

    default:
      DEBUG (
        (DEBUG_NET,
         "\nsnp->undi.nvdata()  %xh:%xh\n",
         Snp->Cdb->StatFlags,
         Snp->Cdb->StatCode)
        );

      return EFI_DEVICE_ERROR;
  }

  ASSERT (Offset < sizeof (Db->Data));

  CopyMem (Buffer, &Db->Data.Byte[Offset], BufferSize);

  return EFI_SUCCESS;
}

/**
  Performs read and write operations on the NVRAM device attached to a network
  interface.

  This function performs read and write operations on the NVRAM device attached
  to a network interface. If ReadWrite is TRUE, a read operation is performed.
  If ReadWrite is FALSE, a write operation is performed. Offset specifies the
  byte offset at which to start either operation. Offset must be a multiple of
  NvRamAccessSize , and it must have a value between zero and NvRamSize.
  BufferSize specifies the length of the read or write operation. BufferSize must
  also be a multiple of NvRamAccessSize, and Offset + BufferSize must not exceed
  NvRamSize.
  If any of the above conditions is not met, then EFI_INVALID_PARAMETER will be
  returned.
  If all the conditions are met and the operation is "read," the NVRAM device
  attached to the network interface will be read into Buffer and EFI_SUCCESS
  will be returned. If this is a write operation, the contents of Buffer will be
  used to update the contents of the NVRAM device attached to the network
  interface and EFI_SUCCESS will be returned.

  It does the basic checking on the input parameters and retrieves snp structure
  and then calls the read_nvdata() call which does the actual reading

  @param This       A pointer to the EFI_SIMPLE_NETWORK_PROTOCOL instance.
  @param ReadWrite  TRUE for read operations, FALSE for write operations.
  @param Offset     Byte offset in the NVRAM device at which to start the read or
                    write operation. This must be a multiple of NvRamAccessSize
                    and less than NvRamSize. (See EFI_SIMPLE_NETWORK_MODE)
  @param BufferSize The number of bytes to read or write from the NVRAM device.
                    This must also be a multiple of NvramAccessSize.
  @param Buffer     A pointer to the data buffer.

  @retval EFI_SUCCESS           The NVRAM access was performed.
  @retval EFI_NOT_STARTED       The network interface has not been started.
  @retval EFI_INVALID_PARAMETER One or more of the following conditions is TRUE:
                                * The This parameter is NULL
                                * The This parameter does not point to a valid
                                  EFI_SIMPLE_NETWORK_PROTOCOL  structure
                                * The Offset parameter is not a multiple of
                                  EFI_SIMPLE_NETWORK_MODE.NvRamAccessSize
                                * The Offset parameter is not less than
                                  EFI_SIMPLE_NETWORK_MODE.NvRamSize
                                * The BufferSize parameter is not a multiple of
                                  EFI_SIMPLE_NETWORK_MODE.NvRamAccessSize
                                * The Buffer parameter is NULL
  @retval EFI_DEVICE_ERROR      The command could not be sent to the network
                                interface.
  @retval EFI_UNSUPPORTED       This function is not supported by the network
                                interface.

**/
EFI_STATUS
EFIAPI
SnpUndi32NvData (
  IN EFI_SIMPLE_NETWORK_PROTOCOL  *This,
  IN BOOLEAN                      ReadWrite,
  IN UINTN                        Offset,
  IN UINTN                        BufferSize,
  IN OUT VOID                     *Buffer
  )
{
  SNP_DRIVER  *Snp;
  EFI_TPL     OldTpl;
  EFI_STATUS  Status;

  //
  // Get pointer to SNP driver instance for *this.
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
  // Return error if non-volatile memory variables are not valid.
  //
  if ((Snp->Mode.NvRamSize == 0) || (Snp->Mode.NvRamAccessSize == 0)) {
    Status = EFI_UNSUPPORTED;
    goto ON_EXIT;
  }

  //
  // Check for invalid parameter combinations.
  //
  if ((BufferSize == 0) ||
      (Buffer == NULL) ||
      (Offset >= Snp->Mode.NvRamSize) ||
      (Offset + BufferSize > Snp->Mode.NvRamSize) ||
      (BufferSize % Snp->Mode.NvRamAccessSize != 0) ||
      (Offset % Snp->Mode.NvRamAccessSize != 0)
      )
  {
    Status = EFI_INVALID_PARAMETER;
    goto ON_EXIT;
  }

  //
  // check the implementation flags of undi if we can write the nvdata!
  //
  if (!ReadWrite) {
    Status = EFI_UNSUPPORTED;
  } else {
    Status = PxeNvDataRead (Snp, Offset, BufferSize, Buffer);
  }

ON_EXIT:
  gBS->RestoreTPL (OldTpl);

  return Status;
}
