/** @file

  Copyright (c) 2024 Loongson Technology Corporation Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef PAGE_H_
#define PAGE_H_

#define INVALID_PAGE  0

#define LEVEL5  5
#define LEVEL4  4
#define LEVEL3  3
#define LEVEL2  2
#define LEVEL1  1

#define PTE_ATTRIBUTES_MASK  0x600000000000007EULL

#define PTE_PPN_MASK              0xFFFFFFFFF000ULL
#define PTE_PPN_SHIFT             EFI_PAGE_SHIFT
#define LOONGARCH_MMU_PAGE_SHIFT  EFI_PAGE_SHIFT

//
// For coding convenience, define the maximum valid
// LoongArch exception.
// Since UEFI V2.11, it will be present in DebugSupport.h.
//
#define MAX_LOONGARCH_EXCEPTION  64

#endif // PAGE_H_
