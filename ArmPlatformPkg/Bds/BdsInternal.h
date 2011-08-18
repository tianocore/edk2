/** @file
*
*  Copyright (c) 2011, ARM Limited. All rights reserved.
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

#ifndef _BDSINTERNAL_H_
#define _BDSINTERNAL_H_

#include <PiDxe.h>
#include <Library/BaseMemoryLib.h>
#include <Library/BdsLib.h>
#include <Library/BdsUnixLib.h>
#include <Library/DebugLib.h>
#include <Library/DevicePathLib.h>
#include <Library/UefiLib.h>
#include <Library/PrintLib.h>
#include <Library/PcdLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>

#include <Protocol/DevicePathFromText.h>
#include <Protocol/DevicePathToText.h>

#include <Guid/GlobalVariable.h>

#define BOOT_DEVICE_DESCRIPTION_MAX   100
#define BOOT_DEVICE_FILEPATH_MAX      100
#define BOOT_DEVICE_OPTION_MAX        300
#define BOOT_DEVICE_ADDRESS_MAX       20

typedef enum {
    BDS_LOADER_EFI_APPLICATION = 0,
    BDS_LOADER_KERNEL_LINUX_ATAG,
    BDS_LOADER_KERNEL_LINUX_FDT,
} BDS_LOADER_TYPE;

typedef struct{
  UINT16                     InitrdPathListLength;
  EFI_DEVICE_PATH_PROTOCOL   *InitrdPathList;
  CHAR8                      CmdLine[BOOT_DEVICE_OPTION_MAX + 1];
} BDS_LINUX_ATAG_ARGUMENTS;

typedef union {
  BDS_LINUX_ATAG_ARGUMENTS     LinuxAtagArguments;
} BDS_LOADER_ARGUMENTS;

typedef struct {
  BDS_LOADER_TYPE            LoaderType;
  BDS_LOADER_ARGUMENTS       Arguments;
} BDS_LOADER_OPTIONAL_DATA;

typedef enum {
  BDS_DEVICE_FILESYSTEM = 0,
  BDS_DEVICE_MEMMAP,
  BDS_DEVICE_PXE,
  BDS_DEVICE_TFTP,
  BDS_DEVICE_MAX
} BDS_SUPPORTED_DEVICE_TYPE;

typedef struct {
  LIST_ENTRY                  Link;
  CHAR16                      Description[BOOT_DEVICE_DESCRIPTION_MAX];
  EFI_DEVICE_PATH_PROTOCOL*   DevicePathProtocol;
  struct _BDS_LOAD_OPTION_SUPPORT*    Support;
} BDS_SUPPORTED_DEVICE;

#define SUPPORTED_BOOT_DEVICE_FROM_LINK(a)   BASE_CR(a, BDS_SUPPORTED_DEVICE, Link)

typedef UINT8* EFI_LOAD_OPTION;

/* This is defined by the UEFI specs, don't change it */
typedef struct {
  LIST_ENTRY                  Link;

  UINT16                      LoadOptionIndex;
  EFI_LOAD_OPTION             LoadOption;
  UINTN                       LoadOptionSize;

  UINT32                      Attributes;
  UINT16                      FilePathListLength;
  CHAR16                      *Description;
  EFI_DEVICE_PATH_PROTOCOL    *FilePathList;
  BDS_LOADER_OPTIONAL_DATA    *OptionalData;
} BDS_LOAD_OPTION;

typedef struct _BDS_LOAD_OPTION_SUPPORT {
  BDS_SUPPORTED_DEVICE_TYPE   Type;
  EFI_STATUS    (*ListDevices)(IN OUT LIST_ENTRY* BdsLoadOptionList);
  BOOLEAN       (*IsSupported)(IN BDS_LOAD_OPTION* BdsLoadOption);
  EFI_STATUS    (*CreateDevicePathNode)(IN BDS_SUPPORTED_DEVICE* BdsLoadOption, OUT EFI_DEVICE_PATH_PROTOCOL **DevicePathNode, OUT BDS_LOADER_TYPE *BootType, OUT UINT32 *Attributes);
  EFI_STATUS    (*UpdateDevicePathNode)(IN EFI_DEVICE_PATH *OldDevicePath, OUT EFI_DEVICE_PATH_PROTOCOL** NewDevicePath, OUT BDS_LOADER_TYPE *BootType, OUT UINT32 *Attributes);
} BDS_LOAD_OPTION_SUPPORT;

