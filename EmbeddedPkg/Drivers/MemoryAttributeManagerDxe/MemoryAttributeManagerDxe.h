/** @file

  Copyright (c) 2023-2024, Mario Bălănică <mariobalanica02@gmail.com>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef MEMORY_ATTRIBUTE_MANAGER_DXE_H_
#define MEMORY_ATTRIBUTE_MANAGER_DXE_H_

#include <Guid/MemoryAttributeManagerFormSet.h>

#define PROTOCOL_ENABLED_DEFAULT  FixedPcdGetBool(PcdMemoryAttributeEnabledDefault)

#define MEMORY_ATTRIBUTE_MANAGER_DATA_VAR_NAME  L"MemoryAttributeManagerData"

typedef struct {
  BOOLEAN    Enabled;
} MEMORY_ATTRIBUTE_MANAGER_VARSTORE_DATA;

#endif // __MEMORY_ATTRIBUTE_MANAGER_DXE_H__
