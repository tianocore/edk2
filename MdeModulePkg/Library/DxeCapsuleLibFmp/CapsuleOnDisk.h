/** @file
  Defines several datastructures used by Capsule On Disk feature.
  They are mainly used for FAT files.

  Copyright (c) 2019, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _CAPSULES_ON_DISK_H_
#define _CAPSULES_ON_DISK_H_

#include <Uefi.h>
#include <Pi/PiMultiPhase.h>

#include <Library/UefiLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/UefiRuntimeLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/FileHandleLib.h>
#include <Library/CapsuleLib.h>
#include <Library/DevicePathLib.h>
#include <Library/PrintLib.h>
#include <Library/UefiBootManagerLib.h>

#include <Protocol/SimpleFileSystem.h>
#include <Protocol/DiskIo.h>
#include <Protocol/BlockIo.h>

#include <Guid/CapsuleVendor.h>
#include <Guid/FileInfo.h>
#include <Guid/GlobalVariable.h>

//
// This data structure is the part of FILE_INFO_ENTRY
//
#define FILE_INFO_SIGNATURE SIGNATURE_32 ('F', 'L', 'I', 'F')

//
// LoadOptionNumber of the boot option where the capsules is relocated.
//
#define COD_RELOCATION_LOAD_OPTION_VAR_NAME   L"CodRelocationLoadOption"

//
// (20 * (6+5+2))+1) unicode characters from EFI FAT spec (doubled for bytes)
//
#define MAX_FILE_NAME_SIZE   522
#define MAX_FILE_NAME_LEN    (MAX_FILE_NAME_SIZE / sizeof(CHAR16))
#define MAX_FILE_INFO_LEN    (OFFSET_OF(EFI_FILE_INFO, FileName) + MAX_FILE_NAME_LEN)

typedef struct {
  UINTN           Signature;
  LIST_ENTRY      Link;                  ///  Linked list members.
  EFI_FILE_INFO   *FileInfo;             ///  Pointer to the FileInfo struct for this file or NULL.
  CHAR16          *FileNameFirstPart;    ///  Text to the left of right-most period in the file name. String is capitialized
  CHAR16          *FileNameSecondPart;   ///  Text to the right of right-most period in the file name.String is capitialized. Maybe NULL
} FILE_INFO_ENTRY;

typedef struct {
  //
  // image address.
  //
  VOID             *ImageAddress;
  //
  // The file info of the image comes from.
  //  if FileInfo == NULL. means image does not come from file
  //
  EFI_FILE_INFO    *FileInfo;
} IMAGE_INFO;

#endif // _CAPSULES_ON_DISK_H_
