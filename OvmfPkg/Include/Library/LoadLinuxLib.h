/** @file
  Load/boot UEFI Linux.

  Copyright (c) 2011 - 2013, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __LOAD_LINUX_LIB__
#define __LOAD_LINUX_LIB__


/**
  Verifies that the kernel setup image is valid and supported.
  The kernel setup image should be checked before using other library
  routines which take the kernel setup as an input.

  @param[in]     KernelSetup - The kernel setup image
  @param[in]     KernelSetupSize - The kernel setup size

  @retval    EFI_SUCCESS - The Linux kernel setup is valid and supported
  @retval    EFI_INVALID_PARAMETER - KernelSetup was NULL
  @retval    EFI_UNSUPPORTED - The Linux kernel is not supported

**/
EFI_STATUS
EFIAPI
LoadLinuxCheckKernelSetup (
  IN VOID        *KernelSetup,
  IN UINTN       KernelSetupSize
  );


/**
  Gets the initial runtime size of the Linux kernel image by examining
  the kernel setup image.

  @param[in]     KernelSetup - The kernel setup image
  @param[in]     KernelSize - The kernel size on disk.

  @retval    0                An error occured
  @retval    !0               The initial size required by the kernel to
                              begin execution.

**/
UINTN
EFIAPI
LoadLinuxGetKernelSize (
  IN VOID        *KernelSetup,
  IN UINTN       KernelSize
  );


/**
  Loads and boots UEFI Linux.

  Note: If successful, then this routine will not return

  @param[in]     Kernel - The main kernel image
  @param[in,out] KernelSetup - The kernel setup image

  @retval    EFI_NOT_FOUND - The Linux kernel was not found
  @retval    EFI_INVALID_PARAMETER - Kernel or KernelSetup was NULL
  @retval    EFI_UNSUPPORTED - The Linux kernel version is not supported

**/
EFI_STATUS
EFIAPI
LoadLinux (
  IN VOID      *Kernel,
  IN OUT VOID  *KernelSetup
  );


/**
  Allocates pages for the kernel setup image.

  @param[in]     Pages - The number of pages

  @retval    NULL - Unable to allocate pages
  @retval    !NULL - The address of the pages allocated

**/
VOID*
EFIAPI
LoadLinuxAllocateKernelSetupPages (
  IN UINTN                  Pages
  );


/**
  Clears the uninitialised space before and after the struct setup_header
  in the kernel setup image. The kernel requires that these be zeroed
  unless explicitly initialised, so this function should be called after
  the setup_header has been copied in from a bzImage, before setting up
  anything else.

  @param[in]     KernelSetup - The kernel setup image

  @retval    EFI_SUCCESS - The Linux kernel setup was successfully initialized
  @retval    EFI_INVALID_PARAMETER - KernelSetup was NULL
  @retval    EFI_UNSUPPORTED - The Linux kernel is not supported

**/
EFI_STATUS
EFIAPI
LoadLinuxInitializeKernelSetup (
  IN VOID        *KernelSetup
  );

/**
  Allocates pages for the kernel.

  @param[in]     KernelSetup - The kernel setup image
  @param[in]     Pages - The number of pages. (It is recommended to use the
                         size returned from LoadLinuxGetKernelSize.)

  @retval    NULL - Unable to allocate pages
  @retval    !NULL - The address of the pages allocated

**/
VOID*
EFIAPI
LoadLinuxAllocateKernelPages (
  IN VOID                   *KernelSetup,
  IN UINTN                  Pages
  );


/**
  Allocates pages for the kernel command line.

  @param[in]     Pages - The number of pages.

  @retval    NULL - Unable to allocate pages
  @retval    !NULL - The address of the pages allocated

**/
VOID*
EFIAPI
LoadLinuxAllocateCommandLinePages (
  IN UINTN                  Pages
  );


/**
  Allocates pages for the initrd image.

  @param[in,out] KernelSetup - The kernel setup image
  @param[in]     Pages - The number of pages.

  @retval    NULL - Unable to allocate pages
  @retval    !NULL - The address of the pages allocated

**/
VOID*
EFIAPI
LoadLinuxAllocateInitrdPages (
  IN VOID                   *KernelSetup,
  IN UINTN                  Pages
  );


/**
  Sets the kernel command line parameter within the setup image.

  @param[in,out] KernelSetup - The kernel setup image
  @param[in]     CommandLine - The kernel command line

  @retval    EFI_SUCCESS - The Linux kernel setup is valid and supported
  @retval    EFI_INVALID_PARAMETER - KernelSetup was NULL
  @retval    EFI_UNSUPPORTED - The Linux kernel is not supported

**/
EFI_STATUS
EFIAPI
LoadLinuxSetCommandLine (
  IN OUT VOID    *KernelSetup,
  IN CHAR8       *CommandLine
  );


/**
  Sets the kernel initial ram disk pointer within the setup image.

  @param[in,out] KernelSetup - The kernel setup image
  @param[in]     Initrd - Pointer to the initial ram disk
  @param[in]     InitrdSize - The initial ram disk image size

  @retval    EFI_SUCCESS - The Linux kernel setup is valid and supported
  @retval    EFI_INVALID_PARAMETER - KernelSetup was NULL
  @retval    EFI_UNSUPPORTED - The Linux kernel is not supported

**/
EFI_STATUS
EFIAPI
LoadLinuxSetInitrd (
  IN OUT VOID    *KernelSetup,
  IN VOID        *Initrd,
  IN UINTN       InitrdSize
  );


#endif

