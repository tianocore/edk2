/**@file

  Copyright (c) 2006 - 2016, Intel Corporation. All rights reserved.<BR>
  Copyright (c) 2011, Andrei Warkentin <andreiw@motorola.com>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

//
// The package level header files this module uses
//
#include <PiPei.h>

//
// The Library classes this module consumes
//
#include <Library/BaseMemoryLib.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include <Library/IoLib.h>
#include <IndustryStandard/I440FxPiix4.h>
#include <IndustryStandard/Microvm.h>
#include <IndustryStandard/Pci22.h>
#include <IndustryStandard/Q35MchIch9.h>
#include <IndustryStandard/QemuCpuHotplug.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/QemuFwCfgLib.h>
#include <Library/QemuFwCfgS3Lib.h>
#include <Library/QemuFwCfgSimpleParserLib.h>
#include <Library/PciLib.h>
#include <Guid/SystemNvDataGuid.h>
#include <Guid/VariableFormat.h>
#include <OvmfPlatforms.h>
#include <Library/TdxLib.h>

#include <Library/PlatformInitLib.h>

#define CPUHP_BUGCHECK_OVERRIDE_FWCFG_FILE \
  "opt/org.tianocore/X-Cpuhp-Bugcheck-Override"

VOID
EFIAPI
PlatformAddIoMemoryBaseSizeHob (
  IN EFI_PHYSICAL_ADDRESS  MemoryBase,
  IN UINT64                MemorySize
  )
{
  BuildResourceDescriptorHob (
    EFI_RESOURCE_MEMORY_MAPPED_IO,
    EFI_RESOURCE_ATTRIBUTE_PRESENT     |
    EFI_RESOURCE_ATTRIBUTE_INITIALIZED |
    EFI_RESOURCE_ATTRIBUTE_UNCACHEABLE |
    EFI_RESOURCE_ATTRIBUTE_TESTED,
    MemoryBase,
    MemorySize
    );
}

VOID
EFIAPI
PlatformAddReservedMemoryBaseSizeHob (
  IN EFI_PHYSICAL_ADDRESS  MemoryBase,
  IN UINT64                MemorySize,
  IN BOOLEAN               Cacheable
  )
{
  BuildResourceDescriptorHob (
    EFI_RESOURCE_MEMORY_RESERVED,
    EFI_RESOURCE_ATTRIBUTE_PRESENT     |
    EFI_RESOURCE_ATTRIBUTE_INITIALIZED |
    EFI_RESOURCE_ATTRIBUTE_UNCACHEABLE |
    (Cacheable ?
     EFI_RESOURCE_ATTRIBUTE_WRITE_COMBINEABLE |
     EFI_RESOURCE_ATTRIBUTE_WRITE_THROUGH_CACHEABLE |
     EFI_RESOURCE_ATTRIBUTE_WRITE_BACK_CACHEABLE :
     0
    ) |
    EFI_RESOURCE_ATTRIBUTE_TESTED,
    MemoryBase,
    MemorySize
    );
}

VOID
EFIAPI
PlatformAddIoMemoryRangeHob (
  IN EFI_PHYSICAL_ADDRESS  MemoryBase,
  IN EFI_PHYSICAL_ADDRESS  MemoryLimit
  )
{
  PlatformAddIoMemoryBaseSizeHob (MemoryBase, (UINT64)(MemoryLimit - MemoryBase));
}

VOID
EFIAPI
PlatformAddMemoryBaseSizeHob (
  IN EFI_PHYSICAL_ADDRESS  MemoryBase,
  IN UINT64                MemorySize
  )
{
  BuildResourceDescriptorHob (
    EFI_RESOURCE_SYSTEM_MEMORY,
    EFI_RESOURCE_ATTRIBUTE_PRESENT |
    EFI_RESOURCE_ATTRIBUTE_INITIALIZED |
    EFI_RESOURCE_ATTRIBUTE_UNCACHEABLE |
    EFI_RESOURCE_ATTRIBUTE_WRITE_COMBINEABLE |
    EFI_RESOURCE_ATTRIBUTE_WRITE_THROUGH_CACHEABLE |
    EFI_RESOURCE_ATTRIBUTE_WRITE_BACK_CACHEABLE |
    EFI_RESOURCE_ATTRIBUTE_TESTED,
    MemoryBase,
    MemorySize
    );
}

VOID
EFIAPI
PlatformAddMemoryRangeHob (
  IN EFI_PHYSICAL_ADDRESS  MemoryBase,
  IN EFI_PHYSICAL_ADDRESS  MemoryLimit
  )
{
  PlatformAddMemoryBaseSizeHob (MemoryBase, (UINT64)(MemoryLimit - MemoryBase));
}

VOID
EFIAPI
PlatformMemMapInitialization (
  IN OUT EFI_HOB_PLATFORM_INFO  *PlatformInfoHob
  )
{
  UINT64  PciIoBase;
  UINT64  PciIoSize;
  UINT64  PciExBarBase;
  UINT32  PciBase;
  UINT32  PciSize;

  PciIoBase = 0xC000;
  PciIoSize = 0x4000;

  //
  // Video memory + Legacy BIOS region
  //
  if (!TdIsEnabled ()) {
    PlatformAddIoMemoryRangeHob (0x0A0000, BASE_1MB);
  }

  if (PlatformInfoHob->HostBridgeDevId == 0xffff /* microvm */) {
    PlatformAddIoMemoryBaseSizeHob (MICROVM_GED_MMIO_BASE, SIZE_4KB);
    PlatformAddIoMemoryBaseSizeHob (0xFEC00000, SIZE_4KB); /* ioapic #1 */
    PlatformAddIoMemoryBaseSizeHob (0xFEC10000, SIZE_4KB); /* ioapic #2 */
    return;
  }

  //
  // address       purpose   size
  // ------------  --------  -------------------------
  // max(top, 2g)  PCI MMIO  0xFC000000 - max(top, 2g)  (pc)
  // max(top, 2g)  PCI MMIO  0xE0000000 - max(top, 2g)  (q35)
  // 0xE0000000    MMCONFIG                     256 MB  (q35)
  // 0xFC000000    gap                           44 MB
  // 0xFEC00000    IO-APIC                        4 KB
  // 0xFEC01000    gap                         1020 KB
  // 0xFED00000    HPET                           1 KB
  // 0xFED00400    gap                          111 KB
  // 0xFED1C000    gap (PIIX4) / RCRB (ICH9)     16 KB
  // 0xFED20000    gap                          896 KB
  // 0xFEE00000    LAPIC                          1 MB
  //
  PlatformGetSystemMemorySizeBelow4gb (PlatformInfoHob);
  PciBase      = PlatformInfoHob->Uc32Base;
  PciExBarBase = 0;
  if (PlatformInfoHob->HostBridgeDevId == INTEL_Q35_MCH_DEVICE_ID) {
    //
    // The MMCONFIG area is expected to fall between the top of low RAM and
    // the base of the 32-bit PCI host aperture.
    //
    PciExBarBase = PcdGet64 (PcdPciExpressBaseAddress);
    ASSERT (PlatformInfoHob->LowMemory <= PciExBarBase);
    ASSERT (PciExBarBase <= MAX_UINT32 - SIZE_256MB);
    PciSize = (UINT32)(PciExBarBase - PciBase);
  } else {
    ASSERT (PlatformInfoHob->LowMemory <= PlatformInfoHob->Uc32Base);
    PciSize = 0xFC000000 - PciBase;
  }

  PlatformAddIoMemoryBaseSizeHob (PciBase, PciSize);

  PlatformInfoHob->PcdPciMmio32Base = PciBase;
  PlatformInfoHob->PcdPciMmio32Size = PciSize;

  PlatformAddIoMemoryBaseSizeHob (0xFEC00000, SIZE_4KB);
  PlatformAddIoMemoryBaseSizeHob (0xFED00000, SIZE_1KB);
  if (PlatformInfoHob->HostBridgeDevId == INTEL_Q35_MCH_DEVICE_ID) {
    PlatformAddIoMemoryBaseSizeHob (ICH9_ROOT_COMPLEX_BASE, SIZE_16KB);
    //
    // Note: there should be an
    //
    //   PlatformAddIoMemoryBaseSizeHob (PciExBarBase, SIZE_256MB);
    //
    // call below, just like the one above for RCBA. However, Linux insists
    // that the MMCONFIG area be marked in the E820 or UEFI memory map as
    // "reserved memory" -- Linux does not content itself with a simple gap
    // in the memory map wherever the MCFG ACPI table points to.
    //
    // This appears to be a safety measure. The PCI Firmware Specification
    // (rev 3.1) says in 4.1.2. "MCFG Table Description": "The resources can
    // *optionally* be returned in [...] EFIGetMemoryMap as reserved memory
    // [...]". (Emphasis added here.)
    //
    // Normally we add memory resource descriptor HOBs in
    // QemuInitializeRam(), and pre-allocate from those with memory
    // allocation HOBs in InitializeRamRegions(). However, the MMCONFIG area
    // is most definitely not RAM; so, as an exception, cover it with
    // uncacheable reserved memory right here.
    //
    PlatformAddReservedMemoryBaseSizeHob (PciExBarBase, SIZE_256MB, FALSE);
    BuildMemoryAllocationHob (
      PciExBarBase,
      SIZE_256MB,
      EfiReservedMemoryType
      );
  }

  PlatformAddIoMemoryBaseSizeHob (PcdGet32 (PcdCpuLocalApicBaseAddress), SIZE_1MB);

  //
  // On Q35, the IO Port space is available for PCI resource allocations from
  // 0x6000 up.
  //
  if (PlatformInfoHob->HostBridgeDevId == INTEL_Q35_MCH_DEVICE_ID) {
    PciIoBase = 0x6000;
    PciIoSize = 0xA000;
    ASSERT ((ICH9_PMBASE_VALUE & 0xF000) < PciIoBase);
  }

  //
  // Add PCI IO Port space available for PCI resource allocations.
  //
  BuildResourceDescriptorHob (
    EFI_RESOURCE_IO,
    EFI_RESOURCE_ATTRIBUTE_PRESENT     |
    EFI_RESOURCE_ATTRIBUTE_INITIALIZED,
    PciIoBase,
    PciIoSize
    );

  PlatformInfoHob->PcdPciIoBase = PciIoBase;
  PlatformInfoHob->PcdPciIoSize = PciIoSize;
}

