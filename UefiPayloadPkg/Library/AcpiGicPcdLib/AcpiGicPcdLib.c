/** @file
  NULL library that discovers the GIC Distributor, Redistributor and
  memory-mapped CPU-interface bases from the ACPI MADT supplied by
  the bootloader and populates the corresponding PCDs before
  ArmGicDxe consumes them.

  UefiPayloadPkg carries QEMU-virt fixed defaults for the GIC PCDs.
  When the outer firmware handed over ACPI tables (via ChainloadApp
  or a Slim Bootloader), the actual GIC location is described by the
  MADT GICD/GICR/GICC structures.  Read those, override the PCDs, and
  add the ranges to the GCD memory space so ArmGicDxe can touch them.

  With no ACPI handover at all the fixed defaults stay in place, which
  is the existing behaviour for a plain QEMU-virt boot.

  Which base ArmGicDxe actually reads is a partly-runtime decision
  (see the file-scope commentary at ArmPkg/Drivers/ArmGicDxe/
  ArmGicDxe.c: it dispatches on the CPU's GICv3 system-register
  feature and on whether ICC_SRE_EL2.SRE can be enabled, or on the
  CPU's GICv5 system-register feature, and drives the interrupt
  controller as a v2 otherwise).  This library therefore
  derives every base the MADT can supply, and only afterwards decides
  whether what it has is sufficient for the path ArmGicDxe will take.
  The MADT GICD's GicVersion byte is read purely as a diagnostic
  cross-check: upstream deliberately does not trust a table-reported
  GIC version, and neither does this code.

  A MADT that is present but does not describe the base that
  ArmGicDxe's chosen path needs is fatal, not a reason to fall back to
  the QEMU-virt defaults: those addresses are not a GIC on any other
  platform, so letting ArmGicDxe walk them yields either a bus abort
  or a plausible-looking read followed by a mute hang with no
  interrupt controller - and in a RELEASE build, no diagnostic either.
  In every such case the specific reason is printed and CpuDeadLoop()
  is called: the DXE AutoGen constructor wrapper only
  ASSERT_EFI_ERROR()s a status returned from here, so in a RELEASE
  build a returned error alone would be discarded and the entry point
  would run anyway.  A payload without an interrupt controller cannot
  boot, so halting loses nothing over the mute hang and makes the
  failure diagnosable.

  Copyright (c) 2026, Amazon.com, Inc. or its affiliates.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <PiDxe.h>
#include <IndustryStandard/Acpi.h>
#include <UniversalPayload/AcpiTable.h>

#include <Library/AcpiTableWalkLib.h>
#include <Library/ArmLib.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/DxeServicesTableLib.h>
#include <Library/HobLib.h>
#include <Library/PcdLib.h>

/**
  Locate an ACPI table by signature via the RSDP the bootloader
  handed over in the gUniversalPayloadAcpiTableGuid HOB.

  @param[in]  Signature  4-byte ACPI table signature.

  @return  Pointer to the table header, or NULL if not found.
**/
STATIC
EFI_ACPI_DESCRIPTION_HEADER *
LocateAcpiTable (
  IN UINT32  Signature
  )
{
  EFI_HOB_GUID_TYPE             *GuidHob;
  UNIVERSAL_PAYLOAD_ACPI_TABLE  *AcpiHob;

  GuidHob = GetFirstGuidHob (&gUniversalPayloadAcpiTableGuid);
  if (GuidHob == NULL) {
    return NULL;
  }

  AcpiHob = (UNIVERSAL_PAYLOAD_ACPI_TABLE *)GET_GUID_HOB_DATA (GuidHob);

  return AcpiFindTableFromRsdp (AcpiHob->Rsdp, Signature);
}

