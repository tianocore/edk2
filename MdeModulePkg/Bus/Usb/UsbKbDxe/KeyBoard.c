/** @file

  Helper functions for USB Keyboard Driver.

Copyright (c) 2004 - 2008, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "KeyBoard.h"
#include <Library/UsbLib.h>

//
// Static English keyboard layout
// Format:<efi key>, <unicode without shift>, <unicode with shift>, <Modifier>, <AffectedAttribute>
//
STATIC
UINT8 KeyboardLayoutTable[USB_KEYCODE_MAX_MAKE + 8][5] = {
  {EfiKeyC1,         'a',      'A',   EFI_NULL_MODIFIER,   EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_CAPS_LOCK},   // 0x04
  {EfiKeyB5,         'b',      'B',   EFI_NULL_MODIFIER,   EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_CAPS_LOCK},   // 0x05
  {EfiKeyB3,         'c',      'C',   EFI_NULL_MODIFIER,   EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_CAPS_LOCK},   // 0x06
  {EfiKeyC3,         'd',      'D',   EFI_NULL_MODIFIER,   EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_CAPS_LOCK},   // 0x07
  {EfiKeyD3,         'e',      'E',   EFI_NULL_MODIFIER,   EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_CAPS_LOCK},   // 0x08
  {EfiKeyC4,         'f',      'F',   EFI_NULL_MODIFIER,   EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_CAPS_LOCK},   // 0x09
  {EfiKeyC5,         'g',      'G',   EFI_NULL_MODIFIER,   EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_CAPS_LOCK},   // 0x0A
  {EfiKeyC6,         'h',      'H',   EFI_NULL_MODIFIER,   EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_CAPS_LOCK},   // 0x0B
  {EfiKeyD8,         'i',      'I',   EFI_NULL_MODIFIER,   EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_CAPS_LOCK},   // 0x0C
  {EfiKeyC7,         'j',      'J',   EFI_NULL_MODIFIER,   EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_CAPS_LOCK},   // 0x0D
  {EfiKeyC8,         'k',      'K',   EFI_NULL_MODIFIER,   EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_CAPS_LOCK},   // 0x0E
  {EfiKeyC9,         'l',      'L',   EFI_NULL_MODIFIER,   EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_CAPS_LOCK},   // 0x0F
  {EfiKeyB7,         'm',      'M',   EFI_NULL_MODIFIER,   EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_CAPS_LOCK},   // 0x10
  {EfiKeyB6,         'n',      'N',   EFI_NULL_MODIFIER,   EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_CAPS_LOCK},   // 0x11
  {EfiKeyD9,         'o',      'O',   EFI_NULL_MODIFIER,   EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_CAPS_LOCK},   // 0x12
  {EfiKeyD10,        'p',      'P',   EFI_NULL_MODIFIER,   EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_CAPS_LOCK},   // 0x13
  {EfiKeyD1,         'q',      'Q',   EFI_NULL_MODIFIER,   EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_CAPS_LOCK},   // 0x14
  {EfiKeyD4,         'r',      'R',   EFI_NULL_MODIFIER,   EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_CAPS_LOCK},   // 0x15
  {EfiKeyC2,         's',      'S',   EFI_NULL_MODIFIER,   EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_CAPS_LOCK},   // 0x16
  {EfiKeyD5,         't',      'T',   EFI_NULL_MODIFIER,   EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_CAPS_LOCK},   // 0x17
  {EfiKeyD7,         'u',      'U',   EFI_NULL_MODIFIER,   EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_CAPS_LOCK},   // 0x18
  {EfiKeyB4,         'v',      'V',   EFI_NULL_MODIFIER,   EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_CAPS_LOCK},   // 0x19
  {EfiKeyD2,         'w',      'W',   EFI_NULL_MODIFIER,   EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_CAPS_LOCK},   // 0x1A
  {EfiKeyB2,         'x',      'X',   EFI_NULL_MODIFIER,   EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_CAPS_LOCK},   // 0x1B
  {EfiKeyD6,         'y',      'Y',   EFI_NULL_MODIFIER,   EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_CAPS_LOCK},   // 0x1C
  {EfiKeyB1,         'z',      'Z',   EFI_NULL_MODIFIER,   EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_CAPS_LOCK},   // 0x1D
  {EfiKeyE1,         '1',      '!',   EFI_NULL_MODIFIER,   EFI_AFFECTED_BY_STANDARD_SHIFT},   // 0x1E
  {EfiKeyE2,         '2',      '@',   EFI_NULL_MODIFIER,   EFI_AFFECTED_BY_STANDARD_SHIFT},   // 0x1F
  {EfiKeyE3,         '3',      '#',   EFI_NULL_MODIFIER,   EFI_AFFECTED_BY_STANDARD_SHIFT},   // 0x20
  {EfiKeyE4,         '4',      '$',   EFI_NULL_MODIFIER,   EFI_AFFECTED_BY_STANDARD_SHIFT},   // 0x21
  {EfiKeyE5,         '5',      '%',   EFI_NULL_MODIFIER,   EFI_AFFECTED_BY_STANDARD_SHIFT},   // 0x22
  {EfiKeyE6,         '6',      '^',   EFI_NULL_MODIFIER,   EFI_AFFECTED_BY_STANDARD_SHIFT},   // 0x23
  {EfiKeyE7,         '7',      '&',   EFI_NULL_MODIFIER,   EFI_AFFECTED_BY_STANDARD_SHIFT},   // 0x24
  {EfiKeyE8,         '8',      '*',   EFI_NULL_MODIFIER,   EFI_AFFECTED_BY_STANDARD_SHIFT},   // 0x25
  {EfiKeyE9,         '9',      '(',   EFI_NULL_MODIFIER,   EFI_AFFECTED_BY_STANDARD_SHIFT},   // 0x26
  {EfiKeyE10,        '0',      ')',   EFI_NULL_MODIFIER,   EFI_AFFECTED_BY_STANDARD_SHIFT},   // 0x27
  {EfiKeyEnter,      0x0d,     0x0d,  EFI_NULL_MODIFIER,   0},                                // 0x28   Enter
  {EfiKeyEsc,        0x1b,     0x1b,  EFI_NULL_MODIFIER,   0},                                // 0x29   Esc
  {EfiKeyBackSpace,  0x08,     0x08,  EFI_NULL_MODIFIER,   0},                                // 0x2A   Backspace
  {EfiKeyTab,        0x09,     0x09,  EFI_NULL_MODIFIER,   0},                                // 0x2B   Tab
  {EfiKeySpaceBar,   ' ',      ' ',   EFI_NULL_MODIFIER,   0},                                // 0x2C   Spacebar
  {EfiKeyE11,        '-',      '_',   EFI_NULL_MODIFIER,   EFI_AFFECTED_BY_STANDARD_SHIFT},   // 0x2D
  {EfiKeyE12,        '=',      '+',   EFI_NULL_MODIFIER,   EFI_AFFECTED_BY_STANDARD_SHIFT},   // 0x2E
  {EfiKeyD11,        '[',      '{',   EFI_NULL_MODIFIER,   EFI_AFFECTED_BY_STANDARD_SHIFT},   // 0x2F
  {EfiKeyD12,        ']',      '}',   EFI_NULL_MODIFIER,   EFI_AFFECTED_BY_STANDARD_SHIFT},   // 0x30
  {EfiKeyD13,        '\\',     '|',   EFI_NULL_MODIFIER,   EFI_AFFECTED_BY_STANDARD_SHIFT},   // 0x31
  {EfiKeyC12,        '\\',     '|',   EFI_NULL_MODIFIER,   EFI_AFFECTED_BY_STANDARD_SHIFT},   // 0x32  Keyboard Non-US # and ~
  {EfiKeyC10,        ';',      ':',   EFI_NULL_MODIFIER,   EFI_AFFECTED_BY_STANDARD_SHIFT},   // 0x33
  {EfiKeyC11,        '\'',     '"',   EFI_NULL_MODIFIER,   EFI_AFFECTED_BY_STANDARD_SHIFT},   // 0x34
  {EfiKeyE0,         '`',      '~',   EFI_NULL_MODIFIER,   EFI_AFFECTED_BY_STANDARD_SHIFT},   // 0x35  Keyboard Grave Accent and Tlide
  {EfiKeyB8,         ',',      '<',   EFI_NULL_MODIFIER,   EFI_AFFECTED_BY_STANDARD_SHIFT},   // 0x36
  {EfiKeyB9,         '.',      '>',   EFI_NULL_MODIFIER,   EFI_AFFECTED_BY_STANDARD_SHIFT},   // 0x37
  {EfiKeyB10,        '/',      '?',   EFI_NULL_MODIFIER,   EFI_AFFECTED_BY_STANDARD_SHIFT},   // 0x38
  {EfiKeyCapsLock,   0x00,     0x00,  EFI_CAPS_LOCK_MODIFIER,            0},                  // 0x39   CapsLock
  {EfiKeyF1,         0x00,     0x00,  EFI_FUNCTION_KEY_ONE_MODIFIER,     0},                  // 0x3A
  {EfiKeyF2,         0x00,     0x00,  EFI_FUNCTION_KEY_TWO_MODIFIER,     0},                  // 0x3B
  {EfiKeyF3,         0x00,     0x00,  EFI_FUNCTION_KEY_THREE_MODIFIER,   0},                  // 0x3C
  {EfiKeyF4,         0x00,     0x00,  EFI_FUNCTION_KEY_FOUR_MODIFIER,    0},                  // 0x3D
  {EfiKeyF5,         0x00,     0x00,  EFI_FUNCTION_KEY_FIVE_MODIFIER,    0},                  // 0x3E
  {EfiKeyF6,         0x00,     0x00,  EFI_FUNCTION_KEY_SIX_MODIFIER,     0},                  // 0x3F
  {EfiKeyF7,         0x00,     0x00,  EFI_FUNCTION_KEY_SEVEN_MODIFIER,   0},                  // 0x40
  {EfiKeyF8,         0x00,     0x00,  EFI_FUNCTION_KEY_EIGHT_MODIFIER,   0},                  // 0x41
  {EfiKeyF9,         0x00,     0x00,  EFI_FUNCTION_KEY_NINE_MODIFIER,    0},                  // 0x42
  {EfiKeyF10,        0x00,     0x00,  EFI_FUNCTION_KEY_TEN_MODIFIER,     0},                  // 0x43
  {EfiKeyF11,        0x00,     0x00,  EFI_FUNCTION_KEY_ELEVEN_MODIFIER,  0},                  // 0x44   F11
  {EfiKeyF12,        0x00,     0x00,  EFI_FUNCTION_KEY_TWELVE_MODIFIER,  0},                  // 0x45   F12
  {EfiKeyPrint,      0x00,     0x00,  EFI_PRINT_MODIFIER,                0},                  // 0x46   PrintScreen
  {EfiKeySLck,       0x00,     0x00,  EFI_SCROLL_LOCK_MODIFIER,          0},                  // 0x47   Scroll Lock
  {EfiKeyPause,      0x00,     0x00,  EFI_PAUSE_MODIFIER,                0},                  // 0x48   Pause
  {EfiKeyIns,        0x00,     0x00,  EFI_INSERT_MODIFIER,               0},                  // 0x49
  {EfiKeyHome,       0x00,     0x00,  EFI_HOME_MODIFIER,                 0},                  // 0x4A
  {EfiKeyPgUp,       0x00,     0x00,  EFI_PAGE_UP_MODIFIER,              0},                  // 0x4B
  {EfiKeyDel,        0x00,     0x00,  EFI_DELETE_MODIFIER,               0},                  // 0x4C
  {EfiKeyEnd,        0x00,     0x00,  EFI_END_MODIFIER,                  0},                  // 0x4D
  {EfiKeyPgDn,       0x00,     0x00,  EFI_PAGE_DOWN_MODIFIER,            0},                  // 0x4E
  {EfiKeyRightArrow, 0x00,     0x00,  EFI_RIGHT_ARROW_MODIFIER,          0},                  // 0x4F
  {EfiKeyLeftArrow,  0x00,     0x00,  EFI_LEFT_ARROW_MODIFIER,           0},                  // 0x50
  {EfiKeyDownArrow,  0x00,     0x00,  EFI_DOWN_ARROW_MODIFIER,           0},                  // 0x51
  {EfiKeyUpArrow,    0x00,     0x00,  EFI_UP_ARROW_MODIFIER,             0},                  // 0x52
  {EfiKeyNLck,       0x00,     0x00,  EFI_NUM_LOCK_MODIFIER,             0},                  // 0x53   NumLock
  {EfiKeySlash,      '/',      '/',   EFI_NULL_MODIFIER,                 0},                  // 0x54
  {EfiKeyAsterisk,   '*',      '*',   EFI_NULL_MODIFIER,                 0},                  // 0x55
  {EfiKeyMinus,      '-',      '-',   EFI_NULL_MODIFIER,                 0},                  // 0x56
  {EfiKeyPlus,       '+',      '+',   EFI_NULL_MODIFIER,                 0},                  // 0x57
  {EfiKeyEnter,      0x0d,     0x0d,  EFI_NULL_MODIFIER,                 0},                  // 0x58
  {EfiKeyOne,        '1',      '1',   EFI_END_MODIFIER,         EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_NUM_LOCK},   // 0x59
  {EfiKeyTwo,        '2',      '2',   EFI_DOWN_ARROW_MODIFIER,  EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_NUM_LOCK},   // 0x5A
  {EfiKeyThree,      '3',      '3',   EFI_PAGE_DOWN_MODIFIER,   EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_NUM_LOCK},   // 0x5B
  {EfiKeyFour,       '4',      '4',   EFI_LEFT_ARROW_MODIFIER,  EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_NUM_LOCK},   // 0x5C
  {EfiKeyFive,       '5',      '5',   EFI_NULL_MODIFIER,        EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_NUM_LOCK},   // 0x5D
  {EfiKeySix,        '6',      '6',   EFI_RIGHT_ARROW_MODIFIER, EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_NUM_LOCK},   // 0x5E
  {EfiKeySeven,      '7',      '7',   EFI_HOME_MODIFIER,        EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_NUM_LOCK},   // 0x5F
  {EfiKeyEight,      '8',      '8',   EFI_UP_ARROW_MODIFIER,    EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_NUM_LOCK},   // 0x60
  {EfiKeyNine,       '9',      '9',   EFI_PAGE_UP_MODIFIER,     EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_NUM_LOCK},   // 0x61
  {EfiKeyZero,       '0',      '0',   EFI_INSERT_MODIFIER,      EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_NUM_LOCK},   // 0x62
  {EfiKeyPeriod,     '.',      '.',   EFI_DELETE_MODIFIER,      EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_NUM_LOCK},   // 0x63
  {EfiKeyB0,         '\\',     '|',   EFI_NULL_MODIFIER,        EFI_AFFECTED_BY_STANDARD_SHIFT}, // 0x64 Keyboard Non-US \ and |
  {EfiKeyA4,         0x00,     0x00,  EFI_MENU_MODIFIER,        0},                              // 0x65 Keyboard Application

  {EfiKeyLCtrl,      0,        0,     EFI_LEFT_CONTROL_MODIFIER,    0},  // 0xe0
  {EfiKeyLShift,     0,        0,     EFI_LEFT_SHIFT_MODIFIER,      0},  // 0xe1
  {EfiKeyLAlt,       0,        0,     EFI_LEFT_ALT_MODIFIER,        0},  // 0xe2
  {EfiKeyA0,         0,        0,     EFI_LEFT_LOGO_MODIFIER,       0},  // 0xe3
  {EfiKeyRCtrl,      0,        0,     EFI_RIGHT_CONTROL_MODIFIER,   0},  // 0xe4
  {EfiKeyRShift,     0,        0,     EFI_RIGHT_SHIFT_MODIFIER,     0},  // 0xe5
  {EfiKeyA2,         0,        0,     EFI_RIGHT_ALT_MODIFIER,       0},  // 0xe6
  {EfiKeyA3,         0,        0,     EFI_RIGHT_LOGO_MODIFIER,      0},  // 0xe7
};

/**
  Initialize KeyConvertionTable by using default keyboard layout.

  @param  UsbKeyboardDevice    The USB_KB_DEV instance.
  @retval None.

**/
VOID
EFIAPI
LoadDefaultKeyboardLayout (
  IN USB_KB_DEV                 *UsbKeyboardDevice
  )
{
  UINTN               Index;
  EFI_KEY_DESCRIPTOR  *KeyDescriptor;

  //
  // Construct KeyConvertionTable by default keyboard layout
  //
  KeyDescriptor = &UsbKeyboardDevice->KeyConvertionTable[0];

  for (Index = 0; Index < (USB_KEYCODE_MAX_MAKE + 8); Index++) {
    KeyDescriptor->Key                 = (EFI_KEY) KeyboardLayoutTable[Index][0];
    KeyDescriptor->Unicode             = KeyboardLayoutTable[Index][1];
    KeyDescriptor->ShiftedUnicode      = KeyboardLayoutTable[Index][2];
    KeyDescriptor->AltGrUnicode        = 0;
    KeyDescriptor->ShiftedAltGrUnicode = 0;
    KeyDescriptor->Modifier            = KeyboardLayoutTable[Index][3];
    KeyDescriptor->AffectedAttribute   = KeyboardLayoutTable[Index][4];

    KeyDescriptor++;
  }
}

