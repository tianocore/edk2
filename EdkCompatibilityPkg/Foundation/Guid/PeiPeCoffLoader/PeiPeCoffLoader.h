/*++

Copyright (c) 2005 - 2010, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:
  
    PeiPeCoffLoader.h
    
Abstract:

  GUID for the PE/COFF Loader APIs shared between PEI and DXE

--*/

#ifndef _PEI_PE_COFF_LOADER_H_
#define _PEI_PE_COFF_LOADER_H_

#include "EfiImage.h"

#define EFI_PEI_PE_COFF_LOADER_GUID  \
  { 0xd8117cff, 0x94a6, 0x11d4, {0x9a, 0x3a, 0x0, 0x90, 0x27, 0x3f, 0xc1, 0x4d} }

EFI_FORWARD_DECLARATION (EFI_PEI_PE_COFF_LOADER_PROTOCOL);

#define EFI_IMAGE_ERROR_SUCCESS                      0
#define EFI_IMAGE_ERROR_IMAGE_READ                   1  
#define EFI_IMAGE_ERROR_INVALID_PE_HEADER_SIGNATURE  2
#define EFI_IMAGE_ERROR_INVALID_MACHINE_TYPE         3
#define EFI_IMAGE_ERROR_INVALID_SUBSYSTEM            4
#define EFI_IMAGE_ERROR_INVALID_IMAGE_ADDRESS        5
#define EFI_IMAGE_ERROR_INVALID_IMAGE_SIZE           6
#define EFI_IMAGE_ERROR_INVALID_SECTION_ALIGNMENT    7
#define EFI_IMAGE_ERROR_SECTION_NOT_LOADED           8
#define EFI_IMAGE_ERROR_FAILED_RELOCATION            9
#define EFI_IMAGE_ERROR_FAILED_ICACHE_FLUSH          10

typedef
EFI_STATUS
(EFIAPI *EFI_PEI_PE_COFF_LOADER_READ_FILE) (
  IN     VOID   *FileHandle,
  IN     UINTN  FileOffset,
  IN OUT UINTN  *ReadSize,
  OUT    VOID   *Buffer
  );

typedef struct {
  EFI_PHYSICAL_ADDRESS              ImageAddress;
  UINT64                            ImageSize;
  EFI_PHYSICAL_ADDRESS              DestinationAddress;
  EFI_PHYSICAL_ADDRESS              EntryPoint;
  EFI_PEI_PE_COFF_LOADER_READ_FILE  ImageRead;
  VOID                              *Handle;
  VOID                              *FixupData;
  UINT32                            SectionAlignment;
  UINT32                            PeCoffHeaderOffset;
  UINT32                            DebugDirectoryEntryRva;
  VOID                              *CodeView;
  CHAR8                             *PdbPointer;
  UINTN                             SizeOfHeaders;
  UINT32                            ImageCodeMemoryType;
  UINT32                            ImageDataMemoryType;
  UINT32                            ImageError;
  UINTN                             FixupDataSize;
  UINT16                            Machine;
  UINT16                            ImageType;
  BOOLEAN                           RelocationsStripped;
  BOOLEAN                           IsTeImage;
#ifdef EFI_NT_EMULATOR
  VOID                              **ModHandle;
#endif
#if (EFI_SPECIFICATION_VERSION >= 0x0002000A)
  EFI_PHYSICAL_ADDRESS              HiiResourceData;
#endif
} EFI_PEI_PE_COFF_LOADER_IMAGE_CONTEXT;

typedef 
EFI_STATUS
(EFIAPI *EFI_PEI_PE_COFF_LOADER_GET_IMAGE_INFO) (
  IN EFI_PEI_PE_COFF_LOADER_PROTOCOL  *This,
  IN OUT EFI_PEI_PE_COFF_LOADER_IMAGE_CONTEXT *ImageContext
  );

typedef 
EFI_STATUS
(EFIAPI *EFI_PEI_PE_COFF_LOADER_LOAD_IMAGE) (
  IN EFI_PEI_PE_COFF_LOADER_PROTOCOL  *This,
  IN OUT EFI_PEI_PE_COFF_LOADER_IMAGE_CONTEXT *ImageContext
  );

typedef 
EFI_STATUS
(EFIAPI *EFI_PEI_PE_COFF_LOADER_RELOCATE_IMAGE) (
  IN EFI_PEI_PE_COFF_LOADER_PROTOCOL  *This,
  IN OUT EFI_PEI_PE_COFF_LOADER_IMAGE_CONTEXT *ImageContext
  );

typedef 
EFI_STATUS
(EFIAPI *EFI_PEI_PE_COFF_LOADER_UNLOAD_IMAGE) (
  IN EFI_PEI_PE_COFF_LOADER_IMAGE_CONTEXT *ImageContext
  );

struct _EFI_PEI_PE_COFF_LOADER_PROTOCOL {
  EFI_PEI_PE_COFF_LOADER_GET_IMAGE_INFO  GetImageInfo;
  EFI_PEI_PE_COFF_LOADER_LOAD_IMAGE      LoadImage;
  EFI_PEI_PE_COFF_LOADER_RELOCATE_IMAGE  RelocateImage;
  EFI_PEI_PE_COFF_LOADER_UNLOAD_IMAGE    UnloadImage;
};

extern EFI_GUID gEfiPeiPeCoffLoaderGuid;

#endif