/**
 * Fetch "opt/ovmf/PcdSetNxForStack" from QEMU
 *
 * @param Setting     The pointer to the setting of "/opt/ovmf/PcdSetNxForStack".
 * @return EFI_SUCCESS  Successfully fetch the settings.
 */
EFI_STATUS
EFIAPI
PlatformNoexecDxeInitialization (
  IN OUT EFI_HOB_PLATFORM_INFO  *PlatformInfoHob
  )
{
  if (TdIsEnabled ()) {
    PlatformInfoHob->PcdSetNxForStack = TRUE;
    return EFI_SUCCESS;
  }

  return QemuFwCfgParseBool ("opt/ovmf/PcdSetNxForStack", &PlatformInfoHob->PcdSetNxForStack);
}

VOID
PciExBarInitialization (
  VOID
  )
{
  union {
    UINT64    Uint64;
    UINT32    Uint32[2];
  } PciExBarBase;

  //
  // We only support the 256MB size for the MMCONFIG area:
  // 256 buses * 32 devices * 8 functions * 4096 bytes config space.
  //
  // The masks used below enforce the Q35 requirements that the MMCONFIG area
  // be (a) correctly aligned -- here at 256 MB --, (b) located under 64 GB.
  //
  // Note that (b) also ensures that the minimum address width we have
  // determined in AddressWidthInitialization(), i.e., 36 bits, will suffice
  // for DXE's page tables to cover the MMCONFIG area.
  //
  PciExBarBase.Uint64 = PcdGet64 (PcdPciExpressBaseAddress);
  ASSERT ((PciExBarBase.Uint32[1] & MCH_PCIEXBAR_HIGHMASK) == 0);
  ASSERT ((PciExBarBase.Uint32[0] & MCH_PCIEXBAR_LOWMASK) == 0);

  //
  // Clear the PCIEXBAREN bit first, before programming the high register.
  //
  PciWrite32 (DRAMC_REGISTER_Q35 (MCH_PCIEXBAR_LOW), 0);

  //
  // Program the high register. Then program the low register, setting the
  // MMCONFIG area size and enabling decoding at once.
  //
  PciWrite32 (DRAMC_REGISTER_Q35 (MCH_PCIEXBAR_HIGH), PciExBarBase.Uint32[1]);
  PciWrite32 (
    DRAMC_REGISTER_Q35 (MCH_PCIEXBAR_LOW),
    PciExBarBase.Uint32[0] | MCH_PCIEXBAR_BUS_FF | MCH_PCIEXBAR_EN
    );
}

