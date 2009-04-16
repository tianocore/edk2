/** @file
  Data structure and functions to load and unload PeImage.

Copyright (c) 2006 - 2008, Intel Corporation. <BR>
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/


#ifndef _IMAGE_H_
#define _IMAGE_H_

#define LOADED_IMAGE_PRIVATE_DATA_SIGNATURE   SIGNATURE_32('l','d','r','i')

typedef struct {
    UINTN                       Signature;
	/// Image handle
    EFI_HANDLE                  Handle;   
	/// Image type
    UINTN                       Type;           
	/// If entrypoint has been called
    BOOLEAN                     Started;        
	/// The image's entry point
    EFI_IMAGE_ENTRY_POINT       EntryPoint;     
	/// loaded image protocol
    EFI_LOADED_IMAGE_PROTOCOL   Info;           
	/// Location in memory
    EFI_PHYSICAL_ADDRESS        ImageBasePage;  
	/// Number of pages
    UINTN                       NumberOfPages;  
	/// Original fixup data
    CHAR8                       *FixupData;     
	/// Tpl of started image
    EFI_TPL                     Tpl;            
	/// Status returned by started image
    EFI_STATUS                  Status;         
	/// Size of ExitData from started image
    UINTN                       ExitDataSize;   
	/// Pointer to exit data from started image
    VOID                        *ExitData;      
	/// Pointer to pool allocation for context save/retore
    VOID                        *JumpBuffer;    
	/// Pointer to buffer for context save/retore
    BASE_LIBRARY_JUMP_BUFFER    *JumpContext;  
	/// Machine type from PE image
    UINT16                      Machine;        
	/// EBC Protocol pointer
    EFI_EBC_PROTOCOL            *Ebc;           
	/// Runtime image list
    EFI_RUNTIME_IMAGE_ENTRY     *RuntimeData;   
	/// Pointer to Loaded Image Device Path Protocl
    EFI_DEVICE_PATH_PROTOCOL    *LoadedImageDevicePath;  
	/// PeCoffLoader ImageContext
    PE_COFF_LOADER_IMAGE_CONTEXT  ImageContext; 

} LOADED_IMAGE_PRIVATE_DATA;

#define LOADED_IMAGE_PRIVATE_DATA_FROM_THIS(a) \
          CR(a, LOADED_IMAGE_PRIVATE_DATA, Info, LOADED_IMAGE_PRIVATE_DATA_SIGNATURE)


//
// Private Data Types
//
#define IMAGE_FILE_HANDLE_SIGNATURE       SIGNATURE_32('i','m','g','f')
typedef struct {
  UINTN               Signature;
  BOOLEAN             FreeBuffer;
  VOID                *Source;
  UINTN               SourceSize;
} IMAGE_FILE_HANDLE;


/**
  Opens a file for (simple) reading.  The simple read abstraction
  will access the file either from a memory copy, from a file
  system interface, or from the load file interface.

  @param  BootPolicy             Policy for Open Image File.
  @param  SourceBuffer           Pointer to the memory location containing copy
                                 of the image to be loaded.
  @param  SourceSize             The size in bytes of SourceBuffer.
  @param  FilePath               The specific file path from which the image is
                                 loaded
  @param  DeviceHandle           Pointer to the return device handle.
  @param  ImageFileHandle        Pointer to the image file handle.
  @param  AuthenticationStatus   Pointer to a caller-allocated UINT32 in which
                                 the authentication status is returned.

  @retval EFI_SUCCESS            Image file successfully opened.
  @retval EFI_LOAD_ERROR         If the caller passed a copy of the file, and
                                 SourceSize is 0.
  @retval EFI_INVALID_PARAMETER  File path is not valid.
  @retval EFI_NOT_FOUND          File not found.

**/
EFI_STATUS
CoreOpenImageFile (
  IN BOOLEAN                        BootPolicy,
  IN VOID                           *SourceBuffer   OPTIONAL,
  IN UINTN                          SourceSize,
  IN OUT EFI_DEVICE_PATH_PROTOCOL   **FilePath,
  OUT EFI_HANDLE                    *DeviceHandle,
  IN IMAGE_FILE_HANDLE              *ImageFileHandle,
  OUT UINT32                        *AuthenticationStatus
  );



/**
  Read image file (specified by UserHandle) into user specified buffer with specified offset
  and length.

  @param  UserHandle             Image file handle
  @param  Offset                 Offset to the source file
  @param  ReadSize               For input, pointer of size to read; For output,
                                 pointer of size actually read.
  @param  Buffer                 Buffer to write into

  @retval EFI_SUCCESS            Successfully read the specified part of file
                                 into buffer.

**/
EFI_STATUS
EFIAPI
CoreReadImageFile (
  IN     VOID    *UserHandle,
  IN     UINTN   Offset,
  IN OUT UINTN   *ReadSize,
  OUT    VOID    *Buffer
  );



#endif
