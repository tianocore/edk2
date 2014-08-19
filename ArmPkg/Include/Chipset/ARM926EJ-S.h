/** @file

  Copyright (c) 2008 - 2009, Apple Inc. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __ARM926EJ_S_H__
#define __ARM926EJ_S_H__

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
#define TT_DESCRIPTOR_TYPE_MASK     (3UL << 0)
#define TT_DESCRIPTOR_TYPE_FAULT    (0UL << 0)
#define TT_DESCRIPTOR_TYPE_COARSE   ((1UL << 0) | (1UL << 4))
#define TT_DESCRIPTOR_TYPE_SECTION  ((2UL << 0) | (1UL << 4))
#define TT_DESCRIPTOR_TYPE_FINE     ((3UL << 0) | (1UL << 4))

// Section descriptor definitions
#define TT_DESCRIPTOR_SECTION_SIZE                              (0x00100000)

#define TT_DESCRIPTOR_SECTION_CACHE_POLICY_MASK                 (3UL <<  2)
#define TT_DESCRIPTOR_SECTION_CACHE_POLICY_UNCACHED_UNBUFFERED  (0UL <<  2)
#define TT_DESCRIPTOR_SECTION_CACHE_POLICY_UNCACHED_BUFFERED    (1UL <<  2)
#define TT_DESCRIPTOR_SECTION_CACHE_POLICY_WRITE_THROUGH        (2UL <<  2)
#define TT_DESCRIPTOR_SECTION_CACHE_POLICY_WRITE_BACK           (3UL <<  2)

#define TT_DESCRIPTOR_SECTION_ACCESS_PERMISSION_MASK            (3UL << 10)
#define TT_DESCRIPTOR_SECTION_ACCESS_PERMISSION_NONE            (1UL << 10)
#define TT_DESCRIPTOR_SECTION_ACCESS_PERMISSION_READ_ONLY       (2UL << 10)
#define TT_DESCRIPTOR_SECTION_ACCESS_PERMISSION_READ_WRITE      (3UL << 10)

#define TT_DESCRIPTOR_SECTION_DOMAIN_MASK                       (0x0FUL << 5)
#define TT_DESCRIPTOR_SECTION_DOMAIN(a)                         (((a) & 0xF) << 5)

#define TT_DESCRIPTOR_SECTION_BASE_ADDRESS_MASK                 (0xFFF00000)
#define TT_DESCRIPTOR_SECTION_BASE_ADDRESS(a)                   (a & TT_DESCRIPTOR_SECTION_BASE_ADDRESS_MASK)

#define TT_DESCRIPTOR_SECTION_WRITE_BACK          (TT_DESCRIPTOR_SECTION_ACCESS_PERMISSION_READ_WRITE | \
                                                   TT_DESCRIPTOR_SECTION_DOMAIN(0)                    | \
                                                   TT_DESCRIPTOR_SECTION_CACHE_POLICY_WRITE_BACK      | \
                                                   TT_DESCRIPTOR_TYPE_SECTION)
#define TT_DESCRIPTOR_SECTION_WRITE_THROUGH       (TT_DESCRIPTOR_SECTION_ACCESS_PERMISSION_READ_WRITE | \
                                                   TT_DESCRIPTOR_SECTION_DOMAIN(0)                    | \
                                                   TT_DESCRIPTOR_SECTION_CACHE_POLICY_WRITE_THROUGH   | \
                                                   TT_DESCRIPTOR_TYPE_SECTION)
#define TT_DESCRIPTOR_SECTION_UNCACHED_UNBUFFERED (TT_DESCRIPTOR_SECTION_ACCESS_PERMISSION_READ_WRITE     | \
                                                   TT_DESCRIPTOR_SECTION_DOMAIN(0)                        | \
                                                   TT_DESCRIPTOR_SECTION_CACHE_POLICY_UNCACHED_UNBUFFERED | \
                                                   TT_DESCRIPTOR_TYPE_SECTION)

#endif // __ARM926EJ_S_H__
