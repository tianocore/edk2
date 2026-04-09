/** @file
  Interpret and execute the S3 data in S3 boot script.

  Copyright (c) 2006 - 2016, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#include "InternalBootScriptLib.h"

/**
  Executes an SMBus operation to an SMBus controller. Returns when either the command has been
  executed or an error is encountered in doing the operation.

  The SmbusExecute() function provides a standard way to execute an operation as defined in the System
  Management Bus (SMBus) Specification. The resulting transaction will be either that the SMBus
  slave devices accept this transaction or that this function returns with error.

  @param  SmbusAddress            Address that encodes the SMBUS Slave Address, SMBUS Command, SMBUS Data Length,
                                  and PEC.
  @param  Operation               Signifies which particular SMBus hardware protocol instance that
                                  it will use to execute the SMBus transactions. This SMBus
                                  hardware protocol is defined by the SMBus Specification and is
                                  not related to EFI.
  @param  Length                  Signifies the number of bytes that this operation will do. The
                                  maximum number of bytes can be revision specific and operation
                                  specific. This field will contain the actual number of bytes that
                                  are executed for this operation. Not all operations require this
                                  argument.
  @param  Buffer                  Contains the value of data to execute to the SMBus slave device.
                                  Not all operations require this argument. The length of this
                                  buffer is identified by Length.

  @retval EFI_SUCCESS             The last data that was returned from the access matched the poll
                                  exit criteria.
  @retval EFI_CRC_ERROR           Checksum is not correct (PEC is incorrect).
  @retval EFI_TIMEOUT             Timeout expired before the operation was completed. Timeout is
                                  determined by the SMBus host controller device.
  @retval EFI_OUT_OF_RESOURCES    The request could not be completed due to a lack of resources.
  @retval EFI_DEVICE_ERROR        The request was not completed because a failure that was
                                  reflected in the Host Status Register bit. Device errors are a
                                  result of a transaction collision, illegal command field,
                                  unclaimed cycle (host initiated), or bus errors (collisions).
  @retval EFI_INVALID_PARAMETER   Operation is not defined in EFI_SMBUS_OPERATION.
  @retval EFI_INVALID_PARAMETER   Length/Buffer is NULL for operations except for EfiSmbusQuickRead
                                  and EfiSmbusQuickWrite. Length is outside the range of valid
                                  values.
  @retval EFI_UNSUPPORTED         The SMBus operation or PEC is not supported.
  @retval EFI_BUFFER_TOO_SMALL    Buffer is not sufficient for this operation.

**/
EFI_STATUS
InternalSmbusExecute (
  IN     UINTN                SmbusAddress,
  IN     EFI_SMBUS_OPERATION  Operation,
  IN OUT UINTN                *Length,
  IN OUT VOID                 *Buffer
  )
{
  EFI_STATUS  Status;
  UINT8       WorkBuffer[MAX_SMBUS_BLOCK_LEN];

  switch (Operation) {
    case EfiSmbusQuickRead:
      DEBUG ((DEBUG_INFO, "EfiSmbusQuickRead - 0x%08x\n", SmbusAddress));
      SmBusQuickRead (SmbusAddress, &Status);
      break;
    case EfiSmbusQuickWrite:
      DEBUG ((DEBUG_INFO, "EfiSmbusQuickWrite - 0x%08x\n", SmbusAddress));
      SmBusQuickWrite (SmbusAddress, &Status);
      break;
    case EfiSmbusReceiveByte:
      DEBUG ((DEBUG_INFO, "EfiSmbusReceiveByte - 0x%08x\n", SmbusAddress));
      SmBusReceiveByte (SmbusAddress, &Status);
      break;
    case EfiSmbusSendByte:
      DEBUG ((DEBUG_INFO, "EfiSmbusSendByte - 0x%08x (0x%02x)\n", SmbusAddress, (UINTN)*(UINT8 *)Buffer));
      SmBusSendByte (SmbusAddress, *(UINT8 *)Buffer, &Status);
      break;
    case EfiSmbusReadByte:
      DEBUG ((DEBUG_INFO, "EfiSmbusReadByte - 0x%08x\n", SmbusAddress));
      SmBusReadDataByte (SmbusAddress, &Status);
      break;
    case EfiSmbusWriteByte:
      DEBUG ((DEBUG_INFO, "EfiSmbusWriteByte - 0x%08x (0x%02x)\n", SmbusAddress, (UINTN)*(UINT8 *)Buffer));
      SmBusWriteDataByte (SmbusAddress, *(UINT8 *)Buffer, &Status);
      break;
    case EfiSmbusReadWord:
      DEBUG ((DEBUG_INFO, "EfiSmbusReadWord - 0x%08x\n", SmbusAddress));
      SmBusReadDataWord (SmbusAddress, &Status);
      break;
    case EfiSmbusWriteWord:
      DEBUG ((DEBUG_INFO, "EfiSmbusWriteWord - 0x%08x (0x%04x)\n", SmbusAddress, (UINTN)*(UINT16 *)Buffer));
      SmBusWriteDataWord (SmbusAddress, *(UINT16 *)Buffer, &Status);
      break;
    case EfiSmbusProcessCall:
      DEBUG ((DEBUG_INFO, "EfiSmbusProcessCall - 0x%08x (0x%04x)\n", SmbusAddress, (UINTN)*(UINT16 *)Buffer));
      SmBusProcessCall (SmbusAddress, *(UINT16 *)Buffer, &Status);
      break;
    case EfiSmbusReadBlock:
      DEBUG ((DEBUG_INFO, "EfiSmbusReadBlock - 0x%08x\n", SmbusAddress));
      SmBusReadBlock (SmbusAddress, WorkBuffer, &Status);
      break;
    case EfiSmbusWriteBlock:
      DEBUG ((DEBUG_INFO, "EfiSmbusWriteBlock - 0x%08x\n", SmbusAddress));
      SmBusWriteBlock ((SmbusAddress + SMBUS_LIB_ADDRESS (0, 0, (*Length), FALSE)), Buffer, &Status);
      break;
    case EfiSmbusBWBRProcessCall:
      DEBUG ((DEBUG_INFO, "EfiSmbusBWBRProcessCall - 0x%08x\n", SmbusAddress));
      SmBusBlockProcessCall ((SmbusAddress + SMBUS_LIB_ADDRESS (0, 0, (*Length), FALSE)), Buffer, WorkBuffer, &Status);
      break;
    default:
      return EFI_INVALID_PARAMETER;
  }

  return Status;
}

/**
  Translates boot script width and address stride to MDE library interface.


  @param Width          Width of the operation.
  @param Address        Address of the operation.
  @param AddressStride  Instride for stepping input buffer.
  @param BufferStride   Outstride for stepping output buffer.

  @retval EFI_SUCCESS  Successful translation.
  @retval EFI_INVALID_PARAMETER Width or Address is invalid.
**/
EFI_STATUS
BuildLoopData (
  IN  S3_BOOT_SCRIPT_LIB_WIDTH  Width,
  IN  UINT64                    Address,
  OUT UINTN                     *AddressStride,
  OUT UINTN                     *BufferStride
  )
{
  UINTN  AlignMask;

  if (Width >= S3BootScriptWidthMaximum) {
    return EFI_INVALID_PARAMETER;
  }

  *AddressStride = (UINT32)(1 << (Width & 0x03));
  *BufferStride  = *AddressStride;

  AlignMask = *AddressStride - 1;
  if ((Address & AlignMask) != 0) {
    return EFI_INVALID_PARAMETER;
  }

  if ((Width >= S3BootScriptWidthFifoUint8) && (Width <= S3BootScriptWidthFifoUint64)) {
    *AddressStride = 0;
  }

  if ((Width >= S3BootScriptWidthFillUint8) && (Width <= S3BootScriptWidthFillUint64)) {
    *BufferStride = 0;
  }

  return EFI_SUCCESS;
}

