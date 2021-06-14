/** @file
  ArmLibPrivate.h

  Copyright (c) 2020, NUVIA Inc. All rights reserved.<BR>
  Copyright (c) 2008 - 2009, Apple Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef ARM_LIB_PRIVATE_H_
#define ARM_LIB_PRIVATE_H_

#define CACHE_SIZE_4_KB             (3UL)
#define CACHE_SIZE_8_KB             (4UL)
#define CACHE_SIZE_16_KB            (5UL)
#define CACHE_SIZE_32_KB            (6UL)
#define CACHE_SIZE_64_KB            (7UL)
#define CACHE_SIZE_128_KB           (8UL)

#define CACHE_ASSOCIATIVITY_DIRECT  (0UL)
#define CACHE_ASSOCIATIVITY_4_WAY   (2UL)
#define CACHE_ASSOCIATIVITY_8_WAY   (3UL)

#define CACHE_PRESENT               (0UL)
#define CACHE_NOT_PRESENT           (1UL)

#define CACHE_LINE_LENGTH_32_BYTES  (2UL)

#define SIZE_FIELD_TO_CACHE_SIZE(x)           (((x) >> 6) & 0x0F)
#define SIZE_FIELD_TO_CACHE_ASSOCIATIVITY(x)  (((x) >> 3) & 0x07)
#define SIZE_FIELD_TO_CACHE_PRESENCE(x)       (((x) >> 2) & 0x01)
#define SIZE_FIELD_TO_CACHE_LINE_LENGTH(x)    (((x) >> 0) & 0x03)

#define DATA_CACHE_SIZE_FIELD(x)              (((x) >> 12) & 0x0FFF)
#define INSTRUCTION_CACHE_SIZE_FIELD(x)       (((x) >>  0) & 0x0FFF)

#define DATA_CACHE_SIZE(x)                    (SIZE_FIELD_TO_CACHE_SIZE(DATA_CACHE_SIZE_FIELD(x)))
#define DATA_CACHE_ASSOCIATIVITY(x)           (SIZE_FIELD_TO_CACHE_ASSOCIATIVITY(DATA_CACHE_SIZE_FIELD(x)))
#define DATA_CACHE_PRESENT(x)                 (SIZE_FIELD_TO_CACHE_PRESENCE(DATA_CACHE_SIZE_FIELD(x)))
#define DATA_CACHE_LINE_LENGTH(x)             (SIZE_FIELD_TO_CACHE_LINE_LENGTH(DATA_CACHE_SIZE_FIELD(x)))

#define INSTRUCTION_CACHE_SIZE(x)             (SIZE_FIELD_TO_CACHE_SIZE(INSTRUCTION_CACHE_SIZE_FIELD(x)))
#define INSTRUCTION_CACHE_ASSOCIATIVITY(x)    (SIZE_FIELD_TO_CACHE_ASSOCIATIVITY(INSTRUCTION_CACHE_SIZE_FIELD(x)))
#define INSTRUCTION_CACHE_PRESENT(x)          (SIZE_FIELD_TO_CACHE_PRESENCE(INSTRUCTION_CACHE_SIZE_FIELD(x)))
#define INSTRUCTION_CACHE_LINE_LENGTH(x)      (SIZE_FIELD_TO_CACHE_LINE_LENGTH(INSTRUCTION_CACHE_SIZE_FIELD(x)))

#define CACHE_TYPE(x)                         (((x) >> 25) & 0x0F)
#define CACHE_TYPE_WRITE_BACK                 (0x0EUL)

#define CACHE_ARCHITECTURE(x)                 (((x) >> 24) & 0x01)
#define CACHE_ARCHITECTURE_UNIFIED            (0UL)
#define CACHE_ARCHITECTURE_SEPARATE           (1UL)

VOID
CPSRMaskInsert (
  IN  UINT32  Mask,
  IN  UINT32  Value
  );

UINT32
CPSRRead (
  VOID
  );

#endif // ARM_LIB_PRIVATE_H_
