/*++

Copyright (c) 2006, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  BootMaint.h

Abstract:

Revision History

--*/

#ifndef _BOOT_MAINT_H
#define _BOOT_MAINT_H

#include "Generic/Bds.h"
#include "Generic/BootMaint/BBSsupport.h"

//
// Constants which are variable names used to access variables
//
#define VarLegacyDevOrder L"LegacyDevOrder"

//
// Guid of a NV Variable which store the information about the
// FD/HD/CD/NET/BEV order
//
#define EFI_LEGACY_DEV_ORDER_VARIABLE_GUID \
  { \
    0xa56074db, 0x65fe, 0x45f7, {0xbd, 0x21, 0x2d, 0x2b, 0xdd, 0x8e, 0x96, 0x52 } \
  }

//
// String Contant
//
#define StrFloppy       L"Floppy Drive #%02x"
#define StrHardDisk     L"HardDisk Drive #%02x"
#define StrCDROM        L"ATAPI CDROM Drive #%02x"
#define StrNET          L"NET Drive #%02x"
#define StrBEV          L"BEV Drive #%02x"
#define StrFloppyHelp   L"Select Floppy Drive #%02x"
#define StrHardDiskHelp L"Select HardDisk Drive #%02x"
#define StrCDROMHelp    L"Select ATAPI CDROM Drive #%02x"
#define StrNETHelp      L"NET Drive #%02x"
#define StrBEVHelp      L"BEV Drive #%02x"

//
// Constant will be used in display and file system navigation
//
#define UPDATE_DATA_SIZE        0x100000
#define MAX_BBS_OFFSET          0xE000
#define NET_OPTION_OFFSET       0xD800
#define BEV_OPTION_OFFSET       0xD000
#define FD_OPTION_OFFSET        0xC000
#define HD_OPTION_OFFSET        0xB000
#define CD_OPTION_OFFSET        0xA000
#define FILE_OPTION_OFFSET      0x8000
#define FILE_OPTION_MASK        0x7FFF
#define HANDLE_OPTION_OFFSET    0x7000
#define CONSOLE_OPTION_OFFSET   0x0A00
#define TERMINAL_OPTION_OFFSET  0x0700
#define NORMAL_GOTO_OFFSET      0x0100
#define MAX_STRING_TOKEN_COUNT  0x00FF
//
// Variable created with this flag will be "Efi:...."
//
#define VAR_FLAG  EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_NON_VOLATILE

//
// Define Maxmim characters that will be accepted
//
#define MAX_CHAR      480
#define MAX_CHAR_SIZE (MAX_CHAR * 2)


//
// Below are the form ids for display, form id is used as callback key value,
// some key value definitions are also defined here. By defining this enum type,
// We can easy know where we are. The int to UINT16 convertion should be ok because
// there is a MAXIMUM_FORM_ID which in within the range of UINT16.
//
typedef enum {
  IplRelative,
  BcvRelative
} BBS_TYPE;

typedef enum {
  FORM_RESERVED_ID                    = 0,
  FORM_MAIN_ID,                         // 0x0001
  FORM_BOOT_ADD_ID,                     // 0x0002
  FORM_BOOT_DEL_ID,                     // 0x0003
  FORM_BOOT_CHG_ID,                     // 0x0004
  FORM_DRV_ADD_ID,                      // 0x0005
  FORM_DRV_DEL_ID,                      // 0x0006
  FORM_DRV_CHG_ID,                      // 0x0007
  FORM_CON_MAIN_ID,                     // 0x0008
  FORM_CON_IN_ID,                       // 0x0009
  FORM_CON_OUT_ID,                      // 0x000A
  FORM_CON_ERR_ID,                      // 0x000B
  FORM_FILE_SEEK_ID,                    // 0x000C
  FORM_FILE_NEW_SEEK_ID,                // 0x000D
  FORM_DRV_ADD_FILE_ID,                 // 0x000E
  FORM_DRV_ADD_HANDLE_ID,               // 0x000F
  FORM_DRV_ADD_HANDLE_DESC_ID,          // 0x0010
  FORM_BOOT_NEXT_ID,                    // 0x0011
  FORM_TIME_OUT_ID,                     // 0x0012
  FORM_RESET,                           // 0x0013
  FORM_BOOT_SETUP_ID,                   // 0x0014
  FORM_DRIVER_SETUP_ID,                 // 0x0015
  FORM_BOOT_LEGACY_DEVICE_ID,           // 0x0016
  FORM_CON_COM_ID,                      // 0x0017
  FORM_CON_COM_SETUP_ID,                // 0x0018
  FORM_SET_FD_ORDER_ID,                 // 0x0019
  FORM_SET_HD_ORDER_ID,                 // 0x001A
  FORM_SET_CD_ORDER_ID,                 // 0x001B
  FORM_SET_NET_ORDER_ID,                // 0x001C
  FORM_SET_BEV_ORDER_ID,                // 0x001D
  FORM_FILE_EXPLORER_ID,                // 0x001E
  FORM_BOOT_ADD_DESCRIPTION_ID,         // 0x001F
  FORM_DRIVER_ADD_FILE_DESCRIPTION_ID,  // 0x0020
} FORM_ID;