/**
  Perform IO read operation

  @param[in]  Width   Width of the operation.
  @param[in]  Address Address of the operation.
  @param[in]  Count   Count of the number of accesses to perform.
  @param[out] Buffer  Pointer to the buffer to read from I/O space.

  @retval EFI_SUCCESS The data was written to the EFI System.
  @retval EFI_INVALID_PARAMETER Width is invalid for this EFI System.
                                Buffer is NULL.
                                The Buffer is not aligned for the given Width.
                                Address is outside the legal range of I/O ports.

**/
EFI_STATUS
ScriptIoRead (
  IN      S3_BOOT_SCRIPT_LIB_WIDTH  Width,
  IN      UINT64                    Address,
  IN      UINTN                     Count,
  OUT     VOID                      *Buffer
  )
{
  EFI_STATUS  Status;
  UINTN       AddressStride;
  UINTN       BufferStride;
  PTR         Out;

  Out.Buf = (UINT8 *)Buffer;

  if (Address > MAX_IO_ADDRESS) {
    return EFI_INVALID_PARAMETER;
  }

  Status = BuildLoopData (Width, Address, &AddressStride, &BufferStride);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Loop for each iteration and move the data
  //
  for ( ; Count > 0; Count--, Address += AddressStride, Out.Buf += BufferStride) {
    switch (Width) {
      case S3BootScriptWidthUint8:
        DEBUG ((DEBUG_INFO, "S3BootScriptWidthUint8 - 0x%08x\n", (UINTN)Address));
        *Out.Uint8 = IoRead8 ((UINTN)Address);
        break;
      case S3BootScriptWidthFifoUint8:
        DEBUG ((DEBUG_INFO, "S3BootScriptWidthFifoUint8 - 0x%08x\n", (UINTN)Address));
        *Out.Uint8 = IoRead8 ((UINTN)Address);
        break;
      case S3BootScriptWidthFillUint8:
        DEBUG ((DEBUG_INFO, "S3BootScriptWidthFillUint8 - 0x%08x\n", (UINTN)Address));
        *Out.Uint8 = IoRead8 ((UINTN)Address);
        break;

      case S3BootScriptWidthUint16:
        DEBUG ((DEBUG_INFO, "S3BootScriptWidthUint16 - 0x%08x\n", (UINTN)Address));
        *Out.Uint16 = IoRead16 ((UINTN)Address);
        break;
      case S3BootScriptWidthFifoUint16:
        DEBUG ((DEBUG_INFO, "S3BootScriptWidthFifoUint16 - 0x%08x\n", (UINTN)Address));
        *Out.Uint16 = IoRead16 ((UINTN)Address);
        break;
      case S3BootScriptWidthFillUint16:
        DEBUG ((DEBUG_INFO, "S3BootScriptWidthFillUint16 - 0x%08x\n", (UINTN)Address));
        *Out.Uint16 = IoRead16 ((UINTN)Address);
        break;

      case S3BootScriptWidthUint32:
        DEBUG ((DEBUG_INFO, "S3BootScriptWidthUint32 - 0x%08x\n", (UINTN)Address));
        *Out.Uint32 = IoRead32 ((UINTN)Address);
        break;
      case S3BootScriptWidthFifoUint32:
        DEBUG ((DEBUG_INFO, "S3BootScriptWidthFifoUint32 - 0x%08x\n", (UINTN)Address));
        *Out.Uint32 = IoRead32 ((UINTN)Address);
        break;
      case S3BootScriptWidthFillUint32:
        DEBUG ((DEBUG_INFO, "S3BootScriptWidthFillUint32 - 0x%08x\n", (UINTN)Address));
        *Out.Uint32 = IoRead32 ((UINTN)Address);
        break;

      case S3BootScriptWidthUint64:
        DEBUG ((DEBUG_INFO, "S3BootScriptWidthUint64 - 0x%08x\n", (UINTN)Address));
        *Out.Uint64 = IoRead64 ((UINTN)Address);
        break;
      case S3BootScriptWidthFifoUint64:
        DEBUG ((DEBUG_INFO, "S3BootScriptWidthFifoUint64 - 0x%08x\n", (UINTN)Address));
        *Out.Uint64 = IoRead64 ((UINTN)Address);
        break;
      case S3BootScriptWidthFillUint64:
        DEBUG ((DEBUG_INFO, "S3BootScriptWidthFillUint64 - 0x%08x\n", (UINTN)Address));
        *Out.Uint64 = IoRead64 ((UINTN)Address);
        break;

      default:
        return EFI_INVALID_PARAMETER;
    }
  }

  return EFI_SUCCESS;
}

/**
  Perform IO write operation

  @param[in]  Width Width of the operation.
  @param[in]  Address Address of the operation.
  @param[in]  Count Count of the number of accesses to perform.
  @param[in]  Buffer Pointer to the buffer to write to I/O space.

  @retval EFI_SUCCESS The data was written to the EFI System.
  @retval EFI_INVALID_PARAMETER Width is invalid for this EFI System.
                                Buffer is NULL.
                                The Buffer is not aligned for the given Width.
                                Address is outside the legal range of I/O ports.

**/
EFI_STATUS
ScriptIoWrite (
  IN      S3_BOOT_SCRIPT_LIB_WIDTH  Width,
  IN      UINT64                    Address,
  IN      UINTN                     Count,
  IN      VOID                      *Buffer
  )
{
  EFI_STATUS  Status;
  UINTN       AddressStride;
  UINTN       BufferStride;
  UINT64      OriginalAddress;
  PTR         In;
  PTR         OriginalIn;

  In.Buf = (UINT8 *)Buffer;

  if (Address > MAX_IO_ADDRESS) {
    return EFI_INVALID_PARAMETER;
  }

  Status = BuildLoopData (Width, Address, &AddressStride, &BufferStride);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Loop for each iteration and move the data
  //
  OriginalAddress = Address;
  OriginalIn.Buf  = In.Buf;
  for ( ; Count > 0; Count--, Address += AddressStride, In.Buf += BufferStride) {
    switch (Width) {
      case S3BootScriptWidthUint8:
        DEBUG ((DEBUG_INFO, "S3BootScriptWidthUint8 - 0x%08x (0x%02x)\n", (UINTN)Address, (UINTN)*In.Uint8));
        IoWrite8 ((UINTN)Address, *In.Uint8);
        break;
      case S3BootScriptWidthFifoUint8:
        DEBUG ((DEBUG_INFO, "S3BootScriptWidthFifoUint8 - 0x%08x (0x%02x)\n", (UINTN)OriginalAddress, (UINTN)*In.Uint8));
        IoWrite8 ((UINTN)OriginalAddress, *In.Uint8);
        break;
      case S3BootScriptWidthFillUint8:
        DEBUG ((DEBUG_INFO, "S3BootScriptWidthFillUint8 - 0x%08x (0x%02x)\n", (UINTN)Address, (UINTN)*OriginalIn.Uint8));
        IoWrite8 ((UINTN)Address, *OriginalIn.Uint8);
        break;
      case S3BootScriptWidthUint16:
        DEBUG ((DEBUG_INFO, "S3BootScriptWidthUint16 - 0x%08x (0x%04x)\n", (UINTN)Address, (UINTN)*In.Uint16));
        IoWrite16 ((UINTN)Address, *In.Uint16);
        break;
      case S3BootScriptWidthFifoUint16:
        DEBUG ((DEBUG_INFO, "S3BootScriptWidthFifoUint16 - 0x%08x (0x%04x)\n", (UINTN)OriginalAddress, (UINTN)*In.Uint16));
        IoWrite16 ((UINTN)OriginalAddress, *In.Uint16);
        break;
      case S3BootScriptWidthFillUint16:
        DEBUG ((DEBUG_INFO, "S3BootScriptWidthFillUint16 - 0x%08x (0x%04x)\n", (UINTN)Address, (UINTN)*OriginalIn.Uint16));
        IoWrite16 ((UINTN)Address, *OriginalIn.Uint16);
        break;
      case S3BootScriptWidthUint32:
        DEBUG ((DEBUG_INFO, "S3BootScriptWidthUint32 - 0x%08x (0x%08x)\n", (UINTN)Address, (UINTN)*In.Uint32));
        IoWrite32 ((UINTN)Address, *In.Uint32);
        break;
      case S3BootScriptWidthFifoUint32:
        DEBUG ((DEBUG_INFO, "S3BootScriptWidthFifoUint32 - 0x%08x (0x%08x)\n", (UINTN)OriginalAddress, (UINTN)*In.Uint32));
        IoWrite32 ((UINTN)OriginalAddress, *In.Uint32);
        break;
      case S3BootScriptWidthFillUint32:
        DEBUG ((DEBUG_INFO, "S3BootScriptWidthFillUint32 - 0x%08x (0x%08x)\n", (UINTN)Address, (UINTN)*OriginalIn.Uint32));
        IoWrite32 ((UINTN)Address, *OriginalIn.Uint32);
        break;
      case S3BootScriptWidthUint64:
        DEBUG ((DEBUG_INFO, "S3BootScriptWidthUint64 - 0x%08x (0x%016lx)\n", (UINTN)Address, *In.Uint64));
        IoWrite64 ((UINTN)Address, *In.Uint64);
        break;
      case S3BootScriptWidthFifoUint64:
        DEBUG ((DEBUG_INFO, "S3BootScriptWidthFifoUint64 - 0x%08x (0x%016lx)\n", (UINTN)OriginalAddress, *In.Uint64));
        IoWrite64 ((UINTN)OriginalAddress, *In.Uint64);
        break;
      case S3BootScriptWidthFillUint64:
        DEBUG ((DEBUG_INFO, "S3BootScriptWidthFillUint64 - 0x%08x (0x%016lx)\n", (UINTN)Address, *OriginalIn.Uint64));
        IoWrite64 ((UINTN)Address, *OriginalIn.Uint64);
        break;
      default:
        return EFI_INVALID_PARAMETER;
    }
  }

  return EFI_SUCCESS;
}

/**
  Interpret the boot script node with EFI_BOOT_SCRIPT_IO_WRITE OP code.

  @param Script       Pointer to the node which is to be interpreted.

  @retval EFI_SUCCESS The data was written to the EFI System.
  @retval EFI_INVALID_PARAMETER Width is invalid for this EFI System.
                                Buffer is NULL.
                                The Buffer is not aligned for the given Width.
                                Address is outside the legal range of I/O ports.

**/
EFI_STATUS
BootScriptExecuteIoWrite (
  IN UINT8  *Script
  )
{
  S3_BOOT_SCRIPT_LIB_WIDTH  Width;
  UINT64                    Address;
  UINTN                     Count;
  VOID                      *Buffer;
  EFI_BOOT_SCRIPT_IO_WRITE  IoWrite;

  CopyMem ((VOID *)&IoWrite, (VOID *)Script, sizeof (EFI_BOOT_SCRIPT_IO_WRITE));
  Width   = (S3_BOOT_SCRIPT_LIB_WIDTH)IoWrite.Width;
  Address = IoWrite.Address;
  Count   = IoWrite.Count;
  Buffer  = Script + sizeof (EFI_BOOT_SCRIPT_IO_WRITE);

  DEBUG ((DEBUG_INFO, "BootScriptExecuteIoWrite - 0x%08x, 0x%08x, 0x%08x\n", (UINTN)Address, Count, (UINTN)Width));
  return ScriptIoWrite (Width, Address, Count, Buffer);
}

