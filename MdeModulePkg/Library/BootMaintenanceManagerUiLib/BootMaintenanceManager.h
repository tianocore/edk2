/** @file
Header file for boot maintenance module.

Copyright (c) 2004 - 2019, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _BOOT_MAINT_H_
#define _BOOT_MAINT_H_

#include "FormGuid.h"

#include <Guid/TtyTerm.h>
#include <Guid/MdeModuleHii.h>
#include <Guid/FileSystemVolumeLabelInfo.h>
#include <Guid/GlobalVariable.h>
#include <Guid/HiiBootMaintenanceFormset.h>

#include <Protocol/LoadFile.h>
#include <Protocol/HiiConfigAccess.h>
#include <Protocol/SimpleFileSystem.h>
#include <Protocol/SerialIo.h>
#include <Protocol/DevicePathToText.h>
#include <Protocol/FormBrowserEx2.h>

#include <Library/PrintLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/DevicePathLib.h>
#include <Library/HiiLib.h>
#include <Library/UefiHiiServicesLib.h>
#include <Library/UefiBootManagerLib.h>
#include <Library/FileExplorerLib.h>
#include "BootMaintenanceManagerCustomizedUi.h"

#pragma pack(1)

///
/// HII specific Vendor Device Path definition.
///
typedef struct {
  VENDOR_DEVICE_PATH          VendorDevicePath;
  EFI_DEVICE_PATH_PROTOCOL    End;
} HII_VENDOR_DEVICE_PATH;
#pragma pack()

//
// Constants which are variable names used to access variables
//

#define VAR_CON_OUT_MODE  L"ConOutMode"

//
// Variable created with this flag will be "Efi:...."
//
#define VAR_FLAG  EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_NON_VOLATILE

extern EFI_GUID  mBootMaintGuid;
extern CHAR16    mBootMaintStorageName[];
//
// These are the VFR compiler generated data representing our VFR data.
//
extern UINT8  BootMaintenanceManagerBin[];

//
// Below are the number of options in Baudrate, Databits,
// Parity and Stopbits selection for serial ports.
//
#define BM_COM_ATTR_BUADRATE  19
#define BM_COM_ATTR_DATABITS  4
#define BM_COM_ATTR_PARITY    5
#define BM_COM_ATTR_STOPBITS  3

//
// Callback function helper
//
#define BMM_CALLBACK_DATA_SIGNATURE  SIGNATURE_32 ('C', 'b', 'c', 'k')
#define BMM_CALLBACK_DATA_FROM_THIS(a)  CR (a, BMM_CALLBACK_DATA, BmmConfigAccess, BMM_CALLBACK_DATA_SIGNATURE)

//
// Enumeration type definition
//
typedef UINT8 BBS_TYPE;

typedef enum _TYPE_OF_TERMINAL {
  TerminalTypePcAnsi = 0,
  TerminalTypeVt100,
  TerminalTypeVt100Plus,
  TerminalTypeVtUtf8,
  TerminalTypeTtyTerm,
  TerminalTypeLinux,
  TerminalTypeXtermR6,
  TerminalTypeVt400,
  TerminalTypeSCO
} TYPE_OF_TERMINAL;

//
// All of the signatures that will be used in list structure
//
#define BM_MENU_OPTION_SIGNATURE      SIGNATURE_32 ('m', 'e', 'n', 'u')
#define BM_LOAD_OPTION_SIGNATURE      SIGNATURE_32 ('l', 'o', 'a', 'd')
#define BM_CONSOLE_OPTION_SIGNATURE   SIGNATURE_32 ('c', 'n', 's', 'l')
#define BM_FILE_OPTION_SIGNATURE      SIGNATURE_32 ('f', 'i', 'l', 'e')
#define BM_HANDLE_OPTION_SIGNATURE    SIGNATURE_32 ('h', 'n', 'd', 'l')
#define BM_TERMINAL_OPTION_SIGNATURE  SIGNATURE_32 ('t', 'r', 'm', 'l')
#define BM_MENU_ENTRY_SIGNATURE       SIGNATURE_32 ('e', 'n', 't', 'r')

#define BM_LOAD_CONTEXT_SELECT      0x0
#define BM_CONSOLE_CONTEXT_SELECT   0x1
#define BM_FILE_CONTEXT_SELECT      0x2
#define BM_HANDLE_CONTEXT_SELECT    0x3
#define BM_TERMINAL_CONTEXT_SELECT  0x5

#define BM_CONSOLE_IN_CONTEXT_SELECT   0x6
#define BM_CONSOLE_OUT_CONTEXT_SELECT  0x7
#define BM_CONSOLE_ERR_CONTEXT_SELECT  0x8

//
// Buffer size for update data
//
#define UPDATE_DATA_SIZE  0x100000

//
// Namespace of callback keys used in display and file system navigation
//
#define MAX_BBS_OFFSET          0xE000
#define NET_OPTION_OFFSET       0xD800
#define BEV_OPTION_OFFSET       0xD000
#define FD_OPTION_OFFSET        0xC000
#define HD_OPTION_OFFSET        0xB000
#define CD_OPTION_OFFSET        0xA000
#define FILE_OPTION_OFFSET      0x8000
#define FILE_OPTION_MASK        0x7FFF
#define HANDLE_OPTION_OFFSET    0x7000
#define CONSOLE_OPTION_OFFSET   0x6000
#define TERMINAL_OPTION_OFFSET  0x5000
#define CONFIG_OPTION_OFFSET    0x1200
#define KEY_VALUE_OFFSET        0x1100
#define FORM_ID_OFFSET          0x1000

//
// VarOffset that will be used to create question
// all these values are computed from the structure
// defined below
//
#define VAR_OFFSET(Field)  ((UINT16) ((UINTN) &(((BMM_FAKE_NV_DATA *) 0)->Field)))

//
// Question Id of Zero is invalid, so add an offset to it
//
#define QUESTION_ID(Field)  (VAR_OFFSET (Field) + CONFIG_OPTION_OFFSET)

#define BOOT_TIME_OUT_VAR_OFFSET        VAR_OFFSET (BootTimeOut)
#define BOOT_NEXT_VAR_OFFSET            VAR_OFFSET (BootNext)
#define COM1_BAUD_RATE_VAR_OFFSET       VAR_OFFSET (COM1BaudRate)
#define COM1_DATA_RATE_VAR_OFFSET       VAR_OFFSET (COM1DataRate)
#define COM1_STOP_BITS_VAR_OFFSET       VAR_OFFSET (COM1StopBits)
#define COM1_PARITY_VAR_OFFSET          VAR_OFFSET (COM1Parity)
#define COM1_TERMINAL_VAR_OFFSET        VAR_OFFSET (COM2TerminalType)
#define COM2_BAUD_RATE_VAR_OFFSET       VAR_OFFSET (COM2BaudRate)
#define COM2_DATA_RATE_VAR_OFFSET       VAR_OFFSET (COM2DataRate)
#define COM2_STOP_BITS_VAR_OFFSET       VAR_OFFSET (COM2StopBits)
#define COM2_PARITY_VAR_OFFSET          VAR_OFFSET (COM2Parity)
#define COM2_TERMINAL_VAR_OFFSET        VAR_OFFSET (COM2TerminalType)
#define DRV_ADD_HANDLE_DESC_VAR_OFFSET  VAR_OFFSET (DriverAddHandleDesc)
#define DRV_ADD_ACTIVE_VAR_OFFSET       VAR_OFFSET (DriverAddActive)
#define DRV_ADD_RECON_VAR_OFFSET        VAR_OFFSET (DriverAddForceReconnect)
#define CON_IN_COM1_VAR_OFFSET          VAR_OFFSET (ConsoleInputCOM1)
#define CON_IN_COM2_VAR_OFFSET          VAR_OFFSET (ConsoleInputCOM2)
#define CON_OUT_COM1_VAR_OFFSET         VAR_OFFSET (ConsoleOutputCOM1)
#define CON_OUT_COM2_VAR_OFFSET         VAR_OFFSET (ConsoleOutputCOM2)
#define CON_ERR_COM1_VAR_OFFSET         VAR_OFFSET (ConsoleErrorCOM1)
#define CON_ERR_COM2_VAR_OFFSET         VAR_OFFSET (ConsoleErrorCOM2)
#define CON_MODE_VAR_OFFSET             VAR_OFFSET (ConsoleOutMode)
#define CON_DEVICE_VAR_OFFSET           VAR_OFFSET (ConsoleCheck)
#define CON_IN_DEVICE_VAR_OFFSET        VAR_OFFSET (ConsoleInCheck)
#define CON_OUT_DEVICE_VAR_OFFSET       VAR_OFFSET (ConsoleOutCheck)
#define CON_ERR_DEVICE_VAR_OFFSET       VAR_OFFSET (ConsoleErrCheck)
#define BOOT_OPTION_ORDER_VAR_OFFSET    VAR_OFFSET (BootOptionOrder)
#define DRIVER_OPTION_ORDER_VAR_OFFSET  VAR_OFFSET (DriverOptionOrder)
#define BOOT_OPTION_DEL_VAR_OFFSET      VAR_OFFSET (BootOptionDel)
#define DRIVER_OPTION_DEL_VAR_OFFSET    VAR_OFFSET (DriverOptionDel)
#define DRIVER_ADD_OPTION_VAR_OFFSET    VAR_OFFSET (DriverAddHandleOptionalData)
#define COM_BAUD_RATE_VAR_OFFSET        VAR_OFFSET (COMBaudRate)
#define COM_DATA_RATE_VAR_OFFSET        VAR_OFFSET (COMDataRate)
#define COM_STOP_BITS_VAR_OFFSET        VAR_OFFSET (COMStopBits)
#define COM_PARITY_VAR_OFFSET           VAR_OFFSET (COMParity)
#define COM_TERMINAL_VAR_OFFSET         VAR_OFFSET (COMTerminalType)
#define COM_FLOWCONTROL_VAR_OFFSET      VAR_OFFSET (COMFlowControl)

#define BOOT_TIME_OUT_QUESTION_ID        QUESTION_ID (BootTimeOut)
#define BOOT_NEXT_QUESTION_ID            QUESTION_ID (BootNext)
#define COM1_BAUD_RATE_QUESTION_ID       QUESTION_ID (COM1BaudRate)
#define COM1_DATA_RATE_QUESTION_ID       QUESTION_ID (COM1DataRate)
#define COM1_STOP_BITS_QUESTION_ID       QUESTION_ID (COM1StopBits)
#define COM1_PARITY_QUESTION_ID          QUESTION_ID (COM1Parity)
#define COM1_TERMINAL_QUESTION_ID        QUESTION_ID (COM2TerminalType)
#define COM2_BAUD_RATE_QUESTION_ID       QUESTION_ID (COM2BaudRate)
#define COM2_DATA_RATE_QUESTION_ID       QUESTION_ID (COM2DataRate)
#define COM2_STOP_BITS_QUESTION_ID       QUESTION_ID (COM2StopBits)
#define COM2_PARITY_QUESTION_ID          QUESTION_ID (COM2Parity)
#define COM2_TERMINAL_QUESTION_ID        QUESTION_ID (COM2TerminalType)
#define DRV_ADD_HANDLE_DESC_QUESTION_ID  QUESTION_ID (DriverAddHandleDesc)
#define DRV_ADD_ACTIVE_QUESTION_ID       QUESTION_ID (DriverAddActive)
#define DRV_ADD_RECON_QUESTION_ID        QUESTION_ID (DriverAddForceReconnect)
#define CON_IN_COM1_QUESTION_ID          QUESTION_ID (ConsoleInputCOM1)
#define CON_IN_COM2_QUESTION_ID          QUESTION_ID (ConsoleInputCOM2)
#define CON_OUT_COM1_QUESTION_ID         QUESTION_ID (ConsoleOutputCOM1)
#define CON_OUT_COM2_QUESTION_ID         QUESTION_ID (ConsoleOutputCOM2)
#define CON_ERR_COM1_QUESTION_ID         QUESTION_ID (ConsoleErrorCOM1)
#define CON_ERR_COM2_QUESTION_ID         QUESTION_ID (ConsoleErrorCOM2)
#define CON_MODE_QUESTION_ID             QUESTION_ID (ConsoleOutMode)
#define CON_DEVICE_QUESTION_ID           QUESTION_ID (ConsoleCheck)
#define CON_IN_DEVICE_QUESTION_ID        QUESTION_ID (ConsoleInCheck)
#define CON_OUT_DEVICE_QUESTION_ID       QUESTION_ID (ConsoleOutCheck)
#define CON_ERR_DEVICE_QUESTION_ID       QUESTION_ID (ConsoleErrCheck)
#define BOOT_OPTION_ORDER_QUESTION_ID    QUESTION_ID (BootOptionOrder)
#define DRIVER_OPTION_ORDER_QUESTION_ID  QUESTION_ID (DriverOptionOrder)
#define BOOT_OPTION_DEL_QUESTION_ID      QUESTION_ID (BootOptionDel)
#define DRIVER_OPTION_DEL_QUESTION_ID    QUESTION_ID (DriverOptionDel)
#define DRIVER_ADD_OPTION_QUESTION_ID    QUESTION_ID (DriverAddHandleOptionalData)
#define COM_BAUD_RATE_QUESTION_ID        QUESTION_ID (COMBaudRate)
#define COM_DATA_RATE_QUESTION_ID        QUESTION_ID (COMDataRate)
#define COM_STOP_BITS_QUESTION_ID        QUESTION_ID (COMStopBits)
#define COM_PARITY_QUESTION_ID           QUESTION_ID (COMParity)
#define COM_TERMINAL_QUESTION_ID         QUESTION_ID (COMTerminalType)
#define COM_FLOWCONTROL_QUESTION_ID      QUESTION_ID (COMFlowControl)

#define STRING_DEPOSITORY_NUMBER  8

#define NONE_BOOTNEXT_VALUE  (0xFFFF + 1)

///
/// Serial Ports attributes, first one is the value for
/// return from callback function, stringtoken is used to
/// display the value properly
///
typedef struct {
  UINTN     Value;
  UINT16    StringToken;
} COM_ATTR;

typedef struct {
  UINT64                      BaudRate;
  UINT8                       DataBits;
  UINT8                       Parity;
  UINT8                       StopBits;

  UINT8                       BaudRateIndex;
  UINT8                       DataBitsIndex;
  UINT8                       ParityIndex;
  UINT8                       StopBitsIndex;

  UINT8                       FlowControl;

  UINT8                       IsConIn;
  UINT8                       IsConOut;
  UINT8                       IsStdErr;
  UINT8                       TerminalType;

  EFI_DEVICE_PATH_PROTOCOL    *DevicePath;
} BM_TERMINAL_CONTEXT;

typedef struct {
  BOOLEAN                     IsBootNext;
  BOOLEAN                     Deleted;

  BOOLEAN                     IsLegacy;

  UINT32                      Attributes;
  UINT16                      FilePathListLength;
  UINT16                      *Description;
  EFI_DEVICE_PATH_PROTOCOL    *FilePathList;
  UINT8                       *OptionalData;
} BM_LOAD_CONTEXT;

typedef struct {
  BOOLEAN                     IsActive;

  BOOLEAN                     IsTerminal;

  EFI_DEVICE_PATH_PROTOCOL    *DevicePath;
} BM_CONSOLE_CONTEXT;

typedef struct {
  UINTN    Column;
  UINTN    Row;
} CONSOLE_OUT_MODE;

typedef struct {
  EFI_HANDLE                      Handle;
  EFI_DEVICE_PATH_PROTOCOL        *DevicePath;
  EFI_FILE_HANDLE                 FHandle;
  UINT16                          *FileName;
  EFI_FILE_SYSTEM_VOLUME_LABEL    *Info;

  BOOLEAN                         IsRoot;
  BOOLEAN                         IsDir;
  BOOLEAN                         IsRemovableMedia;
  BOOLEAN                         IsLoadFile;
  BOOLEAN                         IsBootLegacy;
} BM_FILE_CONTEXT;

typedef struct {
  EFI_HANDLE                  Handle;
  EFI_DEVICE_PATH_PROTOCOL    *DevicePath;
} BM_HANDLE_CONTEXT;

typedef struct {
  UINTN         Signature;
  LIST_ENTRY    Head;
  UINTN         MenuNumber;
} BM_MENU_OPTION;

typedef struct {
  UINTN            Signature;
  LIST_ENTRY       Link;
  UINTN            OptionNumber;
  UINT16           *DisplayString;
  UINT16           *HelpString;
  EFI_STRING_ID    DisplayStringToken;
  EFI_STRING_ID    HelpStringToken;
  UINTN            ContextSelection;
  VOID             *VariableContext;
} BM_MENU_ENTRY;

typedef struct {
  UINTN                             Signature;

  EFI_HII_HANDLE                    BmmHiiHandle;
  EFI_HANDLE                        BmmDriverHandle;
  ///
  /// Boot Maintenance  Manager Produced protocols
  ///
  EFI_HII_CONFIG_ACCESS_PROTOCOL    BmmConfigAccess;
  EFI_FORM_BROWSER2_PROTOCOL        *FormBrowser2;

  BM_MENU_ENTRY                     *MenuEntry;
  BM_HANDLE_CONTEXT                 *HandleContext;
  BM_FILE_CONTEXT                   *FileContext;
  BM_LOAD_CONTEXT                   *LoadContext;
  BM_TERMINAL_CONTEXT               *TerminalContext;
  UINTN                             CurrentTerminal;
  BBS_TYPE                          BbsType;

  //
  // BMM main formset callback data.
  //

  EFI_FORM_ID                       BmmCurrentPageId;
  EFI_FORM_ID                       BmmPreviousPageId;
  BOOLEAN                           BmmAskSaveOrNot;
  BMM_FAKE_NV_DATA                  BmmFakeNvData;
  BMM_FAKE_NV_DATA                  BmmOldFakeNVData;
} BMM_CALLBACK_DATA;

/**

  Find drivers that will be added as Driver#### variables from handles
  in current system environment
  All valid handles in the system except those consume SimpleFs, LoadFile
  are stored in DriverMenu for future use.

  @retval EFI_SUCCESS The function complets successfully.
  @return Other value if failed to build the DriverMenu.

**/
EFI_STATUS
BOpt_FindDrivers (
  VOID
  );

