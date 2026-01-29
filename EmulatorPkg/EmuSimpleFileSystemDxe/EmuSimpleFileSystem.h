/*++ @file
  Produce Simple File System abstractions for a directory on your PC using Unix APIs.
  The configuration of what devices to mount or emulate comes from
  environment variables.

Copyright (c) 2006 - 2009, Intel Corporation. All rights reserved.<BR>
Portions copyright (c) 2011, Apple Inc. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent


**/

#ifndef _EMU_SIMPLE_FILE_SYSTEM_H_
#define _EMU_SIMPLE_FILE_SYSTEM_H_

#include "PiDxe.h"

#include <Guid/FileSystemInfo.h>
#include <Guid/FileInfo.h>
#include <Guid/FileSystemVolumeLabelInfo.h>

#include <Protocol/EmuIoThunk.h>
#include <Protocol/SimpleFileSystem.h>

#include <Library/DebugLib.h>
#include <Library/BaseLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>

extern EFI_DRIVER_BINDING_PROTOCOL   gEmuSimpleFileSystemDriverBinding;
extern EFI_COMPONENT_NAME_PROTOCOL   gEmuSimpleFileSystemComponentName;
extern EFI_COMPONENT_NAME2_PROTOCOL  gEmuSimpleFileSystemComponentName2;

#define EMU_SIMPLE_FILE_SYSTEM_PRIVATE_SIGNATURE  SIGNATURE_32 ('E', 'M', 'f', 's')

typedef struct {
  UINTN                              Signature;
  EMU_IO_THUNK_PROTOCOL              *IoThunk;
  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL    SimpleFileSystem;
  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL    *Io;
  EFI_UNICODE_STRING_TABLE           *ControllerNameTable;
} EMU_SIMPLE_FILE_SYSTEM_PRIVATE;

#define EMU_SIMPLE_FILE_SYSTEM_PRIVATE_DATA_FROM_THIS(a) \
  CR (a, \
      EMU_SIMPLE_FILE_SYSTEM_PRIVATE, \
      SimpleFileSystem, \
      EMU_SIMPLE_FILE_SYSTEM_PRIVATE_SIGNATURE \
      )

#define EMU_EFI_FILE_PRIVATE_SIGNATURE  SIGNATURE_32 ('e', 'm', 'f', 's')

typedef struct {
  UINTN                              Signature;
  EMU_IO_THUNK_PROTOCOL              *IoThunk;
  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL    *SimpleFileSystem;
  EFI_FILE_PROTOCOL                  EfiFile;
  EFI_FILE_PROTOCOL                  *Io;
} EMU_EFI_FILE_PRIVATE;

#define EMU_EFI_FILE_PRIVATE_DATA_FROM_THIS(a) \
  CR (a, \
      EMU_EFI_FILE_PRIVATE, \
      EfiFile, \
      EMU_EFI_FILE_PRIVATE_SIGNATURE \
      )

#endif