/**
  Perform memory read operation

  @param  Width Width of the operation.
  @param  Address Address of the operation.
  @param  Count Count of the number of accesses to perform.
  @param  Buffer Pointer to the buffer read from memory.

  @retval EFI_SUCCESS The data was written to the EFI System.
  @retval EFI_INVALID_PARAMETER Width is invalid for this EFI System.
                                Buffer is NULL.
                                The Buffer is not aligned for the given Width.
  @retval EFI_UNSUPPORTED The address range specified by Address, Width, and Count
                          is not valid for this EFI System.

**/
EFI_STATUS
ScriptMemoryRead (
  IN       S3_BOOT_SCRIPT_LIB_WIDTH  Width,
  IN       UINT64                    Address,
  IN       UINTN                     Count,
  IN OUT   VOID                      *Buffer
  )
{
  EFI_STATUS  Status;
  UINTN       AddressStride;
  UINTN       BufferStride;
  PTR         Out;

  Out.Buf = Buffer;

  Status = BuildLoopData (Width, Address, &AddressStride, &BufferStride);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Loop for each iteration and move the data
  //
  for ( ; Count > 0; Count--, Address += AddressStride, Out.Buf += BufferStride) {
    switch (Width) {
      case S3BootScriptWidthUint8:
        DEBUG ((DEBUG_INFO, "S3BootScriptWidthUint8 - 0x%08x\n", (UINTN)Address));
        *Out.Uint8 = MmioRead8 ((UINTN)Address);
        break;
      case S3BootScriptWidthFifoUint8:
        DEBUG ((DEBUG_INFO, "S3BootScriptWidthFifoUint8 - 0x%08x\n", (UINTN)Address));
        *Out.Uint8 = MmioRead8 ((UINTN)Address);
        break;
      case S3BootScriptWidthFillUint8:
        DEBUG ((DEBUG_INFO, "S3BootScriptWidthFillUint8 - 0x%08x\n", (UINTN)Address));
        *Out.Uint8 = MmioRead8 ((UINTN)Address);
        break;

      case S3BootScriptWidthUint16:
        DEBUG ((DEBUG_INFO, "S3BootScriptWidthUint16 - 0x%08x\n", (UINTN)Address));
        *Out.Uint16 = MmioRead16 ((UINTN)Address);
        break;
      case S3BootScriptWidthFifoUint16:
        DEBUG ((DEBUG_INFO, "S3BootScriptWidthFifoUint16 - 0x%08x\n", (UINTN)Address));
        *Out.Uint16 = MmioRead16 ((UINTN)Address);
        break;
      case S3BootScriptWidthFillUint16:
        DEBUG ((DEBUG_INFO, "S3BootScriptWidthFillUint16 - 0x%08x\n", (UINTN)Address));
        *Out.Uint16 = MmioRead16 ((UINTN)Address);
        break;

      case S3BootScriptWidthUint32:
        DEBUG ((DEBUG_INFO, "S3BootScriptWidthUint32 - 0x%08x\n", (UINTN)Address));
        *Out.Uint32 = MmioRead32 ((UINTN)Address);
        break;
      case S3BootScriptWidthFifoUint32:
        DEBUG ((DEBUG_INFO, "S3BootScriptWidthFifoUint32 - 0x%08x\n", (UINTN)Address));
        *Out.Uint32 = MmioRead32 ((UINTN)Address);
        break;
      case S3BootScriptWidthFillUint32:
        DEBUG ((DEBUG_INFO, "S3BootScriptWidthFillUint32 - 0x%08x\n", (UINTN)Address));
        *Out.Uint32 = MmioRead32 ((UINTN)Address);
        break;

      case S3BootScriptWidthUint64:
        DEBUG ((DEBUG_INFO, "S3BootScriptWidthUint64 - 0x%08x\n", (UINTN)Address));
        *Out.Uint64 = MmioRead64 ((UINTN)Address);
        break;
      case S3BootScriptWidthFifoUint64:
        DEBUG ((DEBUG_INFO, "S3BootScriptWidthFifoUint64 - 0x%08x\n", (UINTN)Address));
        *Out.Uint64 = MmioRead64 ((UINTN)Address);
        break;
      case S3BootScriptWidthFillUint64:
        DEBUG ((DEBUG_INFO, "S3BootScriptWidthFillUint64 - 0x%08x\n", (UINTN)Address));
        *Out.Uint64 = MmioRead64 ((UINTN)Address);
        break;

      default:
        return EFI_UNSUPPORTED;
    }
  }

  return EFI_SUCCESS;
}

/**
  Perform memory write operation

  @param   Width   Width of the operation.
  @param   Address Address of the operation.
  @param   Count   Count of the number of accesses to perform.
  @param   Buffer  Pointer to the buffer write to memory.

  @retval EFI_SUCCESS The data was written to the EFI System.
  @retval EFI_INVALID_PARAMETER Width is invalid for this EFI System.
                                Buffer is NULL.
                                The Buffer is not aligned for the given Width.
  @retval EFI_UNSUPPORTED The address range specified by Address, Width, and Count
                          is not valid for this EFI System.

**/
EFI_STATUS
ScriptMemoryWrite (
  IN      S3_BOOT_SCRIPT_LIB_WIDTH  Width,
  IN      UINT64                    Address,
  IN      UINTN                     Count,
  IN OUT  VOID                      *Buffer
  )
{
  EFI_STATUS  Status;
  UINTN       AddressStride;
  UINT64      OriginalAddress;
  UINTN       BufferStride;
  PTR         In;
  PTR         OriginalIn;

  In.Buf = Buffer;

  Status = BuildLoopData (Width, Address, &AddressStride, &BufferStride);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Loop for each iteration and move the data
  //
  OriginalAddress = Address;
  OriginalIn.Buf  = In.Buf;
  for ( ; Count > 0; Count--, Address += AddressStride, In.Buf += BufferStride) {
    switch (Width) {
      case S3BootScriptWidthUint8:
        DEBUG ((DEBUG_INFO, "S3BootScriptWidthUint8 - 0x%08x (0x%02x)\n", (UINTN)Address, (UINTN)*In.Uint8));
        MmioWrite8 ((UINTN)Address, *In.Uint8);
        break;
      case S3BootScriptWidthFifoUint8:
        DEBUG ((DEBUG_INFO, "S3BootScriptWidthFifoUint8 - 0x%08x (0x%02x)\n", (UINTN)OriginalAddress, (UINTN)*In.Uint8));
        MmioWrite8 ((UINTN)OriginalAddress, *In.Uint8);
        break;
      case S3BootScriptWidthFillUint8:
        DEBUG ((DEBUG_INFO, "S3BootScriptWidthFillUint8 - 0x%08x (0x%02x)\n", (UINTN)Address, (UINTN)*OriginalIn.Uint8));
        MmioWrite8 ((UINTN)Address, *OriginalIn.Uint8);
        break;
      case S3BootScriptWidthUint16:
        DEBUG ((DEBUG_INFO, "S3BootScriptWidthUint16 - 0x%08x (0x%04x)\n", (UINTN)Address, (UINTN)*In.Uint16));
        MmioWrite16 ((UINTN)Address, *In.Uint16);
        break;
      case S3BootScriptWidthFifoUint16:
        DEBUG ((DEBUG_INFO, "S3BootScriptWidthFifoUint16 - 0x%08x (0x%04x)\n", (UINTN)OriginalAddress, (UINTN)*In.Uint16));
        MmioWrite16 ((UINTN)OriginalAddress, *In.Uint16);
        break;
      case S3BootScriptWidthFillUint16:
        DEBUG ((DEBUG_INFO, "S3BootScriptWidthFillUint16 - 0x%08x (0x%04x)\n", (UINTN)Address, (UINTN)*OriginalIn.Uint16));
        MmioWrite16 ((UINTN)Address, *OriginalIn.Uint16);
        break;
      case S3BootScriptWidthUint32:
        DEBUG ((DEBUG_INFO, "S3BootScriptWidthUint32 - 0x%08x (0x%08x)\n", (UINTN)Address, (UINTN)*In.Uint32));
        MmioWrite32 ((UINTN)Address, *In.Uint32);
        break;
      case S3BootScriptWidthFifoUint32:
        DEBUG ((DEBUG_INFO, "S3BootScriptWidthFifoUint32 - 0x%08x (0x%08x)\n", (UINTN)OriginalAddress, (UINTN)*In.Uint32));
        MmioWrite32 ((UINTN)OriginalAddress, *In.Uint32);
        break;
      case S3BootScriptWidthFillUint32:
        DEBUG ((DEBUG_INFO, "S3BootScriptWidthFillUint32 - 0x%08x (0x%08x)\n", (UINTN)Address, (UINTN)*OriginalIn.Uint32));
        MmioWrite32 ((UINTN)Address, *OriginalIn.Uint32);
        break;
      case S3BootScriptWidthUint64:
        DEBUG ((DEBUG_INFO, "S3BootScriptWidthUint64 - 0x%08x (0x%016lx)\n", (UINTN)Address, *In.Uint64));
        MmioWrite64 ((UINTN)Address, *In.Uint64);
        break;
      case S3BootScriptWidthFifoUint64:
        DEBUG ((DEBUG_INFO, "S3BootScriptWidthFifoUint64 - 0x%08x (0x%016lx)\n", (UINTN)OriginalAddress, *In.Uint64));
        MmioWrite64 ((UINTN)OriginalAddress, *In.Uint64);
        break;
      case S3BootScriptWidthFillUint64:
        DEBUG ((DEBUG_INFO, "S3BootScriptWidthFillUint64 - 0x%08x (0x%016lx)\n", (UINTN)Address, *OriginalIn.Uint64));
        MmioWrite64 ((UINTN)Address, *OriginalIn.Uint64);
        break;
      default:
        return EFI_UNSUPPORTED;
    }
  }

  return EFI_SUCCESS;
}

