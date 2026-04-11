/** @file
  The implementation of the EFI UFS Device Config Protocol.

  Copyright (c) 2017, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "UfsPassThru.h"

/**
  Read or write specified device descriptor of a UFS device.

  The function is used to read/write UFS device descriptors. The consumer of this API is
  responsible for allocating the data buffer pointed by Descriptor.

  @param[in]      This          The pointer to the EFI_UFS_DEVICE_CONFIG_PROTOCOL instance.
  @param[in]      Read          The boolean variable to show r/w direction.
  @param[in]      DescId        The ID of device descriptor.
  @param[in]      Index         The Index of device descriptor.
  @param[in]      Selector      The Selector of device descriptor.
  @param[in, out] Descriptor    The buffer of device descriptor to be read or written.
  @param[in, out] DescSize      The size of device descriptor buffer. On input, the size, in bytes,
                                of the data buffer specified by Descriptor. On output, the number
                                of bytes that were actually transferred.

  @retval EFI_SUCCESS           The device descriptor is read/written successfully.
  @retval EFI_INVALID_PARAMETER This is NULL or Descriptor is NULL or DescSize is NULL.
                                DescId, Index and Selector are invalid combination to point to a
                                type of UFS device descriptor.
  @retval EFI_DEVICE_ERROR      The device descriptor is not read/written successfully.

**/
EFI_STATUS
EFIAPI
UfsRwUfsDescriptor (
  IN EFI_UFS_DEVICE_CONFIG_PROTOCOL  *This,
  IN BOOLEAN                         Read,
  IN UINT8                           DescId,
  IN UINT8                           Index,
  IN UINT8                           Selector,
  IN OUT UINT8                       *Descriptor,
  IN OUT UINT32                      *DescSize
  )
{
  EFI_STATUS                  Status;
  UFS_PASS_THRU_PRIVATE_DATA  *Private;

  Private = UFS_PASS_THRU_PRIVATE_DATA_FROM_DEV_CONFIG (This);

  if ((This == NULL) || (Descriptor == NULL) || (DescSize == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  Status = UfsRwDeviceDesc (
             Private,
             Read,
             DescId,
             Index,
             Selector,
             Descriptor,
             DescSize
             );
  if (Status == EFI_TIMEOUT) {
    Status = EFI_DEVICE_ERROR;
  }

  return Status;
}

/**
  Read or write specified flag of a UFS device.

  The function is used to read/write UFS flag descriptors. The consumer of this API is responsible
  for allocating the buffer pointed by Flag. The buffer size is 1 byte as UFS flag descriptor is
  just a single Boolean value that represents a TRUE or FALSE, '0' or '1', ON or OFF type of value.

  @param[in]      This          The pointer to the EFI_UFS_DEVICE_CONFIG_PROTOCOL instance.
  @param[in]      Read          The boolean variable to show r/w direction.
  @param[in]      FlagId        The ID of flag to be read or written.
  @param[in, out] Flag          The buffer to set or clear flag.

  @retval EFI_SUCCESS           The flag descriptor is set/clear successfully.
  @retval EFI_INVALID_PARAMETER This is NULL or Flag is NULL.
                                FlagId is an invalid UFS flag ID.
  @retval EFI_DEVICE_ERROR      The flag is not set/clear successfully.

**/
EFI_STATUS
EFIAPI
UfsRwUfsFlag (
  IN EFI_UFS_DEVICE_CONFIG_PROTOCOL  *This,
  IN BOOLEAN                         Read,
  IN UINT8                           FlagId,
  IN OUT UINT8                       *Flag
  )
{
  EFI_STATUS                  Status;
  UFS_PASS_THRU_PRIVATE_DATA  *Private;

  Private = UFS_PASS_THRU_PRIVATE_DATA_FROM_DEV_CONFIG (This);

  if ((This == NULL) || (Flag == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  Status = UfsRwFlags (Private, Read, FlagId, Flag);
  if (Status == EFI_TIMEOUT) {
    Status = EFI_DEVICE_ERROR;
  }

  return Status;
}

/**
  Read or write specified attribute of a UFS device.

  The function is used to read/write UFS attributes. The consumer of this API is responsible for
  allocating the data buffer pointed by Attribute.

  @param[in]      This          The pointer to the EFI_UFS_DEVICE_CONFIG_PROTOCOL instance.
  @param[in]      Read          The boolean variable to show r/w direction.
  @param[in]      AttrId        The ID of Attribute.
  @param[in]      Index         The Index of Attribute.
  @param[in]      Selector      The Selector of Attribute.
  @param[in, out] Attribute     The buffer of Attribute to be read or written.
  @param[in, out] AttrSize      The size of Attribute buffer. On input, the size, in bytes, of the
                                data buffer specified by Attribute. On output, the number of bytes
                                that were actually transferred.

  @retval EFI_SUCCESS           The attribute is read/written successfully.
  @retval EFI_INVALID_PARAMETER This is NULL or Attribute is NULL or AttrSize is NULL.
                                AttrId, Index and Selector are invalid combination to point to a
                                type of UFS attribute.
  @retval EFI_DEVICE_ERROR      The attribute is not read/written successfully.

**/
EFI_STATUS
EFIAPI
UfsRwUfsAttribute (
  IN EFI_UFS_DEVICE_CONFIG_PROTOCOL  *This,
  IN BOOLEAN                         Read,
  IN UINT8                           AttrId,
  IN UINT8                           Index,
  IN UINT8                           Selector,
  IN OUT UINT8                       *Attribute,
  IN OUT UINT32                      *AttrSize
  )
{
  EFI_STATUS                  Status;
  UFS_PASS_THRU_PRIVATE_DATA  *Private;
  UINT32                      Attribute32;

  Private     = UFS_PASS_THRU_PRIVATE_DATA_FROM_DEV_CONFIG (This);
  Attribute32 = 0;

  if ((This == NULL) || (Attribute == NULL) || (AttrSize == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // According to UFS Version 2.1 Spec (JESD220C) Section 14.3, the size of a attribute will not
  // exceed 32-bit.
  //
  if (*AttrSize > 4) {
    return EFI_INVALID_PARAMETER;
  }

  if (!Read) {
    CopyMem (&Attribute32, Attribute, *AttrSize);
  }

  Status = UfsRwAttributes (
             Private,
             Read,
             AttrId,
             Index,
             Selector,
             &Attribute32
             );
  if (!EFI_ERROR (Status)) {
    if (Read) {
      CopyMem (Attribute, &Attribute32, *AttrSize);
    }
  } else {
    *AttrSize = 0;
    if (Status == EFI_TIMEOUT) {
      Status = EFI_DEVICE_ERROR;
    }
  }

  return Status;
}
