/** @file
  This file declares CPU IO PPI that abstracts CPU IO access

  Copyright (c) 2006, Intel Corporation                                                         
  All rights reserved. This program and the accompanying materials                          
  are licensed and made available under the terms and conditions of the BSD License         
  which accompanies this distribution.  The full text of the license may be found at        
  http://opensource.org/licenses/bsd-license.php                                            

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

  @par Revision Reference:
  This PPI is defined in PI.
  Version 1.0.
  
**/

#ifndef __PEI_CPUIO_PPI_H__
#define __PEI_CPUIO_PPI_H__

#define EFI_PEI_CPU_IO_PPI_INSTALLED_GUID \
  { 0xe6af1f7b, 0xfc3f, 0x46da, {0xa8, 0x28, 0xa3, 0xb4, 0x57, 0xa4, 0x42, 0x82 } }

typedef struct _EFI_PEI_CPU_IO_PPI  EFI_PEI_CPU_IO_PPI;

//
// *******************************************************
// EFI_PEI_CPU_IO_PPI_WIDTH
// *******************************************************
//
typedef enum {
  EfiPeiCpuIoWidthUint8,
  EfiPeiCpuIoWidthUint16,
  EfiPeiCpuIoWidthUint32,
  EfiPeiCpuIoWidthUint64,
  EfiPeiCpuIoWidthFifoUint8,
  EfiPeiCpuIoWidthFifoUint16,
  EfiPeiCpuIoWidthFifoUint32,
  EfiPeiCpuIoWidthFifoUint64,
  EfiPeiCpuIoWidthFillUint8,
  EfiPeiCpuIoWidthFillUint16,
  EfiPeiCpuIoWidthFillUint32,
  EfiPeiCpuIoWidthFillUint64,
  EfiPeiCpuIoWidthMaximum
} EFI_PEI_CPU_IO_PPI_WIDTH;

/**
  Memory-based access services and I/O-based access services.

  @param  PeiServices    An indirect pointer to the PEI Services Table published by the PEI Foundation.
  @param  This           Pointer to local data for the interface.
  @param  Width          The width of the access. Enumerated in bytes.
  @param  Address        The physical address of the access.
  @param  Count          The number of accesses to perform.
  @param  Buffer         A pointer to the buffer of data.

  @retval EFI_SUCCESS           The function completed successfully.
  @retval EFI_NOT_YET_AVAILABLE The service has not been installed.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_PEI_CPU_IO_PPI_IO_MEM) (
  IN  EFI_PEI_SERVICES                  **PeiServices,
  IN  EFI_PEI_CPU_IO_PPI                *This,
  IN  EFI_PEI_CPU_IO_PPI_WIDTH          Width,
  IN  UINT64                            Address,
  IN  UINTN                             Count,
  IN  OUT VOID                          *Buffer
  );

//
// *******************************************************
// EFI_PEI_CPU_IO_PPI_ACCESS
// *******************************************************
//
typedef struct {
  EFI_PEI_CPU_IO_PPI_IO_MEM Read;
  EFI_PEI_CPU_IO_PPI_IO_MEM Write;
} EFI_PEI_CPU_IO_PPI_ACCESS;

/**
  8-bit I/O read operations.

  @param  PeiServices    An indirect pointer to the PEI Services Table published by the PEI Foundation.
  @param  This           Pointer to local data for the interface.
  @param  Address        The physical address of the access.

  @return UINT8

**/
typedef
UINT8
(EFIAPI *EFI_PEI_CPU_IO_PPI_IO_READ8) (
  IN  EFI_PEI_SERVICES        **PeiServices,
  IN  EFI_PEI_CPU_IO_PPI      *This,
  IN  UINT64                  Address
  );

/**
  16-bit I/O read operations.

  @param  PeiServices    An indirect pointer to the PEI Services Table published by the PEI Foundation.
  @param  This           Pointer to local data for the interface.
  @param  Address        The physical address of the access.

  @return UINT16

**/
typedef
UINT16
(EFIAPI *EFI_PEI_CPU_IO_PPI_IO_READ16) (
  IN  EFI_PEI_SERVICES        **PeiServices,
  IN  EFI_PEI_CPU_IO_PPI      *This,
  IN  UINT64                  Address
  );

/**
  32-bit I/O read operations.

  @param  PeiServices    An indirect pointer to the PEI Services Table published by the PEI Foundation.
  @param  This           Pointer to local data for the interface.
  @param  Address        The physical address of the access.

  @return UINT32

**/
typedef
UINT32
(EFIAPI *EFI_PEI_CPU_IO_PPI_IO_READ32) (
  IN  EFI_PEI_SERVICES        **PeiServices,
  IN  EFI_PEI_CPU_IO_PPI      *This,
  IN  UINT64                  Address
  );