#define MAXIMUM_FORM_ID                 0x007F

#define KEY_VALUE_COM_SET_BAUD_RATE     0x0080
#define KEY_VALUE_COM_SET_DATA_BITS     0x0081
#define KEY_VALUE_COM_SET_STOP_BITS     0x0082
#define KEY_VALUE_COM_SET_PARITY        0x0083
#define KEY_VALUE_COM_SET_TERMI_TYPE    0x0084
#define KEY_VALUE_MAIN_BOOT_NEXT        0x0085
#define KEY_VALUE_BOOT_ADD_DESC_DATA    0x0086
#define KEY_VALUE_BOOT_ADD_OPT_DATA     0x0087
#define KEY_VALUE_DRIVER_ADD_DESC_DATA  0x0088
#define KEY_VALUE_DRIVER_ADD_OPT_DATA   0x0089
#define KEY_VALUE_SAVE_AND_EXIT         0x0090
#define KEY_VALUE_NO_SAVE_AND_EXIT      0x0091
#define KEY_VALUE_BOOT_FROM_FILE        0x0092

#define MAXIMUM_NORMAL_KEY_VALUE        NORMAL_GOTO_OFFSET
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
#define BMM_CALLBACK_DATA_SIGNATURE     EFI_SIGNATURE_32 ('C', 'b', 'c', 'k')
#define BMM_CALLBACK_DATA_FROM_THIS(a)  CR (a, BMM_CALLBACK_DATA, BmmDriverCallback, BMM_CALLBACK_DATA_SIGNATURE)

#define FE_CALLBACK_DATA_FROM_THIS(a)   CR (a, BMM_CALLBACK_DATA, FeDriverCallback, BMM_CALLBACK_DATA_SIGNATURE)

//
// Enumeration type definition
//
typedef enum {
  PC_ANSI                             = 0,
  VT_100,
  VT_100_PLUS,
  VT_UTF8
} TYPE_OF_TERMINAL;

typedef enum {
  COM1                                = 0,
  COM2,
  UNKNOW_COM
} TYPE_OF_COM;

typedef enum {
  CONIN                               = 0,
  CONOUT,
  CONERR,
  UNKNOWN_CON
} TYPE_OF_CON;

typedef enum {
  BAUDRATE                            = 0,
  DATABITS,
  PARITY,
  STOPBITS,
  UNKNOW_ATTR
} TYPE_OF_ATTRIBUTE;

typedef enum {
  MANNER_GOTO                         = 0,
  MANNER_CHECK,
  MANNER_ONEOF,
  MANNER_USER_DEFINE
} TYPE_OF_UPATE_MANNER;

typedef enum {
  INACTIVE_STATE                      = 0,
  BOOT_FROM_FILE_STATE,
  ADD_BOOT_OPTION_STATE,
  ADD_DRIVER_OPTION_STATE,
  UNKNOWN_STATE
} FILE_EXPLORER_STATE;

typedef enum {
  FILE_SYSTEM,
  DIRECTORY,
  UNKNOWN_CONTEXT
} FILE_EXPLORER_DISPLAY_CONTEXT;

//
// All of the signatures that will be used in list structure
//
#define BM_MENU_OPTION_SIGNATURE      'menu'
#define BM_LOAD_OPTION_SIGNATURE      'load'
#define BM_CONSOLE_OPTION_SIGNATURE   'cnsl'
#define BM_FILE_OPTION_SIGNATURE      'file'
#define BM_HANDLE_OPTION_SIGNATURE    'hndl'
#define BM_TERMINAL_OPTION_SIGNATURE  'trml'
#define BM_MENU_ENTRY_SIGNATURE       'entr'

#define BM_LOAD_CONTEXT_SELECT        0x0
#define BM_CONSOLE_CONTEXT_SELECT     0x1
#define BM_FILE_CONTEXT_SELECT        0x2
#define BM_HANDLE_CONTEXT_SELECT      0x3
#define BM_TERMINAL_CONTEXT_SELECT    0x5

