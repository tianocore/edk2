/** @file
  SMM Fault Tolerant Write protocol is related to EDK II-specific implementation of FTW,
  provides boot-time service for fault tolerant write capability for block devices in
  EFI SMM environment.  The protocol provides for non-volatile storage of the intermediate
  data and private information a caller would need to recover from a critical fault,
  such as a power failure.

Copyright (c) 2010 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __SMM_FAULT_TOLERANT_WRITE_H__
#define __SMM_FAULT_TOLERANT_WRITE_H__

#include <Protocol/FaultTolerantWrite.h>

#define EFI_SMM_FAULT_TOLERANT_WRITE_PROTOCOL_GUID \
  { \
    0x3868fc3b, 0x7e45, 0x43a7, { 0x90, 0x6c, 0x4b, 0xa4, 0x7d, 0xe1, 0x75, 0x4d } \
  }

//
// SMM Fault Tolerant Write protocol structure is the same as Fault Tolerant Write protocol.
// The SMM one is intend to run in SMM environment, which means it can be used by
// SMM drivers after ExitPmAuth.
//
typedef EFI_FAULT_TOLERANT_WRITE_PROTOCOL EFI_SMM_FAULT_TOLERANT_WRITE_PROTOCOL;

extern EFI_GUID gEfiSmmFaultTolerantWriteProtocolGuid;

#endif
