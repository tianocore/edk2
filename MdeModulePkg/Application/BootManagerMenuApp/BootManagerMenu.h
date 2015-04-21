/** @file
  FrontPage routines to handle the callbacks and browser calls

Copyright (c) 2004 - 2015, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/


#ifndef _BOOT_MANAGER_MENU_H_
#define _BOOT_MANAGER_MENU_H_

#include <Uefi.h>
#include <Guid/MdeModuleHii.h>
#include <Library/UefiBootManagerLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/HiiLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DevicePathLib.h>
#include <Protocol/LoadedImage.h>
#include <Protocol/BootLogo.h>

#define TITLE_TOKEN_COUNT   1
#define HELP_TOKEN_COUNT    3

typedef struct _BOOT_MENU_SCREEN {
  UINTN        StartCol;
  UINTN        StartRow;
  UINTN        Width;
  UINTN        Height;
} BOOT_MENU_SCREEN; 

typedef struct _BOOT_MENU_SCROLL_BAR_CONTROL {
  BOOLEAN      HasScrollBar;
  UINTN        ItemCountPerScreen;
  UINTN        FirstItem;
  UINTN        LastItem;
} BOOT_MENU_SCROLL_BAR_CONTROL; 

typedef struct _BOOT_MENU_POPUP_DATA {
  EFI_STRING_ID                   TitleToken[TITLE_TOKEN_COUNT]; // Title string ID
  UINTN                           ItemCount;                     // Selectable item count
  EFI_STRING_ID                   *PtrTokens;                    // All of selectable items string ID
  EFI_STRING_ID                   HelpToken[HELP_TOKEN_COUNT];   // All of help string ID
  UINTN                           SelectItem;                    // Current select  item	
  BOOT_MENU_SCREEN                MenuScreen;                    // Boot menu screen information
  BOOT_MENU_SCROLL_BAR_CONTROL    ScrollBarControl;              // Boot menu scroll bar inoformation
} BOOT_MENU_POPUP_DATA;

#endif 

