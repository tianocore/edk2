/** @file
  Opal Password PEI driver which is used to unlock Opal Password for S3.

Copyright (c) 2016 - 2019, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _OPAL_PASSWORD_PEI_H_
#define _OPAL_PASSWORD_PEI_H_

#include <PiPei.h>

#include <Library/DebugLib.h>
#include <Library/IoLib.h>
#include <Library/PciLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PeimEntryPoint.h>
#include <Library/PeiServicesLib.h>
#include <Library/LockBoxLib.h>
#include <Library/TcgStorageOpalLib.h>
#include <Library/Tcg2PhysicalPresenceLib.h>
#include <Library/PeiServicesTablePointerLib.h>

#include <Protocol/StorageSecurityCommand.h>

#include <Ppi/IoMmu.h>
#include <Ppi/StorageSecurityCommand.h>

#include "OpalPasswordCommon.h"

//
// The generic command timeout value (unit in us) for Storage Security Command
// PPI ReceiveData/SendData services
//
#define SSC_PPI_GENERIC_TIMEOUT  30000000

#pragma pack(1)

#define OPAL_PEI_DEVICE_SIGNATURE  SIGNATURE_32 ('o', 'p', 'd', 's')

typedef struct {
  UINTN                                    Signature;
  EFI_STORAGE_SECURITY_COMMAND_PROTOCOL    Sscp;
  OPAL_DEVICE_LOCKBOX_DATA                 *Device;
  VOID                                     *Context;
  EDKII_PEI_STORAGE_SECURITY_CMD_PPI       *SscPpi;
  UINTN                                    DeviceIndex;
} OPAL_PEI_DEVICE;

#define OPAL_PEI_DEVICE_FROM_THIS(a)    \
  CR (a, OPAL_PEI_DEVICE, Sscp, OPAL_PEI_DEVICE_SIGNATURE)

#pragma pack()

#endif // _OPAL_PASSWORD_PEI_H_
