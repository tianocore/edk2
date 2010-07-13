/** @file
  Internal include file for the CPU I/O 2 Protocol.

Copyright (c) 2009 - 2010, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

**/

#ifndef _CPU_IO2_DXE_H_
#define _CPU_IO2_DXE_H_

#include <PiDxe.h>

#include <Protocol/CpuIo2.h>

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/IoLib.h>
#include <Library/UefiBootServicesTableLib.h>

#define MAX_IO_PORT_ADDRESS   0xFFFF

/**
  Reads memory-mapped registers.

  The I/O operations are carried out exactly as requested. The caller is responsible 
  for satisfying any alignment and I/O width restrictions that a PI System on a 
  platform might require. For example on some platforms, width requests of 
  EfiCpuIoWidthUint64 do not work. Misaligned buffers, on the other hand, will 
  be handled by the driver.
  
  If Width is EfiCpuIoWidthUint8, EfiCpuIoWidthUint16, EfiCpuIoWidthUint32, 
  or EfiCpuIoWidthUint64, then both Address and Buffer are incremented for 
  each of the Count operations that is performed.
  
  If Width is EfiCpuIoWidthFifoUint8, EfiCpuIoWidthFifoUint16, 
  EfiCpuIoWidthFifoUint32, or EfiCpuIoWidthFifoUint64, then only Buffer is 
  incremented for each of the Count operations that is performed. The read or 
  write operation is performed Count times on the same Address.
  
  If Width is EfiCpuIoWidthFillUint8, EfiCpuIoWidthFillUint16, 
  EfiCpuIoWidthFillUint32, or EfiCpuIoWidthFillUint64, then only Address is 
  incremented for each of the Count operations that is performed. The read or 
  write operation is performed Count times from the first element of Buffer.
  
  @param[in]  This     A pointer to the EFI_CPU_IO2_PROTOCOL instance.
  @param[in]  Width    Signifies the width of the I/O or Memory operation.
  @param[in]  Address  The base address of the I/O operation. 
  @param[in]  Count    The number of I/O operations to perform. The number of 
                       bytes moved is Width size * Count, starting at Address.
  @param[out] Buffer   For read operations, the destination buffer to store the results.
                       For write operations, the source buffer from which to write data.

  @retval EFI_SUCCESS            The data was read from or written to the PI system.
  @retval EFI_INVALID_PARAMETER  Width is invalid for this PI system.
  @retval EFI_INVALID_PARAMETER  Buffer is NULL.
  @retval EFI_UNSUPPORTED        The Buffer is not aligned for the given Width.
  @retval EFI_UNSUPPORTED        The address range specified by Address, Width, 
                                 and Count is not valid for this PI system.

**/
EFI_STATUS
EFIAPI
CpuMemoryServiceRead (
  IN  EFI_CPU_IO2_PROTOCOL       *This,
  IN  EFI_CPU_IO_PROTOCOL_WIDTH  Width,
  IN  UINT64                     Address,
  IN  UINTN                      Count,
  OUT VOID                       *Buffer
  );

/**
  Writes memory-mapped registers.

  The I/O operations are carried out exactly as requested. The caller is responsible 
  for satisfying any alignment and I/O width restrictions that a PI System on a 
  platform might require. For example on some platforms, width requests of 
  EfiCpuIoWidthUint64 do not work. Misaligned buffers, on the other hand, will 
  be handled by the driver.
  
  If Width is EfiCpuIoWidthUint8, EfiCpuIoWidthUint16, EfiCpuIoWidthUint32, 
  or EfiCpuIoWidthUint64, then both Address and Buffer are incremented for 
  each of the Count operations that is performed.
  
  If Width is EfiCpuIoWidthFifoUint8, EfiCpuIoWidthFifoUint16, 
  EfiCpuIoWidthFifoUint32, or EfiCpuIoWidthFifoUint64, then only Buffer is 
  incremented for each of the Count operations that is performed. The read or 
  write operation is performed Count times on the same Address.
  
  If Width is EfiCpuIoWidthFillUint8, EfiCpuIoWidthFillUint16, 
  EfiCpuIoWidthFillUint32, or EfiCpuIoWidthFillUint64, then only Address is 
  incremented for each of the Count operations that is performed. The read or 
  write operation is performed Count times from the first element of Buffer.
  
  @param[in]  This     A pointer to the EFI_CPU_IO2_PROTOCOL instance.
  @param[in]  Width    Signifies the width of the I/O or Memory operation.
  @param[in]  Address  The base address of the I/O operation. 
  @param[in]  Count    The number of I/O operations to perform. The number of 
                       bytes moved is Width size * Count, starting at Address.
  @param[in]  Buffer   For read operations, the destination buffer to store the results.
                       For write operations, the source buffer from which to write data.

  @retval EFI_SUCCESS            The data was read from or written to the PI system.
  @retval EFI_INVALID_PARAMETER  Width is invalid for this PI system.
  @retval EFI_INVALID_PARAMETER  Buffer is NULL.
  @retval EFI_UNSUPPORTED        The Buffer is not aligned for the given Width.
  @retval EFI_UNSUPPORTED        The address range specified by Address, Width, 
                                 and Count is not valid for this PI system.

**/
EFI_STATUS
EFIAPI
CpuMemoryServiceWrite (
  IN EFI_CPU_IO2_PROTOCOL       *This,
  IN EFI_CPU_IO_PROTOCOL_WIDTH  Width,
  IN UINT64                     Address,
  IN UINTN                      Count,
  IN VOID                       *Buffer
  );

