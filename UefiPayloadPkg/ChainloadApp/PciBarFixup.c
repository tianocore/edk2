/** @file
  Program endpoint BARs that the outer firmware left at zero, before
  the payload's light enumeration reads them.

  When PcdPciDisableBusEnumeration is TRUE the payload's PciBusDxe
  trusts the bus numbers and bridge windows it finds and skips full
  resource allocation.  Some outer firmware nevertheless leaves the
  BARs of particular endpoints at zero even though the parent PCI to
  PCI bridge's non-prefetchable memory window is programmed and
  forwarding.  A downstream driver that later calls
  PciIo->GetBarAttributes() on such a BAR trips the translation offset
  ASSERT in PciIo.c on DEBUG builds and gets EFI_UNSUPPORTED on RELEASE
  builds, so the device is unreachable either way.

  The outer firmware's EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL cannot be used
  for this repair: its root bridge takes its bus range from the
  platform bus-range description, and on the platforms that motivate
  this fixup that range is [00,00] while the endpoints needing repair
  sit in a disjoint bus tree starting at bus 1 that no bridge on bus 0
  leads to.  The protocol's config accessors reject any bus outside
  the declared range with EFI_INVALID_PARAMETER, so those buses are
  unreachable through the protocol by construction.  The payload
  itself later reaches them by scanning ECAM directly and synthesizing
  one root bridge per disjoint tree (ScanForRootBridges), so this
  fixup does the same: locate the ECAM aperture from the ACPI MCFG
  the outer firmware publishes, walk configuration space through
  ECAM, group buses into disjoint trees the way the payload will, and
  program any 32-bit non-prefetchable MEM BAR that reads back as zero
  from the unused tail of its parent bridge's non-prefetchable
  window.  By the time the payload's PciBusDxe runs, the BARs are
  simply programmed, as if the outer firmware had done its whole job;
  neither the payload nor the PCI core needs any change.

  A zero-BAR endpoint was unreachable under the outer firmware for the
  same reason it would be unreachable under the payload, so no outer
  driver can have it open, and no outer agent is doing MMIO to it
  while its BARs move.  Config-space sizing (the all-ones write) and
  programming still happen at TPL_HIGH_LEVEL with memory decode
  disabled, so a timer callback cannot observe a half-sized or
  half-programmed device.

  The fixup is deliberately narrow: only 32-bit non-prefetchable MEM
  BARs on Type 0 endpoints under a PCI to PCI bridge with a valid non
  prefetchable window are touched.  Root-bus endpoints, the PPB's own
  two BARs, prefetchable BARs, 64-bit BARs and I/O BARs are all left
  as the outer firmware programmed them.  A programmed Expansion ROM
  BAR or a CardBus bridge child causes the whole parent bridge to be
  skipped.  The allocator is a bottom-up watermark inside the parent
  window; it assumes the outer firmware allocated bottom-up, which is
  true of the platforms that motivate this fixup, and is stated here
  because it is a platform assumption rather than a spec requirement.

  Copyright (c) 2026, Amazon.com, Inc. or its affiliates.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi.h>
#include <IndustryStandard/Acpi.h>
#include <IndustryStandard/Pci.h>
#include <IndustryStandard/MemoryMappedConfigurationSpaceAccessTable.h>
#include <Library/AcpiTableWalkLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>

//
// The ECAM aperture of the MCFG allocation group currently being
// walked.  mEcamBase already accounts for the group's StartBusNumber,
// so EcamAddress() takes absolute bus numbers.
//
STATIC UINTN  mEcamBase;
STATIC UINT8  mEcamStartBus;
STATIC UINT8  mEcamEndBus;

//
// Description of one BAR read from configuration space.
//
typedef struct {
  UINT16     Offset;
  BOOLEAN    IsMem;
  BOOLEAN    IsMem64;
  BOOLEAN    IsPref;
  UINT64     Base;
  UINT64     Length;
  UINT64     Alignment;
} BAR_INFO;

/**
  Compute the ECAM address of a configuration space register.

  @param[in] Bus     PCI bus number (absolute).
  @param[in] Dev     PCI device number.
  @param[in] Func    PCI function number.
  @param[in] Offset  Byte offset into configuration space.

  @return  CPU address of the register in the ECAM aperture.
**/
STATIC
UINTN
EcamAddress (
  IN UINT8   Bus,
  IN UINT8   Dev,
  IN UINT8   Func,
  IN UINT16  Offset
  )
{
  return mEcamBase +
         (((UINTN)Bus - mEcamStartBus) << 20) +
         ((UINTN)Dev << 15) +
         ((UINTN)Func << 12) +
         Offset;
}

