/** @file
  Google Test mocks for MockDtPlatformDtbLoaderLib

  Copyright (c) 2023 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
  Copyright (c) 2023, Intel Corporation. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <GoogleTest/Library/MockDtPlatformDtbLoaderLib.h>

MOCK_INTERFACE_DEFINITION (MockDtPlatformDtbLoaderLib);

MOCK_FUNCTION_DEFINITION (MockDtPlatformDtbLoaderLib, DtPlatformLoadDtb, 2, EFIAPI);
