/** @file
  BDS library definition, include the file and data structure

Copyright (c) 2019, NVIDIA CORPORATION. All rights reserved.
Copyright (c) 2004 - 2018, Intel Corporation. All rights reserved.<BR>
(C) Copyright 2015 Hewlett Packard Enterprise Development LP<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _INTERNAL_BM_H_
#define _INTERNAL_BM_H_

#include <PiDxe.h>

#include <IndustryStandard/Pci.h>
#include <IndustryStandard/PeImage.h>
#include <IndustryStandard/Atapi.h>
#include <IndustryStandard/Scsi.h>
#include <IndustryStandard/Nvme.h>

#include <Protocol/PciRootBridgeIo.h>
#include <Protocol/BlockIo.h>
#include <Protocol/LoadedImage.h>
#include <Protocol/SimpleFileSystem.h>
#include <Protocol/LoadFile.h>
#include <Protocol/DevicePath.h>
#include <Protocol/SimpleTextIn.h>
#include <Protocol/SimpleTextInEx.h>
#include <Protocol/SimpleTextOut.h>
#include <Protocol/SimpleNetwork.h>
#include <Protocol/FirmwareVolume2.h>
#include <Protocol/PciIo.h>
#include <Protocol/GraphicsOutput.h>
#include <Protocol/UsbIo.h>
#include <Protocol/DiskInfo.h>
#include <Protocol/NvmExpressPassthru.h>
#include <Protocol/IdeControllerInit.h>
#include <Protocol/BootLogo.h>
#include <Protocol/DriverHealth.h>
#include <Protocol/FormBrowser2.h>
#include <Protocol/RamDisk.h>
#include <Protocol/DeferredImageLoad.h>
#include <Protocol/PlatformBootManager.h>

#include <Guid/MemoryTypeInformation.h>
#include <Guid/FileInfo.h>
#include <Guid/GlobalVariable.h>
#include <Guid/StatusCodeDataTypeId.h>
#include <Guid/StatusCodeDataTypeVariable.h>

#include <Library/PrintLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/DxeServicesTableLib.h>
#include <Library/HobLib.h>
#include <Library/BaseLib.h>
#include <Library/DevicePathLib.h>
#include <Library/PerformanceLib.h>
#include <Library/PcdLib.h>
#include <Library/PeCoffGetEntryPointLib.h>
#include <Library/UefiBootManagerLib.h>
#include <Library/DxeServicesLib.h>
#include <Library/ReportStatusCodeLib.h>
#include <Library/CapsuleLib.h>
#include <Library/PerformanceLib.h>
#include <Library/HiiLib.h>

#if !defined (EFI_REMOVABLE_MEDIA_FILE_NAME)
  #if defined (MDE_CPU_EBC)
//
// Uefi specification only defines the default boot file name for IA32, X64
// and IPF processor, so need define boot file name for EBC architecture here.
//
#define EFI_REMOVABLE_MEDIA_FILE_NAME  L"\\EFI\\BOOT\\BOOTEBC.EFI"
  #else
    #error "Can not determine the default boot file name for unknown processor type!"
  #endif
#endif

typedef enum {
  BmAcpiFloppyBoot,
  BmHardwareDeviceBoot,
  BmMessageAtapiBoot,
  BmMessageSataBoot,
  BmMessageUsbBoot,
  BmMessageScsiBoot,
  BmMiscBoot
} BM_BOOT_TYPE;

typedef
CHAR16 *
(*BM_GET_BOOT_DESCRIPTION) (
  IN EFI_HANDLE  Handle
  );

//
// PlatformRecovery#### is the load option with the longest name
//
#define BM_OPTION_NAME_LEN  sizeof ("PlatformRecovery####")
extern CHAR16  *mBmLoadOptionName[];

//
// Maximum number of reconnect retry to repair controller; it is to limit the
// number of recursive call of BmRepairAllControllers.
//
#define MAX_RECONNECT_REPAIR  10

/**
  Visitor function to be called by BmForEachVariable for each variable
  in variable storage.

  @param Name    Variable name.
  @param Guid    Variable GUID.
  @param Context The same context passed to BmForEachVariable.
**/
typedef
VOID
(*BM_VARIABLE_VISITOR) (
  CHAR16    *Name,
  EFI_GUID  *Guid,
  VOID      *Context
  );