VOID
EFIAPI
PlatformMiscInitialization (
  IN EFI_HOB_PLATFORM_INFO  *PlatformInfoHob
  )
{
  UINTN   PmCmd;
  UINTN   Pmba;
  UINT32  PmbaAndVal;
  UINT32  PmbaOrVal;
  UINTN   AcpiCtlReg;
  UINT8   AcpiEnBit;

  //
  // Disable A20 Mask
  //
  if (PlatformInfoHob->HostBridgeDevId != CLOUDHV_DEVICE_ID) {
    IoOr8 (0x92, BIT1);
  }

  //
  // Build the CPU HOB with guest RAM size dependent address width and 16-bits
  // of IO space. (Side note: unlike other HOBs, the CPU HOB is needed during
  // S3 resume as well, so we build it unconditionally.)
  //
  BuildCpuHob (PlatformInfoHob->PhysMemAddressWidth, 16);

  //
  // Determine platform type and save Host Bridge DID to PCD
  //
  switch (PlatformInfoHob->HostBridgeDevId) {
    case INTEL_82441_DEVICE_ID:
      PmCmd      = POWER_MGMT_REGISTER_PIIX4 (PCI_COMMAND_OFFSET);
      Pmba       = POWER_MGMT_REGISTER_PIIX4 (PIIX4_PMBA);
      PmbaAndVal = ~(UINT32)PIIX4_PMBA_MASK;
      PmbaOrVal  = PIIX4_PMBA_VALUE;
      AcpiCtlReg = POWER_MGMT_REGISTER_PIIX4 (PIIX4_PMREGMISC);
      AcpiEnBit  = PIIX4_PMREGMISC_PMIOSE;
      break;
    case INTEL_Q35_MCH_DEVICE_ID:
      PmCmd      = POWER_MGMT_REGISTER_Q35 (PCI_COMMAND_OFFSET);
      Pmba       = POWER_MGMT_REGISTER_Q35 (ICH9_PMBASE);
      PmbaAndVal = ~(UINT32)ICH9_PMBASE_MASK;
      PmbaOrVal  = ICH9_PMBASE_VALUE;
      AcpiCtlReg = POWER_MGMT_REGISTER_Q35 (ICH9_ACPI_CNTL);
      AcpiEnBit  = ICH9_ACPI_CNTL_ACPI_EN;
      break;
    case CLOUDHV_DEVICE_ID:
      break;
    default:
      DEBUG ((
        DEBUG_ERROR,
        "%a: Unknown Host Bridge Device ID: 0x%04x\n",
        __func__,
        PlatformInfoHob->HostBridgeDevId
        ));
      ASSERT (FALSE);
      return;
  }

  if (PlatformInfoHob->HostBridgeDevId == CLOUDHV_DEVICE_ID) {
    DEBUG ((DEBUG_INFO, "%a: Cloud Hypervisor is done.\n", __func__));
    return;
  }

  //
  // If the appropriate IOspace enable bit is set, assume the ACPI PMBA has
  // been configured and skip the setup here. This matches the logic in
  // AcpiTimerLibConstructor ().
  //
  if ((PciRead8 (AcpiCtlReg) & AcpiEnBit) == 0) {
    //
    // The PEI phase should be exited with fully accessibe ACPI PM IO space:
    // 1. set PMBA
    //
    PciAndThenOr32 (Pmba, PmbaAndVal, PmbaOrVal);

    //
    // 2. set PCICMD/IOSE
    //
    PciOr8 (PmCmd, EFI_PCI_COMMAND_IO_SPACE);

    //
    // 3. set ACPI PM IO enable bit (PMREGMISC:PMIOSE or ACPI_CNTL:ACPI_EN)
    //
    PciOr8 (AcpiCtlReg, AcpiEnBit);
  }

  if (PlatformInfoHob->HostBridgeDevId == INTEL_Q35_MCH_DEVICE_ID) {
    //
    // Set Root Complex Register Block BAR
    //
    PciWrite32 (
      POWER_MGMT_REGISTER_Q35 (ICH9_RCBA),
      ICH9_ROOT_COMPLEX_BASE | ICH9_RCBA_EN
      );

    //
    // Set PCI Express Register Range Base Address
    //
    PciExBarInitialization ();
  }
}

