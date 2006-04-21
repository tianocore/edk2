/*++

Copyright (c) 2006, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:
  
  MemoryStatusCode.h
   
Abstract:

  Lib to provide status code reporting via memory.

--*/

#ifndef _PEI_MEMORY_STATUS_CODE_H_
#define _PEI_MEMORY_STATUS_CODE_H_

//
// Publicly exported function
//
EFI_STATUS
EFIAPI
InstallMonoStatusCode (
  IN EFI_FFS_FILE_HEADER       *FfsHeader,
  IN EFI_PEI_SERVICES          **PeiServices
  )
;
//
// Publicly exported data
//
extern BOOLEAN  gRunningFromMemory;
//
// Private data
//
//
// Define the amount of heap to use before memory is allocated
//
#define PEI_STATUS_CODE_HEAP_LENGTH     512
#define PEI_STATUS_CODE_MAX_HEAP_ENTRY  (PEI_STATUS_CODE_HEAP_LENGTH / sizeof (EFI_STATUS_CODE_ENTRY))

//
// Define the number of 4K pages of BS memory to allocate (1MB)
//
#define PEI_STATUS_CODE_RT_PAGES      (128)
#define PEI_STATUS_CODE_RT_LENGTH     (PEI_STATUS_CODE_RT_PAGES * 1024 * 4)
#define PEI_STATUS_CODE_MAX_RT_ENTRY  (PEI_STATUS_CODE_RT_LENGTH / sizeof (EFI_STATUS_CODE_ENTRY))

//
// Define a private data structure
//
#define MEMORY_STATUS_CODE_SIGNATURE  EFI_SIGNATURE_32 ('M', 'S', 'C', 'S')

typedef struct _MEMORY_STATUS_CODE_INSTANCE {
  UINT32                              Signature;
  struct _MEMORY_STATUS_CODE_INSTANCE *This;
  EFI_FFS_FILE_HEADER                 *FfsHeader;
  EFI_PEI_PPI_DESCRIPTOR              PpiDescriptor;
  PEI_STATUS_CODE_MEMORY_PPI          StatusCodeMemoryPpi;
  EFI_PEI_NOTIFY_DESCRIPTOR           NotifyDescriptor;
} MEMORY_STATUS_CODE_INSTANCE;

#define MEMORY_STATUS_CODE_FROM_DESCRIPTOR_THIS(a) \
  PEI_CR (a, \
          MEMORY_STATUS_CODE_INSTANCE, \
          PpiDescriptor, \
          MEMORY_STATUS_CODE_SIGNATURE \
      )
#define MEMORY_STATUS_CODE_FROM_NOTIFY_THIS(a) \
  PEI_CR (a, \
          MEMORY_STATUS_CODE_INSTANCE, \
          NotifyDescriptor, \
          MEMORY_STATUS_CODE_SIGNATURE \
      )

//
// Private function declarations
//
EFI_STATUS
EFIAPI
LoadImageCallback (
  IN EFI_PEI_SERVICES           **PeiServices,
  IN EFI_PEI_NOTIFY_DESCRIPTOR  *NotifyDescriptor,
  IN VOID                       *Ppi
  )
;

#endif
