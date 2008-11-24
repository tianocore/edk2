/** @file
  Generic BDS library definition, include the data structure and function.

Copyright (c) 2004 - 2008, Intel Corporation. <BR>
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _GENERIC_BDS_LIB_H_
#define _GENERIC_BDS_LIB_H_

#include <Protocol/HiiDatabase.h>
#include <IndustryStandard/PeImage.h>


extern EFI_HANDLE mBdsImageHandle;

//
// Constants which are variable names used to access variables
//
#define VAR_LEGACY_DEV_ORDER L"LegacyDevOrder"

//
// Data structures and defines
//
#define FRONT_PAGE_QUESTION_ID  0x0000
#define FRONT_PAGE_DATA_WIDTH   0x01

//
// ConnectType
//
#define CONSOLE_OUT 0x00000001
#define STD_ERROR   0x00000002
#define CONSOLE_IN  0x00000004
#define CONSOLE_ALL (CONSOLE_OUT | CONSOLE_IN | STD_ERROR)

//
// Load Option Attributes defined in EFI Specification
//
#define LOAD_OPTION_ACTIVE              0x00000001
#define LOAD_OPTION_FORCE_RECONNECT     0x00000002

#define LOAD_OPTION_HIDDEN              0x00000008
#define LOAD_OPTION_CATEGORY            0x00001F00

#define LOAD_OPTION_CATEGORY_BOOT       0x00000000
#define LOAD_OPTION_CATEGORY_APP        0x00000100

#define EFI_BOOT_OPTION_SUPPORT_KEY     0x00000001
#define EFI_BOOT_OPTION_SUPPORT_APP     0x00000002

#define IS_LOAD_OPTION_TYPE(_c, _Mask)  (BOOLEAN) (((_c) & (_Mask)) != 0)

//
// Define Maxmim characters that will be accepted
//
#define MAX_CHAR            480
#define MAX_CHAR_SIZE       (MAX_CHAR * 2)

#define MIN_ALIGNMENT_SIZE  4
#define ALIGN_SIZE(a)       ((a % MIN_ALIGNMENT_SIZE) ? MIN_ALIGNMENT_SIZE - (a % MIN_ALIGNMENT_SIZE) : 0)

//
// Define maximum characters for boot option variable "BootXXXX"
//
#define BOOT_OPTION_MAX_CHAR 10

//
// This data structure is the part of BDS_CONNECT_ENTRY that we can hard code.
//
#define BDS_LOAD_OPTION_SIGNATURE EFI_SIGNATURE_32 ('B', 'd', 'C', 'O')

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
// Lib Functions
//

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
  Process the boot option follow the UEFI specification and
  special treat the legacy boot option with BBS_DEVICE_PATH.

  @param  Option                 The boot option need to be processed
  @param  DevicePath             The device path which describe where to load the
                                 boot image or the legcy BBS device path to boot
                                 the legacy OS
  @param  ExitDataSize           The size of exit data.
  @param  ExitData               Data returned when Boot image failed.

  @retval EFI_SUCCESS            Boot from the input boot option successfully.
  @retval EFI_NOT_FOUND          If the Device Path is not found in the system

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
  This function will enumerate all possible boot device in the system,
  it will only excute once of every boot.

  @param  BdsBootOptionList      The header of the link list which indexed all
                                 current boot options

  @retval EFI_SUCCESS            Finished all the boot device enumerate and create
                                 the boot option base on that boot device

**/
EFI_STATUS
EFIAPI
BdsLibEnumerateAllBootOption (
  IN OUT LIST_ENTRY          *BdsBootOptionList
  );

/**
  Build the boot option with the handle parsed in

  @param  Handle                 The handle which present the device path to create
                                 boot option
  @param  BdsBootOptionList      The header of the link list which indexed all
                                 current boot options
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
                                 on flash shell boot option
  @param  BdsBootOptionList      The header of the link list which indexed all
                                 current boot options

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
  Return the default value for system Timeout variable.

  @return Timeout value.

**/
UINT16
EFIAPI
BdsLibGetTimeout (
  VOID
  );

/**
  Get boot mode by looking up configuration table and parsing HOB list

  @param  BootMode              Boot mode from PEI handoff HOB.

  @retval EFI_SUCCESS           Successfully get boot mode

**/
EFI_STATUS
EFIAPI
BdsLibGetBootMode (
  OUT EFI_BOOT_MODE       *BootMode
  );


