/** @file
  Provides a way for 3rd party applications to register themselves for launch by the
  Boot Manager based on hot key

Copyright (c) 2007 - 2012, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _HOTKEY_H_
#define _HOTKEY_H_

#include "Bds.h"
#include "String.h"

#define SET_BOOT_OPTION_SUPPORT_KEY_COUNT(a, c) {  \
      (a) = ((a) & ~EFI_BOOT_OPTION_SUPPORT_COUNT) | (((c) << LowBitSet32 (EFI_BOOT_OPTION_SUPPORT_COUNT)) & EFI_BOOT_OPTION_SUPPORT_COUNT); \
      }

#define BDS_HOTKEY_OPTION_SIGNATURE SIGNATURE_32 ('B', 'd', 'K', 'O')

/**
  Get the revision of the EFI_KEY_OPTION structure.

  @param   KeyOption  Pointer to the EFI_KEY_OPTION structure. 

  @return  Revision.
**/
#define KEY_OPTION_REVISION(KeyOption)           ((KeyOption)->KeyData & EFI_KEY_OPTION_REVISION_MASK)

/**
  Get the actual number of entries in EFI_KEY_OPTION.Keys, from 0-3.

  @param   KeyOption  Pointer to the EFI_KEY_OPTION structure. 

  @return  Actual number of entries in EFI_KEY_OPTION.Keys.
**/
#define KEY_OPTION_INPUT_KEY_COUNT(KeyOption)    (((KeyOption)->KeyData & EFI_KEY_OPTION_INPUT_KEY_COUNT_MASK) >> LowBitSet32 (EFI_KEY_OPTION_INPUT_KEY_COUNT_MASK))

/**
  Return whether the Shift key needs pressed.

  @param   KeyOption  Pointer to the EFI_KEY_OPTION structure. 

  @retval  TRUE  Shift key needs pressed.
  @retval  FALSE Shift key needn't pressed.
**/
#define KEY_OPTION_SHIFT_PRESSED(KeyOption)      (BOOLEAN) (((KeyOption)->KeyData & EFI_KEY_OPTION_SHIFT_PRESSED_MASK) != 0)

/**
  Return whether the Control key needs pressed.

  @param   KeyOption  Pointer to the EFI_KEY_OPTION structure. 

  @retval  TRUE  Control key needs pressed.
  @retval  FALSE Control key needn't pressed.
**/
#define KEY_OPTION_CONTROL_PRESSED(KeyOption)    (BOOLEAN) (((KeyOption)->KeyData & EFI_KEY_OPTION_CONTROL_PRESSED_MASK) != 0)

/**
  Return whether the Alt key needs pressed.

  @param   KeyOption  Pointer to the EFI_KEY_OPTION structure. 

  @retval  TRUE  Alt key needs pressed.
  @retval  FALSE Alt key needn't pressed.
**/
#define KEY_OPTION_ALT_PRESSED(KeyOption)        (BOOLEAN) (((KeyOption)->KeyData & EFI_KEY_OPTION_ALT_PRESSED_MASK) != 0)

/**
  Return whether the Logo key needs pressed.

  @param   KeyOption  Pointer to the EFI_KEY_OPTION structure. 

  @retval  TRUE  Logo key needs pressed.
  @retval  FALSE Logo key needn't pressed.
**/
#define KEY_OPTION_LOGO_PRESSED(KeyOption)       (BOOLEAN) (((KeyOption)->KeyData & EFI_KEY_OPTION_LOGO_PRESSED_MASK) != 0)

/**
  Return whether the Menu key needs pressed.

  @param   KeyOption  Pointer to the EFI_KEY_OPTION structure. 

  @retval  TRUE  Menu key needs pressed.
  @retval  FALSE Menu key needn't pressed.
**/
#define KEY_OPTION_MENU_PRESSED(KeyOption)       (BOOLEAN) (((KeyOption)->KeyData & EFI_KEY_OPTION_MENU_PRESSED_MASK) != 0)

/**
  Return whether the SysReq key needs pressed.

  @param   KeyOption  Pointer to the EFI_KEY_OPTION structure. 

  @retval  TRUE  SysReq key needs pressed.
  @retval  FALSE SysReq key needn't pressed.
**/
#define KEY_OPTION_SYS_REQ_PRESSED(KeyOption)    (BOOLEAN) (((KeyOption)->KeyData & EFI_KEY_OPTION_SYS_REQ_PRESSED_MASK) != 0)

typedef struct {
  UINTN                     Signature;
  LIST_ENTRY                Link;

  EFI_HANDLE                NotifyHandle;
  UINT16                    BootOptionNumber;
  UINT8                     CodeCount;
  UINT8                     WaitingKey;
  EFI_KEY_DATA              KeyData[3];
} BDS_HOTKEY_OPTION;

#define BDS_HOTKEY_OPTION_FROM_LINK(a) CR (a, BDS_HOTKEY_OPTION, Link, BDS_HOTKEY_OPTION_SIGNATURE)

/**

  Process all the "Key####" variables, associate Hotkeys with corresponding Boot Options.


  @param VOID

  @retval  EFI_SUCCESS    Hotkey services successfully initialized.

**/
EFI_STATUS
InitializeHotkeyService (
  VOID
  );

/**
  Try to boot the boot option triggered by hotkey.
  @retval  EFI_SUCCESS             There is HotkeyBootOption & it is processed
  @retval  EFI_NOT_FOUND           There is no HotkeyBootOption
**/
EFI_STATUS
HotkeyBoot (
  VOID
  );

#endif
