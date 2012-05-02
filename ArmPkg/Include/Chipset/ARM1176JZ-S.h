/** @file

  Copyright (c) 2008 - 2009, Apple Inc. All rights reserved.<BR>
  Copyright (c) 2011-2012, ARM Limited. All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __ARM1176JZ_S_H__
#define __ARM1176JZ_S_H__

// Domain Access Control Register
#define DOMAIN_ACCESS_CONTROL_MASK(a)     (3UL << (2 * (a)))
#define DOMAIN_ACCESS_CONTROL_NONE(a)     (0UL << (2 * (a)))
#define DOMAIN_ACCESS_CONTROL_CLIENT(a)   (1UL << (2 * (a)))
#define DOMAIN_ACCESS_CONTROL_RESERVED(a) (2UL << (2 * (a)))
#define DOMAIN_ACCESS_CONTROL_MANAGER(a)  (3UL << (2 * (a)))

#define TRANSLATION_TABLE_SIZE            (16 * 1024)
#define TRANSLATION_TABLE_ALIGNMENT       (16 * 1024)
#define TRANSLATION_TABLE_ALIGNMENT_MASK  (TRANSLATION_TABLE_ALIGNMENT - 1)

#define TRANSLATION_TABLE_ENTRY_FOR_VIRTUAL_ADDRESS(table, address) ((UINT32 *)(table) + (((UINTN)(address)) >> 20))

// Translation table descriptor types
#define TT_DESCRIPTOR_TYPE_MASK         ((1UL << 18) | (3UL << 0))
#define TT_DESCRIPTOR_TYPE_PAGE_TABLE   ((0UL << 18) | (1UL << 0))
#define TT_DESCRIPTOR_TYPE_SECTION      ((0UL << 18) | (2UL << 0))
#define TT_DESCRIPTOR_TYPE_SUPERSECTION ((1UL << 18) | (2UL << 0))

// Section descriptor definitions
#define TT_DESCRIPTOR_SECTION_SIZE                              (0x00100000)

#define TT_DESCRIPTOR_SECTION_NS_MASK                           (1UL << 19)
#define TT_DESCRIPTOR_SECTION_NS                                (1UL << 19)

#define TT_DESCRIPTOR_SECTION_NG_MASK                           (1UL << 17)
#define TT_DESCRIPTOR_SECTION_NG_GLOBAL                         (0UL << 17)
#define TT_DESCRIPTOR_SECTION_NG_LOCAL                          (1UL << 17)

#define TT_DESCRIPTOR_SECTION_S_MASK                            (1UL << 16)
#define TT_DESCRIPTOR_SECTION_S_NOT_SHARED                      (0UL << 16)
#define TT_DESCRIPTOR_SECTION_S_SHARED                          (1UL << 16)

#define TT_DESCRIPTOR_SECTION_AP_MASK                           ((1UL << 15) | (3UL << 10))
#define TT_DESCRIPTOR_SECTION_AP_NO_NO                          ((0UL << 15) | (0UL << 10))
#define TT_DESCRIPTOR_SECTION_AP_RW_NO                          ((0UL << 15) | (1UL << 10))
#define TT_DESCRIPTOR_SECTION_AP_RW_RO                          ((0UL << 15) | (2UL << 10))
#define TT_DESCRIPTOR_SECTION_AP_RW_RW                          ((0UL << 15) | (3UL << 10))
#define TT_DESCRIPTOR_SECTION_AP_RO_NO                          ((1UL << 15) | (1UL << 10))
#define TT_DESCRIPTOR_SECTION_AP_RO_RO                          ((1UL << 15) | (3UL << 10))

#define TT_DESCRIPTOR_CACHE_POLICY_NON_CACHEABLE                (0UL)
#define TT_DESCRIPTOR_CACHE_POLICY_WRITE_BACK_ALLOCATE          (1UL)
#define TT_DESCRIPTOR_CACHE_POLICY_WRITE_THROUGH_NO_ALLOCATE    (2UL)
#define TT_DESCRIPTOR_CACHE_POLICY_WRITE_BACK_NO_ALLOCATE       (3UL)

#define TT_DESCRIPTOR_OUTER_CACHE_POLICY_MASK                       ((1UL << 14) | (3UL << 12))
#define TT_DESCRIPTOR_OUTER_CACHE_POLICY_NON_CACHEABLE              ((1UL << 14) | (TT_DESCRIPTOR_CACHE_POLICY_NON_CACHEABLE             << 12))
#define TT_DESCRIPTOR_OUTER_CACHE_POLICY_WRITE_BACK_ALLOCATE        ((1UL << 14) | (TT_DESCRIPTOR_CACHE_POLICY_WRITE_BACK_ALLOCATE       << 12))
#define TT_DESCRIPTOR_OUTER_CACHE_POLICY_WRITE_THROUGH_NO_ALLOCATE  ((1UL << 14) | (TT_DESCRIPTOR_CACHE_POLICY_WRITE_THROUGH_NO_ALLOCATE << 12))
#define TT_DESCRIPTOR_OUTER_CACHE_POLICY_WRITE_BACK_NO_ALLOCATE     ((1UL << 14) | (TT_DESCRIPTOR_CACHE_POLICY_WRITE_BACK_NO_ALLOCATE    << 12))

#define TT_DESCRIPTOR_INNER_CACHE_POLICY_MASK                       (3UL << 2)
#define TT_DESCRIPTOR_INNER_CACHE_POLICY_NON_CACHEABLE              (TT_DESCRIPTOR_CACHE_POLICY_NON_CACHEABLE             << 2)
#define TT_DESCRIPTOR_INNER_CACHE_POLICY_WRITE_BACK_ALLOCATE        (TT_DESCRIPTOR_CACHE_POLICY_WRITE_BACK_ALLOCATE       << 2)
#define TT_DESCRIPTOR_INNER_CACHE_POLICY_WRITE_THROUGH_NO_ALLOCATE  (TT_DESCRIPTOR_CACHE_POLICY_WRITE_THROUGH_NO_ALLOCATE << 2)
#define TT_DESCRIPTOR_INNER_CACHE_POLICY_WRITE_BACK_NO_ALLOCATE     (TT_DESCRIPTOR_CACHE_POLICY_WRITE_BACK_NO_ALLOCATE    << 2)

#define TT_DESCRIPTOR_SECTION_CACHE_POLICY_MASK                   (TT_DESCRIPTOR_OUTER_CACHE_POLICY_MASK                      | TT_DESCRIPTOR_INNER_CACHE_POLICY_MASK)
#define TT_DESCRIPTOR_SECTION_CACHE_POLICY_WRITE_THROUGH_NO_ALLOC (TT_DESCRIPTOR_OUTER_CACHE_POLICY_WRITE_THROUGH_NO_ALLOCATE | TT_DESCRIPTOR_INNER_CACHE_POLICY_WRITE_THROUGH_NO_ALLOCATE)
#define TT_DESCRIPTOR_SECTION_CACHE_POLICY_WRITE_BACK_NO_ALLOC    (TT_DESCRIPTOR_OUTER_CACHE_POLICY_WRITE_BACK_NO_ALLOCATE    | TT_DESCRIPTOR_INNER_CACHE_POLICY_WRITE_BACK_NO_ALLOCATE)
#define TT_DESCRIPTOR_SECTION_CACHE_POLICY_NON_CACHEABLE          (TT_DESCRIPTOR_OUTER_CACHE_POLICY_NON_CACHEABLE             | TT_DESCRIPTOR_INNER_CACHE_POLICY_NON_CACHEABLE)
#define TT_DESCRIPTOR_SECTION_CACHE_POLICY_WRITE_BACK_ALLOC       (TT_DESCRIPTOR_OUTER_CACHE_POLICY_WRITE_BACK_ALLOCATE       | TT_DESCRIPTOR_INNER_CACHE_POLICY_WRITE_BACK_ALLOCATE)

#define TT_DESCRIPTOR_SECTION_DOMAIN_MASK                       (0x0FUL << 5)
#define TT_DESCRIPTOR_SECTION_DOMAIN(a)                         (((a) & 0x0FUL) << 5)

#define TT_DESCRIPTOR_SECTION_BASE_ADDRESS_MASK                 (0xFFF00000)
#define TT_DESCRIPTOR_SECTION_BASE_ADDRESS(a)                   (a & TT_DESCRIPTOR_SECTION_BASE_ADDRESS_MASK)

#define TT_DESCRIPTOR_SECTION_WRITE_BACK(NonSecure)             (TT_DESCRIPTOR_TYPE_SECTION                                                          | \
                                                                (NonSecure ? TT_DESCRIPTOR_SECTION_NS : 0)   | \
                                                                TT_DESCRIPTOR_SECTION_NG_GLOBAL                         | \
                                                                TT_DESCRIPTOR_SECTION_S_NOT_SHARED                      | \
                                                                TT_DESCRIPTOR_SECTION_DOMAIN(0)                         | \
                                                                TT_DESCRIPTOR_SECTION_AP_RW_RW                          | \
                                                                TT_DESCRIPTOR_SECTION_CACHE_POLICY_WRITE_BACK_ALLOC)
#define TT_DESCRIPTOR_SECTION_WRITE_THROUGH(NonSecure)          (TT_DESCRIPTOR_TYPE_SECTION                                                          | \
                                                                (NonSecure ? TT_DESCRIPTOR_SECTION_NS : 0)   | \
                                                                TT_DESCRIPTOR_SECTION_NG_GLOBAL                         | \
                                                                TT_DESCRIPTOR_SECTION_S_NOT_SHARED                      | \
                                                                TT_DESCRIPTOR_SECTION_DOMAIN(0)                         | \
                                                                TT_DESCRIPTOR_SECTION_AP_RW_RW                          | \
                                                                TT_DESCRIPTOR_SECTION_CACHE_POLICY_WRITE_THROUGH_NO_ALLOC)
#define TT_DESCRIPTOR_SECTION_UNCACHED(NonSecure)               (TT_DESCRIPTOR_TYPE_SECTION                                                          | \
                                                                (NonSecure ? TT_DESCRIPTOR_SECTION_NS : 0)   | \
                                                                TT_DESCRIPTOR_SECTION_NG_GLOBAL                         | \
                                                                TT_DESCRIPTOR_SECTION_S_NOT_SHARED                      | \
                                                                TT_DESCRIPTOR_SECTION_DOMAIN(0)                         | \
                                                                TT_DESCRIPTOR_SECTION_AP_RW_RW                          | \
                                                                TT_DESCRIPTOR_SECTION_CACHE_POLICY_NON_CACHEABLE)

#define CPACR_CP_FULL_ACCESS    0x0FFFFFFF

// NSACR - Non-Secure Access Control Register definitions
#define NSACR_CP(cp)            ((1 << (cp)) & 0x3FFF)
#define NSACR_PLE               0
#define NSACR_TL                0
#define NSACR_NS_SMP            0

// SCR - Secure Configuration Register definitions
#define SCR_NS                  (1 << 0)
#define SCR_IRQ                 (1 << 1)
#define SCR_FIQ                 (1 << 2)
#define SCR_EA                  (1 << 3)
#define SCR_FW                  (1 << 4)
#define SCR_AW                  (1 << 5)

#endif // __ARM1176JZ_S_H__
