/** @file
  Generic BDS library defines general interfaces for a BDS driver, including:
    1) BDS boot policy interface.
    2) BDS boot device connect interface.
    3) BDS Misc interfaces for mainting boot variable, ouput string.

Copyright (c) 2004 - 2012, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials are licensed and made available under 
the terms and conditions of the BSD License that accompanies this distribution.  
The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php.                                          
    
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _GENERIC_BDS_LIB_H_
#define _GENERIC_BDS_LIB_H_

#include <Protocol/UserManager.h>

///
/// Constants which are variable names used to access variables.
///
#define VAR_LEGACY_DEV_ORDER L"LegacyDevOrder"

///
/// Data structures and defines.
///
#define FRONT_PAGE_QUESTION_ID  0x0000
#define FRONT_PAGE_DATA_WIDTH   0x01

///
/// ConnectType
///
#define CONSOLE_OUT 0x00000001
#define STD_ERROR   0x00000002
#define CONSOLE_IN  0x00000004
#define CONSOLE_ALL (CONSOLE_OUT | CONSOLE_IN | STD_ERROR)

///
/// Load Option Attributes
///
#define LOAD_OPTION_ACTIVE              0x00000001
#define LOAD_OPTION_FORCE_RECONNECT     0x00000002

#define LOAD_OPTION_HIDDEN              0x00000008
#define LOAD_OPTION_CATEGORY            0x00001F00

#define LOAD_OPTION_CATEGORY_BOOT       0x00000000
#define LOAD_OPTION_CATEGORY_APP        0x00000100

#define EFI_BOOT_OPTION_SUPPORT_KEY     0x00000001
#define EFI_BOOT_OPTION_SUPPORT_APP     0x00000002

#define IS_LOAD_OPTION_TYPE(_c, _Mask)  (BOOLEAN) (((_c) & (_Mask)) != 0)

///
/// Define the maximum characters that will be accepted.
///
#define MAX_CHAR            480
#define MAX_CHAR_SIZE       (MAX_CHAR * 2)

///
/// Define maximum characters for boot option variable "BootXXXX".
///
#define BOOT_OPTION_MAX_CHAR 10

//
// This data structure is the part of BDS_CONNECT_ENTRY
//
#define BDS_LOAD_OPTION_SIGNATURE SIGNATURE_32 ('B', 'd', 'C', 'O')

typedef struct {

  UINTN                     Signature;
  LIST_ENTRY                Link;

  EFI_DEVICE_PATH_PROTOCOL  *DevicePath;

  CHAR16                    *OptionName;
  UINTN                     OptionNumber;
  UINT16                    BootCurrent;
  UINT32                    Attribute;
  CHAR16                    *Description;
  VOID                      *LoadOptions;
  UINT32                    LoadOptionsSize;
  CHAR16                    *StatusString;

} BDS_COMMON_OPTION;

typedef struct {
  EFI_DEVICE_PATH_PROTOCOL  *DevicePath;
  UINTN                     ConnectType;
} BDS_CONSOLE_CONNECT_ENTRY;

//
// Bds boot related lib functions
//
/**
  Boot from the UEFI spec defined "BootNext" variable.

**/
VOID
EFIAPI
BdsLibBootNext (
  VOID
  );

/**
  Process the boot option according to the UEFI specification. The legacy boot option device path includes BBS_DEVICE_PATH.

  @param  Option                 The boot option to be processed.
  @param  DevicePath             The device path describing where to load the
                                 boot image or the legcy BBS device path to boot
                                 the legacy OS.
  @param  ExitDataSize           The size of exit data.
  @param  ExitData               Data returned when Boot image failed.

  @retval EFI_SUCCESS            Boot from the input boot option succeeded.
  @retval EFI_NOT_FOUND          The Device Path is not found in the system.

**/
EFI_STATUS
EFIAPI
BdsLibBootViaBootOption (
  IN  BDS_COMMON_OPTION             * Option,
  IN  EFI_DEVICE_PATH_PROTOCOL      * DevicePath,
  OUT UINTN                         *ExitDataSize,
  OUT CHAR16                        **ExitData OPTIONAL
  );


