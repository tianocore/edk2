/** @file
  Provide Boot Manager related library APIs.

Copyright (c) 2011 - 2016, Intel Corporation. All rights reserved.<BR>
(C) Copyright 2015-2016 Hewlett Packard Enterprise Development LP<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/


#ifndef _UEFI_BOOT_MANAGER_LIB_H_
#define _UEFI_BOOT_MANAGER_LIB_H_

#include <Protocol/DriverHealth.h>
#include <Library/SortLib.h>

//
// Boot Manager load option library functions.
//

//
// Load Option Type
//
typedef enum {
  LoadOptionTypeDriver,
  LoadOptionTypeSysPrep,
  LoadOptionTypeBoot,
  LoadOptionTypePlatformRecovery,
  LoadOptionTypeMax
} EFI_BOOT_MANAGER_LOAD_OPTION_TYPE;

typedef enum {
  LoadOptionNumberMax = 0x10000,
  LoadOptionNumberUnassigned = LoadOptionNumberMax
} EFI_BOOT_MANAGER_LOAD_OPTION_NUMBER;

//
// Common structure definition for DriverOption and BootOption
//
typedef struct {
  //
  // Data read from UEFI NV variables
  //
  UINTN                             OptionNumber;       // #### numerical value, could be LoadOptionNumberUnassigned
  EFI_BOOT_MANAGER_LOAD_OPTION_TYPE OptionType;         // LoadOptionTypeBoot or LoadOptionTypeDriver
  UINT32                            Attributes;         // Load Option Attributes
  CHAR16                            *Description;       // Load Option Description
  EFI_DEVICE_PATH_PROTOCOL          *FilePath;          // Load Option Device Path
  UINT8                             *OptionalData;      // Load Option optional data to pass into image
  UINT32                            OptionalDataSize;   // Load Option size of OptionalData
  EFI_GUID                          VendorGuid;

  //
  // Used at runtime
  //
  EFI_STATUS                        Status;             // Status returned from boot attempt gBS->StartImage ()
  CHAR16                            *ExitData;          // Exit data returned from gBS->StartImage () 
  UINTN                             ExitDataSize;       // Size of ExitData
} EFI_BOOT_MANAGER_LOAD_OPTION;

/**
  Returns an array of load options based on the EFI variable
  L"BootOrder"/L"DriverOrder" and the L"Boot####"/L"Driver####" variables impled by it.
  #### is the hex value of the UINT16 in each BootOrder/DriverOrder entry. 

  @param  LoadOptionCount   Returns number of entries in the array.
  @param  LoadOptionType    The type of the load option.

  @retval NULL  No load options exist.
  @retval !NULL Array of load option entries.

**/
EFI_BOOT_MANAGER_LOAD_OPTION *
EFIAPI
EfiBootManagerGetLoadOptions (
  OUT UINTN                            *LoadOptionCount,
  IN EFI_BOOT_MANAGER_LOAD_OPTION_TYPE LoadOptionType
  );

/**
  Free an array of load options returned from EfiBootManagerGetLoadOptions().

  @param  LoadOptions      Pointer to the array of load options to free.
  @param  LoadOptionCount  Number of array entries in LoadOptions.

  @return EFI_SUCCESS           LoadOptions was freed.
  @return EFI_INVALID_PARAMETER LoadOptions is NULL.
**/
EFI_STATUS
EFIAPI
EfiBootManagerFreeLoadOptions (
  IN  EFI_BOOT_MANAGER_LOAD_OPTION  *LoadOptions,
  IN  UINTN                         LoadOptionCount
  );

/**
  Initialize a load option.

  @param Option           Pointer to the load option to be initialized.
  @param OptionNumber     Option number of the load option.
  @param OptionType       Type of the load option.
  @param Attributes       Attributes of the load option.
  @param Description      Description of the load option.
  @param FilePath         Device path of the load option.
  @param OptionalData     Optional data of the load option.
  @param OptionalDataSize Size of the optional data of the load option.

  @retval EFI_SUCCESS           The load option was initialized successfully.
  @retval EFI_INVALID_PARAMETER Option, Description or FilePath is NULL.
**/
EFI_STATUS
EFIAPI
EfiBootManagerInitializeLoadOption (
  IN OUT EFI_BOOT_MANAGER_LOAD_OPTION   *Option,
  IN  UINTN                             OptionNumber,
  IN  EFI_BOOT_MANAGER_LOAD_OPTION_TYPE OptionType,
  IN  UINT32                            Attributes,
  IN  CHAR16                            *Description,
  IN  EFI_DEVICE_PATH_PROTOCOL          *FilePath,
  IN  UINT8                             *OptionalData,
  IN  UINT32                            OptionalDataSize
  );