//
// EFI_KEY to USB Scan Code convertion table
//
STATIC
UINT8 UsbScanCodeConvertionTable[] = {
  0xe0,  //  EfiKeyLCtrl
  0xe3,  //  EfiKeyA0
  0xe2,  //  EfiKeyLAlt
  0x2c,  //  EfiKeySpaceBar
  0xe6,  //  EfiKeyA2
  0xe7,  //  EfiKeyA3
  0x65,  //  EfiKeyA4
  0xe4,  //  EfiKeyRCtrl
  0x50,  //  EfiKeyLeftArrow
  0x51,  //  EfiKeyDownArrow
  0x4F,  //  EfiKeyRightArrow
  0x62,  //  EfiKeyZero
  0x63,  //  EfiKeyPeriod
  0x28,  //  EfiKeyEnter
  0xe1,  //  EfiKeyLShift
  0x64,  //  EfiKeyB0
  0x1D,  //  EfiKeyB1
  0x1B,  //  EfiKeyB2
  0x06,  //  EfiKeyB3
  0x19,  //  EfiKeyB4
  0x05,  //  EfiKeyB5
  0x11,  //  EfiKeyB6
  0x10,  //  EfiKeyB7
  0x36,  //  EfiKeyB8
  0x37,  //  EfiKeyB9
  0x38,  //  EfiKeyB10
  0xe5,  //  EfiKeyRShift
  0x52,  //  EfiKeyUpArrow
  0x59,  //  EfiKeyOne
  0x5A,  //  EfiKeyTwo
  0x5B,  //  EfiKeyThree
  0x39,  //  EfiKeyCapsLock
  0x04,  //  EfiKeyC1
  0x16,  //  EfiKeyC2
  0x07,  //  EfiKeyC3
  0x09,  //  EfiKeyC4
  0x0A,  //  EfiKeyC5
  0x0B,  //  EfiKeyC6
  0x0D,  //  EfiKeyC7
  0x0E,  //  EfiKeyC8
  0x0F,  //  EfiKeyC9
  0x33,  //  EfiKeyC10
  0x34,  //  EfiKeyC11
  0x32,  //  EfiKeyC12
  0x5C,  //  EfiKeyFour
  0x5D,  //  EfiKeyFive
  0x5E,  //  EfiKeySix
  0x57,  //  EfiKeyPlus
  0x2B,  //  EfiKeyTab
  0x14,  //  EfiKeyD1
  0x1A,  //  EfiKeyD2
  0x08,  //  EfiKeyD3
  0x15,  //  EfiKeyD4
  0x17,  //  EfiKeyD5
  0x1C,  //  EfiKeyD6
  0x18,  //  EfiKeyD7
  0x0C,  //  EfiKeyD8
  0x12,  //  EfiKeyD9
  0x13,  //  EfiKeyD10
  0x2F,  //  EfiKeyD11
  0x30,  //  EfiKeyD12
  0x31,  //  EfiKeyD13
  0x4C,  //  EfiKeyDel
  0x4D,  //  EfiKeyEnd
  0x4E,  //  EfiKeyPgDn
  0x5F,  //  EfiKeySeven
  0x60,  //  EfiKeyEight
  0x61,  //  EfiKeyNine
  0x35,  //  EfiKeyE0
  0x1E,  //  EfiKeyE1
  0x1F,  //  EfiKeyE2
  0x20,  //  EfiKeyE3
  0x21,  //  EfiKeyE4
  0x22,  //  EfiKeyE5
  0x23,  //  EfiKeyE6
  0x24,  //  EfiKeyE7
  0x25,  //  EfiKeyE8
  0x26,  //  EfiKeyE9
  0x27,  //  EfiKeyE10
  0x2D,  //  EfiKeyE11
  0x2E,  //  EfiKeyE12
  0x2A,  //  EfiKeyBackSpace
  0x49,  //  EfiKeyIns
  0x4A,  //  EfiKeyHome
  0x4B,  //  EfiKeyPgUp
  0x53,  //  EfiKeyNLck
  0x54,  //  EfiKeySlash
  0x55,  //  EfiKeyAsterisk
  0x56,  //  EfiKeyMinus
  0x29,  //  EfiKeyEsc
  0x3A,  //  EfiKeyF1
  0x3B,  //  EfiKeyF2
  0x3C,  //  EfiKeyF3
  0x3D,  //  EfiKeyF4
  0x3E,  //  EfiKeyF5
  0x3F,  //  EfiKeyF6
  0x40,  //  EfiKeyF7
  0x41,  //  EfiKeyF8
  0x42,  //  EfiKeyF9
  0x43,  //  EfiKeyF10
  0x44,  //  EfiKeyF11
  0x45,  //  EfiKeyF12
  0x46,  //  EfiKeyPrint
  0x47,  //  EfiKeySLck
  0x48   //  EfiKeyPause
};