/**
  This function will enumerate all possible boot devices in the system, and
  automatically create boot options for Network, Shell, Removable BlockIo, 
  and Non-BlockIo Simplefile devices. 
  
  BDS separates EFI boot options into six types:
  1. Network - The boot option points to the SimpleNetworkProtocol device. 
               Bds will try to automatically create this type of boot option during enumeration.
  2. Shell   - The boot option points to internal flash shell. 
               Bds will try to automatically create this type of boot option during enumeration.
  3. Removable BlockIo      - The boot option points to a removable media
                              device, such as a USB flash drive or DVD drive.
                              These devices should contain a *removable* blockIo
                              protocol in their device handle.
                              Bds will try to automatically create this type boot option 
                              when enumerate.
  4. Fixed BlockIo          - The boot option points to a Fixed blockIo device, 
                              such as a hard disk.
                              These devices should contain a *fixed* blockIo
                              protocol in their device handle.
                              BDS will skip fixed blockIo devices, and not
                              automatically create boot option for them. But BDS 
                              will help to delete those fixed blockIo boot options, 
                              whose description rules conflict with other auto-created
                              boot options.
  5. Non-BlockIo Simplefile - The boot option points to a device whose handle 
                              has SimpleFileSystem Protocol, but has no blockio
                              protocol. These devices do not offer blockIo
                              protocol, but BDS still can get the 
                              \EFI\BOOT\boot{machinename}.EFI by SimpleFileSystem
                              Protocol.
  6. File    - The boot option points to a file. These boot options are usually 
               created by the user, either manually or with an OS loader. BDS will not delete or modify
               these boot options.        
    
  This function will enumerate all possible boot devices in the system, and
  automatically create boot options for Network, Shell, Removable BlockIo, 
  and Non-BlockIo Simplefile devices.
  It will excute once every boot.
  
  @param  BdsBootOptionList      The header of the linked list that indexed all
                                 current boot options.

  @retval EFI_SUCCESS            Finished all the boot device enumerations and 
                                 created the boot option based on the boot device.

  @retval EFI_OUT_OF_RESOURCES   Failed to enumerate the boot device and create 
                                 the boot option list.
**/
EFI_STATUS
EFIAPI
BdsLibEnumerateAllBootOption (
  IN OUT LIST_ENTRY          *BdsBootOptionList
  );

/**
  Build the boot option with the handle parsed in.

  @param  Handle                 The handle representing the device path for which 
                                 to create a boot option.
  @param  BdsBootOptionList      The header of the link list that indexed all
                                 current boot options.
  @param  String                 The description of the boot option.

**/
VOID
EFIAPI
BdsLibBuildOptionFromHandle (
  IN  EFI_HANDLE                 Handle,
  IN  LIST_ENTRY                 *BdsBootOptionList,
  IN  CHAR16                     *String
  );


/**
  Build the on flash shell boot option with the handle parsed in.

  @param  Handle                 The handle which present the device path to create
                                 the on flash shell boot option.
  @param  BdsBootOptionList      The header of the link list that indexed all
                                 current boot options.

**/
VOID
EFIAPI
BdsLibBuildOptionFromShell (
  IN EFI_HANDLE                  Handle,
  IN OUT LIST_ENTRY              *BdsBootOptionList
  );

//
// Bds misc lib functions
//
/**
  Get boot mode by looking up the configuration table and parsing the HOB list.

  @param  BootMode              The boot mode from PEI handoff HOB.

  @retval EFI_SUCCESS           Successfully got boot mode.

**/
EFI_STATUS
EFIAPI
BdsLibGetBootMode (
  OUT EFI_BOOT_MODE       *BootMode
  );


/**
  The function will go through the driver option link list, and then load and start
  every driver to which the driver option device path points.

  @param  BdsDriverLists        The header of the current driver option link list.

**/
VOID
EFIAPI
BdsLibLoadDrivers (
  IN LIST_ENTRY                   *BdsDriverLists
  );


/**
  This function processes BootOrder or DriverOrder variables, by calling

  BdsLibVariableToOption () for each UINT16 in the variables.

  @param  BdsCommonOptionList   The header of the option list base on the variable
                                VariableName.
  @param  VariableName          An EFI Variable name indicate the BootOrder or
                                DriverOrder.

  @retval EFI_SUCCESS           Successfully created the boot option or driver option
                                list.
  @retval EFI_OUT_OF_RESOURCES  Failed to get the boot option or the driver option list.
**/
EFI_STATUS
EFIAPI
BdsLibBuildOptionFromVar (
  IN  LIST_ENTRY                      *BdsCommonOptionList,
  IN  CHAR16                          *VariableName
  );