/**
  Read a naturally aligned 32-bit configuration space register.

  @param[in] Bus     PCI bus number (absolute).
  @param[in] Dev     PCI device number.
  @param[in] Func    PCI function number.
  @param[in] Offset  Byte offset into configuration space.

  @return  The register value.
**/
STATIC
UINT32
EcamRead32 (
  IN UINT8   Bus,
  IN UINT8   Dev,
  IN UINT8   Func,
  IN UINT16  Offset
  )
{
  UINT32  Value;

  MemoryFence ();
  Value = *(volatile UINT32 *)EcamAddress (Bus, Dev, Func, Offset);
  MemoryFence ();
  return Value;
}

/**
  Write a naturally aligned 32-bit configuration space register.

  @param[in] Bus     PCI bus number (absolute).
  @param[in] Dev     PCI device number.
  @param[in] Func    PCI function number.
  @param[in] Offset  Byte offset into configuration space.
  @param[in] Value   The value to write.
**/
STATIC
VOID
EcamWrite32 (
  IN UINT8   Bus,
  IN UINT8   Dev,
  IN UINT8   Func,
  IN UINT16  Offset,
  IN UINT32  Value
  )
{
  MemoryFence ();
  *(volatile UINT32 *)EcamAddress (Bus, Dev, Func, Offset) = Value;
  MemoryFence ();
}

/**
  Read a naturally aligned 16-bit configuration space register.

  @param[in] Bus     PCI bus number (absolute).
  @param[in] Dev     PCI device number.
  @param[in] Func    PCI function number.
  @param[in] Offset  Byte offset into configuration space.

  @return  The register value.
**/
STATIC
UINT16
EcamRead16 (
  IN UINT8   Bus,
  IN UINT8   Dev,
  IN UINT8   Func,
  IN UINT16  Offset
  )
{
  UINT16  Value;

  MemoryFence ();
  Value = *(volatile UINT16 *)EcamAddress (Bus, Dev, Func, Offset);
  MemoryFence ();
  return Value;
}

/**
  Write a naturally aligned 16-bit configuration space register.

  COMMAND must be written with a true 16-bit access: a 32-bit
  read-modify-write of the COMMAND/STATUS dword would write back the
  RW1C STATUS bits currently set and clear them as a side effect.

  @param[in] Bus     PCI bus number (absolute).
  @param[in] Dev     PCI device number.
  @param[in] Func    PCI function number.
  @param[in] Offset  Byte offset into configuration space.
  @param[in] Value   The value to write.
**/
STATIC
VOID
EcamWrite16 (
  IN UINT8   Bus,
  IN UINT8   Dev,
  IN UINT8   Func,
  IN UINT16  Offset,
  IN UINT16  Value
  )
{
  MemoryFence ();
  *(volatile UINT16 *)EcamAddress (Bus, Dev, Func, Offset) = Value;
  MemoryFence ();
}