/**

  Build the BootOptionMenu according to BootOrder Variable.
  This Routine will access the Boot#### to get EFI_LOAD_OPTION.

  @param CallbackData The BMM context data.

  @return The number of the Var Boot####.

**/
EFI_STATUS
BOpt_GetBootOptions (
  IN  BMM_CALLBACK_DATA  *CallbackData
  );

/**

  Build up all DriverOptionMenu

  @param CallbackData The BMM context data.

  @return EFI_SUCESS The functin completes successfully.
  @retval EFI_OUT_OF_RESOURCES Not enough memory to compete the operation.


**/
EFI_STATUS
BOpt_GetDriverOptions (
  IN  BMM_CALLBACK_DATA  *CallbackData
  );

/**
  Free resources allocated in Allocate Rountine.

  @param FreeMenu        Menu to be freed

**/
VOID
BOpt_FreeMenu (
  BM_MENU_OPTION  *FreeMenu
  );

/**

  Get the Option Number that has not been allocated for use.

  @param Type  The type of Option.

  @return The available Option Number.

**/
UINT16
BOpt_GetOptionNumber (
  CHAR16  *Type
  );

/**

  Get the Option Number for Boot#### that does not used.

  @return The available Option Number.

**/
UINT16
BOpt_GetBootOptionNumber (
  VOID
  );

