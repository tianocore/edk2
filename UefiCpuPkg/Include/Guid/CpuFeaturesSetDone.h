/** @file
  CPU Features Set Done PPI/Protocol should be installed after CPU features
  configuration are set.

Copyright (c) 2016, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _CPU_FEATURES_INIT_DONE_H_
#define _CPU_FEATURES_INIT_DONE_H_

#define EDKII_CPU_FEATURES_SET_DONE_GUID \
  { \
    { 0xa82485ce, 0xad6b, 0x4101, { 0x99, 0xd3, 0xe1, 0x35, 0x8c, 0x9e, 0x7e, 0x37 } \
  }

extern EFI_GUID gEdkiiCpuFeaturesSetDoneGuid;

#endif
