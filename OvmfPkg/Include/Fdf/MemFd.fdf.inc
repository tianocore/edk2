## @file
#  Open Virtual Machine Firmware: FDF include for MEMFD
#
#  Copyright (c) 2006 - 2019, Intel Corporation. All rights reserved.<BR>
#  (C) Copyright 2016 Hewlett Packard Enterprise Development LP<BR>
#
#  SPDX-License-Identifier: BSD-2-Clause-Patent
#
##

##########################################################################################
# MEMFD for OVMF, builds with confidential computing support

[FD.MEMFD]
BaseAddress   = $(MEMFD_BASE_ADDRESS)
Size          = 0xF80000
ErasePolarity = 1
BlockSize     = 0x10000
NumBlocks     = 0xF8

#
# initial page tables, created by the reset vector
#
0x000000|0x006000
gUefiOvmfPkgTokenSpaceGuid.PcdOvmfSecPageTablesBase|gUefiOvmfPkgTokenSpaceGuid.PcdOvmfSecPageTablesSize

#
# FIXME
#
0x006000|0x001000
gUefiOvmfPkgTokenSpaceGuid.PcdOvmfLockBoxStorageBase|gUefiOvmfPkgTokenSpaceGuid.PcdOvmfLockBoxStorageSize

#
# FIXME
#
0x007000|0x001000
gEfiMdePkgTokenSpaceGuid.PcdGuidedExtractHandlerTableAddress|gUefiOvmfPkgTokenSpaceGuid.PcdGuidedExtractHandlerTableSize

#
# SEV: page table, for 2M -> 4k split when mapping GHCB page
#
0x008000|0x001000
gUefiOvmfPkgTokenSpaceGuid.PcdOvmfSecGhcbPageTableBase|gUefiOvmfPkgTokenSpaceGuid.PcdOvmfSecGhcbPageTableSize

#
# SEV: GHCB page
# TDX: TD HOB page
#
0x009000|0x002000
gUefiOvmfPkgTokenSpaceGuid.PcdOvmfSecGhcbBase|gUefiOvmfPkgTokenSpaceGuid.PcdOvmfSecGhcbSize

#
# Work Area Page, both TDX and SEV store some confidential guest state there.
#
0x00B000|0x001000
gUefiOvmfPkgTokenSpaceGuid.PcdOvmfWorkAreaBase|gUefiOvmfPkgTokenSpaceGuid.PcdOvmfWorkAreaSize

#
# SEV: FIXME
# TDX: TD Mailbox at early boot.
#
0x00C000|0x001000
gUefiOvmfPkgTokenSpaceGuid.PcdOvmfSecGhcbBackupBase|gUefiOvmfPkgTokenSpaceGuid.PcdOvmfSecGhcbBackupSize

#
# SEV-SNP: secrets page
#
0x00D000|0x001000
gUefiOvmfPkgTokenSpaceGuid.PcdOvmfSnpSecretsBase|gUefiOvmfPkgTokenSpaceGuid.PcdOvmfSnpSecretsSize

#
# SEV-SNP: cpuid page
#
0x00E000|0x001000
gUefiOvmfPkgTokenSpaceGuid.PcdOvmfCpuidBase|gUefiOvmfPkgTokenSpaceGuid.PcdOvmfCpuidSize

#
# SEV-SNP: CAA page (for communication with SVSM)
#
0x00F000|0x001000
gUefiOvmfPkgTokenSpaceGuid.PcdOvmfSecSvsmCaaBase|gUefiOvmfPkgTokenSpaceGuid.PcdOvmfSecSvsmCaaSize

#
# SEV-SNP: pass secrets from host (qemu) to guest
#
0x010000|0x000C00
gUefiOvmfPkgTokenSpaceGuid.PcdSevLaunchSecretBase|gUefiOvmfPkgTokenSpaceGuid.PcdSevLaunchSecretSize

#
# SEV-SNP: pass hashes for kernel/initrd/cmdline from host (qemu) to guest
#
0x010C00|0x000400
gUefiOvmfPkgTokenSpaceGuid.PcdQemuHashTableBase|gUefiOvmfPkgTokenSpaceGuid.PcdQemuHashTableSize

#
# page table, for 2M -> 4k split when mapping APIC
#
0x011000|0x001000
gUefiOvmfPkgTokenSpaceGuid.PcdOvmfSecApicPageTableBase|gUefiOvmfPkgTokenSpaceGuid.PcdOvmfSecApicPageTableSize

#
# initial memory for SEC+PEI
#
0x012000|0x00E000
gUefiOvmfPkgTokenSpaceGuid.PcdOvmfSecPeiTempRamBase|gUefiOvmfPkgTokenSpaceGuid.PcdOvmfSecPeiTempRamSize

#
# PEI firmware volume is uncompressed to this area
#
0x020000|0x0D0000
gUefiOvmfPkgTokenSpaceGuid.PcdOvmfPeiMemFvBase|gUefiOvmfPkgTokenSpaceGuid.PcdOvmfPeiMemFvSize
FV = PEIFV

#
# memory debug log buffer for early boot phase
#
0x0F0000|0x10000
gUefiOvmfPkgTokenSpaceGuid.PcdOvmfEarlyMemDebugLogBase|gUefiOvmfPkgTokenSpaceGuid.PcdOvmfEarlyMemDebugLogSize

#
# DXE firmware volume is uncompressed to this area
#
0x100000|0xE80000
gUefiOvmfPkgTokenSpaceGuid.PcdOvmfDxeMemFvBase|gUefiOvmfPkgTokenSpaceGuid.PcdOvmfDxeMemFvSize
FV = DXEFV

##########################################################################################
# Set the SEV-ES specific work area PCDs (used for all forms of SEV since the
# the SEV STATUS MSR is now saved in the work area)
#
SET gUefiCpuPkgTokenSpaceGuid.PcdSevEsWorkAreaBase = $(MEMFD_BASE_ADDRESS) +  gUefiOvmfPkgTokenSpaceGuid.PcdOvmfWorkAreaBase + gUefiOvmfPkgTokenSpaceGuid.PcdOvmfConfidentialComputingWorkAreaHeader
SET gUefiCpuPkgTokenSpaceGuid.PcdSevEsWorkAreaSize = gUefiOvmfPkgTokenSpaceGuid.PcdOvmfWorkAreaSize - gUefiOvmfPkgTokenSpaceGuid.PcdOvmfConfidentialComputingWorkAreaHeader
