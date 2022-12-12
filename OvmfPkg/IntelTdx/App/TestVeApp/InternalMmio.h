/** @file

  Copyright (c) 2022, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef INTERNAL_MMIO_H
#define INTERNAL_MMIO_H

#include <Uefi/UefiBaseType.h>

UINT64
TestMmioWrite_C6C7_1 (
  VOID
  );

UINT64
TestMmioWrite_C6C7_2 (
  VOID
  );

UINT64
TestMmioWrite_C6C7_4 (
  VOID
  );

UINT64
TestMmioWrite_C6C7_8 (
  VOID
  );

UINT64
TestMmioWrite_8889_1 (
  VOID
  );

UINT64
TestMmioWrite_8889_2 (
  VOID
  );

UINT64
TestMmioWrite_8889_4 (
  VOID
  );

UINT64
TestMmioWrite_8889_8 (
  VOID
  );

UINT64
TestMmioRead_8A8B_1 (
  VOID
  );

UINT64
TestMmioRead_8A8B_2 (
  VOID
  );

UINT64
TestMmioRead_8A8B_4 (
  VOID
  );

UINT64
TestMmioRead_8A8B_8 (
  VOID
  );

UINT64
TestMmioRead_MOVQ1 (
  VOID
  );

UINT64
TestMmioRead_MOVQ2 (
  VOID
  );

UINT64
TestMmioRead_B6B7_1 (
  VOID
  );

UINT64
TestMmioRead_B6B7_2 (
  VOID
  );

UINT64
TestMmioRead_B6B7_3 (
  VOID
  );

UINT64
TestMmioRead_B6B7_4 (
  VOID
  );

UINT64
TestMmioRead_B6B7_5 (
  VOID
  );

UINT64
TestMmioRead_BEBF_1 (
  VOID
  );

UINT64
TestMmioRead_BEBF_2 (
  VOID
  );

UINT64
TestMmioRead_BEBF_3 (
  VOID
  );

UINT64
TestMmioRead_BEBF_4 (
  VOID
  );

UINT64
TestMmioRead_BEBF_5 (
  VOID
  );

UINT64
TestMmioRead_BEBF_6 (
  VOID
  );

UINT64
TestMmioRead_BEBF_7 (
  VOID
  );

UINT64
TestMmioRead_BEBF_8 (
  VOID
  );

UINT64
TestMmioRead (
  UINT64 Address,
  UINT32 MmioSize,
  UINT64 *Val
  );

UINT64
TestMmioWrite (
  UINT64 Address,
  UINT32 MmioSize,
  UINT64 *Val
  );


#endif
