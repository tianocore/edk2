/** @file

  Defines the HOB GUID used to describe the MSEG memory region allocated in PEI.

  Copyright (c) 2015 - 2016, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _MSEG_SMRAM_H_
#define _MSEG_SMRAM_H_

#define MSEG_SMRAM_GUID \
  { \
    0x5802bce4, 0xeeee, 0x4e33, { 0xa1, 0x30, 0xeb, 0xad, 0x27, 0xf0, 0xe4, 0x39 } \
  }

extern EFI_GUID gMsegSmramGuid;

//
// The data portion of this HOB is type EFI_SMRAM_DESCRIPTOR
//

#endif