//
// Keyboard Layout Modifier to EFI Scan Code convertion table
//
STATIC
UINT8 EfiScanCodeConvertionTable[] = {
  SCAN_NULL,       // EFI_NULL_MODIFIER
  SCAN_NULL,       // EFI_LEFT_CONTROL_MODIFIER
  SCAN_NULL,       // EFI_RIGHT_CONTROL_MODIFIER
  SCAN_NULL,       // EFI_LEFT_ALT_MODIFIER
  SCAN_NULL,       // EFI_RIGHT_ALT_MODIFIER
  SCAN_NULL,       // EFI_ALT_GR_MODIFIER
  SCAN_INSERT,     // EFI_INSERT_MODIFIER
  SCAN_DELETE,     // EFI_DELETE_MODIFIER
  SCAN_PAGE_DOWN,  // EFI_PAGE_DOWN_MODIFIER
  SCAN_PAGE_UP,    // EFI_PAGE_UP_MODIFIER
  SCAN_HOME,       // EFI_HOME_MODIFIER
  SCAN_END,        // EFI_END_MODIFIER
  SCAN_NULL,       // EFI_LEFT_SHIFT_MODIFIER
  SCAN_NULL,       // EFI_RIGHT_SHIFT_MODIFIER
  SCAN_NULL,       // EFI_CAPS_LOCK_MODIFIER
  SCAN_NULL,       // EFI_NUM_LOCK_MODIFIER
  SCAN_LEFT,       // EFI_LEFT_ARROW_MODIFIER
  SCAN_RIGHT,      // EFI_RIGHT_ARROW_MODIFIER
  SCAN_DOWN,       // EFI_DOWN_ARROW_MODIFIER
  SCAN_UP,         // EFI_UP_ARROW_MODIFIER
  SCAN_NULL,       // EFI_NS_KEY_MODIFIER
  SCAN_NULL,       // EFI_NS_KEY_DEPENDENCY_MODIFIER
  SCAN_F1,         // EFI_FUNCTION_KEY_ONE_MODIFIER
  SCAN_F2,         // EFI_FUNCTION_KEY_TWO_MODIFIER
  SCAN_F3,         // EFI_FUNCTION_KEY_THREE_MODIFIER
  SCAN_F4,         // EFI_FUNCTION_KEY_FOUR_MODIFIER
  SCAN_F5,         // EFI_FUNCTION_KEY_FIVE_MODIFIER
  SCAN_F6,         // EFI_FUNCTION_KEY_SIX_MODIFIER
  SCAN_F7,         // EFI_FUNCTION_KEY_SEVEN_MODIFIER
  SCAN_F8,         // EFI_FUNCTION_KEY_EIGHT_MODIFIER
  SCAN_F9,         // EFI_FUNCTION_KEY_NINE_MODIFIER
  SCAN_F10,        // EFI_FUNCTION_KEY_TEN_MODIFIER
  SCAN_F11,        // EFI_FUNCTION_KEY_ELEVEN_MODIFIER
  SCAN_F12,        // EFI_FUNCTION_KEY_TWELVE_MODIFIER
};

EFI_GUID  mKeyboardLayoutEventGuid = EFI_HII_SET_KEYBOARD_LAYOUT_EVENT_GUID;


STATIC KB_MODIFIER  KB_Mod[8] = {
  { MOD_CONTROL_L,  0xe0 }, // 11100000
  { MOD_CONTROL_R,  0xe4 }, // 11100100
  { MOD_SHIFT_L,    0xe1 }, // 11100001
  { MOD_SHIFT_R,    0xe5 }, // 11100101
  { MOD_ALT_L,      0xe2 }, // 11100010
  { MOD_ALT_R,      0xe6 }, // 11100110
  { MOD_WIN_L,      0xe3 }, // 11100011
  { MOD_WIN_R,      0xe7 }, // 11100111 
};



/**
  Uses USB I/O to check whether the device is a USB Keyboard device.

  @param  UsbIo    Points to a USB I/O protocol instance.
  @retval None

**/
BOOLEAN
EFIAPI
IsUSBKeyboard (
  IN  EFI_USB_IO_PROTOCOL       *UsbIo
  )
{
  EFI_STATUS                    Status;
  EFI_USB_INTERFACE_DESCRIPTOR  InterfaceDescriptor;

  //
  // Get the Default interface descriptor, currently we
  // assume it is interface 1
  //
  Status = UsbIo->UsbGetInterfaceDescriptor (
                    UsbIo,
                    &InterfaceDescriptor
                    );

  if (EFI_ERROR (Status)) {
    return FALSE;
  }

  if (InterfaceDescriptor.InterfaceClass == CLASS_HID &&
      InterfaceDescriptor.InterfaceSubClass == SUBCLASS_BOOT &&
      InterfaceDescriptor.InterfaceProtocol == PROTOCOL_KEYBOARD
      ) {

    return TRUE;
  }

  return FALSE;
}

/**
  Get current keyboard layout from HII database.

  @retval Pointer to EFI_HII_KEYBOARD_LAYOUT.

**/
EFI_HII_KEYBOARD_LAYOUT *
EFIAPI
GetCurrentKeyboardLayout (
  VOID
  )
{
  EFI_STATUS                Status;
  EFI_HII_DATABASE_PROTOCOL *HiiDatabase;
  EFI_HII_KEYBOARD_LAYOUT   *KeyboardLayout;
  UINT16                    Length;

  //
  // Locate Hii database protocol
  //
  Status = gBS->LocateProtocol (
                  &gEfiHiiDatabaseProtocolGuid,
                  NULL,
                  (VOID **) &HiiDatabase
                  );
  if (EFI_ERROR (Status)) {
    return NULL;
  }

  //
  // Get current keyboard layout from HII database
  //
  Length = 0;
  KeyboardLayout = NULL;
  Status = HiiDatabase->GetKeyboardLayout (
                          HiiDatabase,
                          NULL,
                          &Length,
                          KeyboardLayout
                          );
  if (Status == EFI_BUFFER_TOO_SMALL) {
    KeyboardLayout = AllocatePool (Length);
    ASSERT (KeyboardLayout != NULL);

    Status = HiiDatabase->GetKeyboardLayout (
                            HiiDatabase,
                            NULL,
                            &Length,
                            KeyboardLayout
                            );
    if (EFI_ERROR (Status)) {
      gBS->FreePool (KeyboardLayout);
      KeyboardLayout = NULL;
    }
  }

  return KeyboardLayout;
}