/**
  This function reads the EFI variable (VendorGuid/Name) and returns a dynamically allocated
  buffer and the size of the buffer. If it fails, return NULL.

  @param  Name                  The string part of the  EFI variable name.
  @param  VendorGuid            The GUID part of the EFI variable name.
  @param  VariableSize          Returns the size of the EFI variable that was read.

  @return                       Dynamically allocated memory that contains a copy 
                                of the EFI variable. The caller is responsible for 
                                freeing the buffer.
  @retval NULL                  The variable was not read.

**/
VOID *
EFIAPI
BdsLibGetVariableAndSize (
  IN  CHAR16              *Name,
  IN  EFI_GUID            *VendorGuid,
  OUT UINTN               *VariableSize
  );


/**
  This function prints a series of strings.

  @param  ConOut                A pointer to EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL.
  @param  ...                   A variable argument list containing a series of
                                strings, the last string must be NULL.

  @retval EFI_SUCCESS           Successfully printed out the string using ConOut.
  @retval EFI_STATUS            Return the status of the ConOut->OutputString ().

**/
EFI_STATUS
EFIAPI
BdsLibOutputStrings (
  IN EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL   *ConOut,
  ...
  );

/**
  Build the boot#### or driver#### option from the VariableName. The
  build boot#### or driver#### will also be linked to BdsCommonOptionList.

  @param  BdsCommonOptionList   The header of the boot#### or driver#### option
                                link list.
  @param  VariableName          EFI Variable name, indicates if it is boot#### or
                                driver####.

  @retval BDS_COMMON_OPTION     The option that was created.
  @retval NULL                  Failed to get the new option.

**/
BDS_COMMON_OPTION *
EFIAPI
BdsLibVariableToOption (
  IN OUT LIST_ENTRY                   *BdsCommonOptionList,
  IN  CHAR16                          *VariableName
  );

/**
  This function registers the new boot#### or driver#### option based on
  the VariableName. The new registered boot#### or driver#### will be linked
  to BdsOptionList and also update to the VariableName. After the boot#### or
  driver#### updated, the BootOrder or DriverOrder will also be updated.

  @param  BdsOptionList         The header of the boot#### or driver#### link list.
  @param  DevicePath            The device path that the boot#### or driver####
                                option present.
  @param  String                The description of the boot#### or driver####.
  @param  VariableName          Indicate if the boot#### or driver#### option.

  @retval EFI_SUCCESS           The boot#### or driver#### have been successfully
                                registered.
  @retval EFI_STATUS            Return the status of gRT->SetVariable ().

**/
EFI_STATUS
EFIAPI
BdsLibRegisterNewOption (
  IN  LIST_ENTRY                     *BdsOptionList,
  IN  EFI_DEVICE_PATH_PROTOCOL       *DevicePath,
  IN  CHAR16                         *String,
  IN  CHAR16                         *VariableName
  );

//
// Bds connect and disconnect driver lib funcions
//
/**
  This function connects all system drivers with the corresponding controllers. 

**/
VOID
EFIAPI
BdsLibConnectAllDriversToAllControllers (
  VOID
  );

/**
  This function connects all system drivers to controllers.

**/
VOID
EFIAPI
BdsLibConnectAll (
  VOID
  );

/**
  This function will create all handles associate with every device
  path node. If the handle associate with one device path node can not
  be created successfully, then still give chance to do the dispatch,
  which load the missing drivers if possible.

  @param  DevicePathToConnect   The device path to be connected. Can be
                                a multi-instance device path.

  @retval EFI_SUCCESS           All handles associates with every device path node
                                were created.
  @retval EFI_OUT_OF_RESOURCES  Not enough resources to create new handles.
  @retval EFI_NOT_FOUND         At least one handle could not be created.

**/
EFI_STATUS
EFIAPI
BdsLibConnectDevicePath (
  IN EFI_DEVICE_PATH_PROTOCOL  *DevicePathToConnect
  );

/**
  This function will connect all current system handles recursively.     
  gBS->ConnectController() service is invoked for each handle exist in system handler buffer.  
  If the handle is bus type handler, all childrens also will be connected recursively  by gBS->ConnectController().
  
  @retval EFI_SUCCESS           All handles and child handles have been
                                connected.  
  @retval EFI_STATUS            Return the status of gBS->LocateHandleBuffer().
**/
EFI_STATUS
EFIAPI
BdsLibConnectAllEfi (
  VOID
  );

/**
  This function will disconnect all current system handles.     
  gBS->DisconnectController() is invoked for each handle exists in system handle buffer.  
  If handle is a bus type handle, all childrens also are disconnected recursively by  gBS->DisconnectController().
  
  @retval EFI_SUCCESS           All handles have been disconnected.
  @retval EFI_STATUS            Error status returned by of gBS->LocateHandleBuffer().

**/
EFI_STATUS
EFIAPI
BdsLibDisconnectAllEfi (
  VOID
  );

