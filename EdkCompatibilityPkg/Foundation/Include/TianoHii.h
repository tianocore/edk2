/*++

Copyright (c) 2007 - 2010, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

  TianoHii.h

Abstract:

  Tiano specific HII relative definition.

Revision History

--*/

#ifndef _TIANO_HII_H_
#define _TIANO_HII_H_

#include "EfiHii.h"

#define NARROW_CHAR         0xFFF0
#define WIDE_CHAR           0xFFF1
#define NON_BREAKING_CHAR   0xFFF2

#define GLYPH_WIDTH         EFI_GLYPH_WIDTH
#define GLYPH_HEIGHT        EFI_GLYPH_HEIGHT

//
// State defined for password statemachine 
//
#define BROWSER_STATE_VALIDATE_PASSWORD  0
#define BROWSER_STATE_SET_PASSWORD       1

//
// References to string tokens must use this macro to enable scanning for
// token usages.
//
#define STRING_TOKEN(t) t

//
// GUIDed opcodes defined for Tiano
//
#define EFI_IFR_TIANO_GUID \
  { 0xf0b1735, 0x87a0, 0x4193, {0xb2, 0x66, 0x53, 0x8c, 0x38, 0xaf, 0x48, 0xce} }
//
// ClassGuid for Front Page
//
#define EFI_HII_FRONT_PAGE_CLASS_GUID \
  { 0x94d411b7, 0x7669, 0x45c3, {0xba, 0x3b, 0xf3, 0xa5, 0x8a, 0x71, 0x56, 0x81} }

#pragma pack(1)

#define EFI_IFR_EXTEND_OP_LABEL       0x0
#define EFI_IFR_EXTEND_OP_BANNER      0x1
#define EFI_IFR_EXTEND_OP_TIMEOUT     0x2
#define EFI_IFR_EXTEND_OP_CLASS       0x3
#define EFI_IFR_EXTEND_OP_SUBCLASS    0x4

typedef struct _EFI_IFR_GUID_LABEL {
  EFI_IFR_OP_HEADER   Header;
  EFI_GUID            Guid;
  UINT8               ExtendOpCode;
  UINT16              Number;
} EFI_IFR_GUID_LABEL;

#define EFI_IFR_BANNER_ALIGN_LEFT     0
#define EFI_IFR_BANNER_ALIGN_CENTER   1
#define EFI_IFR_BANNER_ALIGN_RIGHT    2

typedef struct _EFI_IFR_GUID_BANNER {
  EFI_IFR_OP_HEADER   Header;
  EFI_GUID            Guid;
  UINT8               ExtendOpCode; // Extended opcode is EFI_IFR_EXTEND_OP_BANNER
  EFI_STRING_ID       Title;        // The string token for the banner title
  UINT16              LineNumber;   // 1-based line number
  UINT8               Alignment;    // left, center, or right-aligned
} EFI_IFR_GUID_BANNER;

typedef struct _EFI_IFR_GUID_TIMEOUT {
  EFI_IFR_OP_HEADER   Header;
  EFI_GUID            Guid;
  UINT8               ExtendOpCode;
  UINT16              TimeOut;
} EFI_IFR_GUID_TIMEOUT;

#define EFI_NON_DEVICE_CLASS              0x00
#define EFI_DISK_DEVICE_CLASS             0x01
#define EFI_VIDEO_DEVICE_CLASS            0x02
#define EFI_NETWORK_DEVICE_CLASS          0x04
#define EFI_INPUT_DEVICE_CLASS            0x08
#define EFI_ON_BOARD_DEVICE_CLASS         0x10
#define EFI_OTHER_DEVICE_CLASS            0x20

typedef struct _EFI_IFR_GUID_CLASS {
  EFI_IFR_OP_HEADER   Header;
  EFI_GUID            Guid;
  UINT8               ExtendOpCode;
  UINT16              Class;
} EFI_IFR_GUID_CLASS;

#define EFI_SETUP_APPLICATION_SUBCLASS    0x00
#define EFI_GENERAL_APPLICATION_SUBCLASS  0x01
#define EFI_FRONT_PAGE_SUBCLASS           0x02
#define EFI_SINGLE_USE_SUBCLASS           0x03

typedef struct _EFI_IFR_GUID_SUBCLASS {
  EFI_IFR_OP_HEADER   Header;
  EFI_GUID            Guid;
  UINT8               ExtendOpCode;
  UINT16              SubClass;
} EFI_IFR_GUID_SUBCLASS;

#pragma pack()

#endif