/**
  The function will go through the driver optoin link list, load and start
  every driver the driver optoin device path point to.

  @param  BdsDriverLists        The header of the current driver option link list

**/
VOID
EFIAPI
BdsLibLoadDrivers (
  IN LIST_ENTRY                   *BdsDriverLists
  );


/**
  Process BootOrder, or DriverOrder variables, by calling
  BdsLibVariableToOption () for each UINT16 in the variables.

  @param  BdsCommonOptionList   The header of the option list base on variable
                                VariableName
  @param  VariableName          EFI Variable name indicate the BootOrder or
                                DriverOrder

  @retval EFI_SUCCESS           Success create the boot option or driver option
                                list
  @retval EFI_OUT_OF_RESOURCES  Failed to get the boot option or driver option list

**/
EFI_STATUS
EFIAPI
BdsLibBuildOptionFromVar (
  IN  LIST_ENTRY                      *BdsCommonOptionList,
  IN  CHAR16                          *VariableName
  );

/**
  Read the EFI variable (VendorGuid/Name) and return a dynamically allocated
  buffer, and the size of the buffer. If failure return NULL.

  @param  Name                  String part of EFI variable name
  @param  VendorGuid            GUID part of EFI variable name
  @param  VariableSize          Returns the size of the EFI variable that was read

  @return                       Dynamically allocated memory that contains a copy of the EFI variable
                                Caller is responsible freeing the buffer.
  @retval NULL                  Variable was not read

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

  @param  ConOut                Pointer to EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL
  @param  ...                   A variable argument list containing series of
                                strings, the last string must be NULL.

  @retval EFI_SUCCESS           Success print out the string using ConOut.
  @retval EFI_STATUS            Return the status of the ConOut->OutputString ().

**/
EFI_STATUS
EFIAPI
BdsLibOutputStrings (
  IN EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL   *ConOut,
  ...
  );

/**
  Build the boot#### or driver#### option from the VariableName, the
  build boot#### or driver#### will also be linked to BdsCommonOptionList.

  @param  BdsCommonOptionList   The header of the boot#### or driver#### option
                                link list
  @param  VariableName          EFI Variable name indicate if it is boot#### or
                                driver####

  @retval BDS_COMMON_OPTION     Get the option just been created
  @retval NULL                  Failed to get the new option

**/
BDS_COMMON_OPTION *
EFIAPI
BdsLibVariableToOption (
  IN OUT LIST_ENTRY                   *BdsCommonOptionList,
  IN  CHAR16                          *VariableName
  );

/**
  This function will register the new boot#### or driver#### option base on
  the VariableName. The new registered boot#### or driver#### will be linked
  to BdsOptionList and also update to the VariableName. After the boot#### or
  driver#### updated, the BootOrder or DriverOrder will also be updated.

  @param  BdsOptionList         The header of the boot#### or driver#### link list
  @param  DevicePath            The device path which the boot#### or driver####
                                option present
  @param  String                The description of the boot#### or driver####
  @param  VariableName          Indicate if the boot#### or driver#### option

  @retval EFI_SUCCESS           The boot#### or driver#### have been success
                                registered
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
// Bds connect or disconnect driver lib funcion
//
/**
  Connects all drivers to all controllers.
  This function make sure all the current system driver will manage
  the correspoinding controllers if have. And at the same time, make
  sure all the system controllers have driver to manage it if have.

**/
VOID
EFIAPI
BdsLibConnectAllDriversToAllControllers (
  VOID
  );

/**
  This function will connect all the system driver to controller
  first, and then special connect the default console, this make
  sure all the system controller avialbe and the platform default
  console connected.

**/
VOID
EFIAPI
BdsLibConnectAll (
  VOID
  );

/**
  This function will create all handles associate with every device
  path node. If the handle associate with one device path node can not
  be created success, then still give one chance to do the dispatch,
  which load the missing drivers if possible.

  @param  DevicePathToConnect   The device path which will be connected, it can be
                                a multi-instance device path

  @retval EFI_SUCCESS           All handles associate with every device path  node
                                have been created
  @retval EFI_OUT_OF_RESOURCES  There is no resource to create new handles
  @retval EFI_NOT_FOUND         Create the handle associate with one device  path
                                node failed

**/
EFI_STATUS
EFIAPI
BdsLibConnectDevicePath (
  IN EFI_DEVICE_PATH_PROTOCOL  *DevicePathToConnect
  );

