/** @file

 Copyright (c) 2011, ARM Ltd. All rights reserved.<BR>
 This program and the accompanying materials
 are licensed and made available under the terms and conditions of the BSD License
 which accompanies this distribution.  The full text of the license may be found at
 http://opensource.org/licenses/bsd-license.php

 THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
 WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __OMAP3_DSS_GRAPHICS__
#define __OMAP3_DSS_GRAPHICS__

#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/IoLib.h>

#include <Protocol/DevicePathToText.h>
#include <Protocol/EmbeddedExternalDevice.h>
#include <Protocol/Cpu.h>

#include <Guid/GlobalVariable.h>

#include <Omap3530/Omap3530.h>
#include <TPS65950.h>

typedef struct {
  VENDOR_DEVICE_PATH            Guid;
  EFI_DEVICE_PATH_PROTOCOL      End;
} LCD_GRAPHICS_DEVICE_PATH;

typedef struct {
  UINTN                                 Signature;
  EFI_HANDLE                            Handle;
  EFI_GRAPHICS_OUTPUT_MODE_INFORMATION  ModeInfo;
  EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE     Mode;
  EFI_GRAPHICS_OUTPUT_PROTOCOL          Gop;
  LCD_GRAPHICS_DEVICE_PATH              DevicePath;
//  EFI_EVENT                             ExitBootServicesEvent;
} LCD_INSTANCE;

#define LCD_INSTANCE_SIGNATURE  SIGNATURE_32('l', 'c', 'd', '0')
#define LCD_INSTANCE_FROM_GOP_THIS(a)     CR (a, LCD_INSTANCE, Gop, LCD_INSTANCE_SIGNATURE)

typedef struct {
  UINTN             Mode;
  UINTN             HorizontalResolution;
  UINTN             VerticalResolution;

  UINT32            DssDivisor;
  UINT32            DispcDivisor;

  UINT32            HSync;
  UINT32            HFrontPorch;
  UINT32            HBackPorch;

  UINT32            VSync;
  UINT32            VFrontPorch;
  UINT32            VBackPorch;
} LCD_MODE;

EFI_STATUS
InitializeDisplay (
  IN LCD_INSTANCE* Instance
);

EFI_STATUS
EFIAPI
LcdGraphicsQueryMode (
  IN  EFI_GRAPHICS_OUTPUT_PROTOCOL          *This,
  IN  UINT32                                ModeNumber,
  OUT UINTN                                 *SizeOfInfo,
  OUT EFI_GRAPHICS_OUTPUT_MODE_INFORMATION  **Info
);

EFI_STATUS
EFIAPI
LcdGraphicsSetMode (
  IN EFI_GRAPHICS_OUTPUT_PROTOCOL  *This,
  IN UINT32                        ModeNumber
);

EFI_STATUS
EFIAPI
LcdGraphicsBlt (
  IN EFI_GRAPHICS_OUTPUT_PROTOCOL        *This,
  IN OUT EFI_GRAPHICS_OUTPUT_BLT_PIXEL   *BltBuffer,     OPTIONAL
  IN EFI_GRAPHICS_OUTPUT_BLT_OPERATION   BltOperation,
  IN UINTN                               SourceX,
  IN UINTN                               SourceY,
  IN UINTN                               DestinationX,
  IN UINTN                               DestinationY,
  IN UINTN                               Width,
  IN UINTN                               Height,
  IN UINTN                               Delta           OPTIONAL   // Number of BYTES in a row of the BltBuffer
);

// HW registers
#define CM_FCLKEN_DSS   0x48004E00
#define CM_ICLKEN_DSS   0x48004E10

#define DSS_CONTROL     0x48050040
#define DSS_SYSCONFIG   0x48050010
#define DSS_SYSSTATUS   0x48050014

#define DISPC_CONTROL   0x48050440
#define DISPC_CONFIG    0x48050444
#define DISPC_SIZE_LCD  0x4805047C
#define DISPC_TIMING_H  0x48050464
#define DISPC_TIMING_V  0x48050468

#define CM_CLKSEL_DSS   0x48004E40
#define DISPC_DIVISOR   0x48050470
#define DISPC_POL_FREQ  0x4805046C

#define DISPC_GFX_TABLE_BA 0x480504B8
#define DISPC_GFX_BA0   0x48050480
#define DISPC_GFX_BA1   0x48050484
#define DISPC_GFX_POS   0x48050488
#define DISPC_GFX_SIZE  0x4805048C
#define DISPC_GFX_ATTR  0x480504A0
#define DISPC_GFX_PRELD 0x4805062C

#define DISPC_DEFAULT_COLOR_0 0x4805044C

//#define DISPC_IRQSTATUS

// Bits
#define EN_TV           0x4
#define EN_DSS2         0x2
#define EN_DSS1         0x1
#define EN_DSS          0x1

#define DSS_SOFTRESET   0x2
#define DSS_RESETDONE   0x1

#define BYPASS_MODE     (BIT15 | BIT16)

#define LCDENABLE       BIT0
#define ACTIVEMATRIX    BIT3
#define GOLCD           BIT5
#define DATALINES24     (BIT8 | BIT9)
#define LCDENABLESIGNAL BIT28

#define GFXENABLE       BIT0
#define RGB16           (0x6 << 1)
#define BURSTSIZE16     (0x2 << 6)

#define CLEARLOADMODE   ~(BIT2 | BIT1)
#define LOAD_FRAME_ONLY BIT2

#endif
