## @file
# FSP DSC build file for QEMU platform
#
# Copyright (c) 2017 - 2021, Intel Corporation. All rights reserved.<BR>
#
#    This program and the accompanying materials
#    are licensed and made available under the terms and conditions of the BSD License
#    which accompanies this distribution. The full text of the license may be found at
#    http://opensource.org/licenses/bsd-license.php
#
#    THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
#    WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
#
##

################################################################################
#
# Defines Section - statements that will be processed to create a Makefile.
#
################################################################################
[Defines]
  PLATFORM_NAME                  = QemuFspPkg
  PLATFORM_GUID                  = 1BEDB57A-7904-406e-8486-C89FC7FB39EE
  PLATFORM_VERSION               = 0.1
  DSC_SPECIFICATION              = 0x00010005
  OUTPUT_DIRECTORY               = Build/QemuFspPkg
  SUPPORTED_ARCHITECTURES        = IA32|X64
  BUILD_TARGETS                  = DEBUG|RELEASE
  SKUID_IDENTIFIER               = DEFAULT
  FLASH_DEFINITION               = QemuFspPkg/QemuFspPkg.fdf

  #
  # UPD tool definition
  #
  FSP_T_UPD_TOOL_GUID            = 34686CA3-34F9-4901-B82A-BA630F0714C6
  FSP_V_UPD_TOOL_GUID            = 4E2F4725-734A-4399-BAF5-B4E16348EB2F
  FSP_M_UPD_TOOL_GUID            = 39A250DB-E465-4DD1-A2AC-E2BD3C0E2385
  FSP_S_UPD_TOOL_GUID            = CAE3605B-5B34-4C85-B3D7-27D54273C40F
  FSP_T_UPD_FFS_GUID             = 70BCF6A5-FFB1-47D8-B1AE-EFE5508E23EA
  FSP_V_UPD_FFS_GUID             = 0197EF5E-2FFC-4089-8E55-F70400B18146
  FSP_M_UPD_FFS_GUID             = D5B86AEA-6AF7-40D4-8014-982301BC3D89
  FSP_S_UPD_FFS_GUID             = E3CD9B18-998C-4F76-B65E-98B154E5446F

  #
  # Set platform specific package/folder name, same as passed from PREBUILD script.
  # PLATFORM_PACKAGE would be the same as PLATFORM_NAME as well as package build folder
  # DEFINE only takes effect at R9 DSC and FDF.
  #
  DEFINE FSP_PACKAGE                     = QemuFspPkg
  DEFINE FSP_IMAGE_ID                    = 0x245053464D455124  # $QEMFSP$
  DEFINE FSP_IMAGE_REV                   = 0x00001010

  DEFINE CAR_BASE_ADDRESS                = 0x00000000
  DEFINE CAR_REGION_SIZE                 = 0x00080000
  DEFINE CAR_BLD_REGION_SIZE             = 0x00070000
  DEFINE CAR_FSP_REGION_SIZE             = 0x00010000

  DEFINE FSP_ARCH                        = X64

################################################################################
#
# SKU Identification section - list of all SKU IDs supported by this
#                              Platform.
#
################################################################################
[SkuIds]
  0|DEFAULT              # The entry: 0|DEFAULT is reserved and always required.

################################################################################
#
# Library Class section - list of all Library Classes needed by this Platform.
#
################################################################################

!include MdePkg/MdeLibs.dsc.inc

