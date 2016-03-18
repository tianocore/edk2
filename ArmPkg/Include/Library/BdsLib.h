/** @file
*
*  Copyright (c) 2013-2015, ARM Limited. All rights reserved.
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

/**
  This is defined by the UEFI specs, don't change it
**/
typedef struct {
  UINT16                      LoadOptionIndex;
  EFI_LOAD_OPTION             *LoadOption;
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

/**
  Return the value of a global variable defined by its VariableName.
  The variable must be defined with the VendorGuid gEfiGlobalVariableGuid.

  @param  VariableName          A Null-terminated string that is the name of the vendor's
                                variable.
  @param  DefaultValue          Value returned by the function if the variable does not exist
  @param  DataSize              On input, the size in bytes of the return Data buffer.
                                On output the size of data returned in Data.
  @param  Value                 Value read from the UEFI Variable or copy of the default value
                                if the UEFI Variable does not exist

  @retval EFI_SUCCESS           All drivers have been connected
  @retval EFI_NOT_FOUND         No handles match the search.
  @retval EFI_OUT_OF_RESOURCES  There is not resource pool memory to store the matching results.

**/
EFI_STATUS
GetGlobalEnvironmentVariable (
  IN     CONST CHAR16*   VariableName,
  IN     VOID*           DefaultValue,
  IN OUT UINTN*          Size,
  OUT    VOID**          Value
  );

/**
  Return the value of the variable defined by its VariableName and VendorGuid

  @param  VariableName          A Null-terminated string that is the name of the vendor's
                                variable.
  @param  VendorGuid            A unique identifier for the vendor.
  @param  DefaultValue          Value returned by the function if the variable does not exist
  @param  DataSize              On input, the size in bytes of the return Data buffer.
                                On output the size of data returned in Data.
  @param  Value                 Value read from the UEFI Variable or copy of the default value
                                if the UEFI Variable does not exist

  @retval EFI_SUCCESS           All drivers have been connected
  @retval EFI_NOT_FOUND         No handles match the search.
  @retval EFI_OUT_OF_RESOURCES  There is not resource pool memory to store the matching results.

**/
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

EFI_STATUS
BdsLoadImage (
  IN     EFI_DEVICE_PATH       *DevicePath,
  IN     EFI_ALLOCATE_TYPE     Type,
  IN OUT EFI_PHYSICAL_ADDRESS* Image,
  OUT    UINTN                 *FileSize
  );

/**
 * Call BS.ExitBootServices with the appropriate Memory Map information
 */
EFI_STATUS
ShutdownUefiBootServices (
  VOID
  );

/**
  Locate an EFI application in a the Firmware Volumes by its name

  @param  EfiAppGuid            Guid of the EFI Application into the Firmware Volume
  @param  DevicePath            EFI Device Path of the EFI application

  @return EFI_SUCCESS           The function completed successfully.
  @return EFI_NOT_FOUND         The protocol could not be located.
  @return EFI_OUT_OF_RESOURCES  There are not enough resources to find the protocol.

**/
EFI_STATUS
LocateEfiApplicationInFvByName (
  IN  CONST CHAR16*             EfiAppName,
  OUT EFI_DEVICE_PATH           **DevicePath
  );

/**
  Locate an EFI application in a the Firmware Volumes by its GUID

  @param  EfiAppGuid            Guid of the EFI Application into the Firmware Volume
  @param  DevicePath            EFI Device Path of the EFI application

  @return EFI_SUCCESS           The function completed successfully.
  @return EFI_NOT_FOUND         The protocol could not be located.
  @return EFI_OUT_OF_RESOURCES  There are not enough resources to find the protocol.

**/
EFI_STATUS
LocateEfiApplicationInFvByGuid (
  IN  CONST EFI_GUID            *EfiAppGuid,
  OUT EFI_DEVICE_PATH           **DevicePath
  );

#endif
