/** @file

  Copyright (c) 2011, ARM Ltd. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <PiDxe.h>

#include <Library/DebugLib.h>
#include <Library/IoLib.h>
#include <Library/LcdPlatformLib.h>
#include <Library/UefiBootServicesTableLib.h>

#include <Protocol/EdidDiscovered.h>
#include <Protocol/EdidActive.h>

#include <Drivers/PL111Lcd.h>

#include <ArmPlatform.h>

typedef struct {
  UINT32                     Mode;
  UINT32                     HorizontalResolution;
  UINT32                     VerticalResolution;
  LCD_BPP                    Bpp;
  UINT32                     ClcdClk;

  UINT32                     HSync;
  UINT32                     HBackPorch;
  UINT32                     HFrontPorch;
  UINT32                     VSync;
  UINT32                     VBackPorch;
  UINT32                     VFrontPorch;
} CLCD_RESOLUTION;


CLCD_RESOLUTION mResolutions[] = {
  { // Mode 0 : VGA : 640 x 480 x 24 bpp
      VGA, VGA_H_RES_PIXELS, VGA_V_RES_PIXELS, LCD_BITS_PER_PIXEL_24, 0x2C77,
      VGA_H_SYNC, VGA_H_BACK_PORCH, VGA_H_FRONT_PORCH,
      VGA_V_SYNC, VGA_V_BACK_PORCH, VGA_V_FRONT_PORCH
  },
  { // Mode 1 : SVGA : 800 x 600 x 24 bpp
      SVGA, SVGA_H_RES_PIXELS, SVGA_V_RES_PIXELS, LCD_BITS_PER_PIXEL_24, 0x2CAC,
      SVGA_H_SYNC, SVGA_H_BACK_PORCH, SVGA_H_FRONT_PORCH,
      SVGA_V_SYNC, SVGA_V_BACK_PORCH, SVGA_V_FRONT_PORCH
  }
};

EFI_EDID_DISCOVERED_PROTOCOL  mEdidDiscovered = {
  0,
  NULL
};

EFI_EDID_ACTIVE_PROTOCOL      mEdidActive = {
  0,
  NULL
};

EFI_STATUS
LcdPlatformInitializeDisplay (
  IN EFI_HANDLE   Handle
  )
{
  EFI_STATUS  Status;

  MmioWrite32(ARM_EB_SYS_CLCD_REG, 1);

  // Install the EDID Protocols
  Status = gBS->InstallMultipleProtocolInterfaces(
    &Handle,
    &gEfiEdidDiscoveredProtocolGuid,  &mEdidDiscovered,
    &gEfiEdidActiveProtocolGuid,      &mEdidActive,
    NULL
  );

  return Status;
}

EFI_STATUS
LcdPlatformGetVram (
  OUT EFI_PHYSICAL_ADDRESS*  VramBaseAddress,
  OUT UINTN*                 VramSize
  )
{
  *VramBaseAddress = PL111_CLCD_VRAM_BASE;
  *VramSize = SIZE_8MB; //FIXME: Can this size change ?
  return EFI_SUCCESS;
}

UINT32
LcdPlatformGetMaxMode (
  VOID
  )
{
  return (sizeof(mResolutions) / sizeof(CLCD_RESOLUTION));
}

EFI_STATUS
LcdPlatformSetMode (
  IN UINT32                         ModeNumber
  )
{
  if (ModeNumber >= LcdPlatformGetMaxMode ()) {
    return EFI_INVALID_PARAMETER;
  }

  MmioWrite32(ARM_EB_SYS_LOCK_REG,0x0000A05F);
  MmioWrite32(ARM_EB_SYS_OSC4_REG,mResolutions[ModeNumber].ClcdClk);
  MmioWrite32(ARM_EB_SYS_LOCK_REG,0x0);

  return EFI_SUCCESS;
}

EFI_STATUS
LcdPlatformQueryMode (
  IN  UINT32                                ModeNumber,
  OUT EFI_GRAPHICS_OUTPUT_MODE_INFORMATION  *Info
  )
{
  EFI_STATUS    Status;

  if (ModeNumber >= LcdPlatformGetMaxMode ()) {
    return EFI_INVALID_PARAMETER;
  }

  Status = EFI_UNSUPPORTED;

  Info->Version = 0;
  Info->HorizontalResolution = mResolutions[ModeNumber].HorizontalResolution;
  Info->VerticalResolution = mResolutions[ModeNumber].VerticalResolution;
  Info->PixelsPerScanLine = mResolutions[ModeNumber].HorizontalResolution;

  switch (mResolutions[ModeNumber].Bpp) {
    case LCD_BITS_PER_PIXEL_24:
      Info->PixelFormat                   = PixelRedGreenBlueReserved8BitPerColor;
      Info->PixelInformation.RedMask      = LCD_24BPP_RED_MASK;
      Info->PixelInformation.GreenMask    = LCD_24BPP_GREEN_MASK;
      Info->PixelInformation.BlueMask     = LCD_24BPP_BLUE_MASK;
      Info->PixelInformation.ReservedMask = LCD_24BPP_RESERVED_MASK;
      Status = EFI_SUCCESS;
      break;

    case LCD_BITS_PER_PIXEL_16_555:
      Info->PixelFormat                   = PixelBitMask;
      Info->PixelInformation.RedMask      = LCD_16BPP_555_RED_MASK;
      Info->PixelInformation.GreenMask    = LCD_16BPP_555_GREEN_MASK;
      Info->PixelInformation.BlueMask     = LCD_16BPP_555_BLUE_MASK;
      Info->PixelInformation.ReservedMask = LCD_16BPP_555_RESERVED_MASK;
      Status = EFI_SUCCESS;
      break;

    case LCD_BITS_PER_PIXEL_16_565:
      Info->PixelFormat                   = PixelBitMask;
      Info->PixelInformation.RedMask      = LCD_16BPP_565_RED_MASK;
      Info->PixelInformation.GreenMask    = LCD_16BPP_565_GREEN_MASK;
      Info->PixelInformation.BlueMask     = LCD_16BPP_565_BLUE_MASK;
      Info->PixelInformation.ReservedMask = LCD_16BPP_565_RESERVED_MASK;
      Status = EFI_SUCCESS;
      break;

    case LCD_BITS_PER_PIXEL_12_444:
      Info->PixelFormat                   = PixelBitMask;
      Info->PixelInformation.RedMask      = LCD_12BPP_444_RED_MASK;
      Info->PixelInformation.GreenMask    = LCD_12BPP_444_GREEN_MASK;
      Info->PixelInformation.BlueMask     = LCD_12BPP_444_BLUE_MASK;
      Info->PixelInformation.ReservedMask = LCD_12BPP_444_RESERVED_MASK;
      Status = EFI_SUCCESS;
      break;

    case LCD_BITS_PER_PIXEL_8:
    case LCD_BITS_PER_PIXEL_4:
    case LCD_BITS_PER_PIXEL_2:
    case LCD_BITS_PER_PIXEL_1:
    default:
      // These are not supported
      break;
  }

  return Status;
}

EFI_STATUS
LcdPlatformGetTimings (
  IN  UINT32                              ModeNumber,
  OUT UINT32*                             HRes,
  OUT UINT32*                             HSync,
  OUT UINT32*                             HBackPorch,
  OUT UINT32*                             HFrontPorch,
  OUT UINT32*                             VRes,
  OUT UINT32*                             VSync,
  OUT UINT32*                             VBackPorch,
  OUT UINT32*                             VFrontPorch
  )
{
  if (ModeNumber >= LcdPlatformGetMaxMode ()) {
    return EFI_INVALID_PARAMETER;
  }

  *HRes           = mResolutions[ModeNumber].HorizontalResolution;
  *HSync          = mResolutions[ModeNumber].HSync;
  *HBackPorch     = mResolutions[ModeNumber].HBackPorch;
  *HFrontPorch    = mResolutions[ModeNumber].HFrontPorch;
  *VRes           = mResolutions[ModeNumber].VerticalResolution;
  *VSync          = mResolutions[ModeNumber].VSync;
  *VBackPorch     = mResolutions[ModeNumber].VBackPorch;
  *VFrontPorch    = mResolutions[ModeNumber].VFrontPorch;

  return EFI_SUCCESS;
}

EFI_STATUS
LcdPlatformGetBpp (
  IN  UINT32                              ModeNumber,
  OUT LCD_BPP  *                          Bpp
  )
{
  if (ModeNumber >= LcdPlatformGetMaxMode ()) {
    return EFI_INVALID_PARAMETER;
  }

  *Bpp = mResolutions[ModeNumber].Bpp;

  return EFI_SUCCESS;
}