#define BM_CONSOLE_IN_CONTEXT_SELECT  0x6
#define BM_CONSOLE_OUT_CONTEXT_SELECT 0x7
#define BM_CONSOLE_ERR_CONTEXT_SELECT 0x8
#define BM_LEGACY_DEV_CONTEXT_SELECT  0x9

//
// Question Id that will be used to create question
// all these values are computed from the structure
// defined below
//
#define QUESTION_ID(Field)              ((UINTN) &(((BMM_FAKE_NV_DATA *) 0)->Field))

#define BOOT_TIME_OUT_QUESTION_ID       QUESTION_ID (BootTimeOut)
#define BOOT_NEXT_QUESTION_ID           QUESTION_ID (BootNext)
#define COM1_BAUD_RATE_QUESTION_ID      QUESTION_ID (COM1BaudRate)
#define COM1_DATA_RATE_QUESTION_ID      QUESTION_ID (COM1DataRate)
#define COM1_STOP_BITS_QUESTION_ID      QUESTION_ID (COM1StopBits)
#define COM1_PARITY_QUESTION_ID         QUESTION_ID (COM1Parity)
#define COM1_TERMINAL_QUESTION_ID       QUESTION_ID (COM2TerminalType)
#define COM2_BAUD_RATE_QUESTION_ID      QUESTION_ID (COM2BaudRate)
#define COM2_DATA_RATE_QUESTION_ID      QUESTION_ID (COM2DataRate)
#define COM2_STOP_BITS_QUESTION_ID      QUESTION_ID (COM2StopBits)
#define COM2_PARITY_QUESTION_ID         QUESTION_ID (COM2Parity)
#define COM2_TERMINAL_QUESTION_ID       QUESTION_ID (COM2TerminalType)
#define DRV_ADD_HANDLE_DESC_QUESTION_ID QUESTION_ID (DriverAddHandleDesc)
#define DRV_ADD_ACTIVE_QUESTION_ID      QUESTION_ID (DriverAddActive)
#define DRV_ADD_RECON_QUESTION_ID       QUESTION_ID (DriverAddForceReconnect)
#define CON_IN_COM1_QUESTION_ID         QUESTION_ID (ConsoleInputCOM1)
#define CON_IN_COM2_QUESTION_ID         QUESTION_ID (ConsoleInputCOM2)
#define CON_OUT_COM1_QUESTION_ID        QUESTION_ID (ConsoleOutputCOM1)
#define CON_OUT_COM2_QUESTION_ID        QUESTION_ID (ConsoleOutputCOM2)
#define CON_ERR_COM1_QUESTION_ID        QUESTION_ID (ConsoleErrorCOM1)
#define CON_ERR_COM2_QUESTION_ID        QUESTION_ID (ConsoleErrorCOM2)
#define CON_DEVICE_QUESTION_ID          QUESTION_ID (ConsoleCheck)
#define OPTION_ORDER_QUESTION_ID        QUESTION_ID (OptionOrder)
#define DRIVER_OPTION_ORDER_QUESTION_ID QUESTION_ID (DriverOptionToBeDeleted)
#define BOOT_OPTION_DEL_QUESTION_ID     QUESTION_ID (BootOptionDel)
#define DRIVER_OPTION_DEL_QUESTION_ID   QUESTION_ID (DriverOptionDel)
#define DRIVER_ADD_OPTION_QUESTION_ID   QUESTION_ID (DriverAddHandleOptionalData)
#define COM_BAUD_RATE_QUESTION_ID       QUESTION_ID (COMBaudRate)
#define COM_DATA_RATE_QUESTION_ID       QUESTION_ID (COMDataRate)
#define COM_STOP_BITS_QUESTION_ID       QUESTION_ID (COMStopBits)
#define COM_PARITY_QUESTION_ID          QUESTION_ID (COMParity)
#define COM_TERMINAL_QUESTION_ID        QUESTION_ID (COMTerminalType)
#define LEGACY_FD_QUESTION_ID           QUESTION_ID (LegacyFD)
#define LEGACY_HD_QUESTION_ID           QUESTION_ID (LegacyHD)
#define LEGACY_CD_QUESTION_ID           QUESTION_ID (LegacyCD)
#define LEGACY_NET_QUESTION_ID          QUESTION_ID (LegacyNET)
#define LEGACY_BEV_QUESTION_ID          QUESTION_ID (LegacyBEV)

#define STRING_DEPOSITORY_NUMBER        8

//
// #pragma pack(1)
//
// Serial Ports attributes, first one is the value for
// return from callback function, stringtoken is used to
// display the value properly
//
typedef struct {
  UINTN   Value;
  UINT16  StringToken;
} COM_ATTR;

