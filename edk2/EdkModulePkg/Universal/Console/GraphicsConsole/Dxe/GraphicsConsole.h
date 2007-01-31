/*++

Copyright (c) 2006, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  GraphicsConsole.h

Abstract:

  
Revision History

--*/

#ifndef _GRAPHICS_CONSOLE_H
#define _GRAPHICS_CONSOLE_H


#include "ComponentName.h"

//
// Glyph database
//
#define GLYPH_WIDTH   8
#define GLYPH_HEIGHT  19

typedef union {
  EFI_NARROW_GLYPH  NarrowGlyph;
  EFI_WIDE_GLYPH    WideGlyph;
} GLYPH_UNION;

extern EFI_NARROW_GLYPH UsStdNarrowGlyphData[];
extern EFI_WIDE_GLYPH   UsStdWideGlyphData[];

//
// Device Structure
//
#define GRAPHICS_CONSOLE_DEV_SIGNATURE  EFI_SIGNATURE_32 ('g', 's', 't', 'o')

typedef struct {
  UINTN   Columns;
  UINTN   Rows;
  INTN    DeltaX;
  INTN    DeltaY;
  UINT32  GopWidth;
  UINT32  GopHeight;
  UINT32  GopModeNumber;
} GRAPHICS_CONSOLE_MODE_DATA;

#define GRAPHICS_MAX_MODE 3

typedef struct {
  UINTN                         Signature;
  EFI_GRAPHICS_OUTPUT_PROTOCOL  *GraphicsOutput;
  EFI_UGA_DRAW_PROTOCOL         *UgaDraw;
  EFI_SIMPLE_TEXT_OUT_PROTOCOL  SimpleTextOutput;
  EFI_SIMPLE_TEXT_OUTPUT_MODE   SimpleTextOutputMode;
  GRAPHICS_CONSOLE_MODE_DATA    ModeData[GRAPHICS_MAX_MODE];
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL *LineBuffer;
  EFI_HII_HANDLE                HiiHandle;
} GRAPHICS_CONSOLE_DEV;

#define GRAPHICS_CONSOLE_CON_OUT_DEV_FROM_THIS(a) \
  CR (a, GRAPHICS_CONSOLE_DEV, SimpleTextOutput, GRAPHICS_CONSOLE_DEV_SIGNATURE)

//
// Global Variables
//
extern EFI_DRIVER_BINDING_PROTOCOL  gGraphicsConsoleDriverBinding;

//
// Prototypes
//
UINTN
ReturnNarrowFontSize (
  VOID
  );

UINTN
ReturnWideFontSize (
  VOID
  );

EFI_STATUS
EFIAPI
GraphicsConsoleConOutReset (
  IN  EFI_SIMPLE_TEXT_OUT_PROTOCOL    *This,
  IN  BOOLEAN                         ExtendedVerification
  );

EFI_STATUS
EFIAPI
GraphicsConsoleConOutOutputString (
  IN  EFI_SIMPLE_TEXT_OUT_PROTOCOL  *This,
  IN  CHAR16                        *WString
  );

EFI_STATUS
EFIAPI
GraphicsConsoleConOutTestString (
  IN  EFI_SIMPLE_TEXT_OUT_PROTOCOL  *This,
  IN  CHAR16                        *WString
  );

EFI_STATUS
EFIAPI
GraphicsConsoleConOutQueryMode (
  IN  EFI_SIMPLE_TEXT_OUT_PROTOCOL  *This,
  IN  UINTN                         ModeNumber,
  OUT UINTN                         *Columns,
  OUT UINTN                         *Rows
  );

EFI_STATUS
EFIAPI
GraphicsConsoleConOutSetMode (
  IN  EFI_SIMPLE_TEXT_OUT_PROTOCOL  *This,
  IN  UINTN                         ModeNumber
  );

EFI_STATUS
EFIAPI
GraphicsConsoleConOutSetAttribute (
  IN  EFI_SIMPLE_TEXT_OUT_PROTOCOL  *This,
  IN  UINTN                         Attribute
  );

EFI_STATUS
EFIAPI
GraphicsConsoleConOutClearScreen (
  IN  EFI_SIMPLE_TEXT_OUT_PROTOCOL  *This
  );

EFI_STATUS
EFIAPI
GraphicsConsoleConOutSetCursorPosition (
  IN  EFI_SIMPLE_TEXT_OUT_PROTOCOL  *This,
  IN  UINTN                         Column,
  IN  UINTN                         Row
  );

EFI_STATUS
EFIAPI
GraphicsConsoleConOutEnableCursor (
  IN  EFI_SIMPLE_TEXT_OUT_PROTOCOL  *This,
  IN  BOOLEAN                       Visible
  );

EFI_STATUS
EfiLocateHiiProtocol (
  VOID
  );

EFI_STATUS
EFIAPI
GraphicsConsoleControllerDriverSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL    *This,
  IN EFI_HANDLE                     Controller,
  IN EFI_DEVICE_PATH_PROTOCOL       *RemainingDevicePath
  );

EFI_STATUS
EFIAPI
GraphicsConsoleControllerDriverStart (
  IN EFI_DRIVER_BINDING_PROTOCOL    *This,
  IN EFI_HANDLE                     Controller,
  IN EFI_DEVICE_PATH_PROTOCOL       *RemainingDevicePath
  );

EFI_STATUS
EFIAPI
GraphicsConsoleControllerDriverStop (
  IN  EFI_DRIVER_BINDING_PROTOCOL    *This,
  IN  EFI_HANDLE                     Controller,
  IN  UINTN                          NumberOfChildren,
  IN  EFI_HANDLE                     *ChildHandleBuffer
  );

#endif
