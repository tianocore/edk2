/** @file
  Internal include file for the CPU I/O PPI.

Copyright (c) 2009 - 2010, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

**/

#ifndef _CPU_IO2_PEI_H_
#define _CPU_IO2_PEI_H_

#include <PiDxe.h>

#include <Ppi/CpuIo.h>

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/IoLib.h>
#include <Library/PeiServicesLib.h>

#define MAX_IO_PORT_ADDRESS   0xFFFF

/**
  Reads memory-mapped registers.

  @param[in]  PeiServices  An indirect pointer to the PEI Services Table
                           published by the PEI Foundation.
  @param[in]  This         Pointer to local data for the interface.
  @param[in]  Width        The width of the access. Enumerated in bytes.
  @param[in]  Address      The physical address of the access.
  @param[in]  Count        The number of accesses to perform.
  @param[out] Buffer       A pointer to the buffer of data.

  @retval EFI_SUCCESS            The function completed successfully.
  @retval EFI_INVALID_PARAMETER  Width is invalid for this EFI system.
  @retval EFI_INVALID_PARAMETER  Buffer is NULL.
  @retval EFI_UNSUPPORTED        The address range specified by Address, Width, 
                                 and Count is not valid for this EFI system.

**/
EFI_STATUS
EFIAPI
CpuMemoryServiceRead (
  IN  CONST EFI_PEI_SERVICES    **PeiServices,
  IN  CONST EFI_PEI_CPU_IO_PPI  *This,
  IN  EFI_PEI_CPU_IO_PPI_WIDTH  Width,
  IN  UINT64                    Address,
  IN  UINTN                     Count,
  OUT VOID                      *Buffer
  );

/**
  Writes memory-mapped registers.

  @param[in]  PeiServices  An indirect pointer to the PEI Services Table
                           published by the PEI Foundation.
  @param[in]  This         Pointer to local data for the interface.
  @param[in]  Width        The width of the access. Enumerated in bytes.
  @param[in]  Address      The physical address of the access.
  @param[in]  Count        The number of accesses to perform.
  @param[in]  Buffer       A pointer to the buffer of data.

  @retval EFI_SUCCESS            The function completed successfully.
  @retval EFI_INVALID_PARAMETER  Width is invalid for this EFI system.
  @retval EFI_INVALID_PARAMETER  Buffer is NULL.
  @retval EFI_UNSUPPORTED        The address range specified by Address, Width, 
                                 and Count is not valid for this EFI system.

**/
EFI_STATUS
EFIAPI
CpuMemoryServiceWrite (
  IN CONST EFI_PEI_SERVICES    **PeiServices,
  IN CONST EFI_PEI_CPU_IO_PPI  *This,
  IN EFI_PEI_CPU_IO_PPI_WIDTH  Width,
  IN UINT64                    Address,
  IN UINTN                     Count,
  IN VOID                      *Buffer
  );

/**
  Reads I/O registers.

  @param[in]  PeiServices  An indirect pointer to the PEI Services Table
                           published by the PEI Foundation.
  @param[in]  This         Pointer to local data for the interface.
  @param[in]  Width        The width of the access. Enumerated in bytes.
  @param[in]  Address      The physical address of the access.
  @param[in]  Count        The number of accesses to perform.
  @param[out] Buffer       A pointer to the buffer of data.

  @retval EFI_SUCCESS            The function completed successfully.
  @retval EFI_INVALID_PARAMETER  Width is invalid for this EFI system.
  @retval EFI_INVALID_PARAMETER  Buffer is NULL.
  @retval EFI_UNSUPPORTED        The address range specified by Address, Width, 
                                 and Count is not valid for this EFI system.

**/
EFI_STATUS
EFIAPI
CpuIoServiceRead (
  IN  CONST EFI_PEI_SERVICES    **PeiServices,
  IN  CONST EFI_PEI_CPU_IO_PPI  *This,
  IN  EFI_PEI_CPU_IO_PPI_WIDTH  Width,
  IN  UINT64                    Address,
  IN  UINTN                     Count,
  OUT VOID                      *Buffer
  );

/**
  Write I/O registers.

  @param[in]  PeiServices  An indirect pointer to the PEI Services Table
                           published by the PEI Foundation.
  @param[in]  This         Pointer to local data for the interface.
  @param[in]  Width        The width of the access. Enumerated in bytes.
  @param[in]  Address      The physical address of the access.
  @param[in]  Count        The number of accesses to perform.
  @param[in]  Buffer       A pointer to the buffer of data.

  @retval EFI_SUCCESS            The function completed successfully.
  @retval EFI_INVALID_PARAMETER  Width is invalid for this EFI system.
  @retval EFI_INVALID_PARAMETER  Buffer is NULL.
  @retval EFI_UNSUPPORTED        The address range specified by Address, Width, 
                                 and Count is not valid for this EFI system.

**/
EFI_STATUS
EFIAPI
CpuIoServiceWrite (
  IN CONST EFI_PEI_SERVICES    **PeiServices,
  IN CONST EFI_PEI_CPU_IO_PPI  *This,
  IN EFI_PEI_CPU_IO_PPI_WIDTH  Width,
  IN UINT64                    Address,
  IN UINTN                     Count,
  IN VOID                      *Buffer
  );

