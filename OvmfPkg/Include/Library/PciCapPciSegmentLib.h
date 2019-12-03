/** @file
  Library class layered on top of PciCapLib that allows clients to plug a
  PciSegmentLib backend into PciCapLib, for config space access.

  Copyright (C) 2018, Red Hat, Inc.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef __PCI_CAP_PCI_SEGMENT_LIB_H__
#define __PCI_CAP_PCI_SEGMENT_LIB_H__

#include <Library/PciCapLib.h>


/**
  Create a PCI_CAP_DEV object from the PCI Segment:Bus:Device.Function
  quadruplet. The config space accessors are based upon PciSegmentLib.

  @param[in] MaxDomain   If MaxDomain is PciCapExtended, then
                         PciDevice->ReadConfig() and PciDevice->WriteConfig()
                         will delegate extended config space accesses too to
                         PciSegmentReadBuffer() and PciSegmentWriteBuffer(),
                         respectively. Otherwise, PciDevice->ReadConfig() and
                         PciDevice->WriteConfig() will reject accesses to
                         extended config space with RETURN_UNSUPPORTED, without
                         calling PciSegmentReadBuffer() or
                         PciSegmentWriteBuffer(). By setting MaxDomain to
                         PciCapNormal, the platform can prevent undefined
                         PciSegmentLib behavior when the PCI root bridge under
                         the PCI device at Segment:Bus:Device.Function doesn't
                         support extended config space.

  @param[in] Segment     16-bit wide segment number.

  @param[in] Bus         8-bit wide bus number.

  @param[in] Device      5-bit wide device number.

  @param[in] Function    3-bit wide function number.

  @param[out] PciDevice  The PCI_CAP_DEV object constructed as described above.
                         PciDevice can be passed to the PciCapLib APIs.

  @retval RETURN_SUCCESS            PciDevice has been constructed and output.

  @retval RETURN_INVALID_PARAMETER  Device or Function does not fit in the
                                    permitted number of bits.

  @retval RETURN_OUT_OF_RESOURCES   Memory allocation failed.
**/
RETURN_STATUS
EFIAPI
PciCapPciSegmentDeviceInit (
  IN  PCI_CAP_DOMAIN MaxDomain,
  IN  UINT16         Segment,
  IN  UINT8          Bus,
  IN  UINT8          Device,
  IN  UINT8          Function,
  OUT PCI_CAP_DEV    **PciDevice
  );


/**
  Free the resources used by PciDevice.

  @param[in] PciDevice  The PCI_CAP_DEV object to free, originally produced by
                        PciCapPciSegmentDeviceInit().
**/
VOID
EFIAPI
PciCapPciSegmentDeviceUninit (
  IN PCI_CAP_DEV *PciDevice
  );

#endif // __PCI_CAP_PCI_SEGMENT_LIB_H__