/**
  Find Key Descriptor in KeyConvertionTable given its scan code.

  @param  UsbKeyboardDevice   The USB_KB_DEV instance.
  @param  ScanCode            USB scan code.

  @return The Key descriptor in KeyConvertionTable.

**/
EFI_KEY_DESCRIPTOR *
EFIAPI
GetKeyDescriptor (
  IN USB_KB_DEV        *UsbKeyboardDevice,
  IN UINT8             ScanCode
  )
{
  UINT8  Index;

  if (((ScanCode > 0x65) && (ScanCode < 0xe0)) || (ScanCode > 0xe7)) {
    return NULL;
  }

  if (ScanCode <= 0x65) {
    Index = (UINT8) (ScanCode - 4);
  } else {
    Index = (UINT8) (ScanCode - 0xe0 + USB_KEYCODE_MAX_MAKE);
  }

  return &UsbKeyboardDevice->KeyConvertionTable[Index];
}

/**
  Find Non-Spacing key for given KeyDescriptor.

  @param  UsbKeyboardDevice    The USB_KB_DEV instance.
  @param  KeyDescriptor        Key descriptor.

  @retval NULL                 Key list is empty.
  @return Other                The Non-Spacing key.

**/
USB_NS_KEY *
EFIAPI
FindUsbNsKey (
  IN USB_KB_DEV          *UsbKeyboardDevice,
  IN EFI_KEY_DESCRIPTOR  *KeyDescriptor
  )
{
  LIST_ENTRY      *Link;
  USB_NS_KEY      *UsbNsKey;

  Link = GetFirstNode (&UsbKeyboardDevice->NsKeyList);
  while (!IsNull (&UsbKeyboardDevice->NsKeyList, Link)) {
    UsbNsKey = USB_NS_KEY_FORM_FROM_LINK (Link);

    if (UsbNsKey->NsKey[0].Key == KeyDescriptor->Key) {
      return UsbNsKey;
    }

    Link = GetNextNode (&UsbKeyboardDevice->NsKeyList, Link);
  }

  return NULL;
}

/**
  Find physical key definition for a given Key stroke.

  @param  UsbNsKey          The Non-Spacing key information.
  @param  KeyDescriptor     The key stroke.

  @return The physical key definition.

**/
EFI_KEY_DESCRIPTOR *
EFIAPI
FindPhysicalKey (
  IN USB_NS_KEY          *UsbNsKey,
  IN EFI_KEY_DESCRIPTOR  *KeyDescriptor
  )
{
  UINTN               Index;
  EFI_KEY_DESCRIPTOR  *PhysicalKey;

  PhysicalKey = &UsbNsKey->NsKey[1];
  for (Index = 0; Index < UsbNsKey->KeyCount; Index++) {
    if (KeyDescriptor->Key == PhysicalKey->Key) {
      return PhysicalKey;
    }

    PhysicalKey++;
  }

  //
  // No children definition matched, return original key
  //
  return KeyDescriptor;
}

/**
  The notification function for SET_KEYBOARD_LAYOUT_EVENT.

  @param  Event           The instance of EFI_EVENT.
  @param  Context         passing parameter.

**/
VOID
EFIAPI
SetKeyboardLayoutEvent (
  EFI_EVENT                  Event,
  VOID                       *Context
  )
{
  USB_KB_DEV                *UsbKeyboardDevice;
  EFI_HII_KEYBOARD_LAYOUT   *KeyboardLayout;
  EFI_KEY_DESCRIPTOR        TempKey;
  EFI_KEY_DESCRIPTOR        *KeyDescriptor;
  EFI_KEY_DESCRIPTOR        *TableEntry;
  EFI_KEY_DESCRIPTOR        *NsKey;
  USB_NS_KEY                *UsbNsKey;
  UINTN                     Index;
  UINTN                     Index2;
  UINTN                     KeyCount;
  UINT8                     ScanCode;

  UsbKeyboardDevice = (USB_KB_DEV *) Context;

  //
  // Try to get current Keyboard Layout from HII database
  //
  KeyboardLayout = GetCurrentKeyboardLayout ();
  if (KeyboardLayout == NULL) {
    return;
  }

  //
  // Allocate resource for KeyConvertionTable
  //
  ReleaseKeyboardLayoutResources (UsbKeyboardDevice);
  UsbKeyboardDevice->KeyConvertionTable = AllocateZeroPool ((USB_KEYCODE_MAX_MAKE + 8) * sizeof (EFI_KEY_DESCRIPTOR));
  ASSERT (UsbKeyboardDevice->KeyConvertionTable != NULL);

  KeyDescriptor = (EFI_KEY_DESCRIPTOR *) (((UINT8 *) KeyboardLayout) + sizeof (EFI_HII_KEYBOARD_LAYOUT));
  for (Index = 0; Index < KeyboardLayout->DescriptorCount; Index++) {
    //
    // Copy from HII keyboard layout package binary for alignment
    //
    CopyMem (&TempKey, KeyDescriptor, sizeof (EFI_KEY_DESCRIPTOR));

    //
    // Fill the key into KeyConvertionTable (which use USB Scan Code as index)
    //
    ScanCode = UsbScanCodeConvertionTable [(UINT8) (TempKey.Key)];
    TableEntry = GetKeyDescriptor (UsbKeyboardDevice, ScanCode);
    CopyMem (TableEntry, KeyDescriptor, sizeof (EFI_KEY_DESCRIPTOR));

    if (TempKey.Modifier == EFI_NS_KEY_MODIFIER) {
      //
      // Non-spacing key
      //
      UsbNsKey = AllocatePool (sizeof (USB_NS_KEY));
      ASSERT (UsbNsKey != NULL);

      //
      // Search for sequential children physical key definitions
      //
      KeyCount = 0;
      NsKey = KeyDescriptor + 1;
      for (Index2 = Index + 1; Index2 < KeyboardLayout->DescriptorCount; Index2++) {
        CopyMem (&TempKey, NsKey, sizeof (EFI_KEY_DESCRIPTOR));
        if (TempKey.Modifier & EFI_NS_KEY_DEPENDENCY_MODIFIER) {
          KeyCount++;
        } else {
          break;
        }
        NsKey++;
      }

      UsbNsKey->Signature = USB_NS_KEY_SIGNATURE;
      UsbNsKey->KeyCount = KeyCount;
      UsbNsKey->NsKey = AllocateCopyPool (
                          (KeyCount + 1) * sizeof (EFI_KEY_DESCRIPTOR),
                          KeyDescriptor
                          );
      InsertTailList (&UsbKeyboardDevice->NsKeyList, &UsbNsKey->Link);

      //
      // Skip over the child physical keys
      //
      Index += KeyCount;
      KeyDescriptor += KeyCount;
    }

    KeyDescriptor++;
  }

  //
  // There are two EfiKeyEnter, duplicate its Key Descriptor
  //
  TableEntry = GetKeyDescriptor (UsbKeyboardDevice, 0x58);
  KeyDescriptor = GetKeyDescriptor (UsbKeyboardDevice, 0x28);
  CopyMem (TableEntry, KeyDescriptor, sizeof (EFI_KEY_DESCRIPTOR));

  gBS->FreePool (KeyboardLayout);
}

/**
  Destroy resources for Keyboard layout.

  @param  UsbKeyboardDevice    The USB_KB_DEV instance.

**/
VOID
EFIAPI
ReleaseKeyboardLayoutResources (
  IN USB_KB_DEV              *UsbKeyboardDevice
  )
{
  USB_NS_KEY      *UsbNsKey;
  LIST_ENTRY      *Link;

  SafeFreePool (UsbKeyboardDevice->KeyConvertionTable);
  UsbKeyboardDevice->KeyConvertionTable = NULL;

  while (!IsListEmpty (&UsbKeyboardDevice->NsKeyList)) {
    Link = GetFirstNode (&UsbKeyboardDevice->NsKeyList);
    UsbNsKey = USB_NS_KEY_FORM_FROM_LINK (Link);
    RemoveEntryList (&UsbNsKey->Link);

    gBS->FreePool (UsbNsKey->NsKey);
    gBS->FreePool (UsbNsKey);
  }
}

