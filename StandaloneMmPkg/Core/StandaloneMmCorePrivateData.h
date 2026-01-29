/** @file
  The internal header file that declared a data structure that is shared
  between the MM IPL and the MM Core.

  Copyright (c) 2009 - 2014, Intel Corporation. All rights reserved.<BR>
  Copyright (c) 2016 - 2018, ARM Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _STANDALONE_MM_CORE_PRIVATE_DATA_H_
#define _STANDALONE_MM_CORE_PRIVATE_DATA_H_

//
// Page management
//

typedef struct {
  LIST_ENTRY    Link;
  UINTN         NumberOfPages;
} FREE_PAGE_LIST;

extern LIST_ENTRY  mMmMemoryMap;

//
// Pool management
//

//
// MIN_POOL_SHIFT must not be less than 5
//
#define MIN_POOL_SHIFT  6
#define MIN_POOL_SIZE   (1 << MIN_POOL_SHIFT)

//
// MAX_POOL_SHIFT must not be less than EFI_PAGE_SHIFT - 1
//
#define MAX_POOL_SHIFT  (EFI_PAGE_SHIFT - 1)
#define MAX_POOL_SIZE   (1 << MAX_POOL_SHIFT)

//
// MAX_POOL_INDEX are calculated by maximum and minimum pool sizes
//
#define MAX_POOL_INDEX  (MAX_POOL_SHIFT - MIN_POOL_SHIFT + 1)

typedef struct {
  UINTN      Size;
  BOOLEAN    Available;
} POOL_HEADER;

typedef struct {
  POOL_HEADER    Header;
  LIST_ENTRY     Link;
} FREE_POOL_HEADER;

extern LIST_ENTRY  mMmPoolLists[MAX_POOL_INDEX];

#endif
