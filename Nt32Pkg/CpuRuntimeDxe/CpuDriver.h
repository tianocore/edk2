/*++

Copyright (c) 2006, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  CpuDriver.h

Abstract:

  NT Emulation Architectural Protocol Driver as defined in Tiano.

--*/

#ifndef _CPU_ARCHITECTURAL_PROTOCOL_DRIVER_H_
#define _CPU_ARCHITECTURAL_PROTOCOL_DRIVER_H_


#include <FrameworkDxe.h>
#include <Protocol/Cpu.h>
#include <Protocol/DataHub.h>
#include <Protocol/HiiFramework.h>
#include <Guid/DataHubRecords.h>
#include <Protocol/CpuIo.h>
#include <Protocol/WinNtIo.h>
#include <Library/DebugLib.h>
#include <Library/HiiLibFramework.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>


extern UINT8  CpuStrings[];

//
// Internal Data Structures
//
#define CPU_ARCH_PROT_PRIVATE_SIGNATURE EFI_SIGNATURE_32 ('c', 'a', 'p', 'd')

typedef struct {
  UINTN                 Signature;
  EFI_HANDLE            Handle;

  EFI_CPU_ARCH_PROTOCOL Cpu;
  EFI_CPU_IO_PROTOCOL   CpuIo;

  //
  // Local Data for CPU interface goes here
  //
  CRITICAL_SECTION      NtCriticalSection;
  BOOLEAN               InterruptState;

} CPU_ARCH_PROTOCOL_PRIVATE;

#define CPU_ARCH_PROTOCOL_PRIVATE_DATA_FROM_THIS(a) \
  CR (a, \
      CPU_ARCH_PROTOCOL_PRIVATE, \
      Cpu, \
      CPU_ARCH_PROT_PRIVATE_SIGNATURE \
      )

EFI_STATUS
EFIAPI
CpuMemoryServiceRead (
  IN  EFI_CPU_IO_PROTOCOL               *This,
  IN  EFI_CPU_IO_PROTOCOL_WIDTH         Width,
  IN  UINT64                            Address,
  IN  UINTN                             Count,
  IN  OUT VOID                          *Buffer
  );

EFI_STATUS
EFIAPI
CpuMemoryServiceWrite (
  IN EFI_CPU_IO_PROTOCOL                *This,
  IN  EFI_CPU_IO_PROTOCOL_WIDTH         Width,
  IN  UINT64                            Address,
  IN  UINTN                             Count,
  IN  OUT VOID                          *Buffer
  );

EFI_STATUS
EFIAPI
CpuIoServiceRead (
  IN EFI_CPU_IO_PROTOCOL                *This,
  IN  EFI_CPU_IO_PROTOCOL_WIDTH         Width,
  IN  UINT64                            UserAddress,
  IN  UINTN                             Count,
  IN  OUT VOID                          *UserBuffer
  );

EFI_STATUS
EFIAPI
CpuIoServiceWrite (
  IN EFI_CPU_IO_PROTOCOL                *This,
  IN  EFI_CPU_IO_PROTOCOL_WIDTH         Width,
  IN  UINT64                            UserAddress,
  IN  UINTN                             Count,
  IN  OUT VOID                          *UserBuffer
  );


#endif
