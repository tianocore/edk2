/** @file
  Header file for boot maintenance module.

Copyright (c) 2004 - 2008, Intel Corporation. <BR>
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _BOOT_MAINT_H
#define _BOOT_MAINT_H

#include "Bds.h"
#include "BBSsupport.h"
#include "FormGuid.h"
#include "FrontPage.h"

//
// Constants which are variable names used to access variables
//
#define VarLegacyDevOrder L"LegacyDevOrder"

#define VarConOutMode L"ConOutMode"

//
// Guid of a NV Variable which store the information about the
// FD/HD/CD/NET/BEV order
//
#define EFI_LEGACY_DEV_ORDER_VARIABLE_GUID \
  { \
  0xa56074db, 0x65fe, 0x45f7, {0xbd, 0x21, 0x2d, 0x2b, 0xdd, 0x8e, 0x96, 0x52} \
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
// Variable created with this flag will be "Efi:...."
//
#define VAR_FLAG  EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_NON_VOLATILE

//
// Define Maxmim characters that will be accepted
//
#define MAX_CHAR      480
#define MAX_CHAR_SIZE (MAX_CHAR * 2)

//
// Check to see if current build support option active feature of
// some driver option
//
#ifndef LOAD_OPTION_ACTIVE
#define LOAD_OPTION_ACTIVE  0x00000001
#endif

//
// Check to see if current build support force reconnect feature of
// some driver option
//
#ifndef LOAD_OPTION_FORCE_RECONNECT
#define LOAD_OPTION_FORCE_RECONNECT 0x00000002
#endif

extern EFI_GUID mBootMaintGuid;
extern EFI_GUID mFileExplorerGuid;

//
// These are the VFR compiler generated data representing our VFR data.
//
extern UINT8    BmBin[];
extern UINT8    FEBin[];

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
#define BMM_CALLBACK_DATA_FROM_THIS(a)  CR (a, BMM_CALLBACK_DATA, BmmConfigAccess, BMM_CALLBACK_DATA_SIGNATURE)

#define FE_CALLBACK_DATA_FROM_THIS(a)   CR (a, BMM_CALLBACK_DATA, FeConfigAccess, BMM_CALLBACK_DATA_SIGNATURE)

//
// Enumeration type definition
//
typedef UINT8 BBS_TYPE;

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
#define BM_MENU_OPTION_SIGNATURE      EFI_SIGNATURE_32 ('m', 'e', 'n', 'u')
#define BM_LOAD_OPTION_SIGNATURE      EFI_SIGNATURE_32 ('l', 'o', 'a', 'd')
#define BM_CONSOLE_OPTION_SIGNATURE   EFI_SIGNATURE_32 ('c', 'n', 's', 'l')
#define BM_FILE_OPTION_SIGNATURE      EFI_SIGNATURE_32 ('f', 'i', 'l', 'e')
#define BM_HANDLE_OPTION_SIGNATURE    EFI_SIGNATURE_32 ('h', 'n', 'd', 'l')
#define BM_TERMINAL_OPTION_SIGNATURE  EFI_SIGNATURE_32 ('t', 'r', 'm', 'l')
#define BM_MENU_ENTRY_SIGNATURE       EFI_SIGNATURE_32 ('e', 'n', 't', 'r')

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
// Buffer size for update data
//
#define UPDATE_DATA_SIZE        0x100000

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
#define VAR_OFFSET(Field)              ((UINT16) ((UINTN) &(((BMM_FAKE_NV_DATA *) 0)->Field)))

//
// Question Id of Zero is invalid, so add an offset to it
//
#define QUESTION_ID(Field)             (VAR_OFFSET (Field) + CONFIG_OPTION_OFFSET)

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
#define OPTION_ORDER_VAR_OFFSET         VAR_OFFSET (OptionOrder)
#define DRIVER_OPTION_ORDER_VAR_OFFSET  VAR_OFFSET (DriverOptionToBeDeleted)
#define BOOT_OPTION_DEL_VAR_OFFSET      VAR_OFFSET (BootOptionDel)
#define DRIVER_OPTION_DEL_VAR_OFFSET    VAR_OFFSET (DriverOptionDel)
#define DRIVER_ADD_OPTION_VAR_OFFSET    VAR_OFFSET (DriverAddHandleOptionalData)
#define COM_BAUD_RATE_VAR_OFFSET        VAR_OFFSET (COMBaudRate)
#define COM_DATA_RATE_VAR_OFFSET        VAR_OFFSET (COMDataRate)
#define COM_STOP_BITS_VAR_OFFSET        VAR_OFFSET (COMStopBits)
#define COM_PARITY_VAR_OFFSET           VAR_OFFSET (COMParity)
#define COM_TERMINAL_VAR_OFFSET         VAR_OFFSET (COMTerminalType)
#define LEGACY_FD_VAR_OFFSET            VAR_OFFSET (LegacyFD)
#define LEGACY_HD_VAR_OFFSET            VAR_OFFSET (LegacyHD)
#define LEGACY_CD_VAR_OFFSET            VAR_OFFSET (LegacyCD)
#define LEGACY_NET_VAR_OFFSET           VAR_OFFSET (LegacyNET)
#define LEGACY_BEV_VAR_OFFSET           VAR_OFFSET (LegacyBEV)

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
#define CON_MODE_QUESTION_ID            QUESTION_ID (ConsoleOutMode)
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

#pragma pack(1)
typedef struct {
  BBS_TYPE  BbsType;
  //
  // Length = sizeof (UINT16) + SIZEOF (Data)
  //
  UINT16    Length;
  UINT16    *Data;
} BM_LEGACY_DEV_ORDER_CONTEXT;
#pragma pack()

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
  UINTN   Column;
  UINTN   Row;
} CONSOLE_OUT_MODE;

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
  EFI_STRING_ID   DisplayStringToken;
  EFI_STRING_ID   HelpStringToken;
  UINTN           ContextSelection;
  VOID            *VariableContext;
} BM_MENU_ENTRY;

typedef struct {
  //
  // Shared callback data.
  //
  UINTN                          Signature;

  BM_MENU_ENTRY                  *MenuEntry;
  BM_HANDLE_CONTEXT              *HandleContext;
  BM_FILE_CONTEXT                *FileContext;
  BM_LOAD_CONTEXT                *LoadContext;
  BM_TERMINAL_CONTEXT            *TerminalContext;
  UINTN                          CurrentTerminal;
  BBS_TYPE                       BbsType;

  //
  // BMM main formset callback data.
  //
  EFI_HII_HANDLE                 BmmHiiHandle;
  EFI_HANDLE                     BmmDriverHandle;
  EFI_HII_CONFIG_ACCESS_PROTOCOL BmmConfigAccess;
  EFI_FORM_ID                    BmmCurrentPageId;
  EFI_FORM_ID                    BmmPreviousPageId;
  BOOLEAN                        BmmAskSaveOrNot;
  BMM_FAKE_NV_DATA               BmmFakeNvData;
  BMM_FAKE_NV_DATA               BmmOldFakeNVData;

  //
  // File explorer formset callback data.
  //
  EFI_HII_HANDLE                 FeHiiHandle;
  EFI_HANDLE                     FeDriverHandle;
  EFI_HII_CONFIG_ACCESS_PROTOCOL FeConfigAccess;
  FILE_EXPLORER_STATE            FeCurrentState;
  FILE_EXPLORER_DISPLAY_CONTEXT  FeDisplayContext;
  FILE_EXPLORER_NV_DATA          FeFakeNvData;
} BMM_CALLBACK_DATA;

typedef struct _STRING_LIST_NODE {
  EFI_STRING_ID             StringToken;
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
/**
  EDES_TODO: Add function description.

  @param CallbackData    EDES_TODO: Add parameter description

  @return EDES_TODO: Add description for return value

**/
EFI_STATUS
BOpt_FindFileSystem (
  IN BMM_CALLBACK_DATA          *CallbackData
  )