//
// This is the structure that will be used to store the
// question's current value. Use it at initialize time to
// set default value for each question. When using at run
// time, this map is returned by the callback function,
// so dynamically changing the question's value will be
// possible through this mechanism
//
typedef struct {
  //
  // Three questions displayed at the main page
  // for Timeout, BootNext Variables respectively
  //
  UINT16  BootTimeOut;
  UINT16  BootNext;

  //
  // This is the COM1 Attributes value storage
  //
  UINT8   COM1BaudRate;
  UINT8   COM1DataRate;
  UINT8   COM1StopBits;
  UINT8   COM1Parity;
  UINT8   COM1TerminalType;

  //
  // This is the COM2 Attributes value storage
  //
  UINT8   COM2BaudRate;
  UINT8   COM2DataRate;
  UINT8   COM2StopBits;
  UINT8   COM2Parity;
  UINT8   COM2TerminalType;

  //
  // Driver Option Add Handle page storage
  //
  UINT16  DriverAddHandleDesc[100];
  UINT16  DriverAddHandleOptionalData[100];
  UINT8   DriverAddActive;
  UINT8   DriverAddForceReconnect;

  //
  // Console Input/Output/Errorout using COM port check storage
  //
  UINT8   ConsoleInputCOM1;
  UINT8   ConsoleInputCOM2;
  UINT8   ConsoleOutputCOM1;
  UINT8   ConsoleOutputCOM2;
  UINT8   ConsoleErrorCOM1;
  UINT8   ConsoleErrorCOM2;

  //
  // At most 100 input/output/errorout device for console storage
  //
  UINT8   ConsoleCheck[100];

  //
  // Boot or Driver Option Order storage
  //
  UINT8   OptionOrder[100];
  UINT8   DriverOptionToBeDeleted[100];

  //
  // Boot Option Delete storage
  //
  UINT8   BootOptionDel[100];
  UINT8   DriverOptionDel[100];

  //
  // This is the Terminal Attributes value storage
  //
  UINT8   COMBaudRate;
  UINT8   COMDataRate;
  UINT8   COMStopBits;
  UINT8   COMParity;
  UINT8   COMTerminalType;

  //
  // Legacy Device Order Selection Storage
  //
  UINT8   LegacyFD[100];
  UINT8   LegacyHD[100];
  UINT8   LegacyCD[100];
  UINT8   LegacyNET[100];
  UINT8   LegacyBEV[100];

  //
  // We use DisableMap array to record the enable/disable state of each boot device
  // It should be taken as a bit array, from left to right there are totally 256 bits
  // the most left one stands for BBS table item 0, and the most right one stands for item 256
  // If the bit is 1, it means the boot device has been disabled.
  //
  UINT8   DisableMap[32];

  //
  //  UINT16                    PadArea[10];
  //
} BMM_FAKE_NV_DATA;

typedef struct {
  UINT16  DescriptionData[75];
  UINT16                    OptionalData[127];
  UINT8   Active;
  UINT8   ForceReconnect;
} FILE_EXPLORER_NV_DATA;

typedef struct {
  BBS_TYPE  BbsType;
  //
  // Length = sizeof (UINT16) + SIZEOF (Data)
  //
  UINT16    Length;
  UINT16    *Data;
} BM_LEGACY_DEV_ORDER_CONTEXT;

typedef struct {
  UINT64                    BaudRate;
  UINT8                     DataBits;
  UINT8                     Parity;
  UINT8                     StopBits;

  UINT8                     BaudRateIndex;
  UINT8                     DataBitsIndex;
  UINT8                     ParityIndex;
  UINT8                     StopBitsIndex;

  UINT8                     IsConIn;
  UINT8                     IsConOut;
  UINT8                     IsStdErr;
  UINT8                     TerminalType;

  EFI_DEVICE_PATH_PROTOCOL  *DevicePath;
} BM_TERMINAL_CONTEXT;

typedef struct {
  BOOLEAN                   IsBootNext;
  BOOLEAN                   LoadOptionModified;
  BOOLEAN                   Deleted;

  BOOLEAN                   IsLegacy;
  BOOLEAN                   IsActive;
  BOOLEAN                   ForceReconnect;
  UINTN                     OptionalDataSize;

  UINTN                     LoadOptionSize;
  UINT8                     *LoadOption;

  UINT32                    Attributes;
  UINT16                    FilePathListLength;
  UINT16                    *Description;
  EFI_DEVICE_PATH_PROTOCOL  *FilePathList;
  UINT8                     *OptionalData;

  UINT16                    BbsIndex;
} BM_LOAD_CONTEXT;

typedef struct {
  BBS_TABLE *BbsTable;
  UINTN     Index;
  UINTN     BbsCount;
  UINT16    *Description;
} BM_LEGACY_DEVICE_CONTEXT;

