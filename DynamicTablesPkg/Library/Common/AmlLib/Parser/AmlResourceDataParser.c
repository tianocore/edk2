/** @file
  AML Resource Data Parser.

  Copyright (c) 2019 - 2020, Arm Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Glossary:
  - Rd or RD   - Resource Data
  - Rds or RDS - Resource Data Small
  - Rdl or RDL - Resource Data Large
**/

#include <Parser/AmlResourceDataParser.h>

#include <AmlCoreInterface.h>
#include <AmlDbgPrint/AmlDbgPrint.h>
#include <Tree/AmlNode.h>
#include <Tree/AmlTree.h>

/** Get the size of a resource data element using a stream.

  If the resource data element is of the large type, the Header
  is expected to be at least 3 bytes long.

  The use of a stream makes this function safer
  than the version without stream.

  @param  [in]  FStream     Forward stream pointing to a resource data
                            element.
                            The stream must not be at its end.

  @return The size of the resource data element.
          Zero if error.
**/
UINT32
EFIAPI
AmlRdStreamGetRdSize (
  IN  CONST AML_STREAM    * FStream
  )
{
  CONST AML_RD_HEADER   * CurrRdElement;

  if (!IS_STREAM (FStream)        ||
      IS_END_OF_STREAM (FStream)  ||
      !IS_STREAM_FORWARD (FStream)) {
    ASSERT (0);
    return 0;
  }

  CurrRdElement = (CONST AML_RD_HEADER*)AmlStreamGetCurrPos (FStream);
  if (CurrRdElement == NULL) {
    ASSERT (0);
    return 0;
  }

  // If the resource data element is of the large type, check for overflow.
  if (AML_RD_IS_LARGE (CurrRdElement) &&
      (AmlStreamGetFreeSpace (FStream) <
         sizeof (ACPI_LARGE_RESOURCE_HEADER))) {
    return 0;
  }

  return AmlRdGetSize (CurrRdElement);
}

/** Check the nesting of resource data elements that are dependent
    function descriptors.

  @param  [in]  FStream             Forward stream pointing to a resource data
                                    element. The stream is not
                                    modified/progressing.
                                    The stream must not be at its end.
  @param  [in, out] InFunctionDesc  Pointer holding the nesting of the
                                    resource data buffer.
                                    InFunctionDesc holds TRUE if the resource
                                    data at the address of Buffer is currently
                                    in a dependent function descriptor list.

  @retval FALSE   The Header being parsed is ending a function descriptor
                  list when none started. This should not be possible for a
                  resource data buffer.
  @retval TRUE    Otherwise.
**/
STATIC
BOOLEAN
EFIAPI
AmlRdCheckFunctionDescNesting (
  IN      CONST AML_STREAM    * FStream,
  IN  OUT       BOOLEAN       * InFunctionDesc
  )
{
  CONST AML_RD_HEADER   * CurrRdElement;

  if (!IS_STREAM (FStream)        ||
      IS_END_OF_STREAM (FStream)  ||
      (InFunctionDesc == NULL)) {
    ASSERT (0);
    return FALSE;
  }

  CurrRdElement = AmlStreamGetCurrPos (FStream);
  if (CurrRdElement == NULL) {
    ASSERT (0);
    return FALSE;
  }

  // Starting a dependent function descriptor.
  // It is possible to start one when one has already started.
  if (AmlRdCompareDescId (
        CurrRdElement,
        AML_RD_BUILD_SMALL_DESC_ID (
          ACPI_SMALL_START_DEPENDENT_DESCRIPTOR_NAME))) {
    *InFunctionDesc = TRUE;
    return TRUE;
  }

  // Ending a dependent function descriptor.
  if (AmlRdCompareDescId (
        CurrRdElement,
        AML_RD_BUILD_SMALL_DESC_ID (
          ACPI_SMALL_END_DEPENDENT_DESCRIPTOR_NAME))) {
    if (*InFunctionDesc) {
      *InFunctionDesc = FALSE;
      return TRUE;
    }

    // It should not be possible to end a dependent function descriptor
    // when none started.
    return FALSE;
  }

  return TRUE;
}

