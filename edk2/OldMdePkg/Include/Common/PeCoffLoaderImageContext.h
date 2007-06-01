/** @file
  Important data type defined used for Memory Only PE COFF loader. 

  Copyright (c) 2007, Intel Corporation                                                         
  All rights reserved. This program and the accompanying materials                          
  are licensed and made available under the terms and conditions of the BSD License         
  which accompanies this distribution.  The full text of the license may be found at        
  http://opensource.org/licenses/bsd-license.php                                            

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

**/

#ifndef __PECOFFLOADER_IMAGE_CONTEXT_H
#define __PECOFFLOADER_IMAGE_CONTEXT_H

//
// PE/COFF Loader Read Function passed in by caller
//
typedef
RETURN_STATUS
(EFIAPI *PE_COFF_LOADER_READ_FILE) (
  IN     VOID   *FileHandle,
  IN     UINTN  FileOffset,
  IN OUT UINTN  *ReadSize,
  OUT    VOID   *Buffer
  );

//
// Context structure used while PE/COFF image is being loaded and relocated
//
typedef struct {
  PHYSICAL_ADDRESS                  ImageAddress;
  UINT64                            ImageSize;
  PHYSICAL_ADDRESS                  DestinationAddress;
  PHYSICAL_ADDRESS                  EntryPoint;
  PE_COFF_LOADER_READ_FILE          ImageRead;
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
} PE_COFF_LOADER_IMAGE_CONTEXT;

#endif
