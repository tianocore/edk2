## @file
# This package build validate file is used to build validate EDK Shell source, 
# EDK Compatibility Package and the backward compatibility support of EDK II
# build tool.
#
# Copyright (c) 2008 - 2011, Intel Corporation. All rights reserved.<BR>
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

[Defines]
  PLATFORM_NAME                  = EdkShellPkg
  PLATFORM_GUID                  = 761BEE8B-58E3-4014-B8F5-0214A8DFA7EE
  PLATFORM_VERSION               = 1.04
  DSC_SPECIFICATION              = 0x00010005
  OUTPUT_DIRECTORY               = Build/EdkShellPkg
  SUPPORTED_ARCHITECTURES        = IA32|IPF|X64|ARM|AARCH64
  BUILD_TARGETS                  = DEBUG|RELEASE
  SKUID_IDENTIFIER               = DEFAULT
# 
# Change the macro to the directory containing the source code from EDK Shell Project.  
# This is a workspace relative directory
#
# DEFINE EDK_SHELL_DIR           = EdkShellPkg/Shell  # when "Shell" directory is under $(WORKSPACE)/EdkShellPkg 
#
DEFINE EDK_SHELL_DIR             = Shell  # when "Shell" directory is directly under $(WORKSPACE) 

DEFINE MSFT_MACRO                = /D EFI_SPECIFICATION_VERSION=0x0002000A /D PI_SPECIFICATION_VERSION=0x00009000 /D TIANO_RELEASE_VERSION=0x00080006 /D PCD_EDKII_GLUE_PciExpressBaseAddress=0xE0000000 /D EFI_DEBUG
DEFINE INTEL_MACRO               = /D EFI_SPECIFICATION_VERSION=0x0002000A /D PI_SPECIFICATION_VERSION=0x00009000 /D TIANO_RELEASE_VERSION=0x00080006 /D PCD_EDKII_GLUE_PciExpressBaseAddress=0xE0000000 /D EFI_DEBUG
DEFINE GCC_MACRO                 = -DEFI_SPECIFICATION_VERSION=0x0002000A -DPI_SPECIFICATION_VERSION=0x00009000 -DTIANO_RELEASE_VERSION=0x00080006 -DPCD_EDKII_GLUE_PciExpressBaseAddress=0xE0000000 -DEFI_DEBUG -DSTRING_ARRAY_NAME=$(BASE_NAME)Strings -DSTRING_DEFINES_FILE=\"$(BASE_NAME)StrDefs.h\" 


[Libraries]
  #
  # Libraries common to PEI and DXE
  #
  EdkCompatibilityPkg/Foundation/Efi/Guid/EfiGuidLib.inf
  EdkCompatibilityPkg/Foundation/Framework/Guid/EdkFrameworkGuidLib.inf
  EdkCompatibilityPkg/Foundation/Guid/EdkGuidLib.inf
  EdkCompatibilityPkg/Foundation/Library/EfiCommonLib/EfiCommonLib_Edk2.inf
  EdkCompatibilityPkg/Foundation/Library/CustomizedDecompress/CustomizedDecompress.inf
  EdkCompatibilityPkg/Foundation/Library/Dxe/Hob/HobLib.inf
  #
  # PEI libraries
  #
  EdkCompatibilityPkg/Foundation/Framework/Ppi/EdkFrameworkPpiLib.inf
  EdkCompatibilityPkg/Foundation/Ppi/EdkPpiLib.inf
  EdkCompatibilityPkg/Foundation/Library/Pei/PeiLib/PeiLib.inf
  EdkCompatibilityPkg/Foundation/Library/Pei/Hob/PeiHobLib.inf
  #
  # DXE libraries
  #
  EdkCompatibilityPkg/Foundation/Core/Dxe/ArchProtocol/ArchProtocolLib.inf
  EdkCompatibilityPkg/Foundation/Efi/Protocol/EfiProtocolLib.inf
  EdkCompatibilityPkg/Foundation/Framework/Protocol/EdkFrameworkProtocolLib.inf
  EdkCompatibilityPkg/Foundation/Protocol/EdkProtocolLib.inf
  EdkCompatibilityPkg/Foundation/Library/Dxe/EfiDriverLib/EfiDriverLib.inf
  EdkCompatibilityPkg/Foundation/Library/RuntimeDxe/EfiRuntimeLib/EfiRuntimeLib.inf
  EdkCompatibilityPkg/Foundation/Library/Dxe/Graphics/Graphics.inf
  EdkCompatibilityPkg/Foundation/Library/Dxe/EfiIfrSupportLib/EfiIfrSupportLib.inf
  EdkCompatibilityPkg/Foundation/Library/Dxe/UefiEfiIfrSupportLib/UefiEfiIfrSupportLib.inf   
  EdkCompatibilityPkg/Foundation/Library/Dxe/Print/PrintLib.inf
  EdkCompatibilityPkg/Foundation/Library/Dxe/EfiScriptLib/EfiScriptLib.inf
  EdkCompatibilityPkg/Foundation/Library/Dxe/EfiUiLib/EfiUiLib.inf
  #
  # Print/Graphics Library consume SetupBrowser Print Protocol
  #
  EdkCompatibilityPkg/Foundation/Library/Dxe/PrintLite/PrintLib.inf
  EdkCompatibilityPkg/Foundation/Library/Dxe/GraphicsLite/Graphics.inf
  #
  # Shell Library
  #
  $(EDK_SHELL_DIR)/Library/EfiShellLib.inf