/**
  Call Visitor function for each variable in variable storage.

  @param Visitor   Visitor function.
  @param Context   The context passed to Visitor function.
**/
VOID
BmForEachVariable (
  BM_VARIABLE_VISITOR  Visitor,
  VOID                 *Context
  );

#define BM_BOOT_DESCRIPTION_ENTRY_SIGNATURE  SIGNATURE_32 ('b', 'm', 'd', 'h')
typedef struct {
  UINT32                                       Signature;
  LIST_ENTRY                                   Link;
  EFI_BOOT_MANAGER_BOOT_DESCRIPTION_HANDLER    Handler;
} BM_BOOT_DESCRIPTION_ENTRY;

/**
  Repair all the controllers according to the Driver Health status queried.

  @param ReconnectRepairCount     To record the number of recursive call of
                                  this function itself.
**/
VOID
BmRepairAllControllers (
  UINTN  ReconnectRepairCount
  );

#define BM_HOTKEY_SIGNATURE  SIGNATURE_32 ('b', 'm', 'h', 'k')
typedef struct {
  UINT32          Signature;
  LIST_ENTRY      Link;

  BOOLEAN         IsContinue;
  UINT16          BootOption;
  UINT8           CodeCount;
  UINT8           WaitingKey;
  EFI_KEY_DATA    KeyData[3];
} BM_HOTKEY;

#define BM_HOTKEY_FROM_LINK(a)  CR (a, BM_HOTKEY, Link, BM_HOTKEY_SIGNATURE)

/**
  Get the Option Number that wasn't used.

  @param  LoadOptionType      Load option type.
  @param  FreeOptionNumber    To receive the minimal free option number.

  @retval EFI_SUCCESS           The option number is found
  @retval EFI_OUT_OF_RESOURCES  There is no free option number that can be used.
  @retval EFI_INVALID_PARAMETER FreeOptionNumber is NULL

**/
EFI_STATUS
BmGetFreeOptionNumber (
  IN  EFI_BOOT_MANAGER_LOAD_OPTION_TYPE  LoadOptionType,
  OUT UINT16                             *FreeOptionNumber
  );

/**
  This routine adjust the memory information for different memory type and
  save them into the variables for next boot. It resets the system when
  memory information is updated and the current boot option belongs to
  boot category instead of application category. It doesn't count the
  reserved memory occupied by RAM Disk.

  @param Boot               TRUE if current boot option belongs to boot
                            category instead of application category.
**/
VOID
BmSetMemoryTypeInformationVariable (
  IN BOOLEAN  Boot
  );

/**
  Check whether there is a instance in BlockIoDevicePath, which contain multi device path
  instances, has the same partition node with HardDriveDevicePath device path

  @param  BlockIoDevicePath      Multi device path instances which need to check
  @param  HardDriveDevicePath    A device path which starts with a hard drive media
                                 device path.

  @retval TRUE                   There is a matched device path instance.
  @retval FALSE                  There is no matched device path instance.

**/
BOOLEAN
BmMatchPartitionDevicePathNode (
  IN  EFI_DEVICE_PATH_PROTOCOL  *BlockIoDevicePath,
  IN  HARDDRIVE_DEVICE_PATH     *HardDriveDevicePath
  );

/**
  Connect the specific Usb device which match the short form device path.

  @param  DevicePath             A short-form device path that starts with the first
                                 element being a USB WWID or a USB Class device
                                 path

  @return EFI_INVALID_PARAMETER  DevicePath is NULL pointer.
                                 DevicePath is not a USB device path.

  @return EFI_SUCCESS            Success to connect USB device
  @return EFI_NOT_FOUND          Fail to find handle for USB controller to connect.

**/
EFI_STATUS
BmConnectUsbShortFormDevicePath (
  IN EFI_DEVICE_PATH_PROTOCOL  *DevicePath
  );