#define LOAD_OPTION_FROM_LINK(a)   BASE_CR(a, BDS_LOAD_OPTION, Link)

EFI_STATUS
GetEnvironmentVariable (
  IN     CONST CHAR16*   VariableName,
  IN     VOID*           DefaultValue,
  IN OUT UINTN*          Size,
  OUT    VOID**          Value
  );

EFI_STATUS
BootDeviceListSupportedInit (
  IN OUT LIST_ENTRY *SupportedDeviceList
  );

EFI_STATUS
BootDeviceListSupportedFree (
  IN LIST_ENTRY *SupportedDeviceList,
  IN BDS_SUPPORTED_DEVICE *Except
  );

EFI_STATUS
BootDeviceGetDeviceSupport (
  IN  BDS_LOAD_OPTION *BootOption,
  OUT BDS_LOAD_OPTION_SUPPORT**  DeviceSupport
  );

EFI_STATUS
GetHIInputAscii (
  IN OUT CHAR8   *CmdLine,
  IN     UINTN   MaxCmdLine
  );

EFI_STATUS
EditHIInputAscii (
  IN OUT CHAR8   *CmdLine,
  IN     UINTN   MaxCmdLine
  );

EFI_STATUS
GetHIInputInteger (
  IN OUT UINTN   *Integer
  );

EFI_STATUS
GetHIInputIP (
  OUT EFI_IP_ADDRESS   *Ip
  );

EFI_STATUS
GetHIInputBoolean (
  OUT BOOLEAN *Value
  );

BOOLEAN
HasFilePathEfiExtension (
  IN CHAR16* FilePath
  );

EFI_DEVICE_PATH*
GetLastDevicePathNode (
  IN EFI_DEVICE_PATH*  DevicePath
  );

EFI_STATUS
BdsStartBootOption (
  IN CHAR16* BootOption
  );

EFI_STATUS
GenerateDeviceDescriptionName (
  IN  EFI_HANDLE  Handle,
  IN OUT CHAR16*  Description
  );

EFI_STATUS
BootOptionList (
  IN OUT LIST_ENTRY *BootOptionList
  );

EFI_STATUS
BootOptionParseLoadOption (
  IN  EFI_LOAD_OPTION EfiLoadOption,
  IN  UINTN           EfiLoadOptionSize,
  OUT BDS_LOAD_OPTION **BdsLoadOption
  );

EFI_STATUS
BootOptionStart (
  IN BDS_LOAD_OPTION *BootOption
  );

EFI_STATUS
BootOptionCreate (
  IN  UINT32 Attributes,
  IN  CHAR16* BootDescription,
  IN  EFI_DEVICE_PATH_PROTOCOL* DevicePath,
  IN  BDS_LOADER_TYPE   BootType,
  IN  BDS_LOADER_ARGUMENTS *BootArguments,
  OUT BDS_LOAD_OPTION **BdsLoadOption
  );

EFI_STATUS
BootOptionUpdate (
  IN  BDS_LOAD_OPTION *BdsLoadOption,
  IN  UINT32 Attributes,
  IN  CHAR16* BootDescription,
  IN  EFI_DEVICE_PATH_PROTOCOL* DevicePath,
  IN  BDS_LOADER_TYPE   BootType,
  IN  BDS_LOADER_ARGUMENTS *BootArguments
  );

EFI_STATUS
BootOptionDelete (
  IN  BDS_LOAD_OPTION *BootOption
  );

EFI_STATUS
BootMenuMain (
  VOID
  );

#endif /* _BDSINTERNAL_H_ */