/**
  Interpret the boot script node with EFI_BOOT_SCRIPT_MEM_WRITE OP code.

  @param[in]  Script Pointer to the node which is to be interpreted.

  @retval EFI_SUCCESS The data was written to the EFI System.
  @retval EFI_INVALID_PARAMETER Width is invalid for this EFI System.
                                Buffer is NULL.
                                The Buffer is not aligned for the given Width.
  @retval EFI_UNSUPPORTED The address range specified by Address, Width, and Count
                          is not valid for this EFI System.

**/
EFI_STATUS
BootScriptExecuteMemoryWrite (
  IN UINT8  *Script
  )
{
  VOID                       *Buffer;
  S3_BOOT_SCRIPT_LIB_WIDTH   Width;
  UINT64                     Address;
  UINTN                      Count;
  EFI_BOOT_SCRIPT_MEM_WRITE  MemWrite;

  CopyMem ((VOID *)&MemWrite, (VOID *)Script, sizeof (EFI_BOOT_SCRIPT_MEM_WRITE));
  Width   = (S3_BOOT_SCRIPT_LIB_WIDTH)MemWrite.Width;
  Address = MemWrite.Address;
  Count   = MemWrite.Count;
  Buffer  = Script + sizeof (EFI_BOOT_SCRIPT_MEM_WRITE);

  DEBUG ((DEBUG_INFO, "BootScriptExecuteMemoryWrite - 0x%08x, 0x%08x, 0x%08x\n", (UINTN)Address, Count, (UINTN)Width));
  return ScriptMemoryWrite (Width, Address, Count, Buffer);
}

/**
  Performance PCI configuration 2 read operation

  @param  Width   Width of the operation.
  @param  Segment Pci segment number
  @param  Address Address of the operation.
  @param  Count   Count of the number of accesses to perform.
  @param  Buffer  Pointer to the buffer read from PCI config space

  @retval EFI_SUCCESS The read succeed.
  @retval EFI_INVALID_PARAMETER if Width is not defined
  @note  A known Limitations in the implementation which is 64bits operations are not supported.

**/
EFI_STATUS
ScriptPciCfg2Read (
  IN  S3_BOOT_SCRIPT_LIB_WIDTH  Width,
  IN  UINT16                    Segment,
  IN  UINT64                    Address,
  IN  UINTN                     Count,
  OUT VOID                      *Buffer
  )
{
  EFI_STATUS  Status;
  UINTN       AddressStride;
  UINTN       BufferStride;
  PTR         Out;
  UINT64      PciAddress;

  Out.Buf = (UINT8 *)Buffer;

  PciAddress = PCI_ADDRESS_ENCODE (Segment, Address);

  Status = BuildLoopData (Width, PciAddress, &AddressStride, &BufferStride);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Loop for each iteration and move the data
  //
  for ( ; Count > 0; Count--, PciAddress += AddressStride, Out.Buf += BufferStride) {
    switch (Width) {
      case S3BootScriptWidthUint8:
        DEBUG ((DEBUG_INFO, "S3BootScriptWidthUint8 - 0x%016lx\n", PciAddress));
        *Out.Uint8 = PciSegmentRead8 (PciAddress);
        break;
      case S3BootScriptWidthFifoUint8:
        DEBUG ((DEBUG_INFO, "S3BootScriptWidthFifoUint8 - 0x%016lx\n", PciAddress));
        *Out.Uint8 = PciSegmentRead8 (PciAddress);
        break;
      case S3BootScriptWidthFillUint8:
        DEBUG ((DEBUG_INFO, "S3BootScriptWidthFillUint8 - 0x%016lx\n", PciAddress));
        *Out.Uint8 = PciSegmentRead8 (PciAddress);
        break;

      case S3BootScriptWidthUint16:
        DEBUG ((DEBUG_INFO, "S3BootScriptWidthUint16 - 0x%016lx\n", PciAddress));
        *Out.Uint16 = PciSegmentRead16 (PciAddress);
        break;
      case S3BootScriptWidthFifoUint16:
        DEBUG ((DEBUG_INFO, "S3BootScriptWidthFifoUint16 - 0x%016lx\n", PciAddress));
        *Out.Uint16 = PciSegmentRead16 (PciAddress);
        break;
      case S3BootScriptWidthFillUint16:
        DEBUG ((DEBUG_INFO, "S3BootScriptWidthFillUint16 - 0x%016lx\n", PciAddress));
        *Out.Uint16 = PciSegmentRead16 (PciAddress);
        break;

      case S3BootScriptWidthUint32:
        DEBUG ((DEBUG_INFO, "S3BootScriptWidthUint32 - 0x%016lx\n", PciAddress));
        *Out.Uint32 = PciSegmentRead32 (PciAddress);
        break;
      case S3BootScriptWidthFifoUint32:
        DEBUG ((DEBUG_INFO, "S3BootScriptWidthFifoUint32 - 0x%016lx\n", PciAddress));
        *Out.Uint32 = PciSegmentRead32 (PciAddress);
        break;
      case S3BootScriptWidthFillUint32:
        DEBUG ((DEBUG_INFO, "S3BootScriptWidthFillUint32 - 0x%016lx\n", PciAddress));
        *Out.Uint32 = PciSegmentRead32 (PciAddress);
        break;

      default:
        return EFI_INVALID_PARAMETER;
    }
  }

  return EFI_SUCCESS;
}

/**
  Performance PCI configuration 2 write operation

  @param  Width   Width of the operation.
  @param  Segment Pci segment number
  @param  Address Address of the operation.
  @param  Count   Count of the number of accesses to perform.
  @param  Buffer  Pointer to the buffer write to PCI config space

  @retval EFI_SUCCESS The write succeed.
  @retval EFI_INVALID_PARAMETER if Width is not defined
  @note  A known Limitations in the implementation which is 64bits operations are not supported.

**/
EFI_STATUS
ScriptPciCfg2Write (
  IN  S3_BOOT_SCRIPT_LIB_WIDTH  Width,
  IN  UINT16                    Segment,
  IN  UINT64                    Address,
  IN  UINTN                     Count,
  IN  VOID                      *Buffer
  )
{
  EFI_STATUS  Status;
  UINTN       AddressStride;
  UINTN       BufferStride;
  UINT64      OriginalPciAddress;
  PTR         In;
  PTR         OriginalIn;
  UINT64      PciAddress;

  In.Buf = (UINT8 *)Buffer;

  PciAddress = PCI_ADDRESS_ENCODE (Segment, Address);

  Status = BuildLoopData (Width, PciAddress, &AddressStride, &BufferStride);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Loop for each iteration and move the data
  //
  OriginalPciAddress = PciAddress;
  OriginalIn.Buf     = In.Buf;
  for ( ; Count > 0; Count--, PciAddress += AddressStride, In.Buf += BufferStride) {
    switch (Width) {
      case S3BootScriptWidthUint8:
        DEBUG ((DEBUG_INFO, "S3BootScriptWidthUint8 - 0x%016lx (0x%02x)\n", PciAddress, (UINTN)*In.Uint8));
        PciSegmentWrite8 (PciAddress, *In.Uint8);
        break;
      case S3BootScriptWidthFifoUint8:
        DEBUG ((DEBUG_INFO, "S3BootScriptWidthFifoUint8 - 0x%016lx (0x%02x)\n", OriginalPciAddress, (UINTN)*In.Uint8));
        PciSegmentWrite8 (OriginalPciAddress, *In.Uint8);
        break;
      case S3BootScriptWidthFillUint8:
        DEBUG ((DEBUG_INFO, "S3BootScriptWidthFillUint8 - 0x%016lx (0x%02x)\n", PciAddress, (UINTN)*OriginalIn.Uint8));
        PciSegmentWrite8 (PciAddress, *OriginalIn.Uint8);
        break;
      case S3BootScriptWidthUint16:
        DEBUG ((DEBUG_INFO, "S3BootScriptWidthUint16 - 0x%016lx (0x%04x)\n", PciAddress, (UINTN)*In.Uint16));
        PciSegmentWrite16 (PciAddress, *In.Uint16);
        break;
      case S3BootScriptWidthFifoUint16:
        DEBUG ((DEBUG_INFO, "S3BootScriptWidthFifoUint16 - 0x%016lx (0x%04x)\n", OriginalPciAddress, (UINTN)*In.Uint16));
        PciSegmentWrite16 (OriginalPciAddress, *In.Uint16);
        break;
      case S3BootScriptWidthFillUint16:
        DEBUG ((DEBUG_INFO, "S3BootScriptWidthFillUint16 - 0x%016lx (0x%04x)\n", PciAddress, (UINTN)*OriginalIn.Uint16));
        PciSegmentWrite16 (PciAddress, *OriginalIn.Uint16);
        break;
      case S3BootScriptWidthUint32:
        DEBUG ((DEBUG_INFO, "S3BootScriptWidthUint32 - 0x%016lx (0x%08x)\n", PciAddress, (UINTN)*In.Uint32));
        PciSegmentWrite32 (PciAddress, *In.Uint32);
        break;
      case S3BootScriptWidthFifoUint32:
        DEBUG ((DEBUG_INFO, "S3BootScriptWidthFifoUint32 - 0x%016lx (0x%08x)\n", OriginalPciAddress, (UINTN)*In.Uint32));
        PciSegmentWrite32 (OriginalPciAddress, *In.Uint32);
        break;
      case S3BootScriptWidthFillUint32:
        DEBUG ((DEBUG_INFO, "S3BootScriptWidthFillUint32 - 0x%016lx (0x%08x)\n", (UINTN)PciAddress, (UINTN)*OriginalIn.Uint32));
        PciSegmentWrite32 (PciAddress, *OriginalIn.Uint32);
        break;
      default:
        return EFI_INVALID_PARAMETER;
    }
  }

  return EFI_SUCCESS;
}