;

//
// For cleaning up File System menu
//
/**
  EDES_TODO: Add function description.

  @param VOID            EDES_TODO: Add parameter description

  @return EDES_TODO: Add description for return value

**/
VOID
BOpt_FreeFileSystem (
  VOID
  )
;

//
// For initializing File Navigation menu
//
/**
  EDES_TODO: Add function description.

  @param CallbackData    EDES_TODO: Add parameter description
  @param MenuEntry       EDES_TODO: Add parameter description

  @return EDES_TODO: Add description for return value

**/
EFI_STATUS
BOpt_FindFiles (
  IN BMM_CALLBACK_DATA          *CallbackData,
  IN BM_MENU_ENTRY              *MenuEntry
  )
;

//
// For cleaning up File Navigation menu
//
/**
  EDES_TODO: Add function description.

  @param VOID            EDES_TODO: Add parameter description

  @return EDES_TODO: Add description for return value

**/
VOID
BOpt_FreeFiles (
  VOID
  )
;

//
// For Initializing handle navigation menu
//
/**
  EDES_TODO: Add function description.

  @param VOID            EDES_TODO: Add parameter description

  @return EDES_TODO: Add description for return value

**/
EFI_STATUS
BOpt_FindDrivers (
  VOID
  )
;

//
// For Cleaning up handle navigation menu
//
/**
  EDES_TODO: Add function description.

  @param VOID            EDES_TODO: Add parameter description

  @return EDES_TODO: Add description for return value

**/
VOID
BOpt_FreeDrivers(VOID);

//
// For initializing Boot Option menu
//
/**
  EDES_TODO: Add function description.

  @param CallbackData    EDES_TODO: Add parameter description

  @return EDES_TODO: Add description for return value

**/
EFI_STATUS
BOpt_GetBootOptions (
  IN  BMM_CALLBACK_DATA         *CallbackData
  );

