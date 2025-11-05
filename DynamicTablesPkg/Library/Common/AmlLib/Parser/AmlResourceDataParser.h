/** @file
  AML Resource Data Parser.

  Copyright (c) 2019 - 2020, Arm Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Glossary:
  - Rd or RD   - Resource Data
  - Rds or RDS - Resource Data Small
  - Rdl or RDL - Resource Data Large
**/

#ifndef AML_RESOURCE_DATA_PARSER_H_
#define AML_RESOURCE_DATA_PARSER_H_

#include <AmlNodeDefines.h>
#include <Stream/AmlStream.h>
#include <ResourceData/AmlResourceData.h>

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
  IN  CONST AML_STREAM  *FStream
  );

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
  IN  AML_OBJECT_NODE  *BufferNode,
  IN  AML_STREAM       *FStream
  );

#endif // AML_RESOURCE_DATA_PARSER_H_