/**

Get the Option Number for Driver#### that does not used.

@return The unused Option Number.

**/
UINT16
BOpt_GetDriverOptionNumber (
  VOID
  );

/**
  Create a menu entry give a Menu type.

  @param MenuType        The Menu type to be created.


  @retval NULL           If failed to create the menu.
  @return                The menu.

**/
BM_MENU_ENTRY                     *
BOpt_CreateMenuEntry (
  UINTN  MenuType
  );

/**
  Free up all resource allocated for a BM_MENU_ENTRY.

  @param MenuEntry   A pointer to BM_MENU_ENTRY.

**/
VOID
BOpt_DestroyMenuEntry (
  BM_MENU_ENTRY  *MenuEntry
  );

/**
  Get the Menu Entry from the list in Menu Entry List.

  If MenuNumber is great or equal to the number of Menu
  Entry in the list, then ASSERT.

  @param MenuOption      The Menu Entry List to read the menu entry.
  @param MenuNumber      The index of Menu Entry.

  @return The Menu Entry.

**/
BM_MENU_ENTRY                     *
BOpt_GetMenuEntry (
  BM_MENU_OPTION  *MenuOption,
  UINTN           MenuNumber
  );

/**
  Get option number according to Boot#### and BootOrder variable.
  The value is saved as #### + 1.

  @param CallbackData    The BMM context data.
**/
VOID
GetBootOrder (
  IN  BMM_CALLBACK_DATA  *CallbackData
  );