/**
  Stop the hotkey processing.

  @param    Event          Event pointer related to hotkey service.
  @param    Context        Context pass to this function.
**/
VOID
EFIAPI
BmStopHotkeyService (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  );

/**
  Set the variable and report the error through status code upon failure.

  @param  VariableName           A Null-terminated string that is the name of the vendor's variable.
                                 Each VariableName is unique for each VendorGuid. VariableName must
                                 contain 1 or more characters. If VariableName is an empty string,
                                 then EFI_INVALID_PARAMETER is returned.
  @param  VendorGuid             A unique identifier for the vendor.
  @param  Attributes             Attributes bitmask to set for the variable.
  @param  DataSize               The size in bytes of the Data buffer. Unless the EFI_VARIABLE_APPEND_WRITE,
                                 or EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS attribute is set, a size of zero
                                 causes the variable to be deleted. When the EFI_VARIABLE_APPEND_WRITE attribute is
                                 set, then a SetVariable() call with a DataSize of zero will not cause any change to
                                 the variable value (the timestamp associated with the variable may be updated however
                                 even if no new data value is provided,see the description of the
                                 EFI_VARIABLE_AUTHENTICATION_2 descriptor below. In this case the DataSize will not
                                 be zero since the EFI_VARIABLE_AUTHENTICATION_2 descriptor will be populated).
  @param  Data                   The contents for the variable.

  @retval EFI_SUCCESS            The firmware has successfully stored the variable and its data as
                                 defined by the Attributes.
  @retval EFI_INVALID_PARAMETER  An invalid combination of attribute bits, name, and GUID was supplied, or the
                                 DataSize exceeds the maximum allowed.
  @retval EFI_INVALID_PARAMETER  VariableName is an empty string.
  @retval EFI_OUT_OF_RESOURCES   Not enough storage is available to hold the variable and its data.
  @retval EFI_DEVICE_ERROR       The variable could not be retrieved due to a hardware error.
  @retval EFI_WRITE_PROTECTED    The variable in question is read-only.
  @retval EFI_WRITE_PROTECTED    The variable in question cannot be deleted.
  @retval EFI_SECURITY_VIOLATION The variable could not be written due to EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACESS
                                 being set, but the AuthInfo does NOT pass the validation check carried out by the firmware.

  @retval EFI_NOT_FOUND          The variable trying to be updated or deleted was not found.
**/
EFI_STATUS
BmSetVariableAndReportStatusCodeOnError (
  IN CHAR16    *VariableName,
  IN EFI_GUID  *VendorGuid,
  IN UINT32    Attributes,
  IN UINTN     DataSize,
  IN VOID      *Data
  );

/**
  Function compares a device path data structure to that of all the nodes of a
  second device path instance.

  @param  Multi                 A pointer to a multi-instance device path data
                                structure.
  @param  Single                A pointer to a single-instance device path data
                                structure.

  @retval TRUE                  If the Single device path is contained within Multi device path.
  @retval FALSE                 The Single device path is not match within Multi device path.

**/
BOOLEAN
BmMatchDevicePaths (
  IN  EFI_DEVICE_PATH_PROTOCOL  *Multi,
  IN  EFI_DEVICE_PATH_PROTOCOL  *Single
  );

/**
  Delete the instance in Multi which matches partly with Single instance

  @param  Multi                 A pointer to a multi-instance device path data
                                structure.
  @param  Single                A pointer to a single-instance device path data
                                structure.

  @return This function will remove the device path instances in Multi which partly
          match with the Single, and return the result device path. If there is no
          remaining device path as a result, this function will return NULL.

**/
EFI_DEVICE_PATH_PROTOCOL *
BmDelPartMatchInstance (
  IN     EFI_DEVICE_PATH_PROTOCOL  *Multi,
  IN     EFI_DEVICE_PATH_PROTOCOL  *Single
  );

