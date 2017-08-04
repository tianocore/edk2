/** @file
  Instance of Base PCI Segment Library that support multi-segment PCI configuration access.

  PCI Segment Library that consumes segment information provided by PciSegmentInfoLib to
   support multi-segment PCI configuration access through enhanced configuration access mechanism.

  Copyright (c) 2017, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials are
  licensed and made available under the terms and conditions of
  the BSD License which accompanies this distribution.  The full
  text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "PciSegmentLibCommon.h"

/**
  Return the virtual address for the physical address.

  @param  Address  The physical address.

  @retval The virtual address.
**/
UINTN
PciSegmentLibVirtualAddress (
  IN UINTN                     Address
  )
{
  return Address;
}

/**
  Register a PCI device so PCI configuration registers may be accessed after
  SetVirtualAddressMap().

  If any reserved bits in Address are set, then ASSERT().

  @param  Address The address that encodes the PCI Bus, Device, Function and
                  Register.

  @retval RETURN_SUCCESS           The PCI device was registered for runtime access.
  @retval RETURN_UNSUPPORTED       An attempt was made to call this function
                                   after ExitBootServices().
  @retval RETURN_UNSUPPORTED       The resources required to access the PCI device
                                   at runtime could not be mapped.
  @retval RETURN_OUT_OF_RESOURCES  There are not enough resources available to
                                   complete the registration.

**/
RETURN_STATUS
EFIAPI
PciSegmentRegisterForRuntimeAccess (
  IN UINTN  Address
  )
{
  //
  // Use PciSegmentLibGetEcamAddress() to validate the Address.
  //
  DEBUG_CODE (
    UINTN                        Count;
    PCI_SEGMENT_INFO             *SegmentInfo;

    SegmentInfo = GetPciSegmentInfo (&Count);
    PciSegmentLibGetEcamAddress (Address, SegmentInfo, Count);
  );
  return RETURN_SUCCESS;
}
