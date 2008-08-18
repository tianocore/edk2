#/** @file
# This package build validate file is used to build validate EDK Shell source, 
# EDK Compatibility Package and the backward compatibility support of EDK II
# build tool.
#
# Copyright (c) 2008. Intel Corporation
#
#  All rights reserved. This program and the accompanying materials
#    are licensed and made available under the terms and conditions of the BSD License
#    which accompanies this distribution. The full text of the license may be found at
#    http://opensource.org/licenses/bsd-license.php
#
#    THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
#    WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
#
#**/

[Defines]
  PLATFORM_NAME                  = EdkShellPkg
  PLATFORM_GUID                  = 761BEE8B-58E3-4014-B8F5-0214A8DFA7EE
  PLATFORM_VERSION               = 1.04
  DSC_SPECIFICATION              = 0x00010005
  OUTPUT_DIRECTORY               = Build/EdkShellPkg
  SUPPORTED_ARCHITECTURES        = IA32|IPF|X64
  BUILD_TARGETS                  = DEBUG|RELEASE
  SKUID_IDENTIFIER               = DEFAULT

[Libraries]
  #
  # Libraries common to PEI and DXE
  #
  EdkCompatibilityPkg/Foundation/Efi/Guid/EfiGuidLib.inf
  EdkCompatibilityPkg/Foundation/Framework/Guid/EdkFrameworkGuidLib.inf
  EdkCompatibilityPkg/Foundation/Guid/EdkGuidLib.inf
  EdkCompatibilityPkg/Foundation/Library/EfiCommonLib/EfiCommonLib.inf
  EdkCompatibilityPkg/Foundation/Cpu/Pentium/CpuIA32Lib/CpuIA32Lib.inf
  EdkCompatibilityPkg/Foundation/Cpu/Itanium/CpuIA64Lib/CpuIA64Lib.inf
  EdkCompatibilityPkg/Foundation/Library/CompilerStub/CompilerStubLib.inf
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
  EdkShellPkg/Shell/Library/EfiShellLib.inf


[Components]
  #
  # Shell.inf & ShellFull.inf can not be included at once to avoid over
  #
  EdkShellPkg/Shell/Shell.inf
  EdkShellPkg/Shell/ShellFull.inf
  EdkShellPkg/Shell/attrib/attrib.inf
  EdkShellPkg/Shell/cls/cls.inf
  EdkShellPkg/Shell/comp/comp.inf
  EdkShellPkg/Shell/cp/cp.inf
  EdkShellPkg/Shell/date/date.inf
  EdkShellPkg/Shell/dblk/dblk.inf
  EdkShellPkg/Shell/devices/devices.inf
  EdkShellPkg/Shell/DeviceTree/devicetree.inf
  EdkShellPkg/Shell/dmem/dmem.inf
  EdkShellPkg/Shell/dmpstore/dmpstore.inf
  EdkShellPkg/Shell/drivers/drivers.inf
  EdkShellPkg/Shell/drvcfg/drvcfg.inf
  EdkShellPkg/Shell/drvdiag/drvdiag.inf
  EdkShellPkg/Shell/edit/edit.inf
  EdkShellPkg/Shell/EfiCompress/compress.inf
  EdkShellPkg/Shell/EfiDecompress/Decompress.inf
  EdkShellPkg/Shell/err/err.inf
  EdkShellPkg/Shell/guid/guid.inf
  EdkShellPkg/Shell/hexedit/hexedit.inf
  EdkShellPkg/Shell/IfConfig/IfConfig.inf
  EdkShellPkg/Shell/IpConfig/IpConfig.inf
  EdkShellPkg/Shell/load/load.inf
  EdkShellPkg/Shell/LoadPciRom/LoadPciRom.inf
  EdkShellPkg/Shell/ls/ls.inf
  EdkShellPkg/Shell/mem/mem.inf
  EdkShellPkg/Shell/memmap/memmap.inf
  EdkShellPkg/Shell/mkdir/mkdir.inf
  EdkShellPkg/Shell/mm/mm.inf
  EdkShellPkg/Shell/mode/mode.inf
  EdkShellPkg/Shell/mount/mount.inf
  EdkShellPkg/Shell/mv/mv.inf
  EdkShellPkg/Shell/newshell/nshell.inf
  EdkShellPkg/Shell/openinfo/openinfo.inf
  EdkShellPkg/Shell/pci/pci.inf
  EdkShellPkg/Shell/Ping/Ping.inf
  EdkShellPkg/Shell/reset/reset.inf
  EdkShellPkg/Shell/rm/rm.inf
  EdkShellPkg/Shell/sermode/sermode.inf
  EdkShellPkg/Shell/SmbiosView/Smbiosview.inf
  EdkShellPkg/Shell/stall/stall.inf
  EdkShellPkg/Shell/TelnetMgmt/TelnetMgmt.inf
  EdkShellPkg/Shell/time/time.inf
  EdkShellPkg/Shell/touch/touch.inf
  EdkShellPkg/Shell/type/type.inf
  EdkShellPkg/Shell/tzone/timezone.inf
  EdkShellPkg/Shell/unload/unload.inf
  EdkShellPkg/Shell/ver/Ver.inf
  EdkShellPkg/Shell/vol/Vol.inf