/**
  Check for various QEMU bugs concerning CPU numbers.

  Compensate for those bugs if various conditions are satisfied, by updating a
  suitable subset of the input-output parameters. The function may not return
  (it may hang deliberately), even in RELEASE builds, if the QEMU bug is
  impossible to cover up.

  @param[in,out] BootCpuCount  On input, the boot CPU count reported by QEMU via
                               fw_cfg (QemuFwCfgItemSmpCpuCount). The caller is
                               responsible for ensuring (BootCpuCount > 0); that
                               is, if QEMU does not provide the boot CPU count
                               via fw_cfg *at all*, then this function must not
                               be called.

  @param[in,out] Present       On input, the number of present-at-boot CPUs, as
                               reported by QEMU through the modern CPU hotplug
                               register block.

  @param[in,out] Possible      On input, the number of possible CPUs, as
                               reported by QEMU through the modern CPU hotplug
                               register block.
**/
STATIC
VOID
PlatformCpuCountBugCheck (
  IN OUT UINT16  *BootCpuCount,
  IN OUT UINT32  *Present,
  IN OUT UINT32  *Possible
  )
{
  ASSERT (*BootCpuCount > 0);

  //
  // Sanity check: we need at least 1 present CPU (CPU#0 is always present).
  //
  // The legacy-to-modern switching of the CPU hotplug register block got broken
  // (for TCG) in QEMU v5.1.0. Refer to "IO port write width clamping differs
  // between TCG and KVM" at
  // <http://mid.mail-archive.com/aaedee84-d3ed-a4f9-21e7-d221a28d1683@redhat.com>
  // or at
  // <https://lists.gnu.org/archive/html/qemu-devel/2023-01/msg00199.html>.
  //
  // QEMU received the fix in commit dab30fbef389 ("acpi: cpuhp: fix
  // guest-visible maximum access size to the legacy reg block", 2023-01-08), to
  // be included in QEMU v8.0.0.
  //
  // If we're affected by this QEMU bug, then we must not continue: it confuses
  // the multiprocessing in UefiCpuPkg/Library/MpInitLib, and breaks CPU
  // hot(un)plug with SMI in OvmfPkg/CpuHotplugSmm.
  //
  if (*Present == 0) {
    UINTN                      Idx;
    STATIC CONST CHAR8 *CONST  Message[] = {
      "Broken CPU hotplug register block found. Update QEMU to version 8+, or",
      "to a stable release with commit dab30fbef389 backported. Refer to",
      "<https://bugzilla.tianocore.org/show_bug.cgi?id=4250>.",
      "Consequences of the QEMU bug may include, but are not limited to:",
      "- all firmware logic, dependent on the CPU hotplug register block,",
      "  being confused, for example, multiprocessing-related logic;",
      "- guest OS data loss, including filesystem corruption, due to crash or",
      "  hang during ACPI S3 resume;",
      "- SMM privilege escalation, by a malicious guest OS or 3rd partty UEFI",
      "  agent, against the platform firmware.",
      "These symptoms need not necessarily be limited to the QEMU user",
      "attempting to hot(un)plug a CPU.",
      "The firmware will now stop (hang) deliberately, in order to prevent the",
      "above symptoms.",
      "You can forcibly override the hang, *at your own risk*, with the",
      "following *experimental* QEMU command line option:",
      "  -fw_cfg name=" CPUHP_BUGCHECK_OVERRIDE_FWCFG_FILE ",string=yes",
      "Please only report such bugs that you can reproduce *without* the",
      "override.",
    };
    RETURN_STATUS              ParseStatus;
    BOOLEAN                    Override;

    DEBUG ((
      DEBUG_ERROR,
      "%a: Present=%u Possible=%u\n",
      __func__,
      *Present,
      *Possible
      ));
    for (Idx = 0; Idx < ARRAY_SIZE (Message); ++Idx) {
      DEBUG ((DEBUG_ERROR, "%a: %a\n", __func__, Message[Idx]));
    }

    ParseStatus = QemuFwCfgParseBool (
                    CPUHP_BUGCHECK_OVERRIDE_FWCFG_FILE,
                    &Override
                    );
    if (!RETURN_ERROR (ParseStatus) && Override) {
      DEBUG ((
        DEBUG_WARN,
        "%a: \"%a\" active. You've been warned.\n",
        __func__,
        CPUHP_BUGCHECK_OVERRIDE_FWCFG_FILE
        ));
      //
      // The bug is in QEMU v5.1.0+, where we're not affected by the QEMU v2.7
      // reset bug, so BootCpuCount from fw_cfg is reliable. Assume a fully
      // populated topology, like when the modern CPU hotplug interface is
      // unavailable.
      //
      *Present  = *BootCpuCount;
      *Possible = *BootCpuCount;
      return;
    }

    ASSERT (FALSE);
    CpuDeadLoop ();
  }

  //
  // Sanity check: fw_cfg and the modern CPU hotplug interface should expose the
  // same boot CPU count.
  //
  if (*BootCpuCount != *Present) {
    DEBUG ((
      DEBUG_WARN,
      "%a: QEMU v2.7 reset bug: BootCpuCount=%d Present=%u\n",
      __func__,
      *BootCpuCount,
      *Present
      ));
    //
    // The handling of QemuFwCfgItemSmpCpuCount, across CPU hotplug plus
    // platform reset (including S3), was corrected in QEMU commit e3cadac073a9
    // ("pc: fix FW_CFG_NB_CPUS to account for -device added CPUs", 2016-11-16),
    // part of release v2.8.0.
    //
    *BootCpuCount = (UINT16)*Present;
  }
}