typedef struct {

  BOOLEAN                   IsActive;

  BOOLEAN                   IsTerminal;

  EFI_DEVICE_PATH_PROTOCOL  *DevicePath;
} BM_CONSOLE_CONTEXT;

typedef struct {
  EFI_HANDLE                        Handle;
  EFI_DEVICE_PATH_PROTOCOL          *DevicePath;
  EFI_FILE_HANDLE                   FHandle;
  UINT16                            *FileName;
  EFI_FILE_SYSTEM_VOLUME_LABEL_INFO *Info;

  BOOLEAN                           IsRoot;
  BOOLEAN                           IsDir;
  BOOLEAN                           IsRemovableMedia;
  BOOLEAN                           IsLoadFile;
  BOOLEAN                           IsBootLegacy;
} BM_FILE_CONTEXT;

typedef struct {
  EFI_HANDLE                Handle;
  EFI_DEVICE_PATH_PROTOCOL  *DevicePath;
} BM_HANDLE_CONTEXT;

typedef struct {
  UINTN           Signature;
  LIST_ENTRY      Head;
  UINTN           MenuNumber;
} BM_MENU_OPTION;

typedef struct {
  UINTN           Signature;
  LIST_ENTRY      Link;
  UINTN           OptionNumber;
  UINT16          *DisplayString;
  UINT16          *HelpString;
  STRING_REF      DisplayStringToken;
  STRING_REF      HelpStringToken;
  UINTN           ContextSelection;
  VOID            *VariableContext;
} BM_MENU_ENTRY;

typedef struct {
  //
  // Shared callback data.
  //
  UINTN                         Signature;
  EFI_HII_PROTOCOL              *Hii;
  BM_MENU_ENTRY                 *MenuEntry;
  BM_HANDLE_CONTEXT             *HandleContext;
  BM_FILE_CONTEXT               *FileContext;
  BM_LOAD_CONTEXT               *LoadContext;
  BM_TERMINAL_CONTEXT           *TerminalContext;
  UINTN                         CurrentTerminal;
  BBS_TYPE                      BbsType;

  //
  // BMM main formset callback data.
  //
  EFI_HII_HANDLE                BmmHiiHandle;
  EFI_HANDLE                    BmmCallbackHandle;
  EFI_FORM_CALLBACK_PROTOCOL    BmmDriverCallback;
  FORM_ID                       BmmCurrentPageId;
  FORM_ID                       BmmPreviousPageId;
  BOOLEAN                       BmmAskSaveOrNot;
  BMM_FAKE_NV_DATA              *BmmFakeNvData;
  BMM_FAKE_NV_DATA              BmmOldFakeNVData;

  //
  // File explorer formset callback data.
  //
  EFI_HII_HANDLE                FeHiiHandle;
  EFI_HANDLE                    FeCallbackHandle;
  EFI_FORM_CALLBACK_PROTOCOL    FeDriverCallback;
  FILE_EXPLORER_STATE           FeCurrentState;
  FILE_EXPLORER_DISPLAY_CONTEXT FeDisplayContext;
} BMM_CALLBACK_DATA;

typedef struct _STRING_LIST_NODE {
  STRING_REF                StringToken;
  struct _STRING_LIST_NODE  *Next;
} STRING_LIST_NODE;

typedef struct _STRING_DEPOSITORY {
  UINTN             TotalNodeNumber;
  STRING_LIST_NODE  *CurrentNode;
  STRING_LIST_NODE  *ListHead;
} STRING_DEPOSITORY;

//
// #pragma pack()
//
// For initializing File System menu
//
EFI_STATUS
BOpt_FindFileSystem (
  IN BMM_CALLBACK_DATA          *CallbackData
  )
;

//
// For cleaning up File System menu
//
VOID
BOpt_FreeFileSystem (
  VOID
  )
;

//
// For initializing File Navigation menu
//
EFI_STATUS
BOpt_FindFiles (
  IN BMM_CALLBACK_DATA          *CallbackData,
  IN BM_MENU_ENTRY              *MenuEntry
  )
;

//
// For cleaning up File Navigation menu
//
VOID
BOpt_FreeFiles (
  VOID
  )
;

//
// For Initializing handle navigation menu
//
EFI_STATUS
BOpt_FindDrivers (
  VOID
  )
;

//
// For Cleaning up handle navigation menu
//
VOID
BOpt_FreeDrivers();

//
// For initializing Boot Option menu
//
EFI_STATUS
BOpt_GetBootOptions (
  IN  BMM_CALLBACK_DATA         *CallbackData
  );