/**
  8-bit I/O read operations.

  @param[in] PeiServices  An indirect pointer to the PEI Services Table published 
                          by the PEI Foundation.
  @param[in] This         Pointer to local data for the interface.
  @param[in] Address      The physical address of the access.

  @return  An 8-bit value returned from the I/O space.
**/
UINT8
EFIAPI
CpuIoRead8 (
  IN  CONST EFI_PEI_SERVICES      **PeiServices,
  IN  CONST EFI_PEI_CPU_IO_PPI    *This,
  IN  UINT64                      Address
  );

/**
  16-bit I/O read operations.

  @param[in] PeiServices  An indirect pointer to the PEI Services Table published 
                          by the PEI Foundation.
  @param[in] This         Pointer to local data for the interface.
  @param[in] Address      The physical address of the access.

  @return  A 16-bit value returned from the I/O space.

**/
UINT16
EFIAPI
CpuIoRead16 (
  IN  CONST EFI_PEI_SERVICES      **PeiServices,
  IN  CONST EFI_PEI_CPU_IO_PPI    *This,
  IN  UINT64                      Address
  );

/**
  32-bit I/O read operations.

  @param[in] PeiServices  An indirect pointer to the PEI Services Table published 
                          by the PEI Foundation.
  @param[in] This         Pointer to local data for the interface.
  @param[in] Address      The physical address of the access.

  @return  A 32-bit value returned from the I/O space.

**/
UINT32
EFIAPI
CpuIoRead32 (
  IN  CONST EFI_PEI_SERVICES      **PeiServices,
  IN  CONST EFI_PEI_CPU_IO_PPI    *This,
  IN  UINT64                      Address
  );

/**
  64-bit I/O read operations.

  @param[in] PeiServices  An indirect pointer to the PEI Services Table published 
                          by the PEI Foundation.
  @param[in] This         Pointer to local data for the interface.
  @param[in] Address      The physical address of the access.

  @return  A 64-bit value returned from the I/O space.

**/
UINT64
EFIAPI
CpuIoRead64 (
  IN  CONST EFI_PEI_SERVICES      **PeiServices,
  IN  CONST EFI_PEI_CPU_IO_PPI    *This,
  IN  UINT64                      Address
  );

/**
  8-bit I/O write operations.

  @param[in] PeiServices  An indirect pointer to the PEI Services Table published 
                          by the PEI Foundation.
  @param[in] This         Pointer to local data for the interface.
  @param[in] Address      The physical address of the access.
  @param[in] Data         The data to write.

**/
VOID
EFIAPI
CpuIoWrite8 (
  IN  CONST EFI_PEI_SERVICES      **PeiServices,
  IN  CONST EFI_PEI_CPU_IO_PPI    *This,
  IN  UINT64                      Address,
  IN  UINT8                       Data
  );

/**
  16-bit I/O write operations.

  @param[in] PeiServices  An indirect pointer to the PEI Services Table published 
                          by the PEI Foundation.
  @param[in] This         Pointer to local data for the interface.
  @param[in] Address      The physical address of the access.
  @param[in] Data         The data to write.

**/
VOID
EFIAPI
CpuIoWrite16 (
  IN  CONST EFI_PEI_SERVICES      **PeiServices,
  IN  CONST EFI_PEI_CPU_IO_PPI    *This,
  IN  UINT64                      Address,
  IN  UINT16                      Data
  );

/**
  32-bit I/O write operations.

  @param[in] PeiServices  An indirect pointer to the PEI Services Table published 
                          by the PEI Foundation.
  @param[in] This         Pointer to local data for the interface.
  @param[in] Address      The physical address of the access.
  @param[in] Data         The data to write.

**/
VOID
EFIAPI
CpuIoWrite32 (
  IN  CONST EFI_PEI_SERVICES      **PeiServices,
  IN  CONST EFI_PEI_CPU_IO_PPI    *This,
  IN  UINT64                      Address,
  IN  UINT32                      Data
  );

/**
  64-bit I/O write operations.

  @param[in] PeiServices  An indirect pointer to the PEI Services Table published 
                          by the PEI Foundation.
  @param[in] This         Pointer to local data for the interface.
  @param[in] Address      The physical address of the access.
  @param[in] Data         The data to write.

**/
VOID
EFIAPI
CpuIoWrite64 (
  IN  CONST EFI_PEI_SERVICES      **PeiServices,
  IN  CONST EFI_PEI_CPU_IO_PPI    *This,
  IN  UINT64                      Address,
  IN  UINT64                      Data
  );