//
// For Initializing Driver option menu
//
/**
  EDES_TODO: Add function description.

  @param CallbackData    EDES_TODO: Add parameter description

  @return EDES_TODO: Add description for return value

**/
EFI_STATUS
BOpt_GetDriverOptions (
  IN  BMM_CALLBACK_DATA         *CallbackData
  );

//
// For Cleaning up boot option menu
//
/**
  EDES_TODO: Add function description.

  @param VOID            EDES_TODO: Add parameter description

  @return EDES_TODO: Add description for return value

**/
VOID
BOpt_FreeBootOptions (VOID);

//
// For cleaning up driver option menu
//
/**
  EDES_TODO: Add function description.

  @param VOID            EDES_TODO: Add parameter description

  @return EDES_TODO: Add description for return value

**/
VOID
BOpt_FreeDriverOptions(VOID);

//
// For Initializing HD/FD/CD/NET/BEV option menu
//
/**
  EDES_TODO: Add function description.

  @param VOID            EDES_TODO: Add parameter description

  @return EDES_TODO: Add description for return value

**/
EFI_STATUS
BOpt_GetLegacyOptions(VOID);

//
// For cleaning up driver option menu
//
/**
  EDES_TODO: Add function description.

  @param VOID            EDES_TODO: Add parameter description

  @return EDES_TODO: Add description for return value

**/
VOID
BOpt_FreeLegacyOptions(VOID);

//
// this function is used to take place of all other free menu actions
//
/**
  EDES_TODO: Add function description.

  @param FreeMenu        EDES_TODO: Add parameter description

  @return EDES_TODO: Add description for return value

**/
VOID
BOpt_FreeMenu (
  BM_MENU_OPTION        *FreeMenu
  );


//
// Following are the helper functions used
//
/**
  EDES_TODO: Add function description.

  @param Str1            EDES_TODO: Add parameter description
  @param Str2            EDES_TODO: Add parameter description

  @return EDES_TODO: Add description for return value

**/
CHAR16                            *
BOpt_AppendFileName (
  IN  CHAR16  *Str1,
  IN  CHAR16  *Str2
  );

/**
  EDES_TODO: Add function description.

  @param FileName        EDES_TODO: Add parameter description

  @return EDES_TODO: Add description for return value

**/
BOOLEAN
BOpt_IsEfiImageName (
  IN UINT16  *FileName
  );

/**
  EDES_TODO: Add function description.

  @param Dir             EDES_TODO: Add parameter description
  @param FileName        EDES_TODO: Add parameter description

  @return EDES_TODO: Add description for return value

**/
BOOLEAN
BOpt_IsEfiApp (
  IN EFI_FILE_HANDLE Dir,
  IN UINT16          *FileName
  );

//
// Get current unused boot option number
//
/**
  EDES_TODO: Add function description.

  @param VOID            EDES_TODO: Add parameter description

  @return EDES_TODO: Add description for return value

**/
UINT16
BOpt_GetBootOptionNumber (VOID);

//
// Get current unused driver option number
//
/**
  EDES_TODO: Add function description.

  @param VOID            EDES_TODO: Add parameter description

  @return EDES_TODO: Add description for return value

**/
UINT16
BOpt_GetDriverOptionNumber (VOID);

/**
  EDES_TODO: Add function description.

  @param MenuType        EDES_TODO: Add parameter description

  @return EDES_TODO: Add description for return value

**/
BM_MENU_ENTRY                     *
BOpt_CreateMenuEntry (
  UINTN           MenuType
  );

/**
  EDES_TODO: Add function description.

  @param MenuEntry       EDES_TODO: Add parameter description

  @return EDES_TODO: Add description for return value

**/
VOID
BOpt_DestroyMenuEntry (
  BM_MENU_ENTRY         *MenuEntry
  );

/**
  EDES_TODO: Add function description.

  @param MenuOption      EDES_TODO: Add parameter description
  @param MenuNumber      EDES_TODO: Add parameter description

  @return EDES_TODO: Add description for return value

**/
BM_MENU_ENTRY                     *
BOpt_GetMenuEntry (
  BM_MENU_OPTION      *MenuOption,
  UINTN               MenuNumber
  );

//
// a helper function used to free pool type memory
//
/**
  EDES_TODO: Add function description.

  @param Buffer          EDES_TODO: Add parameter description

  @return EDES_TODO: Add description for return value

**/
VOID
SafeFreePool (
  IN VOID *Buffer
  );

//
// Locate all serial io devices for console
//
/**
  EDES_TODO: Add function description.

  @param VOID            EDES_TODO: Add parameter description

  @return EDES_TODO: Add description for return value

**/
EFI_STATUS
LocateSerialIo (VOID);

//
// Initializing Console menu
//
/**
  EDES_TODO: Add function description.

  @param VOID            EDES_TODO: Add parameter description

  @return EDES_TODO: Add description for return value

**/
EFI_STATUS
GetAllConsoles(VOID);

//
// Get current mode information
//
/**
  EDES_TODO: Add function description.

  @param CallbackData    EDES_TODO: Add parameter description

  @return EDES_TODO: Add description for return value

**/
VOID
GetConsoleOutMode (
  IN  BMM_CALLBACK_DATA    *CallbackData
  );

