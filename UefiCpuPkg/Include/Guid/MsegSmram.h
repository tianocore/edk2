/** @file

  Defines the HOB GUID used to describe the MSEG memory region allocated in PEI.

  Copyright (c) 2015 - 2016, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _MSEG_SMRAM_H_
#define _MSEG_SMRAM_H_

#define MSEG_SMRAM_GUID \
  { \
    0x5802bce4, 0xeeee, 0x4e33, { 0xa1, 0x30, 0xeb, 0xad, 0x27, 0xf0, 0xe4, 0x39 } \
  }

extern EFI_GUID  gMsegSmramGuid;

//
// The data portion of this HOB is type EFI_SMRAM_DESCRIPTOR
//

#endif
