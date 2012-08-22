/** @file
  Data structure and functions to load and unload PeImage.

Copyright (c) 2006 - 2011, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
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
  /// Status returned by LoadImage() service.
  EFI_STATUS                  LoadImageStatus;
} LOADED_IMAGE_PRIVATE_DATA;

#define LOADED_IMAGE_PRIVATE_DATA_FROM_THIS(a) \
          CR(a, LOADED_IMAGE_PRIVATE_DATA, Info, LOADED_IMAGE_PRIVATE_DATA_SIGNATURE)


#define LOAD_PE32_IMAGE_PRIVATE_DATA_SIGNATURE  SIGNATURE_32('l','p','e','i')

typedef struct {
  UINTN                       Signature;
  /// Image handle
  EFI_HANDLE                  Handle;         
  EFI_PE32_IMAGE_PROTOCOL     Pe32Image;
} LOAD_PE32_IMAGE_PRIVATE_DATA;

#define LOAD_PE32_IMAGE_PRIVATE_DATA_FROM_THIS(a) \
          CR(a, LOAD_PE32_IMAGE_PRIVATE_DATA, Pe32Image, LOAD_PE32_IMAGE_PRIVATE_DATA_SIGNATURE)


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
  Loads an EFI image into memory and returns a handle to the image with extended parameters.

  @param  This                    Calling context
  @param  ParentImageHandle       The caller's image handle.
  @param  FilePath                The specific file path from which the image is
                                  loaded.
  @param  SourceBuffer            If not NULL, a pointer to the memory location
                                  containing a copy of the image to be loaded.
  @param  SourceSize              The size in bytes of SourceBuffer.
  @param  DstBuffer               The buffer to store the image.
  @param  NumberOfPages           For input, specifies the space size of the
                                  image by caller if not NULL. For output,
                                  specifies the actual space size needed.
  @param  ImageHandle             Image handle for output.
  @param  EntryPoint              Image entry point for output.
  @param  Attribute               The bit mask of attributes to set for the load
                                  PE image.

  @retval EFI_SUCCESS             The image was loaded into memory.
  @retval EFI_NOT_FOUND           The FilePath was not found.
  @retval EFI_INVALID_PARAMETER   One of the parameters has an invalid value.
  @retval EFI_UNSUPPORTED         The image type is not supported, or the device
                                  path cannot be parsed to locate the proper
                                  protocol for loading the file.
  @retval EFI_OUT_OF_RESOURCES    Image was not loaded due to insufficient
                                  resources.
  @retval EFI_LOAD_ERROR          Image was not loaded because the image format was corrupt or not
                                  understood.
  @retval EFI_DEVICE_ERROR        Image was not loaded because the device returned a read error.
  @retval EFI_ACCESS_DENIED       Image was not loaded because the platform policy prohibits the 
                                  image from being loaded. NULL is returned in *ImageHandle.
  @retval EFI_SECURITY_VIOLATION  Image was loaded and an ImageHandle was created with a 
                                  valid EFI_LOADED_IMAGE_PROTOCOL. However, the current 
                                  platform policy specifies that the image should not be started.

**/
EFI_STATUS
EFIAPI
CoreLoadImageEx (
  IN  EFI_PE32_IMAGE_PROTOCOL          *This,
  IN  EFI_HANDLE                       ParentImageHandle,
  IN  EFI_DEVICE_PATH_PROTOCOL         *FilePath,
  IN  VOID                             *SourceBuffer       OPTIONAL,
  IN  UINTN                            SourceSize,
  IN  EFI_PHYSICAL_ADDRESS             DstBuffer           OPTIONAL,
  OUT UINTN                            *NumberOfPages      OPTIONAL,
  OUT EFI_HANDLE                       *ImageHandle,
  OUT EFI_PHYSICAL_ADDRESS             *EntryPoint         OPTIONAL,
  IN  UINT32                           Attribute
  );


/**
  Unload the specified image.

  @param  This                    Indicates the calling context.
  @param  ImageHandle             The specified image handle.

  @retval EFI_INVALID_PARAMETER   Image handle is NULL.
  @retval EFI_UNSUPPORTED         Attempt to unload an unsupported image.
  @retval EFI_SUCCESS             Image successfully unloaded.

**/
EFI_STATUS
EFIAPI
CoreUnloadImageEx (
  IN EFI_PE32_IMAGE_PROTOCOL  *This,
  IN EFI_HANDLE                         ImageHandle
  );
#endif