/**
  8-bit memory read operations.

  @param[in] PeiServices  An indirect pointer to the PEI Services Table published 
                          by the PEI Foundation.
  @param[in] This         Pointer to local data for the interface.
  @param[in] Address      The physical address of the access.

  @return  An 8-bit value returned from the memory space.

**/
UINT8
EFIAPI
CpuMemRead8 (
  IN  CONST EFI_PEI_SERVICES      **PeiServices,
  IN  CONST EFI_PEI_CPU_IO_PPI    *This,
  IN  UINT64                      Address
  );

/**
  16-bit memory read operations.

  @param[in] PeiServices  An indirect pointer to the PEI Services Table published 
                          by the PEI Foundation.
  @param[in] This         Pointer to local data for the interface.
  @param[in] Address      The physical address of the access.

  @return  A 16-bit value returned from the memory space.

**/
UINT16
EFIAPI
CpuMemRead16 (
  IN  CONST EFI_PEI_SERVICES      **PeiServices,
  IN  CONST EFI_PEI_CPU_IO_PPI    *This,
  IN  UINT64                      Address
  );

/**
  32-bit memory read operations.

  @param[in] PeiServices  An indirect pointer to the PEI Services Table published 
                          by the PEI Foundation.
  @param[in] This         Pointer to local data for the interface.
  @param[in] Address      The physical address of the access.

  @return  A 32-bit value returned from the memory space.

**/
UINT32
EFIAPI
CpuMemRead32 (
  IN  CONST EFI_PEI_SERVICES      **PeiServices,
  IN  CONST EFI_PEI_CPU_IO_PPI    *This,
  IN  UINT64                      Address
  );

/**
  64-bit memory read operations.

  @param[in] PeiServices  An indirect pointer to the PEI Services Table published 
                          by the PEI Foundation.
  @param[in] This         Pointer to local data for the interface.
  @param[in] Address      The physical address of the access.

  @return  A 64-bit value returned from the memory space.

**/
UINT64
EFIAPI
CpuMemRead64 (
  IN  CONST EFI_PEI_SERVICES      **PeiServices,
  IN  CONST EFI_PEI_CPU_IO_PPI    *This,
  IN  UINT64                      Address
  );

/**
  8-bit memory write operations.

  @param[in] PeiServices  An indirect pointer to the PEI Services Table published 
                          by the PEI Foundation.
  @param[in] This         Pointer to local data for the interface.
  @param[in] Address      The physical address of the access.
  @param[in] Data         The data to write.

**/
VOID
EFIAPI
CpuMemWrite8 (
  IN  CONST EFI_PEI_SERVICES      **PeiServices,
  IN  CONST EFI_PEI_CPU_IO_PPI    *This,
  IN  UINT64                      Address,
  IN  UINT8                       Data
  );

/**
  16-bit memory write operations.

  @param[in] PeiServices  An indirect pointer to the PEI Services Table published 
                          by the PEI Foundation.
  @param[in] This         Pointer to local data for the interface.
  @param[in] Address      The physical address of the access.
  @param[in] Data         The data to write.

**/
VOID
EFIAPI
CpuMemWrite16 (
  IN  CONST EFI_PEI_SERVICES      **PeiServices,
  IN  CONST EFI_PEI_CPU_IO_PPI    *This,
  IN  UINT64                      Address,
  IN  UINT16                      Data
  );

/**
  32-bit memory write operations.

  @param[in] PeiServices  An indirect pointer to the PEI Services Table published 
                          by the PEI Foundation.
  @param[in] This         Pointer to local data for the interface.
  @param[in] Address      The physical address of the access.
  @param[in] Data         The data to write.

**/
VOID
EFIAPI
CpuMemWrite32 (
  IN  CONST EFI_PEI_SERVICES      **PeiServices,
  IN  CONST EFI_PEI_CPU_IO_PPI    *This,
  IN  UINT64                      Address,
  IN  UINT32                      Data
  );

/**
  64-bit memory write operations.

  @param[in] PeiServices  An indirect pointer to the PEI Services Table published 
                          by the PEI Foundation.
  @param[in] This         Pointer to local data for the interface.
  @param[in] Address      The physical address of the access.
  @param[in] Data         The data to write.

**/
VOID
EFIAPI
CpuMemWrite64 (
  IN  CONST EFI_PEI_SERVICES      **PeiServices,
  IN  CONST EFI_PEI_CPU_IO_PPI    *This,
  IN  UINT64                      Address,
  IN  UINT64                      Data
  );
  
#endif