/**
  Fetch the boot CPU count and the possible CPU count from QEMU, and expose
  them to UefiCpuPkg modules.
**/
VOID
EFIAPI
PlatformMaxCpuCountInitialization (
  IN OUT EFI_HOB_PLATFORM_INFO  *PlatformInfoHob
  )
{
  UINT16  BootCpuCount = 0;
  UINT32  MaxCpuCount;

  if (TdIsEnabled ()) {
    BootCpuCount = (UINT16)TdVCpuNum ();
    MaxCpuCount  = TdMaxVCpuNum ();

    if (BootCpuCount > MaxCpuCount) {
      DEBUG ((DEBUG_ERROR, "%a: Failed with BootCpuCount (%d) more than MaxCpuCount(%u) \n", __func__, BootCpuCount, MaxCpuCount));
      ASSERT (FALSE);
    }

    PlatformInfoHob->PcdCpuMaxLogicalProcessorNumber  = MaxCpuCount;
    PlatformInfoHob->PcdCpuBootLogicalProcessorNumber = BootCpuCount;
    return;
  }

  //
  // Try to fetch the boot CPU count.
  //
  if (QemuFwCfgIsAvailable ()) {
    QemuFwCfgSelectItem (QemuFwCfgItemSmpCpuCount);
    BootCpuCount = QemuFwCfgRead16 ();
  }

  if (BootCpuCount == 0) {
    //
    // QEMU doesn't report the boot CPU count. (BootCpuCount == 0) will let
    // MpInitLib count APs up to (PcdCpuMaxLogicalProcessorNumber - 1), or
    // until PcdCpuApInitTimeOutInMicroSeconds elapses (whichever is reached
    // first).
    //
    DEBUG ((DEBUG_WARN, "%a: boot CPU count unavailable\n", __func__));
    MaxCpuCount = PlatformInfoHob->DefaultMaxCpuNumber;
  } else {
    //
    // We will expose BootCpuCount to MpInitLib. MpInitLib will count APs up to
    // (BootCpuCount - 1) precisely, regardless of timeout.
    //
    // Now try to fetch the possible CPU count.
    //
    UINTN   CpuHpBase;
    UINT32  CmdData2;

    CpuHpBase = ((PlatformInfoHob->HostBridgeDevId == INTEL_Q35_MCH_DEVICE_ID) ?
                 ICH9_CPU_HOTPLUG_BASE : PIIX4_CPU_HOTPLUG_BASE);

    //
    // If only legacy mode is available in the CPU hotplug register block, or
    // the register block is completely missing, then the writes below are
    // no-ops.
    //
    // 1. Switch the hotplug register block to modern mode.
    //
    IoWrite32 (CpuHpBase + QEMU_CPUHP_W_CPU_SEL, 0);
    //
    // 2. Select a valid CPU for deterministic reading of
    //    QEMU_CPUHP_R_CMD_DATA2.
    //
    //    CPU#0 is always valid; it is the always present and non-removable
    //    BSP.
    //
    IoWrite32 (CpuHpBase + QEMU_CPUHP_W_CPU_SEL, 0);
    //
    // 3. Send a command after which QEMU_CPUHP_R_CMD_DATA2 is specified to
    //    read as zero, and which does not invalidate the selector. (The
    //    selector may change, but it must not become invalid.)
    //
    //    Send QEMU_CPUHP_CMD_GET_PENDING, as it will prove useful later.
    //
    IoWrite8 (CpuHpBase + QEMU_CPUHP_W_CMD, QEMU_CPUHP_CMD_GET_PENDING);
    //
    // 4. Read QEMU_CPUHP_R_CMD_DATA2.
    //
    //    If the register block is entirely missing, then this is an unassigned
    //    IO read, returning all-bits-one.
    //
    //    If only legacy mode is available, then bit#0 stands for CPU#0 in the
    //    "CPU present bitmap". CPU#0 is always present.
    //
    //    Otherwise, QEMU_CPUHP_R_CMD_DATA2 is either still reserved (returning
    //    all-bits-zero), or it is specified to read as zero after the above
    //    steps. Both cases confirm modern mode.
    //
    CmdData2 = IoRead32 (CpuHpBase + QEMU_CPUHP_R_CMD_DATA2);
    DEBUG ((DEBUG_VERBOSE, "%a: CmdData2=0x%x\n", __func__, CmdData2));
    if (CmdData2 != 0) {
      //
      // QEMU doesn't support the modern CPU hotplug interface. Assume that the
      // possible CPU count equals the boot CPU count (precluding hotplug).
      //
      DEBUG ((
        DEBUG_WARN,
        "%a: modern CPU hotplug interface unavailable\n",
        __func__
        ));
      MaxCpuCount = BootCpuCount;
    } else {
      //
      // Grab the possible CPU count from the modern CPU hotplug interface.
      //
      UINT32  Present, Possible, Selected;

      Present  = 0;
      Possible = 0;

      //
      // We've sent QEMU_CPUHP_CMD_GET_PENDING last; this ensures
      // QEMU_CPUHP_RW_CMD_DATA can now be read usefully. However,
      // QEMU_CPUHP_CMD_GET_PENDING may have selected a CPU with actual pending
      // hotplug events; therefore, select CPU#0 forcibly.
      //
      IoWrite32 (CpuHpBase + QEMU_CPUHP_W_CPU_SEL, Possible);

      do {
        UINT8  CpuStatus;

        //
        // Read the status of the currently selected CPU. This will help with
        // various CPU count sanity checks.
        //
        CpuStatus = IoRead8 (CpuHpBase + QEMU_CPUHP_R_CPU_STAT);
        if ((CpuStatus & QEMU_CPUHP_STAT_ENABLED) != 0) {
          ++Present;
        }

        //
        // Attempt to select the next CPU.
        //
        ++Possible;
        IoWrite32 (CpuHpBase + QEMU_CPUHP_W_CPU_SEL, Possible);
        //
        // If the selection is successful, then the following read will return
        // the selector (which we know is positive at this point). Otherwise,
        // the read will return 0.
        //
        Selected = IoRead32 (CpuHpBase + QEMU_CPUHP_RW_CMD_DATA);
        ASSERT (Selected == Possible || Selected == 0);
      } while (Selected > 0);

      PlatformCpuCountBugCheck (&BootCpuCount, &Present, &Possible);
      ASSERT (Present > 0);
      ASSERT (Present <= Possible);
      ASSERT (BootCpuCount == Present);

      MaxCpuCount = Possible;
    }
  }

  DEBUG ((
    DEBUG_INFO,
    "%a: BootCpuCount=%d MaxCpuCount=%u\n",
    __func__,
    BootCpuCount,
    MaxCpuCount
    ));
  ASSERT (BootCpuCount <= MaxCpuCount);

  PlatformInfoHob->PcdCpuMaxLogicalProcessorNumber  = MaxCpuCount;
  PlatformInfoHob->PcdCpuBootLogicalProcessorNumber = BootCpuCount;
}