/**
  Read a device's configuration header and probe its BARs.

  Both Type 0 and Type 1 headers are handled.  A BAR whose read-only
  low nibble marks it as I/O is recorded with IsMem = FALSE.  For each
  memory BAR the current base and, by writing all-ones and reading
  back, the natural size and alignment are recorded.  The original BAR
  contents are restored before return.

  Sizing happens at TPL_HIGH_LEVEL: the outer firmware is live, and a
  timer callback that touches the device between the all-ones write
  and the restore would observe a BAR pointing at nothing.

  @param[in]  Bus     PCI bus number of the device.
  @param[in]  Dev     PCI device number.
  @param[in]  Func    PCI function number.
  @param[out] Hdr     The full 64-byte common configuration header.
  @param[out] Bar     Array of PCI_MAX_BAR BAR_INFO records.  Only the
                      first two entries are meaningful for a Type 1
                      header.

  @retval EFI_SUCCESS    The header and BARs were read.
  @retval EFI_NOT_FOUND  No device responds at Bus/Dev/Func.
**/
STATIC
EFI_STATUS
ReadDevice (
  IN  UINT8       Bus,
  IN  UINT8       Dev,
  IN  UINT8       Func,
  OUT PCI_TYPE01  *Hdr,
  OUT BAR_INFO    *Bar
  )
{
  UINT32   *Raw;
  UINT32   Original[2];
  UINT32   Value;
  UINT32   Mask;
  UINT16   VendorId;
  UINT8    BarCount;
  UINT16   Offset;
  UINT8    Idx;
  UINTN    Word;
  EFI_TPL  OldTpl;

  VendorId = EcamRead16 (Bus, Dev, Func, PCI_VENDOR_ID_OFFSET);
  if (VendorId == 0xFFFF) {
    return EFI_NOT_FOUND;
  }

  Raw = (UINT32 *)Hdr;
  for (Word = 0; Word < sizeof (PCI_TYPE01) / sizeof (UINT32); Word++) {
    Raw[Word] = EcamRead32 (Bus, Dev, Func, (UINT16)(Word * sizeof (UINT32)));
  }

  ZeroMem (Bar, PCI_MAX_BAR * sizeof (BAR_INFO));
  if (IS_CARDBUS_BRIDGE (Hdr)) {
    return EFI_SUCCESS;
  }

  BarCount = IS_PCI_BRIDGE (Hdr) ? 2 : PCI_MAX_BAR;
  Offset   = (UINT16)OFFSET_OF (PCI_TYPE00, Device.Bar[0]);

  OldTpl = gBS->RaiseTPL (TPL_HIGH_LEVEL);

  for (Idx = 0; Idx < BarCount; Idx++) {
    Bar[Idx].Offset = Offset;
    Original[0]     = EcamRead32 (Bus, Dev, Func, Offset);
    Original[1]     = 0;

    if ((Original[0] & BIT0) != 0) {
      //
      // I/O space BAR: not touched by this fixup.
      //
      Bar[Idx].IsMem = FALSE;
      Offset        += 4;
      continue;
    }

    Bar[Idx].IsMem   = TRUE;
    Bar[Idx].IsMem64 = ((Original[0] & (BIT1 | BIT2)) == BIT2);
    Bar[Idx].IsPref  = ((Original[0] & BIT3) != 0);

    EcamWrite32 (Bus, Dev, Func, Offset, MAX_UINT32);
    Value = EcamRead32 (Bus, Dev, Func, Offset);
    EcamWrite32 (Bus, Dev, Func, Offset, Original[0]);

    Mask = Value & 0xFFFFFFF0U;

    if (Bar[Idx].IsMem64) {
      Original[1] = EcamRead32 (Bus, Dev, Func, Offset + 4);
      EcamWrite32 (Bus, Dev, Func, Offset + 4, MAX_UINT32);
      Value = EcamRead32 (Bus, Dev, Func, Offset + 4);
      EcamWrite32 (Bus, Dev, Func, Offset + 4, Original[1]);

      Bar[Idx].Base   = ((UINT64)Original[1] << 32) | (Original[0] & 0xFFFFFFF0U);
      Bar[Idx].Length = ~(((UINT64)Value << 32) | Mask) + 1;
    } else {
      Bar[Idx].Base   = Original[0] & 0xFFFFFFF0U;
      Bar[Idx].Length = (~Mask) + 1;
    }

    if (Mask == 0) {
      Bar[Idx].Length = 0;
    }

    Bar[Idx].Alignment = Bar[Idx].Length - 1;

    if (Bar[Idx].IsMem64) {
      Idx++;
      Bar[Idx].Offset = Offset + 4;
      Offset         += 8;
    } else {
      Offset += 4;
    }
  }

  gBS->RestoreTPL (OldTpl);

  return EFI_SUCCESS;
}

