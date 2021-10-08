/** @file
  AML Resource Data.

  Copyright (c) 2019 - 2020, Arm Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Glossary:
  - Rd or RD   - Resource Data
  - Rds or RDS - Resource Data Small
  - Rdl or RDL - Resource Data Large
**/

#ifndef AML_RESOURCE_DATA_H_
#define AML_RESOURCE_DATA_H_

/* This header file does not include internal Node definition,
   i.e. AML_ROOT_NODE, AML_OBJECT_NODE, etc. The node definitions
   must be included by the caller file. The function prototypes must
   only expose AML_NODE_HANDLE, AML_ROOT_NODE_HANDLE, etc. node
   definitions.
   This allows to keep the functions defined here both internal and
   potentially external. If necessary, any function of this file can
   be exposed externally.
   The Api folder is internal to the AmlLib, but should only use these
   functions. They provide a "safe" way to interact with the AmlLib.
*/

#include <AmlInclude.h>
#include <IndustryStandard/Acpi.h>

/**
  @defgroup ResourceDataLibrary Resource data library
  @ingroup AMLLib
  @{
    Resource data are defined in the ACPI 6.3 specification,
    s6.4 "Resource Data Types for ACPI". They can be created in ASL via the
    ResourceTemplate () statement, cf s19.3.3 "ASL Resource Templates".

    Resource data can be of the small or large type. The difference between
    small and large resource data elements is their encoding.

    Resource data are stored in the variable list of arguments of object
    nodes.
  @}
*/

/** Resource Descriptor header for Small/Large Resource Data Object.
    This is the first byte of a Small/Large Resource Data element.

  Can be a ACPI_SMALL_RESOURCE_HEADER or ACPI_LARGE_RESOURCE_HEADER.

  @ingroup ResourceDataStructures
*/
typedef UINT8 AML_RD_HEADER;

/** Mask for the small resource data size.

  @ingroup ResourceDataStructures
*/
#define AML_RD_SMALL_SIZE_MASK    (0x7U)

/** Mask for the small resource data ID.

  @ingroup ResourceDataStructures
*/
#define AML_RD_SMALL_ID_MASK      (0xFU << 3)

/** Mask for the large resource data ID.

  @ingroup ResourceDataStructures
*/
#define AML_RD_LARGE_ID_MASK      (0x7FU)

/**
  @defgroup ResourceDataApis Resource data APIs
  @ingroup ResourceDataLibrary
  @{
    Resource data APIs allow to manipulate/decode resource data elements.
  @}
*/

/** Check whether a resource data is of the large type.

  @ingroup ResourceDataApis

  @param  [in]  Header  Pointer to the first byte of a resource data.

  @retval TRUE  If the resource data is of the large type.
  @retval FALSE Otherwise.
**/
#define AML_RD_IS_LARGE(Header)                                               \
          (((ACPI_SMALL_RESOURCE_HEADER*)Header)->Bits.Type ==                \
          ACPI_LARGE_ITEM_FLAG)

/** Build a small resource data descriptor Id.
    The small/large bit is included in the descriptor Id,
    but the size bits are not included.

  @ingroup ResourceDataApis

  @param  [in]  Id  Descriptor Id.

  @return A descriptor Id.
**/
#define AML_RD_BUILD_SMALL_DESC_ID(Id)  ((AML_RD_HEADER)((Id & 0xF) << 3))

/** Build a large resource data descriptor Id.
    The small/large bit is included in the descriptor Id.

  @ingroup ResourceDataApis

  @param  [in]  Id  Id of the descriptor.

  @return A descriptor Id.
**/
#define AML_RD_BUILD_LARGE_DESC_ID(Id)  ((AML_RD_HEADER)((BIT7) | Id))

/** Check whether the resource data has the input descriptor Id.

  The small/large bit is included in the descriptor Id,
  but the size bits are not included for small resource data elements.

  @ingroup ResourceDataApis

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
  );

/** Get the descriptor Id of the resource data.

  The small/large bit is included in the descriptor Id,
  but the size bits are not included for small resource data elements.

  @ingroup ResourceDataApis

  @param  [in]  Header  Pointer to the first byte of a resource data.

  @return A descriptor Id.
**/
AML_RD_HEADER
EFIAPI
AmlRdGetDescId (
  IN  CONST AML_RD_HEADER   * Header
  );

/** Get the size of a resource data element.

  If the resource data element is of the large type, the Header
  is expected to be at least 3 bytes long.

  @ingroup ResourceDataApis

  @param  [in]  Header  Pointer to the first byte of a resource data.

  @return The size of the resource data element.
**/
UINT32
EFIAPI
AmlRdGetSize (
  IN  CONST AML_RD_HEADER   * Header
  );

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
  );

#endif // AML_RESOURCE_DATA_H_