/**
  Free a load option created by EfiBootManagerInitializeLoadOption()
  or EfiBootManagerVariableToLoadOption().

  @param  LoadOption   Pointer to the load option to free.
  CONCERN: Check Boot#### instead of BootOrder, optimize, spec clarify
  @return EFI_SUCCESS           LoadOption was freed.
  @return EFI_INVALID_PARAMETER LoadOption is NULL.

**/
EFI_STATUS
EFIAPI
EfiBootManagerFreeLoadOption (
  IN  EFI_BOOT_MANAGER_LOAD_OPTION  *LoadOption
  );

/**
  Initialize the load option from the VariableName.

  @param  VariableName          EFI Variable name which could be Boot#### or
                                Driver####
  @param  LoadOption            Pointer to the load option to be initialized

  @retval EFI_SUCCESS           The option was created
  @retval EFI_INVALID_PARAMETER VariableName or LoadOption is NULL.
  @retval EFI_NOT_FOUND         The variable specified by VariableName cannot be found.
**/
EFI_STATUS
EFIAPI
EfiBootManagerVariableToLoadOption (
  IN CHAR16                           *VariableName,
  IN OUT EFI_BOOT_MANAGER_LOAD_OPTION *LoadOption
  );

/**
  Create the Boot#### or Driver#### variable from the load option.
  
  @param  LoadOption      Pointer to the load option.

  @retval EFI_SUCCESS     The variable was created.
  @retval Others          Error status returned by RT->SetVariable.
**/
EFI_STATUS
EFIAPI
EfiBootManagerLoadOptionToVariable (
  IN CONST EFI_BOOT_MANAGER_LOAD_OPTION     *LoadOption
  );

/**
  This function will update the Boot####/Driver####/SysPrep#### and the 
  BootOrder/DriverOrder/SysPrepOrder to add a new load option.

  @param  Option        Pointer to load option to add.
  @param  Position      Position of the new load option to put in the BootOrder/DriverOrder/SysPrepOrder.

  @retval EFI_SUCCESS   The load option has been successfully added.
  @retval Others        Error status returned by RT->SetVariable.
**/
EFI_STATUS
EFIAPI
EfiBootManagerAddLoadOptionVariable (
  IN EFI_BOOT_MANAGER_LOAD_OPTION  *Option,
  IN UINTN                         Position
  );

/**
  Delete the load option according to the OptionNumber and OptionType.
  
  Only the BootOrder/DriverOrder is updated to remove the reference of the OptionNumber.
  
  @param  OptionNumber        Option number of the load option.
  @param  OptionType          Type of the load option.

  @retval EFI_NOT_FOUND       The load option cannot be found.
  @retval EFI_SUCCESS         The load option was deleted.
**/
EFI_STATUS
EFIAPI
EfiBootManagerDeleteLoadOptionVariable (
  IN UINTN                              OptionNumber,
  IN EFI_BOOT_MANAGER_LOAD_OPTION_TYPE  OptionType
  );