/**
  Performance PCI configuration read operation

  @param     Width                      Width of the operation.
  @param     Address                    Address of the operation.
  @param     Count                      Count of the number of accesses to perform.
  @param     Buffer                     Pointer to the buffer to read from PCI config space.

  @retval    EFI_SUCCESS                The data was written to the EFI System.
  @retval    EFI_INVALID_PARAMETER      Width is invalid for this EFI System.
                                        Buffer is NULL.
                                        The Buffer is not aligned for the given Width.
                                        Address is outside the legal range of I/O ports.

**/
EFI_STATUS
ScriptPciCfgRead (
  IN  S3_BOOT_SCRIPT_LIB_WIDTH  Width,
  IN  UINT64                    Address,
  IN  UINTN                     Count,
  OUT VOID                      *Buffer
  )
{
  return ScriptPciCfg2Read (Width, 0, Address, Count, Buffer);
}

/**
  Performance PCI configuration write operation

  @param     Width                      Width of the operation.
  @param     Address                    Address of the operation.
  @param     Count                      Count of the number of accesses to perform.
  @param     Buffer                     Pointer to the buffer to write to PCI config space.

  @retval    EFI_SUCCESS                The data was written to the EFI System.
  @retval    EFI_INVALID_PARAMETER      Width is invalid for this EFI System.
                                        Buffer is NULL.
                                        The Buffer is not aligned for the given Width.
                                        Address is outside the legal range of I/O ports.

**/
EFI_STATUS
EFIAPI
ScriptPciCfgWrite (
  IN  S3_BOOT_SCRIPT_LIB_WIDTH  Width,
  IN  UINT64                    Address,
  IN  UINTN                     Count,
  IN  VOID                      *Buffer
  )
{
  return ScriptPciCfg2Write (Width, 0, Address, Count, Buffer);
}

/**
  Interpret the boot script node with EFI_BOOT_SCRIPT_PCI_CONFIG_WRITE OP code.

  @param  Script        The pointer of typed node in boot script table

  @retval EFI_SUCCESS  The operation was executed successfully
**/
EFI_STATUS
BootScriptExecutePciCfgWrite (
  IN UINT8  *Script
  )
{
  VOID                              *Buffer;
  S3_BOOT_SCRIPT_LIB_WIDTH          Width;
  UINT64                            Address;
  UINTN                             Count;
  EFI_BOOT_SCRIPT_PCI_CONFIG_WRITE  PciCfgWrite;

  CopyMem ((VOID *)&PciCfgWrite, (VOID *)Script, sizeof (EFI_BOOT_SCRIPT_PCI_CONFIG_WRITE));

  Width   = (S3_BOOT_SCRIPT_LIB_WIDTH)PciCfgWrite.Width;
  Address = PciCfgWrite.Address;
  Count   = PciCfgWrite.Count;
  Buffer  = Script + sizeof (EFI_BOOT_SCRIPT_PCI_CONFIG_WRITE);

  DEBUG ((DEBUG_INFO, "BootScriptExecutePciCfgWrite - 0x%016lx, 0x%08x, 0x%08x\n", PCI_ADDRESS_ENCODE (0, Address), Count, (UINTN)Width));
  return ScriptPciCfgWrite (Width, Address, Count, Buffer);
}

/**
  Interpret the boot script node with EFI_BOOT_SCRIPT_IO_READ_WRITE OP code.

  @param Script   The pointer of typed node in boot script table
  @param AndMask  Mask value for 'and' operation
  @param OrMask   Mask value for 'or' operation

  @retval EFI_SUCCESS    The operation was executed successfully
**/
EFI_STATUS
BootScriptExecuteIoReadWrite (
  IN  UINT8   *Script,
  IN  UINT64  AndMask,
  IN  UINT64  OrMask
  )

{
  EFI_STATUS                     Status;
  UINT64                         Data;
  EFI_BOOT_SCRIPT_IO_READ_WRITE  IoReadWrite;

  Data = 0;

  CopyMem ((VOID *)&IoReadWrite, (VOID *)Script, sizeof (EFI_BOOT_SCRIPT_IO_READ_WRITE));

  DEBUG ((DEBUG_INFO, "BootScriptExecuteIoReadWrite - 0x%08x, 0x%016lx, 0x%016lx\n", (UINTN)IoReadWrite.Address, AndMask, OrMask));

  Status = ScriptIoRead (
             (S3_BOOT_SCRIPT_LIB_WIDTH)IoReadWrite.Width,
             IoReadWrite.Address,
             1,
             &Data
             );
  if (!EFI_ERROR (Status)) {
    Data   = (Data & AndMask) | OrMask;
    Status = ScriptIoWrite (
               (S3_BOOT_SCRIPT_LIB_WIDTH)IoReadWrite.Width,
               IoReadWrite.Address,
               1,
               &Data
               );
  }

  return Status;
}

/**
  Interpret the boot script node with EFI_BOOT_SCRIPT_MEM_READ_WRITE OP code.

  @param Script    The pointer of typed node in boot script table
  @param AndMask   Mask value for 'and' operation
  @param OrMask    Mask value for 'or' operation

  @retval EFI_SUCCESS The operation was executed successfully
**/
EFI_STATUS
BootScriptExecuteMemoryReadWrite (
  IN UINT8   *Script,
  IN UINT64  AndMask,
  IN UINT64  OrMask
  )

{
  EFI_STATUS                      Status;
  UINT64                          Data;
  EFI_BOOT_SCRIPT_MEM_READ_WRITE  MemReadWrite;

  Data = 0;

  CopyMem ((VOID *)&MemReadWrite, (VOID *)Script, sizeof (EFI_BOOT_SCRIPT_MEM_READ_WRITE));

  DEBUG ((DEBUG_INFO, "BootScriptExecuteMemoryReadWrite - 0x%08x, 0x%016lx, 0x%016lx\n", (UINTN)MemReadWrite.Address, AndMask, OrMask));

  Status = ScriptMemoryRead (
             (S3_BOOT_SCRIPT_LIB_WIDTH)MemReadWrite.Width,
             MemReadWrite.Address,
             1,
             &Data
             );
  if (!EFI_ERROR (Status)) {
    Data   = (Data & AndMask) | OrMask;
    Status = ScriptMemoryWrite (
               (S3_BOOT_SCRIPT_LIB_WIDTH)MemReadWrite.Width,
               MemReadWrite.Address,
               1,
               &Data
               );
  }

  return Status;
}

/**
  Interpret the boot script node with EFI_BOOT_SCRIPT_PCI_CFG_READ_WRITE OP code.

  @param Script   The pointer of typed node in boot script table
  @param AndMask  Mask value for 'and' operation
  @param OrMask   Mask value for 'or' operation

  @retval EFI_SUCCESS   The operation was executed successfully
**/
EFI_STATUS
BootScriptExecutePciCfgReadWrite (
  IN UINT8   *Script,
  IN UINT64  AndMask,
  IN UINT64  OrMask
  )