/**
  Walk one bus, recursing into any PCI to PCI bridge whose non
  prefetchable memory window is programmed, and program 32-bit non
  prefetchable MEM BARs that read back as zero from the unused tail of
  the parent bridge's non-prefetchable window.

  Space already occupied by sub-bridge non-prefetchable and
  prefetchable windows and by sibling BARs is subtracted first, so the
  fixup never overlaps a region the outer firmware already handed to
  another device.  Each fixup is atomic per device: all zero-base MEM
  BARs on the device are placed before COMMAND is touched, and if any
  is not a 32-bit non-prefetchable BAR that fits the remaining window
  the device is left exactly as found.

  @param[in]  Bus          The bus to walk.
  @param[in]  WindowBase   Base of the enclosing PPB non-prefetchable
                           window, or MAX_UINT64 for the root bus.
  @param[in]  WindowLimit  Inclusive limit of the same window, or 0 for
                           the root bus.
**/
STATIC
VOID
WalkBus (
  IN UINT8   Bus,
  IN UINT64  WindowBase,
  IN UINT64  WindowLimit
  )
{
  EFI_STATUS  Status;
  PCI_TYPE01  Hdr;
  BAR_INFO    Bar[PCI_MAX_BAR];
  UINT64      SubBase;
  UINT64      SubLimit;
  UINT64      Base;
  UINT64      End;
  UINT64      FreeBase;
  UINT64      TryBase;
  UINT64      Size;
  UINT64      NewBase[PCI_MAX_BAR];
  UINT32      Bar32;
  UINT32      ReadBack;
  UINT32      RomBase;
  UINT16      MemoryBase;
  UINT16      MemoryLimit;
  UINT16      Command;
  UINT8       Dev;
  UINT8       Func;
  UINT8       BarIdx;
  UINT8       Idx2;
  BOOLEAN     NeedFixup;
  BOOLEAN     Unfixable;
  BOOLEAN     WriteFailed;
  BOOLEAN     Skip;
  EFI_TPL     OldTpl;

  //
  // Pass 1: find the highest already-occupied address inside this
  // window so that fresh allocations start above it.  Occupancy covers
  // sub-bridge non-prefetch and prefetch windows and sibling BARs.  A
  // programmed Expansion ROM BAR or a CardBus bridge child causes the
  // whole parent bridge to be skipped.
  //
  Skip     = FALSE;
  FreeBase = WindowBase;
  for (Dev = 0; Dev <= PCI_MAX_DEVICE; Dev++) {
    for (Func = 0; Func <= PCI_MAX_FUNC; Func++) {
      Status = ReadDevice (Bus, Dev, Func, &Hdr, Bar);
      if (EFI_ERROR (Status)) {
        if (Func == 0) {
          break;
        }

        continue;
      }

      if (IS_CARDBUS_BRIDGE (&Hdr)) {
        DEBUG ((
          DEBUG_WARN,
          "ChainloadApp: [%02x|%02x|%02x] CardBus child; skipping bridge\n",
          Bus,
          Dev,
          Func
          ));
        Skip = TRUE;
      } else if (IS_PCI_BRIDGE (&Hdr)) {
        SubBase  = ((UINT64)Hdr.Bridge.MemoryBase & 0xFFF0) << 16;
        SubLimit = (((UINT64)Hdr.Bridge.MemoryLimit & 0xFFF0) << 16) | 0xFFFFF;
        if (SubBase <= SubLimit) {
          End = SubLimit + 1;
          if ((End > WindowBase) && (SubBase <= WindowLimit) && (End > FreeBase)) {
            FreeBase = End;
          }
        }

        SubBase = ((UINT64)Hdr.Bridge.PrefetchableBaseUpper32 << 32) |
                  (((UINT64)Hdr.Bridge.PrefetchableMemoryBase & 0xFFF0) << 16);
        SubLimit = ((UINT64)Hdr.Bridge.PrefetchableLimitUpper32 << 32) |
                   (((UINT64)Hdr.Bridge.PrefetchableMemoryLimit & 0xFFF0) << 16) | 0xFFFFF;
        if (SubBase <= SubLimit) {
          End = SubLimit + 1;
          if ((End > WindowBase) && (SubBase <= WindowLimit) && (End > FreeBase)) {
            FreeBase = End;
          }
        }

        RomBase = Hdr.Bridge.ExpansionRomBAR & 0xFFFFF800U;
        if (RomBase != 0) {
          DEBUG ((
            DEBUG_WARN,
            "ChainloadApp: [%02x|%02x|%02x] bridge Expansion ROM at 0x%x; skipping bridge\n",
            Bus,
            Dev,
            Func,
            RomBase
            ));
          Skip = TRUE;
        }
      } else {
        RomBase = ((PCI_TYPE00 *)&Hdr)->Device.ExpansionRomBar & 0xFFFFF800U;
        if (RomBase != 0) {
          DEBUG ((
            DEBUG_WARN,
            "ChainloadApp: [%02x|%02x|%02x] Expansion ROM at 0x%x; skipping bridge\n",
            Bus,
            Dev,
            Func,
            RomBase
            ));
          Skip = TRUE;
        }
      }

      for (BarIdx = 0; BarIdx < PCI_MAX_BAR; BarIdx++) {
        if (Bar[BarIdx].IsMem && (Bar[BarIdx].Length != 0) && (Bar[BarIdx].Base != 0)) {
          Base = Bar[BarIdx].Base;
          End  = Base + MAX (Bar[BarIdx].Alignment + 1, Bar[BarIdx].Length);
          if ((End > WindowBase) && (Base <= WindowLimit) && (End > FreeBase)) {
            FreeBase = End;
          }
        }
      }

      if ((Func == 0) && !IS_PCI_MULTI_FUNC (&Hdr)) {
        break;
      }
    }
  }

  //
  // Pass 2: recurse into sub-bridges with their own window, and assign
  // BARs on direct-child endpoints that still read back as zero.
  //
  for (Dev = 0; Dev <= PCI_MAX_DEVICE; Dev++) {
    for (Func = 0; Func <= PCI_MAX_FUNC; Func++) {
      Status = ReadDevice (Bus, Dev, Func, &Hdr, Bar);
      if (EFI_ERROR (Status)) {
        if (Func == 0) {
          break;
        }

        continue;
      }

      if (IS_PCI_BRIDGE (&Hdr)) {
        MemoryBase  = Hdr.Bridge.MemoryBase;
        MemoryLimit = Hdr.Bridge.MemoryLimit;

        if ((MemoryBase != 0) || (MemoryLimit != 0)) {
          SubBase  = ((UINT64)MemoryBase & 0xFFF0) << 16;
          SubLimit = (((UINT64)MemoryLimit & 0xFFF0) << 16) | 0xFFFFF;
          if ((SubBase != 0) &&
              ((MemoryLimit & 0xFFF0) >= (MemoryBase & 0xFFF0)) &&
              (Hdr.Bridge.SecondaryBus != 0) &&
              (Hdr.Bridge.SecondaryBus > Bus) &&
              (Hdr.Bridge.SecondaryBus <= mEcamEndBus))
          {
            if ((WindowBase <= WindowLimit) &&
                ((SubBase < WindowBase) || (SubLimit > WindowLimit)))
            {
              DEBUG ((
                DEBUG_WARN,
                "ChainloadApp: [%02x|%02x|%02x] sub-window [0x%Lx,0x%Lx] not inside "
                "parent [0x%Lx,0x%Lx]; skipping\n",
                Bus,
                Dev,
                Func,
                SubBase,
                SubLimit,
                WindowBase,
                WindowLimit
                ));
            } else {
              WalkBus (Hdr.Bridge.SecondaryBus, SubBase, SubLimit);
            }
          }
        }

        goto NextFunc;
      }

      if (IS_CARDBUS_BRIDGE (&Hdr)) {
        goto NextFunc;
      }

      //
      // Endpoint.  Root-bus endpoints have no enclosing PPB window and
      // are skipped (WindowBase > WindowLimit at the root).  So is the
      // whole bridge if pass 1 found an Expansion ROM or CardBus child.
      //
      if (Skip || (WindowBase > WindowLimit)) {
        goto NextFunc;
      }

      //
      // Collect every zero-base MEM BAR on this device.  If any of
      // them is not a 32-bit non-prefetchable BAR that fits the
      // remaining window, skip the device entirely and leave COMMAND
      // exactly as found.
      //
      ZeroMem (NewBase, sizeof (NewBase));
      NeedFixup = FALSE;
      Unfixable = FALSE;
      TryBase   = FreeBase;
      for (BarIdx = 0; BarIdx < PCI_MAX_BAR; BarIdx++) {
        if (!Bar[BarIdx].IsMem || (Bar[BarIdx].Length == 0) || (Bar[BarIdx].Base != 0)) {
          continue;
        }

        NeedFixup = TRUE;
        if (Bar[BarIdx].IsMem64 || Bar[BarIdx].IsPref) {
          Unfixable = TRUE;
          break;
        }

        Size    = MAX (Bar[BarIdx].Alignment + 1, Bar[BarIdx].Length);
        TryBase = ALIGN_VALUE (TryBase, Bar[BarIdx].Alignment + 1);
        if ((TryBase == 0) || ((TryBase + Size - 1) > WindowLimit)) {
          Unfixable = TRUE;
          break;
        }

        NewBase[BarIdx] = TryBase;
        TryBase        += Size;
      }

      if (!NeedFixup) {
        goto NextFunc;
      }

      if (Unfixable) {
        DEBUG ((
          DEBUG_WARN,
          "ChainloadApp: [%02x|%02x|%02x] unassigned MEM BAR not fixable in window "
          "[0x%Lx,0x%Lx]; skipping device\n",
          Bus,
          Dev,
          Func,
          WindowBase,
          WindowLimit
          ));
        goto NextFunc;
      }

      //
      // Raise TPL so no timer callback pokes the device while its BARs
      // are being rewritten with MSE clear.  Keep the raised window to
      // config accesses only; DEBUG output happens after RestoreTPL.
      //
      WriteFailed = FALSE;
      OldTpl      = gBS->RaiseTPL (TPL_HIGH_LEVEL);

      Command = Hdr.Hdr.Command & ~(UINT16)EFI_PCI_COMMAND_MEMORY_SPACE;
      EcamWrite16 (Bus, Dev, Func, PCI_COMMAND_OFFSET, Command);

      for (BarIdx = 0; BarIdx < PCI_MAX_BAR; BarIdx++) {
        if (NewBase[BarIdx] == 0) {
          continue;
        }

        Bar32 = (UINT32)NewBase[BarIdx];
        EcamWrite32 (Bus, Dev, Func, Bar[BarIdx].Offset, Bar32);
        ReadBack = EcamRead32 (Bus, Dev, Func, Bar[BarIdx].Offset);
        if ((ReadBack & ~0xFU) != (Bar32 & ~0xFU)) {
          WriteFailed = TRUE;
          break;
        }
      }

      if (WriteFailed) {
        //
        // Roll back so the device is left exactly as found: MSE is
        // still clear here, so zeroing the BARs we already wrote is
        // safe.  Do not advance FreeBase; the next device may reuse
        // this space.
        //
        for (Idx2 = 0; Idx2 <= BarIdx; Idx2++) {
          if (NewBase[Idx2] != 0) {
            EcamWrite32 (Bus, Dev, Func, Bar[Idx2].Offset, 0);
          }
        }
      }

      EcamWrite16 (Bus, Dev, Func, PCI_COMMAND_OFFSET, Hdr.Hdr.Command);

      gBS->RestoreTPL (OldTpl);

      if (WriteFailed) {
        DEBUG ((
          DEBUG_WARN,
          "ChainloadApp: [%02x|%02x|%02x] BAR write/read-back failed; not committing\n",
          Bus,
          Dev,
          Func
          ));
        goto NextFunc;
      }

      for (BarIdx = 0; BarIdx < PCI_MAX_BAR; BarIdx++) {
        if (NewBase[BarIdx] == 0) {
          continue;
        }

        DEBUG ((
          DEBUG_INFO,
          "ChainloadApp: [%02x|%02x|%02x] BAR%u <- 0x%08x (Len=0x%Lx)\n",
          Bus,
          Dev,
          Func,
          BarIdx,
          (UINT32)NewBase[BarIdx],
          Bar[BarIdx].Length
          ));
      }

      FreeBase = TryBase;

NextFunc:
      if ((Func == 0) && !IS_PCI_MULTI_FUNC (&Hdr)) {
        break;
      }
    }
  }
}