[LibraryClasses]
  PeiCoreEntryPoint|MdePkg/Library/PeiCoreEntryPoint/PeiCoreEntryPoint.inf
  PeimEntryPoint|MdePkg/Library/PeimEntryPoint/PeimEntryPoint.inf
  DebugPrintErrorLevelLib|MdePkg/Library/BaseDebugPrintErrorLevelLib/BaseDebugPrintErrorLevelLib.inf
  BaseLib|MdePkg/Library/BaseLib/BaseLib.inf
  IoLib|MdePkg/Library/BaseIoLibIntrinsic/BaseIoLibIntrinsic.inf
  PciLib|MdePkg/Library/BasePciLibPciExpress/BasePciLibPciExpress.inf
  PciCf8Lib|MdePkg/Library/BasePciCf8Lib/BasePciCf8Lib.inf
  PciExpressLib|MdePkg/Library/BasePciExpressLib/BasePciExpressLib.inf
  BaseMemoryLib|MdePkg/Library/BaseMemoryLibRepStr/BaseMemoryLibRepStr.inf
  PrintLib|MdePkg/Library/BasePrintLib/BasePrintLib.inf
  PcdLib|MdePkg/Library/PeiPcdLib/PeiPcdLib.inf
  HobLib|MdePkg/Library/PeiHobLib/PeiHobLib.inf
  PeiServicesTablePointerLib|MdePkg/Library/PeiServicesTablePointerLibIdt/PeiServicesTablePointerLibIdt.inf
  PeiServicesLib|MdePkg/Library/PeiServicesLib/PeiServicesLib.inf
  MemoryAllocationLib|MdePkg/Library/PeiMemoryAllocationLib/PeiMemoryAllocationLib.inf
  PeCoffGetEntryPointLib|MdePkg/Library/BasePeCoffGetEntryPointLib/BasePeCoffGetEntryPointLib.inf
  ReportStatusCodeLib|MdeModulePkg/Library/PeiReportStatusCodeLib/PeiReportStatusCodeLib.inf
  CacheMaintenanceLib|MdePkg/Library/BaseCacheMaintenanceLib/BaseCacheMaintenanceLib.inf
  PeCoffLib|MdePkg/Library/BasePeCoffLib/BasePeCoffLib.inf
  PeCoffExtraActionLib|MdePkg/Library/BasePeCoffExtraActionLibNull/BasePeCoffExtraActionLibNull.inf
  UefiDecompressLib|MdePkg/Library/BaseUefiDecompressLib/BaseUefiDecompressLib.inf
  SynchronizationLib|MdePkg/Library/BaseSynchronizationLib/BaseSynchronizationLib.inf
  CpuLib|MdePkg/Library/BaseCpuLib/BaseCpuLib.inf
  ExtractGuidedSectionLib|MdePkg/Library/PeiExtractGuidedSectionLib/PeiExtractGuidedSectionLib.inf
  CacheLib|IntelFsp2Pkg/Library/BaseCacheLib/BaseCacheLib.inf
  CacheAsRamLib|IntelFsp2Pkg/Library/BaseCacheAsRamLibNull/BaseCacheAsRamLibNull.inf
  FspSwitchStackLib|IntelFsp2Pkg/Library/BaseFspSwitchStackLib/BaseFspSwitchStackLib.inf
  FspCommonLib|IntelFsp2Pkg/Library/BaseFspCommonLib/BaseFspCommonLib.inf
  FspPlatformLib|IntelFsp2Pkg/Library/BaseFspPlatformLib/BaseFspPlatformLib.inf
  PlatformHookLib|MdeModulePkg/Library/BasePlatformHookLibNull/BasePlatformHookLibNull.inf
  PerformanceLib|MdePkg/Library/BasePerformanceLibNull/BasePerformanceLibNull.inf
  OemHookStatusCodeLib|MdeModulePkg/Library/OemHookStatusCodeLibNull/OemHookStatusCodeLibNull.inf
!if $(TARGET) == DEBUG
  DebugLib|MdePkg/Library/BaseDebugLibSerialPort/BaseDebugLibSerialPort.inf
  SerialPortLib|MdeModulePkg/Library/BaseSerialPortLib16550/BaseSerialPortLib16550.inf
!else
  DebugLib|MdePkg/Library/BaseDebugLibNull/BaseDebugLibNull.inf
  SerialPortLib|MdePkg/Library/BaseSerialPortLibNull/BaseSerialPortLibNull.inf
!endif


