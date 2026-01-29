## @file
#  Cryptographic Library Package for UEFI Security Implementation.
#  DSC include file with Feature Flag PCD settings.
#
#  Copyright (c) 2025, Intel Corporation. All rights reserved.<BR>
#  SPDX-License-Identifier: BSD-2-Clause-Patent
#
##

[PcdsFeatureFlag.IA32, PcdsFeatureFlag.X64]
#
# Use Openssl NASM assembly source files that assume Windows calling convention
# if the family is MSFT or the toolchain is CLANGPDB for Windows or Linux.
# Also use Openssl NASM assembly source files that assume Windows calling
# convention if host-based unit tests are being build for Windows.
# Otherwise, the Openssl .S assembly source files that assume the Linux calling
# convention.
#
!if "$(FAMILY)" == "MSFT" || "$(TOOL_CHAIN_TAG)" == "CLANGPDB"
gEfiCryptoPkgTokenSpaceGuid.PcdOpensslLibAssemblySourceStyleNasm|TRUE
!endif
!if $(HOST_BASED_UNIT_TESTING_ENABLED) AND $(WIN_HOST_BUILD)
gEfiCryptoPkgTokenSpaceGuid.PcdOpensslLibAssemblySourceStyleNasm|TRUE
!endif

[PcdsFeatureFlag.AARCH64]
#
# Use the PE target assembly source files when building with the CLANGPDB
# toolchain.
# GCC and CLANGDWARF use the default PCD value of ELF target assembly source files.
#
!if "$(TOOL_CHAIN_TAG)" == "CLANGPDB"
gEfiCryptoPkgTokenSpaceGuid.PcdOpensslLibAssemblySourceStylePe|TRUE
!endif
