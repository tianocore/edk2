/*++

Copyright (c) 2006, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  DxeIpl.h

Abstract:

--*/

#ifndef __PEI_DXEIPL_H__
#define __PEI_DXEIPL_H__

#define STACK_SIZE      0x20000
#define BSP_STORE_SIZE  0x4000

extern BOOLEAN gInMemory;

VOID
SwitchIplStacks (
  VOID  *EntryPoint,
  UINTN Parameter1,
  UINTN Parameter2,
  VOID  *NewStack,
  VOID  *NewBsp
  )
;

EFI_STATUS
PeiFindFile (
  IN  UINT8                  Type,
  IN  UINT16                 SectionType,
  OUT EFI_GUID               *FileName,
  OUT VOID                   **Pe32Data
  )
;

EFI_STATUS
PeiLoadFile (
  IN  EFI_PEI_PE_COFF_LOADER_PROTOCOL           *PeiEfiPeiPeCoffLoader,
  IN  VOID                                      *Pe32Data,
  OUT EFI_PHYSICAL_ADDRESS                      *ImageAddress,
  OUT UINT64                                    *ImageSize,
  OUT EFI_PHYSICAL_ADDRESS                      *EntryPoint
  )
;


EFI_STATUS
CreateArchSpecificHobs (
  OUT EFI_PHYSICAL_ADDRESS                      *BspStore
  )
;

EFI_STATUS
GetImageReadFunction (
  IN      PE_COFF_LOADER_IMAGE_CONTEXT  *ImageContext
  )
;

EFI_STATUS
PeiImageRead (
  IN     VOID    *FileHandle,
  IN     UINTN   FileOffset,
  IN OUT UINTN   *ReadSize,
  OUT    VOID    *Buffer
  )
;

EFI_STATUS
EFIAPI
DxeIplLoadFile (
  IN EFI_PEI_FV_FILE_LOADER_PPI                 *This,
  IN  EFI_FFS_FILE_HEADER                       *FfsHeader,
  OUT EFI_PHYSICAL_ADDRESS                      *ImageAddress,
  OUT UINT64                                    *ImageSize,
  OUT EFI_PHYSICAL_ADDRESS                      *EntryPoint
  );

EFI_STATUS
ShadowDxeIpl (
  IN EFI_FFS_FILE_HEADER                       *DxeIpl,
  IN EFI_PEI_PE_COFF_LOADER_PROTOCOL           *PeiEfiPeiPeCoffLoader
  );

EFI_STATUS
EFIAPI
DxeLoadCore (
  IN EFI_DXE_IPL_PPI       *This,
  IN EFI_PEI_SERVICES      **PeiServices,
  IN EFI_PEI_HOB_POINTERS  HobList
  );

EFI_STATUS
PeiProcessFile (
  IN      UINT16                 SectionType,
  IN OUT  EFI_FFS_FILE_HEADER    **RealFfsFileHeader,
  OUT     VOID                   **Pe32Data
  );

EFI_STATUS
EFIAPI
PeimInitializeDxeIpl (
  IN EFI_FFS_FILE_HEADER       *FfsHeader,
  IN EFI_PEI_SERVICES          **PeiServices
  );

EFI_STATUS
PeiLoadPeImage (
  IN  EFI_PEI_PE_COFF_LOADER_PROTOCOL           *PeiEfiPeiPeCoffLoader,
  IN  VOID                                      *Pe32Data,
  IN  EFI_MEMORY_TYPE                           MemoryType,
  OUT EFI_PHYSICAL_ADDRESS                      *ImageAddress,
  OUT UINT64                                    *ImageSize,
  OUT EFI_PHYSICAL_ADDRESS                      *EntryPoint
  )
;

EFI_PHYSICAL_ADDRESS
CreateIdentityMappingPageTables (
  IN UINT32                NumberOfProcessorPhysicalAddressBits
  )
;

VOID
ActivateLongMode (
  IN  EFI_PHYSICAL_ADDRESS  PageTables,  
  IN  EFI_PHYSICAL_ADDRESS  HobStart,
  IN  EFI_PHYSICAL_ADDRESS  Stack,
  IN  EFI_PHYSICAL_ADDRESS  CodeEntryPoint1,
  IN  EFI_PHYSICAL_ADDRESS  CodeEntryPoint2
  );

VOID
LoadGo64Gdt();

#endif
