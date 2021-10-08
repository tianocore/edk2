/** @file
  AML Resource Data.

  Copyright (c) 2019 - 2020, Arm Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Glossary:
  - Rd or RD   - Resource Data
  - Rds or RDS - Resource Data Small
  - Rdl or RDL - Resource Data Large
**/

#include <ResourceData/AmlResourceData.h>

/** Check whether the resource data has the input descriptor Id.

  The small/large bit is included in the descriptor Id,
  but the size bits are not included for small resource data elements.

  @param  [in]  Header        Pointer to the first byte of a resource data
                              element.
  @param  [in]  DescriptorId  The descriptor to check against.

  @retval TRUE    The resource data has the descriptor Id.
  @retval FALSE   Otherwise.
**/
BOOLEAN
EFIAPI
AmlRdCompareDescId (
  IN  CONST AML_RD_HEADER   * Header,
  IN        AML_RD_HEADER     DescriptorId
  )
{
  if (Header == NULL) {
    ASSERT (0);
    return FALSE;
  }

  if (AML_RD_IS_LARGE (Header)) {
    return ((*Header ^ DescriptorId) == 0);
  } else {
    return (((*Header & AML_RD_SMALL_ID_MASK) ^ DescriptorId) == 0);
  }
}

/** Get the descriptor Id of the resource data.

  The small/large bit is included in the descriptor Id,
  but the size bits are not included for small resource data elements.

  @param  [in]  Header  Pointer to the first byte of a resource data.

  @return A descriptor Id.
**/
AML_RD_HEADER
EFIAPI
AmlRdGetDescId (
  IN  CONST AML_RD_HEADER   * Header
  )
{
  if (Header == NULL) {
    ASSERT (0);
    return FALSE;
  }

  if (AML_RD_IS_LARGE (Header)) {
    return *Header;
  }

  // Header is a small resource data element.
  return *Header & AML_RD_SMALL_ID_MASK;
}

/** Get the size of a resource data element.

  If the resource data element is of the large type, the Header
  is expected to be at least 3 bytes long.

  @param  [in]  Header  Pointer to the first byte of a resource data.

  @return The size of the resource data element.
**/
UINT32
EFIAPI
AmlRdGetSize (
  IN  CONST AML_RD_HEADER   * Header
  )
{
  if (Header == NULL) {
    ASSERT (0);
    return FALSE;
  }

  if (AML_RD_IS_LARGE (Header)) {
    return ((ACPI_LARGE_RESOURCE_HEADER*)Header)->Length +
             sizeof (ACPI_LARGE_RESOURCE_HEADER);
  }

  // Header is a small resource data element.
  return ((ACPI_SMALL_RESOURCE_HEADER*)Header)->Bits.Length +
           sizeof (ACPI_SMALL_RESOURCE_HEADER);
}

/** Set the Checksum of an EndTag resource data.

  ACPI 6.4, s6.4.2.9 "End Tag":
  "This checksum is generated such that adding it to the sum of all the data
  bytes will produce a zero sum."
  "If the checksum field is zero, the resource data is treated as if the
  checksum operation succeeded. Configuration proceeds normally."

  @param  [in]  Header     Pointer to the first byte of a resource data.
  @param  [in]  CheckSum   Checksum value to set.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
**/
EFI_STATUS
EFIAPI
AmlRdSetEndTagChecksum (
  IN  CONST AML_RD_HEADER   * Header,
  IN        UINT8             CheckSum
  )
{
  if ((Header == NULL)  ||
      !AmlRdCompareDescId (
        Header,
        AML_RD_BUILD_SMALL_DESC_ID (ACPI_SMALL_END_TAG_DESCRIPTOR_NAME))) {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  ((EFI_ACPI_END_TAG_DESCRIPTOR*)Header)->Checksum = CheckSum;
  return EFI_SUCCESS;
}
