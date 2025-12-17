## @file
#  Cryptographic Library Package for UEFI Security Implementation.
#  DSC include file with Feature Flag PCD settings.
#
#  Copyright (c) 2025, Intel Corporation. All rights reserved.<BR>
#  SPDX-License-Identifier: BSD-2-Clause-Patent
#
##

[PcdsFeatureFlag.IA32,PcdsFeatureFlag.X64]
!if "$(FAMILY)" == "MSFT" || "$(TOOL_CHAIN_TAG)" == "CLANGPDB"
gEfiCryptoPkgTokenSpaceGuid.PcdOpensslLibAssemblySourceStyleNasm|TRUE
!endif
