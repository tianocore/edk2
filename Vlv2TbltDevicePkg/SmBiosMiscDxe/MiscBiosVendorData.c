/** @file

Copyright (c) 2004  - 2014, Intel Corporation. All rights reserved.<BR>
                                                                                   
  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.  
  The full text of the license may be found at                                     
  http://opensource.org/licenses/bsd-license.php.                                  
                                                                                   
  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,            
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.    
                                                                                   


Module Name:

  MiscBiosVendorData.c

Abstract:

  Static data of BIOS vendor information.
  BIOS vendor information is Misc. subclass type 2 and SMBIOS type 0.


**/


#include "CommonHeader.h"

#include "MiscSubclassDriver.h"

//
// Static (possibly build generated) Bios Vendor data.
//
MISC_SMBIOS_TABLE_DATA(EFI_MISC_BIOS_VENDOR_DATA, MiscBiosVendor)
= {
  STRING_TOKEN(STR_MISC_BIOS_VENDOR),       // BiosVendor
  STRING_TOKEN(STR_MISC_BIOS_VERSION),      // BiosVersion
  STRING_TOKEN(STR_MISC_BIOS_RELEASE_DATE), // BiosReleaseDate
  0xF000, // BiosStartingAddress
  {       // BiosPhysicalDeviceSize
    1,    // Value
    21 ,          // Exponent
  },
  {       // BiosCharacteristics1
    0,    // Reserved1                         :2
    0,    // Unknown                           :1
    0,    // BiosCharacteristicsNotSupported   :1
    0,    // IsaIsSupported                    :1
    0,    // McaIsSupported                    :1
    0,    // EisaIsSupported                   :1
    1,    // PciIsSupported                    :1
    0,    // PcmciaIsSupported                 :1
    0,    // PlugAndPlayIsSupported            :1
    0,    // ApmIsSupported                    :1
    1,    // BiosIsUpgradable                  :1
    1,    // BiosShadowingAllowed              :1
    0,    // VlVesaIsSupported                 :1
    0,    // EscdSupportIsAvailable            :1
    1,    // BootFromCdIsSupported             :1
    1,    // SelectableBootIsSupported         :1
    0,    // RomBiosIsSocketed                 :1
    0,    // BootFromPcmciaIsSupported         :1
    1,    // EDDSpecificationIsSupported       :1
    0,    // JapaneseNecFloppyIsSupported      :1
    0,    // JapaneseToshibaFloppyIsSupported  :1
    0,    // Floppy525_360IsSupported          :1
    0,    // Floppy525_12IsSupported           :1
    0,    // Floppy35_720IsSupported           :1
    0,    // Floppy35_288IsSupported           :1
    0,    // PrintScreenIsSupported            :1
    1,    // Keyboard8042IsSupported           :1
    1,    // SerialIsSupported                 :1
    1,    // PrinterIsSupported                :1
    1,    // CgaMonoIsSupported                :1
    0,    // NecPc98                           :1

//
//BIOS Characteristics Extension Byte 1
//
    1,    // AcpiIsSupported                   :1
    1,    // UsbLegacyIsSupported              :1
    0,    // AgpIsSupported                    :1
    0,    // I20BootIsSupported                :1
    0,    // Ls120BootIsSupported              :1
    1,    // AtapiZipDriveBootIsSupported      :1
    0,    // Boot1394IsSupported               :1
    0,    // SmartBatteryIsSupported           :1

//
//BIOS Characteristics Extension Byte 2
//
    1,    // BiosBootSpecIsSupported           :1
    1,    // FunctionKeyNetworkBootIsSupported :1
    0x1     // Reserved                          :19  Bit 2 is SMBiosIsTargContDistEnabled
  },
  {       // BiosCharacteristics2
    0x0001,// BiosReserved                      :16  Bit 0 is BIOS Splash Screen
    0,    // SystemReserved                    :16
    0     // Reserved                          :32
  },
  0xFF,   // BiosMajorRelease;
  0xFF,   // BiosMinorRelease;
  0xFF,   // BiosEmbeddedFirmwareMajorRelease;
  0xFF,   // BiosEmbeddedFirmwareMinorRelease;
};