/**
  Get driver option order from globalc DriverOptionMenu.

  @param CallbackData    The BMM context data.

**/
VOID
GetDriverOrder (
  IN  BMM_CALLBACK_DATA  *CallbackData
  );

//
// Locate all serial io devices for console
//

/**
  Build a list containing all serial devices.

  @retval EFI_SUCCESS The function complete successfully.
  @retval EFI_UNSUPPORTED No serial ports present.

**/
EFI_STATUS
LocateSerialIo (
  VOID
  );

//
// Initializing Console menu
//

/**
  Build up ConsoleOutMenu, ConsoleInpMenu and ConsoleErrMenu

  @retval EFI_SUCCESS    The function always complete successfully.

**/
EFI_STATUS
GetAllConsoles (
  VOID
  );

//
// Get current mode information
//

/**
  Get mode number according to column and row

  @param CallbackData    The BMM context data.
**/
VOID
GetConsoleOutMode (
  IN  BMM_CALLBACK_DATA  *CallbackData
  );

//
// Cleaning up console menu
//

/**
  Free ConsoleOutMenu, ConsoleInpMenu and ConsoleErrMenu

  @retval EFI_SUCCESS    The function always complete successfully.
**/
EFI_STATUS
FreeAllConsoles (
  VOID
  );

/**
  Update the device path that describing a terminal device
  based on the new BaudRate, Data Bits, parity and Stop Bits
  set.

  @param DevicePath     The devicepath protocol instance wanted to be updated.

**/
VOID
ChangeVariableDevicePath (
  IN OUT EFI_DEVICE_PATH_PROTOCOL  *DevicePath
  );