//
// Bds console related lib functions
//
/**
  This function will search every simpletxt device in the current system,
  and make every simpletxt device a potential console device.

**/
VOID
EFIAPI
BdsLibConnectAllConsoles (
  VOID
  );


/**
  This function will connect console device based on the console
  device variable ConIn, ConOut and ErrOut.

  @retval EFI_SUCCESS              At least one of the ConIn and ConOut devices have
                                   been connected.
  @retval EFI_STATUS               Return the status of BdsLibConnectConsoleVariable ().

**/
EFI_STATUS
EFIAPI
BdsLibConnectAllDefaultConsoles (
  VOID
  );


/**
  This function will connect console device except ConIn base on the console
  device variable ConOut and ErrOut.

  @retval EFI_SUCCESS              At least one of the ConOut device have
                                   been connected success.
  @retval EFI_STATUS               Return the status of BdsLibConnectConsoleVariable ().

**/
EFI_STATUS
EFIAPI
BdsLibConnectAllDefaultConsolesWithOutConIn (
  VOID
  );


/**
  This function updates the console variable based on ConVarName. It can
  add or remove one specific console device path from the variable

  @param  ConVarName               The console-related variable name: ConIn, ConOut,
                                   ErrOut.
  @param  CustomizedConDevicePath  The console device path to be added to
                                   the console variable ConVarName. Cannot be multi-instance.
  @param  ExclusiveDevicePath      The console device path to be removed
                                   from the console variable ConVarName. Cannot be multi-instance.

  @retval EFI_UNSUPPORTED          The added device path is the same as a removed one.
  @retval EFI_SUCCESS              Successfully added or removed the device path from the
                                   console variable.

**/
EFI_STATUS
EFIAPI
BdsLibUpdateConsoleVariable (
  IN  CHAR16                    *ConVarName,
  IN  EFI_DEVICE_PATH_PROTOCOL  *CustomizedConDevicePath,
  IN  EFI_DEVICE_PATH_PROTOCOL  *ExclusiveDevicePath
  );

/**
  Connect the console device base on the variable ConVarName, if
  device path of the ConVarName is multi-instance device path and
  anyone of the instances is connected success, then this function
  will return success.
  If the handle associate with one device path node can not
  be created successfully, then still give chance to do the dispatch,
  which load the missing drivers if possible.

  @param  ConVarName               Console related variable name, ConIn, ConOut,
                                   ErrOut.

  @retval EFI_NOT_FOUND            There is not any console devices connected
                                   success
  @retval EFI_SUCCESS              Success connect any one instance of the console
                                   device path base on the variable ConVarName.

**/
EFI_STATUS
EFIAPI
BdsLibConnectConsoleVariable (
  IN  CHAR16                 *ConVarName
  );

/**
  Connect the console device base on the variable ConVarName, if
  device path of the ConVarName is multi-instance device path and
  anyone of the instances is connected success, then this function
  will return success. 
  Dispatch service is not called when the handle associate with one 
  device path node can not be created successfully. Here no driver 
  dependency is assumed exist, so need not to call this service.

  @param  ConVarName               Console related variable name, ConIn, ConOut,
                                   ErrOut.

  @retval EFI_NOT_FOUND            There is not any console devices connected
                                   success
  @retval EFI_SUCCESS              Success connect any one instance of the console
                                   device path base on the variable ConVarName.

**/
EFI_STATUS
EFIAPI
BdsLibConnectConsoleVariableWithOutDispatch (
  IN  CHAR16                 *ConVarName
  );

//
// Bds device path related lib functions
//
/**
  Delete the instance in Multi that overlaps with Single. 

  @param  Multi                 A pointer to a multi-instance device path data
                                structure.
  @param  Single                A pointer to a single-instance device path data
                                structure.

  @return This function removes the device path instances in Multi that overlap
          Single, and returns the resulting device path. If there is no
          remaining device path as a result, this function will return NULL.

**/
EFI_DEVICE_PATH_PROTOCOL *
EFIAPI
BdsLibDelPartMatchInstance (
  IN     EFI_DEVICE_PATH_PROTOCOL  *Multi,
  IN     EFI_DEVICE_PATH_PROTOCOL  *Single
  );

