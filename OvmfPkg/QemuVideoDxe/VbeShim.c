/** @file
  Install a fake VGABIOS service handler (real mode Int10h) for the buggy
  Windows 2008 R2 SP1 UEFI guest.

  The handler is never meant to be directly executed by a VCPU; it's there for
  the internal real mode emulator of Windows 2008 R2 SP1.

  The code is based on Ralf Brown's Interrupt List:
  <http://www.cs.cmu.edu/~ralf/files.html>
  <http://www.ctyme.com/rbrown.htm>

  Copyright (C) 2014, Red Hat, Inc.
  Copyright (c) 2013 - 2014, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <IndustryStandard/LegacyVgaBios.h>
#include <Library/DebugLib.h>
#include <Library/PciLib.h>
#include <Library/PrintLib.h>
#include <OvmfPlatforms.h>

#include "Qemu.h"
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
STATIC CONST CHAR8  mProductRevision[] = "OVMF Int10h (fake)";

/**
  Install the VBE Info and VBE Mode Info structures, and the VBE service
  handler routine in the C segment. Point the real-mode Int10h interrupt vector
  to the handler. The only advertised mode is 1024x768x32.

  @param[in] CardName         Name of the video card to be exposed in the
                              Product Name field of the VBE Info structure. The
                              parameter must originate from a
                              QEMU_VIDEO_CARD.Name field.
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
  EFI_STATUS            Segment0AllocationStatus;
  UINT16                HostBridgeDevId;
  UINTN                 Pam1Address;
  UINT8                 Pam1;
  UINTN                 SegmentCPages;
  VBE_INFO              *VbeInfoFull;
  VBE_INFO_BASE         *VbeInfo;
  UINT8                 *Ptr;
  UINTN                 Printed;
  VBE_MODE_INFO         *VbeModeInfo;

  if ((PcdGet8 (PcdNullPointerDetectionPropertyMask) & (BIT0|BIT7)) == BIT0) {
    DEBUG ((
      DEBUG_WARN,
      "%a: page 0 protected, not installing VBE shim\n",
      __FUNCTION__
      ));
    DEBUG ((
      DEBUG_WARN,
      "%a: page 0 protection prevents Windows 7 from booting anyway\n",
      __FUNCTION__
      ));
    return;
  }

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
  Segment0Pages            = 1;
  Int0x10                  = (IVT_ENTRY *)(UINTN)(Segment0 + 0x10 * sizeof (IVT_ENTRY));
  Segment0AllocationStatus = gBS->AllocatePages (
                                    AllocateAddress,
                                    EfiBootServicesCode,
                                    Segment0Pages,
                                    &Segment0
                                    );

  if (EFI_ERROR (Segment0AllocationStatus)) {
    EFI_PHYSICAL_ADDRESS  Handler;

    //
    // Check if a video BIOS handler has been installed previously -- we
    // shouldn't override a real video BIOS with our shim, nor our own shim if
    // it's already present.
    //
    Handler = (Int0x10->Segment << 4) + Int0x10->Offset;
    if ((Handler >= SegmentC) && (Handler < SegmentF)) {
      DEBUG ((
        DEBUG_INFO,
        "%a: Video BIOS handler found at %04x:%04x\n",
        __FUNCTION__,
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
      DEBUG_INFO,
      "%a: failed to allocate page at zero: %r\n",
      __FUNCTION__,
      Segment0AllocationStatus
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
  // Start by determining the address of the PAM1 register.
  //
  HostBridgeDevId = PcdGet16 (PcdOvmfHostBridgePciDevId);
  switch (HostBridgeDevId) {
    case INTEL_82441_DEVICE_ID:
      Pam1Address = PMC_REGISTER_PIIX4 (PIIX4_PAM1);
      break;
    case INTEL_Q35_MCH_DEVICE_ID:
      Pam1Address = DRAMC_REGISTER_Q35 (MCH_PAM1);
      break;
    case MICROVM_PSEUDO_DEVICE_ID:
      return;
    default:
      DEBUG ((
        DEBUG_ERROR,
        "%a: unknown host bridge device ID: 0x%04x\n",
        __FUNCTION__,
        HostBridgeDevId
        ));
      ASSERT (FALSE);

      if (!EFI_ERROR (Segment0AllocationStatus)) {
        gBS->FreePages (Segment0, Segment0Pages);
      }

      return;
  }

  //
  // low nibble covers 0xC0000 to 0xC3FFF
  // high nibble covers 0xC4000 to 0xC7FFF
  // bit1 in each nibble is Write Enable
  // bit0 in each nibble is Read Enable
  //
  Pam1 = PciRead8 (Pam1Address);
  PciWrite8 (Pam1Address, Pam1 | (BIT1 | BIT0));

  //
  // We never added memory space during PEI or DXE for the C segment, so we
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
  VbeInfo->VesaVersion = 0x0300;

  VbeInfo->OemNameAddress = (UINT32)SegmentC << 12 | (UINT16)(UINTN)Ptr;
  CopyMem (Ptr, "QEMU", 5);
  Ptr += 5;

  VbeInfo->Capabilities = BIT0; // DAC can be switched into 8-bit mode

  VbeInfo->ModeListAddress = (UINT32)SegmentC << 12 | (UINT16)(UINTN)Ptr;
  *(UINT16 *)Ptr           = 0x00f1; // mode number
  Ptr                     += 2;
  *(UINT16 *)Ptr           = 0xFFFF; // mode list terminator
  Ptr                     += 2;

  VbeInfo->VideoMem64K        = (UINT16)((1024 * 768 * 4 + 65535) / 65536);
  VbeInfo->OemSoftwareVersion = 0x0000;

  VbeInfo->VendorNameAddress = (UINT32)SegmentC << 12 | (UINT16)(UINTN)Ptr;
  CopyMem (Ptr, "OVMF", 5);
  Ptr += 5;

  VbeInfo->ProductNameAddress = (UINT32)SegmentC << 12 | (UINT16)(UINTN)Ptr;
  Printed                     = AsciiSPrint (
                                  (CHAR8 *)Ptr,
                                  sizeof VbeInfoFull->Buffer - (Ptr - VbeInfoFull->Buffer),
                                  "%s",
                                  CardName
                                  );
  Ptr += Printed + 1;

  VbeInfo->ProductRevAddress = (UINT32)SegmentC << 12 | (UINT16)(UINTN)Ptr;
  CopyMem (Ptr, mProductRevision, sizeof mProductRevision);
  Ptr += sizeof mProductRevision;

  ASSERT (sizeof VbeInfoFull->Buffer >= Ptr - VbeInfoFull->Buffer);
  ZeroMem (Ptr, sizeof VbeInfoFull->Buffer - (Ptr - VbeInfoFull->Buffer));

  //
  // Fil in the VBE MODE INFO structure.
  //
  VbeModeInfo = (VBE_MODE_INFO *)(VbeInfoFull + 1);

  //
  // bit0: mode supported by present hardware configuration
  // bit1: optional information available (must be =1 for VBE v1.2+)
  // bit3: set if color, clear if monochrome
  // bit4: set if graphics mode, clear if text mode
  // bit5: mode is not VGA-compatible
  // bit7: linear framebuffer mode supported
  //
  VbeModeInfo->ModeAttr = BIT7 | BIT5 | BIT4 | BIT3 | BIT1 | BIT0;

  //
  // bit0: exists
  // bit1: bit1: readable
  // bit2: writeable
  //
  VbeModeInfo->WindowAAttr = BIT2 | BIT1 | BIT0;

  VbeModeInfo->WindowBAttr              = 0x00;
  VbeModeInfo->WindowGranularityKB      = 0x0040;
  VbeModeInfo->WindowSizeKB             = 0x0040;
  VbeModeInfo->WindowAStartSegment      = 0xA000;
  VbeModeInfo->WindowBStartSegment      = 0x0000;
  VbeModeInfo->WindowPositioningAddress = 0x0000;
  VbeModeInfo->BytesPerScanLine         = 1024 * 4;

  VbeModeInfo->Width                = 1024;
  VbeModeInfo->Height               = 768;
  VbeModeInfo->CharCellWidth        = 8;
  VbeModeInfo->CharCellHeight       = 16;
  VbeModeInfo->NumPlanes            = 1;
  VbeModeInfo->BitsPerPixel         = 32;
  VbeModeInfo->NumBanks             = 1;
  VbeModeInfo->MemoryModel          = 6; // direct color
  VbeModeInfo->BankSizeKB           = 0;
  VbeModeInfo->NumImagePagesLessOne = 0;
  VbeModeInfo->Vbe3                 = 0x01;

  VbeModeInfo->RedMaskSize      = 8;
  VbeModeInfo->RedMaskPos       = 16;
  VbeModeInfo->GreenMaskSize    = 8;
  VbeModeInfo->GreenMaskPos     = 8;
  VbeModeInfo->BlueMaskSize     = 8;
  VbeModeInfo->BlueMaskPos      = 0;
  VbeModeInfo->ReservedMaskSize = 8;
  VbeModeInfo->ReservedMaskPos  = 24;

  //
  // bit1: Bytes in reserved field may be used by application
  //
  VbeModeInfo->DirectColorModeInfo = BIT1;

  VbeModeInfo->LfbAddress       = (UINT32)FrameBufferBase;
  VbeModeInfo->OffScreenAddress = 0;
  VbeModeInfo->OffScreenSizeKB  = 0;

  VbeModeInfo->BytesPerScanLineLinear = 1024 * 4;
  VbeModeInfo->NumImagesLessOneBanked = 0;
  VbeModeInfo->NumImagesLessOneLinear = 0;
  VbeModeInfo->RedMaskSizeLinear      = 8;
  VbeModeInfo->RedMaskPosLinear       = 16;
  VbeModeInfo->GreenMaskSizeLinear    = 8;
  VbeModeInfo->GreenMaskPosLinear     = 8;
  VbeModeInfo->BlueMaskSizeLinear     = 8;
  VbeModeInfo->BlueMaskPosLinear      = 0;
  VbeModeInfo->ReservedMaskSizeLinear = 8;
  VbeModeInfo->ReservedMaskPosLinear  = 24;
  VbeModeInfo->MaxPixelClockHz        = 0;

  ZeroMem (VbeModeInfo->Reserved, sizeof VbeModeInfo->Reserved);

  //
  // Clear Write Enable (bit1), keep Read Enable (bit0) set
  //
  PciWrite8 (Pam1Address, (Pam1 & ~BIT1) | BIT0);

  //
  // Second, point the Int10h vector at the shim.
  //
  Int0x10->Segment = (UINT16)((UINT32)SegmentC >> 4);
  Int0x10->Offset  = (UINT16)((UINTN)(VbeModeInfo + 1) - SegmentC);

  DEBUG ((DEBUG_INFO, "%a: VBE shim installed\n", __FUNCTION__));
}