/**
  Program endpoint BARs the outer firmware left at zero, for every
  disjoint bus tree in every ECAM allocation the outer firmware's
  MCFG describes.

  Called from ChainloadEntry() while boot services are available.
  Buses are grouped into disjoint trees the way the payload's own
  ScanForRootBridges() later groups them: scan a candidate root bus,
  take the highest subordinate bus number claimed by any PPB on it,
  and resume with the bus after that.  Each candidate root is walked
  like a root bus: its own endpoints are left alone, and every PPB on
  it supplies the window for the tree below it.

**/
VOID
FixupUnassignedBars (
  VOID
  )
{
  EFI_STATUS                                                                             Status;
  EFI_ACPI_MEMORY_MAPPED_CONFIGURATION_BASE_ADDRESS_TABLE_HEADER                         *Mcfg;
  EFI_ACPI_MEMORY_MAPPED_ENHANCED_CONFIGURATION_SPACE_BASE_ADDRESS_ALLOCATION_STRUCTURE  *Alloc;
  PCI_TYPE01                                                                             Hdr;
  BAR_INFO                                                                               Bar[PCI_MAX_BAR];
  VOID                                                                                   *Rsdp;
  UINTN                                                                                  Count;
  UINTN                                                                                  Idx;
  UINTN                                                                                  Jdx;
  BOOLEAN                                                                                Skip;
  UINTN                                                                                  RootBus;
  UINTN                                                                                  SubBus;
  UINT8                                                                                  Dev;
  UINT8                                                                                  Func;

  Rsdp   = NULL;
  Status = EfiGetSystemConfigurationTable (&gEfiAcpiTableGuid, &Rsdp);
  if (EFI_ERROR (Status) || (Rsdp == NULL)) {
    Status = EfiGetSystemConfigurationTable (&gEfiAcpi10TableGuid, &Rsdp);
  }

  if (EFI_ERROR (Status) || (Rsdp == NULL)) {
    return;
  }

  Mcfg = (EFI_ACPI_MEMORY_MAPPED_CONFIGURATION_BASE_ADDRESS_TABLE_HEADER *)AcpiFindTableFromRsdp (
                                                                             (EFI_PHYSICAL_ADDRESS)(UINTN)Rsdp,
                                                                             EFI_ACPI_2_0_MEMORY_MAPPED_CONFIGURATION_BASE_ADDRESS_TABLE_SIGNATURE
                                                                             );
  if ((Mcfg == NULL) || (Mcfg->Header.Length <= sizeof (*Mcfg))) {
    //
    // No ECAM description -- nothing was assigned, nothing to repair.
    //
    return;
  }

  Alloc = (VOID *)(Mcfg + 1);
  Count = (Mcfg->Header.Length - sizeof (*Mcfg)) / sizeof (*Alloc);
  for (Idx = 0; Idx < Count; Idx++) {
    if ((Alloc[Idx].BaseAddress == 0) ||
        (Alloc[Idx].EndBusNumber < Alloc[Idx].StartBusNumber))
    {
      continue;
    }

    //
    // Standard ECAM semantics place bus N of an allocation at
    // BaseAddress + (N - StartBusNumber) << 20, so two allocations can
    // never share a BaseAddress: their windows would overlap.  An MCFG
    // that lists the same BaseAddress more than once is therefore
    // describing one flat aperture in which bus N always decodes at
    // BaseAddress + N << 20, split across entries whose StartBusNumber
    // was not folded into the address.  Merge such entries into one
    // group anchored at the lowest StartBusNumber, which restores flat
    // decode; a well-formed single allocation is its own group and is
    // walked with standard semantics, unchanged.
    //
    Skip = FALSE;
    for (Jdx = 0; Jdx < Idx; Jdx++) {
      if ((Alloc[Jdx].BaseAddress == Alloc[Idx].BaseAddress) &&
          (Alloc[Jdx].PciSegmentGroupNumber == Alloc[Idx].PciSegmentGroupNumber))
      {
        Skip = TRUE;
        break;
      }
    }

    if (Skip) {
      continue;
    }

    mEcamBase     = (UINTN)Alloc[Idx].BaseAddress;
    mEcamStartBus = Alloc[Idx].StartBusNumber;
    mEcamEndBus   = Alloc[Idx].EndBusNumber;
    for (Jdx = Idx + 1; Jdx < Count; Jdx++) {
      if ((Alloc[Jdx].BaseAddress != Alloc[Idx].BaseAddress) ||
          (Alloc[Jdx].PciSegmentGroupNumber != Alloc[Idx].PciSegmentGroupNumber) ||
          (Alloc[Jdx].EndBusNumber < Alloc[Jdx].StartBusNumber))
      {
        continue;
      }

      if (Alloc[Jdx].StartBusNumber < mEcamStartBus) {
        mEcamStartBus = Alloc[Jdx].StartBusNumber;
      }

      if (Alloc[Jdx].EndBusNumber > mEcamEndBus) {
        mEcamEndBus = Alloc[Jdx].EndBusNumber;
      }
    }

    DEBUG ((
      DEBUG_INFO,
      "ChainloadApp: ECAM 0x%Lx segment %u buses [%02x,%02x]\n",
      Alloc[Idx].BaseAddress,
      Alloc[Idx].PciSegmentGroupNumber,
      mEcamStartBus,
      mEcamEndBus
      ));

    //
    // Group buses into disjoint trees.  RootBus and SubBus are UINTN so
    // that SubBus + 1 cannot wrap when a tree ends at bus 255.
    //
    for (RootBus = mEcamStartBus; RootBus <= mEcamEndBus; RootBus = SubBus + 1) {
      SubBus = RootBus;
      for (Dev = 0; Dev <= PCI_MAX_DEVICE; Dev++) {
        for (Func = 0; Func <= PCI_MAX_FUNC; Func++) {
          Status = ReadDevice ((UINT8)RootBus, Dev, Func, &Hdr, Bar);
          if (EFI_ERROR (Status)) {
            if (Func == 0) {
              break;
            }

            continue;
          }

          if (IS_PCI_BRIDGE (&Hdr) && (Hdr.Bridge.SubordinateBus > SubBus)) {
            SubBus = Hdr.Bridge.SubordinateBus;
          }

          if ((Func == 0) && !IS_PCI_MULTI_FUNC (&Hdr)) {
            break;
          }
        }
      }

      //
      // The candidate root has no Type 1 memory window of its own, so
      // start the walk with an empty window (Base > Limit); every
      // first-level PPB supplies its own window from its configuration
      // header.
      //
      WalkBus ((UINT8)RootBus, MAX_UINT64, 0);
    }
  }
}
