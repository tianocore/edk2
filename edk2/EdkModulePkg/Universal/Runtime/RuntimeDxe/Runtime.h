/*++

Copyright (c) 2006, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  Runtime.h

Abstract:

  Runtime Architectural Protocol as defined in the DXE CIS

  This code is used to produce the EFI runtime virtual switch over

--*/

#ifndef _RUNTIME_H_
#define _RUNTIME_H_

//
// Data structures
//
typedef struct {
  LIST_ENTRY            Link;
  BOOLEAN               Valid;
  EFI_PHYSICAL_ADDRESS  ImageBase;
  UINTN                 ImageSize;  // In no. of pages
  VOID                  *RelocationData;
} RUNTIME_IMAGE_RELOCATION_DATA;

typedef struct {
  LIST_ENTRY          Link;
  IN UINT32           Type;
  IN EFI_TPL          NotifyTpl;
  IN EFI_EVENT_NOTIFY NotifyFunction;
  IN VOID             *NotifyContext;
  IN EFI_EVENT        Event;
} RUNTIME_NOTIFY_EVENT_DATA;

//
// Function Prototypes
//
VOID
RelocatePeImageForRuntime (
  RUNTIME_IMAGE_RELOCATION_DATA  *Image
  )
;

EFI_STATUS
EFIAPI
RuntimeDriverCalculateCrc32 (
  IN  VOID    *Data,
  IN  UINTN   DataSize,
  OUT UINT32  *CrcOut
  )
;

EFI_STATUS
EFIAPI
RuntimeDriverRegisterImage (
  IN  EFI_RUNTIME_ARCH_PROTOCOL         *This,
  IN  EFI_PHYSICAL_ADDRESS              ImageBase,
  IN  UINTN                             ImageSize,
  IN  VOID                              *RelocationData
  )
;

EFI_STATUS
EFIAPI
RuntimeDriverRegisterEvent (
  IN EFI_RUNTIME_ARCH_PROTOCOL          *This,
  IN UINT32                             Type,
  IN EFI_TPL                            NotifyTpl,
  IN EFI_EVENT_NOTIFY                   NotifyFunction,
  IN VOID                               *NotifyContext,
  IN EFI_EVENT                          *Event
  )
;

EFI_STATUS
EFIAPI
RuntimeDriverConvertPointer (
  IN     UINTN  DebugDisposition,
  IN OUT VOID   **ConvertAddress
  )
;

VOID
RuntimeDriverInitializeCrc32Table (
  VOID
  )
;

EFI_STATUS
EFIAPI
RuntimeDriverInitialize (
  IN EFI_HANDLE                            ImageHandle,
  IN EFI_SYSTEM_TABLE                      *SystemTable
  )
;


//
// Cache Flush Routine.
//
EFI_STATUS
FlushCpuCache (
  IN EFI_PHYSICAL_ADDRESS          Start,
  IN UINT64                        Length
  )
;

#endif
