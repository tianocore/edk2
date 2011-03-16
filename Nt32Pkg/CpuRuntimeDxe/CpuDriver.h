/**@file

Copyright (c) 2006 - 2011, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  CpuDriver.h

Abstract:

  NT Emulation Architectural Protocol Driver as defined in Tiano.

**/

#ifndef _CPU_ARCHITECTURAL_PROTOCOL_DRIVER_H_
#define _CPU_ARCHITECTURAL_PROTOCOL_DRIVER_H_


#include <FrameworkDxe.h>
#include <IndustryStandard/SmBios.h>
#include <Protocol/Cpu.h>
#include <Protocol/Smbios.h>
#include <Protocol/FrameworkHii.h>
#include <Guid/DataHubRecords.h>
#include <Protocol/CpuIo2.h>
#include <Protocol/WinNtIo.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/HiiLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/PcdLib.h>
#include <Library/WinNtLib.h>


extern UINT8  CpuStrings[];

//
// Internal Data Structures
//
#define CPU_ARCH_PROT_PRIVATE_SIGNATURE SIGNATURE_32 ('c', 'a', 'p', 'd')

typedef struct {
  UINTN                 Signature;
  EFI_HANDLE            Handle;

  EFI_CPU_ARCH_PROTOCOL Cpu;
  EFI_CPU_IO2_PROTOCOL  CpuIo;

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
  IN  EFI_CPU_IO2_PROTOCOL              *This,
  IN  EFI_CPU_IO_PROTOCOL_WIDTH         Width,
  IN  UINT64                            Address,
  IN  UINTN                             Count,
  IN  OUT VOID                          *Buffer
  );

EFI_STATUS
EFIAPI
CpuMemoryServiceWrite (
  IN EFI_CPU_IO2_PROTOCOL               *This,
  IN  EFI_CPU_IO_PROTOCOL_WIDTH         Width,
  IN  UINT64                            Address,
  IN  UINTN                             Count,
  IN  OUT VOID                          *Buffer
  );

EFI_STATUS
EFIAPI
CpuIoServiceRead (
  IN EFI_CPU_IO2_PROTOCOL               *This,
  IN  EFI_CPU_IO_PROTOCOL_WIDTH         Width,
  IN  UINT64                            UserAddress,
  IN  UINTN                             Count,
  IN  OUT VOID                          *UserBuffer
  );

EFI_STATUS
EFIAPI
CpuIoServiceWrite (
  IN EFI_CPU_IO2_PROTOCOL               *This,
  IN  EFI_CPU_IO_PROTOCOL_WIDTH         Width,
  IN  UINT64                            UserAddress,
  IN  UINTN                             Count,
  IN  OUT VOID                          *UserBuffer
  );


EFI_STATUS
EFIAPI
InitializeCpu (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  );

EFI_STATUS
EFIAPI
WinNtFlushCpuDataCache (
  IN EFI_CPU_ARCH_PROTOCOL  *This,
  IN EFI_PHYSICAL_ADDRESS   Start,
  IN UINT64                 Length,
  IN EFI_CPU_FLUSH_TYPE     FlushType
  );

EFI_STATUS
EFIAPI
WinNtEnableInterrupt (
  IN EFI_CPU_ARCH_PROTOCOL  *This
  );

EFI_STATUS
EFIAPI
WinNtDisableInterrupt (
  IN EFI_CPU_ARCH_PROTOCOL  *This
  );

EFI_STATUS
EFIAPI
WinNtGetInterruptState (
  IN EFI_CPU_ARCH_PROTOCOL  *This,
  OUT BOOLEAN               *State
  );

EFI_STATUS
EFIAPI
WinNtInit (
  IN EFI_CPU_ARCH_PROTOCOL  *This,
  IN EFI_CPU_INIT_TYPE      InitType
  );

EFI_STATUS
EFIAPI
WinNtRegisterInterruptHandler (
  IN EFI_CPU_ARCH_PROTOCOL      *This,
  IN EFI_EXCEPTION_TYPE         InterruptType,
  IN EFI_CPU_INTERRUPT_HANDLER  InterruptHandler
  );

EFI_STATUS
EFIAPI
WinNtGetTimerValue (
  IN  EFI_CPU_ARCH_PROTOCOL *This,
  IN  UINT32                TimerIndex,
  OUT UINT64                *TimerValue,
  OUT UINT64                *TimerPeriod OPTIONAL
  );

EFI_STATUS
EFIAPI
WinNtSetMemoryAttributes (
  IN EFI_CPU_ARCH_PROTOCOL  *This,
  IN EFI_PHYSICAL_ADDRESS   BaseAddress,
  IN UINT64                 Length,
  IN UINT64                 Attributes
  );






#endif
