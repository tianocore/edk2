#/** @file
# This package build validate file is used to generate the shell binaries in this package.
# It depends on EdkCompatibilityPkg, Edk Shell source packge & BaseTools package. 
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
  EdkCompatibilityPkg/Shell/Library/EfiShellLib.inf


[Components]
  #
  # Shell.inf & ShellFull.inf can not be included at once to avoid over
  #
  EdkCompatibilityPkg/Shell/Shell.inf
  EdkCompatibilityPkg/Shell/ShellFull.inf
  EdkCompatibilityPkg/Shell/attrib/attrib.inf
  EdkCompatibilityPkg/Shell/cls/cls.inf
  EdkCompatibilityPkg/Shell/comp/comp.inf
  EdkCompatibilityPkg/Shell/cp/cp.inf
  EdkCompatibilityPkg/Shell/date/date.inf
  EdkCompatibilityPkg/Shell/dblk/dblk.inf
  EdkCompatibilityPkg/Shell/devices/devices.inf
  EdkCompatibilityPkg/Shell/DeviceTree/devicetree.inf
  EdkCompatibilityPkg/Shell/dmem/dmem.inf
  EdkCompatibilityPkg/Shell/dmpstore/dmpstore.inf
  EdkCompatibilityPkg/Shell/drivers/drivers.inf
  EdkCompatibilityPkg/Shell/drvcfg/drvcfg.inf
  EdkCompatibilityPkg/Shell/drvdiag/drvdiag.inf
  EdkCompatibilityPkg/Shell/edit/edit.inf
  EdkCompatibilityPkg/Shell/EfiCompress/compress.inf
  EdkCompatibilityPkg/Shell/EfiDecompress/Decompress.inf
  EdkCompatibilityPkg/Shell/err/err.inf
  EdkCompatibilityPkg/Shell/guid/guid.inf
  EdkCompatibilityPkg/Shell/hexedit/hexedit.inf
  EdkCompatibilityPkg/Shell/IfConfig/IfConfig.inf
  EdkCompatibilityPkg/Shell/IpConfig/IpConfig.inf
  EdkCompatibilityPkg/Shell/load/load.inf
  EdkCompatibilityPkg/Shell/LoadPciRom/LoadPciRom.inf
  EdkCompatibilityPkg/Shell/ls/ls.inf
  EdkCompatibilityPkg/Shell/mem/mem.inf
  EdkCompatibilityPkg/Shell/memmap/memmap.inf
  EdkCompatibilityPkg/Shell/mkdir/mkdir.inf
  EdkCompatibilityPkg/Shell/mm/mm.inf
  EdkCompatibilityPkg/Shell/mode/mode.inf
  EdkCompatibilityPkg/Shell/mount/mount.inf
  EdkCompatibilityPkg/Shell/mv/mv.inf
  EdkCompatibilityPkg/Shell/newshell/nshell.inf
  EdkCompatibilityPkg/Shell/openinfo/openinfo.inf
  EdkCompatibilityPkg/Shell/pci/pci.inf
  EdkCompatibilityPkg/Shell/Ping/Ping.inf
  EdkCompatibilityPkg/Shell/reset/reset.inf
  EdkCompatibilityPkg/Shell/rm/rm.inf
  EdkCompatibilityPkg/Shell/sermode/sermode.inf
  EdkCompatibilityPkg/Shell/SmbiosView/Smbiosview.inf
  EdkCompatibilityPkg/Shell/stall/stall.inf
  EdkCompatibilityPkg/Shell/TelnetMgmt/TelnetMgmt.inf
  EdkCompatibilityPkg/Shell/time/time.inf
  EdkCompatibilityPkg/Shell/touch/touch.inf
  EdkCompatibilityPkg/Shell/type/type.inf
  EdkCompatibilityPkg/Shell/tzone/timezone.inf
  EdkCompatibilityPkg/Shell/unload/unload.inf
  EdkCompatibilityPkg/Shell/ver/Ver.inf
  EdkCompatibilityPkg/Shell/vol/Vol.inf

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