//
// Cleaning up console menu
//
/**
  EDES_TODO: Add function description.

  @param VOID            EDES_TODO: Add parameter description

  @return EDES_TODO: Add description for return value

**/
EFI_STATUS
FreeAllConsoles(VOID);

/**
  EDES_TODO: Add function description.

  @param DevicePath      EDES_TODO: Add parameter description

  @return EDES_TODO: Add description for return value

**/
VOID
ChangeVariableDevicePath (
  EFI_DEVICE_PATH_PROTOCOL  *DevicePath
);

/**
  EDES_TODO: Add function description.

  @param DevicePath      EDES_TODO: Add parameter description
  @param ChangeTerminal  EDES_TODO: Add parameter description

  @return EDES_TODO: Add description for return value

**/
EFI_STATUS
ChangeTerminalDevicePath (
  EFI_DEVICE_PATH_PROTOCOL  *DevicePath,
  BOOLEAN                   ChangeTerminal
);
//
// Variable operation by menu selection
//
/**
  EDES_TODO: Add function description.

  @param CallbackData    EDES_TODO: Add parameter description
  @param NvRamMap        EDES_TODO: Add parameter description

  @return EDES_TODO: Add description for return value

**/
EFI_STATUS
Var_UpdateBootOption (
  IN  BMM_CALLBACK_DATA                   *CallbackData,
  IN  FILE_EXPLORER_NV_DATA               *NvRamMap
  );

/**
  EDES_TODO: Add function description.

  @param VOID            EDES_TODO: Add parameter description

  @return EDES_TODO: Add description for return value

**/
EFI_STATUS
Var_DelBootOption (VOID);

/**
  EDES_TODO: Add function description.

  @param VOID            EDES_TODO: Add parameter description

  @return EDES_TODO: Add description for return value

**/
EFI_STATUS
Var_ChangeBootOrder (VOID);

/**
  EDES_TODO: Add function description.

  @param CallbackData    EDES_TODO: Add parameter description
  @param HiiHandle       EDES_TODO: Add parameter description
  @param DescriptionData EDES_TODO: Add parameter description
  @param OptionalData    EDES_TODO: Add parameter description
  @param ForceReconnect  EDES_TODO: Add parameter description

  @return EDES_TODO: Add description for return value

**/
EFI_STATUS
Var_UpdateDriverOption (
  IN  BMM_CALLBACK_DATA         *CallbackData,
  IN  EFI_HII_HANDLE            HiiHandle,
  IN  UINT16                    *DescriptionData,
  IN  UINT16                    *OptionalData,
  IN  UINT8                     ForceReconnect
  );

/**
  EDES_TODO: Add function description.

  @param VOID            EDES_TODO: Add parameter description

  @return EDES_TODO: Add description for return value

**/
EFI_STATUS
Var_DelDriverOption (VOID);

/**
  EDES_TODO: Add function description.

  @param VOID            EDES_TODO: Add parameter description

  @return EDES_TODO: Add description for return value

**/
EFI_STATUS
Var_ChangeDriverOrder (VOID);

/**
  EDES_TODO: Add function description.

  @param VOID            EDES_TODO: Add parameter description

  @return EDES_TODO: Add description for return value

**/
EFI_STATUS
Var_UpdateConsoleInpOption (VOID);

/**
  EDES_TODO: Add function description.

  @param VOID            EDES_TODO: Add parameter description

  @return EDES_TODO: Add description for return value

**/
EFI_STATUS
Var_UpdateConsoleOutOption (VOID);

/**
  EDES_TODO: Add function description.

  @param VOID            EDES_TODO: Add parameter description

  @return EDES_TODO: Add description for return value

**/
EFI_STATUS
Var_UpdateErrorOutOption (VOID);

/**
  EDES_TODO: Add function description.

  @param VOID            EDES_TODO: Add parameter description

  @return EDES_TODO: Add description for return value

**/
VOID
Var_UpdateAllConsoleOption (VOID);

/**
  EDES_TODO: Add function description.

  @param CallbackData    EDES_TODO: Add parameter description

  @return EDES_TODO: Add description for return value

**/
EFI_STATUS
Var_UpdateBootNext (
  IN BMM_CALLBACK_DATA            *CallbackData
  );

/**
  EDES_TODO: Add function description.

  @param CallbackData    EDES_TODO: Add parameter description

  @return EDES_TODO: Add description for return value

**/
EFI_STATUS
Var_UpdateBootOrder (
  IN BMM_CALLBACK_DATA            *CallbackData
  );

/**
  EDES_TODO: Add function description.

  @param CallbackData    EDES_TODO: Add parameter description

  @return EDES_TODO: Add description for return value

**/
EFI_STATUS
Var_UpdateDriverOrder (
  IN BMM_CALLBACK_DATA            *CallbackData
  );

