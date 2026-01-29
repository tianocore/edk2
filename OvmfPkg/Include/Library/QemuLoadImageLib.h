/** @file
  Load a kernel image and command line passed to QEMU via
  the command line

  Copyright (C) 2020, Arm, Limited.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef QEMU_LOAD_IMAGE_LIB_H__
#define QEMU_LOAD_IMAGE_LIB_H__

#include <Uefi/UefiBaseType.h>
#include <Base.h>

#include <Protocol/LoadedImage.h>

/**
  Download the kernel, the initial ramdisk, and the kernel command line from
  QEMU's fw_cfg. The kernel will be instructed via its command line to load
  the initrd from the same Simple FileSystem where the kernel was loaded from.

  @param[out] ImageHandle       The image handle that was allocated for
                                loading the image

  @retval EFI_SUCCESS           The image was loaded successfully.
  @retval EFI_NOT_FOUND         Kernel image was not found.
  @retval EFI_OUT_OF_RESOURCES  Memory allocation failed.
  @retval EFI_PROTOCOL_ERROR    Unterminated kernel command line.
  @retval EFI_ACCESS_DENIED     The underlying LoadImage boot service call
                                returned EFI_SECURITY_VIOLATION, and the image
                                was unloaded again.

  @return                       Error codes from any of the underlying
                                functions.
**/
EFI_STATUS
EFIAPI
QemuLoadKernelImage (
  OUT EFI_HANDLE  *ImageHandle
  );

/**
  Transfer control to a kernel image loaded with QemuLoadKernelImage ()

  @param[in,out]  ImageHandle     Handle of image to be started. May assume a
                                  different value on return if the image was
                                  reloaded.

  @retval EFI_INVALID_PARAMETER   ImageHandle is either an invalid image handle
                                  or the image has already been initialized with
                                  StartImage
  @retval EFI_SECURITY_VIOLATION  The current platform policy specifies that the
                                  image should not be started.

  @return                         Error codes returned by the started image.
                                  On success, the function doesn't return.
**/
EFI_STATUS
EFIAPI
QemuStartKernelImage (
  IN  OUT EFI_HANDLE  *ImageHandle
  );

/**
  Unloads an image loaded with QemuLoadKernelImage ().

  @param  ImageHandle             Handle that identifies the image to be
                                  unloaded.

  @retval EFI_SUCCESS             The image has been unloaded.
  @retval EFI_UNSUPPORTED         The image has been started, and does not
                                  support unload.
  @retval EFI_INVALID_PARAMETER   ImageHandle is not a valid image handle.

  @return                         Exit code from the image's unload function.
**/
EFI_STATUS
EFIAPI
QemuUnloadKernelImage (
  IN  EFI_HANDLE  ImageHandle
  );

#endif
