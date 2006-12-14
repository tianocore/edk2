/*++

Copyright (c) 2006, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

  Image.h

Abstract:

Revision History

--*/


#ifndef _IMAGE_H_
#define _IMAGE_H_



#define LOADED_IMAGE_PRIVATE_DATA_SIGNATURE   EFI_SIGNATURE_32('l','d','r','i')

typedef struct {
    UINTN                       Signature;
    EFI_HANDLE                  Handle;         // Image handle
    UINTN                       Type;           // Image type

    BOOLEAN                     Started;        // If entrypoint has been called

    EFI_IMAGE_ENTRY_POINT       EntryPoint;     // The image's entry point
    EFI_LOADED_IMAGE_PROTOCOL   Info;           // loaded image protocol

    EFI_PHYSICAL_ADDRESS        ImageBasePage;  // Location in memory
    UINTN                       NumberOfPages;  // Number of pages

    CHAR8                       *FixupData;     // Original fixup data

    EFI_TPL                     Tpl;            // Tpl of started image
    EFI_STATUS                  Status;         // Status returned by started image

    UINTN                       ExitDataSize;   // Size of ExitData from started image
    VOID                        *ExitData;      // Pointer to exit data from started image
    VOID                        *JumpBuffer;    // Pointer to pool allocation for context save/retore
    BASE_LIBRARY_JUMP_BUFFER    *JumpContext;   // Pointer to buffer for context save/retore
    UINT16                      Machine;        // Machine type from PE image

    EFI_EBC_PROTOCOL            *Ebc;           // EBC Protocol pointer

    EFI_RUNTIME_IMAGE_ENTRY     *RuntimeData;   // Runtime image list

    PE_COFF_LOADER_IMAGE_CONTEXT  ImageContext; // PeCoffLoader ImageContext

} LOADED_IMAGE_PRIVATE_DATA;

#define LOADED_IMAGE_PRIVATE_DATA_FROM_THIS(a) \
          CR(a, LOADED_IMAGE_PRIVATE_DATA, Info, LOADED_IMAGE_PRIVATE_DATA_SIGNATURE)



#define LOAD_PE32_IMAGE_PRIVATE_DATA_SIGNATURE  EFI_SIGNATURE_32('l','p','e','i')

typedef struct {
    UINTN                       Signature;
    EFI_HANDLE                  Handle;         // Image handle
    EFI_PE32_IMAGE_PROTOCOL     Pe32Image;
} LOAD_PE32_IMAGE_PRIVATE_DATA;

#define LOAD_PE32_IMAGE_PRIVATE_DATA_FROM_THIS(a) \
          CR(a, LOAD_PE32_IMAGE_PRIVATE_DATA, Pe32Image, LOAD_PE32_IMAGE_PRIVATE_DATA_SIGNATURE)



//
// Private Data Types
//
#define IMAGE_FILE_HANDLE_SIGNATURE       EFI_SIGNATURE_32('i','m','g','f')
typedef struct {
  UINTN               Signature;
  BOOLEAN             FreeBuffer;
  VOID                *Source;
  UINTN               SourceSize;
} IMAGE_FILE_HANDLE;


//
// Abstractions for reading image contents
//

EFI_STATUS
CoreOpenImageFile (
  IN BOOLEAN                        BootPolicy,
  IN VOID                           *SourceBuffer   OPTIONAL,
  IN UINTN                          SourceSize,
  IN OUT EFI_DEVICE_PATH_PROTOCOL   *FilePath,
  OUT EFI_HANDLE                    *DeviceHandle,
  IN IMAGE_FILE_HANDLE              *ImageFileHandle,
  OUT UINT32                        *AuthenticationStatus
  )
/*++

Routine Description:

    Opens a file for (simple) reading.  The simple read abstraction
    will access the file either from a memory copy, from a file
    system interface, or from the load file interface.

Arguments:

  BootPolicy    - Policy for Open Image File.
  SourceBuffer  - Pointer to the memory location containing copy
                  of the image to be loaded.
  SourceSize    - The size in bytes of SourceBuffer.
  FilePath      - The specific file path from which the image is loaded
  DeviceHandle  - Pointer to the return device handle.
  ImageFileHandle      - Pointer to the image file handle.
  AuthenticationStatus - Pointer to a caller-allocated UINT32 in which the authentication status is returned.

Returns:

    A handle to access the file

--*/
;


EFI_STATUS
EFIAPI
CoreReadImageFile (
  IN     VOID     *UserHandle,
  IN     UINTN    Offset,
  IN OUT UINTN    *ReadSize,
  OUT     VOID    *Buffer
  )
/*++

Routine Description:

  Read image file (specified by UserHandle) into user specified buffer with specified offset
  and length.

Arguments:

  UserHandle      - Image file handle

  Offset          - Offset to the source file

  ReadSize        - For input, pointer of size to read;
                    For output, pointer of size actually read.

  Buffer          - Buffer to write into

Returns:

  EFI_SUCCESS     - Successfully read the specified part of file into buffer.

--*/
;

VOID
EFIAPI
CoreCloseImageFile (
  IN IMAGE_FILE_HANDLE *ImageFileHandle
  )