/**
  This function compares a device path data structure to that of all the nodes of a
  second device path instance.

  @param  Multi                 A pointer to a multi-instance device path data
                                structure.
  @param  Single                A pointer to a single-instance device path data
                                structure.

  @retval TRUE                  If the Single device path is contained within a 
                                Multi device path.
  @retval FALSE                 The Single device path is not contained within a 
                                Multi device path.

**/
BOOLEAN
EFIAPI
BdsLibMatchDevicePaths (
  IN  EFI_DEVICE_PATH_PROTOCOL  *Multi,
  IN  EFI_DEVICE_PATH_PROTOCOL  *Single
  );

/**
  This function converts an input device structure to a Unicode string.

  @param DevPath                  A pointer to the device path structure.

  @return A newly allocated Unicode string that represents the device path.

**/
CHAR16 *
EFIAPI
DevicePathToStr (
  IN EFI_DEVICE_PATH_PROTOCOL     *DevPath
  );

//
// Internal definitions
//
typedef struct {
  CHAR16  *Str;
  UINTN   Len;
  UINTN   Maxlen;
} POOL_PRINT;

typedef
VOID
(*DEV_PATH_FUNCTION) (
  IN OUT POOL_PRINT       *Str,
  IN VOID                 *DevPath
  );

typedef struct {
  UINT8             Type;
  UINT8             SubType;
  DEV_PATH_FUNCTION Function;
} DEVICE_PATH_STRING_TABLE;

typedef struct {
  EFI_DEVICE_PATH_PROTOCOL  Header;
  EFI_GUID                  Guid;
  UINT8                     VendorDefinedData[1];
} VENDOR_DEVICE_PATH_WITH_DATA;

typedef struct {
  EFI_DEVICE_PATH_PROTOCOL  Header;
  UINT16                    NetworkProtocol;
  UINT16                    LoginOption;
  UINT64                    Lun;
  UINT16                    TargetPortalGroupTag;
  CHAR16                    TargetName[1];
} ISCSI_DEVICE_PATH_WITH_NAME;

//
// BBS support macros and functions
//

#if defined(MDE_CPU_IA32) || defined(MDE_CPU_X64)
#define REFRESH_LEGACY_BOOT_OPTIONS \
        BdsDeleteAllInvalidLegacyBootOptions ();\
        BdsAddNonExistingLegacyBootOptions (); \
        BdsUpdateLegacyDevOrder ()
#else
#define REFRESH_LEGACY_BOOT_OPTIONS
#endif

/**
  Delete all the invalid legacy boot options.

  @retval EFI_SUCCESS             All invalid legacy boot options are deleted.
  @retval EFI_OUT_OF_RESOURCES    Failed to allocate necessary memory.
  @retval EFI_NOT_FOUND           Failed to retrieve variable of boot order.

**/
EFI_STATUS
EFIAPI
BdsDeleteAllInvalidLegacyBootOptions (
  VOID
  );

/**
  Add the legacy boot options from BBS table if they do not exist.

  @retval EFI_SUCCESS          The boot options were added successfully, 
                               or they are already in boot options.
  @retval EFI_NOT_FOUND        No legacy boot options is found.
  @retval EFI_OUT_OF_RESOURCE  No enough memory.
  @return Other value          LegacyBoot options are not added.
**/
EFI_STATUS
EFIAPI
BdsAddNonExistingLegacyBootOptions (
  VOID
  );

/**
  Add the legacy boot devices from BBS table into 
  the legacy device boot order.

  @retval EFI_SUCCESS           The boot devices were added successfully.
  @retval EFI_NOT_FOUND         The legacy boot devices are not found.
  @retval EFI_OUT_OF_RESOURCES  Memory or storage is not enough.
  @retval EFI_DEVICE_ERROR      Failed to add the legacy device boot order into EFI variable
                                because of a hardware error.
**/
EFI_STATUS
EFIAPI
BdsUpdateLegacyDevOrder (
  VOID
  );

/**
  Refresh the boot priority for BBS entries based on boot option entry and boot order.

  @param  Entry             The boot option is to be checked for a refreshed BBS table.
  
  @retval EFI_SUCCESS           The boot priority for BBS entries refreshed successfully.
  @retval EFI_NOT_FOUND         BBS entries can't be found.
  @retval EFI_OUT_OF_RESOURCES  Failed to get the legacy device boot order.
**/
EFI_STATUS
EFIAPI
BdsRefreshBbsTableForBoot (
  IN BDS_COMMON_OPTION        *Entry
  );

