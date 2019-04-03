/** @file
Example code to register customized keyboard layout to HII database.

Copyright (c) 2013-2015 Intel Corporation.

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "CommonHeader.h"

//
// Define "`" as Non-spacing key to switch "a","u","i","o","e"
//
#define ENABLE_NS_KEY           1

#ifdef ENABLE_NS_KEY
#define KEYBOARD_KEY_COUNT       (104 + 5)
#else
#define KEYBOARD_KEY_COUNT       104
#endif

#pragma pack (1)
typedef struct {
  //
  // This 4-bytes total array length is required by PreparePackageList()
  //
  UINT32                 Length;

  //
  // Keyboard Layout package definition
  //
  EFI_HII_PACKAGE_HEADER PackageHeader;
  UINT16                 LayoutCount;

  //
  // EFI_HII_KEYBOARD_LAYOUT
  //
  UINT16                 LayoutLength;
  EFI_GUID               Guid;
  UINT32                 LayoutDescriptorStringOffset;
  UINT8                  DescriptorCount;
  EFI_KEY_DESCRIPTOR     KeyDescriptor[KEYBOARD_KEY_COUNT];
  UINT16                 DescriptionCount;      // EFI_DESCRIPTION_STRING_BUNDLE
  CHAR16                 Language[5];        // RFC4646 Language Code: "en-US"
  CHAR16                 Space;
  CHAR16                 DescriptionString[17]; // Description: "English Keyboard"
} KEYBOARD_LAYOUT_PACK_BIN;
#pragma pack()

#define KEYBOARD_LAYOUT_PACKAGE_GUID \
  { \
    0xd66f7b7a, 0x5e06, 0x49f3, { 0xa1, 0xcf, 0x12, 0x8d, 0x4, 0x86, 0xc2, 0x7c } \
  }

#define KEYBOARD_LAYOUT_KEY_GUID \
  { \
    0xd9db96f4, 0xff47, 0x4eb6, { 0x8a, 0x4, 0x79, 0x5b, 0x56, 0x87, 0xb, 0x4e } \
  }

EFI_GUID  mKeyboardLayoutPackageGuid = KEYBOARD_LAYOUT_PACKAGE_GUID;
EFI_GUID  mKeyboardLayoutKeyGuid = KEYBOARD_LAYOUT_KEY_GUID;

KEYBOARD_LAYOUT_PACK_BIN  mKeyboardLayoutBin = {
  sizeof (KEYBOARD_LAYOUT_PACK_BIN),   // Binary size

  //
  // EFI_HII_PACKAGE_HEADER
  //
  {
    sizeof (KEYBOARD_LAYOUT_PACK_BIN) - sizeof (UINT32),
    EFI_HII_PACKAGE_KEYBOARD_LAYOUT
  },
  1,  // LayoutCount
  sizeof (KEYBOARD_LAYOUT_PACK_BIN) - sizeof (UINT32) - sizeof (EFI_HII_PACKAGE_HEADER) - sizeof (UINT16), // LayoutLength
  KEYBOARD_LAYOUT_KEY_GUID,  // KeyGuid
  sizeof (UINT16) + sizeof (EFI_GUID) + sizeof (UINT32) + sizeof (UINT8) + (KEYBOARD_KEY_COUNT * sizeof (EFI_KEY_DESCRIPTOR)), // LayoutDescriptorStringOffset
  KEYBOARD_KEY_COUNT, // DescriptorCount
  {
    //
    // EFI_KEY_DESCRIPTOR
    //
    {EfiKeyC1,         'a',      'A',  0, 0,  EFI_NULL_MODIFIER,   EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_CAPS_LOCK},
    {EfiKeyB5,         'b',      'B',  0, 0,  EFI_NULL_MODIFIER,   EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_CAPS_LOCK},
    {EfiKeyB3,         'c',      'C',  0, 0,  EFI_NULL_MODIFIER,   EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_CAPS_LOCK},
    {EfiKeyC3,         'd',      'D',  0, 0,  EFI_NULL_MODIFIER,   EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_CAPS_LOCK},
    {EfiKeyD3,         'e',      'E',  0, 0,  EFI_NULL_MODIFIER,   EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_CAPS_LOCK},
    {EfiKeyC4,         'f',      'F',  0, 0,  EFI_NULL_MODIFIER,   EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_CAPS_LOCK},
    {EfiKeyC5,         'g',      'G',  0, 0,  EFI_NULL_MODIFIER,   EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_CAPS_LOCK},
    {EfiKeyC6,         'h',      'H',  0, 0,  EFI_NULL_MODIFIER,   EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_CAPS_LOCK},
    {EfiKeyD8,         'i',      'I',  0, 0,  EFI_NULL_MODIFIER,   EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_CAPS_LOCK},
    {EfiKeyC7,         'j',      'J',  0, 0,  EFI_NULL_MODIFIER,   EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_CAPS_LOCK},
    {EfiKeyC8,         'k',      'K',  0, 0,  EFI_NULL_MODIFIER,   EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_CAPS_LOCK},
    {EfiKeyC9,         'l',      'L',  0, 0,  EFI_NULL_MODIFIER,   EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_CAPS_LOCK},
    {EfiKeyB7,         'm',      'M',  0, 0,  EFI_NULL_MODIFIER,   EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_CAPS_LOCK},
    {EfiKeyB6,         'n',      'N',  0, 0,  EFI_NULL_MODIFIER,   EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_CAPS_LOCK},
    {EfiKeyD9,         'o',      'O',  0, 0,  EFI_NULL_MODIFIER,   EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_CAPS_LOCK},
    {EfiKeyD10,        'p',      'P',  0, 0,  EFI_NULL_MODIFIER,   EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_CAPS_LOCK},
    {EfiKeyD1,         'q',      'Q',  0, 0,  EFI_NULL_MODIFIER,   EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_CAPS_LOCK},
    {EfiKeyD4,         'r',      'R',  0, 0,  EFI_NULL_MODIFIER,   EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_CAPS_LOCK},
    {EfiKeyC2,         's',      'S',  0, 0,  EFI_NULL_MODIFIER,   EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_CAPS_LOCK},
    {EfiKeyD5,         't',      'T',  0, 0,  EFI_NULL_MODIFIER,   EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_CAPS_LOCK},
    {EfiKeyD7,         'u',      'U',  0, 0,  EFI_NULL_MODIFIER,   EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_CAPS_LOCK},
    {EfiKeyB4,         'v',      'V',  0, 0,  EFI_NULL_MODIFIER,   EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_CAPS_LOCK},
    {EfiKeyD2,         'w',      'W',  0, 0,  EFI_NULL_MODIFIER,   EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_CAPS_LOCK},
    {EfiKeyB2,         'x',      'X',  0, 0,  EFI_NULL_MODIFIER,   EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_CAPS_LOCK},
    {EfiKeyD6,         'y',      'Y',  0, 0,  EFI_NULL_MODIFIER,   EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_CAPS_LOCK},
    {EfiKeyB1,         'z',      'Z',  0, 0,  EFI_NULL_MODIFIER,   EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_CAPS_LOCK},
    {EfiKeyE1,         '1',      '!',  0, 0,  EFI_NULL_MODIFIER,   EFI_AFFECTED_BY_STANDARD_SHIFT},
    {EfiKeyE2,         '2',      '@',  0, 0,  EFI_NULL_MODIFIER,   EFI_AFFECTED_BY_STANDARD_SHIFT},
    {EfiKeyE3,         '3',      '#',  0, 0,  EFI_NULL_MODIFIER,   EFI_AFFECTED_BY_STANDARD_SHIFT},
    {EfiKeyE4,         '4',      '$',  0, 0,  EFI_NULL_MODIFIER,   EFI_AFFECTED_BY_STANDARD_SHIFT},
    {EfiKeyE5,         '5',      '%',  0, 0,  EFI_NULL_MODIFIER,   EFI_AFFECTED_BY_STANDARD_SHIFT},
    {EfiKeyE6,         '6',      '^',  0, 0,  EFI_NULL_MODIFIER,   EFI_AFFECTED_BY_STANDARD_SHIFT},
    {EfiKeyE7,         '7',      '&',  0, 0,  EFI_NULL_MODIFIER,   EFI_AFFECTED_BY_STANDARD_SHIFT},
    {EfiKeyE8,         '8',      '*',  0, 0,  EFI_NULL_MODIFIER,   EFI_AFFECTED_BY_STANDARD_SHIFT},
    {EfiKeyE9,         '9',      '(',  0, 0,  EFI_NULL_MODIFIER,   EFI_AFFECTED_BY_STANDARD_SHIFT},
    {EfiKeyE10,        '0',      ')',  0, 0,  EFI_NULL_MODIFIER,   EFI_AFFECTED_BY_STANDARD_SHIFT},
    {EfiKeyEnter,      0x0d,     0x0d, 0, 0,  EFI_NULL_MODIFIER,   0},
    {EfiKeyEsc,        0x1b,     0x1b, 0, 0,  EFI_NULL_MODIFIER,   0},
    {EfiKeyBackSpace,  0x08,     0x08, 0, 0,  EFI_NULL_MODIFIER,   0},
    {EfiKeyTab,        0x09,     0x09, 0, 0,  EFI_NULL_MODIFIER,   0},
    {EfiKeySpaceBar,   ' ',      ' ',  0, 0,  EFI_NULL_MODIFIER,   0},
    {EfiKeyE11,        '-',      '_',  0, 0,  EFI_NULL_MODIFIER,   EFI_AFFECTED_BY_STANDARD_SHIFT},
    {EfiKeyE12,        '=',      '+',  0, 0,  EFI_NULL_MODIFIER,   EFI_AFFECTED_BY_STANDARD_SHIFT},
    {EfiKeyD11,        '[',      '{',  0, 0,  EFI_NULL_MODIFIER,   EFI_AFFECTED_BY_STANDARD_SHIFT},
    {EfiKeyD12,        ']',      '}',  0, 0,  EFI_NULL_MODIFIER,   EFI_AFFECTED_BY_STANDARD_SHIFT},
    {EfiKeyD13,        '\\',     '|',  0, 0,  EFI_NULL_MODIFIER,   EFI_AFFECTED_BY_STANDARD_SHIFT},
    {EfiKeyC10,        ';',      ':',  0, 0,  EFI_NULL_MODIFIER,   EFI_AFFECTED_BY_STANDARD_SHIFT},
    {EfiKeyC11,        '\'',     '"',  0, 0,  EFI_NULL_MODIFIER,   EFI_AFFECTED_BY_STANDARD_SHIFT},

#ifdef ENABLE_NS_KEY
    //
    // Non-Spacing key example
    //
    {EfiKeyE0,         0,      0,      0, 0, EFI_NS_KEY_MODIFIER, 0},
    {EfiKeyC1,         0x00E2, 0x00C2, 0, 0, EFI_NS_KEY_DEPENDENCY_MODIFIER, EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_CAPS_LOCK},
    {EfiKeyD3,         0x00EA, 0x00CA, 0, 0, EFI_NS_KEY_DEPENDENCY_MODIFIER, EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_CAPS_LOCK},
    {EfiKeyD8,         0x00EC, 0x00CC, 0, 0, EFI_NS_KEY_DEPENDENCY_MODIFIER, EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_CAPS_LOCK},
    {EfiKeyD9,         0x00F4, 0x00D4, 0, 0, EFI_NS_KEY_DEPENDENCY_MODIFIER, EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_CAPS_LOCK},
    {EfiKeyD7,         0x00FB, 0x00CB, 0, 0, EFI_NS_KEY_DEPENDENCY_MODIFIER, EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_CAPS_LOCK},
#else
    {EfiKeyE0,         '`',      '~',  0, 0,  EFI_NULL_MODIFIER,   EFI_AFFECTED_BY_STANDARD_SHIFT},
#endif

    {EfiKeyB8,         ',',      '<',  0, 0,  EFI_NULL_MODIFIER,   EFI_AFFECTED_BY_STANDARD_SHIFT},
    {EfiKeyB9,         '.',      '>',  0, 0,  EFI_NULL_MODIFIER,   EFI_AFFECTED_BY_STANDARD_SHIFT},
    {EfiKeyB10,        '/',      '?',  0, 0,  EFI_NULL_MODIFIER,   EFI_AFFECTED_BY_STANDARD_SHIFT},
    {EfiKeyCapsLock,   0x00,     0x00, 0, 0,  EFI_CAPS_LOCK_MODIFIER,            0},
    {EfiKeyF1,         0x00,     0x00, 0, 0,  EFI_FUNCTION_KEY_ONE_MODIFIER,     0},
    {EfiKeyF2,         0x00,     0x00, 0, 0,  EFI_FUNCTION_KEY_TWO_MODIFIER,     0},
    {EfiKeyF3,         0x00,     0x00, 0, 0,  EFI_FUNCTION_KEY_THREE_MODIFIER,   0},
    {EfiKeyF4,         0x00,     0x00, 0, 0,  EFI_FUNCTION_KEY_FOUR_MODIFIER,    0},
    {EfiKeyF5,         0x00,     0x00, 0, 0,  EFI_FUNCTION_KEY_FIVE_MODIFIER,    0},
    {EfiKeyF6,         0x00,     0x00, 0, 0,  EFI_FUNCTION_KEY_SIX_MODIFIER,     0},
    {EfiKeyF7,         0x00,     0x00, 0, 0,  EFI_FUNCTION_KEY_SEVEN_MODIFIER,   0},
    {EfiKeyF8,         0x00,     0x00, 0, 0,  EFI_FUNCTION_KEY_EIGHT_MODIFIER,   0},
    {EfiKeyF9,         0x00,     0x00, 0, 0,  EFI_FUNCTION_KEY_NINE_MODIFIER,    0},
    {EfiKeyF10,        0x00,     0x00, 0, 0,  EFI_FUNCTION_KEY_TEN_MODIFIER,     0},
    {EfiKeyF11,        0x00,     0x00, 0, 0,  EFI_FUNCTION_KEY_ELEVEN_MODIFIER,  0},
    {EfiKeyF12,        0x00,     0x00, 0, 0,  EFI_FUNCTION_KEY_TWELVE_MODIFIER,  0},
    {EfiKeyPrint,      0x00,     0x00, 0, 0,  EFI_PRINT_MODIFIER,                0},
    {EfiKeySLck,       0x00,     0x00, 0, 0,  EFI_SCROLL_LOCK_MODIFIER,          0},
    {EfiKeyPause,      0x00,     0x00, 0, 0,  EFI_PAUSE_MODIFIER,                0},
    {EfiKeyIns,        0x00,     0x00, 0, 0,  EFI_INSERT_MODIFIER,               0},
    {EfiKeyHome,       0x00,     0x00, 0, 0,  EFI_HOME_MODIFIER,                 0},
    {EfiKeyPgUp,       0x00,     0x00, 0, 0,  EFI_PAGE_UP_MODIFIER,              0},
    {EfiKeyDel,        0x00,     0x00, 0, 0,  EFI_DELETE_MODIFIER,               0},
    {EfiKeyEnd,        0x00,     0x00, 0, 0,  EFI_END_MODIFIER,                  0},
    {EfiKeyPgDn,       0x00,     0x00, 0, 0,  EFI_PAGE_DOWN_MODIFIER,            0},
    {EfiKeyRightArrow, 0x00,     0x00, 0, 0,  EFI_RIGHT_ARROW_MODIFIER,          0},
    {EfiKeyLeftArrow,  0x00,     0x00, 0, 0,  EFI_LEFT_ARROW_MODIFIER,           0},
    {EfiKeyDownArrow,  0x00,     0x00, 0, 0,  EFI_DOWN_ARROW_MODIFIER,           0},
    {EfiKeyUpArrow,    0x00,     0x00, 0, 0,  EFI_UP_ARROW_MODIFIER,             0},
    {EfiKeyNLck,       0x00,     0x00, 0, 0,  EFI_NUM_LOCK_MODIFIER,             0},
    {EfiKeySlash,      '/',      '/',  0, 0,  EFI_NULL_MODIFIER,                 0},
    {EfiKeyAsterisk,   '*',      '*',  0, 0,  EFI_NULL_MODIFIER,                 0},
    {EfiKeyMinus,      '-',      '-',  0, 0,  EFI_NULL_MODIFIER,                 0},
    {EfiKeyPlus,       '+',      '+',  0, 0,  EFI_NULL_MODIFIER,                 0},
    {EfiKeyEnter,      0x0d,     0x0d, 0, 0,  EFI_NULL_MODIFIER,                 0},
    {EfiKeyOne,        '1',      '1',  0, 0,  EFI_END_MODIFIER,         EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_NUM_LOCK},
    {EfiKeyTwo,        '2',      '2',  0, 0,  EFI_DOWN_ARROW_MODIFIER,  EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_NUM_LOCK},
    {EfiKeyThree,      '3',      '3',  0, 0,  EFI_PAGE_DOWN_MODIFIER,   EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_NUM_LOCK},
    {EfiKeyFour,       '4',      '4',  0, 0,  EFI_LEFT_ARROW_MODIFIER,  EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_NUM_LOCK},
    {EfiKeyFive,       '5',      '5',  0, 0,  EFI_NULL_MODIFIER,        EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_NUM_LOCK},
    {EfiKeySix,        '6',      '6',  0, 0,  EFI_RIGHT_ARROW_MODIFIER, EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_NUM_LOCK},
    {EfiKeySeven,      '7',      '7',  0, 0,  EFI_HOME_MODIFIER,        EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_NUM_LOCK},
    {EfiKeyEight,      '8',      '8',  0, 0,  EFI_UP_ARROW_MODIFIER,    EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_NUM_LOCK},
    {EfiKeyNine,       '9',      '9',  0, 0,  EFI_PAGE_UP_MODIFIER,     EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_NUM_LOCK},
    {EfiKeyZero,       '0',      '0',  0, 0,  EFI_INSERT_MODIFIER,      EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_NUM_LOCK},
    {EfiKeyPeriod,     '.',      '.',  0, 0,  EFI_DELETE_MODIFIER,      EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_NUM_LOCK},
    {EfiKeyA4,         0x00,     0x00, 0, 0,  EFI_MENU_MODIFIER,            0},
    {EfiKeyLCtrl,      0,        0,    0, 0,  EFI_LEFT_CONTROL_MODIFIER,    0},
    {EfiKeyLShift,     0,        0,    0, 0,  EFI_LEFT_SHIFT_MODIFIER,      0},
    {EfiKeyLAlt,       0,        0,    0, 0,  EFI_LEFT_ALT_MODIFIER,        0},
    {EfiKeyA0,         0,        0,    0, 0,  EFI_LEFT_LOGO_MODIFIER,       0},
    {EfiKeyRCtrl,      0,        0,    0, 0,  EFI_RIGHT_CONTROL_MODIFIER,   0},
    {EfiKeyRShift,     0,        0,    0, 0,  EFI_RIGHT_SHIFT_MODIFIER,     0},
    {EfiKeyA2,         0,        0,    0, 0,  EFI_RIGHT_ALT_MODIFIER,       0},
    {EfiKeyA3,         0,        0,    0, 0,  EFI_RIGHT_LOGO_MODIFIER,      0},
  },
  1,                          // DescriptionCount
  {'e', 'n', '-', 'U', 'S'},  // RFC4646 language code
  ' ',                        // Space
  {'E', 'n', 'g', 'l', 'i', 's', 'h', ' ', 'K', 'e', 'y', 'b', 'o', 'a', 'r', 'd', '\0'}, // DescriptionString[17]
};

extern EFI_HANDLE mImageHandle;

EFI_STATUS
InitKeyboardLayout (
  VOID
  )
/*++

  Routine Description:
    Install keyboard layout package and set current keyboard layout.

  Arguments:
    None.

  Returns:
    EFI_STATUS

--*/
{
  EFI_STATUS                   Status;
  EFI_HII_DATABASE_PROTOCOL    *HiiDatabase;
  EFI_HII_HANDLE               HiiHandle;

  //
  // Locate Hii database protocol
  //
  Status = gBS->LocateProtocol (
                  &gEfiHiiDatabaseProtocolGuid,
                  NULL,
                  (VOID**)&HiiDatabase
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Install Keyboard Layout package to HII database
  //
  HiiHandle = HiiAddPackages (
                &mKeyboardLayoutPackageGuid,
                mImageHandle,
                &mKeyboardLayoutBin,
                NULL
                );
  if (HiiHandle == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Set current keyboard layout
  //
  Status = HiiDatabase->SetKeyboardLayout (HiiDatabase, &mKeyboardLayoutKeyGuid);

  return Status;
}
