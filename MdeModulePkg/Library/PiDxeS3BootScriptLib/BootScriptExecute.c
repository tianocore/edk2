/** @file
  Interpret and execute the S3 data in S3 boot script. 

  Copyright (c) 2006 - 2011, Intel Corporation. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions
  of the BSD License which accompanies this distribution.  The
  full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/
#include "InternalBootScriptLib.h"

/**
  Checks the parameter of SmbusExecute().

  This function checks the input parameters of SmbusExecute().  If the input parameters are valid
  for certain SMBus bus protocol, it will return EFI_SUCCESS; otherwise, it will return certain
  error code based on the input SMBus bus protocol.

  @param  SmBusAddress            Address that encodes the SMBUS Slave Address, SMBUS Command, SMBUS Data Length, 
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

  @retval EFI_SUCCESS             All the parameters are valid for the corresponding SMBus bus
                                  protocol. 
  @retval EFI_INVALID_PARAMETER   Operation is not defined in EFI_SMBUS_OPERATION.
  @retval EFI_INVALID_PARAMETER   Length/Buffer is NULL for operations except for EfiSmbusQuickRead
                                  and EfiSmbusQuickWrite. Length is outside the range of valid
                                  values.
  @retval EFI_UNSUPPORTED         The SMBus operation or PEC is not supported.
  @retval EFI_BUFFER_TOO_SMALL    Buffer is not sufficient for this operation.

**/
EFI_STATUS
CheckParameters (
  IN     UINTN                    SmBusAddress,
  IN     EFI_SMBUS_OPERATION      Operation,
  IN OUT UINTN                    *Length,
  IN OUT VOID                     *Buffer
  )
{
  EFI_STATUS  Status;
  UINTN       RequiredLen;
  EFI_SMBUS_DEVICE_COMMAND Command;
  BOOLEAN                  PecCheck;
 
  Command      = SMBUS_LIB_COMMAND (SmBusAddress);
  PecCheck     = SMBUS_LIB_PEC (SmBusAddress);
  //
  // Set default value to be 2:
  // for SmbusReadWord, SmbusWriteWord and SmbusProcessCall. 
  //
  RequiredLen = 2;
  Status      = EFI_SUCCESS;
  switch (Operation) {
    case EfiSmbusQuickRead:
    case EfiSmbusQuickWrite:
      if (PecCheck || Command != 0) {
        return EFI_UNSUPPORTED;
      }
      break;
    case EfiSmbusReceiveByte:
    case EfiSmbusSendByte:
      if (Command != 0) {
        return EFI_UNSUPPORTED;
      }
      //
      // Cascade to check length parameter.
      //
    case EfiSmbusReadByte:
    case EfiSmbusWriteByte:
      RequiredLen = 1;
      //
      // Cascade to check length parameter.
      //
    case EfiSmbusReadWord:
    case EfiSmbusWriteWord:
    case EfiSmbusProcessCall:
      if (Buffer == NULL || Length == NULL) {
        return EFI_INVALID_PARAMETER;
      } else if (*Length < RequiredLen) {
        Status = EFI_BUFFER_TOO_SMALL;
      }
      *Length = RequiredLen;
      break;
    case EfiSmbusReadBlock:
    case EfiSmbusWriteBlock:
      if ((Buffer == NULL) || 
          (Length == NULL) || 
          (*Length < MIN_SMBUS_BLOCK_LEN) ||
          (*Length > MAX_SMBUS_BLOCK_LEN)) {
        return EFI_INVALID_PARAMETER;
      } 
      break;
    case EfiSmbusBWBRProcessCall:
      return EFI_UNSUPPORTED;
    default:
      return EFI_INVALID_PARAMETER;
  }
  return Status;
}

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
SmbusExecute (
  IN     UINTN                    SmbusAddress,
  IN     EFI_SMBUS_OPERATION      Operation,
  IN OUT UINTN                    *Length,
  IN OUT VOID                     *Buffer
  )
{
  EFI_STATUS                      Status;
  UINTN                           WorkBufferLen;
  UINT8                           WorkBuffer[MAX_SMBUS_BLOCK_LEN];

  Status = CheckParameters (SmbusAddress, Operation, Length, Buffer);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  switch (Operation) {
    case EfiSmbusQuickRead:
      DEBUG ((EFI_D_INFO, "EfiSmbusQuickRead - 0x%08x\n", SmbusAddress));
      SmBusQuickRead (SmbusAddress, &Status);
      break;
    case EfiSmbusQuickWrite:
      DEBUG ((EFI_D_INFO, "EfiSmbusQuickWrite - 0x%08x\n", SmbusAddress));
      SmBusQuickWrite (SmbusAddress, &Status);
      break;
    case EfiSmbusReceiveByte:
      DEBUG ((EFI_D_INFO, "EfiSmbusReceiveByte - 0x%08x\n", SmbusAddress));
      *(UINT8 *) Buffer = SmBusReceiveByte (SmbusAddress, &Status);
      break;
    case EfiSmbusSendByte:
      DEBUG ((EFI_D_INFO, "EfiSmbusReceiveByte - 0x%08x (0x%02x)\n", SmbusAddress, (UINTN)*(UINT8 *) Buffer));
      SmBusSendByte (SmbusAddress, *(UINT8 *) Buffer, &Status);
      break;
    case EfiSmbusReadByte:
      DEBUG ((EFI_D_INFO, "EfiSmbusReadByte - 0x%08x\n", SmbusAddress));
      *(UINT8 *) Buffer = SmBusReadDataByte (SmbusAddress, &Status);
      break;
    case EfiSmbusWriteByte:
      DEBUG ((EFI_D_INFO, "EfiSmbusWriteByte - 0x%08x (0x%02x)\n", SmbusAddress, (UINTN)*(UINT8 *) Buffer));
      SmBusWriteDataByte (SmbusAddress, *(UINT8 *) Buffer, &Status);
      break;
    case EfiSmbusReadWord:
      DEBUG ((EFI_D_INFO, "EfiSmbusReadWord - 0x%08x\n", SmbusAddress));
      *(UINT16 *) Buffer = SmBusReadDataWord (SmbusAddress, &Status);
      break;
    case EfiSmbusWriteWord:
      DEBUG ((EFI_D_INFO, "EfiSmbusWriteByte - 0x%08x (0x%04x)\n", SmbusAddress, (UINTN)*(UINT16 *) Buffer));
      SmBusWriteDataWord (SmbusAddress, *(UINT16 *) Buffer, &Status);
      break;
    case EfiSmbusProcessCall:
      DEBUG ((EFI_D_INFO, "EfiSmbusProcessCall - 0x%08x (0x%04x)\n", SmbusAddress, (UINTN)*(UINT16 *) Buffer));
      *(UINT16 *) Buffer = SmBusProcessCall (SmbusAddress, *(UINT16 *) Buffer, &Status);
      break;
    case EfiSmbusReadBlock:
      DEBUG ((EFI_D_INFO, "EfiSmbusReadBlock - 0x%08x\n", SmbusAddress));
      WorkBufferLen = SmBusReadBlock (SmbusAddress, WorkBuffer, &Status);
      if (!EFI_ERROR (Status)) {
        //
        // Read block transaction is complete successfully, and then
        // check whether the output buffer is large enough.  
        //
        if (*Length >= WorkBufferLen) {
          CopyMem (Buffer, WorkBuffer, WorkBufferLen);
        } else {
          Status = EFI_BUFFER_TOO_SMALL;
        }
        *Length = WorkBufferLen;
      }
      break;
    case EfiSmbusWriteBlock:
      DEBUG ((EFI_D_INFO, "EfiSmbusWriteBlock - 0x%08x (0x%04x)\n", SmbusAddress, (UINTN)*(UINT16 *) Buffer));
      SmBusWriteBlock ((SmbusAddress + SMBUS_LIB_ADDRESS (0, 0, (*Length), FALSE))  , Buffer, &Status);
      break;
    case EfiSmbusBWBRProcessCall:
      //
      // BUGBUG: Should this case be handled?
      //
      break;
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
  IN  S3_BOOT_SCRIPT_LIB_WIDTH      Width,
  IN  UINT64                    Address,
  OUT UINTN                     *AddressStride,
  OUT UINTN                     *BufferStride
  )
{
  UINTN AlignMask;

  if (Width >= S3BootScriptWidthMaximum) {
    return EFI_INVALID_PARAMETER;
  }

  *AddressStride  = (UINT32)(1 << (Width & 0x03));
  *BufferStride   = *AddressStride;

  AlignMask       = *AddressStride - 1;
  if ((Address & AlignMask) != 0) {
    return EFI_INVALID_PARAMETER;
  }

  if (Width >= S3BootScriptWidthFifoUint8 && Width <= S3BootScriptWidthFifoUint64) {
    *AddressStride = 0;
  }

  if (Width >= S3BootScriptWidthFillUint8 && Width <= S3BootScriptWidthFillUint64) {
    *BufferStride = 0;
  }

  return EFI_SUCCESS;
}

/**
  Translates boot script to MDE library interface.
  
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
  IN      S3_BOOT_SCRIPT_LIB_WIDTH Width,
  IN      UINT64                   Address,
  IN      UINTN                    Count,
  OUT     VOID                     *Buffer
  )
{
  EFI_STATUS  Status;
  UINTN       AddressStride;
  UINTN       BufferStride;
  PTR         Out;

  Out.Buf = (UINT8 *) Buffer;

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
  for (; Count > 0; Count--, Address += AddressStride, Out.Buf += BufferStride) {
    switch (Width) {

    case S3BootScriptWidthUint8:
    case S3BootScriptWidthFifoUint8:
    case S3BootScriptWidthFillUint8:
      DEBUG ((EFI_D_INFO, "S3BootScriptWidthUint8Read - 0x%08x\n", Address));
      *Out.Uint8 = IoRead8 ((UINTN) Address);
      break;

    case S3BootScriptWidthUint16:
    case S3BootScriptWidthFifoUint16:
    case S3BootScriptWidthFillUint16:
      DEBUG ((EFI_D_INFO, "S3BootScriptWidthUint16Read - 0x%08x\n", Address));
      *Out.Uint16 = IoRead16 ((UINTN) Address);
      break;

    case S3BootScriptWidthUint32:
    case S3BootScriptWidthFifoUint32:
    case S3BootScriptWidthFillUint32:
      DEBUG ((EFI_D_INFO, "S3BootScriptWidthUint32Read - 0x%08x\n", Address));
      *Out.Uint32 = IoRead32 ((UINTN) Address);
      break;

    default:
      return EFI_INVALID_PARAMETER;
    }
  }

  return EFI_SUCCESS;
}

/**
  Perform a write operation
  
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
  IN      UINT64                 Address,
  IN      UINTN                  Count,
  IN      VOID                   *Buffer
  )
{
  EFI_STATUS  Status;
  UINTN       AddressStride;
  UINTN       BufferStride;
  UINT64      OriginalAddress;
  PTR         In;
  PTR         OriginalIn;

  In.Buf = (UINT8 *) Buffer;

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
  OriginalIn.Buf = In.Buf;
  for (; Count > 0; Count--, Address += AddressStride, In.Buf += BufferStride) {
    switch (Width) {
      case S3BootScriptWidthUint8:
        DEBUG ((EFI_D_INFO, "S3BootScriptWidthUint8 - 0x%08x (0x%02x)\n", (UINTN)Address, (UINTN)*In.Uint8));
        IoWrite8 ((UINTN) Address, *In.Uint8);
        break;      
      case S3BootScriptWidthFifoUint8:
        DEBUG ((EFI_D_INFO, "S3BootScriptWidthFifoUint8 - 0x%08x (0x%02x)\n", (UINTN)OriginalAddress, (UINTN)*In.Uint8));
        IoWrite8 ((UINTN) OriginalAddress, *In.Uint8);
        break;       
      case S3BootScriptWidthFillUint8:
        DEBUG ((EFI_D_INFO, "S3BootScriptWidthFillUint8 - 0x%08x (0x%02x)\n", (UINTN)Address, (UINTN)*In.Uint8));
        IoWrite8 ((UINTN) Address, *OriginalIn.Uint8);
        break;
      case S3BootScriptWidthUint16:
        DEBUG ((EFI_D_INFO, "S3BootScriptWidthUint16 - 0x%08x (0x%04x)\n", (UINTN)Address, (UINTN)*In.Uint16));
        IoWrite16 ((UINTN) Address, *In.Uint16);
        break;      
      case S3BootScriptWidthFifoUint16:
        DEBUG ((EFI_D_INFO, "S3BootScriptWidthFifoUint16 - 0x%08x (0x%04x)\n", (UINTN)OriginalAddress, (UINTN)*In.Uint16));
        IoWrite16 ((UINTN) OriginalAddress, *In.Uint16);
        break;      
      case S3BootScriptWidthFillUint16:
        DEBUG ((EFI_D_INFO, "S3BootScriptWidthFillUint16 - 0x%08x (0x%04x)\n", (UINTN)Address, (UINTN)*In.Uint16));
        IoWrite16 ((UINTN) Address, *OriginalIn.Uint16);
        break;
      case S3BootScriptWidthUint32:
        DEBUG ((EFI_D_INFO, "S3BootScriptWidthUint32 - 0x%08x (0x%08x)\n", (UINTN)Address, (UINTN)*In.Uint32));
        IoWrite32 ((UINTN) Address, *In.Uint32);
        break;      
      case S3BootScriptWidthFifoUint32:
        DEBUG ((EFI_D_INFO, "S3BootScriptWidthFifoUint32 - 0x%08x (0x%08x)\n", (UINTN)OriginalAddress, (UINTN)*In.Uint32));
        IoWrite32 ((UINTN) OriginalAddress, *In.Uint32);
        break;
      case S3BootScriptWidthFillUint32:
        DEBUG ((EFI_D_INFO, "S3BootScriptWidthFillUint32 - 0x%08x (0x%08x)\n", (UINTN)Address, (UINTN)*In.Uint32));
        IoWrite32 ((UINTN) Address, *OriginalIn.Uint32);
        break;
      case S3BootScriptWidthUint64:
        DEBUG ((EFI_D_INFO, "S3BootScriptWidthUint64 - 0x%08x (0x%016lx)\n", (UINTN)Address, *In.Uint64));
        IoWrite64 ((UINTN) Address, *In.Uint64);
        break;      
      case S3BootScriptWidthFifoUint64:
        DEBUG ((EFI_D_INFO, "S3BootScriptWidthFifoUint64 - 0x%08x (0x%016lx)\n", (UINTN)Address, *In.Uint64));
        IoWrite64 ((UINTN) OriginalAddress, *In.Uint64);
        break;      
      case S3BootScriptWidthFillUint64:
        DEBUG ((EFI_D_INFO, "S3BootScriptWidthFillUint64 - 0x%08x (0x%016lx)\n", (UINTN)Address, *In.Uint64));
        IoWrite64 ((UINTN) Address, *OriginalIn.Uint64);
        break;
      default:
        return EFI_INVALID_PARAMETER;
    }
  }
  

  return EFI_SUCCESS;
}
/**
  Interprete the IO write entry in S3 boot script and perform the write operation
  
  @param Script       Pointer to the node which is to be interpreted.

  @retval EFI_SUCCESS The data was written to the EFI System.
  @retval EFI_INVALID_PARAMETER Width is invalid for this EFI System.
                                Buffer is NULL.
                                The Buffer is not aligned for the given Width.
                                Address is outside the legal range of I/O ports.  
                                
**/
EFI_STATUS
BootScriptExecuteIoWrite (
  IN UINT8                    *Script    
  )
{
  S3_BOOT_SCRIPT_LIB_WIDTH   Width;
  UINT64                     Address;
  UINTN                      Count;
  VOID                      *Buffer;
  EFI_BOOT_SCRIPT_IO_WRITE   IoWrite;
  
  CopyMem ((VOID*)&IoWrite, (VOID*)Script, sizeof(EFI_BOOT_SCRIPT_IO_WRITE));
  Width = (S3_BOOT_SCRIPT_LIB_WIDTH) IoWrite.Width;
  Address = IoWrite.Address;
  Count = IoWrite.Count;
  Buffer = Script + sizeof (EFI_BOOT_SCRIPT_IO_WRITE);
  
  return ScriptIoWrite(Width, Address, Count, Buffer);
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
  IN       S3_BOOT_SCRIPT_LIB_WIDTH    Width,
  IN       UINT64                   Address,
  IN       UINTN                    Count,
  IN OUT   VOID                     *Buffer
  )
{
  EFI_STATUS  Status;
  UINTN       AddressStride;
  UINTN       BufferStride;
  PTR         Out;

  Out.Buf = Buffer;

  Status  = BuildLoopData (Width, Address, &AddressStride, &BufferStride);
  if (EFI_ERROR (Status)) {
    return Status;
  }
  //
  // Loop for each iteration and move the data
  //
  for (; Count > 0; Count--, Address += AddressStride, Out.Buf += BufferStride) {
    switch (Width) {
    case S3BootScriptWidthUint8:
    case S3BootScriptWidthFifoUint8:
    case S3BootScriptWidthFillUint8:
      DEBUG ((EFI_D_INFO, "S3BootScriptWidthUint8 - 0x%08x\n", (UINTN)Address));
      *Out.Uint8 = MmioRead8 ((UINTN) Address);
      break;

    case S3BootScriptWidthUint16:
    case S3BootScriptWidthFifoUint16:
    case S3BootScriptWidthFillUint16:
      DEBUG ((EFI_D_INFO, "S3BootScriptWidthUint16 - 0x%08x\n", (UINTN)Address));
      *Out.Uint16 = MmioRead16 ((UINTN) Address);
      break;

    case S3BootScriptWidthUint32:
    case S3BootScriptWidthFifoUint32:
    case S3BootScriptWidthFillUint32:
      DEBUG ((EFI_D_INFO, "S3BootScriptWidthUint32 - 0x%08x\n", (UINTN)Address));
      *Out.Uint32 = MmioRead32 ((UINTN) Address);
      break;

    case S3BootScriptWidthUint64:
    case S3BootScriptWidthFifoUint64:
    case S3BootScriptWidthFillUint64:
      DEBUG ((EFI_D_INFO, "S3BootScriptWidthUint64 - 0x%08x\n", (UINTN)Address));
      *Out.Uint64 = MmioRead64 ((UINTN) Address);
      break;

    default:
      return EFI_UNSUPPORTED;
    }
  }

  return EFI_SUCCESS;
}
/**
  Translates boot script to MDE library interface.
  
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
  IN      S3_BOOT_SCRIPT_LIB_WIDTH Width,
  IN      UINT64                Address,
  IN      UINTN                 Count,
  IN OUT  VOID                 *Buffer
  )
{
  EFI_STATUS  Status;
  UINTN       AddressStride;
  UINT64      OriginalAddress;  
  UINTN       BufferStride;
  PTR         In;
  PTR         OriginalIn;

  In.Buf  = Buffer;

  Status  = BuildLoopData (Width, Address, &AddressStride, &BufferStride);
  if (EFI_ERROR (Status)) {
    return Status;
  }
  //
  // Loop for each iteration and move the data
  //
  OriginalAddress = Address;
  OriginalIn.Buf = In.Buf;  
  for (; Count > 0; Count--, Address += AddressStride, In.Buf += BufferStride) {
    switch (Width) {
      case S3BootScriptWidthUint8:
        DEBUG ((EFI_D_INFO, "S3BootScriptWidthUint8 - 0x%08x (0x%02x)\n", (UINTN)Address, (UINTN)*In.Uint8));
        MmioWrite8 ((UINTN) Address, *In.Uint8);
        break;      
      case S3BootScriptWidthFifoUint8:
        DEBUG ((EFI_D_INFO, "S3BootScriptWidthFifoUint8 - 0x%08x (0x%02x)\n", (UINTN)OriginalAddress, (UINTN)*In.Uint8));
        MmioWrite8 ((UINTN) OriginalAddress, *In.Uint8);
        break;      
      case S3BootScriptWidthFillUint8:
        DEBUG ((EFI_D_INFO, "S3BootScriptWidthFillUint8 - 0x%08x (0x%02x)\n", (UINTN)Address, (UINTN)*In.Uint8));
        MmioWrite8 ((UINTN) Address, *OriginalIn.Uint8);
        break;
      case S3BootScriptWidthUint16:
        DEBUG ((EFI_D_INFO, "S3BootScriptWidthUint16 - 0x%08x (0x%04x)\n", (UINTN)Address, (UINTN)*In.Uint16));
        MmioWrite16 ((UINTN) Address, *In.Uint16);
        break;      
      case S3BootScriptWidthFifoUint16:
        DEBUG ((EFI_D_INFO, "S3BootScriptWidthFifoUint16 - 0x%08x (0x%04x)\n", (UINTN)OriginalAddress, (UINTN)*In.Uint16));
        MmioWrite16 ((UINTN) OriginalAddress, *In.Uint16);
        break;      
      case S3BootScriptWidthFillUint16:
        DEBUG ((EFI_D_INFO, "S3BootScriptWidthFillUint16 - 0x%08x (0x%04x)\n", (UINTN)Address, (UINTN)*In.Uint16));
        MmioWrite16 ((UINTN) Address, *OriginalIn.Uint16);
        break;
      case S3BootScriptWidthUint32:
        DEBUG ((EFI_D_INFO, "S3BootScriptWidthUint32 - 0x%08x (0x%08x)\n", (UINTN)Address, (UINTN)*In.Uint32));
        MmioWrite32 ((UINTN) Address, *In.Uint32);
        break;      
      case S3BootScriptWidthFifoUint32:
        DEBUG ((EFI_D_INFO, "S3BootScriptWidthFifoUint32 - 0x%08x (0x%08x)\n", (UINTN)OriginalAddress, (UINTN)*In.Uint32));
        MmioWrite32 ((UINTN) OriginalAddress, *In.Uint32);
        break;      
      case S3BootScriptWidthFillUint32:
        DEBUG ((EFI_D_INFO, "S3BootScriptWidthFillUint32 - 0x%08x (0x%08x)\n", (UINTN)Address, (UINTN)*In.Uint32));
        MmioWrite32 ((UINTN) Address, *OriginalIn.Uint32);
        break;
      case S3BootScriptWidthUint64:
        DEBUG ((EFI_D_INFO, "S3BootScriptWidthUint64 - 0x%08x (0x%016lx)\n", (UINTN)Address, *In.Uint64));
        MmioWrite64 ((UINTN) Address, *In.Uint64);
        break;      
      case S3BootScriptWidthFifoUint64:
        DEBUG ((EFI_D_INFO, "S3BootScriptWidthFifoUint64 - 0x%08x (0x%016lx)\n", (UINTN)OriginalAddress, *In.Uint64));
        MmioWrite64 ((UINTN) OriginalAddress, *In.Uint64);
        break;      
      case S3BootScriptWidthFillUint64:
        DEBUG ((EFI_D_INFO, "S3BootScriptWidthFillUint64 - 0x%08x (0x%016lx)\n", (UINTN)Address, *In.Uint64));
        MmioWrite64 ((UINTN) Address, *OriginalIn.Uint64);
        break;
      default:
        return EFI_UNSUPPORTED;
    }
  }
  return EFI_SUCCESS;
}
/**
  Interprete the boot script node with EFI_BOOT_SCRIPT_MEM_WRITE OP code.

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
  IN UINT8             *Script
  )
{
  VOID            *Buffer;
  S3_BOOT_SCRIPT_LIB_WIDTH Width;
  UINT64           Address;
  UINTN            Count;
  EFI_BOOT_SCRIPT_MEM_WRITE  MemWrite;
  
  CopyMem((VOID*)&MemWrite, (VOID*)Script, sizeof(EFI_BOOT_SCRIPT_MEM_WRITE));
  Width   = (S3_BOOT_SCRIPT_LIB_WIDTH)MemWrite.Width;
  Address = MemWrite.Address;
  Count   = MemWrite.Count;
  Buffer  = Script + sizeof(EFI_BOOT_SCRIPT_MEM_WRITE);

  DEBUG ((EFI_D_INFO, "BootScriptExecuteMemoryWrite - 0x%08x, 0x%08x, 0x%08x\n", (UINTN)Address, (UINTN)Count, (UINTN)Width));
  return ScriptMemoryWrite (Width,Address, Count,  Buffer);
  
}  
/**
  Translates boot script to MDE library interface for PCI configuration read operation

  @param  Width   Width of the operation.
  @param  Address Address of the operation.
  @param  Buffer  Pointer to the buffer reaf from PCI config space
  
  @retval EFI_SUCCESS The read succeed.
  @retval EFI_INVALID_PARAMETER if Width is not defined  
                                
**/
EFI_STATUS
ScriptPciCfgRead (
  IN  S3_BOOT_SCRIPT_LIB_WIDTH    Width,
  IN  UINT64                       Address,
  OUT VOID                        *Buffer
  )
{
    switch (Width) {
    case S3BootScriptWidthUint8:
      DEBUG ((EFI_D_INFO, "S3BootScriptWidthUint8 - 0x%016lx\n", Address));
      * (UINT8 *) Buffer = PciRead8 (PCI_ADDRESS_ENCODE(Address));
      break;

    case S3BootScriptWidthUint16:
      DEBUG ((EFI_D_INFO, "S3BootScriptWidthUint16 - 0x%016lx\n", Address));
      * (UINT16 *) Buffer = PciRead16 (PCI_ADDRESS_ENCODE(Address));
      break;

    case S3BootScriptWidthUint32:
      DEBUG ((EFI_D_INFO, "S3BootScriptWidthUint32 - 0x%016lx\n", Address));
      * (UINT32 *) Buffer = PciRead32 (PCI_ADDRESS_ENCODE(Address));
      break;

    default:
      return EFI_INVALID_PARAMETER;
  }
    return EFI_SUCCESS;
}

/**
  Translates boot script to MDE library interface for PCI configuration write operation

  @param  Width   Width of the operation.
  @param  Address Address of the operation.
  @param  Buffer  Pointer to the buffer reaf from PCI config space
  
  @retval EFI_SUCCESS The write succeed.
  @retval EFI_INVALID_PARAMETER if Width is not defined  
                                
**/
EFI_STATUS
ScriptPciCfgWrite (
  IN  S3_BOOT_SCRIPT_LIB_WIDTH     Width,
  IN  UINT64                       Address,
  OUT VOID                         *Buffer
  )
{
  switch (Width) {
    case S3BootScriptWidthUint8:
    case S3BootScriptWidthFifoUint8:
    case S3BootScriptWidthFillUint8:      
      DEBUG ((EFI_D_INFO, "S3BootScriptWidthUint8 - 0x%016lx (0x%02x)\n", Address, (UINTN)*(UINT8 *) Buffer));
      PciWrite8 (PCI_ADDRESS_ENCODE(Address), *(UINT8 *) Buffer);
      break;
    case S3BootScriptWidthUint16:
    case S3BootScriptWidthFifoUint16:
    case S3BootScriptWidthFillUint16:       
      DEBUG ((EFI_D_INFO, "S3BootScriptWidthUint16 - 0x%016lx (0x%04x)\n", Address, (UINTN)*(UINT16 *) Buffer));
      PciWrite16 (PCI_ADDRESS_ENCODE(Address), *(UINT16 *) Buffer);
      break;
    case S3BootScriptWidthUint32:
    case S3BootScriptWidthFifoUint32:
    case S3BootScriptWidthFillUint32:       
      DEBUG ((EFI_D_INFO, "S3BootScriptWidthUint32 - 0x%016lx (0x%08x)\n", Address, (UINTN)*(UINT32 *) Buffer));
      PciWrite32 (PCI_ADDRESS_ENCODE(Address), *(UINT32 *) Buffer);
      break;
    default:
      return EFI_INVALID_PARAMETER;
  }
    return EFI_SUCCESS;
}
/**
  Perform pci configure 2 read operation.
  
  @param     Width                      Width of the operation.
  @param     Segment                    Pci segment number
  @param     Address                    Address of the operation.
  @param     Buffer                     Pointer to the buffer to write to I/O space.  

  @retval    EFI_SUCCESS                The data was written to the EFI System.
  @retval    EFI_INVALID_PARAMETER      Width is invalid for this EFI System.
                                        Buffer is NULL.
                                        The Buffer is not aligned for the given Width.
                                        Address is outside the legal range of I/O ports.
  @note  A known Limitations in the implementation which is the 'Segment' parameter is assumed as 
         Zero, or else, assert.
**/
EFI_STATUS
ScriptPciCfg2Read (
  IN  S3_BOOT_SCRIPT_LIB_WIDTH    Width,
  IN  UINT16                   Segment,  
  IN  UINT64                   Address,
  OUT VOID                     *Buffer
  )
{
  ASSERT (Segment==0);
  
  return ScriptPciCfgRead (Width, Address, Buffer);
}
/**
  Perform pci configure write operation.
  
  @param     Width                      Width of the operation.
  @param     Segment                    Pci segment number
  @param     Address                    Address of the operation.
  @param     Buffer                     Pointer to the buffer to write to I/O space.  

  @retval    EFI_SUCCESS                The data was written to the EFI System.
  @retval    EFI_INVALID_PARAMETER      Width is invalid for this EFI System.
                                        Buffer is NULL.
                                        The Buffer is not aligned for the given Width.
                                        Address is outside the legal range of I/O ports.
  @note  A known Limitations in the implementation which is the 'Segment' parameter is assumed as 
         Zero, or else, assert.
                                
**/
EFI_STATUS
EFIAPI
ScriptPciCfg2Write (
  IN  S3_BOOT_SCRIPT_LIB_WIDTH    Width,
  IN  UINT16                   Segment,  
  IN  UINT64                   Address,
  OUT VOID                     *Buffer
  )
{
  ASSERT (Segment==0);
  return ScriptPciCfgWrite (Width, Address, Buffer);
}
/**
  Perform Pci configuration Write operation.
  
  @param  Script        The pointer of typed node in boot script table 
  
  @retval EFI_SUCCESS  The operation was executed successfully
**/
EFI_STATUS
BootScriptExecutePciCfgWrite (
  IN UINT8                    *Script
  )
{
  EFI_STATUS  Status;
  UINT8       *Buffer;
  UINTN       DataWidth;
  UINTN       Index;
  UINT64      PciAddress;
  UINT8       Reg;
  EFI_BOOT_SCRIPT_PCI_CONFIG_WRITE  PciConfigWrite;

  CopyMem ((VOID*)&PciConfigWrite, (VOID*)Script, sizeof(EFI_BOOT_SCRIPT_PCI_CONFIG_WRITE));
  Status      = EFI_SUCCESS;

  PciAddress  = PciConfigWrite.Address;
  DataWidth   = (UINT32)(0x01 << (PciConfigWrite.Width));
  Buffer      = Script + sizeof (EFI_BOOT_SCRIPT_PCI_CONFIG_WRITE);

  DEBUG ((EFI_D_INFO, "BootScriptExecutePciCfgWrite - 0x%08x, 0x%08x, 0x%08x\n", (UINTN)PciAddress, (UINTN)PciConfigWrite.Count, (UINTN)DataWidth));

  for (Index = 0; Index < PciConfigWrite.Count; Index++) {
    Status = ScriptPciCfgWrite (
               (S3_BOOT_SCRIPT_LIB_WIDTH) PciConfigWrite.Width,
               PciAddress,
               Buffer
               );

   if ( S3BootScriptWidthFillUint8 !=  PciConfigWrite.Width ||
        S3BootScriptWidthFillUint16 != PciConfigWrite.Width || 
        S3BootScriptWidthFillUint32 != PciConfigWrite.Width ||
        S3BootScriptWidthFillUint64 != PciConfigWrite.Width){
      Reg         = (UINT8) ((UINT8) PciAddress + DataWidth);
      PciAddress  = (PciAddress & 0xFFFFFFFFFFFFFF00ULL) + Reg;
    }

    if (S3BootScriptWidthFifoUint8 != PciConfigWrite.Width ||
        S3BootScriptWidthFifoUint16 != PciConfigWrite.Width || 
        S3BootScriptWidthFifoUint32 != PciConfigWrite.Width ||
        S3BootScriptWidthFifoUint64 != PciConfigWrite.Width) {
      Buffer += DataWidth;
    }
  }

  return Status;
}
/**
  Excute the script to perform IO modification operation.

  @param Script   The pointer of typed node in boot script table 
  @param AndMask  Mask value for 'and' operation
  @param OrMask   Mask value for 'or' operation

  @retval EFI_SUCCESS    The operation was executed successfully
**/
EFI_STATUS
BootScriptExecuteIoReadWrite (
  IN  UINT8                        *Script,
  IN  UINT64                       AndMask,
  IN  UINT64                        OrMask
  )

{
  EFI_STATUS  Status;
  UINT64      Data;
  EFI_BOOT_SCRIPT_IO_READ_WRITE IoReadWrite;
  
  Data = 0;
  
  CopyMem((VOID*)&IoReadWrite, (VOID*)Script, sizeof(EFI_BOOT_SCRIPT_IO_READ_WRITE));

  DEBUG ((EFI_D_INFO, "BootScriptExecuteIoReadWrite - 0x%08x, 0x%016lx, 0x%016lx\n", (UINTN)IoReadWrite.Address, (UINT64)AndMask, (UINT64)OrMask));

  Status = ScriptIoRead (
             (S3_BOOT_SCRIPT_LIB_WIDTH) IoReadWrite.Width,
             IoReadWrite.Address,
             1,
             &Data
             );
  if (!EFI_ERROR (Status)) {
    Data = (Data & AndMask) | OrMask;
    Status = ScriptIoWrite (
               (S3_BOOT_SCRIPT_LIB_WIDTH) IoReadWrite.Width,
               IoReadWrite.Address,
               1,
               &Data
               );
  }
  return Status;
}
/**
  Excute the script to perform memory modification operation.

  @param Script    The pointer of typed node in boot script table 
  @param AndMask   Mask value for 'and' operation
  @param OrMask    Mask value for 'or' operation

  @retval EFI_SUCCESS The operation was executed successfully
**/
EFI_STATUS
BootScriptExecuteMemoryReadWrite (
  IN UINT8                        *Script,
  IN UINT64                        AndMask,
  IN UINT64                        OrMask
  )

{
  EFI_STATUS  Status;
  UINT64      Data;
  EFI_BOOT_SCRIPT_MEM_READ_WRITE  MemReadWrite;
  
  Data = 0;
  
  CopyMem((VOID*)&MemReadWrite, (VOID*)Script, sizeof(EFI_BOOT_SCRIPT_MEM_READ_WRITE));

  DEBUG ((EFI_D_INFO, "BootScriptExecuteMemoryReadWrite - 0x%08x, 0x%016lx, 0x%016lx\n", (UINTN)MemReadWrite.Address, (UINT64)AndMask, (UINT64)OrMask));
  
  Status = ScriptMemoryRead (
             (S3_BOOT_SCRIPT_LIB_WIDTH) MemReadWrite.Width,
             MemReadWrite.Address,
             1,
             &Data
             );
  if (!EFI_ERROR (Status)) {
    Data = (Data & AndMask) | OrMask;
    Status = ScriptMemoryWrite (
               (S3_BOOT_SCRIPT_LIB_WIDTH) MemReadWrite.Width,
               MemReadWrite.Address,
               1,
               &Data
               );
  }
  return Status;
}
/**
  Excute the script to perform PCI IO modification operation.

  @param Script   The pointer of typed node in boot script table 
  @param AndMask  Mask value for 'and' operation
  @param OrMask   Mask value for 'or' operation

  @retval EFI_SUCCESS   The operation was executed successfully
**/
EFI_STATUS
BootScriptExecutePciCfgReadWrite (
  IN UINT8                     *Script,
  IN UINT64                    AndMask,
  IN UINT64                    OrMask
  )

{
  EFI_STATUS  Status;
  UINT64      Data;
  EFI_BOOT_SCRIPT_PCI_CONFIG_READ_WRITE  PciCfgReadWrite;
  
  CopyMem((VOID*)&PciCfgReadWrite, (VOID*)Script, sizeof(EFI_BOOT_SCRIPT_PCI_CONFIG_READ_WRITE));

  DEBUG ((EFI_D_INFO, "BootScriptExecutePciCfgReadWrite - 0x%08x, 0x%016lx, 0x%016lx\n", (UINTN)PciCfgReadWrite.Address, (UINT64)AndMask, (UINT64)OrMask));
  
  Status = ScriptPciCfgRead (
             (S3_BOOT_SCRIPT_LIB_WIDTH) PciCfgReadWrite.Width,
             PciCfgReadWrite.Address,
             &Data
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Data = (Data & AndMask) | OrMask;

  Status = ScriptPciCfgWrite (
             (S3_BOOT_SCRIPT_LIB_WIDTH) PciCfgReadWrite.Width,
             PciCfgReadWrite.Address,
             &Data
             );

  return Status;
}
/**
  To Execute SMBUS command. 

  @param Script  The pointer of typed node in boot script table 
 
  @retval EFI_SUCCESS      The operation was executed successfully
  @retval EFI_UNSUPPORTED  Cannot locate smbus ppi or occur error of script execution
  @retval Others           Result of script execution 
**/
EFI_STATUS
BootScriptExecuteSmbusExecute (
  IN UINT8                     *Script
  )
{
  UINTN                    SmBusAddress;
  UINTN                    DataSize;
  EFI_BOOT_SCRIPT_SMBUS_EXECUTE SmbusExecuteEntry;
  
  CopyMem ((VOID*)&SmbusExecuteEntry, (VOID*)Script, sizeof(EFI_BOOT_SCRIPT_SMBUS_EXECUTE ));

  DEBUG ((EFI_D_INFO, "BootScriptExecuteSmbusExecute - 0x%08x, 0x%08x\n", (UINTN)SmbusExecuteEntry.SmBusAddress, (UINTN)SmbusExecuteEntry.Operation));

  SmBusAddress = (UINTN)SmbusExecuteEntry.SmBusAddress;
  DataSize = (UINTN) SmbusExecuteEntry.DataSize;
  return SmbusExecute (
           SmBusAddress,
           (EFI_SMBUS_OPERATION) SmbusExecuteEntry.Operation,
           &DataSize,
           Script + sizeof (EFI_BOOT_SCRIPT_SMBUS_EXECUTE)
           );
}
/**
  Execute stall operation in boot script table.

  @param Script      The pointer of typed node in boot script table 
  
  @retval EFI_SUCCESS The operation was executed successfully
**/
EFI_STATUS
BootScriptExecuteStall (
  IN UINT8                 *Script
  )
{
  EFI_BOOT_SCRIPT_STALL    Stall;
  
  CopyMem ((VOID*)&Stall, (VOID*)Script, sizeof(EFI_BOOT_SCRIPT_STALL));

  DEBUG ((EFI_D_INFO, "BootScriptExecuteStall - 0x%08x\n", (UINTN)Stall.Duration));

  MicroSecondDelay ((UINTN) Stall.Duration);
  return EFI_SUCCESS;
}
/**
 To execute assigned function.
  
 @param Script  The pointer of typed node in boot script table 
 @retval EFI_SUCCESS  The operation was executed successfully
**/
EFI_STATUS
BootScriptExecuteDispatch (
  IN UINT8                  *Script
  )
{
  EFI_STATUS                Status;
  DISPATCH_ENTRYPOINT_FUNC  EntryFunc;
  EFI_BOOT_SCRIPT_DISPATCH  ScriptDispatch;
  
  CopyMem ((VOID*)&ScriptDispatch, (VOID*)Script, sizeof(EFI_BOOT_SCRIPT_DISPATCH));
  EntryFunc = (DISPATCH_ENTRYPOINT_FUNC) (UINTN) (ScriptDispatch.EntryPoint);

  DEBUG ((EFI_D_INFO, "BootScriptExecuteDispatch - 0x%08x\n", (UINTN)ScriptDispatch.EntryPoint));

  Status    = EntryFunc (NULL, NULL);

  return Status;
}
/**
  Execute dispach2 opertion code which is to invoke a spcified function with one parameter. 

  @param  Script       The pointer of typed node in boot script table 
  @retval EFI_SUCCESS  The operation was executed successfully
**/
EFI_STATUS
BootScriptExecuteDispatch2 (
  IN UINT8                  *Script
  )
{
  EFI_STATUS                Status;
  DISPATCH_ENTRYPOINT_FUNC  EntryFunc;
  EFI_BOOT_SCRIPT_DISPATCH_2  ScriptDispatch2;
  
  CopyMem ((VOID*)&ScriptDispatch2, (VOID*)Script, sizeof(EFI_BOOT_SCRIPT_DISPATCH_2));

  DEBUG ((EFI_D_INFO, "BootScriptExecuteDispatch2 - 0x%08x(0x%08x)\n", (UINTN)ScriptDispatch2.EntryPoint, (UINTN)ScriptDispatch2.Context));
  
  EntryFunc = (DISPATCH_ENTRYPOINT_FUNC) (UINTN) (ScriptDispatch2.EntryPoint);

  Status    = EntryFunc (NULL, (VOID *) (UINTN) ScriptDispatch2.Context);

  return Status;
}
/**
  Excute the script to poll memory.

  @param  Script  The pointer of typed node in boot script table 
  @param  AndMask  Mask value for 'and' operation
  @param  OrMask   Mask value for 'or' operation
  
  @retval EFI_DEVICE_ERROR Data polled from memory does not equal to 
                           the epecting data within the Loop Times.
  @retval EFI_SUCCESS      The operation was executed successfully
**/
EFI_STATUS
BootScriptExecuteMemPoll (
  IN UINT8                        *Script,
  IN UINT64                        AndMask,
  IN UINT64                        OrMask  
  )
{
  
  UINT64        Data;
  UINT64        LoopTimes;
  EFI_STATUS    Status;
  EFI_BOOT_SCRIPT_MEM_POLL       MemPoll;
  
  CopyMem ((VOID*)&MemPoll, (VOID*)Script, sizeof(EFI_BOOT_SCRIPT_MEM_POLL));

  DEBUG ((EFI_D_INFO, "BootScriptExecuteMemPoll - 0x%08x\n", (UINTN)MemPoll.Address));

  Data = 0;
  Status = ScriptMemoryRead (
               (S3_BOOT_SCRIPT_LIB_WIDTH) MemPoll.Width,
               MemPoll.Address,
               1,
               &Data
               );
  if ((!EFI_ERROR (Status)) && (Data & AndMask) == OrMask) {
    return EFI_SUCCESS;
  }

  for (LoopTimes = 0; LoopTimes < MemPoll.LoopTimes; LoopTimes++) {
    NanoSecondDelay ((UINTN)MemPoll.Duration);

    Data = 0;
    Status = ScriptMemoryRead (
               (S3_BOOT_SCRIPT_LIB_WIDTH) MemPoll.Width,
               MemPoll.Address,
               1,
               &Data
               );
   if ((!EFI_ERROR (Status)) && (Data & AndMask) == OrMask) {
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
  Execute the boot script to interpret the  Store arbitrary information. 
  This opcode is a no-op on dispatch and is only used for debugging script issues.

  @param Script       The pointer of node in boot script table 
 
**/
VOID
BootScriptExecuteInformation (
  IN UINT8       *Script
  )

{
  UINT32                        Index;
  EFI_BOOT_SCRIPT_INFORMATION   Information;

  CopyMem ((VOID*)&Information, (VOID*)Script, sizeof(Information));

  DEBUG ((EFI_D_INFO, "BootScriptExecuteInformation - 0x%08x\n", (UINTN)Information.Information));

  DEBUG ((EFI_D_INFO, "BootScriptInformation: "));
  for (Index = 0; Index < Information.InformationLength; Index++) {
    DEBUG ((EFI_D_INFO, "%02x ", *(UINT8 *)(UINTN)(Information.Information + Index)));
  }
  DEBUG ((EFI_D_INFO, "\n"));
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
  OUT UINT64                      *AndMask,
  OUT UINT64                      *OrMask,
  IN  UINT8                       *Script
  )
{
  UINT8 *DataPtr;
  UINTN Size;

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
    *AndMask  = (UINT64) *(DataPtr + 1);
    *OrMask   = (UINT64) (*DataPtr);
    break;

  case S3BootScriptWidthUint16:
    *AndMask  = (UINT64) (*(UINT16 *) (DataPtr + 2));
    *OrMask   = (UINT64) (*(UINT16 *) DataPtr);
    break;

  case S3BootScriptWidthUint32:
    *AndMask  = (UINT64) (*(UINT32 *) (DataPtr + 4));
    *OrMask   = (UINT64) (*(UINT32 *) DataPtr);
    break;

  case S3BootScriptWidthUint64:
    *AndMask  = (UINT64) (*(UINT64 *) (DataPtr + 8));
    *OrMask   = (UINT64) (*(UINT64 *) DataPtr);
    break;

  default:
    break;
  }

  return;
}
/**
  Excute the script to poll Io port for some time

  @param  Script  The pointer of typed node in boot script table 
  @param  AndMask  Mask value for 'and' operation
  @param  OrMask   Mask value for 'or' operation
  
  @retval EFI_DEVICE_ERROR Data polled from memory does not equal to 
                           the epecting data within the Loop Times.
  @retval EFI_SUCCESS      The operation was executed successfully
**/
EFI_STATUS
BootScriptExecuteIoPoll (
  IN UINT8       *Script,
  IN UINT64      AndMask,
  IN UINT64      OrMask
  )
{
  EFI_STATUS    Status;
  UINT64        Data;
  UINT64        LoopTimes;
  EFI_BOOT_SCRIPT_IO_POLL       IoPoll;
  
  CopyMem ((VOID*)&IoPoll, (VOID*)Script, sizeof(EFI_BOOT_SCRIPT_IO_POLL));

  DEBUG ((EFI_D_INFO, "BootScriptExecuteIoPoll - 0x%08x\n", (UINTN)IoPoll.Address));

  Data = 0;
  Status = ScriptIoRead (
             (S3_BOOT_SCRIPT_LIB_WIDTH) IoPoll.Width,
             IoPoll.Address,
             1,
             &Data
             );
  if ((!EFI_ERROR (Status)) && (Data & AndMask) == OrMask) {
    return EFI_SUCCESS;
  }
  for (LoopTimes = 0; LoopTimes < IoPoll.Delay; LoopTimes++) {
    NanoSecondDelay (100);
    Data = 0;
    Status = ScriptIoRead (
               (S3_BOOT_SCRIPT_LIB_WIDTH) IoPoll.Width,
               IoPoll.Address,
               1,
               &Data
               );
    if ((!EFI_ERROR (Status)) &&(Data & AndMask) == OrMask) {
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
  Perform Pci configuration Write operation.

  @param    Script              The pointer of S3 boot script

  @retval   EFI_SUCCESS         The operation was executed successfully

**/
EFI_STATUS
BootScriptExecutePciCfg2Write (
  IN UINT8             *Script
  )
{
  UINT8       Reg;
  UINT8       *Buffer;
  UINTN       DataWidth;
  UINTN       Index;
  UINT16      Segment;
  UINT64      PciAddress;
  EFI_STATUS  Status;
  EFI_BOOT_SCRIPT_PCI_CONFIG2_WRITE  PciCfg2Write;
  
  CopyMem ((VOID*)&PciCfg2Write, (VOID*)Script, sizeof(EFI_BOOT_SCRIPT_PCI_CONFIG2_WRITE));
  Status      = EFI_SUCCESS;
  Segment     = PciCfg2Write.Segment;
  PciAddress  = PciCfg2Write.Address;
  DataWidth   = (UINT32)(0x01 << (PciCfg2Write.Width));
  Buffer      = Script + sizeof (EFI_BOOT_SCRIPT_PCI_CONFIG2_WRITE);

  DEBUG ((EFI_D_INFO, "BootScriptExecutePciCfg2Write - 0x%08x\n", (UINTN)PciAddress));

  for (Index = 0; Index < PciCfg2Write.Count; Index++) {
    Status = ScriptPciCfg2Write (
               (S3_BOOT_SCRIPT_LIB_WIDTH) PciCfg2Write.Width,
               Segment,
               PciAddress,
               Buffer
               );
    if (S3BootScriptWidthFillUint8  != PciCfg2Write.Width ||
        S3BootScriptWidthFillUint16 != PciCfg2Write.Width || 
        S3BootScriptWidthFillUint32 != PciCfg2Write.Width ||
        S3BootScriptWidthFillUint64 != PciCfg2Write.Width){
      Reg         = (UINT8) ((UINT8) PciAddress + DataWidth);
      PciAddress  = (PciAddress & 0xFFFFFFFFFFFFFF00ULL) + Reg;
    }

    if (S3BootScriptWidthFifoUint8  != PciCfg2Write.Width ||
        S3BootScriptWidthFifoUint16 != PciCfg2Write.Width || 
        S3BootScriptWidthFifoUint32 != PciCfg2Write.Width ||
        S3BootScriptWidthFifoUint64 != PciCfg2Write.Width) {
      Buffer += DataWidth;
    }
  }
  return Status;
}


/**
  Perform pci configuration read & Write operation.
  
  @param     Script                     The pointer of S3 boot script
  @param     AndMask                    Mask value for 'and' operation
  @param     OrMask                     Mask value for 'or' operation

  @retval    EFI_SUCCESS                The operation was executed successfully

**/
EFI_STATUS
BootScriptExecutePciCfg2ReadWrite (
  IN UINT8                        *Script,
  IN UINT64                        AndMask,
  IN UINT64                        OrMask
  )
{
  UINT64      Data;
  EFI_STATUS  Status;
  EFI_BOOT_SCRIPT_PCI_CONFIG2_READ_WRITE PciCfg2ReadWrite;
  CopyMem ((VOID*)&PciCfg2ReadWrite, (VOID*)Script, sizeof(EFI_BOOT_SCRIPT_PCI_CONFIG2_READ_WRITE));

  DEBUG ((EFI_D_INFO, "BootScriptExecutePciCfg2ReadWrite - 0x%08x\n", (UINTN)PciCfg2ReadWrite.Address));
  
  Status = ScriptPciCfg2Read (
             (S3_BOOT_SCRIPT_LIB_WIDTH) PciCfg2ReadWrite.Width,
             PciCfg2ReadWrite.Segment,
             PciCfg2ReadWrite.Address,
             &Data
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Data = (Data & AndMask) | OrMask;
  Status = ScriptPciCfg2Write (
             (S3_BOOT_SCRIPT_LIB_WIDTH) PciCfg2ReadWrite.Width,
             PciCfg2ReadWrite.Segment,
             PciCfg2ReadWrite.Address,
             &Data
             );
  return Status;
}
/**
  To perform poll pci configure operation.
  
  @param     Script                     The pointer of S3 boot script
  @param     AndMask                    Mask value for 'and' operation
  @param     OrMask                     Mask value for 'or' operation

  @retval    EFI_SUCCESS                The operation was executed successfully
  @retval    EFI_DEVICE_ERROR           Data polled from Pci configuration space does not equal to 
                                        epecting data within the Loop Times.
**/
EFI_STATUS
BootScriptPciCfgPoll (
  IN UINT8                         *Script,
  IN UINT64                        AndMask,
  IN UINT64                        OrMask  
  )
{
  UINT64        Data;
  UINT64        LoopTimes;
  EFI_STATUS    Status;
  EFI_BOOT_SCRIPT_PCI_CONFIG_POLL PciCfgPoll;
  CopyMem ((VOID*)&PciCfgPoll, (VOID*)Script, sizeof(EFI_BOOT_SCRIPT_PCI_CONFIG_POLL));

  DEBUG ((EFI_D_INFO, "BootScriptPciCfgPoll - 0x%08x\n", (UINTN)PciCfgPoll.Address));
  
  Data = 0;
  Status = ScriptPciCfgRead (
             (S3_BOOT_SCRIPT_LIB_WIDTH) PciCfgPoll.Width,
             PciCfgPoll.Address,
             &Data
             );
  if ((!EFI_ERROR (Status)) &&(Data & AndMask) == OrMask) {
    return EFI_SUCCESS;
  }

  for (LoopTimes = 0; LoopTimes < PciCfgPoll.Delay; LoopTimes++) {
    NanoSecondDelay (100);
    Data = 0;
    Status = ScriptPciCfgRead (
               (S3_BOOT_SCRIPT_LIB_WIDTH) PciCfgPoll.Width,
               PciCfgPoll.Address,
               &Data
               );
    if ((!EFI_ERROR (Status)) &&
       (Data & AndMask) == OrMask) {
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
  To perform poll pci configure operation.
  
  @param     Script                     The pointer of S3 Boot Script
  @param     AndMask                    Mask value for 'and' operation
  @param     OrMask                     Mask value for 'or' operation

  @retval    EFI_SUCCESS                The operation was executed successfully
  @retval    EFI_DEVICE_ERROR           Data polled from Pci configuration space does not equal to 
                                        epecting data within the Loop Times.

**/
EFI_STATUS
BootScriptPciCfg2Poll (
  IN UINT8                        *Script,
  IN UINT64                        AndMask,
  IN UINT64                        OrMask  
  )
{
  EFI_STATUS    Status;
  UINT64        Data;
  UINT64        LoopTimes;
  EFI_BOOT_SCRIPT_PCI_CONFIG2_POLL PciCfg2Poll;

  Data = 0;
  CopyMem ((VOID*)&PciCfg2Poll, (VOID*)Script, sizeof(EFI_BOOT_SCRIPT_PCI_CONFIG2_POLL));

  DEBUG ((EFI_D_INFO, "BootScriptPciCfg2Poll - 0x%08x\n", (UINTN)PciCfg2Poll.Address));
 
  Status = ScriptPciCfg2Read (
             (S3_BOOT_SCRIPT_LIB_WIDTH) PciCfg2Poll.Width,
             PciCfg2Poll.Segment,
             PciCfg2Poll.Address,
             &Data
             );
  if ((!EFI_ERROR (Status)) && (Data & AndMask) == OrMask) {
    return EFI_SUCCESS;
  }

  for (LoopTimes = 0; LoopTimes < PciCfg2Poll.Delay; LoopTimes++) {
    NanoSecondDelay (100);

    Data = 0;
    Status = ScriptPciCfg2Read (
               (S3_BOOT_SCRIPT_LIB_WIDTH) PciCfg2Poll.Width,
               PciCfg2Poll.Segment,               
               PciCfg2Poll.Address,
               &Data
               );
    if ((!EFI_ERROR (Status)) &&  (Data & AndMask) == OrMask) {
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
  
  @note  A known Limitations in the implementation: When interpreting the opcode  EFI_BOOT_SCRIPT_PCI_CONFIG2_WRITE_OPCODE
         EFI_BOOT_SCRIPT_PCI_CONFIG2_READ_WRITE_OPCODE and EFI_BOOT_SCRIPT_PCI_CONFIG2_POLL_OPCODE, the 'Segment' parameter is assumed as 
         Zero, or else, assert.
**/
RETURN_STATUS
EFIAPI
S3BootScriptExecute (
  VOID
  )
{
  EFI_STATUS            Status;
  UINT8*                Script;
  UINTN                 StartAddress;
  UINT32                TableLength;
  UINT64                AndMask;
  UINT64                OrMask;
  EFI_BOOT_SCRIPT_COMMON_HEADER  ScriptHeader;
  EFI_BOOT_SCRIPT_TABLE_HEADER   TableHeader;
  Script = mS3BootScriptTablePtr->TableBase;
  if (Script != 0) {    
    CopyMem ((VOID*)&TableHeader, Script, sizeof(EFI_BOOT_SCRIPT_TABLE_HEADER));
  } else {
    return EFI_INVALID_PARAMETER;
  }

  DEBUG ((EFI_D_INFO, "S3BootScriptExecute:\n"));
  if (TableHeader.OpCode != S3_BOOT_SCRIPT_LIB_TABLE_OPCODE) {
    return EFI_UNSUPPORTED;
  }

  DEBUG ((EFI_D_INFO, "TableHeader - 0x%08x\n", Script));
  
  StartAddress  = (UINTN) Script;
  TableLength   = TableHeader.TableLength;
  Script    =    Script + TableHeader.Length;
  Status        = EFI_SUCCESS;
  AndMask       = 0;
  OrMask        = 0;

  DEBUG ((EFI_D_INFO, "TableHeader.TableLength - 0x%08x\n", (UINTN)TableLength));

  while ((UINTN) Script < (UINTN) (StartAddress + TableLength)) {
    DEBUG ((EFI_D_INFO, "ExecuteBootScript - %08x\n", (UINTN)Script));
    
    CopyMem ((VOID*)&ScriptHeader, Script, sizeof(EFI_BOOT_SCRIPT_COMMON_HEADER));
    switch (ScriptHeader.OpCode) {

    case EFI_BOOT_SCRIPT_MEM_WRITE_OPCODE:
      DEBUG ((EFI_D_INFO, "EFI_BOOT_SCRIPT_MEM_WRITE_OPCODE\n"));
      Status = BootScriptExecuteMemoryWrite (Script);
      break;

    case EFI_BOOT_SCRIPT_MEM_READ_WRITE_OPCODE:
      DEBUG ((EFI_D_INFO, "EFI_BOOT_SCRIPT_MEM_READ_WRITE_OPCODE\n"));
      CheckAndOrMask (&ScriptHeader, &AndMask, &OrMask, Script);
      Status = BootScriptExecuteMemoryReadWrite (
                 Script,
                 AndMask,
                 OrMask
                 );
      break;

    case EFI_BOOT_SCRIPT_IO_WRITE_OPCODE:
      DEBUG ((EFI_D_INFO, "EFI_BOOT_SCRIPT_IO_WRITE_OPCODE\n"));
      Status = BootScriptExecuteIoWrite (Script);
      break;

    case EFI_BOOT_SCRIPT_PCI_CONFIG_WRITE_OPCODE:
      DEBUG ((EFI_D_INFO, "EFI_BOOT_SCRIPT_PCI_CONFIG_WRITE_OPCODE\n"));
      Status = BootScriptExecutePciCfgWrite (Script);
      break;

    case EFI_BOOT_SCRIPT_PCI_CONFIG_READ_WRITE_OPCODE:
      DEBUG ((EFI_D_INFO, "EFI_BOOT_SCRIPT_PCI_CONFIG_READ_WRITE_OPCODE\n"));
      CheckAndOrMask (&ScriptHeader, &AndMask, &OrMask, Script);
      Status = BootScriptExecutePciCfgReadWrite (
                 Script,
                 AndMask,
                 OrMask
                 );
      break;
    case EFI_BOOT_SCRIPT_PCI_CONFIG2_WRITE_OPCODE:
      DEBUG ((EFI_D_INFO, "EFI_BOOT_SCRIPT_PCI_CONFIG2_WRITE_OPCODE\n"));
      Status = BootScriptExecutePciCfg2Write (Script);
      break;

    case EFI_BOOT_SCRIPT_PCI_CONFIG2_READ_WRITE_OPCODE:
      DEBUG ((EFI_D_INFO, "EFI_BOOT_SCRIPT_PCI_CONFIG2_READ_WRITE_OPCODE\n"));
      CheckAndOrMask (&ScriptHeader, &AndMask, &OrMask, Script);
      Status = BootScriptExecutePciCfg2ReadWrite (
                 Script,
                 AndMask,
                 OrMask
                 );
      break;
    case EFI_BOOT_SCRIPT_DISPATCH_OPCODE:
      DEBUG ((EFI_D_INFO, "EFI_BOOT_SCRIPT_DISPATCH_OPCODE\n"));
      Status = BootScriptExecuteDispatch (Script);
      break;

    case EFI_BOOT_SCRIPT_DISPATCH_2_OPCODE:
      DEBUG ((EFI_D_INFO, "EFI_BOOT_SCRIPT_DISPATCH_2_OPCODE\n"));
      Status = BootScriptExecuteDispatch2 (Script);
      break;

    case EFI_BOOT_SCRIPT_INFORMATION_OPCODE:
      DEBUG ((EFI_D_INFO, "EFI_BOOT_SCRIPT_INFORMATION_OPCODE\n"));
      BootScriptExecuteInformation (Script);
      break;    

    case S3_BOOT_SCRIPT_LIB_TERMINATE_OPCODE:
      DEBUG ((EFI_D_INFO, "S3_BOOT_SCRIPT_LIB_TERMINATE_OPCODE\n"));
      return EFI_SUCCESS;

    case EFI_BOOT_SCRIPT_IO_READ_WRITE_OPCODE:
      DEBUG ((EFI_D_INFO, "EFI_BOOT_SCRIPT_IO_READ_WRITE_OPCODE\n"));
      CheckAndOrMask (&ScriptHeader, &AndMask, &OrMask, Script);
      Status = BootScriptExecuteIoReadWrite (
                Script,
                AndMask,
                OrMask
                );
      break;

    case EFI_BOOT_SCRIPT_SMBUS_EXECUTE_OPCODE:
      DEBUG ((EFI_D_INFO, "EFI_BOOT_SCRIPT_SMBUS_EXECUTE_OPCODE\n"));
      Status = BootScriptExecuteSmbusExecute (Script);
      break;

    case EFI_BOOT_SCRIPT_STALL_OPCODE:
      DEBUG ((EFI_D_INFO, "EFI_BOOT_SCRIPT_STALL_OPCODE\n"));
      Status = BootScriptExecuteStall (Script);
      break;

    case EFI_BOOT_SCRIPT_MEM_POLL_OPCODE:
      DEBUG ((EFI_D_INFO, "EFI_BOOT_SCRIPT_MEM_POLL_OPCODE\n"));
      CheckAndOrMask (&ScriptHeader, &AndMask, &OrMask, Script);
      Status = BootScriptExecuteMemPoll (Script, AndMask, OrMask);
      
      break;
    
    case EFI_BOOT_SCRIPT_IO_POLL_OPCODE:
      DEBUG ((EFI_D_INFO, "EFI_BOOT_SCRIPT_IO_POLL_OPCODE\n"));
      CheckAndOrMask (&ScriptHeader, &AndMask, &OrMask, Script);
      Status = BootScriptExecuteIoPoll (Script, AndMask, OrMask);
      break;
      
    case EFI_BOOT_SCRIPT_PCI_CONFIG_POLL_OPCODE:
      DEBUG ((EFI_D_INFO, "EFI_BOOT_SCRIPT_PCI_CONFIG_POLL_OPCODE\n"));
      CheckAndOrMask (&ScriptHeader, &AndMask, &OrMask, Script);
      Status = BootScriptPciCfgPoll (Script, AndMask, OrMask);
      break;
      
    case EFI_BOOT_SCRIPT_PCI_CONFIG2_POLL_OPCODE:
     DEBUG ((EFI_D_INFO, "EFI_BOOT_SCRIPT_PCI_CONFIG2_POLL_OPCODE\n"));
     CheckAndOrMask (&ScriptHeader, &AndMask, &OrMask, Script);
     Status = BootScriptPciCfg2Poll (Script, AndMask, OrMask);
     break;

    case S3_BOOT_SCRIPT_LIB_LABEL_OPCODE:
      //
      // For label
      //
      DEBUG ((EFI_D_INFO, "S3_BOOT_SCRIPT_LIB_LABEL_OPCODE\n"));
      break;
    default:
      DEBUG ((EFI_D_INFO, "S3BootScriptDone - %r\n", EFI_UNSUPPORTED));
      return EFI_UNSUPPORTED;
    }

    if (EFI_ERROR (Status)) {
      DEBUG ((EFI_D_INFO, "S3BootScriptDone - %r\n", Status));
      return Status;
    }

    Script  = Script + ScriptHeader.Length;
  }

  DEBUG ((EFI_D_INFO, "S3BootScriptDone - %r\n", Status));

  return Status;
}