/**
  Delete the Boot Option from EFI Variable. The Boot Order Arrray
  is also updated.

  @param OptionNumber    The number of Boot options wanting to be deleted.
  @param BootOrder       The Boot Order array.
  @param BootOrderSize   The size of the Boot Order Array.

  @retval  EFI_SUCCESS           The Boot Option Variable was found and removed.
  @retval  EFI_UNSUPPORTED       The Boot Option Variable store was inaccessible.
  @retval  EFI_NOT_FOUND         The Boot Option Variable was not found.
**/
EFI_STATUS
EFIAPI
BdsDeleteBootOption (
  IN UINTN                       OptionNumber,
  IN OUT UINT16                  *BootOrder,
  IN OUT UINTN                   *BootOrderSize
  );

//
//The interface functions related to the Setup Browser Reset Reminder feature
//
/**
  Enable the setup browser reset reminder feature.
  This routine is used in a platform tip. If the platform policy needs the feature, use the routine to enable it.

**/
VOID
EFIAPI
EnableResetReminderFeature (
  VOID
  );

/**
  Disable the setup browser reset reminder feature.
  This routine is used in a platform tip. If the platform policy does not want the feature, use the routine to disable it.

**/
VOID
EFIAPI
DisableResetReminderFeature (
  VOID
  );

/**
  Record the info that a reset is required.
  A module boolean variable is used to record whether a reset is required.

**/
VOID
EFIAPI
EnableResetRequired (
  VOID
  );


/**
  Record the info that no reset is required.
  A module boolean variable is used to record whether a reset is required.

**/
VOID
EFIAPI
DisableResetRequired (
  VOID
  );

/**
  Check whether platform policy enables the reset reminder feature. The default is enabled.

**/
BOOLEAN
EFIAPI
IsResetReminderFeatureEnable (
  VOID
  );

/**
  Check if the user changed any option setting that needs a system reset to be effective.

**/
BOOLEAN
EFIAPI
IsResetRequired (
  VOID
  );

/**
  Check whether a reset is needed, and finish the reset reminder feature.
  If a reset is needed, pop up a menu to notice user, and finish the feature
  according to the user selection.

**/
VOID
EFIAPI
SetupResetReminder (
  VOID
  );


///
/// Define the boot type with which to classify the boot option type.
/// Different boot option types could have different boot behaviors.
/// Use their device path node (Type + SubType) as the type value.
/// The boot type here can be added according to requirements.
///

///
/// ACPI boot type. For ACPI devices, using sub-types to distinguish devices is not allowed, so hardcode their values.
///
#define  BDS_EFI_ACPI_FLOPPY_BOOT         0x0201
///
/// Message boot type
/// If a device path of boot option only points to a message node, the boot option is a message boot type.
///
#define  BDS_EFI_MESSAGE_ATAPI_BOOT       0x0301 // Type 03; Sub-Type 01
#define  BDS_EFI_MESSAGE_SCSI_BOOT        0x0302 // Type 03; Sub-Type 02
#define  BDS_EFI_MESSAGE_USB_DEVICE_BOOT  0x0305 // Type 03; Sub-Type 05
#define  BDS_EFI_MESSAGE_SATA_BOOT        0x0312 // Type 03; Sub-Type 18
#define  BDS_EFI_MESSAGE_MAC_BOOT         0x030b // Type 03; Sub-Type 11
#define  BDS_EFI_MESSAGE_MISC_BOOT        0x03FF

///
/// Media boot type
/// If a device path of boot option contains a media node, the boot option is media boot type.
///
#define  BDS_EFI_MEDIA_HD_BOOT            0x0401 // Type 04; Sub-Type 01
#define  BDS_EFI_MEDIA_CDROM_BOOT         0x0402 // Type 04; Sub-Type 02
///
/// BBS boot type
/// If a device path of boot option contains a BBS node, the boot option is BBS boot type.
///
#define  BDS_LEGACY_BBS_BOOT              0x0501 //  Type 05; Sub-Type 01

#define  BDS_EFI_UNSUPPORT                0xFFFF

/**
  Check whether an instance in BlockIoDevicePath has the same partition node as the HardDriveDevicePath device path.

  @param  BlockIoDevicePath      Multi device path instances to check.
  @param  HardDriveDevicePath    A device path starting with a hard drive media
                                 device path.

  @retval TRUE                   There is a matched device path instance.
  @retval FALSE                  There is no matched device path instance.

**/
BOOLEAN
EFIAPI
MatchPartitionDevicePathNode (
  IN  EFI_DEVICE_PATH_PROTOCOL   *BlockIoDevicePath,
  IN  HARDDRIVE_DEVICE_PATH      *HardDriveDevicePath
  );


