/** @file
BIOS vendor information static data.
Misc. subclass type 2.
SMBIOS type 0.

Copyright (c) 2013-2015 Intel Corporation.

SPDX-License-Identifier: BSD-2-Clause-Patent


**/


#include "CommonHeader.h"

#include "SmbiosMisc.h"


//
// Static (possibly build generated) Bios Vendor data.
//
MISC_SMBIOS_TABLE_DATA(EFI_MISC_BIOS_VENDOR, MiscBiosVendor) = {
  STRING_TOKEN (STR_MISC_BIOS_VENDOR),       // BiosVendor
  STRING_TOKEN (STR_MISC_BIOS_VERSION),      // BiosVersion
  STRING_TOKEN (STR_MISC_BIOS_RELEASE_DATE), // BiosReleaseDate
  0xE0000,                    // BiosStartingAddress
  {                           // BiosPhysicalDeviceSize
    2,                        // Value
    20,                       // Exponent
  },
  {                           // BiosCharacteristics1
    0,                        // Reserved1                         :2
    0,                        // Unknown                           :1
    0,                        // BiosCharacteristicsNotSupported   :1
    0,                        // IsaIsSupported                    :1
    0,                        // McaIsSupported                    :1

    0,                        // EisaIsSupported                   :1
    1,                        // PciIsSupported                    :1
    0,                        // PcmciaIsSupported                 :1
    0,                        // PlugAndPlayIsSupported            :1
    0,                        // ApmIsSupported                    :1

    1,                        // BiosIsUpgradable                  :1
    1,                        // BiosShadowingAllowed              :1
    0,                        // VlVesaIsSupported                 :1
    0,                        // EscdSupportIsAvailable            :1
    1,                        // BootFromCdIsSupported             :1

    1,                        // SelectableBootIsSupported         :1
    0,                        // RomBiosIsSocketed                 :1
    0,                        // BootFromPcmciaIsSupported         :1
    1,                        // EDDSpecificationIsSupported       :1
    0,                        // JapaneseNecFloppyIsSupported      :1

    0,                        // JapaneseToshibaFloppyIsSupported  :1
    0,                        // Floppy525_360IsSupported          :1
    0,                        // Floppy525_12IsSupported           :1
    0,                        // Floppy35_720IsSupported           :1
    0,                        // Floppy35_288IsSupported           :1

    1,                        // PrintScreenIsSupported            :1
    1,                        // Keyboard8042IsSupported           :1
    1,                        // SerialIsSupported                 :1
    1,                        // PrinterIsSupported                :1
    1,                        // CgaMonoIsSupported                :1

    0,                        // NecPc98                           :1
    1,                        // AcpiIsSupported                   :1
    1,                        // UsbLegacyIsSupported              :1
    0,                        // AgpIsSupported                    :1
    0,                        // I20BootIsSupported                :1

    0,                        // Ls120BootIsSupported              :1
    0,                        // AtapiZipDriveBootIsSupported      :1
    0,                        // Boot1394IsSupported               :1
    0,                        // SmartBatteryIsSupported           :1
    1,                        // BiosBootSpecIsSupported           :1

    1,                        // FunctionKeyNetworkBootIsSupported :1
    0                         // Reserved                          :22
  },
  {                           // BiosCharacteristics2
    0,                        // BiosReserved                      :16
    0,                        // SystemReserved                    :16
    0                         // Reserved                          :32
  },
  0x1,                        // System BIOS Major Release
  0x0,                        // System BIOS Minor Release
  0xFF,                       // Embedded controller firmware major Release
  0xFF,                       // Embedded controller firmware minor Release
};
