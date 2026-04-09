/** @file
  Install a fake VGABIOS service handler (real mode Int10h) for the buggy
  Windows 2008 R2 SP1 UEFI guest.

  The handler is never meant to be directly executed by a VCPU; it's there for
  the internal real mode emulator of Windows 2008 R2 SP1.

  The code is based on Ralf Brown's Interrupt List:
  <http://www.cs.cmu.edu/~ralf/files.html>
  <http://www.ctyme.com/rbrown.htm>

  Copyright (C) 2020, Rebecca Cran <rebecca@bsdio.com>
  Copyright (C) 2015, Nahanni Systems, Inc.
  Copyright (C) 2014, Red Hat, Inc.
  Copyright (c) 2013 - 2014, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <IndustryStandard/LegacyVgaBios.h>
#include <Library/DebugLib.h>
#include <Library/PciLib.h>
#include <Library/PrintLib.h>

#include "Gop.h"
#include "VbeShim.h"

#pragma pack (1)
typedef struct {
  UINT16    Offset;
  UINT16    Segment;
} IVT_ENTRY;
#pragma pack ()

//
// This string is displayed by Windows 2008 R2 SP1 in the Screen Resolution,
// Advanced Settings dialog. It should be short.
//
STATIC CONST CHAR8  mProductRevision[] = "2.0";

#define NUM_VBE_MODES  3
STATIC CONST UINT16  vbeModeIds[] = {
  0x13f,  // 640x480x32
  0x140,  // 800x600x32
  0x141   // 1024x768x32
};

// Modes can be toggled with bit-0
#define VBE_MODE_ENABLED   0x00BB
#define VBE_MODE_DISABLED  0x00BA

STATIC VBE2_MODE_INFO  vbeModes[] = {
  { // 0x13f 640x480x32
    // ModeAttr - BytesPerScanLine
    VBE_MODE_DISABLED, 0x07,   0x00, 0x40, 0x40, 0xA000, 0x00, 0x0000, 640*4,
    // Width, Height..., Vbe3
    640,               480,    16,   8,    1,    32,     1,    0x06,   0,     0,  1,
    // Masks
    0x08,              0x10,   0x08, 0x08, 0x08, 0x00,   0x08, 0x18,   0x00,
    // Framebuffer
    0xdeadbeef,        0x0000, 0x0000
  },
  { // 0x140 800x600x32
    // ModeAttr - BytesPerScanLine
    VBE_MODE_DISABLED, 0x07,   0x00, 0x40, 0x40, 0xA000, 0x00, 0x0000, 800*4,
    // Width, Height..., Vbe3
    800,               600,    16,   8,    1,    32,     1,    0x06,   0,     0,  1,
    // Masks
    0x08,              0x10,   0x08, 0x08, 0x08, 0x00,   0x08, 0x18,   0x00,
    // Framebuffer
    0xdeadbeef,        0x0000, 0x0000
  },
  { // 0x141 1024x768x32
    // ModeAttr - BytesPerScanLine
    VBE_MODE_ENABLED,  0x07,   0x00, 0x40, 0x40, 0xA000, 0x00, 0x0000, 1024*4,
    // Width, Height..., Vbe3
    1024,              768,    16,   8,    1,    32,     1,    0x06,   0,     0,  1,
    // Masks
    0x08,              0x10,   0x08, 0x08, 0x08, 0x00,   0x08, 0x18,   0x00,
    // Framebuffer
    0xdeadbeef,        0x0000, 0x0000
  }
};

/**
  Install the VBE Info and VBE Mode Info structures, and the VBE service
  handler routine in the C segment. Point the real-mode Int10h interrupt vector
  to the handler. The only advertised mode is 1024x768x32.

  @param[in] CardName         Name of the video card to be exposed in the
                              Product Name field of the VBE Info structure.
  @param[in] FrameBufferBase  Guest-physical base address of the video card's
                              frame buffer.
**/
VOID
InstallVbeShim (
  IN CONST CHAR16          *CardName,
  IN EFI_PHYSICAL_ADDRESS  FrameBufferBase
  )
{
  EFI_PHYSICAL_ADDRESS  Segment0, SegmentC, SegmentF;
  UINTN                 Segment0Pages;
  IVT_ENTRY             *Int0x10;
  EFI_STATUS            Status;
  UINTN                 Pam1Address;
  UINT8                 Pam1;
  UINTN                 SegmentCPages;
  VBE_INFO              *VbeInfoFull;
  VBE_INFO_BASE         *VbeInfo;
  UINT8                 *Ptr;
  UINTN                 Printed;
  VBE_MODE_INFO         *VbeModeInfo;
  UINTN                 i;

  Segment0 = 0x00000;
  SegmentC = 0xC0000;
  SegmentF = 0xF0000;

  //
  // Attempt to cover the real mode IVT with an allocation. This is a UEFI
  // driver, hence the arch protocols have been installed previously. Among
  // those, the CPU arch protocol has configured the IDT, so we can overwrite
  // the IVT used in real mode.
  //
  // The allocation request may fail, eg. if LegacyBiosDxe has already run.
  //
  Segment0Pages = 1;
  Int0x10       = (IVT_ENTRY *)(UINTN)Segment0 + 0x10;
  Status        = gBS->AllocatePages (
                         AllocateAddress,
                         EfiBootServicesCode,
                         Segment0Pages,
                         &Segment0
                         );

  if (EFI_ERROR (Status)) {
    EFI_PHYSICAL_ADDRESS  Handler;

    //
    // Check if a video BIOS handler has been installed previously -- we
    // shouldn't override a real video BIOS with our shim, nor our own shim if
    // it's already present.
    //
    Handler = (Int0x10->Segment << 4) + Int0x10->Offset;
    if ((Handler >= SegmentC) && (Handler < SegmentF)) {
      DEBUG ((
        DEBUG_VERBOSE,
        "%a: Video BIOS handler found at %04x:%04x\n",
        __func__,
        Int0x10->Segment,
        Int0x10->Offset
        ));
      return;
    }

    //
    // Otherwise we'll overwrite the Int10h vector, even though we may not own
    // the page at zero.
    //
    DEBUG ((
      DEBUG_VERBOSE,
      "%a: failed to allocate page at zero: %r\n",
      __func__,
      Status
      ));
  } else {
    //
    // We managed to allocate the page at zero. SVN r14218 guarantees that it
    // is NUL-filled.
    //
    ASSERT (Int0x10->Segment == 0x0000);
    ASSERT (Int0x10->Offset  == 0x0000);
  }

  //
  // Put the shim in place first.
  //
  Pam1Address = PCI_LIB_ADDRESS (0, 0, 0, 0x5A);
  //
  // low nibble covers 0xC0000 to 0xC3FFF
  // high nibble covers 0xC4000 to 0xC7FFF
  // bit1 in each nibble is Write Enable
  // bit0 in each nibble is Read Enable
  //
  Pam1 = PciRead8 (Pam1Address);
  PciWrite8 (Pam1Address, Pam1 | (BIT1 | BIT0));

  //
  // We never added memory space durig PEI or DXE for the C segment, so we
  // don't need to (and can't) allocate from there. Also, guest operating
  // systems will see a hole in the UEFI memory map there.
  //
  SegmentCPages = 4;

  ASSERT (sizeof mVbeShim <= EFI_PAGES_TO_SIZE (SegmentCPages));
  CopyMem ((VOID *)(UINTN)SegmentC, mVbeShim, sizeof mVbeShim);

  //
  // Fill in the VBE INFO structure.
  //
  VbeInfoFull = (VBE_INFO *)(UINTN)SegmentC;
  VbeInfo     = &VbeInfoFull->Base;
  Ptr         = VbeInfoFull->Buffer;

  CopyMem (VbeInfo->Signature, "VESA", 4);
  VbeInfo->VesaVersion = 0x0200;

  VbeInfo->OemNameAddress = (UINT32)SegmentC << 12 | (UINT16)((UINTN)Ptr-SegmentC);
  CopyMem (Ptr, "FBSD", 5);
  Ptr += 5;

  VbeInfo->Capabilities = BIT1 | BIT0; // DAC can be switched into 8-bit mode

  VbeInfo->ModeListAddress = (UINT32)SegmentC << 12 | (UINT16)((UINTN)Ptr-SegmentC);
  for (i = 0; i < NUM_VBE_MODES; i++) {
    *(UINT16 *)Ptr = vbeModeIds[i];  // mode number
    Ptr           += 2;
  }

  *(UINT16 *)Ptr = 0xFFFF; // mode list terminator
  Ptr           += 2;

  VbeInfo->VideoMem64K        = (UINT16)((1024 * 768 * 4 + 65535) / 65536);
  VbeInfo->OemSoftwareVersion = 0x0200;

  VbeInfo->VendorNameAddress = (UINT32)SegmentC << 12 | (UINT16)((UINTN)Ptr-SegmentC);
  CopyMem (Ptr, "FBSD", 5);
  Ptr += 5;

  VbeInfo->ProductNameAddress = (UINT32)SegmentC << 12 | (UINT16)((UINTN)Ptr-SegmentC);
  Printed                     = AsciiSPrint (
                                  (CHAR8 *)Ptr,
                                  sizeof VbeInfoFull->Buffer - (Ptr - VbeInfoFull->Buffer),
                                  "%s",
                                  CardName
                                  );
  Ptr += Printed + 1;

  VbeInfo->ProductRevAddress = (UINT32)SegmentC << 12 | (UINT16)((UINTN)Ptr-SegmentC);
  CopyMem (Ptr, mProductRevision, sizeof mProductRevision);
  Ptr += sizeof mProductRevision;

  ASSERT (sizeof VbeInfoFull->Buffer >= Ptr - VbeInfoFull->Buffer);
  ZeroMem (Ptr, sizeof VbeInfoFull->Buffer - (Ptr - VbeInfoFull->Buffer));

  //
  // Fill in the VBE MODE INFO structure list
  //
  VbeModeInfo = (VBE_MODE_INFO *)(VbeInfoFull + 1);
  Ptr         = (UINT8 *)VbeModeInfo;
  for (i = 0; i < NUM_VBE_MODES; i++) {
    vbeModes[i].LfbAddress = (UINT32)FrameBufferBase;
    CopyMem (Ptr, &vbeModes[i], 0x32);
    Ptr += 0x32;
  }

  ZeroMem (Ptr, 56);     // Clear remaining bytes

  //
  // Clear Write Enable (bit1), keep Read Enable (bit0) set
  //
  PciWrite8 (Pam1Address, (Pam1 & ~BIT1) | BIT0);

  //
  // Second, point the Int10h vector at the shim.
  //
  Int0x10->Segment = (UINT16)((UINT32)SegmentC >> 4);
  Int0x10->Offset  = (UINT16)((UINTN)(VbeModeInfo + 1) - SegmentC);

  DEBUG ((
    DEBUG_INFO,
    "%a: VBE shim installed to %x:%x\n",
    __func__,
    Int0x10->Segment,
    Int0x10->Offset
    ));
}
