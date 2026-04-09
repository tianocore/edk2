/** @file
  Implementation of image related services in the UEFI Boot Services table for use in unit tests.

Copyright (c) Microsoft Corporation
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "UnitTestUefiBootServicesTableLib.h"

/**
  Loads an EFI image into memory and returns a handle to the image.

  @param  BootPolicy              If TRUE, indicates that the request originates
                                  from the boot manager, and that the boot
                                  manager is attempting to load FilePath as a
                                  boot selection.
  @param  ParentImageHandle       The caller's image handle.
  @param  FilePath                The specific file path from which the image is
                                  loaded.
  @param  SourceBuffer            If not NULL, a pointer to the memory location
                                  containing a copy of the image to be loaded.
  @param  SourceSize              The size in bytes of SourceBuffer.
  @param  ImageHandle             Pointer to the returned image handle that is
                                  created when the image is successfully loaded.

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
UnitTestLoadImage (
  IN BOOLEAN                   BootPolicy,
  IN EFI_HANDLE                ParentImageHandle,
  IN EFI_DEVICE_PATH_PROTOCOL  *FilePath,
  IN VOID                      *SourceBuffer   OPTIONAL,
  IN UINTN                     SourceSize,
  OUT EFI_HANDLE               *ImageHandle
  )
{
  return EFI_NOT_AVAILABLE_YET;
}

/**
  Transfer control to a loaded image's entry point.

  @param  ImageHandle             Handle of image to be started.
  @param  ExitDataSize            Pointer of the size to ExitData
  @param  ExitData                Pointer to a pointer to a data buffer that
                                  includes a Null-terminated string,
                                  optionally followed by additional binary data.
                                  The string is a description that the caller may
                                  use to further indicate the reason for the
                                  image's exit.

  @retval EFI_INVALID_PARAMETER   Invalid parameter
  @retval EFI_OUT_OF_RESOURCES    No enough buffer to allocate
  @retval EFI_SECURITY_VIOLATION  The current platform policy specifies that the image should not be started.
  @retval EFI_SUCCESS             Successfully transfer control to the image's
                                  entry point.

**/
EFI_STATUS
EFIAPI
UnitTestStartImage (
  IN EFI_HANDLE  ImageHandle,
  OUT UINTN      *ExitDataSize,
  OUT CHAR16     **ExitData  OPTIONAL
  )
{
  return EFI_NOT_AVAILABLE_YET;
}

/**
  Terminates the currently loaded EFI image and returns control to boot services.

  @param  ImageHandle             Handle that identifies the image. This
                                  parameter is passed to the image on entry.
  @param  Status                  The image's exit code.
  @param  ExitDataSize            The size, in bytes, of ExitData. Ignored if
                                  ExitStatus is EFI_SUCCESS.
  @param  ExitData                Pointer to a data buffer that includes a
                                  Null-terminated Unicode string, optionally
                                  followed by additional binary data. The string
                                  is a description that the caller may use to
                                  further indicate the reason for the image's
                                  exit.

  @retval EFI_INVALID_PARAMETER   Image handle is NULL or it is not current
                                  image.
  @retval EFI_SUCCESS             Successfully terminates the currently loaded
                                  EFI image.
  @retval EFI_ACCESS_DENIED       Should never reach there.
  @retval EFI_OUT_OF_RESOURCES    Could not allocate pool

**/
EFI_STATUS
EFIAPI
UnitTestExit (
  IN EFI_HANDLE  ImageHandle,
  IN EFI_STATUS  Status,
  IN UINTN       ExitDataSize,
  IN CHAR16      *ExitData  OPTIONAL
  )
{
  return EFI_NOT_AVAILABLE_YET;
}

/**
  Unloads an image.

  @param  ImageHandle             Handle that identifies the image to be
                                  unloaded.

  @retval EFI_SUCCESS             The image has been unloaded.
  @retval EFI_UNSUPPORTED         The image has been started, and does not support
                                  unload.
  @retval EFI_INVALID_PARAMPETER  ImageHandle is not a valid image handle.

**/
EFI_STATUS
EFIAPI
UnitTestUnloadImage (
  IN EFI_HANDLE  ImageHandle
  )
{
  return EFI_NOT_AVAILABLE_YET;
}

/**
  Terminates all boot services.

  @param  ImageHandle            Handle that identifies the exiting image.
  @param  MapKey                 Key to the latest memory map.

  @retval EFI_SUCCESS            Boot Services terminated
  @retval EFI_INVALID_PARAMETER  MapKey is incorrect.

**/
EFI_STATUS
EFIAPI
UnitTestExitBootServices (
  IN EFI_HANDLE  ImageHandle,
  IN UINTN       MapKey
  )
{
  return EFI_NOT_AVAILABLE_YET;
}