/**
  64-bit I/O read operations.

  @param  PeiServices    An indirect pointer to the PEI Services Table published by the PEI Foundation.
  @param  This           Pointer to local data for the interface.
  @param  Address        The physical address of the access.

  @return UINT64

**/
typedef
UINT64
(EFIAPI *EFI_PEI_CPU_IO_PPI_IO_READ64) (
  IN  EFI_PEI_SERVICES        **PeiServices,
  IN  EFI_PEI_CPU_IO_PPI      *This,
  IN  UINT64                  Address
  );

/**
  8-bit I/O write operations.

  @param  PeiServices    An indirect pointer to the PEI Services Table published by the PEI Foundation.
  @param  This           Pointer to local data for the interface.
  @param  Address        The physical address of the access.
  @param  Data           The data to write.

  @return None

**/
typedef
VOID
(EFIAPI *EFI_PEI_CPU_IO_PPI_IO_WRITE8) (
  IN  EFI_PEI_SERVICES        **PeiServices,
  IN  EFI_PEI_CPU_IO_PPI      *This,
  IN  UINT64                  Address,
  IN  UINT8                   Data
  );

/**
  16-bit I/O write operations.

  @param  PeiServices    An indirect pointer to the PEI Services Table published by the PEI Foundation.
  @param  This           Pointer to local data for the interface.
  @param  Address        The physical address of the access.
  @param  Data           The data to write.

  @return None

**/
typedef
VOID
(EFIAPI *EFI_PEI_CPU_IO_PPI_IO_WRITE16) (
  IN  EFI_PEI_SERVICES        **PeiServices,
  IN  EFI_PEI_CPU_IO_PPI      *This,
  IN  UINT64                  Address,
  IN  UINT16                  Data
  );

/**
  32-bit I/O write operations.

  @param  PeiServices    An indirect pointer to the PEI Services Table published by the PEI Foundation.
  @param  This           Pointer to local data for the interface.
  @param  Address        The physical address of the access.
  @param  Data           The data to write.

  @return None

**/
typedef
VOID
(EFIAPI *EFI_PEI_CPU_IO_PPI_IO_WRITE32) (
  IN  EFI_PEI_SERVICES        **PeiServices,
  IN  EFI_PEI_CPU_IO_PPI      *This,
  IN  UINT64                  Address,
  IN  UINT32                  Data
  );

/**
  64-bit I/O write operations.

  @param  PeiServices    An indirect pointer to the PEI Services Table published by the PEI Foundation.
  @param  This           Pointer to local data for the interface.
  @param  Address        The physical address of the access.
  @param  Data           The data to write.

  @return None

**/
typedef
VOID
(EFIAPI *EFI_PEI_CPU_IO_PPI_IO_WRITE64) (
  IN  EFI_PEI_SERVICES        **PeiServices,
  IN  EFI_PEI_CPU_IO_PPI      *This,
  IN  UINT64                  Address,
  IN  UINT64                  Data
  );

/**
  8-bit Memory read operations.

  @param  PeiServices    An indirect pointer to the PEI Services Table published by the PEI Foundation.
  @param  This           Pointer to local data for the interface.
  @param  Address        The physical address of the access.

  @return UINT8

**/
typedef
UINT8
(EFIAPI *EFI_PEI_CPU_IO_PPI_MEM_READ8) (
  IN  EFI_PEI_SERVICES        **PeiServices,
  IN  EFI_PEI_CPU_IO_PPI      *This,
  IN  UINT64                  Address
  );

/**
  16-bit Memory read operations.

  @param  PeiServices    An indirect pointer to the PEI Services Table published by the PEI Foundation.
  @param  This           Pointer to local data for the interface.
  @param  Address        The physical address of the access.

  @return UINT16

**/
typedef
UINT16
(EFIAPI *EFI_PEI_CPU_IO_PPI_MEM_READ16) (
  IN  EFI_PEI_SERVICES        **PeiServices,
  IN  EFI_PEI_CPU_IO_PPI      *This,
  IN  UINT64                  Address
  );

/**
  32-bit Memory read operations.

  @param  PeiServices    An indirect pointer to the PEI Services Table published by the PEI Foundation.
  @param  This           Pointer to local data for the interface.
  @param  Address        The physical address of the access.

  @return UINT32

**/
typedef
UINT32
(EFIAPI *EFI_PEI_CPU_IO_PPI_MEM_READ32) (
  IN  EFI_PEI_SERVICES        **PeiServices,
  IN  EFI_PEI_CPU_IO_PPI      *This,
  IN  UINT64                  Address
  );

/**
  64-bit Memory read operations.

  @param  PeiServices    An indirect pointer to the PEI Services Table published by the PEI Foundation.
  @param  This           Pointer to local data for the interface.
  @param  Address        The physical address of the access.

  @return UINT64

**/
typedef
UINT64
(EFIAPI *EFI_PEI_CPU_IO_PPI_MEM_READ64) (
  IN  EFI_PEI_SERVICES        **PeiServices,
  IN  EFI_PEI_CPU_IO_PPI      *This,
  IN  UINT64                  Address
  );

