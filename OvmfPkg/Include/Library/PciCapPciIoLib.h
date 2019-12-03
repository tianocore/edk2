/** @file
  Library class layered on top of PciCapLib that allows clients to plug an
  EFI_PCI_IO_PROTOCOL backend into PciCapLib, for config space access.

  Copyright (C) 2018, Red Hat, Inc.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef __PCI_CAP_PCI_IO_LIB_H__
#define __PCI_CAP_PCI_IO_LIB_H__

#include <Protocol/PciIo.h>

#include <Library/PciCapLib.h>


/**
  Create a PCI_CAP_DEV object from an EFI_PCI_IO_PROTOCOL instance. The config
  space accessors are based upon EFI_PCI_IO_PROTOCOL.Pci.Read() and
  EFI_PCI_IO_PROTOCOL.Pci.Write().

  @param[in] PciIo       EFI_PCI_IO_PROTOCOL representation of the PCI device.

  @param[out] PciDevice  The PCI_CAP_DEV object constructed as described above.
                         PciDevice can be passed to the PciCapLib APIs.

  @retval EFI_SUCCESS           PciDevice has been constructed and output.

  @retval EFI_OUT_OF_RESOURCES  Memory allocation failed.
**/
EFI_STATUS
EFIAPI
PciCapPciIoDeviceInit (
  IN  EFI_PCI_IO_PROTOCOL *PciIo,
  OUT PCI_CAP_DEV         **PciDevice
  );


/**
  Free the resources used by PciDevice.

  @param[in] PciDevice  The PCI_CAP_DEV object to free, originally produced by
                        PciCapPciIoDeviceInit().
**/
VOID
EFIAPI
PciCapPciIoDeviceUninit (
  IN PCI_CAP_DEV *PciDevice
  );

#endif // __PCI_CAP_PCI_IO_LIB_H__