################################################################################
#
# Pcd Section - list of all EDK II PCD Entries defined by this Platform
#
################################################################################
[PcdsFixedAtBuild]
  gEfiMdeModulePkgTokenSpaceGuid.PcdShadowPeimOnS3Boot    | TRUE
  gQemuFspPkgTokenSpaceGuid.PcdFspHeaderRevision          | 0x03
  gQemuFspPkgTokenSpaceGuid.PcdFspImageIdString           | $(FSP_IMAGE_ID)
  gQemuFspPkgTokenSpaceGuid.PcdFspImageRevision           | $(FSP_IMAGE_REV)
  #
  # FSP CAR Usages  (BL RAM | FSP RAM | FSP CODE)
  #
  gIntelFsp2PkgTokenSpaceGuid.PcdTemporaryRamBase         | $(CAR_BASE_ADDRESS)
  gIntelFsp2PkgTokenSpaceGuid.PcdTemporaryRamSize         | $(CAR_REGION_SIZE)
  gIntelFsp2PkgTokenSpaceGuid.PcdFspTemporaryRamSize      | $(CAR_FSP_REGION_SIZE)
  gIntelFsp2PkgTokenSpaceGuid.PcdFspReservedBufferSize    | 0x0100

  # This defines how much space will be used for heap in FSP temporary memory
  # x % of FSP temporary memory will be used for heap
  # (100 - x) % of FSP temporary memory will be used for stack
  gIntelFsp2PkgTokenSpaceGuid.PcdFspHeapSizePercentage    | 65

  # This is a platform specific global pointer used by FSP
  gIntelFsp2PkgTokenSpaceGuid.PcdGlobalDataPointerAddress | 0xFED00148
  gIntelFsp2PkgTokenSpaceGuid.PcdFspReservedMemoryLength  | 0x00100000

!if $(TARGET) == RELEASE
  gEfiMdePkgTokenSpaceGuid.PcdFixedDebugPrintErrorLevel   | 0x00000000
  gEfiMdePkgTokenSpaceGuid.PcdDebugPropertyMask           | 0
!else
  gEfiMdePkgTokenSpaceGuid.PcdFixedDebugPrintErrorLevel   | 0x80000047
  gEfiMdePkgTokenSpaceGuid.PcdDebugPropertyMask           | 0x27
!endif

[PcdsPatchableInModule]
  gEfiMdePkgTokenSpaceGuid.PcdPciExpressBaseAddress       | 0xE0000000
  #
  # This entry will be patched during the build process
  #
  gEfiMdeModulePkgTokenSpaceGuid.PcdVpdBaseAddress        | 0x12345678

!if $(TARGET) == RELEASE
  gEfiMdePkgTokenSpaceGuid.PcdDebugPrintErrorLevel        | 0
!else
  gEfiMdePkgTokenSpaceGuid.PcdDebugPrintErrorLevel        | 0x80000047
!endif

