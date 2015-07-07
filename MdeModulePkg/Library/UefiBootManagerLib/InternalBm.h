/** @file
  BDS library definition, include the file and data structure

Copyright (c) 2004 - 2015, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _INTERNAL_BM_H_
#define _INTERNAL_BM_H_

#include <PiDxe.h>

#include <IndustryStandard/Pci.h>
#include <IndustryStandard/PeImage.h>
#include <IndustryStandard/Atapi.h>
#include <IndustryStandard/Scsi.h>

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
#include <Protocol/IdeControllerInit.h>
#include <Protocol/BootLogo.h>
#include <Protocol/DriverHealth.h>
#include <Protocol/FormBrowser2.h>

#include <Guid/ZeroGuid.h>
#include <Guid/MemoryTypeInformation.h>
#include <Guid/FileInfo.h>
#include <Guid/GlobalVariable.h>
#include <Guid/Performance.h>
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
#include <Library/TimerLib.h>
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
        #define EFI_REMOVABLE_MEDIA_FILE_NAME L"\\EFI\\BOOT\\BOOTEBC.EFI"
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
  BmMessageNetworkBoot,
  BmMessageHttpBoot,
  BmMiscBoot
} BM_BOOT_TYPE;

typedef
CHAR16 *
(* BM_GET_BOOT_DESCRIPTION) (
  IN EFI_HANDLE          Handle
  );

#define BM_OPTION_NAME_LEN                          sizeof ("SysPrep####")
extern CHAR16  *mBmLoadOptionName[];

typedef
VOID
(*VARIABLE_VISITOR) (
  CHAR16                *Name,
  EFI_GUID              *Guid,
  VOID                  *Context
  );

/**
  Call Visitor function for each variable in variable storage.

  @param Visitor   Visitor function.
  @param Context   The context passed to Visitor function.
**/
VOID
ForEachVariable (
  VARIABLE_VISITOR            Visitor,
  VOID                        *Context
  );

#define BM_BOOT_DESCRIPTION_ENTRY_SIGNATURE SIGNATURE_32 ('b', 'm', 'd', 'h')
typedef struct {
  UINT32                                    Signature;
  LIST_ENTRY                                Link;
  EFI_BOOT_MANAGER_BOOT_DESCRIPTION_HANDLER Handler;
} BM_BOOT_DESCRIPTION_ENTRY;

/**
  Repair all the controllers according to the Driver Health status queried.
**/
VOID
BmRepairAllControllers (
  VOID
  );

#define BM_HOTKEY_SIGNATURE SIGNATURE_32 ('b', 'm', 'h', 'k')
typedef struct {
  UINT32                    Signature;
  LIST_ENTRY                Link;

  BOOLEAN                   IsContinue;
  UINT16                    BootOption;
  UINT8                     CodeCount;
  UINT8                     WaitingKey;
  EFI_KEY_DATA              KeyData[3];
} BM_HOTKEY;

#define BM_HOTKEY_FROM_LINK(a) CR (a, BM_HOTKEY, Link, BM_HOTKEY_SIGNATURE)

/**
  Get the image file buffer data and buffer size by its device path. 

  @param FilePath  On input, a pointer to an allocated buffer containing the device
                   path of the file.
                   On output the pointer could be NULL when the function fails to
                   load the boot option, or could point to an allocated buffer containing
                   the device path of the file.
                   It could be updated by either short-form device path expanding,
                   or default boot file path appending.
                   Caller is responsible to free it when it's non-NULL.
  @param FileSize  A pointer to the size of the file buffer.

  @retval NULL   File is NULL, or FileSize is NULL. Or, the file can't be found.
  @retval other  The file buffer. The caller is responsible to free the memory.
**/
VOID *
BmLoadEfiBootOption (
  IN OUT EFI_DEVICE_PATH_PROTOCOL **FilePath,
  OUT    UINTN                    *FileSize
  );

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
  IN  EFI_BOOT_MANAGER_LOAD_OPTION_TYPE LoadOptionType,
  OUT UINT16                            *FreeOptionNumber
  );

/**

  Writes performance data of booting into the allocated memory.
  OS can process these records.

  @param  Event                 The triggered event.
  @param  Context               Context for this event.

**/
VOID
EFIAPI
BmWriteBootToOsPerformanceData (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  );


