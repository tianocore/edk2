/** @file
  MpInitLibDepLib PPI definitions

  Copyright (c) 2022, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef MPINITLIB_DEP_H_
#define MPINITLIB_DEP_H_

// {138F9CF4-F0E7-4721-8F49-F5FFECF42D40}
#define EFI_PEI_MPINITLIB_MP_DEP_PPI_GUID \
{ \
  0x138f9cf4, 0xf0e7, 0x4721, { 0x8f, 0x49, 0xf5, 0xff, 0xec, 0xf4, 0x2d, 0x40 } \
};

extern EFI_GUID  gEfiPeiMpInitLibMpDepPpiGuid;

// {0B590774-BC67-49F4-A7DB-E82E89E6B5D6}
#define EFI_PEI_MPINITLIB_UP_DEP_PPI_GUID \
{ \
  0xb590774, 0xbc67, 0x49f4, { 0xa7, 0xdb, 0xe8, 0x2e, 0x89, 0xe6, 0xb5, 0xd6 } \
};

extern EFI_GUID  gEfiPeiMpInitLibUpDepPpiGuid;

#endif