/*++

Routine Description:

  A function out of date, should be removed.

Arguments:

  ImageFileHandle    - Handle of the file to close

Returns:

  None

--*/
;

//
// Image processing worker functions
//
EFI_STATUS
CoreDevicePathToInterface (
  IN EFI_GUID                     *Protocol,
  IN OUT EFI_DEVICE_PATH_PROTOCOL **FilePath,
  OUT VOID                        **Interface,
  OUT EFI_HANDLE                  *Handle
  )
/*++

Routine Description:

  Search a handle to a device on a specified device path that supports a specified protocol,
  interface of that protocol on that handle is another output.

Arguments:

  Protocol      - The protocol to search for

  FilePath      - The specified device path

  Interface     - Interface of the protocol on the handle

  Handle        - The handle to the device on the specified device path that supports the protocol.

Returns:

  Status code.

--*/
;

EFI_STATUS
CoreLoadPeImage (
  IN  VOID                       *Pe32Handle,
  IN  LOADED_IMAGE_PRIVATE_DATA  *Image,
  IN  EFI_PHYSICAL_ADDRESS       DstBuffer   OPTIONAL,
  OUT EFI_PHYSICAL_ADDRESS       *EntryPoint  OPTIONAL,
  IN  UINT32                     Attribute
  )
/*++

Routine Description:

  Loads, relocates, and invokes a PE/COFF image

Arguments:

  Pe32Handle       - The handle of PE32 image
  Image            - PE image to be loaded
  DstBuffer        - The buffer to store the image
  EntryPoint       - A pointer to the entry point
  Attribute        - The bit mask of attributes to set for the load PE image

Returns:

  EFI_SUCCESS           - The file was loaded, relocated, and invoked

  EFI_OUT_OF_RESOURCES  - There was not enough memory to load and relocate the PE/COFF file

  EFI_INVALID_PARAMETER - Invalid parameter

  EFI_BUFFER_TOO_SMALL  - Buffer for image is too small

--*/
;

LOADED_IMAGE_PRIVATE_DATA *
CoreLoadedImageInfo (
  IN EFI_HANDLE  ImageHandle
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  ImageHandle - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

VOID
CoreUnloadAndCloseImage (
  IN LOADED_IMAGE_PRIVATE_DATA  *Image,
  IN BOOLEAN                    FreePage
  )
/*++

Routine Description:

  Unloads EFI image from memory.

Arguments:

  Image      - EFI image
  FreePage   - Free allocated pages

Returns:

  None

--*/
;


//
// Exported Image functions
//

EFI_STATUS
EFIAPI
CoreLoadImageEx (
  IN EFI_PE32_IMAGE_PROTOCOL           *This,
  IN  EFI_HANDLE                       ParentImageHandle,
  IN  EFI_DEVICE_PATH_PROTOCOL         *FilePath,
  IN  VOID                             *SourceBuffer       OPTIONAL,
  IN  UINTN                            SourceSize,
  IN  EFI_PHYSICAL_ADDRESS             DstBuffer           OPTIONAL,
  OUT UINTN                            *NumberOfPages      OPTIONAL,
  OUT EFI_HANDLE                       *ImageHandle,
  OUT EFI_PHYSICAL_ADDRESS             *EntryPoint         OPTIONAL,
  IN  UINT32                           Attribute
  )
/*++

Routine Description:

  Loads an EFI image into memory and returns a handle to the image with extended parameters.

Arguments:

  ParentImageHandle   - The caller's image handle.
  FilePath            - The specific file path from which the image is loaded.
  SourceBuffer        - If not NULL, a pointer to the memory location containing a copy of
                        the image to be loaded.
  SourceSize          - The size in bytes of SourceBuffer.
  DstBuffer           - The buffer to store the image.
  NumberOfPages       - For input, specifies the space size of the image by caller if not NULL.
                        For output, specifies the actual space size needed.
  ImageHandle         - Image handle for output.
  EntryPoint          - Image entry point for output.
  Attribute           - The bit mask of attributes to set for the load PE image.

Returns:

  EFI_SUCCESS            - The image was loaded into memory.
  EFI_NOT_FOUND          - The FilePath was not found.
  EFI_INVALID_PARAMETER  - One of the parameters has an invalid value.
  EFI_UNSUPPORTED        - The image type is not supported, or the device path cannot be
                           parsed to locate the proper protocol for loading the file.
  EFI_OUT_OF_RESOURCES   - Image was not loaded due to insufficient resources.
--*/
;

EFI_STATUS
EFIAPI
CoreUnloadImageEx (
  IN EFI_PE32_IMAGE_PROTOCOL            *This,
  IN EFI_HANDLE                         ImageHandle
  )
/*++

Routine Description:

  Unload the specified image.

Arguments:

  This              - Indicates the calling context.

  ImageHandle       - The specified image handle.

Returns:

  EFI_INVALID_PARAMETER       - Image handle is NULL.

  EFI_UNSUPPORTED             - Attempt to unload an unsupported image.

  EFI_SUCCESS                 - Image successfully unloaded.

--*/
;
#endif
