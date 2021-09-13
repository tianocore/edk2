/** @file
  TDX specific definitions for EFI_TEE_MEASUREMENT_PROTOCOL

Copyright (c) 2020 - 2021, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef TD_PROTOCOL_H_
#define TD_PROTOCOL_H_

#define TDX_MR_INDEX_MRTD  0
#define TDX_MR_INDEX_RTMR0 1
#define TDX_MR_INDEX_RTMR1 2
#define TDX_MR_INDEX_RTMR2 3
#define TDX_MR_INDEX_RTMR3 4

/**
  In current version, we use below mapping:
    PCR0    -> MRTD  (Index 0)
    PCR1    -> RTMR0 (Index 1)
    PCR2~6  -> RTMR1 (Index 2)
    PCR7    -> RTMR0 (Index 1)
    PCR8~15 -> RTMR2 (Index 3)

typedef
EFI_STATUS
(EFIAPI * EFI_TEE_MAP_PCR_TO_MR_INDEX) (
  IN  EFI_TEE_MEASUREMENT_PROTOCOL   *This,
  IN  TCG_PCRINDEX                   PcrIndex,
  OUT EFI_TEE_MR_INDEX               *MrIndex
  );

**/

#endif
