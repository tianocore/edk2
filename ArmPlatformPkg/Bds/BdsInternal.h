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
#define BOOT_DEVICE_ADDRESS_MAX       (sizeof(L"0x0000000000000000"))

#define ARM_BDS_OPTIONAL_DATA_SIGNATURE   SIGNATURE_32('a', 'b', 'o', 'd')

#define IS_ARM_BDS_BOOTENTRY(ptr)  (ReadUnaligned32 ((CONST UINT32*)&((ARM_BDS_LOADER_OPTIONAL_DATA*)((ptr)->OptionalData))->Header.Signature) == ARM_BDS_OPTIONAL_DATA_SIGNATURE)

#define UPDATE_BOOT_ENTRY L"Update entry: "
#define DELETE_BOOT_ENTRY L"Delete entry: "

typedef enum {
    BDS_LOADER_EFI_APPLICATION = 0,
    BDS_LOADER_KERNEL_LINUX_ATAG,
    BDS_LOADER_KERNEL_LINUX_FDT,
} ARM_BDS_LOADER_TYPE;

typedef struct {
  UINT16                     CmdLineSize;
  UINT16                     InitrdSize;

  // These following fields have variable length and are packed:
  //CHAR8                      *CmdLine;
  //EFI_DEVICE_PATH_PROTOCOL   *InitrdPathList;
} ARM_BDS_LINUX_ARGUMENTS;

typedef union {
  ARM_BDS_LINUX_ARGUMENTS    LinuxArguments;
} ARM_BDS_LOADER_ARGUMENTS;

typedef struct {
  UINT32                     Signature;
  ARM_BDS_LOADER_TYPE        LoaderType;
} ARM_BDS_LOADER_OPTIONAL_DATA_HEADER;

typedef struct {
  ARM_BDS_LOADER_OPTIONAL_DATA_HEADER Header;
  ARM_BDS_LOADER_ARGUMENTS            Arguments;
} ARM_BDS_LOADER_OPTIONAL_DATA;

typedef struct {
  LIST_ENTRY                  Link;
  BDS_LOAD_OPTION*            BdsLoadOption;
} BDS_LOAD_OPTION_ENTRY;

typedef enum {
  BDS_DEVICE_FILESYSTEM = 0,
  BDS_DEVICE_MEMMAP,
  BDS_DEVICE_PXE,
  BDS_DEVICE_TFTP,
  BDS_DEVICE_MAX
} BDS_SUPPORTED_DEVICE_TYPE;

typedef struct {
  LIST_ENTRY                          Link;
  CHAR16                              Description[BOOT_DEVICE_DESCRIPTION_MAX];
  EFI_DEVICE_PATH_PROTOCOL*           DevicePathProtocol;
  struct _BDS_LOAD_OPTION_SUPPORT*    Support;
} BDS_SUPPORTED_DEVICE;

#define SUPPORTED_BOOT_DEVICE_FROM_LINK(a)   BASE_CR(a, BDS_SUPPORTED_DEVICE, Link)

typedef struct _BDS_LOAD_OPTION_SUPPORT {
  BDS_SUPPORTED_DEVICE_TYPE   Type;
  EFI_STATUS    (*ListDevices)(IN OUT LIST_ENTRY* BdsLoadOptionList);
  BOOLEAN       (*IsSupported)(IN  EFI_DEVICE_PATH *DevicePath);
  EFI_STATUS    (*CreateDevicePathNode)(IN CHAR16* FileName, OUT EFI_DEVICE_PATH_PROTOCOL **DevicePathNodes, OUT ARM_BDS_LOADER_TYPE *BootType, OUT UINT32 *Attributes);
  EFI_STATUS    (*UpdateDevicePathNode)(IN EFI_DEVICE_PATH *OldDevicePath, IN CHAR16* FileName, OUT EFI_DEVICE_PATH_PROTOCOL** NewDevicePath, OUT ARM_BDS_LOADER_TYPE *BootType, OUT UINT32 *Attributes);
} BDS_LOAD_OPTION_SUPPORT;

#define LOAD_OPTION_ENTRY_FROM_LINK(a)  BASE_CR(a, BDS_LOAD_OPTION_ENTRY, Link)
#define LOAD_OPTION_FROM_LINK(a)        ((BDS_LOAD_OPTION_ENTRY*)BASE_CR(a, BDS_LOAD_OPTION_ENTRY, Link))->BdsLoadOption

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
  IN  EFI_DEVICE_PATH           *DevicePath,
  OUT BDS_LOAD_OPTION_SUPPORT   **DeviceSupport
  );

EFI_STATUS
GetHIInputStr (
  IN OUT CHAR16  *CmdLine,
  IN     UINTN   MaxCmdLine
  );

EFI_STATUS
EditHIInputStr (
  IN OUT CHAR16  *CmdLine,
  IN     UINTN   MaxCmdLine
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

UINTN
GetUnalignedDevicePathSize (
  IN EFI_DEVICE_PATH* DevicePath
  );

EFI_DEVICE_PATH*
GetAlignedDevicePath (
  IN EFI_DEVICE_PATH* DevicePath
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
  IN  UINT32                    Attributes,
  IN  CHAR16*                   BootDescription,
  IN  EFI_DEVICE_PATH_PROTOCOL* DevicePath,
  IN  ARM_BDS_LOADER_TYPE       BootType,
  IN  ARM_BDS_LOADER_ARGUMENTS* BootArguments,
  OUT BDS_LOAD_OPTION**         BdsLoadOption
  );

EFI_STATUS
BootOptionUpdate (
  IN  BDS_LOAD_OPTION*          BdsLoadOption,
  IN  UINT32                    Attributes,
  IN  CHAR16*                   BootDescription,
  IN  EFI_DEVICE_PATH_PROTOCOL* DevicePath,
  IN  ARM_BDS_LOADER_TYPE       BootType,
  IN  ARM_BDS_LOADER_ARGUMENTS* BootArguments
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
