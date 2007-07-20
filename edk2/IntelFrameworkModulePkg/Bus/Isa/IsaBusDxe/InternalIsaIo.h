/**@file
  The header file for EFI_ISA_IO protocol implementation.
  
Copyright (c) 2006 - 2007, Intel Corporation.<BR>
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _EFI_ISA_IO_LOCAL_H
#define _EFI_ISA_IO_LOCAL_H

#include "InternalIsaBus.h"

//
// ISA I/O Support Function Prototypes
//

EFI_STATUS
IsaIoVerifyAccess (
  IN     ISA_IO_DEVICE              *IsaIoDevice,
  IN     ISA_ACCESS_TYPE            Type,
  IN     EFI_ISA_IO_PROTOCOL_WIDTH  Width,
  IN     UINTN                      Count,
  IN OUT UINT32                     *Offset
  );
  
EFI_STATUS
EFIAPI
IsaIoIoRead (
  IN     EFI_ISA_IO_PROTOCOL                        *This,
  IN     EFI_ISA_IO_PROTOCOL_WIDTH                  Width,
  IN     UINT32                                     Offset,
  IN     UINTN                                      Count,
  IN OUT VOID                                       *Buffer
  );

EFI_STATUS
EFIAPI
IsaIoIoWrite (
  IN     EFI_ISA_IO_PROTOCOL                        *This,
  IN     EFI_ISA_IO_PROTOCOL_WIDTH                  Width,
  IN     UINT32                                     Offset,
  IN     UINTN                                      Count,
  IN OUT VOID                                       *Buffer
  );

EFI_STATUS
EFIAPI
IsaIoMap (
  IN     EFI_ISA_IO_PROTOCOL                               *This,
  IN     EFI_ISA_IO_PROTOCOL_OPERATION                     Operation,
  IN     UINT8                                             ChannelNumber      OPTIONAL,
  IN     UINT32                                            ChannelAttributes,
  IN     VOID                                              *HostAddress,
  IN OUT UINTN                                             *NumberOfBytes,
  OUT    EFI_PHYSICAL_ADDRESS                              *DeviceAddress,
  OUT    VOID                                              **Mapping
  );

EFI_STATUS
EFIAPI
IsaIoUnmap (
  IN EFI_ISA_IO_PROTOCOL                  *This,
  IN VOID                                 *Mapping
  );

EFI_STATUS
EFIAPI
IsaIoFlush (
  IN EFI_ISA_IO_PROTOCOL                  *This
  );

EFI_STATUS
ReportErrorStatusCode (
  EFI_STATUS_CODE_VALUE code
  );

EFI_STATUS
WriteDmaPort (
  IN EFI_ISA_IO_PROTOCOL                  *This,
  IN UINT32                               AddrOffset,
  IN UINT32                               PageOffset,
  IN UINT32                               CountOffset,
  IN UINT32                               BaseAddress,
  IN UINT16                               Count
  );

EFI_STATUS
WritePort (
  IN EFI_ISA_IO_PROTOCOL                  *This,
  IN UINT32                               Offset,
  IN UINT8                                Value
  );    

EFI_STATUS
EFIAPI
IsaIoMemRead (
  IN     EFI_ISA_IO_PROTOCOL                       *This,
  IN     EFI_ISA_IO_PROTOCOL_WIDTH                 Width,
  IN     UINT32                                    Offset,
  IN     UINTN                                     Count,
  IN OUT VOID                                      *Buffer
  );


EFI_STATUS
EFIAPI
IsaIoMemWrite (
  IN     EFI_ISA_IO_PROTOCOL                        *This,
  IN     EFI_ISA_IO_PROTOCOL_WIDTH                  Width,
  IN     UINT32                                     Offset,
  IN     UINTN                                      Count,
  IN OUT VOID                                       *Buffer
  );

EFI_STATUS
EFIAPI
IsaIoCopyMem (
  IN EFI_ISA_IO_PROTOCOL                        *This,
  IN EFI_ISA_IO_PROTOCOL_WIDTH                  Width,
  IN UINT32                                     DestOffset,
  IN UINT32                                     SrcOffset,
  IN UINTN                                      Count
  );

EFI_STATUS
EFIAPI
IsaIoAllocateBuffer (
  IN  EFI_ISA_IO_PROTOCOL                  *This,
  IN  EFI_ALLOCATE_TYPE                    Type,
  IN  EFI_MEMORY_TYPE                      MemoryType,
  IN  UINTN                                Pages,
  OUT VOID                                 **HostAddress,
  IN  UINT64                               Attributes
  );

EFI_STATUS
EFIAPI
IsaIoFreeBuffer (
  IN EFI_ISA_IO_PROTOCOL                  *This,
  IN UINTN                                Pages,
  IN VOID                                 *HostAddress
  );

#endif
