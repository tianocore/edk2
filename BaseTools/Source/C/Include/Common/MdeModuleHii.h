/** @file
  EDK II specific HII relative definition.

  Copyright (c) 2006 - 2011, Intel Corporation. All rights reserved.<BR>

  This program and the accompanying materials are licensed and made available
  under the terms and conditions of the BSD License which accompanies this
  distribution.  The full text of the license may be found at:
    http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

  File Name: MdeModuleHii.h

**/

#ifndef _MDEMODULE_HII_H
#define _MDEMODULE_HII_H

#define NARROW_CHAR         0xFFF0
#define WIDE_CHAR           0xFFF1
#define NON_BREAKING_CHAR   0xFFF2

///
/// State defined for password statemachine .
///
#define BROWSER_STATE_VALIDATE_PASSWORD  0
#define BROWSER_STATE_SET_PASSWORD       1

///
/// GUIDed opcodes defined for EDKII implementation.
///
#define EFI_IFR_TIANO_GUID \
  { 0xf0b1735, 0x87a0, 0x4193, {0xb2, 0x66, 0x53, 0x8c, 0x38, 0xaf, 0x48, 0xce} }

#pragma pack(1)

///
/// EDKII implementation extension opcodes, new extension can be added here later.
///
#define EFI_IFR_EXTEND_OP_LABEL       0x0
#define EFI_IFR_EXTEND_OP_BANNER      0x1
#define EFI_IFR_EXTEND_OP_TIMEOUT     0x2
#define EFI_IFR_EXTEND_OP_CLASS       0x3
#define EFI_IFR_EXTEND_OP_SUBCLASS    0x4

///
/// Label opcode.
///
typedef struct _EFI_IFR_GUID_LABEL {
  EFI_IFR_OP_HEADER   Header;
  ///
  /// EFI_IFR_TIANO_GUID.
  ///
  EFI_GUID            Guid;
  ///
  /// EFI_IFR_EXTEND_OP_LABEL.
  ///
  UINT8               ExtendOpCode;
  ///
  /// Label Number.
  ///
  UINT16              Number;
} EFI_IFR_GUID_LABEL;

#define EFI_IFR_BANNER_ALIGN_LEFT     0
#define EFI_IFR_BANNER_ALIGN_CENTER   1
#define EFI_IFR_BANNER_ALIGN_RIGHT    2

///
/// Banner opcode.
///
typedef struct _EFI_IFR_GUID_BANNER {
  EFI_IFR_OP_HEADER   Header;
  ///
  /// EFI_IFR_TIANO_GUID.
  ///
  EFI_GUID            Guid;
  ///
  /// EFI_IFR_EXTEND_OP_BANNER
  ///
  UINT8               ExtendOpCode;
  EFI_STRING_ID       Title;        ///< The string token for the banner title.
  UINT16              LineNumber;   ///< 1-based line number.
  UINT8               Alignment;    ///< left, center, or right-aligned.
} EFI_IFR_GUID_BANNER;

///
/// Timeout opcode.
///
typedef struct _EFI_IFR_GUID_TIMEOUT {
  EFI_IFR_OP_HEADER   Header;
  ///
  /// EFI_IFR_TIANO_GUID.
  ///
  EFI_GUID            Guid;
  ///
  /// EFI_IFR_EXTEND_OP_TIMEOUT.
  ///
  UINT8               ExtendOpCode;
  UINT16              TimeOut;       ///< TimeOut Value.
} EFI_IFR_GUID_TIMEOUT;

#define EFI_NON_DEVICE_CLASS              0x00
#define EFI_DISK_DEVICE_CLASS             0x01
#define EFI_VIDEO_DEVICE_CLASS            0x02
#define EFI_NETWORK_DEVICE_CLASS          0x04
#define EFI_INPUT_DEVICE_CLASS            0x08
#define EFI_ON_BOARD_DEVICE_CLASS         0x10
#define EFI_OTHER_DEVICE_CLASS            0x20