/**
  Update the multi-instance device path of Terminal Device based on
  the global TerminalMenu. If ChangeTernimal is TRUE, the terminal
  device path in the Terminal Device in TerminalMenu is also updated.

  @param DevicePath      The multi-instance device path.
  @param ChangeTerminal  TRUE, then device path in the Terminal Device
                         in TerminalMenu is also updated; FALSE, no update.

  @return EFI_SUCCESS    The function completes successfully.

**/
EFI_STATUS
ChangeTerminalDevicePath (
  IN OUT EFI_DEVICE_PATH_PROTOCOL  *DevicePath,
  IN BOOLEAN                       ChangeTerminal
  );

//
// Variable operation by menu selection
//

/**
  This function create a currently loaded Boot Option from
  the BMM. It then appends this Boot Option to the end of
  the "BootOrder" list. It also append this Boot Opotion to the end
  of BootOptionMenu.

  @param CallbackData           The BMM context data.

  @retval EFI_OUT_OF_RESOURCES  If not enought memory to complete the operation.
  @retval EFI_SUCCESS           If function completes successfully.

**/
EFI_STATUS
Var_UpdateBootOption (
  IN  BMM_CALLBACK_DATA  *CallbackData
  );

/**
  Delete Boot Option that represent a Deleted state in BootOptionMenu.

  @retval EFI_SUCCESS   If all boot load option EFI Variables corresponding to
                        BM_LOAD_CONTEXT marked for deletion is deleted
  @return Others        If failed to update the "BootOrder" variable after deletion.

**/
EFI_STATUS
Var_DelBootOption (
  VOID
  );

/**
  This function create a currently loaded Drive Option from
  the BMM. It then appends this Driver Option to the end of
  the "DriverOrder" list. It append this Driver Opotion to the end
  of DriverOptionMenu.

  @param CallbackData    The BMM context data.
  @param HiiHandle       The HII handle associated with the BMM formset.
  @param DescriptionData The description of this driver option.
  @param OptionalData    The optional load option.
  @param ForceReconnect  If to force reconnect.

  @retval EFI_OUT_OF_RESOURCES If not enought memory to complete the operation.
  @retval EFI_SUCCESS          If function completes successfully.

**/
EFI_STATUS
Var_UpdateDriverOption (
  IN  BMM_CALLBACK_DATA  *CallbackData,
  IN  EFI_HII_HANDLE     HiiHandle,
  IN  UINT16             *DescriptionData,
  IN  UINT16             *OptionalData,
  IN  UINT8              ForceReconnect
  );

/**
  Delete Load Option that represent a Deleted state in DriverOptionMenu.

  @retval EFI_SUCCESS Load Option is successfully updated.
  @return Other value than EFI_SUCCESS if failed to update "Driver Order" EFI
          Variable.

**/
EFI_STATUS
Var_DelDriverOption (
  VOID
  );

/**
  This function delete and build multi-instance device path ConIn
  console device.

  @retval EFI_SUCCESS    The function complete successfully.
  @return The EFI variable can not be saved. See gRT->SetVariable for detail return information.
**/
EFI_STATUS
Var_UpdateConsoleInpOption (
  VOID
  );

/**
  This function delete and build multi-instance device path ConOut console device.

  @retval EFI_SUCCESS    The function complete successfully.
  @return The EFI variable can not be saved. See gRT->SetVariable for detail return information.
**/
EFI_STATUS
Var_UpdateConsoleOutOption (
  VOID
  );

/**
  This function delete and build multi-instance device path ErrOut console device.

  @retval EFI_SUCCESS    The function complete successfully.
  @return The EFI variable can not be saved. See gRT->SetVariable for detail return information.
**/
EFI_STATUS
Var_UpdateErrorOutOption (
  VOID
  );

/**
  This function delete and build Out of Band console device.

  @param   MenuIndex   Menu index which user select in the terminal menu list.

  @retval EFI_SUCCESS    The function complete successfully.
  @return The EFI variable can not be saved. See gRT->SetVariable for detail return information.
**/
EFI_STATUS
Var_UpdateOutOfBandOption (
  IN  UINT16  MenuIndex
  );

/**
  This function update the "BootNext" EFI Variable. If there is no "BootNex" specified in BMM,
  this EFI Variable is deleted.
  It also update the BMM context data specified the "BootNext" value.

  @param CallbackData    The BMM context data.

  @retval EFI_SUCCESS    The function complete successfully.
  @return The EFI variable can not be saved. See gRT->SetVariable for detail return information.

**/
EFI_STATUS
Var_UpdateBootNext (
  IN BMM_CALLBACK_DATA  *CallbackData
  );