/**
  EDES_TODO: Add function description.

  @param CallbackData    EDES_TODO: Add parameter description

  @return EDES_TODO: Add description for return value

**/
EFI_STATUS
Var_UpdateBBSOption (
  IN BMM_CALLBACK_DATA            *CallbackData
  );

/**
  EDES_TODO: Add function description.

  @param CallbackData    EDES_TODO: Add parameter description

  @return EDES_TODO: Add description for return value

**/
EFI_STATUS
Var_UpdateConMode (
  IN BMM_CALLBACK_DATA            *CallbackData
  );

//
// Following are page create and refresh functions
//
/**
  EDES_TODO: Add function description.

  @param VOID            EDES_TODO: Add parameter description

  @return EDES_TODO: Add description for return value

**/
VOID
RefreshUpdateData (
  VOID
  );

/**
  EDES_TODO: Add function description.

  @param LabelId         EDES_TODO: Add parameter description
  @param CallbackData    EDES_TODO: Add parameter description

  @return EDES_TODO: Add description for return value

**/
VOID
CleanUpPage (
  IN UINT16                           LabelId,
  IN BMM_CALLBACK_DATA                *CallbackData
  );

/**
  EDES_TODO: Add function description.

  @param CallbackData    EDES_TODO: Add parameter description
  @param UpdatingMenu    EDES_TODO: Add parameter description
  @param UpdatingPage    EDES_TODO: Add parameter description
  @param UpdatingManner  EDES_TODO: Add parameter description
  @param QuestionIdStart EDES_TODO: Add parameter description
  @param GotoForm        EDES_TODO: Add parameter description
  @param GotoAlternateForm EDES_TODO: Add parameter description
  @param DisplayTokenStart EDES_TODO: Add parameter description
  @param HelpTokenStart  EDES_TODO: Add parameter description
  @param KeyValueStart   EDES_TODO: Add parameter description

  @return EDES_TODO: Add description for return value

**/
EFI_STATUS
UpdatePage (
  IN BMM_CALLBACK_DATA                *CallbackData,
  IN BM_MENU_OPTION                   *UpdatingMenu,
  IN UINT16                           UpdatingPage,
  IN UINT16                           UpdatingManner,
  IN UINT16                           QuestionIdStart,
  IN UINT16                           GotoForm,
  IN UINT16                           GotoAlternateForm,
  IN EFI_STRING_ID                    DisplayTokenStart,
  IN EFI_STRING_ID                    HelpTokenStart,
  IN UINT16                           KeyValueStart
  );

/**
  EDES_TODO: Add function description.

  @param CallbackData    EDES_TODO: Add parameter description

  @return EDES_TODO: Add description for return value

**/
VOID
UpdateBootAddPage (
  IN BMM_CALLBACK_DATA                *CallbackData
  );

/**
  EDES_TODO: Add function description.

  @param CallbackData    EDES_TODO: Add parameter description

  @return EDES_TODO: Add description for return value

**/
VOID
UpdateBootDelPage (
  IN BMM_CALLBACK_DATA                *CallbackData
  );

/**
  EDES_TODO: Add function description.

  @param CallbackData    EDES_TODO: Add parameter description

  @return EDES_TODO: Add description for return value

**/
VOID
UpdateDrvAddFilePage (
  IN BMM_CALLBACK_DATA                *CallbackData
  );

/**
  EDES_TODO: Add function description.

  @param CallbackData    EDES_TODO: Add parameter description

  @return EDES_TODO: Add description for return value

**/
VOID
UpdateDrvAddHandlePage (
  IN BMM_CALLBACK_DATA                *CallbackData
  );

/**
  EDES_TODO: Add function description.

  @param CallbackData    EDES_TODO: Add parameter description

  @return EDES_TODO: Add description for return value

**/
VOID
UpdateDrvDelPage (
  IN BMM_CALLBACK_DATA                *CallbackData
  );

/**
  EDES_TODO: Add function description.

  @param CallbackData    EDES_TODO: Add parameter description

  @return EDES_TODO: Add description for return value

**/
VOID
UpdateDriverAddHandleDescPage (
  IN BMM_CALLBACK_DATA                *CallbackData
  );

/**
  EDES_TODO: Add function description.

  @param CallbackData    EDES_TODO: Add parameter description

  @return EDES_TODO: Add description for return value

**/
VOID
UpdateBootTimeOut (
  IN BMM_CALLBACK_DATA                *CallbackData
  );

/**
  EDES_TODO: Add function description.

  @param CallbackData    EDES_TODO: Add parameter description

  @return EDES_TODO: Add description for return value

**/
VOID
UpdateConInPage (
  IN BMM_CALLBACK_DATA                *CallbackData
  );

/**
  EDES_TODO: Add function description.

  @param CallbackData    EDES_TODO: Add parameter description

  @return EDES_TODO: Add description for return value

**/
VOID
UpdateConOutPage (
  IN BMM_CALLBACK_DATA                *CallbackData
  );