/**
  Sort the load options. The DriverOrder/BootOrder variables will be re-created to 
  reflect the new order.

  @param OptionType        The type of the load option.
  @param CompareFunction   The comparator function pointer.
**/
VOID
EFIAPI
EfiBootManagerSortLoadOptionVariable (
  IN EFI_BOOT_MANAGER_LOAD_OPTION_TYPE OptionType,
  IN SORT_COMPARE                      CompareFunction
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
EFIAPI
EfiBootManagerFindLoadOption (
  IN CONST EFI_BOOT_MANAGER_LOAD_OPTION *Key,
  IN CONST EFI_BOOT_MANAGER_LOAD_OPTION *Array,
  IN UINTN                              Count
  );

//
// Boot Manager hot key library functions.
//

#pragma pack(1)
///
/// EFI Key Option.
///
typedef struct {
  ///
  /// Specifies options about how the key will be processed.
  ///
  EFI_BOOT_KEY_DATA  KeyData;
  ///
  /// The CRC-32 which should match the CRC-32 of the entire EFI_LOAD_OPTION to
  /// which BootOption refers. If the CRC-32s do not match this value, then this key
  /// option is ignored.
  ///
  UINT32             BootOptionCrc;
  ///
  /// The Boot#### option which will be invoked if this key is pressed and the boot option
  /// is active (LOAD_OPTION_ACTIVE is set).
  ///
  UINT16             BootOption;
  ///
  /// The key codes to compare against those returned by the
  /// EFI_SIMPLE_TEXT_INPUT and EFI_SIMPLE_TEXT_INPUT_EX protocols.
  /// The number of key codes (0-3) is specified by the EFI_KEY_CODE_COUNT field in KeyOptions.
  ///
  EFI_INPUT_KEY      Keys[3];
  UINT16             OptionNumber;
} EFI_BOOT_MANAGER_KEY_OPTION;
#pragma pack()

/**
  Start the hot key service so that the key press can trigger the boot option.

  @param HotkeyTriggered  Return the waitable event and it will be signaled 
                          when a valid hot key is pressed.

  @retval EFI_SUCCESS     The hot key service is started.
**/
EFI_STATUS
EFIAPI
EfiBootManagerStartHotkeyService (
  IN EFI_EVENT      *HotkeyTriggered
  );

//
// Modifier for EfiBootManagerAddKeyOptionVariable and EfiBootManagerDeleteKeyOptionVariable
//
#define EFI_BOOT_MANAGER_SHIFT_PRESSED    0x00000001
#define EFI_BOOT_MANAGER_CONTROL_PRESSED  0x00000002
#define EFI_BOOT_MANAGER_ALT_PRESSED      0x00000004
#define EFI_BOOT_MANAGER_LOGO_PRESSED     0x00000008
#define EFI_BOOT_MANAGER_MENU_KEY_PRESSED 0x00000010
#define EFI_BOOT_MANAGER_SYS_REQ_PRESSED  0x00000020

/**
  Add the key option.
  It adds the key option variable and the key option takes affect immediately.

  @param AddedOption      Return the added key option.
  @param BootOptionNumber The boot option number for the key option.
  @param Modifier         Key shift state.
  @param ...              Parameter list of pointer of EFI_INPUT_KEY.

  @retval EFI_SUCCESS         The key option is added.
  @retval EFI_ALREADY_STARTED The hot key is already used by certain key option.
**/
EFI_STATUS
EFIAPI
EfiBootManagerAddKeyOptionVariable (
  OUT EFI_BOOT_MANAGER_KEY_OPTION *AddedOption,   OPTIONAL
  IN UINT16                       BootOptionNumber,
  IN UINT32                       Modifier,
  ...
  );

/**
  Delete the Key Option variable and unregister the hot key

  @param DeletedOption  Return the deleted key options.
  @param Modifier       Key shift state.
  @param ...            Parameter list of pointer of EFI_INPUT_KEY.

  @retval EFI_SUCCESS   The key option is deleted.
  @retval EFI_NOT_FOUND The key option cannot be found.
**/
EFI_STATUS
EFIAPI
EfiBootManagerDeleteKeyOptionVariable (
  IN EFI_BOOT_MANAGER_KEY_OPTION *DeletedOption, OPTIONAL
  IN UINT32                      Modifier,
  ...
  );

/**
  Register the key option to exit the waiting of the Boot Manager timeout.
  Platform should ensure that the continue key option isn't conflict with
  other boot key options.

  @param Modifier     Key shift state.
  @param  ...         Parameter list of pointer of EFI_INPUT_KEY.

  @retval EFI_SUCCESS         Successfully register the continue key option.
  @retval EFI_ALREADY_STARTED The continue key option is already registered.
**/
EFI_STATUS
EFIAPI
EfiBootManagerRegisterContinueKeyOption (
  IN UINT32           Modifier,
  ...
  );

/**
  Try to boot the boot option triggered by hot key.
**/
VOID
EFIAPI
EfiBootManagerHotkeyBoot (
  VOID
  );
//
// Boot Manager boot library functions.
//

/**
  The function creates boot options for all possible bootable medias in the following order:
  1. Removable BlockIo            - The boot option only points to the removable media
                                    device, like USB key, DVD, Floppy etc.
  2. Fixed BlockIo                - The boot option only points to a Fixed blockIo device,
                                    like HardDisk.
  3. Non-BlockIo SimpleFileSystem - The boot option points to a device supporting
                                    SimpleFileSystem Protocol, but not supporting BlockIo
                                    protocol.
  4. LoadFile                     - The boot option points to the media supporting 
                                    LoadFile protocol.
  Reference: UEFI Spec chapter 3.3 Boot Option Variables Default Boot Behavior

  The function won't delete the boot option not added by itself.
**/
VOID
EFIAPI
EfiBootManagerRefreshAllBootOption (
  VOID
  );

/**
  Attempt to boot the EFI boot option. This routine sets L"BootCurent" and
  signals the EFI ready to boot event. If the device path for the option starts
  with a BBS device path a legacy boot is attempted. Short form device paths are
  also supported via this rountine. A device path starting with 
  MEDIA_HARDDRIVE_DP, MSG_USB_WWID_DP, MSG_USB_CLASS_DP gets expaned out
  to find the first device that matches. If the BootOption Device Path 
  fails the removable media boot algorithm is attempted (\EFI\BOOTIA32.EFI,
  \EFI\BOOTX64.EFI,... only one file type is tried per processor type)

  @param  BootOption    Boot Option to try and boot.
                        On return, BootOption->Status contains the boot status:
                        EFI_SUCCESS     BootOption was booted
                        EFI_UNSUPPORTED BootOption isn't supported.
                        EFI_NOT_FOUND   The BootOption was not found on the system
                        Others          BootOption failed with this error status

**/
VOID
EFIAPI
EfiBootManagerBoot (
  IN  EFI_BOOT_MANAGER_LOAD_OPTION  *BootOption
  );

/**
  Return the boot option corresponding to the Boot Manager Menu.
  It may automatically create one if the boot option hasn't been created yet.

  @param BootOption    Return the Boot Manager Menu.

  @retval EFI_SUCCESS   The Boot Manager Menu is successfully returned.
  @retval EFI_NOT_FOUND The Boot Manager Menu cannot be found.
  @retval others        Return status of gRT->SetVariable (). BootOption still points
                        to the Boot Manager Menu even the Status is not EFI_SUCCESS
                        and EFI_NOT_FOUND.
**/
EFI_STATUS
EFIAPI
EfiBootManagerGetBootManagerMenu (
  EFI_BOOT_MANAGER_LOAD_OPTION *BootOption
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
EFIAPI
EfiBootManagerGetLoadOptionBuffer (
  IN  EFI_DEVICE_PATH_PROTOCOL          *FilePath,
  OUT EFI_DEVICE_PATH_PROTOCOL          **FullPath,
  OUT UINTN                             *FileSize
  );

/**
  The function enumerates all the legacy boot options, creates them and 
  registers them in the BootOrder variable.
**/
typedef
VOID
(EFIAPI *EFI_BOOT_MANAGER_REFRESH_LEGACY_BOOT_OPTION) (
  VOID
  );

/**
  The function boots a legacy boot option.
**/
typedef
VOID
(EFIAPI *EFI_BOOT_MANAGER_LEGACY_BOOT) (
  IN  EFI_BOOT_MANAGER_LOAD_OPTION  *BootOption
  );

/**
  The function registers the legacy boot support capabilities.

  @param RefreshLegacyBootOption The function pointer to create all the legacy boot options.
  @param LegacyBoot              The function pointer to boot the legacy boot option.
**/
VOID
EFIAPI
EfiBootManagerRegisterLegacyBootSupport (
  EFI_BOOT_MANAGER_REFRESH_LEGACY_BOOT_OPTION   RefreshLegacyBootOption,
  EFI_BOOT_MANAGER_LEGACY_BOOT                  LegacyBoot
  );

/**
  Return the platform provided boot option description for the controller.

  @param Handle                Controller handle.
  @param DefaultDescription    Default boot description provided by core.

  @return  The callee allocated description string
           or NULL if the handler wants to use DefaultDescription.
**/
typedef
CHAR16 *
(EFIAPI *EFI_BOOT_MANAGER_BOOT_DESCRIPTION_HANDLER) (
  IN EFI_HANDLE                  Handle,
  IN CONST CHAR16                *DefaultDescription
  );

/**
  Register the platform provided boot description handler.

  @param Handler  The platform provided boot description handler

  @retval EFI_SUCCESS          The handler was registered successfully.
  @retval EFI_ALREADY_STARTED  The handler was already registered.
  @retval EFI_OUT_OF_RESOURCES There is not enough resource to perform the registration.
**/
EFI_STATUS
EFIAPI
EfiBootManagerRegisterBootDescriptionHandler (
  IN EFI_BOOT_MANAGER_BOOT_DESCRIPTION_HANDLER  Handler
  );

//
// Boot Manager connect and disconnect library functions
//

/**
  This function will connect all the system driver to controller
  first, and then special connect the default console, this make
  sure all the system controller available and the platform default
  console connected.
**/
VOID
EFIAPI
EfiBootManagerConnectAll (
  VOID
  );

/**
  This function will create all handles associate with every device
  path node. If the handle associate with one device path node can not
  be created successfully, then still give chance to do the dispatch,
  which load the missing drivers if possible.

  @param  DevicePathToConnect   The device path which will be connected, it can be
                                a multi-instance device path
  @param  MatchingHandle        Return the controller handle closest to the DevicePathToConnect

  @retval EFI_SUCCESS            All handles associate with every device path node
                                 have been created.
  @retval EFI_OUT_OF_RESOURCES   There is no resource to create new handles.
  @retval EFI_NOT_FOUND          Create the handle associate with one device path
                                 node failed.
  @retval EFI_SECURITY_VIOLATION The user has no permission to start UEFI device 
                                 drivers on the DevicePath.
**/
EFI_STATUS
EFIAPI
EfiBootManagerConnectDevicePath (
  IN  EFI_DEVICE_PATH_PROTOCOL  *DevicePathToConnect,
  OUT EFI_HANDLE                *MatchingHandle          OPTIONAL
  );

/**
  This function will disconnect all current system handles. 
  
  gBS->DisconnectController() is invoked for each handle exists in system handle buffer.
  If handle is a bus type handle, all childrens also are disconnected recursively by
  gBS->DisconnectController().
**/
VOID
EFIAPI
EfiBootManagerDisconnectAll (
  VOID
  );


//
// Boot Manager console library functions
//

typedef enum {
  ConIn,
  ConOut,
  ErrOut,
  ConInDev,
  ConOutDev,
  ErrOutDev,
  ConsoleTypeMax
} CONSOLE_TYPE;

/**
  This function will connect all the console devices base on the console
  device variable ConIn, ConOut and ErrOut.

  @retval EFI_DEVICE_ERROR         All the consoles were not connected due to an error.
  @retval EFI_SUCCESS              Success connect any one instance of the console
                                   device path base on the variable ConVarName.
**/
EFI_STATUS
EFIAPI
EfiBootManagerConnectAllDefaultConsoles (
  VOID
  );

/**
  This function updates the console variable based on ConVarName. It can
  add or remove one specific console device path from the variable

  @param  ConsoleType              ConIn, ConOut, ErrOut, ConInDev, ConOutDev or ErrOutDev.
  @param  CustomizedConDevicePath  The console device path to be added to
                                   the console variable. Cannot be multi-instance.
  @param  ExclusiveDevicePath      The console device path to be removed
                                   from the console variable. Cannot be multi-instance.

  @retval EFI_UNSUPPORTED          The added device path is the same as a removed one.
  @retval EFI_SUCCESS              Successfully added or removed the device path from the
                                   console variable.

**/
EFI_STATUS
EFIAPI
EfiBootManagerUpdateConsoleVariable (
  IN  CONSOLE_TYPE              ConsoleType,
  IN  EFI_DEVICE_PATH_PROTOCOL  *CustomizedConDevicePath,
  IN  EFI_DEVICE_PATH_PROTOCOL  *ExclusiveDevicePath
  );

/**
  Connect the console device base on the variable ConVarName, if
  device path of the ConVarName is multi-instance device path, if
  anyone of the instances is connected success, then this function
  will return success.

  @param  ConsoleType              ConIn, ConOut or ErrOut.

  @retval EFI_NOT_FOUND            There is not any console devices connected
                                   success
  @retval EFI_SUCCESS              Success connect any one instance of the console
                                   device path base on the variable ConVarName.

**/
EFI_STATUS
EFIAPI
EfiBootManagerConnectConsoleVariable (
  IN  CONSOLE_TYPE              ConsoleType
  );

/**
  Query all the children of VideoController and return the device paths of all the 
  children that support GraphicsOutput protocol.

  @param VideoController       PCI handle of video controller.

  @return  Device paths of all the children that support GraphicsOutput protocol.
**/
EFI_DEVICE_PATH_PROTOCOL *
EFIAPI
EfiBootManagerGetGopDevicePath (
  IN  EFI_HANDLE               VideoController
  );

/**
  Connect the platform active active video controller.

  @param VideoController       PCI handle of video controller.

  @retval EFI_NOT_FOUND There is no active video controller.
  @retval EFI_SUCCESS   The video controller is connected.
**/
EFI_STATUS
EFIAPI
EfiBootManagerConnectVideoController (
  EFI_HANDLE                 VideoController  OPTIONAL
  );

//
// Boot Manager driver health library functions.
//

typedef struct {
  EFI_DRIVER_HEALTH_PROTOCOL      *DriverHealth;

  ///
  /// Driver relative handles
  ///
  EFI_HANDLE                      DriverHealthHandle;
  EFI_HANDLE                      ControllerHandle;
  EFI_HANDLE                      ChildHandle;

  ///
  /// Driver health messages of the specify Driver 
  ///
  EFI_DRIVER_HEALTH_HII_MESSAGE   *MessageList;

  ///
  /// HII relative handles
  ///
  EFI_HII_HANDLE                  HiiHandle;

  ///
  /// Driver Health status
  ///
  EFI_DRIVER_HEALTH_STATUS        HealthStatus;
} EFI_BOOT_MANAGER_DRIVER_HEALTH_INFO;

/**
  Return all the Driver Health information.

  When the cumulative health status of all the controllers managed by the
  driver who produces the EFI_DRIVER_HEALTH_PROTOCOL is healthy, only one
  EFI_BOOT_MANAGER_DRIVER_HEALTH_INFO entry is created for such
  EFI_DRIVER_HEALTH_PROTOCOL instance.
  Otherwise, every controller creates one EFI_BOOT_MANAGER_DRIVER_HEALTH_INFO
  entry. Additionally every child controller creates one
  EFI_BOOT_MANAGER_DRIVER_HEALTH_INFO entry if the driver is a bus driver.

  @param Count      Return the count of the Driver Health information.

  @retval NULL      No Driver Health information is returned.
  @retval !NULL     Pointer to the Driver Health information array.
**/
EFI_BOOT_MANAGER_DRIVER_HEALTH_INFO *
EFIAPI
EfiBootManagerGetDriverHealthInfo (
  UINTN    *Count
  );

/**
  Free the Driver Health information array.

  @param DriverHealthInfo       Pointer to array of the Driver Health information.
  @param Count                  Count of the array.

  @retval EFI_SUCCESS           The array is freed.
  @retval EFI_INVALID_PARAMETER The array is NULL.
**/
EFI_STATUS
EFIAPI
EfiBootManagerFreeDriverHealthInfo (
  EFI_BOOT_MANAGER_DRIVER_HEALTH_INFO  *DriverHealthInfo,
  UINTN                                Count
  );

/**
  Process (load and execute) the load option.

  @param LoadOption  Pointer to the load option.

  @retval EFI_INVALID_PARAMETER  The load option type is invalid, 
                                 or the load option file path doesn't point to a valid file.
  @retval EFI_UNSUPPORTED        The load option type is of LoadOptionTypeBoot.
  @retval EFI_SUCCESS            The load option is inactive, or successfully loaded and executed.
**/
EFI_STATUS
EFIAPI
EfiBootManagerProcessLoadOption (
  EFI_BOOT_MANAGER_LOAD_OPTION       *LoadOption
  );

/**
  Check whether the VariableName is a valid load option variable name
  and return the load option type and option number.

  @param VariableName The name of the load option variable.
  @param OptionType   Return the load option type.
  @param OptionNumber Return the load option number.

  @retval TRUE  The variable name is valid; The load option type and
                load option number are returned.
  @retval FALSE The variable name is NOT valid.
**/
BOOLEAN
EFIAPI
EfiBootManagerIsValidLoadOptionVariableName (
  IN CHAR16                             *VariableName,
  OUT EFI_BOOT_MANAGER_LOAD_OPTION_TYPE *OptionType   OPTIONAL,
  OUT UINT16                            *OptionNumber OPTIONAL
  );


/**
  Dispatch the deferred images that are returned from all DeferredImageLoad instances.

  @retval EFI_SUCCESS       At least one deferred image is loaded successfully and started.
  @retval EFI_NOT_FOUND     There is no deferred image.
  @retval EFI_ACCESS_DENIED There are deferred images but all of them are failed to load.
**/
EFI_STATUS
EFIAPI
EfiBootManagerDispatchDeferredImages (
  VOID
  );
#endif