/**
  Expand a device path that starts with a hard drive media device path node to be a
  full device path that includes the full hardware path to the device. This function enables the device to boot. 
  To avoid requiring a connect on every boot, the front match is saved in a variable (the part point
  to the partition node. E.g. ACPI() /PCI()/ATA()/Partition() ).
  All successful history device paths
  that point to the front part of the partition node will be saved.

  @param  HardDriveDevicePath    EFI Device Path to boot, if it starts with a hard
                                 drive media device path.
  @return A Pointer to the full device path, or NULL if a valid Hard Drive devic path
          cannot be found.

**/
EFI_DEVICE_PATH_PROTOCOL *
EFIAPI
BdsExpandPartitionPartialDevicePathToFull (
  IN  HARDDRIVE_DEVICE_PATH      *HardDriveDevicePath
  );
  
/**
  Return the bootable media handle.
  First, check whether the device is connected.
  Second, check whether the device path points to a device that supports SimpleFileSystemProtocol.
  Third, detect the the default boot file in the Media, and return the removable Media handle.

  @param  DevicePath             The Device Path to a  bootable device.

  @return  The bootable media handle. If the media on the DevicePath is not bootable, NULL will return.

**/
EFI_HANDLE
EFIAPI
BdsLibGetBootableHandle (
  IN  EFI_DEVICE_PATH_PROTOCOL      *DevicePath
  );
  

/**
  Checks whether the Device path in a boot option points to a valid bootable device, and if the device
  is ready to boot now.

  @param  DevPath     The Device path in a boot option.
  @param  CheckMedia  If true, check whether the device is ready to boot now.

  @retval TRUE        The Device path is valid.
  @retval FALSE       The Device path is invalid.

**/
BOOLEAN
EFIAPI
BdsLibIsValidEFIBootOptDevicePath (
  IN EFI_DEVICE_PATH_PROTOCOL     *DevPath,
  IN BOOLEAN                      CheckMedia
  );

/**
  Checks whether the Device path in a boot option points to a valid bootable device, and if the device
  is ready to boot now.
  If Description is not NULL and the device path points to a fixed BlockIo
  device, this function checks whether the description conflicts with other auto-created
  boot options.

  @param  DevPath     The Device path in a boot option.
  @param  CheckMedia  If true, checks if the device is ready to boot now.
  @param  Description The description of a boot option.

  @retval TRUE        The Device path is valid.
  @retval FALSE       The Device path is invalid.

**/
BOOLEAN
EFIAPI
BdsLibIsValidEFIBootOptDevicePathExt (
  IN EFI_DEVICE_PATH_PROTOCOL     *DevPath,
  IN BOOLEAN                      CheckMedia,
  IN CHAR16                       *Description
  );

/**
  For a bootable Device path, return its boot type.

  @param  DevicePath                      The bootable device Path to check.

  @retval BDS_EFI_MEDIA_HD_BOOT           The given device path contains MEDIA_DEVICE_PATH type device path node,
                                          whose subtype is MEDIA_HARDDRIVE_DP.  
  @retval BDS_EFI_MEDIA_CDROM_BOOT        If given device path contains MEDIA_DEVICE_PATH type device path node, 
                                          whose subtype is MEDIA_CDROM_DP.  
  @retval BDS_EFI_ACPI_FLOPPY_BOOT        A given device path contains ACPI_DEVICE_PATH type device path node,                                          
                                          whose HID is floppy device.  
  @retval BDS_EFI_MESSAGE_ATAPI_BOOT      A given device path contains MESSAGING_DEVICE_PATH type device path node, 
                                          and its last device path node's subtype is MSG_ATAPI_DP.  
  @retval BDS_EFI_MESSAGE_SCSI_BOOT       A given device path contains MESSAGING_DEVICE_PATH type device path node,
                                          and its last device path node's subtype is MSG_SCSI_DP. 
  @retval BDS_EFI_MESSAGE_USB_DEVICE_BOOT A given device path contains MESSAGING_DEVICE_PATH type device path node, 
                                          and its last device path node's subtype is MSG_USB_DP.
  @retval BDS_EFI_MESSAGE_MISC_BOOT       The device path does not contain any media device path node, and  
                                          its last device path node points to a message device path node.  
  @retval BDS_LEGACY_BBS_BOOT             A given device path contains BBS_DEVICE_PATH type device path node. 
  @retval BDS_EFI_UNSUPPORT               An EFI Removable BlockIO device path does not point to a media and message device.   

  **/
UINT32
EFIAPI
BdsGetBootTypeFromDevicePath (
  IN  EFI_DEVICE_PATH_PROTOCOL     *DevicePath
  );


