/** @file

  Copyright (c) 2011-2015, ARM Ltd. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <PiDxe.h>

#include <Library/ArmPlatformSysConfigLib.h>
#include <Library/IoLib.h>
#include <Library/PcdLib.h>
#include <Library/DebugLib.h>
#include <Library/LcdPlatformLib.h>
#include <Library/UefiBootServicesTableLib.h>

#include <Protocol/Cpu.h>
#include <Protocol/EdidDiscovered.h>
#include <Protocol/EdidActive.h>

#include <ArmPlatform.h>

typedef struct {
  UINT32                     Mode;
  UINT32                     HorizontalResolution;
  UINT32                     VerticalResolution;
  LCD_BPP                    Bpp;
  UINT32                     OscFreq;

  UINT32                     HSync;
  UINT32                     HBackPorch;
  UINT32                     HFrontPorch;
  UINT32                     VSync;
  UINT32                     VBackPorch;
  UINT32                     VFrontPorch;
} LCD_RESOLUTION;


LCD_RESOLUTION mResolutions[] = {
  { // Mode 0 : VGA : 640 x 480 x 24 bpp
      VGA, VGA_H_RES_PIXELS, VGA_V_RES_PIXELS, LCD_BITS_PER_PIXEL_24, VGA_OSC_FREQUENCY,
      VGA_H_SYNC, VGA_H_BACK_PORCH, VGA_H_FRONT_PORCH,
      VGA_V_SYNC, VGA_V_BACK_PORCH, VGA_V_FRONT_PORCH
  },
  { // Mode 1 : SVGA : 800 x 600 x 24 bpp
      SVGA, SVGA_H_RES_PIXELS, SVGA_V_RES_PIXELS, LCD_BITS_PER_PIXEL_24, SVGA_OSC_FREQUENCY,
      SVGA_H_SYNC, SVGA_H_BACK_PORCH, SVGA_H_FRONT_PORCH,
      SVGA_V_SYNC, SVGA_V_BACK_PORCH, SVGA_V_FRONT_PORCH
  },
  { // Mode 2 : XGA : 1024 x 768 x 24 bpp
      XGA, XGA_H_RES_PIXELS, XGA_V_RES_PIXELS, LCD_BITS_PER_PIXEL_24, XGA_OSC_FREQUENCY,
      XGA_H_SYNC, XGA_H_BACK_PORCH, XGA_H_FRONT_PORCH,
      XGA_V_SYNC, XGA_V_BACK_PORCH, XGA_V_FRONT_PORCH
  },
  { // Mode 3 : SXGA : 1280 x 1024 x 24 bpp
      SXGA, SXGA_H_RES_PIXELS, SXGA_V_RES_PIXELS, LCD_BITS_PER_PIXEL_24, (SXGA_OSC_FREQUENCY/2),
      SXGA_H_SYNC, SXGA_H_BACK_PORCH, SXGA_H_FRONT_PORCH,
      SXGA_V_SYNC, SXGA_V_BACK_PORCH, SXGA_V_FRONT_PORCH
  },
  { // Mode 4 : UXGA : 1600 x 1200 x 24 bpp
      UXGA, UXGA_H_RES_PIXELS, UXGA_V_RES_PIXELS, LCD_BITS_PER_PIXEL_24, (UXGA_OSC_FREQUENCY/2),
      UXGA_H_SYNC, UXGA_H_BACK_PORCH, UXGA_H_FRONT_PORCH,
      UXGA_V_SYNC, UXGA_V_BACK_PORCH, UXGA_V_FRONT_PORCH
  },
  { // Mode 5 : HD : 1920 x 1080 x 24 bpp
      HD, HD_H_RES_PIXELS, HD_V_RES_PIXELS, LCD_BITS_PER_PIXEL_24, (HD_OSC_FREQUENCY/2),
      HD_H_SYNC, HD_H_BACK_PORCH, HD_H_FRONT_PORCH,
      HD_V_SYNC, HD_V_BACK_PORCH, HD_V_FRONT_PORCH
  },
  { // Mode 6 : VGA : 640 x 480 x 16 bpp (565 Mode)
      VGA, VGA_H_RES_PIXELS, VGA_V_RES_PIXELS, LCD_BITS_PER_PIXEL_16_565, VGA_OSC_FREQUENCY,
      VGA_H_SYNC, VGA_H_BACK_PORCH, VGA_H_FRONT_PORCH,
      VGA_V_SYNC, VGA_V_BACK_PORCH, VGA_V_FRONT_PORCH
  },
  { // Mode 7 : SVGA : 800 x 600 x 16 bpp (565 Mode)
      SVGA, SVGA_H_RES_PIXELS, SVGA_V_RES_PIXELS, LCD_BITS_PER_PIXEL_16_565, SVGA_OSC_FREQUENCY,
      SVGA_H_SYNC, SVGA_H_BACK_PORCH, SVGA_H_FRONT_PORCH,
      SVGA_V_SYNC, SVGA_V_BACK_PORCH, SVGA_V_FRONT_PORCH
  },
  { // Mode 8 : XGA : 1024 x 768 x 16 bpp (565 Mode)
      XGA, XGA_H_RES_PIXELS, XGA_V_RES_PIXELS, LCD_BITS_PER_PIXEL_16_565, XGA_OSC_FREQUENCY,
      XGA_H_SYNC, XGA_H_BACK_PORCH, XGA_H_FRONT_PORCH,
      XGA_V_SYNC, XGA_V_BACK_PORCH, XGA_V_FRONT_PORCH
  },
  { // Mode 9 : VGA : 640 x 480 x 15 bpp
      VGA, VGA_H_RES_PIXELS, VGA_V_RES_PIXELS, LCD_BITS_PER_PIXEL_16_555, VGA_OSC_FREQUENCY,
      VGA_H_SYNC, VGA_H_BACK_PORCH, VGA_H_FRONT_PORCH,
      VGA_V_SYNC, VGA_V_BACK_PORCH, VGA_V_FRONT_PORCH
  },
  { // Mode 10 : SVGA : 800 x 600 x 15 bpp
      SVGA, SVGA_H_RES_PIXELS, SVGA_V_RES_PIXELS, LCD_BITS_PER_PIXEL_16_555, SVGA_OSC_FREQUENCY,
      SVGA_H_SYNC, SVGA_H_BACK_PORCH, SVGA_H_FRONT_PORCH,
      SVGA_V_SYNC, SVGA_V_BACK_PORCH, SVGA_V_FRONT_PORCH
  },
  { // Mode 11 : XGA : 1024 x 768 x 15 bpp
      XGA, XGA_H_RES_PIXELS, XGA_V_RES_PIXELS, LCD_BITS_PER_PIXEL_16_555, XGA_OSC_FREQUENCY,
      XGA_H_SYNC, XGA_H_BACK_PORCH, XGA_H_FRONT_PORCH,
      XGA_V_SYNC, XGA_V_BACK_PORCH, XGA_V_FRONT_PORCH
  },
  { // Mode 12 : XGA : 1024 x 768 x 15 bpp - All the timing info is derived from Linux Kernel Driver Settings
      XGA, XGA_H_RES_PIXELS, XGA_V_RES_PIXELS, LCD_BITS_PER_PIXEL_16_555, 63500000,
      XGA_H_SYNC, XGA_H_BACK_PORCH, XGA_H_FRONT_PORCH,
      XGA_V_SYNC, XGA_V_BACK_PORCH, XGA_V_FRONT_PORCH
  },
  { // Mode 13 : VGA : 640 x 480 x 12 bpp (444 Mode)
      VGA, VGA_H_RES_PIXELS, VGA_V_RES_PIXELS, LCD_BITS_PER_PIXEL_12_444, VGA_OSC_FREQUENCY,
      VGA_H_SYNC, VGA_H_BACK_PORCH, VGA_H_FRONT_PORCH,
      VGA_V_SYNC, VGA_V_BACK_PORCH, VGA_V_FRONT_PORCH
  },
  { // Mode 14 : SVGA : 800 x 600 x 12 bpp (444 Mode)
      SVGA, SVGA_H_RES_PIXELS, SVGA_V_RES_PIXELS, LCD_BITS_PER_PIXEL_12_444, SVGA_OSC_FREQUENCY,
      SVGA_H_SYNC, SVGA_H_BACK_PORCH, SVGA_H_FRONT_PORCH,
      SVGA_V_SYNC, SVGA_V_BACK_PORCH, SVGA_V_FRONT_PORCH
  },
  { // Mode 15 : XGA : 1024 x 768 x 12 bpp (444 Mode)
      XGA, XGA_H_RES_PIXELS, XGA_V_RES_PIXELS, LCD_BITS_PER_PIXEL_12_444, XGA_OSC_FREQUENCY,
      XGA_H_SYNC, XGA_H_BACK_PORCH, XGA_H_FRONT_PORCH,
      XGA_V_SYNC, XGA_V_BACK_PORCH, XGA_V_FRONT_PORCH
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

  // Set the FPGA multiplexer to select the video output from the motherboard or the daughterboard
  Status = ArmPlatformSysConfigSet (SYS_CFG_MUXFPGA, PL111_CLCD_SITE);
  if (!EFI_ERROR(Status)) {
    // Install the EDID Protocols
    Status = gBS->InstallMultipleProtocolInterfaces(
      &Handle,
      &gEfiEdidDiscoveredProtocolGuid,  &mEdidDiscovered,
      &gEfiEdidActiveProtocolGuid,      &mEdidActive,
      NULL
    );
  }

  return Status;
}

EFI_STATUS
LcdPlatformGetVram (
  OUT EFI_PHYSICAL_ADDRESS*  VramBaseAddress,
  OUT UINTN*                 VramSize
  )
{
  EFI_STATUS              Status;
  EFI_CPU_ARCH_PROTOCOL  *Cpu;

  Status = EFI_SUCCESS;

  // Is it on the motherboard or on the daughterboard?
  switch(PL111_CLCD_SITE) {

  case ARM_VE_MOTHERBOARD_SITE:
    *VramBaseAddress = (EFI_PHYSICAL_ADDRESS) PL111_CLCD_VRAM_MOTHERBOARD_BASE;
    *VramSize = LCD_VRAM_SIZE;
    break;

  case ARM_VE_DAUGHTERBOARD_1_SITE:
    *VramBaseAddress = (EFI_PHYSICAL_ADDRESS) LCD_VRAM_CORE_TILE_BASE;
    *VramSize = LCD_VRAM_SIZE;

    // Allocate the VRAM from the DRAM so that nobody else uses it.
    Status = gBS->AllocatePages( AllocateAddress, EfiBootServicesData, EFI_SIZE_TO_PAGES(((UINTN)LCD_VRAM_SIZE)), VramBaseAddress);
    if (EFI_ERROR(Status)) {
      return Status;
    }

    // Ensure the Cpu architectural protocol is already installed
    Status = gBS->LocateProtocol (&gEfiCpuArchProtocolGuid, NULL, (VOID **)&Cpu);
    ASSERT_EFI_ERROR(Status);

    // Mark the VRAM as un-cachable. The VRAM is inside the DRAM, which is cachable.
    Status = Cpu->SetMemoryAttributes(Cpu, *VramBaseAddress, *VramSize, EFI_MEMORY_UC);
    ASSERT_EFI_ERROR(Status);
    if (EFI_ERROR(Status)) {
      gBS->FreePool(VramBaseAddress);
      return Status;
    }
    break;

  default:
    // Unsupported site
    Status = EFI_UNSUPPORTED;
    break;
  }

  return Status;
}

UINT32
LcdPlatformGetMaxMode (
  VOID
  )
{
  // The following line will report correctly the total number of graphics modes
  // supported by the PL111CLCD.
  //return (sizeof(mResolutions) / sizeof(CLCD_RESOLUTION)) - 1;

  // However, on some platforms it is desirable to ignore some graphics modes.
  // This could be because the specific implementation of PL111 has certain limitations.

  // Set the maximum mode allowed
  return (PcdGet32(PcdPL111LcdMaxMode));
}

EFI_STATUS
LcdPlatformSetMode (
  IN UINT32                         ModeNumber
  )
{
  EFI_STATUS            Status;
  UINT32                LcdSite;
  UINT32                OscillatorId;
  SYS_CONFIG_FUNCTION   Function;
  UINT32                SysId;

  if (ModeNumber >= LcdPlatformGetMaxMode ()) {
    return EFI_INVALID_PARAMETER;
  }

  LcdSite = PL111_CLCD_SITE;

  switch(LcdSite) {
  case ARM_VE_MOTHERBOARD_SITE:
    Function = SYS_CFG_OSC;
    OscillatorId = PL111_CLCD_MOTHERBOARD_VIDEO_MODE_OSC_ID;
    break;
  case ARM_VE_DAUGHTERBOARD_1_SITE:
    Function = SYS_CFG_OSC_SITE1;
    OscillatorId = (UINT32)PcdGet32(PcdPL111LcdVideoModeOscId);
    break;
  default:
    return EFI_UNSUPPORTED;
  }

  // Set the video mode oscillator
  Status = ArmPlatformSysConfigSetDevice (Function, OscillatorId, mResolutions[ModeNumber].OscFreq);
  if (EFI_ERROR(Status)) {
    ASSERT_EFI_ERROR (Status);
    return Status;
  }

  // The FVP foundation model does not have an LCD.
  // On the FVP models the GIC variant in encoded in bits [15:12].
  // Note: The DVI Mode is not modelled by RTSM or FVP models.
  SysId = MmioRead32 (ARM_VE_SYS_ID_REG);
  if (SysId != ARM_RTSM_SYS_ID) {
    // Take out the FVP GIC variant to reduce the permutations.
    SysId &= ~ARM_FVP_SYS_ID_VARIANT_MASK;
    if (SysId != ARM_FVP_BASE_BOARD_SYS_ID) {
      // Set the DVI into the new mode
      Status = ArmPlatformSysConfigSet (SYS_CFG_DVIMODE, mResolutions[ModeNumber].Mode);
      if (EFI_ERROR(Status)) {
        ASSERT_EFI_ERROR (Status);
        return Status;
      }
    }
  }

  // Set the multiplexer
  Status = ArmPlatformSysConfigSet (SYS_CFG_MUXFPGA, LcdSite);
  if (EFI_ERROR(Status)) {
    ASSERT_EFI_ERROR (Status);
    return Status;
  }

  return Status;
}

EFI_STATUS
LcdPlatformQueryMode (
  IN  UINT32                                ModeNumber,
  OUT EFI_GRAPHICS_OUTPUT_MODE_INFORMATION  *Info
  )
{
  if (ModeNumber >= LcdPlatformGetMaxMode ()) {
    return EFI_INVALID_PARAMETER;
  }

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
      break;

    case LCD_BITS_PER_PIXEL_16_555:
    case LCD_BITS_PER_PIXEL_16_565:
    case LCD_BITS_PER_PIXEL_12_444:
    case LCD_BITS_PER_PIXEL_8:
    case LCD_BITS_PER_PIXEL_4:
    case LCD_BITS_PER_PIXEL_2:
    case LCD_BITS_PER_PIXEL_1:
    default:
      // These are not supported
      ASSERT(FALSE);
      break;
  }

  return EFI_SUCCESS;
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
