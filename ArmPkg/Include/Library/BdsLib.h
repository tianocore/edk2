/** @file
*
*  Copyright (c) 2013, ARM Limited. All rights reserved.
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

#ifndef __BDS_ENTRY_H__
#define __BDS_ENTRY_H__

typedef UINT8* EFI_LOAD_OPTION;

/**
  This is defined by the UEFI specs, don't change it
**/
typedef struct {
  UINT16                      LoadOptionIndex;
  EFI_LOAD_OPTION             LoadOption;
  UINTN                       LoadOptionSize;

  UINT32                      Attributes;
  UINT16                      FilePathListLength;
  CHAR16                      *Description;
  EFI_DEVICE_PATH_PROTOCOL    *FilePathList;

  VOID*                       OptionalData;
  UINTN                       OptionalDataSize;
} BDS_LOAD_OPTION;

/**
  Connect a Device Path and return the handle of the driver that support this DevicePath

  @param  DevicePath            Device Path of the File to connect
  @param  Handle                Handle of the driver that support this DevicePath
  @param  RemainingDevicePath   Remaining DevicePath nodes that do not match the driver DevicePath

  @retval EFI_SUCCESS           A driver that matches the Device Path has been found
  @retval EFI_NOT_FOUND         No handles match the search.
  @retval EFI_INVALID_PARAMETER DevicePath or Handle is NULL

**/
EFI_STATUS
BdsConnectDevicePath (
  IN  EFI_DEVICE_PATH_PROTOCOL* DevicePath,
  OUT EFI_HANDLE                *Handle,
  OUT EFI_DEVICE_PATH_PROTOCOL  **RemainingDevicePath
  );

/**
  Connect all DXE drivers

  @retval EFI_SUCCESS           All drivers have been connected
  @retval EFI_NOT_FOUND         No handles match the search.
  @retval EFI_OUT_OF_RESOURCES  There is not resource pool memory to store the matching results.

**/
EFI_STATUS
BdsConnectAllDrivers (
  VOID
  );

EFI_STATUS
GetGlobalEnvironmentVariable (
  IN     CONST CHAR16*   VariableName,
  IN     VOID*           DefaultValue,
  IN OUT UINTN*          Size,
  OUT    VOID**          Value
  );

EFI_STATUS
GetEnvironmentVariable (
  IN     CONST CHAR16*   VariableName,
  IN     EFI_GUID*       VendorGuid,
  IN     VOID*           DefaultValue,
  IN OUT UINTN*          Size,
  OUT    VOID**          Value
  );

EFI_STATUS
BootOptionFromLoadOptionIndex (
  IN  UINT16            LoadOptionIndex,
  OUT BDS_LOAD_OPTION** BdsLoadOption
  );

EFI_STATUS
BootOptionFromLoadOptionVariable (
  IN  CHAR16*           BootVariableName,
  OUT BDS_LOAD_OPTION** BdsLoadOption
  );

EFI_STATUS
BootOptionToLoadOptionVariable (
  IN BDS_LOAD_OPTION*   BdsLoadOption
  );

UINT16
BootOptionAllocateBootIndex (
  VOID
  );

/**
  Start a Linux kernel from a Device Path

  @param  LinuxKernel           Device Path to the Linux Kernel
  @param  Parameters            Linux kernel arguments

  @retval EFI_SUCCESS           All drivers have been connected
  @retval EFI_NOT_FOUND         The Linux kernel Device Path has not been found
  @retval EFI_OUT_OF_RESOURCES  There is not enough resource memory to store the matching results.

**/
EFI_STATUS
BdsBootLinuxAtag (
  IN  EFI_DEVICE_PATH_PROTOCOL* LinuxKernelDevicePath,
  IN  EFI_DEVICE_PATH_PROTOCOL* InitrdDevicePath,
  IN  CONST CHAR8*              Arguments
  );

/**
  Start a Linux kernel from a Device Path

  @param  LinuxKernel           Device Path to the Linux Kernel
  @param  Parameters            Linux kernel arguments
  @param  Fdt                   Device Path to the Flat Device Tree

  @retval EFI_SUCCESS           All drivers have been connected
  @retval EFI_NOT_FOUND         The Linux kernel Device Path has not been found
  @retval EFI_OUT_OF_RESOURCES  There is not enough resource memory to store the matching results.

**/
EFI_STATUS
BdsBootLinuxFdt (
  IN  EFI_DEVICE_PATH_PROTOCOL* LinuxKernelDevicePath,
  IN  EFI_DEVICE_PATH_PROTOCOL* InitrdDevicePath,
  IN  CONST CHAR8*              Arguments,
  IN  EFI_DEVICE_PATH_PROTOCOL* FdtDevicePath
  );

/**
  Start an EFI Application from a Device Path

  @param  ParentImageHandle     Handle of the calling image
  @param  DevicePath            Location of the EFI Application

  @retval EFI_SUCCESS           All drivers have been connected
  @retval EFI_NOT_FOUND         The Linux kernel Device Path has not been found
  @retval EFI_OUT_OF_RESOURCES  There is not enough resource memory to store the matching results.

**/
EFI_STATUS
BdsStartEfiApplication (
  IN EFI_HANDLE                  ParentImageHandle,
  IN EFI_DEVICE_PATH_PROTOCOL    *DevicePath,
  IN UINTN                       LoadOptionsSize,
  IN VOID*                       LoadOptions
  );

/**
  Start an EFI Application from any Firmware Volume

  @param  EfiApp                EFI Application Name

  @retval EFI_SUCCESS           All drivers have been connected
  @retval EFI_NOT_FOUND         The Linux kernel Device Path has not been found
  @retval EFI_OUT_OF_RESOURCES  There is not enough resource memory to store the matching results.

**/
EFI_STATUS
BdsLoadApplication (
  IN EFI_HANDLE                  ParentImageHandle,
  IN CHAR16*                     EfiApp,
  IN UINTN                       LoadOptionsSize,
  IN VOID*                       LoadOptions
  );

EFI_STATUS
BdsLoadImage (
  IN     EFI_DEVICE_PATH       *DevicePath,
  IN     EFI_ALLOCATE_TYPE     Type,
  IN OUT EFI_PHYSICAL_ADDRESS* Image,
  OUT    UINTN                 *FileSize
  );

#endif