///
/// Device Class opcode.
///
typedef struct _EFI_IFR_GUID_CLASS {
  EFI_IFR_OP_HEADER   Header;
  ///
  /// EFI_IFR_TIANO_GUID.
  ///
  EFI_GUID            Guid;
  ///
  /// EFI_IFR_EXTEND_OP_CLASS.
  ///
  UINT8               ExtendOpCode;
  UINT16              Class;           ///< Device Class from the above.
} EFI_IFR_GUID_CLASS;

#define EFI_SETUP_APPLICATION_SUBCLASS    0x00
#define EFI_GENERAL_APPLICATION_SUBCLASS  0x01
#define EFI_FRONT_PAGE_SUBCLASS           0x02
#define EFI_SINGLE_USE_SUBCLASS           0x03

///
/// SubClass opcode
///
typedef struct _EFI_IFR_GUID_SUBCLASS {
  EFI_IFR_OP_HEADER   Header;
  ///
  /// EFI_IFR_TIANO_GUID.
  ///
  EFI_GUID            Guid;
  ///
  /// EFI_IFR_EXTEND_OP_SUBCLASS.
  ///
  UINT8               ExtendOpCode;
  UINT16              SubClass;      ///< Sub Class type from the above.
} EFI_IFR_GUID_SUBCLASS;

///
/// GUIDed opcodes support for framework vfr.
///
#define EFI_IFR_FRAMEWORK_GUID \
  { 0x31ca5d1a, 0xd511, 0x4931, { 0xb7, 0x82, 0xae, 0x6b, 0x2b, 0x17, 0x8c, 0xd7 } }

///
/// Two extended opcodes are added, and new extensions can be added here later.
/// One is for framework OneOf question Option Key value;
/// another is for framework vareqval.
///
#define EFI_IFR_EXTEND_OP_OPTIONKEY   0x0
#define EFI_IFR_EXTEND_OP_VAREQNAME   0x1

///
/// Store the framework vfr option key value.
///
typedef struct _EFI_IFR_GUID_OPTIONKEY {
  EFI_IFR_OP_HEADER   Header;
  ///
  /// EFI_IFR_FRAMEWORK_GUID.
  ///
  EFI_GUID            Guid;
  ///
  /// EFI_IFR_EXTEND_OP_OPTIONKEY.
  ///
  UINT8               ExtendOpCode;
  ///
  /// OneOf Questiond ID binded by OneOf Option.
  ///
  EFI_QUESTION_ID     QuestionId;
  ///
  /// The OneOf Option Value.
  ///
  EFI_IFR_TYPE_VALUE  OptionValue;
  ///
  /// The Framework OneOf Option Key Value.
  ///
  UINT16              KeyValue;
} EFI_IFR_GUID_OPTIONKEY;

///
/// Store the framework vfr vareqval name number.
///
typedef struct _EFI_IFR_GUID_VAREQNAME {
  EFI_IFR_OP_HEADER   Header;
  ///
  /// EFI_IFR_FRAMEWORK_GUID.
  ///
  EFI_GUID            Guid;
  ///
  /// EFI_IFR_EXTEND_OP_VAREQNAME.
  ///
  UINT8               ExtendOpCode;
  ///
  /// Question ID of the Numeric Opcode created.
  ///
  EFI_QUESTION_ID     QuestionId;
  ///
  /// For vareqval (0x100), NameId is 0x100.
  /// This value will convert to a Unicode String following this rule;
  ///            sprintf(StringBuffer, "%d", NameId) .
  /// The the Unicode String will be used as a EFI Variable Name.
  ///
  UINT16              NameId;
} EFI_IFR_GUID_VAREQNAME;

#pragma pack()

extern EFI_GUID gEfiIfrTianoGuid;
extern EFI_GUID gEfiIfrFrameworkGuid;

#endif