/**
  This function will connect all current system handles recursively. The
  connection will finish until every handle's child handle created if it have.

  @retval EFI_SUCCESS           All handles and it's child handle have been
                                connected
  @retval EFI_STATUS            Return the status of gBS->LocateHandleBuffer().

**/
EFI_STATUS
EFIAPI
BdsLibConnectAllEfi (
  VOID
  );


/**
  This function will disconnect all current system handles. The disconnection
  will finish until every handle have been disconnected.

  @retval EFI_SUCCESS           All handles have been disconnected
  @retval EFI_STATUS            Return the status of gBS->LocateHandleBuffer().

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
  This function will search every simpletxt devive in current system,
  and make every simpletxt device as pertantial console device.

**/
VOID
EFIAPI
BdsLibConnectAllConsoles (
  VOID
  );


/**
  This function will connect console device base on the console
  device variable ConIn, ConOut and ErrOut.

  @retval EFI_SUCCESS              At least one of the ConIn and ConOut device have
                                   been connected success.
  @retval EFI_STATUS               Return the status of BdsLibConnectConsoleVariable ().

**/
EFI_STATUS
EFIAPI
BdsLibConnectAllDefaultConsoles (
  VOID
  );

/**
  This function update console variable based on ConVarName, it can
  add or remove one specific console device path from the variable

  @param  ConVarName               Console related variable name, ConIn, ConOut,
                                   ErrOut.
  @param  CustomizedConDevicePath  The console device path which will be added to
                                   the console variable ConVarName, this parameter
                                   can not be multi-instance.
  @param  ExclusiveDevicePath      The console device path which will be removed
                                   from the console variable ConVarName, this
                                   parameter can not be multi-instance.

  @retval EFI_UNSUPPORTED          The added device path is same to the removed one.
  @retval EFI_SUCCESS              Success add or remove the device path from  the
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
  device path of the ConVarName is multi-instance device path, if
  anyone of the instances is connected success, then this function
  will return success.

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

//
// Bds device path related lib functions
//
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
EFIAPI
BdsLibDelPartMatchInstance (
  IN     EFI_DEVICE_PATH_PROTOCOL  *Multi,
  IN     EFI_DEVICE_PATH_PROTOCOL  *Single
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
EFIAPI
BdsLibMatchDevicePaths (
  IN  EFI_DEVICE_PATH_PROTOCOL  *Multi,
  IN  EFI_DEVICE_PATH_PROTOCOL  *Single
  );

/**
  This function converts an input device structure to a Unicode string.

  @param DevPath                  A pointer to the device path structure.

  @return A new allocated Unicode string that represents the device path.

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
  CHAR16  *str;
  UINTN   len;
  UINTN   maxlen;
} POOL_PRINT;

typedef struct {
  UINT8 Type;
  UINT8 SubType;
  VOID (*Function) (POOL_PRINT *, VOID *);
} DEVICE_PATH_STRING_TABLE;

extern EFI_GUID mEfiDevicePathMessagingUartFlowControlGuid;

typedef struct {
  EFI_DEVICE_PATH_PROTOCOL  Header;
  EFI_GUID                  Guid;
  UINT8                     VendorDefinedData[1];
} VENDOR_DEVICE_PATH_WITH_DATA;


extern EFI_GUID mEfiDevicePathMessagingSASGuid;

typedef struct {
  EFI_DEVICE_PATH_PROTOCOL  Header;
  UINT16                    NetworkProtocol;
  UINT16                    LoginOption;
  UINT64                    Lun;
  UINT16                    TargetPortalGroupTag;
  CHAR16                    iSCSITargetName[1];
} ISCSI_DEVICE_PATH_WITH_NAME;


//
// Notes: EFI 64 shadow all option rom
//
#if defined (MDE_CPU_IPF)
#define EFI64_SHADOW_ALL_LEGACY_ROM() ShadowAllOptionRom ();
#else
#define EFI64_SHADOW_ALL_LEGACY_ROM()
#endif

/**
  Shadow all Legacy OptionRom. 

**/
VOID
EFIAPI
ShadowAllOptionRom (
  VOID
  );

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

  @retval EFI_SUCCESS             All invalide legacy boot options are deleted.
  @retval EFI_OUT_OF_RESOURCES    Fail to allocate necessary memory.
  @retval EFI_NOT_FOUND           Fail to retrive variable of boot order.