/**
  This routine registers a function to adjust the different types of memory page numbers
  just before booting, and saves the updated info into the variable for the next boot to use.

**/
VOID
EFIAPI
BdsLibSaveMemoryTypeInformation (
  VOID
  );
  
/**
  Identify a user and, if authenticated, returns the current user profile handle.

  @param[out]  User           Points to the user profile handle.
  
  @retval EFI_SUCCESS         The user is successfully identified, or user identification
                              is not supported.
  @retval EFI_ACCESS_DENIED   The user was not successfully identified.

**/
EFI_STATUS
EFIAPI
BdsLibUserIdentify (
  OUT EFI_USER_PROFILE_HANDLE         *User
  );  

/**
  This function checks if a Fv file device path is valid, according to a file GUID. If it is invalid,
  it tries to return the valid device path.
  FV address maybe changes for memory layout adjust from time to time, use this funciton
  could promise the Fv file device path is right.

  @param  DevicePath             On input, the Fv file device path to check. On
                                 output, the updated valid Fv file device path
  @param  FileGuid               the Fv file GUID.

  @retval EFI_INVALID_PARAMETER  The input DevicePath or FileGuid is invalid.
  @retval EFI_UNSUPPORTED        The input DevicePath does not contain an Fv file
                                 GUID at all.
  @retval EFI_ALREADY_STARTED    The input DevicePath has pointed to the Fv file and is
                                 valid.
  @retval EFI_SUCCESS            Successfully updated the invalid DevicePath
                                 and returned the updated device path in DevicePath.

**/
EFI_STATUS
EFIAPI
BdsLibUpdateFvFileDevicePath (
  IN  OUT EFI_DEVICE_PATH_PROTOCOL      ** DevicePath,
  IN  EFI_GUID                          *FileGuid
  );


/**
  Connect the specific USB device that matches the RemainingDevicePath,
  and whose bus is determined by Host Controller (Uhci or Ehci).

  @param  HostControllerPI      Uhci (0x00) or Ehci (0x20) or Both uhci and ehci
                                (0xFF).
  @param  RemainingDevicePath   A short-form device path that starts with the first
                                element being a USB WWID or a USB Class device
                                path.

  @retval EFI_SUCCESS           The specific Usb device is connected successfully.
  @retval EFI_INVALID_PARAMETER Invalid HostControllerPi (not 0x00, 0x20 or 0xFF) 
                                or RemainingDevicePath is not the USB class device path.
  @retval EFI_NOT_FOUND         The device specified by device path is not found.

**/
EFI_STATUS
EFIAPI
BdsLibConnectUsbDevByShortFormDP(
  IN UINT8                      HostControllerPI,
  IN EFI_DEVICE_PATH_PROTOCOL   *RemainingDevicePath
  );
  

//
// The implementation of this function is provided by Platform code.
//
/**
  Convert Vendor device path to a device name.

  @param  Str      The buffer storing device name.
  @param  DevPath  The pointer to vendor device path.

**/
VOID
DevPathVendor (
  IN OUT POOL_PRINT       *Str,
  IN VOID                 *DevPath
  );

/**
  Concatenates a formatted unicode string to an allocated pool.
  The caller must free the resulting buffer.

  @param  Str      Tracks the allocated pool, size in use, and amount of pool allocated.
  @param  Fmt      The format string.
  @param  ...      The data will be printed.

  @return Allocated buffer with the formatted string printed in it.
          The caller must free the allocated buffer.
          The buffer allocation is not packed.

**/
CHAR16 *
EFIAPI
CatPrint (
  IN OUT POOL_PRINT   *Str,
  IN CHAR16           *Fmt,
  ...
  );

/**
  Use SystemTable ConOut to stop video based Simple Text Out consoles from going
  to the video device. Put up LogoFile on every video device that is a console.

  @param[in]  LogoFile   The file name of logo to display on the center of the screen.

  @retval EFI_SUCCESS     ConsoleControl has been flipped to graphics and logo displayed.
  @retval EFI_UNSUPPORTED Logo not found.

**/
EFI_STATUS
EFIAPI
EnableQuietBoot (
  IN  EFI_GUID  *LogoFile
  );


/**
  Use SystemTable ConOut to turn on video based Simple Text Out consoles. The 
  Simple Text Out screens will now be synced up with all non-video output devices.

  @retval EFI_SUCCESS     UGA devices are back in text mode and synced up.

**/
EFI_STATUS
EFIAPI
DisableQuietBoot (
  VOID
  );

#endif

