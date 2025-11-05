/** @file
  CPU Features Init Done PPI/Protocol should be installed after CPU features
  are initialized.

Copyright (c) 2016, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _CPU_FEATURES_INIT_DONE_H_
#define _CPU_FEATURES_INIT_DONE_H_

#define EDKII_CPU_FEATURES_INIT_DONE_GUID \
  { \
    { 0xc77c3a41, 0x61ab, 0x4143, { 0x98, 0x3e, 0x33, 0x39, 0x28, 0x6, 0x28, 0xe5 } \
  }

extern EFI_GUID  gEdkiiCpuFeaturesInitDoneGuid;

#endif
