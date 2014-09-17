/** @file
	Function prototypes and defines on Memory Only PE COFF loader

	Copyright (c) 2006 - 2014, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials                          
	are licensed and made available under the terms and conditions of the BSD License         
	which accompanies this distribution.  The full text of the license may be found at        
	http://opensource.org/licenses/bsd-license.php                                            

	THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
	WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

**/

#ifndef __BASE_PE_COFF_LIB_H__
#define __BASE_PE_COFF_LIB_H__

//
// Return status codes from the PE/COFF Loader services
// BUGBUG: Find where used and see if can be replaced by RETURN_STATUS codes
//
#define IMAGE_ERROR_SUCCESS                      0
#define IMAGE_ERROR_IMAGE_READ                   1  
#define IMAGE_ERROR_INVALID_PE_HEADER_SIGNATURE  2
#define IMAGE_ERROR_INVALID_MACHINE_TYPE         3
#define IMAGE_ERROR_INVALID_SUBSYSTEM            4
#define IMAGE_ERROR_INVALID_IMAGE_ADDRESS        5
#define IMAGE_ERROR_INVALID_IMAGE_SIZE           6
#define IMAGE_ERROR_INVALID_SECTION_ALIGNMENT    7
#define IMAGE_ERROR_SECTION_NOT_LOADED           8
#define IMAGE_ERROR_FAILED_RELOCATION            9
#define IMAGE_ERROR_FAILED_ICACHE_FLUSH          10


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


/**
	Retrieves information on a PE/COFF image

	@param	ImageContext The context of the image being loaded

	@retval	EFI_SUCCESS The information on the PE/COFF image was collected.
	@retval	EFI_INVALID_PARAMETER ImageContext is NULL.
	@retval	EFI_UNSUPPORTED The PE/COFF image is not supported.
	@retval	Otherwise The error status from reading the PE/COFF image using the
	ImageContext->ImageRead() function

**/
RETURN_STATUS
EFIAPI
PeCoffLoaderGetImageInfo (
  IN OUT PE_COFF_LOADER_IMAGE_CONTEXT  *ImageContext
  )
;

/**
	Relocates a PE/COFF image in memory

	@param	ImageContext Contains information on the loaded image to relocate

	@retval EFI_SUCCESS      if the PE/COFF image was relocated
	@retval EFI_LOAD_ERROR   if the image is not a valid PE/COFF image
	@retval EFI_UNSUPPORTED  not support

**/
RETURN_STATUS
EFIAPI
PeCoffLoaderRelocateImage (
  IN OUT PE_COFF_LOADER_IMAGE_CONTEXT  *ImageContext
  )
;

/**
	Loads a PE/COFF image into memory

	@param	ImageContext Contains information on image to load into memory

	@retval EFI_SUCCESS            if the PE/COFF image was loaded
	@retval EFI_BUFFER_TOO_SMALL   if the caller did not provide a large enough buffer
	@retval EFI_LOAD_ERROR         if the image is a runtime driver with no relocations
	@retval EFI_INVALID_PARAMETER  if the image address is invalid

**/
RETURN_STATUS
EFIAPI
PeCoffLoaderLoadImage (
  IN OUT PE_COFF_LOADER_IMAGE_CONTEXT  *ImageContext
  )
;

VOID *
EFIAPI
PeCoffLoaderGetPdbPointer (
  IN VOID  *Pe32Data
  )
;

RETURN_STATUS
EFIAPI
PeCoffLoaderGetEntryPoint (
  IN  VOID  *Pe32Data,
  OUT VOID  **EntryPoint,
  OUT VOID  **BaseOfImage
  )
;

//
// These functions are used by the ARM PE/COFF relocation code and by
// the ELF to PE/COFF converter so that is why they are public
//

/**
  Pass in a pointer to an ARM MOVT or MOVW immediate instruciton and 
  return the immediate data encoded in the instruction

  @param  Instruction   Pointer to ARM MOVT or MOVW immediate instruction

  @return Immediate address encoded in the instruction

**/
UINT16
EFIAPI
ThumbMovtImmediateAddress (
  IN UINT16 *Instruction
  );

/**
  Update an ARM MOVT or MOVW immediate instruction immediate data.

  @param  Instruction   Pointer to ARM MOVT or MOVW immediate instruction
  @param  Address       New addres to patch into the instruction

**/
VOID
EFIAPI
ThumbMovtImmediatePatch (
  IN OUT UINT16 *Instruction,
  IN     UINT16 Address
  );


/**
  Pass in a pointer to an ARM MOVW/MOVT instruciton pair and 
  return the immediate data encoded in the two` instruction

  @param  Instructions  Pointer to ARM MOVW/MOVT insturction pair

  @return Immediate address encoded in the instructions

**/
UINT32
EFIAPI
ThumbMovwMovtImmediateAddress (
  IN UINT16 *Instructions
  );

/**
  Update an ARM MOVW/MOVT immediate instruction instruction pair.

  @param  Instructions  Pointer to ARM MOVW/MOVT instruction pair
  @param  Address       New addres to patch into the instructions
**/
VOID
EFIAPI
ThumbMovwMovtImmediatePatch (
  IN OUT UINT16 *Instructions,
  IN     UINT32 Address
  );



#endif