/** Check whether the input stream is pointing to a valid list
    of resource data elements.

  The check is based on the size of resource data elements.
  This means that a buffer can pass this check with non-existing descriptor Ids
  that have a correct size.

  A list of resource data elements can contain one unique resource data
  element, without an end tag resource data. This is the case for
  a FieldList.

  @param  [in]  FStream   Forward stream ideally pointing to a resource
                          data element. The stream is not
                          modified/progressing.
                          The stream must not be at its end.

  @retval TRUE    The buffer is holding a valid list of resource data elements.
  @retval FALSE   Otherwise.
**/
BOOLEAN
EFIAPI
AmlRdIsResourceDataBuffer (
  IN  CONST AML_STREAM    * FStream
  )
{
  EFI_STATUS              Status;
  UINT32                  FreeSpace;
  AML_STREAM              SubStream;
  CONST AML_RD_HEADER   * CurrRdElement;
  UINT32                  CurrRdElementSize;
  BOOLEAN                 InFunctionDesc;

  if (!IS_STREAM (FStream)        ||
      IS_END_OF_STREAM (FStream)  ||
      !IS_STREAM_FORWARD (FStream)) {
    ASSERT (0);
    return FALSE;
  }

  // Create a sub-stream from the input stream to leave it untouched.
  Status = AmlStreamInitSubStream (FStream, &SubStream);
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    return FALSE;
  }

  CurrRdElement = AmlStreamGetCurrPos (&SubStream);
  if (CurrRdElement == NULL) {
    ASSERT (0);
    return FALSE;
  }

  // The first element cannot be an end tag.
  if (AmlRdCompareDescId (
       CurrRdElement,
       AML_RD_BUILD_SMALL_DESC_ID (ACPI_SMALL_END_TAG_DESCRIPTOR_NAME))) {
    return FALSE;
  }

  InFunctionDesc = FALSE;
  while (TRUE) {
    FreeSpace = AmlStreamGetFreeSpace (&SubStream);
    CurrRdElement = AmlStreamGetCurrPos (&SubStream);
    CurrRdElementSize = AmlRdStreamGetRdSize (&SubStream);
    if ((FreeSpace == 0)          ||
        (CurrRdElement == NULL)   ||
        (CurrRdElementSize == 0)) {
      return FALSE;
    }

    if (!AmlRdCheckFunctionDescNesting (&SubStream, &InFunctionDesc)) {
      return FALSE;
    }

    if (CurrRdElementSize > FreeSpace) {
      return FALSE;
    } else if (CurrRdElementSize == FreeSpace) {
      return TRUE;
    }

    // @todo Might want to check the CRC when available.
    // An end tag resource data element must be the last element of the list.
    // Thus the function should have already returned.
    if (AmlRdCompareDescId (
          CurrRdElement,
          AML_RD_BUILD_SMALL_DESC_ID (ACPI_SMALL_END_TAG_DESCRIPTOR_NAME))) {
      return FALSE;
    }

    Status = AmlStreamProgress (&SubStream, CurrRdElementSize);
    if (EFI_ERROR (Status)) {
      ASSERT (0);
      return FALSE;
    }
  } // while

  return FALSE;
}

/** Parse a ResourceDataBuffer.

  For each resource data element, create a data node
  and add them to the variable list of arguments of the BufferNode.

  The input stream is expected to point to a valid list of resource data
  elements. A function is available to check it for the caller.

  @param  [in]  BufferNode    Buffer node.
  @param  [in]  FStream       Forward stream pointing to a resource data
                              element.
                              The stream must not be at its end.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_OUT_OF_RESOURCES    Could not allocate memory.
**/
EFI_STATUS
EFIAPI
AmlParseResourceData (
  IN  AML_OBJECT_NODE   * BufferNode,
  IN  AML_STREAM        * FStream
  )
{
  EFI_STATUS              Status;
  AML_DATA_NODE         * NewNode;
  UINT32                  FreeSpace;
  CONST AML_RD_HEADER   * CurrRdElement;
  UINT32                  CurrRdElementSize;

  // Check that BufferNode is an ObjectNode and has a ByteList.
  if (!AmlNodeHasAttribute (BufferNode, AML_HAS_BYTE_LIST)  ||
      !IS_STREAM (FStream)                                  ||
      IS_END_OF_STREAM (FStream)                            ||
      !IS_STREAM_FORWARD (FStream)) {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  // Iterate through the resource data elements and create nodes.
  // We assume the Buffer has already been validated as a list of
  // resource data elements, so less checks are made.
  while (TRUE) {
    FreeSpace = AmlStreamGetFreeSpace (FStream);
    if (FreeSpace == 0) {
      break;
    }

    CurrRdElement = (CONST AML_RD_HEADER*)AmlStreamGetCurrPos (FStream);
    CurrRdElementSize = AmlRdStreamGetRdSize (FStream);

    Status = AmlCreateDataNode (
               EAmlNodeDataTypeResourceData,
               (CONST UINT8*)CurrRdElement,
               CurrRdElementSize,
               &NewNode
               );
    if (EFI_ERROR (Status)) {
      ASSERT (0);
      return Status;
    }

    Status = AmlVarListAddTailInternal (
               (AML_NODE_HEADER*)BufferNode,
               (AML_NODE_HEADER*)NewNode
               );
    if (EFI_ERROR (Status)) {
      ASSERT (0);
      AmlDeleteTree ((AML_NODE_HEADER*)NewNode);
      return Status;
    }

    Status = AmlStreamProgress (FStream, CurrRdElementSize);
    if (EFI_ERROR (Status)) {
      ASSERT (0);
      return Status;
    }

    AMLDBG_DUMP_RAW (CurrRdElement, CurrRdElementSize);

    // Exit the loop when finding the resource data end tag.
    if (AmlRdCompareDescId (
          CurrRdElement,
          AML_RD_BUILD_SMALL_DESC_ID (ACPI_SMALL_END_TAG_DESCRIPTOR_NAME))) {
      if (FreeSpace != CurrRdElementSize) {
        ASSERT (0);
        return EFI_INVALID_PARAMETER;
      }
      break;
    }
  } // while

  return EFI_SUCCESS;
}