[PcdsDynamicVpd.Upd]
  #
  # This section is not used by the normal build process
  # However, FSP will use dedicated tool to handle it and generate a
  # VPD similar binary block (User Configuration Data). This block will
  # be accessed through a generated data structure directly rather than
  # PCD services. This is for size consideration.
  # Format:
  #   gQemuFspPkgTokenSpaceGuid.Updxxxxxxxxxxxxn      | OFFSET | LENGTH | VALUE
  # Only simple data type is supported
  #

  #
  # Comments with !BSF will be used to generate BSF file
  # Comments with !HDR will be used to generate H header file
  #

  # Global definitions in BSF
  # !BSF PAGES:{TMP:"FSP T", MEM:"FSP MemoryInit Settings", SIL:"FSP SiliconInit Settings"}
  # !BSF BLOCK:{NAME:"QEMU Platform", VER:"0.1"}

  # !BSF FIND:{QEMUPD_T}
  # !HDR COMMENT:{FSP_UPD_HEADER:FSP UPD Header}
  # !HDR EMBED:{FSP_UPD_HEADER:FspUpdHeader:START}
  # FsptUpdSignature: {QEMUPD_T}
  gQemuFspPkgTokenSpaceGuid.Signature                   | * | 0x08 | 0x545F4450554D4551
  # !BSF NAME:{FsptUpdRevision}
  gQemuFspPkgTokenSpaceGuid.Revision                    | * | 0x01 | 0x01
  # !HDR EMBED:{FSP_UPD_HEADER:FspUpdHeader:END}
  gQemuFspPkgTokenSpaceGuid.Reserved                    | * | 0x17 | {0x00}

  # !HDR COMMENT:{FSPT_ARCH_UPD:FSPT_ARCH_UPD}
  # !HDR EMBED:{FSPT_ARCH_UPD:FsptArchUpd:START}
  gQemuFspPkgTokenSpaceGuid.Revision                    | * | 0x01 | 0x01
  gQemuFspPkgTokenSpaceGuid.Reserved                    | * | 0x03 | {0x00}
  gQemuFspPkgTokenSpaceGuid.Length                      | * | 0x04 | 0x00000020
  gQemuFspPkgTokenSpaceGuid.FspDebugHandler             | * | 0x04 | 0x00000000
  # !HDR EMBED:{FSPT_ARCH_UPD:FsptArchUpd:END}
  gQemuFspPkgTokenSpaceGuid.Reserved1                   | * | 0x14 | {0x00}

  # !HDR COMMENT:{FSPT_COMMON_UPD:Fsp T Common UPD}
  # !HDR EMBED:{FSPT_COMMON_UPD:FsptCommonUpd:START}
  gQemuFspPkgTokenSpaceGuid.Revision                    | * | 0x01 | 0x01
  gQemuFspPkgTokenSpaceGuid.Reserved                    | * | 0x03 | {0x00}

  # Base address of the microcode region.
  gQemuFspPkgTokenSpaceGuid.MicrocodeRegionBase         | * | 0x04 | 0x00000000

  # Length of the microcode region.
  gQemuFspPkgTokenSpaceGuid.MicrocodeRegionLength       | * | 0x04 | 0x00000000

  # Base address of the cacheable flash region.
  gQemuFspPkgTokenSpaceGuid.CodeRegionBase              | * | 0x04 | 0x00000000

  # Length of the cacheable flash region.
  gQemuFspPkgTokenSpaceGuid.CodeRegionLength            | * | 0x04 | 0x00000000

  # !HDR EMBED:{FSPT_COMMON_UPD:FsptCommonUpd:END}
  gQemuFspPkgTokenSpaceGuid.Reserved1                   | * | 0x0C | {0x00}

  # !HDR COMMENT:{FSP_T_CONFIG:Fsp T Configuration}
  # !HDR EMBED:{FSP_T_CONFIG:FsptConfig:START}
  # !BSF PAGE:{TMP}
  # !BSF NAME:{Chicken bytes to test Hex config}
  # !BSF TYPE:{EditNum, HEX, (0x00000000,0xFFFFFFFF)}
  # !BSF HELP:{This option shows how to present option for 4 bytes data}
  gQemuFspPkgTokenSpaceGuid.ChickenBytes                | * | 0x04 | 0x00000000

  # !HDR EMBED:{FSP_T_CONFIG:FsptConfig:END}
  gQemuFspPkgTokenSpaceGuid.ReservedFsptUpd1            | * | 0x1C | {0x00}

  # Note please keep "UpdTerminator" at the end of each UPD region.
  # The tool will use this field to determine the actual end of the UPD data
  # structure.
  gQemuFspPkgTokenSpaceGuid.UpdTerminator               | * | 0x02 | 0x55AA

  ################################################################################
  #
  # UPDs consumed in FspMemoryInit Api
  #
  ################################################################################
  # !BSF FIND:{QEMUPD_M}
  # !HDR COMMENT:{FSP_UPD_HEADER:FSP UPD Header}
  # !HDR EMBED:{FSP_UPD_HEADER:FspUpdHeader:START}
  # FspmUpdSignature: {QEMUPD_M}
  gQemuFspPkgTokenSpaceGuid.Signature                   | * | 0x08 | 0x4D5F4450554D4551
  # !BSF NAME:{FspmUpdRevision}
  gQemuFspPkgTokenSpaceGuid.Revision                    | * | 0x01 | 0x01
  # !HDR EMBED:{FSP_UPD_HEADER:FspUpdHeader:END}
  gQemuFspPkgTokenSpaceGuid.Reserved                    | * | 0x17 | {0x00}

  # !HDR COMMENT:{FSPM_ARCH_UPD:Fsp M Architectural UPD}
  # !HDR EMBED:{FSPM_ARCH_UPD:FspmArchUpd:START}

  gQemuFspPkgTokenSpaceGuid.Revision                    | * | 0x01 | 0x01

  gQemuFspPkgTokenSpaceGuid.Reserved                    | * | 0x03 | {0x00}

  # !HDR STRUCT:{VOID*}
  gQemuFspPkgTokenSpaceGuid.NvsBufferPtr                | * | 0x04 | 0x00000000

  # !HDR STRUCT:{VOID*}
  # !BSF NAME:{StackBase}
  # !BSF HELP:{Stack base for FSP use. Default: 0xFEF16000}
  gQemuFspPkgTokenSpaceGuid.StackBase                   | * | 0x04 | $(CAR_BLD_REGION_SIZE)

  # !BSF NAME:{StackSize}
  # !BSF HELP:{To pass the stack size for FSP use. Bootloader can programmatically get the FSP requested StackSize by using the defaults in the FSP-M component. This is the minimum stack size expected by this revision of FSP. Default: 0x2A000}
  gQemuFspPkgTokenSpaceGuid.StackSize                   | * | 0x04 | $(CAR_FSP_REGION_SIZE)

  # !BSF NAME:{BootLoaderTolumSize}
  # !BSF HELP:{To pass Bootloader Tolum size.}
  gQemuFspPkgTokenSpaceGuid.BootLoaderTolumSize         | * | 0x04 | 0x00000000

  # !BSF NAME:{Bootmode}
  # !BSF HELP:{To maintain Bootmode details.}
  gPlatformFspPkgTokenSpaceGuid.Bootmode                   | * | 0x04 | 0x00000000

  # !HDR EMBED:{FSPM_ARCH_UPD:FspmArchUpd:END}
  gQemuFspPkgTokenSpaceGuid.Reserved1                   | * | 0x08 | {0x00}

  # !HDR COMMENT:{FSP_M_CONFIG:Fsp M Configuration}
  # !HDR EMBED:{FSP_M_CONFIG:FspmConfig:START}
  # !BSF PAGE:{MEM}
  # !BSF NAME:{Debug Serial Port Base address}
  # !BSF TYPE:{EditNum, HEX, (0x00000000,0xFFFFFFFF)}
  # !BSF HELP:{Debug serial port base address. This option will be used only when the 'Serial Port Debug Device'}
  # !BSF HELP:{+ option is set to 'External Device'. 0x00000000(Default).}
  gQemuFspPkgTokenSpaceGuid.SerialDebugPortAddress      | * | 0x04 | 0x00000000

  # !BSF NAME:{Debug Serial Port Type} TYPE:{Combo}
  # !BSF OPTION:{0:NONE, 1:I/O, 2:MMIO}
  # !BSF HELP:{16550 compatible debug serial port resource type. NONE means no serial port support. 0x02:MMIO(Default).}
  gQemuFspPkgTokenSpaceGuid.SerialDebugPortType         | * | 0x01 | 0x02

  # !BSF NAME:{Serial Port Debug Device} TYPE:{Combo}
  # !BSF OPTION:{0:SOC UART0, 1:SOC UART1, 2:SOC UART2, 3:External Device}
  # !BSF HELP:{Select active serial port device for debug.}
  # !BSF HELP:{+For SOC UART devices,'Debug Serial Port Base' options will be ignored. 0x02:SOC UART2(Default).}
  gQemuFspPkgTokenSpaceGuid.SerialDebugPortDevice       | * | 0x01 | 0x02

  # !BSF NAME:{Debug Serial Port Stride Size} TYPE:{Combo}
  # !BSF OPTION:{0:1, 2:4}
  # !BSF HELP:{Debug serial port register map stride size in bytes. 0x00:1, 0x02:4(Default).}
  gQemuFspPkgTokenSpaceGuid.SerialDebugPortStrideSize   | * | 0x01 | 0x02


  # !HDR EMBED:{FSP_M_CONFIG:FspmConfig:END}
  gQemuFspPkgTokenSpaceGuid.ReservedFspmUpd             | * | 0x04 | {0x00}


  # Note please keep "UpdTerminator" at the end of each UPD region.
  # The tool will use this field to determine the actual end of the UPD data
  # structure.
  gQemuFspPkgTokenSpaceGuid.UpdTerminator               | * | 0x02 | 0x55AA

  ################################################################################
  #
  # UPDs consumed in FspSiliconInit Api
  #
  ################################################################################
  # !BSF FIND:{QEMUPD_S}
  # !HDR COMMENT:{FSP_UPD_HEADER:FSP UPD Header}
  # !HDR EMBED:{FSP_UPD_HEADER:FspUpdHeader:START}
  # FspsUpdSignature: {QEMUPD_S}
  gQemuFspPkgTokenSpaceGuid.Signature                   | * | 0x08 | 0x535F4450554D4551
  # !BSF NAME:{FspsUpdRevision}
  gQemuFspPkgTokenSpaceGuid.Revision                    | * | 0x01 | 0x01
  # !HDR EMBED:{FSP_UPD_HEADER:FspUpdHeader:END}
  gQemuFspPkgTokenSpaceGuid.Reserved                    | * | 0x17 | {0x00}

  # !HDR COMMENT:{FSPS_ARCH_UPD:FSPS_ARCH_UPD}
  # !HDR EMBED:{FSPS_ARCH_UPD:FspsArchUpd:START}
  gQemuFspPkgTokenSpaceGuid.Revision                    | * | 0x01 | 0x01
  gQemuFspPkgTokenSpaceGuid.Reserved                    | * | 0x03 | {0x00}
  gQemuFspPkgTokenSpaceGuid.Length                      | * | 0x04 | 0x00000020
  gQemuFspPkgTokenSpaceGuid.FspEventHandler             | * | 0x04 | 0x00000000
  gQemuFspPkgTokenSpaceGuid.EnableMultiPhaseSiliconInit | * | 0x01 | 0x00
  # !HDR EMBED:{FSPS_ARCH_UPD:FspsArchUpd:END}
  gQemuFspPkgTokenSpaceGuid.Reserved1                   | * | 0x13 | {0x00}

  # !HDR COMMENT:{FSP_S_CONFIG:Fsp S Configuration}
  # !HDR EMBED:{FSP_S_CONFIG:FspsConfig:START}
  # !BSF PAGE:{SIL}

  # !BSF NAME:{BMP Logo Data Size}
  # !BSF TYPE:{Reserved}
  # !BSF HELP:{BMP logo data buffer size. 0x00000000(Default).}
  gQemuFspPkgTokenSpaceGuid.LogoSize                    | * | 0x04 | 0x00000000

  # !BSF NAME:{BMP Logo Data Pointer}
  # !BSF TYPE:{Reserved}
  # !BSF HELP:{BMP logo data pointer to a BMP format buffer. 0x00000000(Default).}
  gQemuFspPkgTokenSpaceGuid.LogoPtr                     | * | 0x04 | 0x00000000

  # !BSF NAME:{Graphics Configuration Data Pointer}
  # !BSF TYPE:{Reserved}
  # !BSF HELP:{Graphics configuration data used for initialization. 0x00000000(Default).}
  gQemuFspPkgTokenSpaceGuid.GraphicsConfigPtr           | * | 0x04 | 0x00000000

  # !BSF NAME:{PCI GFX Temporary MMIO Base}
  # !BSF TYPE:{EditNum, HEX, (0x80000000,0xDFFFFFFF)}
  # !BSF HELP:{PCI Temporary PCI GFX Base used before full PCI enumeration. 0x80000000(Default).}
  gQemuFspPkgTokenSpaceGuid.PciTempResourceBase         | * | 0x04 | 0x80000000

  # !HDR EMBED:{FSP_S_CONFIG:FspsConfig:END}
  gQemuFspPkgTokenSpaceGuid.ReservedFspsUpd             | * | 0x01 | 0x00

  # Note please keep "UpdTerminator" at the end of each UPD region.
  # The tool will use this field to determine the actual end of the UPD data
  # structure.
  gQemuFspPkgTokenSpaceGuid.UpdTerminator               | * | 0x02 | 0x55AA