[Libraries.IA32, Libraries.X64]
  EdkCompatibilityPkg/Foundation/Library/CompilerStub/CompilerStubLib.inf
  EdkCompatibilityPkg/Foundation/Cpu/Pentium/CpuIA32Lib/CpuIA32Lib_Edk2.inf

[Libraries.IPF]
  EdkCompatibilityPkg/Foundation/Cpu/Itanium/CpuIa64Lib/CpuIA64Lib.inf

[Libraries.ARM, Libraries.AARCH64]
  EdkCompatibilityPkg/Foundation/Library/CompilerStub/CompilerStubLib.inf
  ArmPkg/Library/CompilerIntrinsicsLib/CompilerIntrinsicsLib.inf

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

[Components]
  $(EDK_SHELL_DIR)/Shell.inf {
    <BuildOptions>
      #
      # Can not do this in nmake section of edk INF
      #
      GCC:*_*_*_CC_FLAGS     = -DEFI_MONOSHELL 
      GCC:*_*_*_VFRPP_FLAGS  = -DEFI_MONOSHELL 
      GCC:*_*_*_APP_FLAGS    = -DEFI_MONOSHELL 
      GCC:*_*_*_PP_FLAGS     = -DEFI_MONOSHELL 

      RVCT:*_*_ARM_CC_FLAGS     = -DEFI_MONOSHELL 
      RVCT:*_*_ARM_VFRPP_FLAGS  = -DEFI_MONOSHELL
      RVCT:*_*_ARM_APP_FLAGS    = -DEFI_MONOSHELL 
      RVCT:*_*_ARM_PP_FLAGS     = -DEFI_MONOSHELL 
  }
  
  $(EDK_SHELL_DIR)/ShellFull.inf {
    <BuildOptions>
      GCC:*_*_*_CC_FLAGS     = -DEFI_MONOSHELL -DEFI_FULLSHELL 
      GCC:*_*_*_VFRPP_FLAGS  = -DEFI_MONOSHELL -DEFI_FULLSHELL
      GCC:*_*_*_APP_FLAGS    = -DEFI_MONOSHELL -DEFI_FULLSHELL
      GCC:*_*_*_PP_FLAGS     = -DEFI_MONOSHELL -DEFI_FULLSHELL

      RVCT:*_*_ARM_CC_FLAGS     = -DEFI_MONOSHELL -DEFI_FULLSHELL 
      RVCT:*_*_ARM_VFRPP_FLAGS  = -DEFI_MONOSHELL -DEFI_FULLSHELL
      RVCT:*_*_ARM_APP_FLAGS    = -DEFI_MONOSHELL -DEFI_FULLSHELL
      RVCT:*_*_ARM_PP_FLAGS     = -DEFI_MONOSHELL -DEFI_FULLSHELL
  }
  
  $(EDK_SHELL_DIR)/attrib/attrib.inf
  $(EDK_SHELL_DIR)/cls/cls.inf
  $(EDK_SHELL_DIR)/comp/comp.inf
  $(EDK_SHELL_DIR)/cp/cp.inf
  $(EDK_SHELL_DIR)/date/date.inf
  $(EDK_SHELL_DIR)/dblk/dblk.inf
  $(EDK_SHELL_DIR)/devices/devices.inf
  $(EDK_SHELL_DIR)/DeviceTree/devicetree.inf
  $(EDK_SHELL_DIR)/dmem/dmem.inf
  $(EDK_SHELL_DIR)/dmpstore/dmpstore.inf
  $(EDK_SHELL_DIR)/drivers/drivers.inf
  $(EDK_SHELL_DIR)/drvcfg/drvcfg.inf
  $(EDK_SHELL_DIR)/drvdiag/drvdiag.inf
  $(EDK_SHELL_DIR)/edit/edit.inf
  $(EDK_SHELL_DIR)/EfiCompress/compress.inf
  $(EDK_SHELL_DIR)/EfiDecompress/Decompress.inf
  $(EDK_SHELL_DIR)/err/err.inf
  $(EDK_SHELL_DIR)/guid/guid.inf
  $(EDK_SHELL_DIR)/hexedit/hexedit.inf
  $(EDK_SHELL_DIR)/IfConfig/IfConfig.inf
  $(EDK_SHELL_DIR)/IpConfig/IpConfig.inf
  $(EDK_SHELL_DIR)/load/load.inf
  $(EDK_SHELL_DIR)/LoadPciRom/LoadPciRom.inf
  $(EDK_SHELL_DIR)/ls/ls.inf
  $(EDK_SHELL_DIR)/mem/mem.inf
  $(EDK_SHELL_DIR)/memmap/memmap.inf
  $(EDK_SHELL_DIR)/mkdir/mkdir.inf
  $(EDK_SHELL_DIR)/mm/mm.inf
  $(EDK_SHELL_DIR)/mode/mode.inf
  $(EDK_SHELL_DIR)/mount/mount.inf
  $(EDK_SHELL_DIR)/mv/mv.inf
  $(EDK_SHELL_DIR)/newshell/nshell.inf
  $(EDK_SHELL_DIR)/openinfo/openinfo.inf
  $(EDK_SHELL_DIR)/pci/pci.inf
  $(EDK_SHELL_DIR)/Ping/Ping.inf
  $(EDK_SHELL_DIR)/reset/reset.inf
  $(EDK_SHELL_DIR)/rm/rm.inf
  $(EDK_SHELL_DIR)/sermode/sermode.inf
  $(EDK_SHELL_DIR)/SmbiosView/Smbiosview.inf
  $(EDK_SHELL_DIR)/stall/stall.inf
  $(EDK_SHELL_DIR)/TelnetMgmt/TelnetMgmt.inf
  $(EDK_SHELL_DIR)/time/time.inf
  $(EDK_SHELL_DIR)/touch/touch.inf
  $(EDK_SHELL_DIR)/type/type.inf
  $(EDK_SHELL_DIR)/tzone/timezone.inf
  $(EDK_SHELL_DIR)/unload/unload.inf
  $(EDK_SHELL_DIR)/ver/Ver.inf
  $(EDK_SHELL_DIR)/vol/Vol.inf

