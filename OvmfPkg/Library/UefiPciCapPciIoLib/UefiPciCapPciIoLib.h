/** @file
  Plug an EFI_PCI_IO_PROTOCOL backend into PciCapLib, for config space access
  -- internal macro and type definitions.

  Copyright (C) 2018, Red Hat, Inc.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef __UEFI_PCI_CAP_PCI_IO_LIB_H__
#define __UEFI_PCI_CAP_PCI_IO_LIB_H__

#include <Library/DebugLib.h>

#include <Library/PciCapPciIoLib.h>

#define PROTO_DEV_SIG SIGNATURE_64 ('P', 'C', 'P', 'I', 'O', 'P', 'R', 'T')

typedef struct {
  //
  // Signature identifying the derived class.
  //
  UINT64 Signature;
  //
  // Members added by the derived class, specific to the use of
  // EFI_PCI_IO_PROTOCOL.
  //
  EFI_PCI_IO_PROTOCOL *PciIo;
  //
  // Base class.
  //
  PCI_CAP_DEV BaseDevice;
} PROTO_DEV;

#define PROTO_DEV_FROM_PCI_CAP_DEV(PciDevice) \
  CR (PciDevice, PROTO_DEV, BaseDevice, PROTO_DEV_SIG)

#endif // __UEFI_PCI_CAP_PCI_IO_LIB_H__
