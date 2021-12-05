/** @file
  Plug a PciSegmentLib backend into PciCapLib, for config space access.

  Copyright (C) 2018, Red Hat, Inc.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <IndustryStandard/Pci23.h>

#include <Library/BaseLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PciSegmentLib.h>

#include "BasePciCapPciSegmentLib.h"

/**
  Read the config space of a given PCI device (both normal and extended).

  SegmentDevReadConfig() performs as few config space accesses as possible
  (without attempting 64-bit wide accesses).

  @param[in] PciDevice           Implementation-specific unique representation
                                 of the PCI device in the PCI hierarchy.

  @param[in] SourceOffset        Source offset in the config space of the PCI
                                 device to start reading from.

  @param[out] DestinationBuffer  Buffer to store the read data to.

  @param[in] Size                The number of bytes to transfer.

  @retval RETURN_SUCCESS      Size bytes have been transferred from config
                              space to DestinationBuffer.

  @retval RETURN_UNSUPPORTED  Accessing Size bytes from SourceOffset exceeds
                              the config space limit of the PCI device.
                              Although PCI_CAP_DEV_READ_CONFIG allows reading
                              fewer than Size bytes in this case,
                              SegmentDevReadConfig() will read none.
**/
STATIC
RETURN_STATUS
EFIAPI
SegmentDevReadConfig (
  IN  PCI_CAP_DEV  *PciDevice,
  IN  UINT16       SourceOffset,
  OUT VOID         *DestinationBuffer,
  IN  UINT16       Size
  )
{
  SEGMENT_DEV  *SegmentDev;
  UINT16       ConfigSpaceSize;
  UINT64       SourceAddress;

  SegmentDev      = SEGMENT_DEV_FROM_PCI_CAP_DEV (PciDevice);
  ConfigSpaceSize = (SegmentDev->MaxDomain == PciCapNormal ?
                     PCI_MAX_CONFIG_OFFSET : PCI_EXP_MAX_CONFIG_OFFSET);
  //
  // Note that all UINT16 variables below are promoted to INT32, and the
  // addition and the comparison is carried out in INT32.
  //
  if (SourceOffset + Size > ConfigSpaceSize) {
    return RETURN_UNSUPPORTED;
  }

  SourceAddress = PCI_SEGMENT_LIB_ADDRESS (
                    SegmentDev->SegmentNr,
                    SegmentDev->BusNr,
                    SegmentDev->DeviceNr,
                    SegmentDev->FunctionNr,
                    SourceOffset
                    );
  PciSegmentReadBuffer (SourceAddress, Size, DestinationBuffer);
  return RETURN_SUCCESS;
}

/**
  Write the config space of a given PCI device (both normal and extended).

  SegmentDevWriteConfig() performs as few config space accesses as possible
  (without attempting 64-bit wide accesses).

  @param[in] PciDevice          Implementation-specific unique representation
                                of the PCI device in the PCI hierarchy.

  @param[in] DestinationOffset  Destination offset in the config space of the
                                PCI device to start writing at.

  @param[in] SourceBuffer       Buffer to read the data to be stored from.

  @param[in] Size               The number of bytes to transfer.

  @retval RETURN_SUCCESS      Size bytes have been transferred from
                              SourceBuffer to config space.

  @retval RETURN_UNSUPPORTED  Accessing Size bytes at DestinationOffset exceeds
                              the config space limit of the PCI device.
                              Although PCI_CAP_DEV_WRITE_CONFIG allows writing
                              fewer than Size bytes in this case,
                              SegmentDevWriteConfig() will write none.
**/
STATIC
RETURN_STATUS
EFIAPI
SegmentDevWriteConfig (
  IN PCI_CAP_DEV  *PciDevice,
  IN UINT16       DestinationOffset,
  IN VOID         *SourceBuffer,
  IN UINT16       Size
  )
{
  SEGMENT_DEV  *SegmentDev;
  UINT16       ConfigSpaceSize;
  UINT64       DestinationAddress;

  SegmentDev      = SEGMENT_DEV_FROM_PCI_CAP_DEV (PciDevice);
  ConfigSpaceSize = (SegmentDev->MaxDomain == PciCapNormal ?
                     PCI_MAX_CONFIG_OFFSET : PCI_EXP_MAX_CONFIG_OFFSET);
  //
  // Note that all UINT16 variables below are promoted to INT32, and the
  // addition and the comparison is carried out in INT32.
  //
  if (DestinationOffset + Size > ConfigSpaceSize) {
    return RETURN_UNSUPPORTED;
  }

  DestinationAddress = PCI_SEGMENT_LIB_ADDRESS (
                         SegmentDev->SegmentNr,
                         SegmentDev->BusNr,
                         SegmentDev->DeviceNr,
                         SegmentDev->FunctionNr,
                         DestinationOffset
                         );
  PciSegmentWriteBuffer (DestinationAddress, Size, SourceBuffer);
  return RETURN_SUCCESS;
}

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
  IN  PCI_CAP_DOMAIN  MaxDomain,
  IN  UINT16          Segment,
  IN  UINT8           Bus,
  IN  UINT8           Device,
  IN  UINT8           Function,
  OUT PCI_CAP_DEV     **PciDevice
  )
{
  SEGMENT_DEV  *SegmentDev;

  if ((Device > PCI_MAX_DEVICE) || (Function > PCI_MAX_FUNC)) {
    return RETURN_INVALID_PARAMETER;
  }

  SegmentDev = AllocatePool (sizeof *SegmentDev);
  if (SegmentDev == NULL) {
    return RETURN_OUT_OF_RESOURCES;
  }

  SegmentDev->Signature              = SEGMENT_DEV_SIG;
  SegmentDev->MaxDomain              = MaxDomain;
  SegmentDev->SegmentNr              = Segment;
  SegmentDev->BusNr                  = Bus;
  SegmentDev->DeviceNr               = Device;
  SegmentDev->FunctionNr             = Function;
  SegmentDev->BaseDevice.ReadConfig  = SegmentDevReadConfig;
  SegmentDev->BaseDevice.WriteConfig = SegmentDevWriteConfig;

  *PciDevice = &SegmentDev->BaseDevice;
  return RETURN_SUCCESS;
}

/**
  Free the resources used by PciDevice.

  @param[in] PciDevice  The PCI_CAP_DEV object to free, originally produced by
                        PciCapPciSegmentDeviceInit().
**/
VOID
EFIAPI
PciCapPciSegmentDeviceUninit (
  IN PCI_CAP_DEV  *PciDevice
  )
{
  SEGMENT_DEV  *SegmentDev;

  SegmentDev = SEGMENT_DEV_FROM_PCI_CAP_DEV (PciDevice);
  FreePool (SegmentDev);
}