[BuildOptions]
  *_*_IA32_CC_FLAGS    = /D EFI_SPECIFICATION_VERSION=0x0002000A /D PI_SPECIFICATION_VERSION=0x00010000 /D TIANO_RELEASE_VERSION=0x00080006 /D EFI32
  *_*_IA32_ASM_FLAGS   = /DEFI32
  *_*_IA32_VFRPP_FLAGS = /D EFI_SPECIFICATION_VERSION=0x0002000A /D PI_SPECIFICATION_VERSION=0x00010000 /D TIANO_RELEASE_VERSION=0x00080006 /D EFI32
  *_*_IA32_APP_FLAGS   = /D EFI_SPECIFICATION_VERSION=0x0002000A /D PI_SPECIFICATION_VERSION=0x00010000 /D TIANO_RELEASE_VERSION=0x00080006 /D EFI32
  *_*_IA32_PP_FLAGS    = /D EFI_SPECIFICATION_VERSION=0x0002000A /D PI_SPECIFICATION_VERSION=0x00010000 /D TIANO_RELEASE_VERSION=0x00080006 /D EFI32

  *_*_X64_CC_FLAGS    = /D EFI_SPECIFICATION_VERSION=0x0002000A /D PI_SPECIFICATION_VERSION=0x00010000 /D TIANO_RELEASE_VERSION=0x00080006 /D EFIX64
  *_*_X64_ASM_FLAGS   = /DEFIX64
  *_*_X64_VFRPP_FLAGS = /D EFI_SPECIFICATION_VERSION=0x0002000A /D PI_SPECIFICATION_VERSION=0x00010000 /D TIANO_RELEASE_VERSION=0x00080006 /D EFIX64
  *_*_X64_APP_FLAGS   = /D EFI_SPECIFICATION_VERSION=0x0002000A /D PI_SPECIFICATION_VERSION=0x00010000 /D TIANO_RELEASE_VERSION=0x00080006 /D EFIX64
  *_*_X64_PP_FLAGS    = /D EFI_SPECIFICATION_VERSION=0x0002000A /D PI_SPECIFICATION_VERSION=0x00010000 /D TIANO_RELEASE_VERSION=0x00080006 /D EFIX64

  *_*_IPF_CC_FLAGS    = /D EFI_SPECIFICATION_VERSION=0x0002000A /D PI_SPECIFICATION_VERSION=0x00010000 /D TIANO_RELEASE_VERSION=0x00080006 /D EFI64
  *_*_IPF_ASM_FLAGS   = 
  *_*_IPF_VFRPP_FLAGS = /D EFI_SPECIFICATION_VERSION=0x0002000A /D PI_SPECIFICATION_VERSION=0x00010000 /D TIANO_RELEASE_VERSION=0x00080006 /D EFI64
  *_*_IPF_APP_FLAGS   = /D EFI_SPECIFICATION_VERSION=0x0002000A /D PI_SPECIFICATION_VERSION=0x00010000 /D TIANO_RELEASE_VERSION=0x00080006 /D EFI64
  *_*_IPF_PP_FLAGS    = /D EFI_SPECIFICATION_VERSION=0x0002000A /D PI_SPECIFICATION_VERSION=0x00010000 /D TIANO_RELEASE_VERSION=0x00080006 /D EFI64