{
  EFI_STATUS                             Status;
  UINT64                                 Data;
  EFI_BOOT_SCRIPT_PCI_CONFIG_READ_WRITE  PciCfgReadWrite;

  Data = 0;

  CopyMem ((VOID *)&PciCfgReadWrite, (VOID *)Script, sizeof (EFI_BOOT_SCRIPT_PCI_CONFIG_READ_WRITE));

  DEBUG ((DEBUG_INFO, "BootScriptExecutePciCfgReadWrite - 0x%016lx, 0x%016lx, 0x%016lx\n", PCI_ADDRESS_ENCODE (0, PciCfgReadWrite.Address), AndMask, OrMask));

  Status = ScriptPciCfgRead (
             (S3_BOOT_SCRIPT_LIB_WIDTH)PciCfgReadWrite.Width,
             PciCfgReadWrite.Address,
             1,
             &Data
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Data = (Data & AndMask) | OrMask;

  Status = ScriptPciCfgWrite (
             (S3_BOOT_SCRIPT_LIB_WIDTH)PciCfgReadWrite.Width,
             PciCfgReadWrite.Address,
             1,
             &Data
             );

  return Status;
}

/**
  Interpret the boot script node with EFI_BOOT_SCRIPT_SMBUS_EXECUTE OP code.

  @param Script  The pointer of typed node in boot script table

  @retval EFI_SUCCESS      The operation was executed successfully
  @retval EFI_UNSUPPORTED  Cannot locate smbus ppi or occur error of script execution
  @retval Others           Result of script execution
**/
EFI_STATUS
BootScriptExecuteSmbusExecute (
  IN UINT8  *Script
  )
{
  UINTN                          SmBusAddress;
  UINTN                          DataSize;
  EFI_BOOT_SCRIPT_SMBUS_EXECUTE  SmbusExecuteEntry;

  CopyMem ((VOID *)&SmbusExecuteEntry, (VOID *)Script, sizeof (EFI_BOOT_SCRIPT_SMBUS_EXECUTE));

  DEBUG ((DEBUG_INFO, "BootScriptExecuteSmbusExecute - 0x%08x, 0x%08x\n", (UINTN)SmbusExecuteEntry.SmBusAddress, (UINTN)SmbusExecuteEntry.Operation));

  SmBusAddress = (UINTN)SmbusExecuteEntry.SmBusAddress;
  DataSize     = (UINTN)SmbusExecuteEntry.DataSize;
  return InternalSmbusExecute (
           SmBusAddress,
           (EFI_SMBUS_OPERATION)SmbusExecuteEntry.Operation,
           &DataSize,
           Script + sizeof (EFI_BOOT_SCRIPT_SMBUS_EXECUTE)
           );
}

/**
  Interpret the boot script node with EFI_BOOT_SCRIPT_STALL OP code.

  @param Script      The pointer of typed node in boot script table

  @retval EFI_SUCCESS The operation was executed successfully
**/
EFI_STATUS
BootScriptExecuteStall (
  IN UINT8  *Script
  )
{
  EFI_BOOT_SCRIPT_STALL  Stall;

  CopyMem ((VOID *)&Stall, (VOID *)Script, sizeof (EFI_BOOT_SCRIPT_STALL));

  DEBUG ((DEBUG_INFO, "BootScriptExecuteStall - 0x%08x\n", (UINTN)Stall.Duration));

  MicroSecondDelay ((UINTN)Stall.Duration);
  return EFI_SUCCESS;
}

/**
  Interpret the boot script node with EFI_BOOT_SCRIPT_DISPATCH OP code.

  @param Script  The pointer of typed node in boot script table
  @retval EFI_SUCCESS  The operation was executed successfully
**/
EFI_STATUS
BootScriptExecuteDispatch (
  IN UINT8  *Script
  )
{
  EFI_STATUS                Status;
  DISPATCH_ENTRYPOINT_FUNC  EntryFunc;
  EFI_BOOT_SCRIPT_DISPATCH  ScriptDispatch;

  CopyMem ((VOID *)&ScriptDispatch, (VOID *)Script, sizeof (EFI_BOOT_SCRIPT_DISPATCH));
  EntryFunc = (DISPATCH_ENTRYPOINT_FUNC)(UINTN)(ScriptDispatch.EntryPoint);

  DEBUG ((DEBUG_INFO, "BootScriptExecuteDispatch - 0x%08x\n", (UINTN)ScriptDispatch.EntryPoint));

  Status = EntryFunc (NULL, NULL);

  return Status;
}

/**
  Interpret the boot script node with EFI_BOOT_SCRIPT_DISPATCH_2 OP code.

  @param  Script       The pointer of typed node in boot script table
  @retval EFI_SUCCESS  The operation was executed successfully
**/
EFI_STATUS
BootScriptExecuteDispatch2 (
  IN UINT8  *Script
  )
{
  EFI_STATUS                  Status;
  DISPATCH_ENTRYPOINT_FUNC    EntryFunc;
  EFI_BOOT_SCRIPT_DISPATCH_2  ScriptDispatch2;

  CopyMem ((VOID *)&ScriptDispatch2, (VOID *)Script, sizeof (EFI_BOOT_SCRIPT_DISPATCH_2));

  DEBUG ((DEBUG_INFO, "BootScriptExecuteDispatch2 - 0x%08x(0x%08x)\n", (UINTN)ScriptDispatch2.EntryPoint, (UINTN)ScriptDispatch2.Context));

  EntryFunc = (DISPATCH_ENTRYPOINT_FUNC)(UINTN)(ScriptDispatch2.EntryPoint);

  Status = EntryFunc (NULL, (VOID *)(UINTN)ScriptDispatch2.Context);

  return Status;
}

/**
  Interpret the boot script node with EFI_BOOT_SCRIPT_MEM_POLL OP code.

  @param  Script  The pointer of typed node in boot script table
  @param  AndMask  Mask value for 'and' operation
  @param  OrMask   Mask value for 'or' operation

  @retval EFI_DEVICE_ERROR Data polled from memory does not equal to
                           the epecting data within the Loop Times.
  @retval EFI_SUCCESS      The operation was executed successfully
**/
EFI_STATUS
BootScriptExecuteMemPoll (
  IN UINT8   *Script,
  IN UINT64  AndMask,
  IN UINT64  OrMask
  )
{
  UINT64                    Data;
  UINT64                    LoopTimes;
  EFI_STATUS                Status;
  EFI_BOOT_SCRIPT_MEM_POLL  MemPoll;

  CopyMem ((VOID *)&MemPoll, (VOID *)Script, sizeof (EFI_BOOT_SCRIPT_MEM_POLL));

  DEBUG ((DEBUG_INFO, "BootScriptExecuteMemPoll - 0x%08x, 0x%016lx, 0x%016lx\n", (UINTN)MemPoll.Address, AndMask, OrMask));

  Data   = 0;
  Status = ScriptMemoryRead (
             (S3_BOOT_SCRIPT_LIB_WIDTH)MemPoll.Width,
             MemPoll.Address,
             1,
             &Data
             );
  if ((!EFI_ERROR (Status)) && ((Data & AndMask) == OrMask)) {
    return EFI_SUCCESS;
  }

  for (LoopTimes = 0; LoopTimes < MemPoll.LoopTimes; LoopTimes++) {
    MicroSecondDelay ((UINTN)MemPoll.Duration);

    Data   = 0;
    Status = ScriptMemoryRead (
               (S3_BOOT_SCRIPT_LIB_WIDTH)MemPoll.Width,
               MemPoll.Address,
               1,
               &Data
               );
    if ((!EFI_ERROR (Status)) && ((Data & AndMask) == OrMask)) {
      return EFI_SUCCESS;
    }
  }

  if (LoopTimes < MemPoll.LoopTimes) {
    return EFI_SUCCESS;
  } else {
    return EFI_DEVICE_ERROR;
  }
}

/**
  Execute the boot script to interpret the Store arbitrary information.
  This opcode is a no-op on dispatch and is only used for debugging script issues.

  @param Script       The pointer of node in boot script table

**/
VOID
BootScriptExecuteInformation (
  IN UINT8  *Script
  )

{
  UINT32                       Index;
  EFI_BOOT_SCRIPT_INFORMATION  Information;
  UINT8                        *InformationData;

  CopyMem ((VOID *)&Information, (VOID *)Script, sizeof (EFI_BOOT_SCRIPT_INFORMATION));

  InformationData = Script + sizeof (EFI_BOOT_SCRIPT_INFORMATION);
  DEBUG ((DEBUG_INFO, "BootScriptExecuteInformation - 0x%08x\n", (UINTN)InformationData));

  DEBUG ((DEBUG_INFO, "BootScriptInformation: "));
  for (Index = 0; Index < Information.InformationLength; Index++) {
    DEBUG ((DEBUG_INFO, "%02x ", InformationData[Index]));
  }

  DEBUG ((DEBUG_INFO, "\n"));
}

/**
  Execute the boot script to interpret the Label information.

  @param Script       The pointer of node in boot script table

**/
VOID
BootScriptExecuteLabel (
  IN UINT8  *Script
  )

{
  UINT32                       Index;
  EFI_BOOT_SCRIPT_INFORMATION  Information;
  UINT8                        *InformationData;

  CopyMem ((VOID *)&Information, (VOID *)Script, sizeof (EFI_BOOT_SCRIPT_INFORMATION));

  InformationData = Script + sizeof (EFI_BOOT_SCRIPT_INFORMATION);
  DEBUG ((DEBUG_INFO, "BootScriptExecuteLabel - 0x%08x\n", (UINTN)InformationData));

  DEBUG ((DEBUG_INFO, "BootScriptLabel: "));
  for (Index = 0; Index < Information.InformationLength; Index++) {
    DEBUG ((DEBUG_INFO, "%02x ", InformationData[Index]));
  }

  DEBUG ((DEBUG_INFO, "\n"));
}

/**
  calculate the mask value for 'and' and 'or' operation
  @param ScriptHeader   The pointer of header of node in boot script table
  @param AndMask  The Mask value for 'and' operation
  @param OrMask   The Mask value for 'or' operation
  @param Script   Pointer to the entry.

**/
VOID
CheckAndOrMask (
  IN   EFI_BOOT_SCRIPT_COMMON_HEADER  *ScriptHeader,
  OUT UINT64                          *AndMask,
  OUT UINT64                          *OrMask,
  IN  UINT8                           *Script
  )
{
  UINT8  *DataPtr;
  UINTN  Size;

  switch (ScriptHeader->OpCode) {
    case EFI_BOOT_SCRIPT_IO_READ_WRITE_OPCODE:
      Size = sizeof (EFI_BOOT_SCRIPT_IO_READ_WRITE);
      break;

    case EFI_BOOT_SCRIPT_MEM_READ_WRITE_OPCODE:
      Size = sizeof (EFI_BOOT_SCRIPT_MEM_READ_WRITE);
      break;

    case EFI_BOOT_SCRIPT_PCI_CONFIG_READ_WRITE_OPCODE:
      Size = sizeof (EFI_BOOT_SCRIPT_PCI_CONFIG_READ_WRITE);
      break;
    case EFI_BOOT_SCRIPT_MEM_POLL_OPCODE:
      Size = sizeof (EFI_BOOT_SCRIPT_MEM_POLL);
      break;

    case EFI_BOOT_SCRIPT_IO_POLL_OPCODE:
      Size = sizeof (EFI_BOOT_SCRIPT_IO_POLL);
      break;

    case EFI_BOOT_SCRIPT_PCI_CONFIG2_READ_WRITE_OPCODE:
      Size = sizeof (EFI_BOOT_SCRIPT_PCI_CONFIG2_READ_WRITE);
      break;

    case EFI_BOOT_SCRIPT_PCI_CONFIG2_POLL_OPCODE:
      Size = sizeof (EFI_BOOT_SCRIPT_PCI_CONFIG2_POLL);
      break;

    case EFI_BOOT_SCRIPT_PCI_CONFIG_POLL_OPCODE:
      Size = sizeof (EFI_BOOT_SCRIPT_PCI_CONFIG_POLL);
      break;

    default:
      return;
  }

  DataPtr = Script + Size;

  switch (ScriptHeader->Width) {
    case S3BootScriptWidthUint8:
      *AndMask = (UINT64)(*(UINT8 *)(DataPtr + 1));
      *OrMask  = (UINT64)(*DataPtr);
      break;

    case S3BootScriptWidthUint16:
      *AndMask = (UINT64)(*(UINT16 *)(DataPtr + 2));
      *OrMask  = (UINT64)(*(UINT16 *)DataPtr);
      break;

    case S3BootScriptWidthUint32:
      *AndMask = (UINT64)(*(UINT32 *)(DataPtr + 4));
      *OrMask  = (UINT64)(*(UINT32 *)DataPtr);
      break;

    case S3BootScriptWidthUint64:
      *AndMask = (UINT64)(*(UINT64 *)(DataPtr + 8));
      *OrMask  = (UINT64)(*(UINT64 *)DataPtr);
      break;

    default:
      break;
  }

  return;
}

/**
  Interpret the boot script node with EFI_BOOT_SCRIPT_IO_POLL OP code.

  @param  Script  The pointer of typed node in boot script table
  @param  AndMask  Mask value for 'and' operation
  @param  OrMask   Mask value for 'or' operation

  @retval EFI_DEVICE_ERROR Data polled from memory does not equal to
                           the epecting data within the Loop Times.
  @retval EFI_SUCCESS      The operation was executed successfully
**/
EFI_STATUS
BootScriptExecuteIoPoll (
  IN UINT8   *Script,
  IN UINT64  AndMask,
  IN UINT64  OrMask
  )
{
  EFI_STATUS               Status;
  UINT64                   Data;
  UINT64                   LoopTimes;
  EFI_BOOT_SCRIPT_IO_POLL  IoPoll;

  CopyMem ((VOID *)&IoPoll, (VOID *)Script, sizeof (EFI_BOOT_SCRIPT_IO_POLL));

  DEBUG ((DEBUG_INFO, "BootScriptExecuteIoPoll - 0x%08x, 0x%016lx, 0x%016lx\n", (UINTN)IoPoll.Address, AndMask, OrMask));

  Data   = 0;
  Status = ScriptIoRead (
             (S3_BOOT_SCRIPT_LIB_WIDTH)IoPoll.Width,
             IoPoll.Address,
             1,
             &Data
             );
  if ((!EFI_ERROR (Status)) && ((Data & AndMask) == OrMask)) {
    return EFI_SUCCESS;
  }

  for (LoopTimes = 0; LoopTimes < IoPoll.Delay; LoopTimes++) {
    NanoSecondDelay (100);
    Data   = 0;
    Status = ScriptIoRead (
               (S3_BOOT_SCRIPT_LIB_WIDTH)IoPoll.Width,
               IoPoll.Address,
               1,
               &Data
               );
    if ((!EFI_ERROR (Status)) && ((Data & AndMask) == OrMask)) {
      return EFI_SUCCESS;
    }
  }

  if (LoopTimes < IoPoll.Delay) {
    return EFI_SUCCESS;
  } else {
    return EFI_DEVICE_ERROR;
  }
}

/**
  Interpret the boot script node with EFI_BOOT_SCRIPT_PCI_CONFIG2_WRITE OP code.

  @param    Script              The pointer of S3 boot script

  @retval   EFI_SUCCESS         The operation was executed successfully

**/
EFI_STATUS
BootScriptExecutePciCfg2Write (
  IN UINT8  *Script
  )
{
  VOID                               *Buffer;
  S3_BOOT_SCRIPT_LIB_WIDTH           Width;
  UINT16                             Segment;
  UINT64                             Address;
  UINTN                              Count;
  EFI_BOOT_SCRIPT_PCI_CONFIG2_WRITE  PciCfg2Write;

  CopyMem ((VOID *)&PciCfg2Write, (VOID *)Script, sizeof (EFI_BOOT_SCRIPT_PCI_CONFIG2_WRITE));

  Width   = (S3_BOOT_SCRIPT_LIB_WIDTH)PciCfg2Write.Width;
  Segment = PciCfg2Write.Segment;
  Address = PciCfg2Write.Address;
  Count   = PciCfg2Write.Count;
  Buffer  = Script + sizeof (EFI_BOOT_SCRIPT_PCI_CONFIG2_WRITE);

  DEBUG ((DEBUG_INFO, "BootScriptExecutePciCfg2Write - 0x%016lx, 0x%08x, 0x%08x\n", PCI_ADDRESS_ENCODE (Segment, Address), Count, (UINTN)Width));
  return ScriptPciCfg2Write (Width, Segment, Address, Count, Buffer);
}

/**
  Interpret the boot script node with EFI_BOOT_SCRIPT_PCI_CONFIG2_READ_WRITE OP code.

  @param     Script                     The pointer of S3 boot script
  @param     AndMask                    Mask value for 'and' operation
  @param     OrMask                     Mask value for 'or' operation

  @retval    EFI_SUCCESS                The operation was executed successfully

**/
EFI_STATUS
BootScriptExecutePciCfg2ReadWrite (
  IN UINT8   *Script,
  IN UINT64  AndMask,
  IN UINT64  OrMask
  )
{
  UINT64                                  Data;
  EFI_STATUS                              Status;
  EFI_BOOT_SCRIPT_PCI_CONFIG2_READ_WRITE  PciCfg2ReadWrite;

  Data = 0;

  CopyMem ((VOID *)&PciCfg2ReadWrite, (VOID *)Script, sizeof (EFI_BOOT_SCRIPT_PCI_CONFIG2_READ_WRITE));

  DEBUG ((DEBUG_INFO, "BootScriptExecutePciCfg2ReadWrite - 0x%016lx, 0x%016lx, 0x%016lx\n", PCI_ADDRESS_ENCODE (PciCfg2ReadWrite.Segment, PciCfg2ReadWrite.Address), AndMask, OrMask));

  Status = ScriptPciCfg2Read (
             (S3_BOOT_SCRIPT_LIB_WIDTH)PciCfg2ReadWrite.Width,
             PciCfg2ReadWrite.Segment,
             PciCfg2ReadWrite.Address,
             1,
             &Data
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Data   = (Data & AndMask) | OrMask;
  Status = ScriptPciCfg2Write (
             (S3_BOOT_SCRIPT_LIB_WIDTH)PciCfg2ReadWrite.Width,
             PciCfg2ReadWrite.Segment,
             PciCfg2ReadWrite.Address,
             1,
             &Data
             );
  return Status;
}

/**
  Interpret the boot script node with EFI_BOOT_SCRIPT_PCI_CONFIG_POLL OP code.

  @param     Script                     The pointer of S3 boot script
  @param     AndMask                    Mask value for 'and' operation
  @param     OrMask                     Mask value for 'or' operation

  @retval    EFI_SUCCESS                The operation was executed successfully
  @retval    EFI_DEVICE_ERROR           Data polled from Pci configuration space does not equal to
                                        epecting data within the Loop Times.
**/
EFI_STATUS
BootScriptPciCfgPoll (
  IN UINT8   *Script,
  IN UINT64  AndMask,
  IN UINT64  OrMask
  )
{
  UINT64                           Data;
  UINT64                           LoopTimes;
  EFI_STATUS                       Status;
  EFI_BOOT_SCRIPT_PCI_CONFIG_POLL  PciCfgPoll;

  CopyMem ((VOID *)&PciCfgPoll, (VOID *)Script, sizeof (EFI_BOOT_SCRIPT_PCI_CONFIG_POLL));

  DEBUG ((DEBUG_INFO, "BootScriptPciCfgPoll - 0x%016lx, 0x%016lx, 0x%016lx\n", PCI_ADDRESS_ENCODE (0, PciCfgPoll.Address), AndMask, OrMask));

  Data   = 0;
  Status = ScriptPciCfgRead (
             (S3_BOOT_SCRIPT_LIB_WIDTH)PciCfgPoll.Width,
             PciCfgPoll.Address,
             1,
             &Data
             );
  if ((!EFI_ERROR (Status)) && ((Data & AndMask) == OrMask)) {
    return EFI_SUCCESS;
  }

  for (LoopTimes = 0; LoopTimes < PciCfgPoll.Delay; LoopTimes++) {
    NanoSecondDelay (100);
    Data   = 0;
    Status = ScriptPciCfgRead (
               (S3_BOOT_SCRIPT_LIB_WIDTH)PciCfgPoll.Width,
               PciCfgPoll.Address,
               1,
               &Data
               );
    if ((!EFI_ERROR (Status)) &&
        ((Data & AndMask) == OrMask))
    {
      return EFI_SUCCESS;
    }
  }

  if (LoopTimes < PciCfgPoll.Delay) {
    return EFI_SUCCESS;
  } else {
    return EFI_DEVICE_ERROR;
  }
}

/**
  Interpret the boot script node with EFI_BOOT_SCRIPT_PCI_CONFIG2_POLL OP code.

  @param     Script                     The pointer of S3 Boot Script
  @param     AndMask                    Mask value for 'and' operation
  @param     OrMask                     Mask value for 'or' operation

  @retval    EFI_SUCCESS                The operation was executed successfully
  @retval    EFI_DEVICE_ERROR           Data polled from Pci configuration space does not equal to
                                        epecting data within the Loop Times.

**/
EFI_STATUS
BootScriptPciCfg2Poll (
  IN UINT8   *Script,
  IN UINT64  AndMask,
  IN UINT64  OrMask
  )
{
  EFI_STATUS                        Status;
  UINT64                            Data;
  UINT64                            LoopTimes;
  EFI_BOOT_SCRIPT_PCI_CONFIG2_POLL  PciCfg2Poll;

  Data = 0;
  CopyMem ((VOID *)&PciCfg2Poll, (VOID *)Script, sizeof (EFI_BOOT_SCRIPT_PCI_CONFIG2_POLL));

  DEBUG ((DEBUG_INFO, "BootScriptPciCfg2Poll - 0x%016lx, 0x%016lx, 0x%016lx\n", PCI_ADDRESS_ENCODE (PciCfg2Poll.Segment, PciCfg2Poll.Address), AndMask, OrMask));

  Status = ScriptPciCfg2Read (
             (S3_BOOT_SCRIPT_LIB_WIDTH)PciCfg2Poll.Width,
             PciCfg2Poll.Segment,
             PciCfg2Poll.Address,
             1,
             &Data
             );
  if ((!EFI_ERROR (Status)) && ((Data & AndMask) == OrMask)) {
    return EFI_SUCCESS;
  }

  for (LoopTimes = 0; LoopTimes < PciCfg2Poll.Delay; LoopTimes++) {
    NanoSecondDelay (100);

    Data   = 0;
    Status = ScriptPciCfg2Read (
               (S3_BOOT_SCRIPT_LIB_WIDTH)PciCfg2Poll.Width,
               PciCfg2Poll.Segment,
               PciCfg2Poll.Address,
               1,
               &Data
               );
    if ((!EFI_ERROR (Status)) &&  ((Data & AndMask) == OrMask)) {
      return EFI_SUCCESS;
    }
  }

  if (LoopTimes < PciCfg2Poll.Delay) {
    return EFI_SUCCESS;
  } else {
    return EFI_DEVICE_ERROR;
  }
}

/**
  Executes the S3 boot script table.

  @retval RETURN_SUCCESS           The boot script table was executed successfully.
  @retval RETURN_UNSUPPORTED       Invalid script table or opcode.

**/
RETURN_STATUS
EFIAPI
S3BootScriptExecute (
  VOID
  )
{
  EFI_STATUS                     Status;
  UINT8                          *Script;
  UINTN                          StartAddress;
  UINT32                         TableLength;
  UINT64                         AndMask;
  UINT64                         OrMask;
  EFI_BOOT_SCRIPT_COMMON_HEADER  ScriptHeader;
  EFI_BOOT_SCRIPT_TABLE_HEADER   TableHeader;

  Script = mS3BootScriptTablePtr->TableBase;
  if (Script != 0) {
    CopyMem ((VOID *)&TableHeader, Script, sizeof (EFI_BOOT_SCRIPT_TABLE_HEADER));
  } else {
    return EFI_INVALID_PARAMETER;
  }

  DEBUG ((DEBUG_INFO, "S3BootScriptExecute:\n"));
  if (TableHeader.OpCode != S3_BOOT_SCRIPT_LIB_TABLE_OPCODE) {
    return EFI_UNSUPPORTED;
  }

  DEBUG ((DEBUG_INFO, "TableHeader - 0x%08x\n", Script));

  StartAddress = (UINTN)Script;
  TableLength  = TableHeader.TableLength;
  Script       =    Script + TableHeader.Length;
  Status       = EFI_SUCCESS;
  AndMask      = 0;
  OrMask       = 0;

  DEBUG ((DEBUG_INFO, "TableHeader.Version - 0x%04x\n", (UINTN)TableHeader.Version));
  DEBUG ((DEBUG_INFO, "TableHeader.TableLength - 0x%08x\n", (UINTN)TableLength));

  while ((UINTN)Script < (UINTN)(StartAddress + TableLength)) {
    DEBUG ((DEBUG_INFO, "ExecuteBootScript - %08x\n", (UINTN)Script));

    CopyMem ((VOID *)&ScriptHeader, Script, sizeof (EFI_BOOT_SCRIPT_COMMON_HEADER));
    switch (ScriptHeader.OpCode) {
      case EFI_BOOT_SCRIPT_MEM_WRITE_OPCODE:
        DEBUG ((DEBUG_INFO, "EFI_BOOT_SCRIPT_MEM_WRITE_OPCODE\n"));
        Status = BootScriptExecuteMemoryWrite (Script);
        break;

      case EFI_BOOT_SCRIPT_MEM_READ_WRITE_OPCODE:
        DEBUG ((DEBUG_INFO, "EFI_BOOT_SCRIPT_MEM_READ_WRITE_OPCODE\n"));
        CheckAndOrMask (&ScriptHeader, &AndMask, &OrMask, Script);
        Status = BootScriptExecuteMemoryReadWrite (
                   Script,
                   AndMask,
                   OrMask
                   );
        break;

      case EFI_BOOT_SCRIPT_IO_WRITE_OPCODE:
        DEBUG ((DEBUG_INFO, "EFI_BOOT_SCRIPT_IO_WRITE_OPCODE\n"));
        Status = BootScriptExecuteIoWrite (Script);
        break;

      case EFI_BOOT_SCRIPT_PCI_CONFIG_WRITE_OPCODE:
        DEBUG ((DEBUG_INFO, "EFI_BOOT_SCRIPT_PCI_CONFIG_WRITE_OPCODE\n"));
        Status = BootScriptExecutePciCfgWrite (Script);
        break;

      case EFI_BOOT_SCRIPT_PCI_CONFIG_READ_WRITE_OPCODE:
        DEBUG ((DEBUG_INFO, "EFI_BOOT_SCRIPT_PCI_CONFIG_READ_WRITE_OPCODE\n"));
        CheckAndOrMask (&ScriptHeader, &AndMask, &OrMask, Script);
        Status = BootScriptExecutePciCfgReadWrite (
                   Script,
                   AndMask,
                   OrMask
                   );
        break;
      case EFI_BOOT_SCRIPT_PCI_CONFIG2_WRITE_OPCODE:
        DEBUG ((DEBUG_INFO, "EFI_BOOT_SCRIPT_PCI_CONFIG2_WRITE_OPCODE\n"));
        Status = BootScriptExecutePciCfg2Write (Script);
        break;

      case EFI_BOOT_SCRIPT_PCI_CONFIG2_READ_WRITE_OPCODE:
        DEBUG ((DEBUG_INFO, "EFI_BOOT_SCRIPT_PCI_CONFIG2_READ_WRITE_OPCODE\n"));
        CheckAndOrMask (&ScriptHeader, &AndMask, &OrMask, Script);
        Status = BootScriptExecutePciCfg2ReadWrite (
                   Script,
                   AndMask,
                   OrMask
                   );
        break;
      case EFI_BOOT_SCRIPT_DISPATCH_OPCODE:
        DEBUG ((DEBUG_INFO, "EFI_BOOT_SCRIPT_DISPATCH_OPCODE\n"));
        Status = BootScriptExecuteDispatch (Script);
        break;

      case EFI_BOOT_SCRIPT_DISPATCH_2_OPCODE:
        DEBUG ((DEBUG_INFO, "EFI_BOOT_SCRIPT_DISPATCH_2_OPCODE\n"));
        Status = BootScriptExecuteDispatch2 (Script);
        break;

      case EFI_BOOT_SCRIPT_INFORMATION_OPCODE:
        DEBUG ((DEBUG_INFO, "EFI_BOOT_SCRIPT_INFORMATION_OPCODE\n"));
        BootScriptExecuteInformation (Script);
        break;

      case S3_BOOT_SCRIPT_LIB_TERMINATE_OPCODE:
        DEBUG ((DEBUG_INFO, "S3_BOOT_SCRIPT_LIB_TERMINATE_OPCODE\n"));
        DEBUG ((DEBUG_INFO, "S3BootScriptDone - %r\n", EFI_SUCCESS));
        return EFI_SUCCESS;

      case EFI_BOOT_SCRIPT_IO_READ_WRITE_OPCODE:
        DEBUG ((DEBUG_INFO, "EFI_BOOT_SCRIPT_IO_READ_WRITE_OPCODE\n"));
        CheckAndOrMask (&ScriptHeader, &AndMask, &OrMask, Script);
        Status = BootScriptExecuteIoReadWrite (
                   Script,
                   AndMask,
                   OrMask
                   );
        break;

      case EFI_BOOT_SCRIPT_SMBUS_EXECUTE_OPCODE:
        DEBUG ((DEBUG_INFO, "EFI_BOOT_SCRIPT_SMBUS_EXECUTE_OPCODE\n"));
        Status = BootScriptExecuteSmbusExecute (Script);
        break;

      case EFI_BOOT_SCRIPT_STALL_OPCODE:
        DEBUG ((DEBUG_INFO, "EFI_BOOT_SCRIPT_STALL_OPCODE\n"));
        Status = BootScriptExecuteStall (Script);
        break;

      case EFI_BOOT_SCRIPT_MEM_POLL_OPCODE:
        DEBUG ((DEBUG_INFO, "EFI_BOOT_SCRIPT_MEM_POLL_OPCODE\n"));
        CheckAndOrMask (&ScriptHeader, &AndMask, &OrMask, Script);
        Status = BootScriptExecuteMemPoll (Script, AndMask, OrMask);

        break;

      case EFI_BOOT_SCRIPT_IO_POLL_OPCODE:
        DEBUG ((DEBUG_INFO, "EFI_BOOT_SCRIPT_IO_POLL_OPCODE\n"));
        CheckAndOrMask (&ScriptHeader, &AndMask, &OrMask, Script);
        Status = BootScriptExecuteIoPoll (Script, AndMask, OrMask);
        break;

      case EFI_BOOT_SCRIPT_PCI_CONFIG_POLL_OPCODE:
        DEBUG ((DEBUG_INFO, "EFI_BOOT_SCRIPT_PCI_CONFIG_POLL_OPCODE\n"));
        CheckAndOrMask (&ScriptHeader, &AndMask, &OrMask, Script);
        Status = BootScriptPciCfgPoll (Script, AndMask, OrMask);
        break;

      case EFI_BOOT_SCRIPT_PCI_CONFIG2_POLL_OPCODE:
        DEBUG ((DEBUG_INFO, "EFI_BOOT_SCRIPT_PCI_CONFIG2_POLL_OPCODE\n"));
        CheckAndOrMask (&ScriptHeader, &AndMask, &OrMask, Script);
        Status = BootScriptPciCfg2Poll (Script, AndMask, OrMask);
        break;

      case S3_BOOT_SCRIPT_LIB_LABEL_OPCODE:
        //
        // For label
        //
        DEBUG ((DEBUG_INFO, "S3_BOOT_SCRIPT_LIB_LABEL_OPCODE\n"));
        BootScriptExecuteLabel (Script);
        break;
      default:
        DEBUG ((DEBUG_INFO, "S3BootScriptDone - %r\n", EFI_UNSUPPORTED));
        return EFI_UNSUPPORTED;
    }

    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_INFO, "S3BootScriptDone - %r\n", Status));
      return Status;
    }

    Script = Script + ScriptHeader.Length;
  }

  DEBUG ((DEBUG_INFO, "S3BootScriptDone - %r\n", Status));

  return Status;
}
