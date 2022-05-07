/** @file
  MpInitLibDep Protocol Guid definitions

  Copyright (c) 2022, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef MPINITLIB_DEP_PROTOCOLS_H_
#define MPINITLIB_DEP_PROTOCOLS_H_

// {BB00A5CA-08CE-462F-A537-43C74A825CA4}
#define EFI_MPINITLIB_MP_DEP_PROTOCOL_GUID \
{ \
  0xbb00a5ca, 0x8ce, 0x462f, { 0xa5, 0x37, 0x43, 0xc7, 0x4a, 0x82, 0x5c, 0xa4 } \
};

extern EFI_GUID  gEfiMpInitLibMpDepProtocolGuid;

// {A9E7CEF1-5682-42CC-B123-9930973F4A9F}
#define EFI_PEI_MPINITLIB_UP_DEP_PPI_GUID \
{ \
  0xa9e7cef1, 0x5682, 0x42cc, { 0xb1, 0x23, 0x99, 0x30, 0x97, 0x3f, 0x4a, 0x9f } \
};

extern EFI_GUID  gEfiMpInitLibUpDepProtocolGuid;

#endif