//
// For Initializing Driver option menu
//
EFI_STATUS
BOpt_GetDriverOptions (
  IN  BMM_CALLBACK_DATA         *CallbackData
  );

//
// For Cleaning up boot option menu
//
VOID
BOpt_FreeBootOptions ();

//
// For cleaning up driver option menu
//
VOID
BOpt_FreeDriverOptions();

//
// For Initializing HD/FD/CD/NET/BEV option menu
//
EFI_STATUS
BOpt_GetLegacyOptions();

//
// For cleaning up driver option menu
//
VOID
BOpt_FreeLegacyOptions();

//
// this function is used to take place of all other free menu actions
//
VOID
BOpt_FreeMenu (
  BM_MENU_OPTION        *FreeMenu
  );


//
// Following are the helper functions used
//
CHAR16                            *
BOpt_AppendFileName (
  IN  CHAR16  *Str1,
  IN  CHAR16  *Str2
  );

BOOLEAN
BOpt_IsEfiImageName (
  IN UINT16  *FileName
  );

BOOLEAN
BOpt_IsEfiApp (
  IN EFI_FILE_HANDLE Dir,
  IN UINT16          *FileName
  );

//
// Get current unused boot option number
//
UINT16
BOpt_GetBootOptionNumber ();

//
// Get current unused driver option number
//
UINT16
BOpt_GetDriverOptionNumber ();

BM_MENU_ENTRY                     *
BOpt_CreateMenuEntry (
  UINTN           MenuType
  );

VOID
BOpt_DestroyMenuEntry (
  BM_MENU_ENTRY         *MenuEntry
  );

BM_MENU_ENTRY                     *
BOpt_GetMenuEntry (
  BM_MENU_OPTION      *MenuOption,
  UINTN               MenuNumber
  );

//
// a helper function used to free pool type memory
//
VOID
SafeFreePool (
  IN VOID *Buffer
  );

//
// Locate all serial io devices for console
//
EFI_STATUS
LocateSerialIo ();

//
// Initializing Console menu
//
EFI_STATUS
GetAllConsoles();

//
// Cleaning up console menu
//
EFI_STATUS
FreeAllConsoles();

VOID
ChangeVariableDevicePath (
  EFI_DEVICE_PATH_PROTOCOL  *DevicePath
); 

EFI_STATUS
ChangeTerminalDevicePath (
  EFI_DEVICE_PATH_PROTOCOL  *DevicePath,
  BOOLEAN                   ChangeTerminal
);
//
// Variable operation by menu selection
//
EFI_STATUS
Var_UpdateBootOption (
  IN  BMM_CALLBACK_DATA                   *CallbackData,
  IN  FILE_EXPLORER_NV_DATA               *NvRamMap
  );

EFI_STATUS
Var_DelBootOption ();

EFI_STATUS
Var_ChangeBootOrder ();

EFI_STATUS
Var_UpdateDriverOption (
  IN  BMM_CALLBACK_DATA         *CallbackData,
  IN  EFI_HII_HANDLE            HiiHandle,
  IN  UINT16                    *DescriptionData,
  IN  UINT16                    *OptionalData,
  IN  UINT8                     ForceReconnect
  );

EFI_STATUS
Var_DelDriverOption ();

EFI_STATUS
Var_ChangeDriverOrder ();

EFI_STATUS
Var_UpdateConsoleInpOption ();

EFI_STATUS
Var_UpdateConsoleOutOption ();

EFI_STATUS
Var_UpdateErrorOutOption ();

VOID
Var_UpdateAllConsoleOption ();

EFI_STATUS
Var_UpdateBootNext (
  IN BMM_CALLBACK_DATA            *CallbackData
  );

EFI_STATUS
Var_UpdateBootOrder (
  IN BMM_CALLBACK_DATA            *CallbackData
  );

EFI_STATUS
Var_UpdateDriverOrder (
  IN BMM_CALLBACK_DATA            *CallbackData
  );

EFI_STATUS
Var_UpdateBBSOption (
  IN BMM_CALLBACK_DATA            *CallbackData
  );

//
// Following are page create and refresh functions
//
VOID
RefreshUpdateData (
  IN BOOLEAN                          FormSetUpdate,
  IN EFI_PHYSICAL_ADDRESS             FormCallbackHandle,
  IN BOOLEAN                          FormUpdate,
  IN STRING_REF                       FormTitle,
  IN UINT16                           DataCount
  );

VOID
CleanUpPage (
  IN EFI_FORM_LABEL                   LabelId,
  IN BMM_CALLBACK_DATA                *CallbackData
  );

