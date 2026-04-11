/** @file
  This file provides Smbios Type0 Data

  Based on the files under Nt32Pkg/MiscSubClassPlatformDxe/

  Copyright (c) 2021, NUVIA Inc. All rights reserved.<BR>
  Copyright (c) 2006 - 2009, Intel Corporation. All rights reserved.<BR>
  Copyright (c) 2015, Hisilicon Limited. All rights reserved.<BR>
  Copyright (c) 2015, Linaro Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "SmbiosMisc.h"

//
// Static (possibly build generated) Bios Vendor data.
//
SMBIOS_MISC_TABLE_DATA (SMBIOS_TABLE_TYPE0, MiscBiosVendor) = {
  {                                          // Hdr
    EFI_SMBIOS_TYPE_BIOS_INFORMATION,        // Type,
    0,                                       // Length,
    0                                        // Handle
  },
  1,                                         // Vendor
  2,                                         // BiosVersion
  0xE000,                                    // BiosSegment
  3,                                         // BiosReleaseDate
  0,                                         // BiosSize
  {                                          // BiosCharacteristics
    0,                                       // Reserved                          :2
    0,                                       // Unknown                           :1
    0,                                       // BiosCharacteristicsNotSupported   :1
    0,                                       // IsaIsSupported                    :1
    0,                                       // McaIsSupported                    :1
    0,                                       // EisaIsSupported                   :1
    1,                                       // PciIsSupported                    :1
    0,                                       // PcmciaIsSupported                 :1
    1,                                       // PlugAndPlayIsSupported            :1
    0,                                       // ApmIsSupported                    :1
    1,                                       // BiosIsUpgradable                  :1
    1,                                       // BiosShadowingAllowed              :1
    0,                                       // VlVesaIsSupported                 :1
    0,                                       // EscdSupportIsAvailable            :1
    1,                                       // BootFromCdIsSupported             :1
    1,                                       // SelectableBootIsSupported         :1
    0,                                       // RomBiosIsSocketed                 :1
    0,                                       // BootFromPcmciaIsSupported         :1
    0,                                       // EDDSpecificationIsSupported       :1
    0,                                       // JapaneseNecFloppyIsSupported      :1
    0,                                       // JapaneseToshibaFloppyIsSupported  :1
    0,                                       // Floppy525_360IsSupported          :1
    0,                                       // Floppy525_12IsSupported           :1
    0,                                       // Floppy35_720IsSupported           :1
    0,                                       // Floppy35_288IsSupported           :1
    0,                                       // PrintScreenIsSupported            :1
    0,                                       // Keyboard8042IsSupported           :1
    0,                                       // SerialIsSupported                 :1
    0,                                       // PrinterIsSupported                :1
    0,                                       // CgaMonoIsSupported                :1
    0,                                       // NecPc98                           :1
    0                                        // ReservedForVendor                 :32
  },

  {
    0x01,                                        // BIOSCharacteristicsExtensionBytes[0]
    //  {                                          // BiosReserved
    //    1,                                         // AcpiIsSupported                   :1
    //    0,                                         // UsbLegacyIsSupported              :1
    //    0,                                         // AgpIsSupported                    :1
    //    0,                                         // I20BootIsSupported                :1
    //    0,                                         // Ls120BootIsSupported              :1
    //    0,                                         // AtapiZipDriveBootIsSupported      :1
    //    0,                                         // Boot1394IsSupported               :1
    //    0                                          // SmartBatteryIsSupported           :1
    //  },
    0x0C                                         // BIOSCharacteristicsExtensionBytes[1]
    //  {                                          //SystemReserved
    //    0,                                         //BiosBootSpecIsSupported            :1
    //    0,                                         //FunctionKeyNetworkBootIsSupported  :1
    //    1,                                         //TargetContentDistributionEnabled   :1
    //    1,                                         //UefiSpecificationSupported         :1
    //    0,                                         //VirtualMachineSupported            :1
    //    0                                          //ExtensionByte2Reserved             :3
    //  },
  },
  0xFF,                                     // SystemBiosMajorRelease;
  0xFF,                                     // SystemBiosMinorRelease;
  0xFF,                                     // EmbeddedControllerFirmwareMajorRelease;
  0xFF                                      // EmbeddedControllerFirmwareMinorRelease;
};