###################################################################################################
#
# Components Section - list of the modules and components that will be processed by compilation
#                      tools and the EDK II tools to generate PE32/PE32+/Coff image files.
#
# Note: The EDK II DSC file is not used to specify how compiled binary images get placed
#       into firmware volume images. This section is just a list of modules to compile from
#       source into UEFI-compliant binaries.
#       It is the FDF file that contains information on combining binary files into firmware
#       volume images, whose concept is beyond UEFI and is described in PI specification.
#       Binary modules do not need to be listed in this section, as they should be
#       specified in the FDF file. For example: Shell binary (Shell_Full.efi), FAT binary (Fat.efi),
#       Logo (Logo.bmp), and etc.
#       There may also be modules listed in this section that are not required in the FDF file,
#       When a module listed here is excluded from FDF file, then UEFI-compliant binary will be
#       generated for it, but the binary will not be put into any firmware volume.
#
###################################################################################################
[Components.IA32]
  #
  # FSP Binary Components
  #
  $(FSP_PACKAGE)/FspHeader/FspHeader.inf

  #
  # SEC
  #
  IntelFsp2Pkg/FspSecCore/FspSecCoreT.inf {
    <LibraryClasses>
      FspSecPlatformLib|$(FSP_PACKAGE)/Library/PlatformSecLib/Vtf0PlatformSecTLib.inf
  }

