/** @file
*
*  Copyright (c) 2011-2013, ARM Limited. All rights reserved.
*
*  This program and the accompanying materials
*  are licensed and made available under the terms and conditions of the BSD License
*  which accompanies this distribution.  The full text of the license may be found at
*  http://opensource.org/licenses/bsd-license.php
*
*  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
*  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
*
**/

#ifndef __LOADER_INTERNAL_H
#define __LOADER_INTERNAL_H

#include <Uefi.h>
#include <Library/BaseMemoryLib.h>
#include <Library/BdsLib.h>
#include <Library/DebugLib.h>
#include <Library/DevicePathLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>

#include <Protocol/LoadedImage.h>
#include <Protocol/DevicePathToText.h>

#define LINUX_KERNEL_NAME               L"zImage"
#define FDT_NAME                        L"platform.dtb"

#define LINUX_LOADER_SIGNATURE    SIGNATURE_32('l', 'i', 'l', 'o')

typedef struct {
  UINT32                        Signature;
  UINT16                        CmdLineLength;
  UINT16                        InitrdPathListLength;

  // These following fields have variable length:
  //CHAR8*                      CmdLine;
  //CHAR16*                     Initrd;
} LINUX_LOADER_OPTIONAL_DATA;

EFI_STATUS
LinuxLoaderConfig (
  IN EFI_LOADED_IMAGE_PROTOCOL   *LoadedImage
  );

#endif