[BuildOptions.Common.EDK]
  MSFT:*_*_IA32_CC_FLAGS    = /D EFI_SPECIFICATION_VERSION=0x0002000A /D PI_SPECIFICATION_VERSION=0x00010000 /D TIANO_RELEASE_VERSION=0x00080006 /D EFI32
  MSFT:*_*_IA32_ASM_FLAGS   = /DEFI32
  MSFT:*_*_IA32_VFRPP_FLAGS = /D EFI_SPECIFICATION_VERSION=0x0002000A /D PI_SPECIFICATION_VERSION=0x00010000 /D TIANO_RELEASE_VERSION=0x00080006 /D EFI32
  MSFT:*_*_IA32_APP_FLAGS   = /D EFI_SPECIFICATION_VERSION=0x0002000A /D PI_SPECIFICATION_VERSION=0x00010000 /D TIANO_RELEASE_VERSION=0x00080006 /D EFI32
  MSFT:*_*_IA32_PP_FLAGS    = /D EFI_SPECIFICATION_VERSION=0x0002000A /D PI_SPECIFICATION_VERSION=0x00010000 /D TIANO_RELEASE_VERSION=0x00080006 /D EFI32

  MSFT:*_*_X64_CC_FLAGS    = /D EFI_SPECIFICATION_VERSION=0x0002000A /D PI_SPECIFICATION_VERSION=0x00010000 /D TIANO_RELEASE_VERSION=0x00080006 /D EFIX64
  MSFT:*_*_X64_ASM_FLAGS   = /DEFIX64
  MSFT:*_*_X64_VFRPP_FLAGS = /D EFI_SPECIFICATION_VERSION=0x0002000A /D PI_SPECIFICATION_VERSION=0x00010000 /D TIANO_RELEASE_VERSION=0x00080006 /D EFIX64
  MSFT:*_*_X64_APP_FLAGS   = /D EFI_SPECIFICATION_VERSION=0x0002000A /D PI_SPECIFICATION_VERSION=0x00010000 /D TIANO_RELEASE_VERSION=0x00080006 /D EFIX64
  MSFT:*_*_X64_PP_FLAGS    = /D EFI_SPECIFICATION_VERSION=0x0002000A /D PI_SPECIFICATION_VERSION=0x00010000 /D TIANO_RELEASE_VERSION=0x00080006 /D EFIX64

  MSFT:*_*_IPF_CC_FLAGS    = /D EFI_SPECIFICATION_VERSION=0x0002000A /D PI_SPECIFICATION_VERSION=0x00010000 /D TIANO_RELEASE_VERSION=0x00080006 /D EFI64
  MSFT:*_*_IPF_ASM_FLAGS   = 
  MSFT:*_*_IPF_VFRPP_FLAGS = /D EFI_SPECIFICATION_VERSION=0x0002000A /D PI_SPECIFICATION_VERSION=0x00010000 /D TIANO_RELEASE_VERSION=0x00080006 /D EFI64
  MSFT:*_*_IPF_APP_FLAGS   = /D EFI_SPECIFICATION_VERSION=0x0002000A /D PI_SPECIFICATION_VERSION=0x00010000 /D TIANO_RELEASE_VERSION=0x00080006 /D EFI64
  MSFT:*_*_IPF_PP_FLAGS    = /D EFI_SPECIFICATION_VERSION=0x0002000A /D PI_SPECIFICATION_VERSION=0x00010000 /D TIANO_RELEASE_VERSION=0x00080006 /D EFI64

  MSFT:*_*_*_BUILD_FLAGS   = -s

  INTEL:*_*_IA32_CC_FLAGS    = /D EFI_SPECIFICATION_VERSION=0x0002000A /D PI_SPECIFICATION_VERSION=0x00010000 /D TIANO_RELEASE_VERSION=0x00080006 /D EFI32
  INTEL:*_*_IA32_ASM_FLAGS   = /DEFI32
  INTEL:*_*_IA32_VFRPP_FLAGS = /D EFI_SPECIFICATION_VERSION=0x0002000A /D PI_SPECIFICATION_VERSION=0x00010000 /D TIANO_RELEASE_VERSION=0x00080006 /D EFI32
  INTEL:*_*_IA32_APP_FLAGS   = /D EFI_SPECIFICATION_VERSION=0x0002000A /D PI_SPECIFICATION_VERSION=0x00010000 /D TIANO_RELEASE_VERSION=0x00080006 /D EFI32
  INTEL:*_*_IA32_PP_FLAGS    = /D EFI_SPECIFICATION_VERSION=0x0002000A /D PI_SPECIFICATION_VERSION=0x00010000 /D TIANO_RELEASE_VERSION=0x00080006 /D EFI32

  INTEL:*_*_X64_CC_FLAGS    = /D EFI_SPECIFICATION_VERSION=0x0002000A /D PI_SPECIFICATION_VERSION=0x00010000 /D TIANO_RELEASE_VERSION=0x00080006 /D EFIX64
  INTEL:*_*_X64_ASM_FLAGS   = /DEFIX64
  INTEL:*_*_X64_VFRPP_FLAGS = /D EFI_SPECIFICATION_VERSION=0x0002000A /D PI_SPECIFICATION_VERSION=0x00010000 /D TIANO_RELEASE_VERSION=0x00080006 /D EFIX64
  INTEL:*_*_X64_APP_FLAGS   = /D EFI_SPECIFICATION_VERSION=0x0002000A /D PI_SPECIFICATION_VERSION=0x00010000 /D TIANO_RELEASE_VERSION=0x00080006 /D EFIX64
  INTEL:*_*_X64_PP_FLAGS    = /D EFI_SPECIFICATION_VERSION=0x0002000A /D PI_SPECIFICATION_VERSION=0x00010000 /D TIANO_RELEASE_VERSION=0x00080006 /D EFIX64

  INTEL:*_*_IPF_CC_FLAGS    = /D EFI_SPECIFICATION_VERSION=0x0002000A /D PI_SPECIFICATION_VERSION=0x00010000 /D TIANO_RELEASE_VERSION=0x00080006 /D EFI64
  INTEL:*_*_IPF_ASM_FLAGS   = 
  INTEL:*_*_IPF_VFRPP_FLAGS = /D EFI_SPECIFICATION_VERSION=0x0002000A /D PI_SPECIFICATION_VERSION=0x00010000 /D TIANO_RELEASE_VERSION=0x00080006 /D EFI64
  INTEL:*_*_IPF_APP_FLAGS   = /D EFI_SPECIFICATION_VERSION=0x0002000A /D PI_SPECIFICATION_VERSION=0x00010000 /D TIANO_RELEASE_VERSION=0x00080006 /D EFI64
  INTEL:*_*_IPF_PP_FLAGS    = /D EFI_SPECIFICATION_VERSION=0x0002000A /D PI_SPECIFICATION_VERSION=0x00010000 /D TIANO_RELEASE_VERSION=0x00080006 /D EFI64

  INTEL:*_*_*_BUILD_FLAGS   = -s

  GCC:*_*_IA32_CC_FLAGS     = -DEFI32 $(GCC_MACRO)
  GCC:*_*_IA32_VFRPP_FLAGS  = -DEFI32 $(GCC_MACRO)
  GCC:*_*_IA32_APP_FLAGS    = -DEFI32 $(GCC_MACRO)
  GCC:*_*_IA32_PP_FLAGS     = -DEFI32 $(GCC_MACRO)

  GCC:*_*_X64_CC_FLAGS     = -DEFIX64 $(GCC_MACRO)
  GCC:*_*_X64_VFRPP_FLAGS  = -DEFIX64 $(GCC_MACRO)
  GCC:*_*_X64_APP_FLAGS    = -DEFIX64 $(GCC_MACRO)
  GCC:*_*_X64_PP_FLAGS     = -DEFIX64 $(GCC_MACRO)

  GCC:*_*_IPF_CC_FLAGS     = -DEFI64 $(GCC_MACRO)
  GCC:*_*_IPF_VFRPP_FLAGS  = -DEFI64 $(GCC_MACRO)
  GCC:*_*_IPF_APP_FLAGS    = -DEFI64 $(GCC_MACRO)
  GCC:*_*_IPF_PP_FLAGS     = -DEFI64 $(GCC_MACRO)
  
  GCC:*_*_ARM_CC_FLAGS     = -DEFIARM $(GCC_MACRO)
  GCC:*_*_ARM_VFRPP_FLAGS  = -DEFIARM $(GCC_MACRO)
  GCC:*_*_ARM_APP_FLAGS    = -DEFIARM $(GCC_MACRO)
  GCC:*_*_ARM_PP_FLAGS     = -DEFIARM $(GCC_MACRO)

  RVCT:*_*_ARM_CC_FLAGS     = -DEFIARM $(GCC_MACRO)
  RVCT:*_*_ARM_VFRPP_FLAGS  = -DEFIARM $(GCC_MACRO)
  RVCT:*_*_ARM_APP_FLAGS    = -DEFIARM $(GCC_MACRO)
  RVCT:*_*_ARM_PP_FLAGS     = -DEFIARM $(GCC_MACRO)

  GCC:*_*_AARCH64_CC_FLAGS     = -DEFIAARCH64 $(GCC_MACRO)
  GCC:*_*_AARCH64_VFRPP_FLAGS  = -DEFIAARCH64 $(GCC_MACRO)
  GCC:*_*_AARCH64_APP_FLAGS    = -DEFIAARCH64 $(GCC_MACRO)
  GCC:*_*_AARCH64_PP_FLAGS     = -DEFIAARCH64 $(GCC_MACRO)
