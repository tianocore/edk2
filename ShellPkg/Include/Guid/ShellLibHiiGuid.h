/** @file
  GUIDs for HII package list installed by Shell libraries.

  Copyright (c) 2011, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _SHELLLIB_HII_GUID_H_
#define _SHELLLIB_HII_GUID_H_

#define HANDLE_PARSING_HII_GUID \
  { \
  0xb8969637, 0x81de, 0x43af, { 0xbc, 0x9a, 0x24, 0xd9, 0x89, 0x13, 0xf2, 0xf6 } \
  }

#define SHELL_DEBUG1_HII_GUID \
  { \
    0x25f200aa, 0xd3cb, 0x470a, { 0xbf, 0x51, 0xe7, 0xd1, 0x62, 0xd2, 0x2e, 0x6f } \
  }

#define SHELL_DRIVER1_HII_GUID \
  { \
  0xaf0b742, 0x63ec, 0x45bd, {0x8d, 0xb6, 0x71, 0xad, 0x7f, 0x2f, 0xe8, 0xe8} \
  }

#define SHELL_INSTALL1_HII_GUID \
  { \
    0x7d574d54, 0xd364, 0x4d4a, { 0x95, 0xe3, 0x49, 0x45, 0xdb, 0x7a, 0xd3, 0xee } \
  }

#define SHELL_LEVEL1_HII_GUID \
  { \
    0xdec5daa4, 0x6781, 0x4820, { 0x9c, 0x63, 0xa7, 0xb0, 0xe4, 0xf1, 0xdb, 0x31 } \
  }

#define SHELL_LEVEL2_HII_GUID \
  { \
    0xf95a7ccc, 0x4c55, 0x4426, { 0xa7, 0xb4, 0xdc, 0x89, 0x61, 0x95, 0xb, 0xae } \
  }

#define SHELL_LEVEL3_HII_GUID \
  { \
    0x4344558d, 0x4ef9, 0x4725, { 0xb1, 0xe4, 0x33, 0x76, 0xe8, 0xd6, 0x97, 0x4f } \
  }

#define SHELL_NETWORK1_HII_GUID \
  { \
    0xf3d301bb, 0xf4a5, 0x45a8, { 0xb0, 0xb7, 0xfa, 0x99, 0x9c, 0x62, 0x37, 0xae } \
  }

extern EFI_GUID gHandleParsingHiiGuid;
extern EFI_GUID gShellDebug1HiiGuid;
extern EFI_GUID gShellDriver1HiiGuid;
extern EFI_GUID gShellInstall1HiiGuid;
extern EFI_GUID gShellLevel1HiiGuid;
extern EFI_GUID gShellLevel2HiiGuid;
extern EFI_GUID gShellLevel3HiiGuid;
extern EFI_GUID gShellNetwork1HiiGuid;

#endif