/**
  Check padding data all bit should be 1.

  @param[in] Buffer     - A pointer to buffer header
  @param[in] BufferSize - Buffer size

  @retval  TRUE   - The padding data is valid.
  @retval  TRUE  - The padding data is invalid.

**/
BOOLEAN
CheckPaddingData (
  IN UINT8   *Buffer,
  IN UINT32  BufferSize
  )
{
  UINT32  index;

  for (index = 0; index < BufferSize; index++) {
    if (Buffer[index] != 0xFF) {
      return FALSE;
    }
  }

  return TRUE;
}

/**
  Check the integrity of NvVarStore.

  @param[in] NvVarStoreBase - A pointer to NvVarStore header
  @param[in] NvVarStoreSize - NvVarStore size

  @retval  TRUE   - The NvVarStore is valid.
  @retval  FALSE  - The NvVarStore is invalid.

**/
BOOLEAN
EFIAPI
PlatformValidateNvVarStore (
  IN UINT8   *NvVarStoreBase,
  IN UINT32  NvVarStoreSize
  )
{
  UINT16                         Checksum;
  UINTN                          VariableBase;
  UINT32                         VariableOffset;
  UINT32                         VariableOffsetBeforeAlign;
  EFI_FIRMWARE_VOLUME_HEADER     *NvVarStoreFvHeader;
  VARIABLE_STORE_HEADER          *NvVarStoreHeader;
  AUTHENTICATED_VARIABLE_HEADER  *VariableHeader;

  static EFI_GUID  FvHdrGUID       = EFI_SYSTEM_NV_DATA_FV_GUID;
  static EFI_GUID  VarStoreHdrGUID = EFI_AUTHENTICATED_VARIABLE_GUID;

  VariableOffset = 0;

  if (NvVarStoreBase == NULL) {
    DEBUG ((DEBUG_ERROR, "NvVarStore pointer is NULL.\n"));
    return FALSE;
  }

  //
  // Verify the header zerovetor, filesystemguid,
  // revision, signature, attributes, fvlength, checksum
  // HeaderLength cannot be an odd number
  //
  NvVarStoreFvHeader = (EFI_FIRMWARE_VOLUME_HEADER *)NvVarStoreBase;

  if ((!IsZeroBuffer (NvVarStoreFvHeader->ZeroVector, 16)) ||
      (!CompareGuid (&FvHdrGUID, &NvVarStoreFvHeader->FileSystemGuid)) ||
      (NvVarStoreFvHeader->Signature != EFI_FVH_SIGNATURE) ||
      (NvVarStoreFvHeader->Attributes != 0x4feff) ||
      ((NvVarStoreFvHeader->HeaderLength & 0x01) != 0) ||
      (NvVarStoreFvHeader->Revision != EFI_FVH_REVISION) ||
      (NvVarStoreFvHeader->FvLength != NvVarStoreSize)
      )
  {
    DEBUG ((DEBUG_ERROR, "NvVarStore FV headers were invalid.\n"));
    return FALSE;
  }

  //
  // Verify the header checksum
  //
  Checksum = CalculateSum16 ((VOID *)NvVarStoreFvHeader, NvVarStoreFvHeader->HeaderLength);

  if (Checksum != 0) {
    DEBUG ((DEBUG_ERROR, "NvVarStore FV checksum was invalid.\n"));
    return FALSE;
  }

  //
  // Verify the header signature, size, format, state
  //
  NvVarStoreHeader = (VARIABLE_STORE_HEADER *)(NvVarStoreBase + NvVarStoreFvHeader->HeaderLength);
  if ((!CompareGuid (&VarStoreHdrGUID, &NvVarStoreHeader->Signature)) ||
      (NvVarStoreHeader->Format != VARIABLE_STORE_FORMATTED) ||
      (NvVarStoreHeader->State != VARIABLE_STORE_HEALTHY) ||
      (NvVarStoreHeader->Size > (NvVarStoreFvHeader->FvLength - NvVarStoreFvHeader->HeaderLength)) ||
      (NvVarStoreHeader->Size < sizeof (VARIABLE_STORE_HEADER))
      )
  {
    DEBUG ((DEBUG_ERROR, "NvVarStore header signature/size/format/state were invalid.\n"));
    return FALSE;
  }

  //
  // Verify the header startId, state
  // Verify data to the end
  //
  VariableBase = (UINTN)NvVarStoreBase + NvVarStoreFvHeader->HeaderLength + sizeof (VARIABLE_STORE_HEADER);
  while (VariableOffset  < (NvVarStoreHeader->Size - sizeof (VARIABLE_STORE_HEADER))) {
    VariableHeader = (AUTHENTICATED_VARIABLE_HEADER *)(VariableBase + VariableOffset);
    if (VariableHeader->StartId != VARIABLE_DATA) {
      if (!CheckPaddingData ((UINT8 *)VariableHeader, NvVarStoreHeader->Size - sizeof (VARIABLE_STORE_HEADER) - VariableOffset)) {
        DEBUG ((DEBUG_ERROR, "NvVarStore variable header StartId was invalid.\n"));
        return FALSE;
      }

      VariableOffset = NvVarStoreHeader->Size - sizeof (VARIABLE_STORE_HEADER);
    } else {
      if (!((VariableHeader->State == VAR_HEADER_VALID_ONLY) ||
            (VariableHeader->State == VAR_ADDED) ||
            (VariableHeader->State == (VAR_ADDED & VAR_DELETED)) ||
            (VariableHeader->State == (VAR_ADDED & VAR_IN_DELETED_TRANSITION)) ||
            (VariableHeader->State == (VAR_ADDED & VAR_IN_DELETED_TRANSITION & VAR_DELETED))))
      {
        DEBUG ((DEBUG_ERROR, "NvVarStore Variable header State was invalid.\n"));
        return FALSE;
      }

      VariableOffset += sizeof (AUTHENTICATED_VARIABLE_HEADER) + VariableHeader->NameSize + VariableHeader->DataSize;
      // Verify VariableOffset should be less than or equal NvVarStoreHeader->Size - sizeof(VARIABLE_STORE_HEADER)
      if (VariableOffset > (NvVarStoreHeader->Size - sizeof (VARIABLE_STORE_HEADER))) {
        DEBUG ((DEBUG_ERROR, "NvVarStore Variable header VariableOffset was invalid.\n"));
        return FALSE;
      }

      VariableOffsetBeforeAlign = VariableOffset;
      // 4 byte align
      VariableOffset = (VariableOffset  + 3) & (UINTN)(~3);

      if (!CheckPaddingData ((UINT8 *)(VariableBase + VariableOffsetBeforeAlign), VariableOffset - VariableOffsetBeforeAlign)) {
        DEBUG ((DEBUG_ERROR, "NvVarStore Variable header PaddingData was invalid.\n"));
        return FALSE;
      }
    }
  }

  return TRUE;
}

