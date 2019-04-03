/** @file
Formset guids, form id and VarStore data structure for Boot Maintenance Manager.

Copyright (c) 2004 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#ifndef _FORM_GUID_H_
#define _FORM_GUID_H_

#define BOOT_MAINT_FORMSET_GUID \
  { \
  0x642237c7, 0x35d4, 0x472d, {0x83, 0x65, 0x12, 0xe0, 0xcc, 0xf2, 0x7a, 0x22} \
  }

#define FORM_MAIN_ID                         0x1001
#define FORM_BOOT_ADD_ID                     0x1002
#define FORM_BOOT_DEL_ID                     0x1003
#define FORM_BOOT_CHG_ID                     0x1004
#define FORM_DRV_ADD_ID                      0x1005
#define FORM_DRV_DEL_ID                      0x1006
#define FORM_DRV_CHG_ID                      0x1007
#define FORM_CON_MAIN_ID                     0x1008
#define FORM_CON_IN_ID                       0x1009
#define FORM_CON_OUT_ID                      0x100A
#define FORM_CON_ERR_ID                      0x100B
#define FORM_FILE_SEEK_ID                    0x100C
#define FORM_FILE_NEW_SEEK_ID                0x100D
#define FORM_DRV_ADD_FILE_ID                 0x100E
#define FORM_DRV_ADD_HANDLE_ID               0x100F
#define FORM_DRV_ADD_HANDLE_DESC_ID          0x1010
#define FORM_BOOT_NEXT_ID                    0x1011
#define FORM_TIME_OUT_ID                     0x1012
#define FORM_BOOT_SETUP_ID                   0x1014
#define FORM_DRIVER_SETUP_ID                 0x1015
#define FORM_BOOT_LEGACY_DEVICE_ID           0x1016
#define FORM_CON_COM_ID                      0x1017
#define FORM_CON_COM_SETUP_ID                0x1018
#define FORM_BOOT_ADD_DESCRIPTION_ID         0x101F
#define FORM_DRIVER_ADD_FILE_DESCRIPTION_ID  0x1020
#define FORM_CON_MODE_ID                     0x1021
#define FORM_BOOT_FROM_FILE_ID               0x1024


#define MAXIMUM_FORM_ID                      0x10FF

#define KEY_VALUE_COM_SET_BAUD_RATE          0x1101
#define KEY_VALUE_COM_SET_DATA_BITS          0x1102
#define KEY_VALUE_COM_SET_STOP_BITS          0x1103
#define KEY_VALUE_COM_SET_PARITY             0x1104
#define KEY_VALUE_COM_SET_TERMI_TYPE         0x1105
#define KEY_VALUE_MAIN_BOOT_NEXT             0x1106
#define KEY_VALUE_BOOT_ADD_DESC_DATA         0x1107
#define KEY_VALUE_BOOT_ADD_OPT_DATA          0x1108
#define KEY_VALUE_DRIVER_ADD_DESC_DATA       0x1109
#define KEY_VALUE_DRIVER_ADD_OPT_DATA        0x110A
#define KEY_VALUE_SAVE_AND_EXIT              0x110B
#define KEY_VALUE_NO_SAVE_AND_EXIT           0x110C
#define KEY_VALUE_BOOT_FROM_FILE             0x110D
#define FORM_RESET                           0x110E
#define KEY_VALUE_BOOT_DESCRIPTION           0x110F
#define KEY_VALUE_BOOT_OPTION                0x1110
#define KEY_VALUE_DRIVER_DESCRIPTION         0x1111
#define KEY_VALUE_DRIVER_OPTION              0x1112
#define KEY_VALUE_SAVE_AND_EXIT_BOOT         0x1113
#define KEY_VALUE_NO_SAVE_AND_EXIT_BOOT      0x1114
#define KEY_VALUE_SAVE_AND_EXIT_DRIVER       0x1115
#define KEY_VALUE_NO_SAVE_AND_EXIT_DRIVER    0x1116
#define KEY_VALUE_TRIGGER_FORM_OPEN_ACTION   0x1117

#define MAXIMUM_NORMAL_KEY_VALUE             0x11FF

//
// Varstore ID defined for Buffer Storage
//
#define VARSTORE_ID_BOOT_MAINT               0x1000

//
// End Label
//
#define LABEL_FORM_MAIN_START                0xfffc
#define LABEL_FORM_MAIN_END                  0xfffd

#define LABEL_BMM_PLATFORM_INFORMATION       0xfffe
#define LABEL_END                            0xffff
#define MAX_MENU_NUMBER                      100


///
/// This is the structure that will be used to store the
/// question's current value. Use it at initialize time to
/// set default value for each question. When using at run
/// time, this map is returned by the callback function,
/// so dynamically changing the question's value will be
/// possible through this mechanism
///
typedef struct {
  //
  // Three questions displayed at the main page
  // for Timeout, BootNext, Variables respectively
  //
  UINT16  BootTimeOut;
  UINT32  BootNext;

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
  UINT16  DriverAddHandleDesc[MAX_MENU_NUMBER];
  UINT16  DriverAddHandleOptionalData[MAX_MENU_NUMBER];
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
  UINT8   ConsoleCheck[MAX_MENU_NUMBER];

  //
  // At most 100 input/output/errorout device for console storage
  //
  UINT8   ConsoleInCheck[MAX_MENU_NUMBER];
  UINT8   ConsoleOutCheck[MAX_MENU_NUMBER];
  UINT8   ConsoleErrCheck[MAX_MENU_NUMBER];

  //
  // Boot or Driver Option Order storage
  // The value is the OptionNumber+1 because the order list value cannot be 0
  // Use UINT32 to hold the potential value 0xFFFF+1=0x10000
  //
  UINT32  BootOptionOrder[MAX_MENU_NUMBER];
  UINT32  DriverOptionOrder[MAX_MENU_NUMBER];
  //
  // Boot or Driver Option Delete storage
  //
  BOOLEAN BootOptionDel[MAX_MENU_NUMBER];
  BOOLEAN DriverOptionDel[MAX_MENU_NUMBER];
  BOOLEAN BootOptionDelMark[MAX_MENU_NUMBER];
  BOOLEAN DriverOptionDelMark[MAX_MENU_NUMBER];

  //
  // This is the Terminal Attributes value storage
  //
  UINT8   COMBaudRate[MAX_MENU_NUMBER];
  UINT8   COMDataRate[MAX_MENU_NUMBER];
  UINT8   COMStopBits[MAX_MENU_NUMBER];
  UINT8   COMParity[MAX_MENU_NUMBER];
  UINT8   COMTerminalType[MAX_MENU_NUMBER];
  UINT8   COMFlowControl[MAX_MENU_NUMBER];

  //
  // We use DisableMap array to record the enable/disable state of each boot device
  // It should be taken as a bit array, from left to right there are totally 256 bits
  // the most left one stands for BBS table item 0, and the most right one stands for item 256
  // If the bit is 1, it means the boot device has been disabled.
  //
  UINT8   DisableMap[32];

  //
  // Console Output Text Mode
  //
  UINT16  ConsoleOutMode;

  //
  //  UINT16                    PadArea[10];
  //

  UINT16  BootDescriptionData[MAX_MENU_NUMBER];
  UINT16  BootOptionalData[127];
  UINT16  DriverDescriptionData[MAX_MENU_NUMBER];
  UINT16  DriverOptionalData[127];
  BOOLEAN BootOptionChanged;
  BOOLEAN DriverOptionChanged;
  UINT8   Active;
  UINT8   ForceReconnect;
} BMM_FAKE_NV_DATA;

#endif