/**
  This function update the "BootOrder" EFI Variable based on BMM Formset's NV map. It then refresh
  BootOptionMenu with the new "BootOrder" list.

  @param CallbackData           The BMM context data.

  @retval EFI_SUCCESS           The function complete successfully.
  @retval EFI_OUT_OF_RESOURCES  Not enough memory to complete the function.
  @return not The EFI variable can not be saved. See gRT->SetVariable for detail return information.

**/
EFI_STATUS
Var_UpdateBootOrder (
  IN BMM_CALLBACK_DATA  *CallbackData
  );

/**
  This function update the "DriverOrder" EFI Variable based on
  BMM Formset's NV map. It then refresh DriverOptionMenu
  with the new "DriverOrder" list.

  @param CallbackData    The BMM context data.

  @retval EFI_SUCCESS           The function complete successfully.
  @retval EFI_OUT_OF_RESOURCES  Not enough memory to complete the function.
  @return The EFI variable can not be saved. See gRT->SetVariable for detail return information.

**/
EFI_STATUS
Var_UpdateDriverOrder (
  IN BMM_CALLBACK_DATA  *CallbackData
  );

/**
  Update the Text Mode of Console.

  @param CallbackData  The context data for BMM.

  @retval EFI_SUCCSS If the Text Mode of Console is updated.
  @return Other value if the Text Mode of Console is not updated.

**/
EFI_STATUS
Var_UpdateConMode (
  IN BMM_CALLBACK_DATA  *CallbackData
  );

//
// Following are page create and refresh functions
//

/**
 Create the global UpdateData structure.

**/
VOID
CreateUpdateData (
  VOID
  );

/**
  Refresh the global UpdateData structure.

**/
VOID
RefreshUpdateData (
  VOID
  );

/**
  Clean up the dynamic opcode at label and form specified by
  both LabelId.

  @param LabelId         It is both the Form ID and Label ID for
                         opcode deletion.
  @param CallbackData    The BMM context data.

**/
VOID
CleanUpPage (
  IN UINT16             LabelId,
  IN BMM_CALLBACK_DATA  *CallbackData
  );

/**
  Create a lit of boot option from global BootOptionMenu. It
  allow user to delete the boot option.

  @param CallbackData    The BMM context data.

**/
VOID
UpdateBootDelPage (
  IN BMM_CALLBACK_DATA  *CallbackData
  );

/**
  Create a lit of driver option from global DriverMenu.

  @param CallbackData    The BMM context data.
**/
VOID
UpdateDrvAddHandlePage (
  IN BMM_CALLBACK_DATA  *CallbackData
  );

/**
  Create a lit of driver option from global DriverOptionMenu. It
  allow user to delete the driver option.

  @param CallbackData    The BMM context data.
**/
VOID
UpdateDrvDelPage (
  IN BMM_CALLBACK_DATA  *CallbackData
  );

/**
  Prepare the page to allow user to add description for a Driver Option.

  @param CallbackData    The BMM context data.
**/
VOID
UpdateDriverAddHandleDescPage (
  IN BMM_CALLBACK_DATA  *CallbackData
  );

/**
  Dispatch the correct update page function to call based on the UpdatePageId.

  @param UpdatePageId    The form ID.
  @param CallbackData    The BMM context data.
**/
VOID
UpdatePageBody (
  IN UINT16             UpdatePageId,
  IN BMM_CALLBACK_DATA  *CallbackData
  );

/**
  Create the dynamic page which allows user to set the property such as Baud Rate, Data Bits,
  Parity, Stop Bits, Terminal Type.

  @param CallbackData    The BMM context data.
**/
VOID
UpdateTerminalPage (
  IN BMM_CALLBACK_DATA  *CallbackData
  );

/**
  Refresh the text mode page

  @param CallbackData    The BMM context data.
**/
VOID
UpdateConModePage (
  IN BMM_CALLBACK_DATA  *CallbackData
  );

/**
  Create a list of Goto Opcode for all terminal devices logged
  by TerminaMenu. This list will be inserted to form FORM_CON_COM_SETUP_ID.

  @param CallbackData    The BMM context data.
**/
VOID
UpdateConCOMPage (
  IN BMM_CALLBACK_DATA  *CallbackData
  );

/**
 Update add boot/driver option page.

  @param CallbackData    The BMM context data.
  @param FormId             The form ID to be updated.
  @param DevicePath       Device path.

**/
VOID
UpdateOptionPage (
  IN   BMM_CALLBACK_DATA         *CallbackData,
  IN   EFI_FORM_ID               FormId,
  IN   EFI_DEVICE_PATH_PROTOCOL  *DevicePath
  );

/**
  Function deletes the variable specified by VarName and VarGuid.


  @param VarName            A Null-terminated Unicode string that is
                            the name of the vendor's variable.

  @param VarGuid            A unique identifier for the vendor.

  @retval  EFI_SUCCESS           The variable was found and removed
  @retval  EFI_UNSUPPORTED       The variable store was inaccessible
  @retval  EFI_OUT_OF_RESOURCES  The temporary buffer was not available
  @retval  EFI_NOT_FOUND         The variable was not found

**/
EFI_STATUS
EfiLibDeleteVariable (
  IN CHAR16    *VarName,
  IN EFI_GUID  *VarGuid
  );

/**
  Function is used to determine the number of device path instances
  that exist in a device path.


  @param DevicePath      A pointer to a device path data structure.

  @return This function counts and returns the number of device path instances
          in DevicePath.

**/
UINTN
EfiDevicePathInstanceCount (
  IN EFI_DEVICE_PATH_PROTOCOL  *DevicePath
  );

