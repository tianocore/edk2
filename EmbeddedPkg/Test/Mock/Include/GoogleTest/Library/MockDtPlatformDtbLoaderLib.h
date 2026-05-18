/** @file
  Google Test mocks for DtPlatformDtbLoaderLib

  Copyright (c) 2023 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
  Copyright (c) 2023, Intel Corporation. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#pragma once

#include <Library/GoogleTestLib.h>
#include <Library/FunctionMockLib.h>
extern "C" {
  #include <Pi/PiMultiPhase.h>
  #include <Uefi.h>
  #include <Library/DtPlatformDtbLoaderLib.h>
}

struct MockDtPlatformDtbLoaderLib {
  MOCK_INTERFACE_DECLARATION (MockDtPlatformDtbLoaderLib);

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    DtPlatformLoadDtb,
    (OUT   VOID   **Dtb,
     OUT   UINTN  *DtbSize)
    );
};