/**
 Allocate storage for NV variables early on so it will be
 at a consistent address.  Since VM memory is preserved
 across reboots, this allows the NV variable storage to survive
 a VM reboot.

 *
 * @retval VOID* The pointer to the storage for NV Variables
 */
VOID *
EFIAPI
PlatformReserveEmuVariableNvStore (
  VOID
  )
{
  VOID    *VariableStore;
  UINT32  VarStoreSize;

  VarStoreSize = 2 * PcdGet32 (PcdFlashNvStorageFtwSpareSize);
  //
  // Allocate storage for NV variables early on so it will be
  // at a consistent address.  Since VM memory is preserved
  // across reboots, this allows the NV variable storage to survive
  // a VM reboot.
  //
  VariableStore =
    AllocateRuntimePages (
      EFI_SIZE_TO_PAGES (VarStoreSize)
      );
  DEBUG ((
    DEBUG_INFO,
    "Reserved variable store memory: 0x%p; size: %dkb\n",
    VariableStore,
    VarStoreSize / 1024
    ));

  return VariableStore;
}

/**
 When OVMF is lauched with -bios parameter, UEFI variables will be
 partially emulated, and non-volatile variables may lose their contents
 after a reboot. This makes the secure boot feature not working.

 This function is used to initialize the EmuVariableNvStore
 with the conent in PcdOvmfFlashNvStorageVariableBase.

 @param[in] EmuVariableNvStore      - A pointer to EmuVariableNvStore

 @retval  EFI_SUCCESS   - Successfully init the EmuVariableNvStore
 @retval  Others        - As the error code indicates
 */