/**
  EDES_TODO: Add function description.

  @param CallbackData    EDES_TODO: Add parameter description

  @return EDES_TODO: Add description for return value

**/
VOID
UpdateStdErrPage (
  IN BMM_CALLBACK_DATA                *CallbackData
  );

/**
  EDES_TODO: Add function description.

  @param UpdatePageId    EDES_TODO: Add parameter description
  @param CallbackData    EDES_TODO: Add parameter description

  @return EDES_TODO: Add description for return value

**/
VOID
UpdatePageBody (
  IN UINT16                           UpdatePageId,
  IN BMM_CALLBACK_DATA                *CallbackData
  );

/**
  EDES_TODO: Add function description.

  @param CallbackData    EDES_TODO: Add parameter description

  @return EDES_TODO: Add description for return value

**/
VOID
UpdateCOM1Page (
  IN BMM_CALLBACK_DATA                *CallbackData
  );

/**
  EDES_TODO: Add function description.

  @param CallbackData    EDES_TODO: Add parameter description

  @return EDES_TODO: Add description for return value

**/
VOID
UpdateCOM2Page (
  IN BMM_CALLBACK_DATA                *CallbackData
  );

/**
  EDES_TODO: Add function description.

  @param CallbackData    EDES_TODO: Add parameter description

  @return EDES_TODO: Add description for return value

**/
VOID
UpdateBootOrderPage (
  IN BMM_CALLBACK_DATA                *CallbackData
  );

/**
  EDES_TODO: Add function description.

  @param CallbackData    EDES_TODO: Add parameter description

  @return EDES_TODO: Add description for return value

**/
VOID
UpdateDriverOrderPage (
  IN BMM_CALLBACK_DATA                *CallbackData
  );

/**
  EDES_TODO: Add function description.

  @param CallbackData    EDES_TODO: Add parameter description

  @return EDES_TODO: Add description for return value

**/
VOID
UpdateBootNextPage (
  IN BMM_CALLBACK_DATA                *CallbackData
  );

/**
  EDES_TODO: Add function description.

  @param CallbackData    EDES_TODO: Add parameter description

  @return EDES_TODO: Add description for return value

**/
VOID
UpdateTimeOutPage (
  IN BMM_CALLBACK_DATA                *CallbackData
  );

/**
  EDES_TODO: Add function description.

  @param CallbackData    EDES_TODO: Add parameter description

  @return EDES_TODO: Add description for return value

**/
VOID
UpdateTerminalPage (
  IN BMM_CALLBACK_DATA                *CallbackData
  );

/**
  EDES_TODO: Add function description.

  @param CallbackData    EDES_TODO: Add parameter description

  @return EDES_TODO: Add description for return value

**/
VOID
UpdateConModePage (
  IN BMM_CALLBACK_DATA                *CallbackData
  );

/**
  EDES_TODO: Add function description.

  @param CallbackData    EDES_TODO: Add parameter description

  @return EDES_TODO: Add description for return value

**/
VOID
UpdateConCOMPage (
  IN BMM_CALLBACK_DATA                *CallbackData
  );

/**
  EDES_TODO: Add function description.

  @param UpdatePageId    EDES_TODO: Add parameter description
  @param CallbackData    EDES_TODO: Add parameter description

  @return EDES_TODO: Add description for return value

**/
VOID
UpdateSetLegacyDeviceOrderPage (
  IN UINT16                           UpdatePageId,
  IN BMM_CALLBACK_DATA                *CallbackData
);

/**
  EDES_TODO: Add function description.

  @param BbsType         EDES_TODO: Add parameter description
  @param BbsFlag         EDES_TODO: Add parameter description

  @return EDES_TODO: Add description for return value

**/
EFI_STATUS
BootLegacy (
  IN  UINT16  BbsType,
  IN  UINT16  BbsFlag
);

/**
  EDES_TODO: Add function description.

  @param TerminalNumber  EDES_TODO: Add parameter description

  @return EDES_TODO: Add description for return value

**/
BM_MENU_ENTRY                     *
GetCurrentTerminal (
  UINTN       TerminalNumber
);

/**
  EDES_TODO: Add function description.

  @param DeviceHandle    EDES_TODO: Add parameter description

  @return EDES_TODO: Add description for return value

**/
EFI_FILE_HANDLE
EfiLibOpenRoot (
  IN EFI_HANDLE       DeviceHandle
  );

/**
  EDES_TODO: Add function description.

  @param FHand           EDES_TODO: Add parameter description

  @return EDES_TODO: Add description for return value

**/
EFI_FILE_SYSTEM_VOLUME_LABEL_INFO *
EfiLibFileSystemVolumeLabelInfo (
  IN EFI_FILE_HANDLE      FHand
  );

/**
  EDES_TODO: Add function description.

  @param FHand           EDES_TODO: Add parameter description

  @return EDES_TODO: Add description for return value

**/
EFI_FILE_INFO                     *
EfiLibFileInfo (
  IN EFI_FILE_HANDLE      FHand
  );

