/** @file
  Interface Definitions for I2C Lib.
  
  Copyright (c) 2004  - 2015, Intel Corporation. All rights reserved.<BR>
                                                                                   
  SPDX-License-Identifier: BSD-2-Clause-Patent
                                                                               
--*/

#include <Uefi.h>
#include <Library/IoLib.h>

#ifndef I2C_LIB_HEADER_H
#define I2C_LIB_HEADER_H


/**
  Reads a Byte from I2C Device.
 
  @param  I2cControllerIndex   I2C Bus no to which the I2C device has been connected
  @param  SlaveAddress         Device Address from which the byte value has to be read
  @param  Offset               Offset from which the data has to be read
  @param  ReadBytes            Number of bytes to be read
  @param  *ReadBuffer          Address to which the value read has to be stored
                                
  @return  EFI_SUCCESS       If the byte value has been successfully read
  @return  EFI_DEVICE_ERROR  Operation Failed, Device Error
**/
EFI_STATUS 
ByteReadI2C(
  IN  UINT8 BusNo, 
  IN  UINT8 SlaveAddress, 
  IN  UINT8 Offset,  
  IN  UINTN ReadBytes,
  OUT UINT8 *ReadBuffer
  );

/**
  Writes a Byte to I2C Device.
 
  @param  I2cControllerIndex  I2C Bus no to which the I2C device has been connected
  @param  SlaveAddress        Device Address from which the byte value has to be written
  @param  Offset              Offset from which the data has to be written
  @param  WriteBytes          Number of bytes to be written
  @param  *Byte               Address to which the value written is stored
                                
  @return  EFI_SUCCESS       If the byte value has been successfully read
  @return  EFI_DEVICE_ERROR  Operation Failed, Device Error
**/  
EFI_STATUS ByteWriteI2C(
  IN  UINT8 BusNo, 
  IN  UINT8 SlaveAddress,
  IN  UINT8 Offset,
  IN  UINTN WriteBytes,
  IN  UINT8 *WriteBuffer
  );

#endif