EFI_STATUS
UpdatePage (
  IN BMM_CALLBACK_DATA                *CallbackData,
  IN BM_MENU_OPTION                   *UpdatingMenu,
  IN UINT16                           UpdatingPage,
  IN UINT16                           UpdatingManner,
  IN UINT16                           QuestionIdStart,
  IN UINT16                           GotoForm,
  IN UINT16                           GotoAlternateForm,
  IN STRING_REF                       DisplayTokenStart,
  IN STRING_REF                       HelpTokenStart,
  IN UINT16                           KeyValueStart
  );

VOID
UpdateBootAddPage (
  IN BMM_CALLBACK_DATA                *CallbackData
  );

VOID
UpdateBootDelPage (
  IN BMM_CALLBACK_DATA                *CallbackData
  );

VOID
UpdateDrvAddFilePage (
  IN BMM_CALLBACK_DATA                *CallbackData
  );

VOID
UpdateDrvAddHandlePage (
  IN BMM_CALLBACK_DATA                *CallbackData
  );

VOID
UpdateDrvDelPage (
  IN BMM_CALLBACK_DATA                *CallbackData
  );

VOID
UpdateDriverAddHandleDescPage (
  IN BMM_CALLBACK_DATA                *CallbackData
  );

VOID
UpdateBootTimeOut (
  IN BMM_CALLBACK_DATA                *CallbackData
  );

VOID
UpdateConInPage (
  IN BMM_CALLBACK_DATA                *CallbackData
  );

VOID
UpdateConOutPage (
  IN BMM_CALLBACK_DATA                *CallbackData
  );

VOID
UpdateStdErrPage (
  IN BMM_CALLBACK_DATA                *CallbackData
  );

VOID
UpdatePageBody (
  IN UINT16                           UpdatePageId,
  IN BMM_CALLBACK_DATA                *CallbackData
  );

VOID
UpdateCOM1Page (
  IN BMM_CALLBACK_DATA                *CallbackData
  );

VOID
UpdateCOM2Page (
  IN BMM_CALLBACK_DATA                *CallbackData
  );

VOID
UpdateBootOrderPage (
  IN BMM_CALLBACK_DATA                *CallbackData
  );

VOID
UpdateDriverOrderPage (
  IN BMM_CALLBACK_DATA                *CallbackData
  );

VOID
UpdateBootNextPage (
  IN BMM_CALLBACK_DATA                *CallbackData
  );

VOID
UpdateTimeOutPage (
  IN BMM_CALLBACK_DATA                *CallbackData
  );

VOID
UpdateTerminalPage (
  IN BMM_CALLBACK_DATA                *CallbackData
  );

VOID
UpdateConCOMPage (
  IN BMM_CALLBACK_DATA                *CallbackData
  );

VOID
UpdateSetLegacyDeviceOrderPage (
  IN UINT16                           UpdatePageId,
  IN BMM_CALLBACK_DATA                *CallbackData
);

EFI_STATUS
BootLegacy (
  IN  UINT16  BbsType,
  IN  UINT16  BbsFlag
);

BM_MENU_ENTRY                     *
GetCurrentTerminal (
  UINTN       TerminalNumber
);

EFI_FILE_HANDLE
EfiLibOpenRoot (
  IN EFI_HANDLE       DeviceHandle
  );

EFI_FILE_SYSTEM_VOLUME_LABEL_INFO *
EfiLibFileSystemVolumeLabelInfo (
  IN EFI_FILE_HANDLE      FHand
  );

EFI_FILE_INFO                     *
EfiLibFileInfo (
  IN EFI_FILE_HANDLE      FHand
  );

CHAR16                            *
DevicePathToStr (
  EFI_DEVICE_PATH_PROTOCOL     *DevPath
  );

EFI_STATUS
EfiLibLocateProtocol (
  IN  EFI_GUID        *ProtocolGuid,
  OUT VOID            **Interface
  );

VOID                              *
EfiReallocatePool (
  IN VOID                 *OldPool,
  IN UINTN                OldSize,
  IN UINTN                NewSize
  );

CHAR16                            *
DevicePathToStr (
  EFI_DEVICE_PATH_PROTOCOL     *DevPath
  );

VOID                              *
BdsLibGetVariableAndSize (
  IN CHAR16               *Name,
  IN EFI_GUID             *VendorGuid,
  OUT UINTN               *VarSize
  );

EFI_STATUS
EfiLibDeleteVariable (
  IN CHAR16   *VarName,
  IN EFI_GUID *VarGuid
  );

CHAR16                            *
EfiStrDuplicate (
  IN CHAR16   *Src
  );

BOOLEAN
EfiLibMatchDevicePaths (
  IN  EFI_DEVICE_PATH_PROTOCOL *Multi,
  IN  EFI_DEVICE_PATH_PROTOCOL *Single
  );