/**
  Add a device MMIO range to the GCD memory map and mark it uncacheable
  and non-executable, so that ArmCpuDxe populates a page-table entry
  for it.

  If the range is already present in the GCD map, only proceed when the
  existing descriptor is an unowned MMIO descriptor that covers the
  whole request.  A bogus MADT can point a GIC base into DRAM, and
  remapping live system memory uncacheable and non-executable is not a
  warning-level event.  This is the same check the PL031 RTC library
  performs for the same situation.

  @param[in] Base    Base physical address.
  @param[in] Length  Length in bytes.

  @retval EFI_SUCCESS        The range is MMIO and is now mapped UC|XP.
  @retval EFI_ACCESS_DENIED  The range is already described as something
                             other than unowned MMIO covering it whole.
  @retval other              GCD service failure.
**/
STATIC
EFI_STATUS
MapGicMmio (
  IN EFI_PHYSICAL_ADDRESS  Base,
  IN UINT64                Length
  )
{
  EFI_STATUS                       Status;
  EFI_GCD_MEMORY_SPACE_DESCRIPTOR  Desc;

  Status = gDS->AddMemorySpace (
                  EfiGcdMemoryTypeMemoryMappedIo,
                  Base,
                  Length,
                  EFI_MEMORY_UC | EFI_MEMORY_XP
                  );
  if (Status == EFI_ACCESS_DENIED) {
    //
    // Something already describes part or all of the range, and
    // EFI_ACCESS_DENIED does not say what.  Refuse to touch the
    // attributes unless it is MMIO that no driver owns, and unless that
    // single descriptor covers the whole request: AddMemorySpace() also
    // returns EFI_ACCESS_DENIED for a partial overlap, while
    // GetMemorySpaceDescriptor() only returns the descriptor containing
    // Base.
    //
    Status = gDS->GetMemorySpaceDescriptor (Base, &Desc);
    if (EFI_ERROR (Status)) {
      return Status;
    }

    if ((Desc.GcdMemoryType != EfiGcdMemoryTypeMemoryMappedIo) ||
        (Desc.ImageHandle != NULL) ||
        (Desc.BaseAddress > Base) ||
        ((Base + Length) > (Desc.BaseAddress + Desc.Length)))
    {
      DEBUG ((
        DEBUG_ERROR,
        "%a: 0x%Lx(0x%Lx) is already described as GCD type %u owned by %p "
        "over 0x%Lx(0x%Lx); refusing to remap\n",
        __func__,
        Base,
        Length,
        (UINT32)Desc.GcdMemoryType,
        Desc.ImageHandle,
        Desc.BaseAddress,
        Desc.Length
        ));
      return EFI_ACCESS_DENIED;
    }

    //
    // A pre-existing MMIO descriptor need not carry the UC and XP
    // capabilities, and CoreSetMemorySpaceAttributes() rejects any
    // attribute that is absent from Capabilities.  Add them first.
    //
    if ((Desc.Capabilities & (EFI_MEMORY_UC | EFI_MEMORY_XP)) !=
        (EFI_MEMORY_UC | EFI_MEMORY_XP))
    {
      Status = gDS->SetMemorySpaceCapabilities (
                      Base,
                      Length,
                      Desc.Capabilities | EFI_MEMORY_UC | EFI_MEMORY_XP
                      );
      if (EFI_ERROR (Status)) {
        DEBUG ((
          DEBUG_ERROR,
          "%a: SetMemorySpaceCapabilities(0x%Lx, 0x%Lx): %r\n",
          __func__,
          Base,
          Length,
          Status
          ));
        return Status;
      }
    }
  } else if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: AddMemorySpace(0x%Lx, 0x%Lx): %r\n",
      __func__,
      Base,
      Length,
      Status
      ));
    return Status;
  }

  Status = gDS->SetMemorySpaceAttributes (
                  Base,
                  Length,
                  EFI_MEMORY_UC | EFI_MEMORY_XP
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: SetMemorySpaceAttributes(0x%Lx, 0x%Lx): %r\n",
      __func__,
      Base,
      Length,
      Status
      ));
  }

  return Status;
}

/**
  Halt after the caller has printed why the MADT is unusable.

  See the file header for why the QEMU-virt build-time defaults must
  not survive an unusable MADT, and why returning an error status is
  not sufficient in a RELEASE build.
**/
STATIC
VOID
GicMadtFatal (
  VOID
  )
{
  DEBUG ((
    DEBUG_ERROR,
    "AcpiGicPcdLib: no usable interrupt controller can be derived from "
    "the MADT and the build-time PCD defaults are only correct on QEMU "
    "virt; halting.\n"
    ));
  CpuDeadLoop ();
}