/**
  8-bit Memory write operations.

  @param  PeiServices    An indirect pointer to the PEI Services Table published by the PEI Foundation.
  @param  This           Pointer to local data for the interface.
  @param  Address        The physical address of the access.
  @param  Data           The data to write.

  @return None

**/
typedef
VOID
(EFIAPI *EFI_PEI_CPU_IO_PPI_MEM_WRITE8) (
  IN  EFI_PEI_SERVICES        **PeiServices,
  IN  EFI_PEI_CPU_IO_PPI      *This,
  IN  UINT64                  Address,
  IN  UINT8                   Data
  );

/**
  16-bit Memory write operations.

  @param  PeiServices    An indirect pointer to the PEI Services Table published by the PEI Foundation.
  @param  This           Pointer to local data for the interface.
  @param  Address        The physical address of the access.
  @param  Data           The data to write.

  @return None

**/
typedef
VOID
(EFIAPI *EFI_PEI_CPU_IO_PPI_MEM_WRITE16) (
  IN  EFI_PEI_SERVICES        **PeiServices,
  IN  EFI_PEI_CPU_IO_PPI      *This,
  IN  UINT64                  Address,
  IN  UINT16                  Data
  );

/**
  32-bit Memory write operations.

  @param  PeiServices    An indirect pointer to the PEI Services Table published by the PEI Foundation.
  @param  This           Pointer to local data for the interface.
  @param  Address        The physical address of the access.
  @param  Data           The data to write.

  @return None

**/
typedef
VOID
(EFIAPI *EFI_PEI_CPU_IO_PPI_MEM_WRITE32) (
  IN  EFI_PEI_SERVICES        **PeiServices,
  IN  EFI_PEI_CPU_IO_PPI      *This,
  IN  UINT64                  Address,
  IN  UINT32                  Data
  );

/**
  64-bit Memory write operations.

  @param  PeiServices    An indirect pointer to the PEI Services Table published by the PEI Foundation.
  @param  This           Pointer to local data for the interface.
  @param  Address        The physical address of the access.
  @param  Data           The data to write.

  @return None

**/
typedef
VOID
(EFIAPI *EFI_PEI_CPU_IO_PPI_MEM_WRITE64) (
  IN  EFI_PEI_SERVICES        **PeiServices,
  IN  EFI_PEI_CPU_IO_PPI      *This,
  IN  UINT64                  Address,
  IN  UINT64                  Data
  );

/**
  @par Ppi Description:
  EFI_PEI_CPU_IO_PPI provides a set of memory and I/O-based services.
  The perspective of the services is that of the processor, not the bus or system.

  @param Mem
  Collection of memory-access services.

  @param I/O
  Collection of I/O-access services.

  @param IoRead8
  8-bit read service.

  @param IoRead16
  16-bit read service.

  @param IoRead32
  32-bit read service.

  @param IoRead64
  64-bit read service.

  @param IoWrite8
  8-bit write service.

  @param IoWrite16
  16-bit write service.

  @param IoWrite32
  32-bit write service.

  @param IoWrite64
  64-bit write service.

  @param MemRead8
  8-bit read service.

  @param MemRead16
  16-bit read service.

  @param MemRead32
  32-bit read service.

  @param MemRead64
  64-bit read service.

  @param MemWrite8
  8-bit write service.

  @param MemWrite16
  16-bit write service.

  @param MemWrite32
  32-bit write service.

  @param MemWrite64
  64-bit write service.

**/
struct _EFI_PEI_CPU_IO_PPI {
  EFI_PEI_CPU_IO_PPI_ACCESS       Mem;
  EFI_PEI_CPU_IO_PPI_ACCESS       Io;
  EFI_PEI_CPU_IO_PPI_IO_READ8     IoRead8;
  EFI_PEI_CPU_IO_PPI_IO_READ16    IoRead16;
  EFI_PEI_CPU_IO_PPI_IO_READ32    IoRead32;
  EFI_PEI_CPU_IO_PPI_IO_READ64    IoRead64;
  EFI_PEI_CPU_IO_PPI_IO_WRITE8    IoWrite8;
  EFI_PEI_CPU_IO_PPI_IO_WRITE16   IoWrite16;
  EFI_PEI_CPU_IO_PPI_IO_WRITE32   IoWrite32;
  EFI_PEI_CPU_IO_PPI_IO_WRITE64   IoWrite64;
  EFI_PEI_CPU_IO_PPI_MEM_READ8    MemRead8;
  EFI_PEI_CPU_IO_PPI_MEM_READ16   MemRead16;
  EFI_PEI_CPU_IO_PPI_MEM_READ32   MemRead32;
  EFI_PEI_CPU_IO_PPI_MEM_READ64   MemRead64;
  EFI_PEI_CPU_IO_PPI_MEM_WRITE8   MemWrite8;
  EFI_PEI_CPU_IO_PPI_MEM_WRITE16  MemWrite16;
  EFI_PEI_CPU_IO_PPI_MEM_WRITE32  MemWrite32;
  EFI_PEI_CPU_IO_PPI_MEM_WRITE64  MemWrite64;
};

extern EFI_GUID gEfiPeiCpuIoPpiInServiceTableGuid;

#endif