/**
  Reads I/O registers.

  The I/O operations are carried out exactly as requested. The caller is responsible 
  for satisfying any alignment and I/O width restrictions that a PI System on a 
  platform might require. For example on some platforms, width requests of 
  EfiCpuIoWidthUint64 do not work. Misaligned buffers, on the other hand, will 
  be handled by the driver.
  
  If Width is EfiCpuIoWidthUint8, EfiCpuIoWidthUint16, EfiCpuIoWidthUint32, 
  or EfiCpuIoWidthUint64, then both Address and Buffer are incremented for 
  each of the Count operations that is performed.
  
  If Width is EfiCpuIoWidthFifoUint8, EfiCpuIoWidthFifoUint16, 
  EfiCpuIoWidthFifoUint32, or EfiCpuIoWidthFifoUint64, then only Buffer is 
  incremented for each of the Count operations that is performed. The read or 
  write operation is performed Count times on the same Address.
  
  If Width is EfiCpuIoWidthFillUint8, EfiCpuIoWidthFillUint16, 
  EfiCpuIoWidthFillUint32, or EfiCpuIoWidthFillUint64, then only Address is 
  incremented for each of the Count operations that is performed. The read or 
  write operation is performed Count times from the first element of Buffer.
  
  @param[in]  This     A pointer to the EFI_CPU_IO2_PROTOCOL instance.
  @param[in]  Width    Signifies the width of the I/O or Memory operation.
  @param[in]  Address  The base address of the I/O operation. 
  @param[in]  Count    The number of I/O operations to perform. The number of 
                       bytes moved is Width size * Count, starting at Address.
  @param[out] Buffer   For read operations, the destination buffer to store the results.
                       For write operations, the source buffer from which to write data.

  @retval EFI_SUCCESS            The data was read from or written to the PI system.
  @retval EFI_INVALID_PARAMETER  Width is invalid for this PI system.
  @retval EFI_INVALID_PARAMETER  Buffer is NULL.
  @retval EFI_UNSUPPORTED        The Buffer is not aligned for the given Width.
  @retval EFI_UNSUPPORTED        The address range specified by Address, Width, 
                                 and Count is not valid for this PI system.

**/
EFI_STATUS
EFIAPI
CpuIoServiceRead (
  IN  EFI_CPU_IO2_PROTOCOL       *This,
  IN  EFI_CPU_IO_PROTOCOL_WIDTH  Width,
  IN  UINT64                     Address,
  IN  UINTN                      Count,
  OUT VOID                       *Buffer
  );

/**
  Write I/O registers.

  The I/O operations are carried out exactly as requested. The caller is responsible 
  for satisfying any alignment and I/O width restrictions that a PI System on a 
  platform might require. For example on some platforms, width requests of 
  EfiCpuIoWidthUint64 do not work. Misaligned buffers, on the other hand, will 
  be handled by the driver.
  
  If Width is EfiCpuIoWidthUint8, EfiCpuIoWidthUint16, EfiCpuIoWidthUint32, 
  or EfiCpuIoWidthUint64, then both Address and Buffer are incremented for 
  each of the Count operations that is performed.
  
  If Width is EfiCpuIoWidthFifoUint8, EfiCpuIoWidthFifoUint16, 
  EfiCpuIoWidthFifoUint32, or EfiCpuIoWidthFifoUint64, then only Buffer is 
  incremented for each of the Count operations that is performed. The read or 
  write operation is performed Count times on the same Address.
  
  If Width is EfiCpuIoWidthFillUint8, EfiCpuIoWidthFillUint16, 
  EfiCpuIoWidthFillUint32, or EfiCpuIoWidthFillUint64, then only Address is 
  incremented for each of the Count operations that is performed. The read or 
  write operation is performed Count times from the first element of Buffer.
  
  @param[in]  This     A pointer to the EFI_CPU_IO2_PROTOCOL instance.
  @param[in]  Width    Signifies the width of the I/O or Memory operation.
  @param[in]  Address  The base address of the I/O operation. 
  @param[in]  Count    The number of I/O operations to perform. The number of 
                       bytes moved is Width size * Count, starting at Address.
  @param[in]  Buffer   For read operations, the destination buffer to store the results.
                       For write operations, the source buffer from which to write data.

  @retval EFI_SUCCESS            The data was read from or written to the PI system.
  @retval EFI_INVALID_PARAMETER  Width is invalid for this PI system.
  @retval EFI_INVALID_PARAMETER  Buffer is NULL.
  @retval EFI_UNSUPPORTED        The Buffer is not aligned for the given Width.
  @retval EFI_UNSUPPORTED        The address range specified by Address, Width, 
                                 and Count is not valid for this PI system.
                                 
**/
EFI_STATUS
EFIAPI
CpuIoServiceWrite (
  IN EFI_CPU_IO2_PROTOCOL       *This,
  IN EFI_CPU_IO_PROTOCOL_WIDTH  Width,
  IN UINT64                     Address,
  IN UINTN                      Count,
  IN VOID                       *Buffer
  );

#endif