/**
  EDES_TODO: Add function description.

  @param UStr            EDES_TODO: Add parameter description
  @param Length          EDES_TODO: Add parameter description
  @param AStr            EDES_TODO: Add parameter description

  @return EDES_TODO: Add description for return value

**/
UINTN
UnicodeToAscii (
  IN  CHAR16  *UStr,
  IN  UINTN   Length,
  OUT CHAR8   *AStr
  );

/**
  EDES_TODO: Add function description.

  @param DevPath         EDES_TODO: Add parameter description

  @return EDES_TODO: Add description for return value

**/
CHAR16                            *
DevicePathToStr (
  EFI_DEVICE_PATH_PROTOCOL     *DevPath
  );

/**
  EDES_TODO: Add function description.

  @param Size            EDES_TODO: Add parameter description

  @return EDES_TODO: Add description for return value

**/
VOID                              *
EfiAllocateZeroPool (
  IN UINTN            Size
  );

/**
  EDES_TODO: Add function description.

  @param ProtocolGuid    EDES_TODO: Add parameter description
  @param Interface       EDES_TODO: Add parameter description

  @return EDES_TODO: Add description for return value

**/
EFI_STATUS
EfiLibLocateProtocol (
  IN  EFI_GUID        *ProtocolGuid,
  OUT VOID            **Interface
  );

/**
  EDES_TODO: Add function description.

  @param OldPool         EDES_TODO: Add parameter description
  @param OldSize         EDES_TODO: Add parameter description
  @param NewSize         EDES_TODO: Add parameter description

  @return EDES_TODO: Add description for return value

**/
VOID                              *
EfiReallocatePool (
  IN VOID                 *OldPool,
  IN UINTN                OldSize,
  IN UINTN                NewSize
  );

/**
  EDES_TODO: Add function description.

  @param DevPath         EDES_TODO: Add parameter description

  @return EDES_TODO: Add description for return value

**/
CHAR16                            *
DevicePathToStr (
  EFI_DEVICE_PATH_PROTOCOL     *DevPath
  );

/**
  EDES_TODO: Add function description.

  @param Name            EDES_TODO: Add parameter description
  @param VendorGuid      EDES_TODO: Add parameter description
  @param VarSize         EDES_TODO: Add parameter description

  @return EDES_TODO: Add description for return value

**/
VOID                              *
BdsLibGetVariableAndSize (
  IN CHAR16               *Name,
  IN EFI_GUID             *VendorGuid,
  OUT UINTN               *VarSize
  );

/**
  EDES_TODO: Add function description.

  @param VarName         EDES_TODO: Add parameter description
  @param VarGuid         EDES_TODO: Add parameter description

  @return EDES_TODO: Add description for return value

**/
EFI_STATUS
EfiLibDeleteVariable (
  IN CHAR16   *VarName,
  IN EFI_GUID *VarGuid
  );

/**
  EDES_TODO: Add function description.

  @param Src             EDES_TODO: Add parameter description

  @return EDES_TODO: Add description for return value

**/
CHAR16                            *
EfiStrDuplicate (
  IN CHAR16   *Src
  );

/**
  EDES_TODO: Add function description.

  @param Multi           EDES_TODO: Add parameter description
  @param Single          EDES_TODO: Add parameter description

  @return EDES_TODO: Add description for return value

**/
BOOLEAN
EfiLibMatchDevicePaths (
  IN  EFI_DEVICE_PATH_PROTOCOL *Multi,
  IN  EFI_DEVICE_PATH_PROTOCOL *Single
  );

/**
  EDES_TODO: Add function description.

  @param DevicePath      EDES_TODO: Add parameter description

  @return EDES_TODO: Add description for return value

**/
UINTN
EfiDevicePathInstanceCount (
  IN EFI_DEVICE_PATH_PROTOCOL      *DevicePath
  );

/**
  EDES_TODO: Add function description.

  @param CallbackData    EDES_TODO: Add parameter description
  @param HiiHandle       EDES_TODO: Add parameter description
  @param MenuOption      EDES_TODO: Add parameter description

  @return EDES_TODO: Add description for return value

**/
EFI_STATUS
CreateMenuStringToken (
  IN BMM_CALLBACK_DATA                *CallbackData,
  IN EFI_HII_HANDLE                   HiiHandle,
  IN BM_MENU_OPTION                   *MenuOption
  );

/**
  EDES_TODO: Add function description.

  @param DevPath         EDES_TODO: Add parameter description

  @return EDES_TODO: Add description for return value

**/
UINT16                            *
EfiLibStrFromDatahub (
  IN EFI_DEVICE_PATH_PROTOCOL                 *DevPath
  );

/**
  EDES_TODO: Add function description.

  @param DeviceType      EDES_TODO: Add parameter description
  @param OptionIndex     EDES_TODO: Add parameter description
  @param OptionSize      EDES_TODO: Add parameter description

  @return EDES_TODO: Add description for return value

**/
VOID                              *
GetLegacyBootOptionVar (
  IN  UINTN                            DeviceType,
  OUT UINTN                            *OptionIndex,
  OUT UINTN                            *OptionSize
 );

