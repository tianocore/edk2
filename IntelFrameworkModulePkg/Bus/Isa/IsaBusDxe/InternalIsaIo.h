/*++

Copyright (c) 2006 - 2007, Intel Corporation. All rights reserved. 
This software and associated documentation (if any) is furnished
under a license and may only be used or copied in accordance
with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be
reproduced, stored in a retrieval system, or transmitted in any
form or by any means without the express written consent of
Intel Corporation.

Module Name:

  IsaIo.h
  
Abstract:
  
  The header file for EFI_ISA_IO protocol implementation.
  
--*/

#ifndef _EFI_ISA_IO_LOCAL_H
#define _EFI_ISA_IO_LOCAL_H

//
// Include common header file for this module.
//
#include "CommonHeader.h"

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
