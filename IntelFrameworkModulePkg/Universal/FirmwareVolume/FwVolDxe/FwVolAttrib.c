/** @file

  Implements get/set firmware volume attributes.

  Copyright (c) 2006 - 2018, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

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
  OUT EFI_FV_ATTRIBUTES             *Attributes
  )
{
  EFI_STATUS                          Status;
  FV_DEVICE                           *FvDevice;
  EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL  *Fvb;
  EFI_FVB_ATTRIBUTES_2                FvbAttributes;

  FvDevice  = FV_DEVICE_FROM_THIS (This);
  Fvb       = FvDevice->Fvb;

  //
  // First get the Firmware Volume Block Attributes
  //
  Status = Fvb->GetAttributes (Fvb, &FvbAttributes);
  FvbAttributes &= 0xfffff0ff;

  *Attributes = FvbAttributes;
  *Attributes |= EFI_FV2_WRITE_POLICY_RELIABLE;
  return Status;
}

/**
  Sets current attributes for volume.

  @param  This          Calling context
  @param  Attributes    On input, FvAttributes is a pointer to
                        an EFI_FV_ATTRIBUTES containing the
                        desired firmware volume settings. On
                        successful return, it contains the new
                        settings of the firmware volume. On
                        unsuccessful return, FvAttributes is not
                        modified and the firmware volume
                        settings are not changed.

  @retval EFI_SUCCESS             The requested firmware volume attributes
                                  were set and the resulting
                                  EFI_FV_ATTRIBUTES is returned in
                                  FvAttributes.
  @retval EFI_ACCESS_DENIED       Atrribute is locked down.
  @retval EFI_INVALID_PARAMETER   Atrribute is not valid.

**/
EFI_STATUS
EFIAPI
FvSetVolumeAttributes (
  IN CONST EFI_FIRMWARE_VOLUME2_PROTOCOL   *This,
  IN OUT EFI_FV_ATTRIBUTES          *Attributes
  )
{
  EFI_STATUS                          Status;
  FV_DEVICE                           *FvDevice;
  EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL  *Fvb;
  EFI_FVB_ATTRIBUTES_2                OldFvbAttributes;
  EFI_FVB_ATTRIBUTES_2                NewFvbAttributes;
  UINT64                              NewStatus;
  UINT32                              Capabilities;

  FvDevice  = FV_DEVICE_FROM_THIS (This);
  Fvb       = FvDevice->Fvb;

  //
  // First get the current Volume Attributes
  //
  Status = Fvb->GetAttributes (
                  Fvb,
                  &OldFvbAttributes
                  );

  if ((OldFvbAttributes & EFI_FVB2_LOCK_STATUS) != 0) {
    return EFI_ACCESS_DENIED;
  }
  //
  // Only status attributes can be updated.
  //
  Capabilities  = OldFvbAttributes & EFI_FVB2_CAPABILITIES;
  NewStatus     = (*Attributes) & EFI_FVB2_STATUS;

  //
  // Test read disable
  //
  if ((Capabilities & EFI_FVB2_READ_DISABLED_CAP) == 0) {
    if ((NewStatus & EFI_FVB2_READ_STATUS) == 0) {
      return EFI_INVALID_PARAMETER;
    }
  }
  //
  // Test read enable
  //
  if ((Capabilities & EFI_FVB2_READ_ENABLED_CAP) == 0) {
    if ((NewStatus & EFI_FVB2_READ_STATUS) != 0) {
      return EFI_INVALID_PARAMETER;
    }
  }
  //
  // Test write disable
  //
  if ((Capabilities & EFI_FVB2_WRITE_DISABLED_CAP) == 0) {
    if ((NewStatus & EFI_FVB2_WRITE_STATUS) == 0) {
      return EFI_INVALID_PARAMETER;
    }
  }
  //
  // Test write enable
  //
  if ((Capabilities & EFI_FVB2_WRITE_ENABLED_CAP) == 0) {
    if ((NewStatus & EFI_FVB2_WRITE_STATUS) != 0) {
      return EFI_INVALID_PARAMETER;
    }
  }
  //
  // Test lock
  //
  if ((Capabilities & EFI_FVB2_LOCK_CAP) == 0) {
    if ((NewStatus & EFI_FVB2_LOCK_STATUS) != 0) {
      return EFI_INVALID_PARAMETER;
    }
  }

  NewFvbAttributes = OldFvbAttributes & (0xFFFFFFFF & (~EFI_FVB2_STATUS));
  NewFvbAttributes |= NewStatus;
  Status = Fvb->SetAttributes (
                  Fvb,
                  &NewFvbAttributes
                  );

  if (EFI_ERROR (Status)) {
    return Status;
  }

  *Attributes = 0;

  This->GetVolumeAttributes (
          This,
          Attributes
          );

  return EFI_SUCCESS;
}

/**
  Return information of type InformationType for the requested firmware
  volume.

  @param This             Pointer to EFI_FIRMWARE_VOLUME2_PROTOCOL.
  @param InformationType  InformationType for requested.
  @param BufferSize       On input, size of Buffer.On output, the amount of
                          data returned in Buffer.
  @param Buffer           A poniter to the data buffer to return.

  @return EFI_UNSUPPORTED Could not get.

**/
EFI_STATUS
EFIAPI
FvGetVolumeInfo (
  IN  CONST EFI_FIRMWARE_VOLUME2_PROTOCOL       *This,
  IN  CONST EFI_GUID                            *InformationType,
  IN OUT UINTN                                  *BufferSize,
  OUT VOID                                      *Buffer
  )
{
  return EFI_UNSUPPORTED;
}

/**
  Set information with InformationType into the requested firmware volume.

  @param  This             Pointer to EFI_FIRMWARE_VOLUME2_PROTOCOL.
  @param  InformationType  InformationType for requested.
  @param  BufferSize       Size of Buffer data.
  @param  Buffer           A poniter to the data buffer to be set.

  @retval EFI_UNSUPPORTED  Could not set.

**/
EFI_STATUS
EFIAPI
FvSetVolumeInfo (
  IN  CONST EFI_FIRMWARE_VOLUME2_PROTOCOL       *This,
  IN  CONST EFI_GUID                            *InformationType,
  IN  UINTN                                     BufferSize,
  IN CONST  VOID                                *Buffer
  )
{
  return EFI_UNSUPPORTED;
}