EFI_STATUS
EFIAPI
PlatformInitEmuVariableNvStore (
  IN VOID  *EmuVariableNvStore
  )
{
  UINT8   *Base;
  UINT32  Size;
  UINT32  EmuVariableNvStoreSize;

  EmuVariableNvStoreSize = 2 * PcdGet32 (PcdFlashNvStorageFtwSpareSize);
  if ((EmuVariableNvStore == NULL) || (EmuVariableNvStoreSize == 0)) {
    DEBUG ((DEBUG_ERROR, "Invalid EmuVariableNvStore parameter.\n"));
    return EFI_INVALID_PARAMETER;
  }

  Base = (UINT8 *)(UINTN)PcdGet32 (PcdOvmfFlashNvStorageVariableBase);
  Size = (UINT32)PcdGet32 (PcdFlashNvStorageVariableSize);
  ASSERT (Size < EmuVariableNvStoreSize);

  if (!PlatformValidateNvVarStore (Base, PcdGet32 (PcdCfvRawDataSize))) {
    ASSERT (FALSE);
    return EFI_INVALID_PARAMETER;
  }

  DEBUG ((DEBUG_INFO, "Init EmuVariableNvStore with the content in FlashNvStorage\n"));

  CopyMem (EmuVariableNvStore, Base, Size);

  return EFI_SUCCESS;
}
