/** @file
  This file declares EDKII Shadow Microcode PPI.

  Copyright (c) 2020, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __PPI_SHADOW_MICROCODE_H__
#define __PPI_SHADOW_MICROCODE_H__

#define EDKII_PEI_SHADOW_MICROCODE_PPI_GUID \
  { \
    0x430f6965, 0x9a69, 0x41c5, { 0x93, 0xed, 0x8b, 0xf0, 0x64, 0x35, 0xc1, 0xc6 } \
  }

typedef struct _EDKII_PEI_SHADOW_MICROCODE_PPI  EDKII_PEI_SHADOW_MICROCODE_PPI;

typedef struct {
  UINT32         ProcessorSignature;
  UINT8          PlatformId;
} EDKII_PEI_MICROCODE_CPU_ID;

/**
  Shadow microcode update patches to memory.

  The function is used for shadowing microcode update patches to a continuous memory.
  It shall allocate memory buffer and only shadow the microcode patches for those
  processors specified by MicrocodeCpuId array. The checksum verification may be
  skiped in this function so the caller must perform checksum verification before
  using the microcode patches in returned memory buffer.

  @param[in]  This                 The PPI instance pointer.
  @param[in]  CpuIdCount           Number of elements in MicrocodeCpuId array.
  @param[in]  MicrocodeCpuId       A pointer to an array of EDKII_PEI_MICROCODE_CPU_ID
                                   structures.
  @param[out] BufferSize           Pointer to receive the total size of Buffer.
  @param[out] Buffer               Pointer to receive address of allocated memory
                                   with microcode patches data in it.

  @retval EFI_SUCCESS              The microcode has been shadowed to memory.
  @retval EFI_OUT_OF_RESOURCES     The operation fails due to lack of resources.

**/
typedef
EFI_STATUS
(EFIAPI *EDKII_PEI_SHADOW_MICROCODE) (
  IN  EDKII_PEI_SHADOW_MICROCODE_PPI        *This,
  IN  UINTN                                 CpuIdCount,
  IN  EDKII_PEI_MICROCODE_CPU_ID            *MicrocodeCpuId,
  OUT UINTN                                 *BufferSize,
  OUT VOID                                  **Buffer
  );

///
/// This PPI is installed by some platform or chipset-specific PEIM that
/// abstracts handling microcode shadow support.
///
struct _EDKII_PEI_SHADOW_MICROCODE_PPI {
  EDKII_PEI_SHADOW_MICROCODE          ShadowMicrocode;
};

extern EFI_GUID gEdkiiPeiShadowMicrocodePpiGuid;

#endif