/**
  Get the headers (dos, image, optional header) from an image

  @param  Device                SimpleFileSystem device handle
  @param  FileName              File name for the image
  @param  DosHeader             Pointer to dos header
  @param  Hdr                   The buffer in which to return the PE32, PE32+, or TE header.

  @retval EFI_SUCCESS           Successfully get the machine type.
  @retval EFI_NOT_FOUND         The file is not found.
  @retval EFI_LOAD_ERROR        File is not a valid image file.

**/
EFI_STATUS
BmGetImageHeader (
  IN  EFI_HANDLE                  Device,
  IN  CHAR16                      *FileName,
  OUT EFI_IMAGE_DOS_HEADER        *DosHeader,
  OUT EFI_IMAGE_OPTIONAL_HEADER_PTR_UNION   Hdr
  );

/**
  This routine adjust the memory information for different memory type and 
  save them into the variables for next boot.
**/
VOID
BmSetMemoryTypeInformationVariable (
  VOID
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
  IN  EFI_DEVICE_PATH_PROTOCOL   *BlockIoDevicePath,
  IN  HARDDRIVE_DEVICE_PATH      *HardDriveDevicePath
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
  IN EFI_DEVICE_PATH_PROTOCOL   *DevicePath
  );

/**
  Stop the hotkey processing.
  
  @param    Event          Event pointer related to hotkey service. 
  @param    Context        Context pass to this function. 
**/
VOID
EFIAPI
BmStopHotkeyService (
  IN EFI_EVENT    Event,
  IN VOID         *Context
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
                                 EFI_VARIABLE_AUTHENTICATED_WRITE_ACCESS, or 
                                 EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS attribute is set, a size of zero 
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
  @retval EFI_SECURITY_VIOLATION The variable could not be written due to EFI_VARIABLE_AUTHENTICATED_WRITE_ACCESS 
                                 or EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACESS being set, but the AuthInfo 
                                 does NOT pass the validation check carried out by the firmware.

  @retval EFI_NOT_FOUND          The variable trying to be updated or deleted was not found.
**/
EFI_STATUS
BmSetVariableAndReportStatusCodeOnError (
  IN CHAR16     *VariableName,
  IN EFI_GUID   *VendorGuid,
  IN UINT32     Attributes,
  IN UINTN      DataSize,
  IN VOID       *Data
  );

/**
  Get the load option by its device path.

  @param FilePath  The device path pointing to a load option.
                   It could be a short-form device path.
  @param FullPath  Return the full device path of the load option after
                   short-form device path expanding.
                   Caller is responsible to free it.
  @param FileSize  Return the load option size.

  @return The load option buffer. Caller is responsible to free the memory.
**/
VOID *
BmGetLoadOptionBuffer (
  IN  EFI_DEVICE_PATH_PROTOCOL          *FilePath,
  OUT EFI_DEVICE_PATH_PROTOCOL          **FullPath,
  OUT UINTN                             *FileSize
  );

/**
  Return whether the PE header of the load option is valid or not.

  @param[in] Type       The load option type.
  @param[in] FileBuffer The PE file buffer of the load option.
  @param[in] FileSize   The size of the load option file.

  @retval TRUE  The PE header of the load option is valid.
  @retval FALSE The PE header of the load option is not valid.
**/
BOOLEAN
BmIsLoadOptionPeHeaderValid (
  IN EFI_BOOT_MANAGER_LOAD_OPTION_TYPE Type,
  IN VOID                              *FileBuffer,
  IN UINTN                             FileSize
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
  Return the index of the load option in the load option array.

  The function consider two load options are equal when the 
  OptionType, Attributes, Description, FilePath and OptionalData are equal.

  @param Key    Pointer to the load option to be found.
  @param Array  Pointer to the array of load options to be found.
  @param Count  Number of entries in the Array.

  @retval -1          Key wasn't found in the Array.
  @retval 0 ~ Count-1 The index of the Key in the Array.
**/
INTN
BmFindLoadOption (
  IN CONST EFI_BOOT_MANAGER_LOAD_OPTION *Key,
  IN CONST EFI_BOOT_MANAGER_LOAD_OPTION *Array,
  IN UINTN                              Count
  );

/**
  Repair all the controllers according to the Driver Health status queried.
**/
VOID
BmRepairAllControllers (
  VOID
  );

/**
  Print the device path info.

  @param DevicePath           The device path need to print.
**/
VOID
BmPrintDp (
  EFI_DEVICE_PATH_PROTOCOL            *DevicePath
  );

#endif // _INTERNAL_BM_H_