/**
  EDES_TODO: Add function description.

  @param VOID            EDES_TODO: Add parameter description

  @return EDES_TODO: Add description for return value

**/
EFI_STATUS
InitializeBM (
  VOID
  );

/**
  EDES_TODO: Add function description.

  @param VOID            EDES_TODO: Add parameter description

  @return EDES_TODO: Add description for return value

**/
EFI_STATUS
BdsStartBootMaint (
  VOID
  );

/**
  EDES_TODO: Add function description.

  @param VOID            EDES_TODO: Add parameter description

  @return EDES_TODO: Add description for return value

**/
VOID
InitializeStringDepository (VOID);

/**
  EDES_TODO: Add function description.

  @param CallbackData    EDES_TODO: Add parameter description
  @param StringDepository EDES_TODO: Add parameter description

  @return EDES_TODO: Add description for return value

**/
EFI_STRING_ID
GetStringTokenFromDepository (
  IN   BMM_CALLBACK_DATA     *CallbackData,
  IN   STRING_DEPOSITORY     *StringDepository
  );

/**
  EDES_TODO: Add function description.

  @param VOID            EDES_TODO: Add parameter description

  @return EDES_TODO: Add description for return value

**/
VOID
ReclaimStringDepository (
  VOID
  );

/**
  EDES_TODO: Add function description.

  @param VOID            EDES_TODO: Add parameter description

  @return EDES_TODO: Add description for return value

**/
VOID
CleanUpStringDepository (
  VOID
  );

/**
  EDES_TODO: Add function description.

  @param Private         EDES_TODO: Add parameter description
  @param CurrentFakeNVMap EDES_TODO: Add parameter description
  @param FormId          EDES_TODO: Add parameter description

  @return EDES_TODO: Add description for return value

**/
EFI_STATUS
ApplyChangeHandler (
  IN  BMM_CALLBACK_DATA               *Private,
  IN  BMM_FAKE_NV_DATA                *CurrentFakeNVMap,
  IN  EFI_FORM_ID                     FormId
  );

/**
  EDES_TODO: Add function description.

  @param Private         EDES_TODO: Add parameter description
  @param CurrentFakeNVMap EDES_TODO: Add parameter description

  @return EDES_TODO: Add description for return value

**/
VOID
DiscardChangeHandler (
  IN  BMM_CALLBACK_DATA               *Private,
  IN  BMM_FAKE_NV_DATA                *CurrentFakeNVMap
  );

/**
  EDES_TODO: Add function description.

  @param Private         EDES_TODO: Add parameter description
  @param NewPageId       EDES_TODO: Add parameter description

  @return EDES_TODO: Add description for return value

**/
VOID
UpdatePageId (
  BMM_CALLBACK_DATA              *Private,
  UINT16                         NewPageId
  );

/**
  EDES_TODO: Add function description.

  @param FileContext     EDES_TODO: Add parameter description

  @return EDES_TODO: Add description for return value

**/
EFI_STATUS
BootThisFile (
  IN BM_FILE_CONTEXT                   *FileContext
  );

/**
  EDES_TODO: Add function description.

  @param CallbackData    EDES_TODO: Add parameter description
  @param KeyValue        EDES_TODO: Add parameter description

  @return EDES_TODO: Add description for return value

**/
BOOLEAN
UpdateFileExplorer (
  IN BMM_CALLBACK_DATA            *CallbackData,
  IN UINT16                       KeyValue
  );

/**
  EDES_TODO: Add function description.

  @param This            EDES_TODO: Add parameter description
  @param Action          EDES_TODO: Add parameter description
  @param QuestionId      EDES_TODO: Add parameter description
  @param Type            EDES_TODO: Add parameter description
  @param Value           EDES_TODO: Add parameter description
  @param ActionRequest   EDES_TODO: Add parameter description

  @return EDES_TODO: Add description for return value

**/
EFI_STATUS
EFIAPI
FileExplorerCallback (
  IN  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL   *This,
  IN  EFI_BROWSER_ACTION                     Action,
  IN  EFI_QUESTION_ID                        QuestionId,
  IN  UINT8                                  Type,
  IN  EFI_IFR_TYPE_VALUE                     *Value,
  OUT EFI_BROWSER_ACTION_REQUEST             *ActionRequest
  );

/**
  EDES_TODO: Add function description.

  @param CallbackData    EDES_TODO: Add parameter description

  @return EDES_TODO: Add description for return value

**/
EFI_STATUS
FormSetDispatcher (
  IN  BMM_CALLBACK_DATA    *CallbackData
  );

/**
  EDES_TODO: Add function description.

  @param Name            EDES_TODO: Add parameter description
  @param VendorGuid      EDES_TODO: Add parameter description

  @return EDES_TODO: Add description for return value

**/
VOID *
EfiLibGetVariable (
  IN CHAR16               *Name,
  IN EFI_GUID             *VendorGuid
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
extern EFI_HII_UPDATE_DATA        gUpdateData;
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
