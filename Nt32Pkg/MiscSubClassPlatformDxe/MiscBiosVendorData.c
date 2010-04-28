/**@file

Copyright (c) 2006 - 2009, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  MiscBiosVendorData.c
  
Abstract: 

  This driver parses the mMiscSubclassDataTable structure and reports
  any generated data to the DataHub.

**/

#include "MiscSubclassDriver.h"

//
// Static (possibly build generated) Bios Vendor data.
//
MISC_SMBIOS_TABLE_DATA(EFI_MISC_BIOS_VENDOR_DATA, MiscBiosVendor) = {
  STRING_TOKEN(STR_MISC_BIOS_VENDOR),       // BiosVendor
  STRING_TOKEN(STR_MISC_BIOS_VERSION),      // BiosVersion
  STRING_TOKEN(STR_MISC_BIOS_RELEASE_DATE), // BiosReleaseDate
  0xBABE, // BiosStartingAddress
  {       // BiosPhysicalDeviceSize
    2,    // Value
    3,    // Exponent
  },
  {       // BiosCharacteristics1
    0,    // Reserved1                         :2
    0,    // Unknown                           :1
    1,    // BiosCharacteristicsNotSupported   :1
    0,    // IsaIsSupported                    :1
    0,    // McaIsSupported                    :1
    0,    // EisaIsSupported                   :1
    0,    // PciIsSupported                    :1
    0,    // PcmciaIsSupported                 :1
    0,    // PlugAndPlayIsSupported            :1
    0,    // ApmIsSupported                    :1
    0,    // BiosIsUpgradable                  :1
    0,    // BiosShadowingAllowed              :1
    0,    // VlVesaIsSupported                 :1
    0,    // EscdSupportIsAvailable            :1
    0,    // BootFromCdIsSupported             :1
    0,    // SelectableBootIsSupported         :1
    0,    // RomBiosIsSocketed                 :1
    0,    // BootFromPcmciaIsSupported         :1
    0,    // EDDSpecificationIsSupported       :1
    0,    // JapaneseNecFloppyIsSupported      :1
    0,    // JapaneseToshibaFloppyIsSupported  :1
    0,    // Floppy525_360IsSupported          :1
    0,    // Floppy525_12IsSupported           :1
    0,    // Floppy35_720IsSupported           :1
    0,    // Floppy35_288IsSupported           :1
    0,    // PrintScreenIsSupported            :1
    0,    // Keyboard8042IsSupported           :1
    0,    // SerialIsSupported                 :1
    0,    // PrinterIsSupported                :1
    0,    // CgaMonoIsSupported                :1
    0,    // NecPc98                           :1
    0,    // AcpiIsSupported                   :1
    0,    // UsbLegacyIsSupported              :1
    0,    // AgpIsSupported                    :1
    0,    // I20BootIsSupported                :1
    0,    // Ls120BootIsSupported              :1
    0,    // AtapiZipDriveBootIsSupported      :1
    0,    // Boot1394IsSupported               :1
    0,    // SmartBatteryIsSupported           :1
    0,    // BiosBootSpecIsSupported           :1
    0,    // FunctionKeyNetworkBootIsSupported :1
    0     // Reserved                          :22
  },
  {       // BiosCharacteristics2
    0,    // BiosReserved                      :16
    0,    // SystemReserved                    :16
    0     // Reserved                          :32
  },
};

/* eof - MiscBiosVendorData.c */
