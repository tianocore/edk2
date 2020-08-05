/** @file
  AML Parser.

  Copyright (c) 2019 - 2020, Arm Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef AML_PARSER_H_
#define AML_PARSER_H_

#include <AmlNodeDefines.h>
#include <Stream/AmlStream.h>

/** Parse the list of fixed arguments of the input ObjectNode.

  For each argument, create a node and add it to the fixed argument list
  of the Node.
  If a fixed argument has children, parse them.

  @param  [in]  ObjectNode        Object node to parse the fixed arguments
                                  from.
  @param  [in]  FStream           Forward stream containing the AML
                                  bytecode to parse.
                                  The stream must not be at its end.
  @param  [in]  NameSpaceRefList  List of namespace reference nodes.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_BUFFER_TOO_SMALL    No space left in the buffer.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_OUT_OF_RESOURCES    Could not allocate memory.
**/
EFI_STATUS
EFIAPI
AmlParseFixedArguments (
  IN  AML_OBJECT_NODE   * ObjectNode,
  IN  AML_STREAM        * FStream,
  IN  LIST_ENTRY        * NameSpaceRefList
  );

/** Parse the variable list of arguments of the input ObjectNode.

  For each variable argument, create a node and add it to the variable list of
  arguments of the Node.
  If a variable argument has children, parse them recursively.

  The arguments of method invocation nodes are added to the variable list of
  arguments of the method invocation node. It is necessary to first get
  the number of arguments to parse for this kind of node. A method invocation
  can have at most 7 fixed arguments.

  @param  [in]  Node              Node to parse the variable arguments
                                  from.
  @param  [in]  FStream           Forward stream containing the AML
                                  bytecode to parse.
                                  The stream must not be at its end.
  @param  [in]  NameSpaceRefList  List of namespace reference nodes.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_BUFFER_TOO_SMALL    No space left in the buffer.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_OUT_OF_RESOURCES    Could not allocate memory.
**/
EFI_STATUS
EFIAPI
AmlParseVariableArguments (
  IN  AML_NODE_HEADER   * Node,
  IN  AML_STREAM        * FStream,
  IN  LIST_ENTRY        * NameSpaceRefList
  );

#endif // AML_PARSER_H_
