## @file
#  This FDF include file computes the end of the scratch buffer used in
#  DecompressMemFvs() [OvmfPkg/Sec/SecMain.c]. It is based on the decompressed
#  (ie. original) size of the LZMA-compressed section of the one FFS file in
#  the FVMAIN_COMPACT firmware volume.
#
#  Copyright (C) 2015, Red Hat, Inc.
#
#  SPDX-License-Identifier: BSD-2-Clause-Patent
##

# The GUID EE4E5898-3914-4259-9D6E-DC7BD79403CF means "LzmaCustomDecompress".
# The decompressed output will have the following structure (see the file
# "9E21FD93-9C72-4c15-8C4B-E77F1DB2D792SEC1.guided.dummy" in the
# Build/Ovmf*/*/FV/Ffs/9E21FD93-9C72-4c15-8C4B-E77F1DB2D792/ directory):
#
# Size                 Contents
# -------------------  --------------------------------------------------------
#                   4  EFI_COMMON_SECTION_HEADER, stating size 124 (0x7C) and
#                      type 0x19 (EFI_SECTION_RAW). The purpose of this section
#                      is to pad the start of PEIFV to 128 bytes.
#                 120  Zero bytes (padding).
#
#                   4  EFI_COMMON_SECTION_HEADER, stating size
#                      (PcdOvmfPeiMemFvSize + 4), and type 0x17
#                      (EFI_SECTION_FIRMWARE_VOLUME_IMAGE).
# PcdOvmfPeiMemFvSize  PEIFV. Note that the above sizes pad the offset of this
#                      object to 128 bytes. See also the "guided.dummy.txt"
#                      file in the same directory.
#
#                   4  EFI_COMMON_SECTION_HEADER, stating size 12 (0xC) and
#                      type 0x19 (EFI_SECTION_RAW). The purpose of this section
#                      is to pad the start of DXEFV to 16 bytes.
#                   8  Zero bytes (padding).
#
#                   4  EFI_COMMON_SECTION_HEADER, stating size
#                      (PcdOvmfDxeMemFvSize + 4), and type 0x17
#                      (EFI_SECTION_FIRMWARE_VOLUME_IMAGE).
# PcdOvmfDxeMemFvSize  DXEFV. Note that the above sizes pad the offset of this
#                      object to 16 bytes. See also the "guided.dummy.txt" file
#                      in the same directory.
#
# The total size after decompression is (128 + PcdOvmfPeiMemFvSize + 16 +
# PcdOvmfDxeMemFvSize).

DEFINE OUTPUT_SIZE = (128 + gUefiOvmfPkgTokenSpaceGuid.PcdOvmfPeiMemFvSize + 16 + gUefiOvmfPkgTokenSpaceGuid.PcdOvmfDxeMemFvSize)

# LzmaCustomDecompressLib uses a constant scratch buffer size of 64KB; see
# SCRATCH_BUFFER_REQUEST_SIZE in
# "MdeModulePkg/Library/LzmaCustomDecompressLib/LzmaDecompress.c".

DEFINE DECOMP_SCRATCH_SIZE = 0x00010000

# Note: when we use PcdOvmfDxeMemFvBase in this context, BaseTools have not yet
# offset it with MEMFD's base address. For that reason we have to do it manually.
#
# The calculation below mirrors DecompressMemFvs() [OvmfPkg/Sec/SecMain.c].

DEFINE OUTPUT_BASE                   = ($(MEMFD_BASE_ADDRESS) + gUefiOvmfPkgTokenSpaceGuid.PcdOvmfDxeMemFvBase + 0x00100000)
DEFINE DECOMP_SCRATCH_BASE_UNALIGNED = ($(OUTPUT_BASE) + $(OUTPUT_SIZE))
DEFINE DECOMP_SCRATCH_BASE_ALIGNMENT = 0x000FFFFF
DEFINE DECOMP_SCRATCH_BASE_MASK      = 0xFFF00000
DEFINE DECOMP_SCRATCH_BASE           = (($(DECOMP_SCRATCH_BASE_UNALIGNED) + $(DECOMP_SCRATCH_BASE_ALIGNMENT)) & $(DECOMP_SCRATCH_BASE_MASK))

SET gUefiOvmfPkgTokenSpaceGuid.PcdOvmfDecompressionScratchEnd = $(DECOMP_SCRATCH_BASE) + $(DECOMP_SCRATCH_SIZE)