/**
  Initialize USB Keyboard layout.

  @param  UsbKeyboardDevice      The USB_KB_DEV instance.

  @retval EFI_SUCCESS            Initialization Success.
  @retval Other                  Keyboard layout initial failed.

**/
EFI_STATUS
EFIAPI
InitKeyboardLayout (
  IN USB_KB_DEV   *UsbKeyboardDevice
  )
{
  EFI_HII_KEYBOARD_LAYOUT   *KeyboardLayout;
  EFI_STATUS                Status;

  UsbKeyboardDevice->KeyConvertionTable = AllocateZeroPool ((USB_KEYCODE_MAX_MAKE + 8) * sizeof (EFI_KEY_DESCRIPTOR));
  ASSERT (UsbKeyboardDevice->KeyConvertionTable != NULL);

  InitializeListHead (&UsbKeyboardDevice->NsKeyList);
  UsbKeyboardDevice->CurrentNsKey = NULL;
  UsbKeyboardDevice->KeyboardLayoutEvent = NULL;

  //
  // Register SET_KEYBOARD_LAYOUT_EVENT notification
  //
  Status = gBS->CreateEventEx (
                  EVT_NOTIFY_SIGNAL,
                  TPL_NOTIFY,
                  SetKeyboardLayoutEvent,
                  UsbKeyboardDevice,
                  &mKeyboardLayoutEventGuid,
                  &UsbKeyboardDevice->KeyboardLayoutEvent
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Try to get current keyboard layout from HII database
  //
  KeyboardLayout = GetCurrentKeyboardLayout ();
  if (KeyboardLayout != NULL) {
    //
    // Force to initialize the keyboard layout
    //
    gBS->SignalEvent (UsbKeyboardDevice->KeyboardLayoutEvent);
  } else {
    if (FeaturePcdGet (PcdDisableDefaultKeyboardLayoutInUsbKbDriver)) {
      return EFI_NOT_READY;
    } else {

      //
      // Fail to get keyboard layout from HII database,
      // use default keyboard layout
      //
      LoadDefaultKeyboardLayout (UsbKeyboardDevice);
    }
  }
  
  return EFI_SUCCESS;
}


/**
  Initialize USB Keyboard device and all private data structures.

  @param  UsbKeyboardDevice  The USB_KB_DEV instance.

  @retval EFI_SUCCESS        Initialization is successful.
  @retval EFI_DEVICE_ERROR   Configure hardware failed.

**/
EFI_STATUS
EFIAPI
InitUSBKeyboard (
  IN USB_KB_DEV   *UsbKeyboardDevice
  )
{
  UINT8               ConfigValue;
  UINT8               Protocol;
  UINT8               ReportId;
  UINT8               Duration;
  EFI_STATUS          Status;
  UINT32              TransferResult;

  KbdReportStatusCode (
    UsbKeyboardDevice->DevicePath,
    EFI_PROGRESS_CODE,
    PcdGet32 (PcdStatusCodeValueKeyboardSelfTest)
    );

  InitUSBKeyBuffer (&(UsbKeyboardDevice->KeyboardBuffer));

  //
  // default configurations
  //
  ConfigValue = 0x01;

  //
  // Uses default configuration to configure the USB Keyboard device.
  //
  Status = UsbSetConfiguration (
            UsbKeyboardDevice->UsbIo,
            (UINT16) ConfigValue,
            &TransferResult
            );
  if (EFI_ERROR (Status)) {
    //
    // If configuration could not be set here, it means
    // the keyboard interface has some errors and could
    // not be initialized
    //
    KbdReportStatusCode (
      UsbKeyboardDevice->DevicePath,
      EFI_ERROR_CODE | EFI_ERROR_MINOR,
      PcdGet32 (PcdStatusCodeValueKeyboardInterfaceError)
      );

    return EFI_DEVICE_ERROR;
  }

  UsbGetProtocolRequest (
    UsbKeyboardDevice->UsbIo,
    UsbKeyboardDevice->InterfaceDescriptor.InterfaceNumber,
    &Protocol
    );
  //
  // Sets boot protocol for the USB Keyboard.
  // This driver only supports boot protocol.
  // !!BugBug: How about the device that does not support boot protocol?
  //
  if (Protocol != BOOT_PROTOCOL) {
    UsbSetProtocolRequest (
      UsbKeyboardDevice->UsbIo,
      UsbKeyboardDevice->InterfaceDescriptor.InterfaceNumber,
      BOOT_PROTOCOL
      );
  }
  //
  // the duration is indefinite, so the endpoint will inhibit reporting forever,
  // and only reporting when a change is detected in the report data.
  //

  //
  // idle value for all report ID
  //
  ReportId = 0;
  //
  // idle forever until there is a key pressed and released.
  //
  Duration = 0;
  UsbSetIdleRequest (
    UsbKeyboardDevice->UsbIo,
    UsbKeyboardDevice->InterfaceDescriptor.InterfaceNumber,
    ReportId,
    Duration
    );

  UsbKeyboardDevice->CtrlOn     = 0;
  UsbKeyboardDevice->AltOn      = 0;
  UsbKeyboardDevice->ShiftOn    = 0;
  UsbKeyboardDevice->NumLockOn  = 0;
  UsbKeyboardDevice->CapsOn     = 0;
  UsbKeyboardDevice->ScrollOn   = 0;
  
  UsbKeyboardDevice->LeftCtrlOn   = 0;
  UsbKeyboardDevice->LeftAltOn    = 0;
  UsbKeyboardDevice->LeftShiftOn  = 0;
  UsbKeyboardDevice->LeftLogoOn   = 0;
  UsbKeyboardDevice->RightCtrlOn  = 0;
  UsbKeyboardDevice->RightAltOn   = 0;
  UsbKeyboardDevice->RightShiftOn = 0;
  UsbKeyboardDevice->RightLogoOn  = 0;
  UsbKeyboardDevice->MenuKeyOn    = 0;
  UsbKeyboardDevice->SysReqOn     = 0;

  UsbKeyboardDevice->AltGrOn      = 0;

  UsbKeyboardDevice->CurrentNsKey = NULL;

  //
  // Sync the initial state of lights
  //
  SetKeyLED (UsbKeyboardDevice);

  ZeroMem (UsbKeyboardDevice->LastKeyCodeArray, sizeof (UINT8) * 8);

  //
  // Set a timer for repeat keys' generation.
  //
  if (UsbKeyboardDevice->RepeatTimer != NULL) {
    gBS->CloseEvent (UsbKeyboardDevice->RepeatTimer);
    UsbKeyboardDevice->RepeatTimer = 0;
  }

  Status = gBS->CreateEvent (
                  EVT_TIMER | EVT_NOTIFY_SIGNAL,
                  TPL_NOTIFY,
                  USBKeyboardRepeatHandler,
                  UsbKeyboardDevice,
                  &UsbKeyboardDevice->RepeatTimer
                  );

  if (UsbKeyboardDevice->DelayedRecoveryEvent != NULL) {
    gBS->CloseEvent (UsbKeyboardDevice->DelayedRecoveryEvent);
    UsbKeyboardDevice->DelayedRecoveryEvent = 0;
  }

  Status = gBS->CreateEvent (
                  EVT_TIMER | EVT_NOTIFY_SIGNAL,
                  TPL_NOTIFY,
                  USBKeyboardRecoveryHandler,
                  UsbKeyboardDevice,
                  &UsbKeyboardDevice->DelayedRecoveryEvent
                  );

  return EFI_SUCCESS;
}


/**
  Handler function for USB Keyboard's asynchronous interrupt transfer.

  @param  Data             A pointer to a buffer that is filled with key data which is
                           retrieved via asynchronous interrupt transfer.
  @param  DataLength       Indicates the size of the data buffer.
  @param  Context          Pointing to USB_KB_DEV instance.
  @param  Result           Indicates the result of the asynchronous interrupt transfer.

  @retval EFI_SUCCESS      Handler is successful.
  @retval EFI_DEVICE_ERROR Hardware Error

**/
EFI_STATUS
EFIAPI
KeyboardHandler (
  IN  VOID          *Data,
  IN  UINTN         DataLength,
  IN  VOID          *Context,
  IN  UINT32        Result
  )
{
  USB_KB_DEV          *UsbKeyboardDevice;
  EFI_USB_IO_PROTOCOL *UsbIo;
  UINT8               *CurKeyCodeBuffer;
  UINT8               *OldKeyCodeBuffer;
  UINT8               CurModifierMap;
  UINT8               OldModifierMap;
  UINT8               Index;
  UINT8               Index2;
  BOOLEAN             Down;
  BOOLEAN             KeyRelease;
  BOOLEAN             KeyPress;
  UINT8               SavedTail;
  USB_KEY             UsbKey;
  UINT8               NewRepeatKey;
  UINT32              UsbStatus;
  EFI_KEY_DESCRIPTOR  *KeyDescriptor;

  ASSERT (Context);

  NewRepeatKey      = 0;
  UsbKeyboardDevice = (USB_KB_DEV *) Context;
  UsbIo             = UsbKeyboardDevice->UsbIo;

  //
  // Analyzes the Result and performs corresponding action.
  //
  if (Result != EFI_USB_NOERROR) {
    //
    // Some errors happen during the process
    //
    KbdReportStatusCode (
      UsbKeyboardDevice->DevicePath,
      EFI_ERROR_CODE | EFI_ERROR_MINOR,
      PcdGet32 (PcdStatusCodeValueKeyboardInputError)
      );

    //
    // stop the repeat key generation if any
    //
    UsbKeyboardDevice->RepeatKey = 0;

    gBS->SetTimer (
          UsbKeyboardDevice->RepeatTimer,
          TimerCancel,
          USBKBD_REPEAT_RATE
          );

    if ((Result & EFI_USB_ERR_STALL) == EFI_USB_ERR_STALL) {
      UsbClearEndpointHalt (
        UsbIo,
        UsbKeyboardDevice->IntEndpointDescriptor.EndpointAddress,
        &UsbStatus
        );
    }

    //
    // Delete & Submit this interrupt again
    //

    UsbIo->UsbAsyncInterruptTransfer (
                      UsbIo,
                      UsbKeyboardDevice->IntEndpointDescriptor.EndpointAddress,
                      FALSE,
                      0,
                      0,
                      NULL,
                      NULL
                      );

    gBS->SetTimer (
          UsbKeyboardDevice->DelayedRecoveryEvent,
          TimerRelative,
          EFI_USB_INTERRUPT_DELAY
          );

    return EFI_DEVICE_ERROR;
  }

  if (DataLength == 0 || Data == NULL) {
    return EFI_SUCCESS;
  }

  CurKeyCodeBuffer  = (UINT8 *) Data;
  OldKeyCodeBuffer  = UsbKeyboardDevice->LastKeyCodeArray;

  //
  // checks for new key stroke.
  // if no new key got, return immediately.
  //
  for (Index = 0; Index < 8; Index++) {
    if (OldKeyCodeBuffer[Index] != CurKeyCodeBuffer[Index]) {
      break;
    }
  }

  if (Index == 8) {
    return EFI_SUCCESS;
  }

  //
  // Parse the modifier key
  //
  CurModifierMap  = CurKeyCodeBuffer[0];
  OldModifierMap  = OldKeyCodeBuffer[0];

  //
  // handle modifier key's pressing or releasing situation.
  //
  for (Index = 0; Index < 8; Index++) {

    if ((CurModifierMap & KB_Mod[Index].Mask) != (OldModifierMap & KB_Mod[Index].Mask)) {
      //
      // if current modifier key is up, then
      // CurModifierMap & KB_Mod[Index].Mask = 0;
      // otherwize it is a non-zero value.
      // Inserts the pressed modifier key into key buffer.
      //
      Down = (UINT8) (CurModifierMap & KB_Mod[Index].Mask);
      InsertKeyCode (&(UsbKeyboardDevice->KeyboardBuffer), KB_Mod[Index].Key, Down);
    }
  }

  //
  // handle normal key's releasing situation
  //
  KeyRelease = FALSE;
  for (Index = 2; Index < 8; Index++) {

    if (!USBKBD_VALID_KEYCODE (OldKeyCodeBuffer[Index])) {
      continue;
    }

    KeyRelease = TRUE;
    for (Index2 = 2; Index2 < 8; Index2++) {

      if (!USBKBD_VALID_KEYCODE (CurKeyCodeBuffer[Index2])) {
        continue;
      }

      if (OldKeyCodeBuffer[Index] == CurKeyCodeBuffer[Index2]) {
        KeyRelease = FALSE;
        break;
      }
    }

    if (KeyRelease) {
      InsertKeyCode (
        &(UsbKeyboardDevice->KeyboardBuffer),
        OldKeyCodeBuffer[Index],
        0
        );
      //
      // the original reapeat key is released.
      //
      if (OldKeyCodeBuffer[Index] == UsbKeyboardDevice->RepeatKey) {
        UsbKeyboardDevice->RepeatKey = 0;
      }
    }
  }

  //
  // original repeat key is released, cancel the repeat timer
  //
  if (UsbKeyboardDevice->RepeatKey == 0) {
    gBS->SetTimer (
          UsbKeyboardDevice->RepeatTimer,
          TimerCancel,
          USBKBD_REPEAT_RATE
          );
  }

  //
  // handle normal key's pressing situation
  //
  KeyPress = FALSE;
  for (Index = 2; Index < 8; Index++) {

    if (!USBKBD_VALID_KEYCODE (CurKeyCodeBuffer[Index])) {
      continue;
    }

    KeyPress = TRUE;
    for (Index2 = 2; Index2 < 8; Index2++) {

      if (!USBKBD_VALID_KEYCODE (OldKeyCodeBuffer[Index2])) {
        continue;
      }

      if (CurKeyCodeBuffer[Index] == OldKeyCodeBuffer[Index2]) {
        KeyPress = FALSE;
        break;
      }
    }

    if (KeyPress) {
      InsertKeyCode (&(UsbKeyboardDevice->KeyboardBuffer), CurKeyCodeBuffer[Index], 1);
      //
      // NumLock pressed or CapsLock pressed
      //
      KeyDescriptor = GetKeyDescriptor (UsbKeyboardDevice, CurKeyCodeBuffer[Index]);
      if (KeyDescriptor->Modifier == EFI_NUM_LOCK_MODIFIER || KeyDescriptor->Modifier == EFI_CAPS_LOCK_MODIFIER) {
        UsbKeyboardDevice->RepeatKey = 0;
      } else {
        NewRepeatKey = CurKeyCodeBuffer[Index];
        //
        // do not repeat the original repeated key
        //
        UsbKeyboardDevice->RepeatKey = 0;
      }
    }
  }

  //
  // Update LastKeycodeArray[] buffer in the
  // Usb Keyboard Device data structure.
  //
  for (Index = 0; Index < 8; Index++) {
    UsbKeyboardDevice->LastKeyCodeArray[Index] = CurKeyCodeBuffer[Index];
  }

  //
  // pre-process KeyboardBuffer, pop out the ctrl,alt,del key in sequence
  // and judge whether it will invoke reset event.
  //
  SavedTail = UsbKeyboardDevice->KeyboardBuffer.bTail;
  Index     = UsbKeyboardDevice->KeyboardBuffer.bHead;
  while (Index != SavedTail) {
    RemoveKeyCode (&(UsbKeyboardDevice->KeyboardBuffer), &UsbKey);

    KeyDescriptor = GetKeyDescriptor (UsbKeyboardDevice, UsbKey.KeyCode);

    switch (KeyDescriptor->Modifier) {

    case EFI_LEFT_CONTROL_MODIFIER:
    case EFI_RIGHT_CONTROL_MODIFIER:
      if (UsbKey.Down != 0) {
        UsbKeyboardDevice->CtrlOn = 1;
      } else {
        UsbKeyboardDevice->CtrlOn = 0;
      }
      break;

    case EFI_LEFT_ALT_MODIFIER:
    case EFI_RIGHT_ALT_MODIFIER:
      if (UsbKey.Down != 0) {
        UsbKeyboardDevice->AltOn = 1;
      } else {
        UsbKeyboardDevice->AltOn = 0;
      }
      break;

    case EFI_ALT_GR_MODIFIER:
      if (UsbKey.Down != 0) {
        UsbKeyboardDevice->AltGrOn = 1;
      } else {
        UsbKeyboardDevice->AltGrOn = 0;
      }
      break;

    //
    // Del Key Code
    //
    case EFI_DELETE_MODIFIER:
      if (UsbKey.Down != 0) {
        if ((UsbKeyboardDevice->CtrlOn != 0) && (UsbKeyboardDevice->AltOn != 0)) {
          gRT->ResetSystem (EfiResetWarm, EFI_SUCCESS, 0, NULL);
        }
      }
      break;

    default:
      break;
    }

    //
    // insert the key back to the buffer.
    // so the key sequence will not be destroyed.
    //
    InsertKeyCode (
      &(UsbKeyboardDevice->KeyboardBuffer),
      UsbKey.KeyCode,
      UsbKey.Down
      );
    Index = UsbKeyboardDevice->KeyboardBuffer.bHead;

  }
  //
  // If have new key pressed, update the RepeatKey value, and set the
  // timer to repeate delay timer
  //
  if (NewRepeatKey != 0) {
    //
    // sets trigger time to "Repeat Delay Time",
    // to trigger the repeat timer when the key is hold long
    // enough time.
    //
    gBS->SetTimer (
          UsbKeyboardDevice->RepeatTimer,
          TimerRelative,
          USBKBD_REPEAT_DELAY
          );
    UsbKeyboardDevice->RepeatKey = NewRepeatKey;
  }

  return EFI_SUCCESS;
}


/**
  Retrieves a key character after parsing the raw data in keyboard buffer.

  @param  UsbKeyboardDevice    The USB_KB_DEV instance.
  @param  KeyChar              Points to the Key character after key parsing.

  @retval EFI_SUCCESS          Parse key is successful.
  @retval EFI_NOT_READY        Device is not ready.

**/
EFI_STATUS
EFIAPI
USBParseKey (
  IN OUT  USB_KB_DEV  *UsbKeyboardDevice,
  OUT     UINT8       *KeyChar
  )
{
  USB_KEY             UsbKey;
  EFI_KEY_DESCRIPTOR  *KeyDescriptor;

  *KeyChar = 0;

  while (!IsUSBKeyboardBufferEmpty (&UsbKeyboardDevice->KeyboardBuffer)) {
    //
    // pops one raw data off.
    //
    RemoveKeyCode (&(UsbKeyboardDevice->KeyboardBuffer), &UsbKey);

    KeyDescriptor = GetKeyDescriptor (UsbKeyboardDevice, UsbKey.KeyCode);
    if (UsbKey.Down == 0) {
      switch (KeyDescriptor->Modifier) {

      //
      // CTRL release
      //
      case EFI_LEFT_CONTROL_MODIFIER:
        UsbKeyboardDevice->LeftCtrlOn = 0;
        UsbKeyboardDevice->CtrlOn = 0;
        break;
      case EFI_RIGHT_CONTROL_MODIFIER:
        UsbKeyboardDevice->RightCtrlOn = 0;
        UsbKeyboardDevice->CtrlOn = 0;
        break;

      //
      // Shift release
      //
      case EFI_LEFT_SHIFT_MODIFIER:
        UsbKeyboardDevice->LeftShiftOn = 0;
        UsbKeyboardDevice->ShiftOn = 0;
        break;
      case EFI_RIGHT_SHIFT_MODIFIER:
        UsbKeyboardDevice->RightShiftOn = 0;
        UsbKeyboardDevice->ShiftOn = 0;
        break;

      //
      // Alt release
      //
      case EFI_LEFT_ALT_MODIFIER:
        UsbKeyboardDevice->LeftAltOn = 0;
        UsbKeyboardDevice->AltOn = 0;
        break;
      case EFI_RIGHT_ALT_MODIFIER:
        UsbKeyboardDevice->RightAltOn = 0;
        UsbKeyboardDevice->AltOn = 0;
        break;

      //
      // Left Logo release
      //
      case EFI_LEFT_LOGO_MODIFIER:
        UsbKeyboardDevice->LeftLogoOn = 0;
        break;

      //
      // Right Logo release
      //
      case EFI_RIGHT_LOGO_MODIFIER:
        UsbKeyboardDevice->RightLogoOn = 0;
        break;

      //
      // Menu key release
      //
      case EFI_MENU_MODIFIER:
        UsbKeyboardDevice->MenuKeyOn = 0;
        break;

      //
      // SysReq release
      //
      case EFI_PRINT_MODIFIER:
      case EFI_SYS_REQUEST_MODIFIER:
        UsbKeyboardDevice->SysReqOn = 0;
        break;

      //
      // AltGr release
      //
      case EFI_ALT_GR_MODIFIER:
        UsbKeyboardDevice->AltGrOn = 0;
        break;

      default:
        break;
      }

      continue;
    }

    //
    // Analyzes key pressing situation
    //
    switch (KeyDescriptor->Modifier) {

    //
    // CTRL press
    //
    case EFI_LEFT_CONTROL_MODIFIER:
      UsbKeyboardDevice->LeftCtrlOn = 1;
      UsbKeyboardDevice->CtrlOn = 1;
      continue;
      break;
    case EFI_RIGHT_CONTROL_MODIFIER:
      UsbKeyboardDevice->RightCtrlOn = 1;
      UsbKeyboardDevice->CtrlOn = 1;
      continue;
      break;

    //
    // Shift press
    //
    case EFI_LEFT_SHIFT_MODIFIER:
      UsbKeyboardDevice->LeftShiftOn = 1;
      UsbKeyboardDevice->ShiftOn = 1;
      continue;
      break;
    case EFI_RIGHT_SHIFT_MODIFIER:
      UsbKeyboardDevice->RightShiftOn = 1;
      UsbKeyboardDevice->ShiftOn = 1;
      continue;
      break;

    //
    // Alt press
    //
    case EFI_LEFT_ALT_MODIFIER:
      UsbKeyboardDevice->LeftAltOn = 1;
      UsbKeyboardDevice->AltOn = 1;
      continue;
      break;
    case EFI_RIGHT_ALT_MODIFIER:
      UsbKeyboardDevice->RightAltOn = 1;
      UsbKeyboardDevice->AltOn = 1;
      continue;
      break;

    //
    // Left Logo press
    //
    case EFI_LEFT_LOGO_MODIFIER:
      UsbKeyboardDevice->LeftLogoOn = 1;
      break;

    //
    // Right Logo press
    //
    case EFI_RIGHT_LOGO_MODIFIER:
      UsbKeyboardDevice->RightLogoOn = 1;
      break;

    //
    // Menu key press
    //
    case EFI_MENU_MODIFIER:
      UsbKeyboardDevice->MenuKeyOn = 1;
      break;

    //
    // SysReq press
    //
    case EFI_PRINT_MODIFIER:
    case EFI_SYS_REQUEST_MODIFIER:
      UsbKeyboardDevice->SysReqOn = 1;
      continue;
      break;

    //
    // AltGr press
    //
    case EFI_ALT_GR_MODIFIER:
      UsbKeyboardDevice->AltGrOn = 1;
      break;

    case EFI_NUM_LOCK_MODIFIER:
      UsbKeyboardDevice->NumLockOn ^= 1;
      //
      // Turn on the NumLock light on KB
      //
      SetKeyLED (UsbKeyboardDevice);
      continue;
      break;

    case EFI_CAPS_LOCK_MODIFIER:
      UsbKeyboardDevice->CapsOn ^= 1;
      //
      // Turn on the CapsLock light on KB
      //
      SetKeyLED (UsbKeyboardDevice);
      continue;
      break;

    case EFI_SCROLL_LOCK_MODIFIER:
      UsbKeyboardDevice->ScrollOn ^= 1;
      //
      // Turn on the ScrollLock light on KB
      //
      SetKeyLED (UsbKeyboardDevice);
      continue;
      break;

    //
    // F11,F12,PrintScreen,Pause/Break
    // could not be retrieved via SimpleTxtInEx protocol
    //
    case EFI_FUNCTION_KEY_ELEVEN_MODIFIER:
    case EFI_FUNCTION_KEY_TWELVE_MODIFIER:
    case EFI_PAUSE_MODIFIER:
    case EFI_BREAK_MODIFIER:
      //
      // fall through
      //
      continue;
      break;

    default:
      break;
    }

    //
    // When encountered Del Key...
    //
    if (KeyDescriptor->Modifier == EFI_DELETE_MODIFIER) {
      if ((UsbKeyboardDevice->CtrlOn != 0) && (UsbKeyboardDevice->AltOn != 0)) {
        gRT->ResetSystem (EfiResetWarm, EFI_SUCCESS, 0, NULL);
      }
    }

    *KeyChar = UsbKey.KeyCode;
    return EFI_SUCCESS;
  }

  return EFI_NOT_READY;
}


/**
  Converts USB Keyboard code to EFI Scan Code.

  @param  UsbKeyboardDevice    The USB_KB_DEV instance.
  @param  KeyChar              Indicates the key code that will be interpreted.
  @param  Key                  A pointer to a buffer that is filled in with
                               the keystroke information for the key that
                               was pressed.

  @retval EFI_NOT_READY        Device is not ready
  @retval EFI_SUCCESS          Success.

**/
EFI_STATUS
EFIAPI
USBKeyCodeToEFIScanCode (
  IN  USB_KB_DEV      *UsbKeyboardDevice,
  IN  UINT8           KeyChar,
  OUT EFI_INPUT_KEY   *Key
  )
{
  UINT8               Index;
  EFI_KEY_DESCRIPTOR  *KeyDescriptor;

  if (!USBKBD_VALID_KEYCODE (KeyChar)) {
    return EFI_NOT_READY;
  }

  //
  // valid USB Key Code starts from 4
  //
  Index = (UINT8) (KeyChar - 4);

  if (Index >= USB_KEYCODE_MAX_MAKE) {
    return EFI_NOT_READY;
  }

  KeyDescriptor = GetKeyDescriptor (UsbKeyboardDevice, KeyChar);

  //
  // Check for Non-spacing key
  //
  if (KeyDescriptor->Modifier == EFI_NS_KEY_MODIFIER) {
    UsbKeyboardDevice->CurrentNsKey = FindUsbNsKey (UsbKeyboardDevice, KeyDescriptor);
    return EFI_NOT_READY;
  }

  //
  // Check whether this keystroke follows a Non-spacing key
  //
  if (UsbKeyboardDevice->CurrentNsKey != NULL) {
    KeyDescriptor = FindPhysicalKey (UsbKeyboardDevice->CurrentNsKey, KeyDescriptor);
    UsbKeyboardDevice->CurrentNsKey = NULL;
  }

  Key->ScanCode = EfiScanCodeConvertionTable[KeyDescriptor->Modifier];
  Key->UnicodeChar = KeyDescriptor->Unicode;

  if (KeyDescriptor->AffectedAttribute & EFI_AFFECTED_BY_STANDARD_SHIFT) {
    if (UsbKeyboardDevice->ShiftOn != 0) {
      Key->UnicodeChar = KeyDescriptor->ShiftedUnicode;

      //
      // Need not return associated shift state if a class of printable characters that
      // are normally adjusted by shift modifiers. e.g. Shift Key + 'f' key = 'F'
      //
      if (KeyDescriptor->AffectedAttribute & EFI_AFFECTED_BY_CAPS_LOCK) {
        UsbKeyboardDevice->LeftShiftOn = 0;
        UsbKeyboardDevice->RightShiftOn = 0;
      }

      if (UsbKeyboardDevice->AltGrOn != 0) {
        Key->UnicodeChar = KeyDescriptor->ShiftedAltGrUnicode;
      }
    } else {
      //
      // Shift off
      //
      Key->UnicodeChar = KeyDescriptor->Unicode;

      if (UsbKeyboardDevice->AltGrOn != 0) {
        Key->UnicodeChar = KeyDescriptor->AltGrUnicode;
      }
    }
  }

  if (KeyDescriptor->AffectedAttribute & EFI_AFFECTED_BY_CAPS_LOCK) {
    if (UsbKeyboardDevice->CapsOn != 0) {

      if (Key->UnicodeChar == KeyDescriptor->Unicode) {

        Key->UnicodeChar = KeyDescriptor->ShiftedUnicode;

      } else if (Key->UnicodeChar == KeyDescriptor->ShiftedUnicode) {

        Key->UnicodeChar = KeyDescriptor->Unicode;

      }
    }
  }

  //
  // Translate the CTRL-Alpha characters to their corresponding control value  (ctrl-a = 0x0001 through ctrl-Z = 0x001A)
  //
  if (UsbKeyboardDevice->CtrlOn != 0) {
    if (Key->UnicodeChar >= 'a' && Key->UnicodeChar <= 'z') {
      Key->UnicodeChar = (UINT8) (Key->UnicodeChar - 'a' + 1);
    } else if (Key->UnicodeChar >= 'A' && Key->UnicodeChar <= 'Z') {
      Key->UnicodeChar = (UINT8) (Key->UnicodeChar - 'A' + 1);
    }
  }

  if (KeyDescriptor->AffectedAttribute & EFI_AFFECTED_BY_NUM_LOCK) {

    if ((UsbKeyboardDevice->NumLockOn != 0) && (UsbKeyboardDevice->ShiftOn == 0)) {

      Key->ScanCode = SCAN_NULL;

    } else {
      Key->UnicodeChar = 0x00;
    }
  }

  //
  // Translate Unicode 0x1B (ESC) to EFI Scan Code
  //
  if (Key->UnicodeChar == 0x1B && Key->ScanCode == SCAN_NULL) {
    Key->ScanCode = SCAN_ESC;
    Key->UnicodeChar = 0x00;
  }

  if (Key->UnicodeChar == 0 && Key->ScanCode == SCAN_NULL) {
    return EFI_NOT_READY;
  }


  //
  // Save Shift/Toggle state
  //
  if (UsbKeyboardDevice->LeftCtrlOn == 1) {
    UsbKeyboardDevice->KeyState.KeyShiftState |= EFI_LEFT_CONTROL_PRESSED;
  }
  if (UsbKeyboardDevice->RightCtrlOn == 1) {
    UsbKeyboardDevice->KeyState.KeyShiftState |= EFI_RIGHT_CONTROL_PRESSED;
  }
  if (UsbKeyboardDevice->LeftAltOn == 1) {
    UsbKeyboardDevice->KeyState.KeyShiftState |= EFI_LEFT_ALT_PRESSED;
  }
  if (UsbKeyboardDevice->RightAltOn == 1) {
    UsbKeyboardDevice->KeyState.KeyShiftState |= EFI_RIGHT_ALT_PRESSED;
  }
  if (UsbKeyboardDevice->LeftShiftOn == 1) {
    UsbKeyboardDevice->KeyState.KeyShiftState |= EFI_LEFT_SHIFT_PRESSED;
  }
  if (UsbKeyboardDevice->RightShiftOn == 1) {
    UsbKeyboardDevice->KeyState.KeyShiftState |= EFI_RIGHT_SHIFT_PRESSED;
  }
  if (UsbKeyboardDevice->LeftLogoOn == 1) {
    UsbKeyboardDevice->KeyState.KeyShiftState |= EFI_LEFT_LOGO_PRESSED;
  }
  if (UsbKeyboardDevice->RightLogoOn == 1) {
    UsbKeyboardDevice->KeyState.KeyShiftState |= EFI_RIGHT_LOGO_PRESSED;
  }
  if (UsbKeyboardDevice->MenuKeyOn == 1) {
    UsbKeyboardDevice->KeyState.KeyShiftState |= EFI_MENU_KEY_PRESSED;
  }
  if (UsbKeyboardDevice->SysReqOn == 1) {
    UsbKeyboardDevice->KeyState.KeyShiftState |= EFI_SYS_REQ_PRESSED;
  }

  if (UsbKeyboardDevice->ScrollOn == 1) {
    UsbKeyboardDevice->KeyState.KeyToggleState |= EFI_SCROLL_LOCK_ACTIVE;
  }
  if (UsbKeyboardDevice->NumLockOn == 1) {
    UsbKeyboardDevice->KeyState.KeyToggleState |= EFI_NUM_LOCK_ACTIVE;
  }
  if (UsbKeyboardDevice->CapsOn == 1) {
    UsbKeyboardDevice->KeyState.KeyToggleState |= EFI_CAPS_LOCK_ACTIVE;
  }

  return EFI_SUCCESS;

}


/**
  Resets USB Keyboard Buffer.

  @param  KeyboardBuffer     Points to the USB Keyboard Buffer.

  @retval EFI_SUCCESS        Init key buffer successfully.

**/
EFI_STATUS
EFIAPI
InitUSBKeyBuffer (
  IN OUT  USB_KB_BUFFER   *KeyboardBuffer
  )
{
  ZeroMem (KeyboardBuffer, sizeof (USB_KB_BUFFER));

  KeyboardBuffer->bHead = KeyboardBuffer->bTail;

  return EFI_SUCCESS;
}


/**
  Check whether USB Keyboard buffer is empty.

  @param  KeyboardBuffer     USB Keyboard Buffer.

  @retval TRUE               Key buffer is empty.
  @retval FALSE              Key buffer is not empty.

**/
BOOLEAN
EFIAPI
IsUSBKeyboardBufferEmpty (
  IN  USB_KB_BUFFER   *KeyboardBuffer
  )
{
  //
  // meet FIFO empty condition
  //
  return (BOOLEAN) (KeyboardBuffer->bHead == KeyboardBuffer->bTail);
}


/**
  Check whether USB Keyboard buffer is full.

  @param  KeyboardBuffer     USB Keyboard Buffer.

  @retval TRUE               Key buffer is full.
  @retval FALSE              Key buffer is not full.

**/
BOOLEAN
EFIAPI
IsUSBKeyboardBufferFull (
  IN  USB_KB_BUFFER   *KeyboardBuffer
  )
{
  return (BOOLEAN)(((KeyboardBuffer->bTail + 1) % (MAX_KEY_ALLOWED + 1)) ==
                                                        KeyboardBuffer->bHead);
}


/**
  Inserts a key code into keyboard buffer.

  @param  KeyboardBuffer     Points to the USB Keyboard Buffer.
  @param  Key                Key code
  @param  Down               Special key

  @retval EFI_SUCCESS        Success

**/
EFI_STATUS
EFIAPI
InsertKeyCode (
  IN OUT  USB_KB_BUFFER *KeyboardBuffer,
  IN      UINT8         Key,
  IN      UINT8         Down
  )
{
  USB_KEY UsbKey;

  //
  // if keyboard buffer is full, throw the
  // first key out of the keyboard buffer.
  //
  if (IsUSBKeyboardBufferFull (KeyboardBuffer)) {
    RemoveKeyCode (KeyboardBuffer, &UsbKey);
  }

  KeyboardBuffer->buffer[KeyboardBuffer->bTail].KeyCode = Key;
  KeyboardBuffer->buffer[KeyboardBuffer->bTail].Down    = Down;

  //
  // adjust the tail pointer of the FIFO keyboard buffer.
  //
  KeyboardBuffer->bTail = (UINT8) ((KeyboardBuffer->bTail + 1) % (MAX_KEY_ALLOWED + 1));

  return EFI_SUCCESS;
}


/**
  Pops a key code off from keyboard buffer.

  @param  KeyboardBuffer     Points to the USB Keyboard Buffer.
  @param  UsbKey             Points to the buffer that contains a usb key code.

  @retval EFI_SUCCESS        Success
  @retval EFI_DEVICE_ERROR   Hardware Error

**/
EFI_STATUS
EFIAPI
RemoveKeyCode (
  IN OUT  USB_KB_BUFFER *KeyboardBuffer,
  OUT     USB_KEY       *UsbKey
  )
{
  if (IsUSBKeyboardBufferEmpty (KeyboardBuffer)) {
    return EFI_DEVICE_ERROR;
  }

  UsbKey->KeyCode = KeyboardBuffer->buffer[KeyboardBuffer->bHead].KeyCode;
  UsbKey->Down    = KeyboardBuffer->buffer[KeyboardBuffer->bHead].Down;

  //
  // adjust the head pointer of the FIFO keyboard buffer.
  //
  KeyboardBuffer->bHead = (UINT8) ((KeyboardBuffer->bHead + 1) % (MAX_KEY_ALLOWED + 1));

  return EFI_SUCCESS;
}


/**
  Sets USB Keyboard LED state.

  @param  UsbKeyboardDevice  The USB_KB_DEV instance.

  @retval EFI_SUCCESS        Success

**/
EFI_STATUS
EFIAPI
SetKeyLED (
  IN  USB_KB_DEV    *UsbKeyboardDevice
  )
{
  LED_MAP Led;
  UINT8   ReportId;

  //
  // Set each field in Led map.
  //
  Led.NumLock    = (UINT8) UsbKeyboardDevice->NumLockOn;
  Led.CapsLock   = (UINT8) UsbKeyboardDevice->CapsOn;
  Led.ScrollLock = (UINT8) UsbKeyboardDevice->ScrollOn;
  Led.Resrvd     = 0;

  ReportId       = 0;
  //
  // call Set Report Request to lighten the LED.
  //
  UsbSetReportRequest (
    UsbKeyboardDevice->UsbIo,
    UsbKeyboardDevice->InterfaceDescriptor.InterfaceNumber,
    ReportId,
    HID_OUTPUT_REPORT,
    1,
    (UINT8 *) &Led
    );

  return EFI_SUCCESS;
}


/**
  Timer handler for Repeat Key timer.

  @param  Event              The Repeat Key event.
  @param  Context            Points to the USB_KB_DEV instance.


**/
VOID
EFIAPI
USBKeyboardRepeatHandler (
  IN    EFI_EVENT    Event,
  IN    VOID         *Context
  )
{
  USB_KB_DEV  *UsbKeyboardDevice;

  UsbKeyboardDevice = (USB_KB_DEV *) Context;

  //
  // Do nothing when there is no repeat key.
  //
  if (UsbKeyboardDevice->RepeatKey != 0) {
    //
    // Inserts one Repeat key into keyboard buffer,
    //
    InsertKeyCode (
      &(UsbKeyboardDevice->KeyboardBuffer),
      UsbKeyboardDevice->RepeatKey,
      1
      );

    //
    // set repeate rate for repeat key generation.
    //
    gBS->SetTimer (
          UsbKeyboardDevice->RepeatTimer,
          TimerRelative,
          USBKBD_REPEAT_RATE
          );

  }
}


/**
  Timer handler for Delayed Recovery timer.

  @param  Event              The Delayed Recovery event.
  @param  Context            Points to the USB_KB_DEV instance.


**/
VOID
EFIAPI
USBKeyboardRecoveryHandler (
  IN    EFI_EVENT    Event,
  IN    VOID         *Context
  )
{

  USB_KB_DEV          *UsbKeyboardDevice;
  EFI_USB_IO_PROTOCOL *UsbIo;
  UINT8               PacketSize;

  UsbKeyboardDevice = (USB_KB_DEV *) Context;

  UsbIo             = UsbKeyboardDevice->UsbIo;

  PacketSize        = (UINT8) (UsbKeyboardDevice->IntEndpointDescriptor.MaxPacketSize);

  UsbIo->UsbAsyncInterruptTransfer (
          UsbIo,
          UsbKeyboardDevice->IntEndpointDescriptor.EndpointAddress,
          TRUE,
          UsbKeyboardDevice->IntEndpointDescriptor.Interval,
          PacketSize,
          KeyboardHandler,
          UsbKeyboardDevice
          );
}