/**
  Constructor: parse the ACPI MADT for the GICD/GICR/GICC bases,
  override the GIC PCDs, and map the MMIO ranges.

  @param  ImageHandle  Image handle (unused).
  @param  SystemTable  System table (unused).

  @retval EFI_SUCCESS  No ACPI handover, so the fixed PCD defaults
                       stand; or the base(s) required by ArmGicDxe's
                       chosen init path were derived and mapped.
  @return              An error status only reachable in the caller if
                       CpuDeadLoop() were to return; documents which
                       failure was hit.
**/
EFI_STATUS
EFIAPI
AcpiGicPcdLibConstructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_ACPI_DESCRIPTION_HEADER             *Madt;
  EFI_ACPI_6_0_GIC_DISTRIBUTOR_STRUCTURE  *Gicd;
  EFI_ACPI_6_0_GICR_STRUCTURE             *Gicr;
  EFI_ACPI_6_0_GIC_STRUCTURE              *Gicc;
  EFI_STATUS                              Status;
  RETURN_STATUS                           PcdStatus;
  UINT8                                   *Ptr;
  UINT8                                   *End;
  UINT64                                  DistBase;
  UINT64                                  RedistBase;
  UINT64                                  RedistLen;
  UINT64                                  CpuIfBase;
  UINT64                                  ThisCpuIfBase;
  UINTN                                   GicrCount;
  UINTN                                   GiccRedistCount;
  UINTN                                   Length;
  UINT8                                   GicVersion;
  BOOLEAN                                 HaveSysRegs;
  BOOLEAN                                 CpuIfMismatch;

  Madt = LocateAcpiTable (
           EFI_ACPI_6_0_MULTIPLE_APIC_DESCRIPTION_TABLE_SIGNATURE
           );
  if (Madt == NULL) {
    DEBUG ((DEBUG_INFO, "%a: no MADT, keeping fixed GIC PCDs\n", __func__));
    return EFI_SUCCESS;
  }

  if (Madt->Length < sizeof (EFI_ACPI_6_0_MULTIPLE_APIC_DESCRIPTION_TABLE_HEADER)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: MADT length %u is shorter than its own header\n",
      __func__,
      Madt->Length
      ));
    GicMadtFatal ();
    return EFI_VOLUME_CORRUPTED;
  }

  DistBase        = 0;
  RedistBase      = 0;
  RedistLen       = 0;
  CpuIfBase       = 0;
  GicrCount       = 0;
  GiccRedistCount = 0;
  GicVersion      = 0;
  CpuIfMismatch   = FALSE;

  Ptr = (UINT8 *)Madt +
        sizeof (EFI_ACPI_6_0_MULTIPLE_APIC_DESCRIPTION_TABLE_HEADER);
  End = (UINT8 *)Madt + Madt->Length;

  //
  // Walk the interrupt controller structures.  Each one is Type, Length,
  // then a type-specific body, so both bytes must be present before
  // Length can be read, Length must be large enough to advance, and it
  // must not run past the end of the table.
  //
  while ((UINTN)(End - Ptr) >= 2) {
    Length = Ptr[1];

    if ((Length < 2) || (Length > (UINTN)(End - Ptr))) {
      DEBUG ((
        DEBUG_ERROR,
        "%a: MADT structure type %u at offset 0x%Lx has bad length %u\n",
        __func__,
        (UINT32)Ptr[0],
        (UINT64)(UINTN)(Ptr - (UINT8 *)Madt),
        (UINT32)Length
        ));
      GicMadtFatal ();
      return EFI_VOLUME_CORRUPTED;
    }

    switch (Ptr[0]) {
      case EFI_ACPI_6_0_GICD:
        if (Length < (OFFSET_OF (EFI_ACPI_6_0_GIC_DISTRIBUTOR_STRUCTURE, PhysicalBaseAddress) +
                      sizeof (UINT64)))
        {
          DEBUG ((
            DEBUG_WARN,
            "%a: GICD structure is %u bytes, too short for a base address; ignoring\n",
            __func__,
            (UINT32)Length
            ));
          break;
        }

        Gicd     = (EFI_ACPI_6_0_GIC_DISTRIBUTOR_STRUCTURE *)Ptr;
        DistBase = ReadUnaligned64 (&Gicd->PhysicalBaseAddress);

        //
        // GicVersion sits after SystemVectorBase and was added in
        // ACPI 6.0.  A pre-6.0 GICD is 24 bytes and does not carry it,
        // so bound it separately and leave GicVersion at 0
        // ("unspecified") when absent.  It is read only for the
        // cross-check warning below and is never a gate.
        //
        if (Length >= (OFFSET_OF (EFI_ACPI_6_0_GIC_DISTRIBUTOR_STRUCTURE, GicVersion) +
                       sizeof (UINT8)))
        {
          GicVersion = Gicd->GicVersion;
        }

        break;

      case EFI_ACPI_6_0_GICR:
        if (Length < (OFFSET_OF (EFI_ACPI_6_0_GICR_STRUCTURE, DiscoveryRangeLength) +
                      sizeof (UINT32)))
        {
          DEBUG ((
            DEBUG_WARN,
            "%a: GICR structure is %u bytes, too short for a discovery range; ignoring\n",
            __func__,
            (UINT32)Length
            ));
          break;
        }

        Gicr = (EFI_ACPI_6_0_GICR_STRUCTURE *)Ptr;
        GicrCount++;

        //
        // Several GICR structures are legal: they describe several
        // discovery ranges.  PcdGicRedistributorsBase carries one base,
        // and ArmGicDxe walks frames from it until GICR_TYPER.Last, so
        // it can never reach the others.  Keep the first and warn below,
        // rather than silently keeping whichever came last.
        //
        if (GicrCount == 1) {
          RedistBase = ReadUnaligned64 (&Gicr->DiscoveryRangeBaseAddress);
          RedistLen  = ReadUnaligned32 (&Gicr->DiscoveryRangeLength);
        }

        break;

      case EFI_ACPI_6_0_GIC:
        //
        // GICC.PhysicalBaseAddress is the memory-mapped CPU-interface
        // address that GicV2DxeInitialize() reads via
        // PcdGicInterruptInterfaceBase.  In GICv2 all PEs share one
        // GICC MMIO window (banked per-PE), which is why the PCD is a
        // single value; every enabled GICC entry should therefore
        // report the same address.  The ACPI spec permits it to be 0
        // on a platform without GICv2 compatibility support, so 0 is
        // "not derivable" here rather than a value to publish.
        //
        // The field has been present since the GICC structure was
        // introduced in ACPI 5.0, so bound it independently of
        // GICRBaseAddress below.
        //
        if (Length >= (OFFSET_OF (EFI_ACPI_6_0_GIC_STRUCTURE, PhysicalBaseAddress) +
                       sizeof (UINT64)))
        {
          Gicc          = (EFI_ACPI_6_0_GIC_STRUCTURE *)Ptr;
          ThisCpuIfBase = ReadUnaligned64 (&Gicc->PhysicalBaseAddress);

          if (ThisCpuIfBase != 0) {
            if (CpuIfBase == 0) {
              CpuIfBase = ThisCpuIfBase;
            } else if (CpuIfBase != ThisCpuIfBase) {
              CpuIfMismatch = TRUE;
            }
          }
        }

        //
        // GICRBaseAddress was added to the GICC structure in ACPI 5.1;
        // an older, shorter GICC simply does not describe it.
        //
        if (Length < (OFFSET_OF (EFI_ACPI_6_0_GIC_STRUCTURE, GICRBaseAddress) +
                      sizeof (UINT64)))
        {
          break;
        }

        Gicc = (EFI_ACPI_6_0_GIC_STRUCTURE *)Ptr;
        if (ReadUnaligned64 (&Gicc->GICRBaseAddress) != 0) {
          GiccRedistCount++;
        }

        break;

      default:
        break;
    }

    Ptr += Length;
  }

  if (GicrCount > 1) {
    DEBUG ((
      DEBUG_WARN,
      "%a: MADT has %u GICR structures; only the first discovery range is used\n",
      __func__,
      (UINT32)GicrCount
      ));
  }

  if (CpuIfMismatch) {
    DEBUG ((
      DEBUG_WARN,
      "%a: MADT GICC entries report differing PhysicalBaseAddress values; "
      "using 0x%Lx for PcdGicInterruptInterfaceBase\n",
      __func__,
      CpuIfBase
      ));
  }

  if (DistBase == 0) {
    DEBUG ((DEBUG_ERROR, "%a: MADT describes no GIC distributor\n", __func__));
    GicMadtFatal ();
    return EFI_NOT_FOUND;
  }

  //
  // ArmGicDxe (see ArmPkg/Drivers/ArmGicDxe/ArmGicDxe.c) chooses its
  // init path from the CPU, not from the MADT: GicV3Supported() checks
  // ArmHasGicSystemRegisters() and then whether ICC_SRE_EL2.SRE can be
  // set, because "the GICC IIDR Architecture version [...] does not
  // seem to be very reliable"; ArmHasGicV5SystemRegisters() also
  // selects the v3 path (ArmGicDxe.c:77).  Only when neither is
  // available does it drive the GIC as a v2.  Whether SRE sticks depends
  // on the higher exception level and cannot be predicted here without
  // repeating the write ArmGicDxe is about to perform.
  //
  // Use the same first-order predicate to decide which base is
  // required.  When system registers are present the v3 path is the
  // likely one and needs a redistributor; when they are absent the v2
  // path is certain and needs the CPU interface.  In either case
  // derive and publish the CPU-interface base whenever GICC supplies a
  // non-zero one, so the SRE-denied fallback to v2 has a correct value
  // rather than the QEMU-virt build-time default.
  //
  HaveSysRegs = ArmHasGicSystemRegisters ();

  if (HaveSysRegs) {
    if (GicrCount == 0) {
      if (GiccRedistCount != 0) {
        //
        // ACPI 6.0 lets a platform describe the redistributors per PE in
        // GICC.GICRBaseAddress instead of as one contiguous discovery
        // range, and that is exactly the case where the frames are not
        // contiguous.  PcdGicRedistributorsBase carries a single base and
        // ArmGicDxe walks frames from it until GICR_TYPER.Last, so any
        // range synthesised from the per-PE bases would be a guess about
        // the platform's layout.  Report it instead of guessing.
        //
        DEBUG ((
          DEBUG_ERROR,
          "%a: MADT describes the redistributors per PE in GICC.GICRBaseAddress "
          "(%u of them); PcdGicRedistributorsBase cannot express that\n",
          __func__,
          (UINT32)GiccRedistCount
          ));
        GicMadtFatal ();
        return EFI_UNSUPPORTED;
      }

      DEBUG ((
        DEBUG_ERROR,
        "%a: CPU has GICv3 system registers but the MADT describes no "
        "GIC redistributor in either form\n",
        __func__
        ));
      GicMadtFatal ();
      return EFI_NOT_FOUND;
    }

    if ((RedistBase == 0) || (RedistLen == 0)) {
      //
      // ArmGicDxe walks redistributor frames from the published base until
      // one reports GICR_TYPER.Last, so the whole discovery range has to
      // be mapped.  A GICv3 redistributor is 128 KiB and a GICv4 one is
      // 256 KiB, so no fixed guess covers an SMP system: the walk would
      // read past the mapping.  If the MADT gives no length, fail.
      //
      DEBUG ((
        DEBUG_ERROR,
        "%a: MADT GICR discovery range is unusable: base 0x%Lx, length 0x%Lx\n",
        __func__,
        RedistBase,
        RedistLen
        ));
      GicMadtFatal ();
      return EFI_UNSUPPORTED;
    }
  } else {
    if (CpuIfBase == 0) {
      DEBUG ((
        DEBUG_ERROR,
        "%a: CPU has no GICv3 system registers so ArmGicDxe will take the "
        "v2 path, but the MADT GICC entries describe no memory-mapped CPU "
        "interface (PhysicalBaseAddress is zero or absent)\n",
        __func__
        ));
      GicMadtFatal ();
      return EFI_NOT_FOUND;
    }
  }

  //
  // Cross-check the GICD.GicVersion byte against what the structures
  // and CPU imply, purely for diagnostics.  0 means the field is
  // absent (pre-6.0 GICD) or the firmware left it unspecified.  This
  // never gates anything for the reason quoted above.
  //
  if (GicVersion != 0) {
    if (HaveSysRegs && (GicVersion < EFI_ACPI_6_0_GIC_V3)) {
      DEBUG ((
        DEBUG_WARN,
        "%a: MADT GICD.GicVersion is %u but the CPU implements GICv3 "
        "system registers; ignoring the reported version\n",
        __func__,
        (UINT32)GicVersion
        ));
    } else if (!HaveSysRegs && (GicVersion >= EFI_ACPI_6_0_GIC_V3)) {
      DEBUG ((
        DEBUG_WARN,
        "%a: MADT GICD.GicVersion is %u but the CPU has no GICv3 system "
        "registers; ignoring the reported version\n",
        __func__,
        (UINT32)GicVersion
        ));
    }
  }

  //
  // The MADT carries no distributor length.  ArmGicDxe maps the
  // distributor as GICD_V3_SIZE (64 KiB) on the v3 path and
  // GICD_V2_SIZE (4 KiB) on the v2 path (see GicV3DxeInitialize()
  // and GicV2DxeInitialize() respectively), so map at least what the
  // path that will run needs.  Mapping the larger unconditionally is
  // not safe on a genuine GICv2 platform: the 60 KiB beyond the
  // distributor is not the GIC's, and if any of it is already in the
  // GCD map MapGicMmio() refuses to touch it and this constructor
  // halts a boot that would otherwise succeed.  HaveSysRegs is the
  // same first-order predicate GicV3Supported() uses to choose the
  // path, and the same predicate this constructor already used above
  // to decide sufficiency.  In the SRE-denied corner case (HaveSysRegs
  // TRUE, ArmGicDxe falls back to v2) 64 KiB is mapped where the v2
  // path uses only 4; that is safe because on such a platform the
  // distributor block is architecturally 64 KiB regardless of which
  // interface the driver chooses.
  //
  Status = MapGicMmio (DistBase, HaveSysRegs ? SIZE_64KB : SIZE_4KB);
  if (EFI_ERROR (Status)) {
    GicMadtFatal ();
    return Status;
  }

  PcdStatus = PcdSet64S (PcdGicDistributorBase, DistBase);
  ASSERT_RETURN_ERROR (PcdStatus);

  if ((GicrCount != 0) && (RedistBase != 0) && (RedistLen != 0)) {
    Status = MapGicMmio (RedistBase, RedistLen);
    if (EFI_ERROR (Status)) {
      GicMadtFatal ();
      return Status;
    }

    PcdStatus = PcdSet64S (PcdGicRedistributorsBase, RedistBase);
    ASSERT_RETURN_ERROR (PcdStatus);
  } else {
    DEBUG ((
      DEBUG_WARN,
      "%a: PcdGicRedistributorsBase left at its build-time default 0x%Lx\n",
      __func__,
      PcdGet64 (PcdGicRedistributorsBase)
      ));
  }

  if (CpuIfBase != 0) {
    //
    // GicV2DxeInitialize() maps the CPU interface itself as
    // GICC_V2_SIZE, i.e. 8 KiB.  Use the same size here.
    //
    Status = MapGicMmio (CpuIfBase, SIZE_8KB);
    if (EFI_ERROR (Status)) {
      GicMadtFatal ();
      return Status;
    }

    PcdStatus = PcdSet64S (PcdGicInterruptInterfaceBase, CpuIfBase);
    ASSERT_RETURN_ERROR (PcdStatus);
  } else {
    DEBUG ((
      DEBUG_WARN,
      "%a: PcdGicInterruptInterfaceBase left at its build-time default "
      "0x%Lx; the GICv2 fallback would use it if ICC_SRE_EL2.SRE were "
      "denied\n",
      __func__,
      PcdGet64 (PcdGicInterruptInterfaceBase)
      ));
  }

  DEBUG ((
    DEBUG_INFO,
    "%a: GICD 0x%Lx, GICR 0x%Lx (len 0x%Lx), GICC 0x%Lx from MADT; "
    "CPU %a GICv3 sysregs\n",
    __func__,
    DistBase,
    RedistBase,
    RedistLen,
    CpuIfBase,
    HaveSysRegs ? "has" : "lacks"
    ));

  return EFI_SUCCESS;
}