/**
  Get a string from the Data Hub record based on
  a device path.

  @param DevPath         The device Path.

  @return A string located from the Data Hub records based on
          the device path.
  @retval NULL  If failed to get the String from Data Hub.

**/
UINT16 *
EfiLibStrFromDatahub (
  IN EFI_DEVICE_PATH_PROTOCOL  *DevPath
  );

/**
  Get the index number (#### in Boot####) for the boot option pointed to a BBS legacy device type
  specified by DeviceType.

  @param DeviceType      The legacy device type. It can be floppy, network, harddisk, cdrom,
                         etc.
  @param OptionIndex     Returns the index number (#### in Boot####).
  @param OptionSize      Return the size of the Boot### variable.

**/
VOID *
GetLegacyBootOptionVar (
  IN  UINTN  DeviceType,
  OUT UINTN  *OptionIndex,
  OUT UINTN  *OptionSize
  );

/**
  Discard all changes done to the BMM pages such as Boot Order change,
  Driver order change.

  @param Private         The BMM context data.
  @param CurrentFakeNVMap The current Fack NV Map.

**/
VOID
DiscardChangeHandler (
  IN  BMM_CALLBACK_DATA  *Private,
  IN  BMM_FAKE_NV_DATA   *CurrentFakeNVMap
  );

/**
  This function is to clean some useless data before submit changes.

  @param Private            The BMM context data.

**/
VOID
CleanUselessBeforeSubmit (
  IN  BMM_CALLBACK_DATA  *Private
  );

/**
  Dispatch the display to the next page based on NewPageId.

  @param Private         The BMM context data.
  @param NewPageId       The original page ID.

**/
VOID
UpdatePageId (
  BMM_CALLBACK_DATA  *Private,
  UINT16             NewPageId
  );

/**
  Remove the installed BootMaint and FileExplorer HiiPackages.

**/
VOID
FreeBMPackage (
  VOID
  );

/**
  Install BootMaint and FileExplorer HiiPackages.

**/
VOID
InitBootMaintenance (
  VOID
  );

/**

  Initialize console input device check box to ConsoleInCheck[MAX_MENU_NUMBER]
  in BMM_FAKE_NV_DATA structure.

  @param CallbackData    The BMM context data.

**/
VOID
GetConsoleInCheck (
  IN  BMM_CALLBACK_DATA  *CallbackData
  );

/**

  Initialize console output device check box to ConsoleOutCheck[MAX_MENU_NUMBER]
  in BMM_FAKE_NV_DATA structure.

  @param CallbackData    The BMM context data.

**/
VOID
GetConsoleOutCheck (
  IN  BMM_CALLBACK_DATA  *CallbackData
  );

/**

  Initialize standard error output device check box to ConsoleErrCheck[MAX_MENU_NUMBER]
  in BMM_FAKE_NV_DATA structure.

  @param CallbackData    The BMM context data.

**/
VOID
GetConsoleErrCheck (
  IN  BMM_CALLBACK_DATA  *CallbackData
  );

/**

  Initialize terminal attributes (baudrate, data rate, stop bits, parity and terminal type)
  to BMM_FAKE_NV_DATA structure.

  @param CallbackData    The BMM context data.

**/
VOID
GetTerminalAttribute (
  IN  BMM_CALLBACK_DATA  *CallbackData
  );

/**
  This function will change video resolution and text mode
  according to defined setup mode or defined boot mode

  @param  IsSetupMode   Indicate mode is changed to setup mode or boot mode.

  @retval  EFI_SUCCESS  Mode is changed successfully.
  @retval  Others             Mode failed to be changed.

**/
EFI_STATUS
BmmSetConsoleMode (
  BOOLEAN  IsSetupMode
  );

/**
  This function converts an input device structure to a Unicode string.

  @param DevPath       A pointer to the device path structure.

  @return              A new allocated Unicode string that represents the device path.

**/
CHAR16 *
UiDevicePathToStr (
  IN EFI_DEVICE_PATH_PROTOCOL  *DevPath
  );

/**
  Extract filename from device path. The returned buffer is allocated using AllocateCopyPool.
  The caller is responsible for freeing the allocated buffer using FreePool().

  @param DevicePath      Device path.

  @return                A new allocated string that represents the file name.

**/
CHAR16 *
ExtractFileNameFromDevicePath (
  IN   EFI_DEVICE_PATH_PROTOCOL  *DevicePath
  );

/**
  This function allows a caller to extract the current configuration for one
  or more named elements from the target driver.

  @param This            Points to the EFI_HII_CONFIG_ACCESS_PROTOCOL.
  @param Request         A null-terminated Unicode string in <ConfigRequest> format.
  @param Progress        On return, points to a character in the Request string.
                         Points to the string's null terminator if request was successful.
                         Points to the most recent '&' before the first failing name/value
                         pair (or the beginning of the string if the failure is in the
                         first name/value pair) if the request was not successful.
  @param Results         A null-terminated Unicode string in <ConfigAltResp> format which
                         has all values filled in for the names in the Request string.
                         String to be allocated by the called function.

  @retval  EFI_SUCCESS            The Results is filled with the requested values.
  @retval  EFI_OUT_OF_RESOURCES   Not enough memory to store the results.
  @retval  EFI_INVALID_PARAMETER  Request is NULL, illegal syntax, or unknown name.
  @retval  EFI_NOT_FOUND          Routing data doesn't match any storage in this driver.

**/
EFI_STATUS
EFIAPI
BootMaintExtractConfig (
  IN  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL  *This,
  IN  CONST EFI_STRING                      Request,
  OUT EFI_STRING                            *Progress,
  OUT EFI_STRING                            *Results
  );