/**
  Print the device path info.

  @param DevicePath           The device path need to print.
**/
VOID
BmPrintDp (
  EFI_DEVICE_PATH_PROTOCOL  *DevicePath
  );

/**
  Convert a single character to number.
  It assumes the input Char is in the scope of L'0' ~ L'9' and L'A' ~ L'F'

  @param    Char   The input char which need to convert to int.

  @return  The converted 8-bit number or (UINTN) -1 if conversion failed.
**/
UINTN
BmCharToUint (
  IN CHAR16  Char
  );

/**
  Return the boot description for the controller.

  @param Handle                Controller handle.

  @return  The description string.
**/
CHAR16 *
BmGetBootDescription (
  IN EFI_HANDLE  Handle
  );

/**
  Enumerate all boot option descriptions and append " 2"/" 3"/... to make
  unique description.

  @param BootOptions            Array of boot options.
  @param BootOptionCount        Count of boot options.
**/
VOID
BmMakeBootOptionDescriptionUnique (
  EFI_BOOT_MANAGER_LOAD_OPTION  *BootOptions,
  UINTN                         BootOptionCount
  );

/**
  Get the file buffer from the specified Load File instance.

  @param LoadFileHandle The specified Load File instance.
  @param FilePath       The file path which will pass to LoadFile().

  @return  The full device path pointing to the load option buffer.
**/
EFI_DEVICE_PATH_PROTOCOL *
BmExpandLoadFile (
  IN  EFI_HANDLE                LoadFileHandle,
  IN  EFI_DEVICE_PATH_PROTOCOL  *FilePath
  );

/**
  Return the RAM Disk device path created by LoadFile.

  @param FilePath  The source file path.

  @return Callee-to-free RAM Disk device path
**/
EFI_DEVICE_PATH_PROTOCOL *
BmGetRamDiskDevicePath (
  IN EFI_DEVICE_PATH_PROTOCOL  *FilePath
  );

/**
  Destroy the RAM Disk.

  The destroy operation includes to call RamDisk.Unregister to
  unregister the RAM DISK from RAM DISK driver, free the memory
  allocated for the RAM Disk.

  @param RamDiskDevicePath    RAM Disk device path.
**/
VOID
BmDestroyRamDisk (
  IN EFI_DEVICE_PATH_PROTOCOL  *RamDiskDevicePath
  );

/**
  Get the next possible full path pointing to the load option.

  @param FilePath  The device path pointing to a load option.
                   It could be a short-form device path.
  @param FullPath  The full path returned by the routine in last call.
                   Set to NULL in first call.

  @return The next possible full path pointing to the load option.
          Caller is responsible to free the memory.
**/
EFI_DEVICE_PATH_PROTOCOL *
BmGetNextLoadOptionDevicePath (
  IN  EFI_DEVICE_PATH_PROTOCOL  *FilePath,
  IN  EFI_DEVICE_PATH_PROTOCOL  *FullPath
  );

/**
  Return the next matched load option buffer.
  The routine keeps calling BmGetNextLoadOptionDevicePath() until a valid
  load option is read.

  @param Type      The load option type.
                   It's used to check whether the load option is valid.
                   When it's LoadOptionTypeMax, the routine only guarantees
                   the load option is a valid PE image but doesn't guarantee
                   the PE's subsystem type is valid.
  @param FilePath  The device path pointing to a load option.
                   It could be a short-form device path.
  @param FullPath  Return the next full device path of the load option after
                   short-form device path expanding.
                   Caller is responsible to free it.
                   NULL to return the first matched full device path.
  @param FileSize  Return the load option size.

  @return The load option buffer. Caller is responsible to free the memory.
**/
VOID *
BmGetNextLoadOptionBuffer (
  IN  EFI_BOOT_MANAGER_LOAD_OPTION_TYPE  Type,
  IN  EFI_DEVICE_PATH_PROTOCOL           *FilePath,
  OUT EFI_DEVICE_PATH_PROTOCOL           **FullPath,
  OUT UINTN                              *FileSize
  );

#endif // _INTERNAL_BM_H_