**/
EFI_STATUS
EFIAPI
BdsDeleteAllInvalidLegacyBootOptions (
  VOID
  );

/**

  Add the legacy boot options from BBS table if they do not exist.

  @retval EFI_SUCCESS       The boot options are added successfully 
                            or they are already in boot options.

**/
EFI_STATUS
EFIAPI
BdsAddNonExistingLegacyBootOptions (
  VOID
  );

/**

  Add the legacy boot devices from BBS table into 
  the legacy device boot order.

  @retval EFI_SUCCESS       The boot devices are added successfully.

**/
EFI_STATUS
EFIAPI
BdsUpdateLegacyDevOrder (
  VOID
  );

/**

  Set the boot priority for BBS entries based on boot option entry and boot order.

  @param  Entry             The boot option is to be checked for refresh BBS table.
  
  @retval EFI_SUCCESS       The boot priority for BBS entries is refreshed successfully.

**/
EFI_STATUS
EFIAPI
BdsRefreshBbsTableForBoot (
  IN BDS_COMMON_OPTION        *Entry
  );

/**

  Delete boot option specified by OptionNumber and adjust the boot order.

  @param  OptionNumber      The boot option to be deleted.
  @param  BootOrder         Boot order list to be adjusted by remove this boot option.
  @param  BootOrderSize     The size of Boot order list will be modified.
  
  @retval EFI_SUCCESS       The boot option is deleted successfully.

**/
EFI_STATUS
EFIAPI
BdsDeleteBootOption (
  IN UINTN                       OptionNumber,
  IN OUT UINT16                  *BootOrder,
  IN OUT UINTN                   *BootOrderSize
  );

//
//The interface functions relate with Setup Browser Reset Reminder feature
//
/**
  Enable the setup browser reset reminder feature.
  This routine is used in platform tip. If the platform policy need the feature, use the routine to enable it.

**/
VOID
EFIAPI
EnableResetReminderFeature (
  VOID
  );

/**
  Disable the setup browser reset reminder feature.
  This routine is used in platform tip. If the platform policy do not want the feature, use the routine to disable it.

**/
VOID
EFIAPI
DisableResetReminderFeature (
  VOID
  );

/**
  Record the info that  a reset is required.
  A  module boolean variable is used to record whether a reset is required.

**/
VOID
EFIAPI
EnableResetRequired (
  VOID
  );


/**
  Record the info that  no reset is required.
  A  module boolean variable is used to record whether a reset is required.

**/
VOID
EFIAPI
DisableResetRequired (
  VOID
  );

/**
  Check whether platform policy enable the reset reminder feature. The default is enabled.

**/
BOOLEAN
EFIAPI
IsResetReminderFeatureEnable (
  VOID
  );

/**
  Check if  user changed any option setting which needs a system reset to be effective.

**/
BOOLEAN
EFIAPI
IsResetRequired (
  VOID
  );

