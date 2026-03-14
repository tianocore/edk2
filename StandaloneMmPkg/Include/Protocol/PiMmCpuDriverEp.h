/** @file
  This file describes the PiMm Cpu Driver Entry Point Protocol.
  the protocol is for defining handler for mm communication request event
  according to cpu driver.
  // Driver has been dereived from ARM MM implementation
  Copyright (c) 2025, Rivos Inc. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef PI_MM_CPU_DRIVER_EP_H_
#define PI_MM_CPU_DRIVER_EP_H_

#include <PiMm.h>

#define EDKII_PI_MM_CPU_DRIVER_EP_GUID  { \
  0x6ecbd5a1, 0xc0f8, 0x4702, { 0x83, 0x01, 0x4f, 0xc2, 0xc5, 0x47, 0x0a, 0x51 } \
  }

/**
  The PI Standalone MM entry point for handling mm communication request
  Here is an example of how the EDKII_PI_MM_CPU_DRIVER_EP_PROTOCOL
  is utilized in RISCV64:
    1. StandaloneMmCoreEntryPoint loads StandaloneMmCore.
    2. StandaloneMmCore dispatches all MM drivers,
       including the StandaloneMmCpu driver.
    3. The StandaloneMmCpu driver declares its MMI CPU entry point through
       the PI_MM_CPU_DRIVER_EP_PROTOCOL.
    4. After all drivers have been dispatched,
       StandaloneMmCoreEntryPoint retrieves the MMI CPU entry point
       by locating the protocol.
    5. The DelegatedEventLoop then calls the MM CPU entry point.

  See MdePkg/Library/StandaloneMmCoreEntryPoint/RiscV64/StandaloneMmCoreEntryPoint.c

  @param  [in] EventId            The event Id based on firmware.
  @param  [in] CommBufferAddr     Address of the communication buffer.

  @retval   EFI_SUCCESS             Success.
  @retval   EFI_INVALID_PARAMETER   A parameter was invalid.
  @retval   EFI_ACCESS_DENIED       Access not permitted.
  @retval   EFI_OUT_OF_RESOURCES    Out of resources.
  @retval   EFI_UNSUPPORTED         Operation not supported.
**/
typedef
EFI_STATUS
(EFIAPI *EDKII_PI_MM_CPU_DRIVER_ENTRYPOINT)(
  IN UINTN  EventId,
  IN UINTN  CpuNumber,
  IN UINTN  CommBufferAddr
  );

typedef struct _EDKII_PI_MM_CPU_DRIVER_EP_PROTOCOL EDKII_PI_MM_CPU_DRIVER_EP_PROTOCOL;

struct _EDKII_PI_MM_CPU_DRIVER_EP_PROTOCOL {
  EDKII_PI_MM_CPU_DRIVER_ENTRYPOINT    PiMmCpuDriverEntryPoint;
};

extern EFI_GUID  gEdkiiPiMmCpuDriverEpProtocolGuid;

#endif
