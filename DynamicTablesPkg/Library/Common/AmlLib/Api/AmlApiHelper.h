/** @file
  AML Helper.

  Copyright (c) 2020, Arm Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef AML_HELPER_H_
#define AML_HELPER_H_

#include <AmlNodeDefines.h>
#include <ResourceData/AmlResourceData.h>

/** Compare the NameString defined by the "Name ()" ASL function,
    and stored in the NameOpNode, with the input NameString.

  An ASL NameString is expected to be NULL terminated, and can be composed
  of NameSegs that have less that 4 chars, like "DEV". "DEV" will be expanded
  as "DEV_".

  An AML NameString is not NULL terminated and is is only composed of
  4 chars long NameSegs.

  @param  [in] NameOpNode   NameOp object node defining a variable.
                            Must have an AML_NAME_OP/0 OpCode/SubOpCode.
                            NameOp object nodes are defined in ASL
                            using the "Name ()" function.
  @param  [in] AslName      ASL NameString to compare the NameOp's name with.
                            Must be NULL terminated.

  @retval TRUE If the AslName and the AmlName defined by the NameOp node
          are similar.
  @retval FALSE Otherwise.
**/
BOOLEAN
EFIAPI
AmlNameOpCompareName (
  IN  AML_OBJECT_NODE_HANDLE  NameOpNode,
  IN  CHAR8                   *AslName
  );

/** Check whether ObjectNode has the input OpCode/SubOpcode couple.

  @param  [in]  ObjectNode  Pointer to an object node.
  @param  [in]  OpCode      OpCode to check
  @param  [in]  SubOpCode   SubOpCode to check

  @retval TRUE    The node is an object node and
                  the Opcode and SubOpCode match.
  @retval FALSE   Otherwise.
**/
BOOLEAN
EFIAPI
AmlNodeHasOpCode (
  IN  AML_OBJECT_NODE_HANDLE  ObjectNode,
  IN  UINT8                   OpCode,
  IN  UINT8                   SubOpCode
  );

/** Check whether DataNode has the input DataType.

  @param  [in]  DataNode   Pointer to a data node.
  @param  [in]  DataType   DataType to check.

  @retval TRUE    The node is a data node and
                  the DataType match.
  @retval FALSE   Otherwise.
**/
BOOLEAN
EFIAPI
AmlNodeHasDataType (
  IN  AML_DATA_NODE_HANDLE  DataNode,
  IN  EAML_NODE_DATA_TYPE   DataType
  );

/** Check whether RdNode has the input RdDataType.

  @param  [in]  RdNode      Pointer to a data node.
  @param  [in]  RdDataType  DataType to check.

  @retval TRUE    The node is a Resource Data node and
                  the RdDataType match.
  @retval FALSE   Otherwise.
**/
BOOLEAN
EFIAPI
AmlNodeHasRdDataType (
  IN  AML_DATA_NODE_HANDLE  RdNode,
  IN  AML_RD_HEADER         RdDataType
  );

#endif // AML_HELPER_H_
