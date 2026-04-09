/** @file
  Implements get/set firmware volume attributes

Copyright (c) 2006 - 2012, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "DxeMain.h"
#include "FwVolDriver.h"

/**
  Retrieves attributes, insures positive polarity of attribute bits, returns
  resulting attributes in output parameter.

  @param  This             Calling context
  @param  Attributes       output buffer which contains attributes

  @retval EFI_SUCCESS      Successfully got volume attributes

**/
EFI_STATUS
EFIAPI
FvGetVolumeAttributes (
  IN  CONST EFI_FIRMWARE_VOLUME2_PROTOCOL  *This,
  OUT       EFI_FV_ATTRIBUTES              *Attributes
  )
{
  EFI_STATUS                          Status;
  FV_DEVICE                           *FvDevice;
  EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL  *Fvb;
  EFI_FVB_ATTRIBUTES_2                FvbAttributes;

  FvDevice = FV_DEVICE_FROM_THIS (This);
  Fvb      = FvDevice->Fvb;

  //
  // First get the Firmware Volume Block Attributes
  //
  Status = Fvb->GetAttributes (Fvb, &FvbAttributes);

  //
  // Mask out Fvb bits that are not defined in FV
  //
  FvbAttributes &= 0xfffff0ff;

  *Attributes = (EFI_FV_ATTRIBUTES)FvbAttributes;

  return Status;
}

/**
  Sets current attributes for volume

  @param  This             Calling context
  @param  Attributes       At input, contains attributes to be set.  At output
                           contains new value of FV

  @retval EFI_UNSUPPORTED  Could not be set.

**/
EFI_STATUS
EFIAPI
FvSetVolumeAttributes (
  IN     CONST EFI_FIRMWARE_VOLUME2_PROTOCOL  *This,
  IN OUT       EFI_FV_ATTRIBUTES              *Attributes
  )
{
  return EFI_UNSUPPORTED;
}

/**
  Return information of type InformationType for the requested firmware
  volume.

  @param  This             Pointer to EFI_FIRMWARE_VOLUME2_PROTOCOL.
  @param  InformationType  InformationType for requested.
  @param  BufferSize       On input, size of Buffer.On output, the amount of data
                           returned in Buffer.
  @param  Buffer           A poniter to the data buffer to return.

  @retval EFI_SUCCESS      Successfully got volume Information.

**/
EFI_STATUS
EFIAPI
FvGetVolumeInfo (
  IN  CONST EFI_FIRMWARE_VOLUME2_PROTOCOL  *This,
  IN  CONST EFI_GUID                       *InformationType,
  IN OUT UINTN                             *BufferSize,
  OUT VOID                                 *Buffer
  )
{
  return EFI_UNSUPPORTED;
}

/**
  Set information of type InformationType for the requested firmware
  volume.

  @param  This             Pointer to EFI_FIRMWARE_VOLUME2_PROTOCOL.
  @param  InformationType  InformationType for requested.
  @param  BufferSize       On input, size of Buffer.On output, the amount of data
                           returned in Buffer.
  @param  Buffer           A poniter to the data buffer to return.

  @retval EFI_SUCCESS      Successfully set volume Information.

**/
EFI_STATUS
EFIAPI
FvSetVolumeInfo (
  IN  CONST EFI_FIRMWARE_VOLUME2_PROTOCOL  *This,
  IN  CONST EFI_GUID                       *InformationType,
  IN  UINTN                                BufferSize,
  IN CONST  VOID                           *Buffer
  )
{
  return EFI_UNSUPPORTED;
}