/**
  Check whether a reset is needed, and finish the reset reminder feature.
  If a reset is needed, Popup a menu to notice user, and finish the feature
  according to the user selection.

**/
VOID
EFIAPI
SetupResetReminder (
  VOID
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
EFIAPI
BdsLibGetImageHeader (
  IN  EFI_HANDLE                  Device,
  IN  CHAR16                      *FileName,
  OUT EFI_IMAGE_DOS_HEADER        *DosHeader,
  OUT EFI_IMAGE_OPTIONAL_HEADER_PTR_UNION   Hdr
  );

//
// Define the boot type which to classify the boot option type
// Different boot option type could have different boot behavior
// Use their device path node (Type + SubType) as type value
// The boot type here can be added according to requirement
//
//
// ACPI boot type. For ACPI device, cannot use sub-type to distinguish device, so hardcode their value
//
#define  BDS_EFI_ACPI_FLOPPY_BOOT         0x0201
//
// Message boot type
// If a device path of boot option only point to a message node, the boot option is message boot type
//
#define  BDS_EFI_MESSAGE_ATAPI_BOOT       0x0301 // Type 03; Sub-Type 01
#define  BDS_EFI_MESSAGE_SCSI_BOOT        0x0302 // Type 03; Sub-Type 02
#define  BDS_EFI_MESSAGE_USB_DEVICE_BOOT  0x0305 // Type 03; Sub-Type 05
#define  BDS_EFI_MESSAGE_MISC_BOOT        0x03FF
//
// Media boot type
// If a device path of boot option contain a media node, the boot option is media boot type
//
#define  BDS_EFI_MEDIA_HD_BOOT            0x0401 // Type 04; Sub-Type 01
#define  BDS_EFI_MEDIA_CDROM_BOOT         0x0402 // Type 04; Sub-Type 02
//
// BBS boot type
// If a device path of boot option contain a BBS node, the boot option is BBS boot type
//
#define  BDS_LEGACY_BBS_BOOT              0x0501 //  Type 05; Sub-Type 01

#define  BDS_EFI_UNSUPPORT                0xFFFF

//
// USB host controller Programming Interface.
//
#define  PCI_CLASSC_PI_UHCI               0x00
#define  PCI_CLASSC_PI_EHCI               0x20

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
EFIAPI
MatchPartitionDevicePathNode (
  IN  EFI_DEVICE_PATH_PROTOCOL   *BlockIoDevicePath,
  IN  HARDDRIVE_DEVICE_PATH      *HardDriveDevicePath
  );


/**
  Expand a device path that starts with a hard drive media device path node to be a
  full device path that includes the full hardware path to the device. We need
  to do this so it can be booted. As an optimizaiton the front match (the part point
  to the partition node. E.g. ACPI() /PCI()/ATA()/Partition() ) is saved in a variable
  so a connect all is not required on every boot. All successful history device path
  which point to partition node (the front part) will be saved.

  @param  HardDriveDevicePath    EFI Device Path to boot, if it starts with a hard
                                 drive media device path.
  @return A Pointer to the full device path or NULL if a valid Hard Drive devic path
          cannot be found.

**/
EFI_DEVICE_PATH_PROTOCOL *
EFIAPI
BdsExpandPartitionPartialDevicePathToFull (
  IN  HARDDRIVE_DEVICE_PATH      *HardDriveDevicePath
  );
  
/**
  Return the bootable media handle.
  First, check the device is connected
  Second, check whether the device path point to a device which support SimpleFileSystemProtocol,
  Third, detect the the default boot file in the Media, and return the removable Media handle.

  @param  DevicePath             Device Path to a  bootable device

  @retval NULL                   The media on the DevicePath is not bootable

**/
EFI_HANDLE
EFIAPI
BdsLibGetBootableHandle (
  IN  EFI_DEVICE_PATH_PROTOCOL      *DevicePath
  );
  

/**
  Check whether the Device path in a boot option point to a valide bootable device,
  And if CheckMedia is true, check the device is ready to boot now.

  @param  DevPath     the Device path in a boot option
  @param  CheckMedia  if true, check the device is ready to boot now.

  @retval TRUE        the Device path  is valide
  @retval FALSE       the Device path  is invalide .

**/
BOOLEAN
EFIAPI
BdsLibIsValidEFIBootOptDevicePath (
  IN EFI_DEVICE_PATH_PROTOCOL     *DevPath,
  IN BOOLEAN                      CheckMedia
  );
  
/**
  For a bootable Device path, return its boot type.

  @param  DevicePath                      The bootable device Path to check

  @retval BDS_EFI_MEDIA_HD_BOOT           If the device path contains any media deviec path node, it is media boot type
                                          For the floppy node, handle it as media node
  @retval BDS_EFI_MEDIA_CDROM_BOOT        If the device path contains any media deviec path node, it is media boot type
                                          For the floppy node, handle it as media node
  @retval BDS_EFI_ACPI_FLOPPY_BOOT        If the device path contains any media deviec path node, it is media boot type
                                          For the floppy node, handle it as media node
  @retval BDS_EFI_MESSAGE_ATAPI_BOOT      If the device path not contains any media deviec path node,  and
                                          its last device path node point to a message device path node, it is
  
  @retval BDS_EFI_MESSAGE_SCSI_BOOT       If the device path not contains any media deviec path node,  and
                                          its last device path node point to a message device path node, it is
  @retval BDS_EFI_MESSAGE_USB_DEVICE_BOOT If the device path not contains any media deviec path node,  and
                                          its last device path node point to a message device path node, it is
  @retval BDS_EFI_MESSAGE_MISC_BOOT       If the device path not contains any media deviec path node,  and
                                          its last device path node point to a message device path node, it is
  @retval BDS_LEGACY_BBS_BOOT             Legacy boot type
  @retval BDS_EFI_UNSUPPORT               An EFI Removable BlockIO device path not point to a media and message devie,   

**/
UINT32
EFIAPI
BdsGetBootTypeFromDevicePath (
  IN  EFI_DEVICE_PATH_PROTOCOL     *DevicePath
  );


/**
  This routine register a function to adjust the different type memory page number
  just before booting and save the updated info into the variable for next boot to use.

**/
VOID
EFIAPI
BdsLibSaveMemoryTypeInformation (
  VOID
  );
  

/**
  According to a file guild, check a Fv file device path is valid. If it is invalid,
  try to return the valid device path.
  FV address maybe changes for memory layout adjust from time to time, use this funciton
  could promise the Fv file device path is right.

  @param  DevicePath             on input, the Fv file device path need to check on
                                 output, the updated valid Fv file device path
  @param  FileGuid               the Fv file guild

  @retval EFI_INVALID_PARAMETER  the input DevicePath or FileGuid is invalid
                                 parameter
  @retval EFI_UNSUPPORTED        the input DevicePath does not contain Fv file
                                 guild at all
  @retval EFI_ALREADY_STARTED    the input DevicePath has pointed to Fv file, it is
                                 valid
  @retval EFI_SUCCESS            has successfully updated the invalid DevicePath,
                                 and return the updated device path in DevicePath

**/
EFI_STATUS
EFIAPI
BdsLibUpdateFvFileDevicePath (
  IN  OUT EFI_DEVICE_PATH_PROTOCOL      ** DevicePath,
  IN  EFI_GUID                          *FileGuid
  );


/**
  Connect the specific Usb device which match the short form device path,
  and whose bus is determined by Host Controller (Uhci or Ehci)

  @param  HostControllerPI      Uhci (0x00) or Ehci (0x20) or Both uhci and ehci
                                (0xFF)
  @param  RemainingDevicePath   a short-form device path that starts with the first
                                element being a USB WWID or a USB Class device
                                path

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
  Convert Vendor device path to device name.

  @param  Str      The buffer store device name
  @param  DevPath  Pointer to vendor device path

**/
VOID
EFIAPI
DevPathVendor (
  IN OUT POOL_PRINT       *Str,
  IN VOID                 *DevPath
  );

/**
  Concatenates a formatted unicode string to allocated pool.
  The caller must free the resulting buffer.

  @param  Str      Tracks the allocated pool, size in use, and amount of pool allocated.
  @param  fmt      The format string
  @param  ...      The data will be printed.

  @return Allocated buffer with the formatted string printed in it.
          The caller must free the allocated buffer.
          The buffer allocation is not packed.

**/
CHAR16 *
EFIAPI
CatPrint (
  IN OUT POOL_PRINT   *Str,
  IN CHAR16           *fmt,
  ...
  );

/**
  Use Console Control to turn off UGA based Simple Text Out consoles from going
  to the UGA device. Put up LogoFile on every UGA device that is a console

  @param[in]  LogoFile   File name of logo to display on the center of the screen.

  @retval EFI_SUCCESS     ConsoleControl has been flipped to graphics and logo displayed.
  @retval EFI_UNSUPPORTED Logo not found

**/
EFI_STATUS
EFIAPI
EnableQuietBoot (
  IN  EFI_GUID  *LogoFile
  );


/**
  Use Console Control to turn on UGA based Simple Text Out consoles. The UGA 
  Simple Text Out screens will now be synced up with all non UGA output devices

  @retval EFI_SUCCESS     UGA devices are back in text mode and synced up.

**/
EFI_STATUS
EFIAPI
DisableQuietBoot (
  VOID
  );

/**
  Use Console Control Protocol to lock the Console In Spliter virtual handle. 
  This is the ConInHandle and ConIn handle in the EFI system table. All key
  presses will be ignored until the Password is typed in. The only way to
  disable the password is to type it in to a ConIn device.

  @param  Password        Password used to lock ConIn device.

  @retval EFI_SUCCESS     lock the Console In Spliter virtual handle successfully.
  @retval EFI_UNSUPPORTED Password not found

**/
EFI_STATUS
EFIAPI
LockKeyboards (
  IN  CHAR16    *Password
  );

#endif