[Components.$(FSP_ARCH)]
  IntelFsp2Pkg/FspSecCore/FspSecCoreV.inf {
    <LibraryClasses>
      FspSecPlatformLib|$(FSP_PACKAGE)/Library/PlatformSecLib/Vtf0PlatformSecVLib.inf
  }

  IntelFsp2Pkg/FspSecCore/FspSecCoreM.inf {
    <LibraryClasses>
      FspSecPlatformLib|$(FSP_PACKAGE)/Library/PlatformSecLib/Vtf0PlatformSecMLib.inf
  }

  IntelFsp2Pkg/FspSecCore/FspSecCoreS.inf {
    <LibraryClasses>
      FspSecPlatformLib|$(FSP_PACKAGE)/Library/PlatformSecLib/Vtf0PlatformSecSLib.inf
  }

  #
  # PEI Core
  #
  MdeModulePkg/Core/Pei/PeiMain.inf

  #
  # PCD
  #
  MdeModulePkg/Universal/PCD/Pei/Pcd.inf {
    <LibraryClasses>
      DebugLib|MdePkg/Library/BaseDebugLibNull/BaseDebugLibNull.inf
      PcdLib|MdePkg/Library/BasePcdLibNull/BasePcdLibNull.inf
  }

  $(FSP_PACKAGE)/FspvInit/FspvInit.inf
  $(FSP_PACKAGE)/FspmInit/FspmInit.inf
  $(FSP_PACKAGE)/FspsInit/FspsInit.inf
  $(FSP_PACKAGE)/QemuVideo/QemuVideo.inf
  MdeModulePkg/Core/DxeIplPeim/DxeIpl.inf {
    <LibraryClasses>
      DebugAgentLib|MdeModulePkg/Library/DebugAgentLibNull/DebugAgentLibNull.inf
      ResetSystemLib|MdeModulePkg/Library/BaseResetSystemLibNull/BaseResetSystemLibNull.inf
  }
  IntelFsp2Pkg/FspNotifyPhase/FspNotifyPhasePeim.inf

###################################################################################################
#
# BuildOptions Section - Define the module specific tool chain flags that should be used as
#                        the default flags for a module. These flags are appended to any
#                        standard flags that are defined by the build process. They can be
#                        applied for any modules or only those modules with the specific
#                        module style (EDK or EDKII) specified in [Components] section.
#
###################################################################################################
[BuildOptions]
# Append build options for EDK and EDKII drivers (= is Append, == is Replace)
  # Enable link-time optimization when building with GCC49
  *_GCC49_IA32_CC_FLAGS = -flto
  *_GCC49_IA32_DLINK_FLAGS = -flto
  *_GCC5_IA32_CC_FLAGS = -fno-pic
  *_GCC5_IA32_DLINK_FLAGS = -no-pie
  *_GCC5_IA32_ASLCC_FLAGS = -fno-pic
  *_GCC5_IA32_ASLDLINK_FLAGS = -no-pie
