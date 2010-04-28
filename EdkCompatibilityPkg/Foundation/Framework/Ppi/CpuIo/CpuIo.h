/*++

Copyright (c) 2004, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  CpuIo.h

Abstract:

  CPU IO PPI defined in Tiano
  CPU IO PPI abstracts CPU IO access
  

--*/

#ifndef _PEI_CPUIO_PPI_H_
#define _PEI_CPUIO_PPI_H_

#define PEI_CPU_IO_PPI_GUID \
  { \
    0xe6af1f7b, 0xfc3f, 0x46da, {0xa8, 0x28, 0xa3, 0xb4, 0x57, 0xa4, 0x42, 0x82} \
  }

EFI_FORWARD_DECLARATION (PEI_CPU_IO_PPI);

//
// *******************************************************
// PEI_CPU_IO_PPI_WIDTH
// *******************************************************
//
typedef enum {
  PeiCpuIoWidthUint8,
  PeiCpuIoWidthUint16,
  PeiCpuIoWidthUint32,
  PeiCpuIoWidthUint64,
  PeiCpuIoWidthFifoUint8,
  PeiCpuIoWidthFifoUint16,
  PeiCpuIoWidthFifoUint32,
  PeiCpuIoWidthFifoUint64,
  PeiCpuIoWidthFillUint8,
  PeiCpuIoWidthFillUint16,
  PeiCpuIoWidthFillUint32,
  PeiCpuIoWidthFillUint64,
  PeiCpuIoWidthMaximum
} PEI_CPU_IO_PPI_WIDTH;

//
// *******************************************************
// PEI_CPU_IO_PPI_IO_MEM
// *******************************************************
//
typedef
EFI_STATUS
(EFIAPI *PEI_CPU_IO_PPI_IO_MEM) (
  IN  EFI_PEI_SERVICES                  **PeiServices,
  IN PEI_CPU_IO_PPI                     * This,
  IN  PEI_CPU_IO_PPI_WIDTH              Width,
  IN  UINT64                            Address,
  IN  UINTN                             Count,
  IN  OUT VOID                          *Buffer
  );

//
// *******************************************************
// PEI_CPU_IO_PPI_ACCESS
// *******************************************************
//
typedef struct {
  PEI_CPU_IO_PPI_IO_MEM Read;
  PEI_CPU_IO_PPI_IO_MEM Write;
} PEI_CPU_IO_PPI_ACCESS;

//
// *******************************************************
// Base IO Class Functions
// *******************************************************
//
typedef
UINT8
(EFIAPI *PEI_CPU_IO_PPI_IO_READ8) (
  IN  EFI_PEI_SERVICES        **PeiServices,
  IN PEI_CPU_IO_PPI           * This,
  IN  UINT64                  Address
  );

typedef
UINT16
(EFIAPI *PEI_CPU_IO_PPI_IO_READ16) (
  IN  EFI_PEI_SERVICES        **PeiServices,
  IN PEI_CPU_IO_PPI           * This,
  IN  UINT64                  Address
  );

typedef
UINT32
(EFIAPI *PEI_CPU_IO_PPI_IO_READ32) (
  IN  EFI_PEI_SERVICES        **PeiServices,
  IN PEI_CPU_IO_PPI           * This,
  IN  UINT64                  Address
  );

typedef
UINT64
(EFIAPI *PEI_CPU_IO_PPI_IO_READ64) (
  IN  EFI_PEI_SERVICES        **PeiServices,
  IN PEI_CPU_IO_PPI           * This,
  IN  UINT64                  Address
  );

typedef
VOID
(EFIAPI *PEI_CPU_IO_PPI_IO_WRITE8) (
  IN  EFI_PEI_SERVICES        **PeiServices,
  IN PEI_CPU_IO_PPI           * This,
  IN  UINT64                  Address,
  IN  UINT8                   Data
  );

typedef
VOID
(EFIAPI *PEI_CPU_IO_PPI_IO_WRITE16) (
  IN  EFI_PEI_SERVICES        **PeiServices,
  IN PEI_CPU_IO_PPI           * This,
  IN  UINT64                  Address,
  IN  UINT16                  Data
  );

typedef
VOID
(EFIAPI *PEI_CPU_IO_PPI_IO_WRITE32) (
  IN  EFI_PEI_SERVICES        **PeiServices,
  IN PEI_CPU_IO_PPI           * This,
  IN  UINT64                  Address,
  IN  UINT32                  Data
  );

