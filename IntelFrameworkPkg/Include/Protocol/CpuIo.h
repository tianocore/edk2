/** @file
  This code abstracts the CPU IO Protocol which installed by some platform or chipset-specific
  PEIM that abstracts the processor-visible I/O operations.

  Note: This is a runtime protocol and can be used by runtime drivers after ExitBootServices().
  It is different from the PI 1.2 CPU I/O 2 Protocol, which is a boot services only protocol
  and may not be used by runtime drivers after ExitBootServices().

Copyright (c) 2007 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Revision Reference:
  CPU IO Protocol is defined in Framework of EFI CPU IO Protocol Spec
  Version 0.9.

**/

#ifndef _CPUIO_H_
#define _CPUIO_H_

#include <Protocol/CpuIo2.h>

#define EFI_CPU_IO_PROTOCOL_GUID \
  { \
    0xB0732526, 0x38C8, 0x4b40, {0x88, 0x77, 0x61, 0xC7, 0xB0, 0x6A, 0xAC, 0x45 } \
  }

//
// Framework CPU IO protocol structure is the same as CPU IO 2 protocol defined in PI 1.2 spec.
// However, there is a significant different between the Framework CPU I/O
// Protocol and the PI 1.2 CPU I/O 2 Protocol.  The Framework one is a runtime
// protocol, which means it can be used by runtime drivers after ExitBootServices().
// The PI one is not runtime safe, so it is a boot services only protocol and may
// not be used by runtime drivers after ExitBootServices().
//
typedef EFI_CPU_IO2_PROTOCOL EFI_CPU_IO_PROTOCOL;

extern EFI_GUID gEfiCpuIoProtocolGuid;

#endif