/**
  This function applies changes in a driver's configuration.
  Input is a Configuration, which has the routing data for this
  driver followed by name / value configuration pairs. The driver
  must apply those pairs to its configurable storage. If the
  driver's configuration is stored in a linear block of data
  and the driver's name / value pairs are in <BlockConfig>
  format, it may use the ConfigToBlock helper function (above) to
  simplify the job. Currently not implemented.

  @param[in]  This                Points to the EFI_HII_CONFIG_ACCESS_PROTOCOL.
  @param[in]  Configuration       A null-terminated Unicode string in
                                  <ConfigString> format.
  @param[out] Progress            A pointer to a string filled in with the
                                  offset of the most recent '&' before the
                                  first failing name / value pair (or the
                                  beginn ing of the string if the failure
                                  is in the first name / value pair) or
                                  the terminating NULL if all was
                                  successful.

  @retval EFI_SUCCESS             The results have been distributed or are
                                  awaiting distribution.
  @retval EFI_OUT_OF_RESOURCES    Not enough memory to store the
                                  parts of the results that must be
                                  stored awaiting possible future
                                  protocols.
  @retval EFI_INVALID_PARAMETERS  Passing in a NULL for the
                                  Results parameter would result
                                  in this type of error.
  @retval EFI_NOT_FOUND           Target for the specified routing data
                                  was not found.
**/
EFI_STATUS
EFIAPI
BootMaintRouteConfig (
  IN CONST EFI_HII_CONFIG_ACCESS_PROTOCOL  *This,
  IN CONST EFI_STRING                      Configuration,
  OUT EFI_STRING                           *Progress
  );

/**
  This function processes the results of changes in configuration.


  @param This               Points to the EFI_HII_CONFIG_ACCESS_PROTOCOL.
  @param Action             Specifies the type of action taken by the browser.
  @param QuestionId         A unique value which is sent to the original exporting driver
                            so that it can identify the type of data to expect.
  @param Type               The type of value for the question.
  @param Value              A pointer to the data being sent to the original exporting driver.
  @param ActionRequest      On return, points to the action requested by the callback function.

  @retval EFI_SUCCESS           The callback successfully handled the action.
  @retval EFI_OUT_OF_RESOURCES  Not enough storage is available to hold the variable and its data.
  @retval EFI_DEVICE_ERROR      The variable could not be saved.
  @retval EFI_UNSUPPORTED       The specified Action is not supported by the callback.
  @retval EFI_INVALID_PARAMETER The parameter of Value or ActionRequest is invalid.
**/
EFI_STATUS
EFIAPI
BootMaintCallback (
  IN  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL  *This,
  IN        EFI_BROWSER_ACTION              Action,
  IN        EFI_QUESTION_ID                 QuestionId,
  IN        UINT8                           Type,
  IN        EFI_IFR_TYPE_VALUE              *Value,
  OUT       EFI_BROWSER_ACTION_REQUEST      *ActionRequest
  );

/**
  Create boot option base on the input file path info.

  @param FilePath    Point to the file path.

  @retval TRUE   Exit caller function.
  @retval FALSE  Not exit caller function.

**/
BOOLEAN
EFIAPI
CreateBootOptionFromFile (
  IN EFI_DEVICE_PATH_PROTOCOL  *FilePath
  );

/**
  Create driver option base on the input file path info.

  @param FilePath    Point to the file path.

  @retval TRUE   Exit caller function.
  @retval FALSE  Not exit caller function.
**/
BOOLEAN
EFIAPI
CreateDriverOptionFromFile (
  IN EFI_DEVICE_PATH_PROTOCOL  *FilePath
  );

/**
  Boot the file specified by the input file path info.

  @param FilePath    Point to the file path.

  @retval TRUE   Exit caller function.
  @retval FALSE  Not exit caller function.

**/
BOOLEAN
EFIAPI
BootFromFile (
  IN EFI_DEVICE_PATH_PROTOCOL  *FilePath
  );

//
// Global variable in this program (defined in data.c)
//
extern BM_MENU_OPTION            BootOptionMenu;
extern BM_MENU_OPTION            DriverOptionMenu;
extern BM_MENU_OPTION            ConsoleInpMenu;
extern BM_MENU_OPTION            ConsoleOutMenu;
extern BM_MENU_OPTION            ConsoleErrMenu;
extern BM_MENU_OPTION            DriverMenu;
extern BM_MENU_OPTION            TerminalMenu;
extern UINT16                    TerminalType[9];
extern COM_ATTR                  BaudRateList[19];
extern COM_ATTR                  DataBitsList[4];
extern COM_ATTR                  ParityList[5];
extern COM_ATTR                  StopBitsList[3];
extern EFI_GUID                  TerminalTypeGuid[9];
extern EFI_DEVICE_PATH_PROTOCOL  EndDevicePath[];
extern UINT16                    mFlowControlType[2];
extern UINT32                    mFlowControlValue[2];

//
// Shared IFR form update data
//
extern VOID                *mStartOpCodeHandle;
extern VOID                *mEndOpCodeHandle;
extern EFI_IFR_GUID_LABEL  *mStartLabel;
extern EFI_IFR_GUID_LABEL  *mEndLabel;
extern BMM_CALLBACK_DATA   gBootMaintenancePrivate;
extern BMM_CALLBACK_DATA   *mBmmCallbackInfo;

#endif