UINTN
EfiDevicePathInstanceCount (
  IN EFI_DEVICE_PATH_PROTOCOL      *DevicePath
  );

EFI_STATUS
CreateMenuStringToken (
  IN BMM_CALLBACK_DATA                *CallbackData,
  IN EFI_HII_HANDLE                   HiiHandle,
  IN BM_MENU_OPTION                   *MenuOption
  );

UINT16                            *
EfiLibStrFromDatahub (
  IN EFI_DEVICE_PATH_PROTOCOL                 *DevPath
  );

VOID                              *
GetLegacyBootOptionVar (
  IN  UINTN                            DeviceType,
  OUT UINTN                            *OptionIndex,
  OUT UINTN                            *OptionSize
 );

EFI_STATUS
InitializeBM (
  VOID
  );

EFI_STATUS
BdsStartBootMaint (
  VOID
  );

VOID
InitializeStringDepository ();

STRING_REF
GetStringTokenFromDepository (
  IN   BMM_CALLBACK_DATA     *CallbackData,
  IN   STRING_DEPOSITORY     *StringDepository
  ) ;

VOID
ReclaimStringDepository (
  VOID
  );

VOID
CleanUpStringDepository (
  VOID
  );

EFI_STATUS
ApplyChangeHandler (
  IN  BMM_CALLBACK_DATA               *Private,
  IN  BMM_FAKE_NV_DATA                *CurrentFakeNVMap,
  IN  FORM_ID                         FormId
  );

VOID
DiscardChangeHandler (
  IN  BMM_CALLBACK_DATA               *Private,
  IN  BMM_FAKE_NV_DATA                *CurrentFakeNVMap
  );

VOID
UpdatePageId (
  BMM_CALLBACK_DATA              *Private,
  UINT16                         NewPageId
  );

EFI_STATUS
BootThisFile (
  IN BM_FILE_CONTEXT                   *FileContext
  );

BOOLEAN
UpdateFileExplorer (
  IN BMM_CALLBACK_DATA            *CallbackData,
  IN UINT16                       KeyValue
  );

EFI_STATUS
EFIAPI
FileExplorerCallback (
  IN EFI_FORM_CALLBACK_PROTOCOL       *This,
  IN UINT16                           KeyValue,
  IN EFI_IFR_DATA_ARRAY               *Data,
  OUT EFI_HII_CALLBACK_PACKET         **Packet
  );

EFI_STATUS
FormSetDispatcher (
  IN  BMM_CALLBACK_DATA    *CallbackData
  );

VOID CreateCallbackPacket (
  OUT EFI_HII_CALLBACK_PACKET         **Packet,
  IN  UINT16                          Flags
  );

//
// Global variable in this program (defined in data.c)
//
extern BM_MENU_OPTION             BootOptionMenu;
extern BM_MENU_OPTION             DriverOptionMenu;
extern BM_MENU_OPTION             FsOptionMenu;
extern BM_MENU_OPTION             ConsoleInpMenu;
extern BM_MENU_OPTION             ConsoleOutMenu;
extern BM_MENU_OPTION             ConsoleErrMenu;
extern BM_MENU_OPTION             DirectoryMenu;
extern BM_MENU_OPTION             DriverMenu;
extern BM_MENU_OPTION             TerminalMenu;
extern BM_MENU_OPTION             LegacyFDMenu;
extern BM_MENU_OPTION             LegacyHDMenu;
extern BM_MENU_OPTION             LegacyCDMenu;
extern BM_MENU_OPTION             LegacyNETMenu;
extern BM_MENU_OPTION             LegacyBEVMenu;
extern UINT16                     TerminalType[];
extern COM_ATTR                   BaudRateList[19];
extern COM_ATTR                   DataBitsList[4];
extern COM_ATTR                   ParityList[5];
extern COM_ATTR                   StopBitsList[3];
extern EFI_GUID                   Guid[4];
extern EFI_HII_UPDATE_DATA        *UpdateData;
extern STRING_DEPOSITORY          *FileOptionStrDepository;
extern STRING_DEPOSITORY          *ConsoleOptionStrDepository;
extern STRING_DEPOSITORY          *BootOptionStrDepository;
extern STRING_DEPOSITORY          *BootOptionHelpStrDepository;
extern STRING_DEPOSITORY          *DriverOptionStrDepository;
extern STRING_DEPOSITORY          *DriverOptionHelpStrDepository;
extern STRING_DEPOSITORY          *TerminalStrDepository;
extern EFI_DEVICE_PATH_PROTOCOL   EndDevicePath[];
extern EFI_GUID                   EfiLegacyDevOrderGuid;

#endif