typedef
VOID
(EFIAPI *PEI_CPU_IO_PPI_IO_WRITE64) (
  IN  EFI_PEI_SERVICES        **PeiServices,
  IN PEI_CPU_IO_PPI           * This,
  IN  UINT64                  Address,
  IN  UINT64                  Data
  );

typedef
UINT8
(EFIAPI *PEI_CPU_IO_PPI_MEM_READ8) (
  IN  EFI_PEI_SERVICES        **PeiServices,
  IN PEI_CPU_IO_PPI           * This,
  IN  UINT64                  Address
  );

typedef
UINT16
(EFIAPI *PEI_CPU_IO_PPI_MEM_READ16) (
  IN  EFI_PEI_SERVICES        **PeiServices,
  IN PEI_CPU_IO_PPI           * This,
  IN  UINT64                  Address
  );

typedef
UINT32
(EFIAPI *PEI_CPU_IO_PPI_MEM_READ32) (
  IN  EFI_PEI_SERVICES        **PeiServices,
  IN PEI_CPU_IO_PPI           * This,
  IN  UINT64                  Address
  );

typedef
UINT64
(EFIAPI *PEI_CPU_IO_PPI_MEM_READ64) (
  IN  EFI_PEI_SERVICES        **PeiServices,
  IN PEI_CPU_IO_PPI           * This,
  IN  UINT64                  Address
  );

typedef
VOID
(EFIAPI *PEI_CPU_IO_PPI_MEM_WRITE8) (
  IN  EFI_PEI_SERVICES        **PeiServices,
  IN PEI_CPU_IO_PPI           * This,
  IN  UINT64                  Address,
  IN  UINT8                   Data
  );

typedef
VOID
(EFIAPI *PEI_CPU_IO_PPI_MEM_WRITE16) (
  IN  EFI_PEI_SERVICES        **PeiServices,
  IN PEI_CPU_IO_PPI           * This,
  IN  UINT64                  Address,
  IN  UINT16                  Data
  );

typedef
VOID
(EFIAPI *PEI_CPU_IO_PPI_MEM_WRITE32) (
  IN  EFI_PEI_SERVICES        **PeiServices,
  IN PEI_CPU_IO_PPI           * This,
  IN  UINT64                  Address,
  IN  UINT32                  Data
  );

typedef
VOID
(EFIAPI *PEI_CPU_IO_PPI_MEM_WRITE64) (
  IN  EFI_PEI_SERVICES        **PeiServices,
  IN PEI_CPU_IO_PPI           * This,
  IN  UINT64                  Address,
  IN  UINT64                  Data
  );

//
// *******************************************************
// PEI_CPU_IO_PPI
// *******************************************************
//
struct _PEI_CPU_IO_PPI {
  PEI_CPU_IO_PPI_ACCESS       Mem;
  PEI_CPU_IO_PPI_ACCESS       Io;
  PEI_CPU_IO_PPI_IO_READ8     IoRead8;
  PEI_CPU_IO_PPI_IO_READ16    IoRead16;
  PEI_CPU_IO_PPI_IO_READ32    IoRead32;
  PEI_CPU_IO_PPI_IO_READ64    IoRead64;
  PEI_CPU_IO_PPI_IO_WRITE8    IoWrite8;
  PEI_CPU_IO_PPI_IO_WRITE16   IoWrite16;
  PEI_CPU_IO_PPI_IO_WRITE32   IoWrite32;
  PEI_CPU_IO_PPI_IO_WRITE64   IoWrite64;
  PEI_CPU_IO_PPI_MEM_READ8    MemRead8;
  PEI_CPU_IO_PPI_MEM_READ16   MemRead16;
  PEI_CPU_IO_PPI_MEM_READ32   MemRead32;
  PEI_CPU_IO_PPI_MEM_READ64   MemRead64;
  PEI_CPU_IO_PPI_MEM_WRITE8   MemWrite8;
  PEI_CPU_IO_PPI_MEM_WRITE16  MemWrite16;
  PEI_CPU_IO_PPI_MEM_WRITE32  MemWrite32;
  PEI_CPU_IO_PPI_MEM_WRITE64  MemWrite64;
};

extern EFI_GUID gPeiCpuIoPpiInServiceTableGuid;

#endif
