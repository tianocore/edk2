/** @file
  AML Tree Enumerator.

  Copyright (c) 2019 - 2020, Arm Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <AmlNodeDefines.h>

#include <AmlCoreInterface.h>
#include <Tree/AmlTree.h>

/** Enumerate all nodes of the subtree under the input Node in the AML
    bytestream order (i.e. in a depth first order), and call the CallBack
    function with the input Context.
    The prototype of the Callback function is EDKII_AML_TREE_ENUM_CALLBACK.

  @param  [in]      Node      Enumerate nodes of the subtree under this Node.
                              Must be a valid node.
  @param  [in]      CallBack  Callback function to call on each node.
  @param  [in, out] Context   Void pointer used to pass some information
                              to the Callback function.
                              Optional, can be NULL.
  @param  [out]     Status    Optional parameter that can be used to get
                              the status of the Callback function.
                              If used, need to be init to EFI_SUCCESS.

  @retval TRUE if the enumeration can continue or has finished without
          interruption.
  @retval FALSE if the enumeration needs to stopped or has stopped.
**/
BOOLEAN
EFIAPI
AmlEnumTree (
  IN      AML_NODE_HEADER               * Node,
  IN      EDKII_AML_TREE_ENUM_CALLBACK    CallBack,
  IN  OUT VOID                          * Context,  OPTIONAL
      OUT EFI_STATUS                    * Status    OPTIONAL
  )
{
  BOOLEAN               ContinueEnum;

  EAML_PARSE_INDEX      Index;
  EAML_PARSE_INDEX      MaxIndex;

  LIST_ENTRY          * StartLink;
  LIST_ENTRY          * CurrentLink;

  if (!IS_AML_NODE_VALID (Node) || (CallBack == NULL)) {
    ASSERT (0);
    if (Status != NULL) {
      *Status = EFI_INVALID_PARAMETER;
    }
    return FALSE;
  }

  ContinueEnum = (*CallBack)(Node, Context, Status);
  if (ContinueEnum == FALSE) {
    return ContinueEnum;
  }

  // Iterate through the fixed list of arguments.
  MaxIndex = (EAML_PARSE_INDEX)AmlGetFixedArgumentCount (
                                 (AML_OBJECT_NODE*)Node
                                 );
  for (Index = EAmlParseIndexTerm0; Index < MaxIndex; Index++) {
    ContinueEnum = AmlEnumTree (
                     AmlGetFixedArgument ((AML_OBJECT_NODE*)Node, Index),
                     CallBack,
                     Context,
                     Status
                     );
    if (ContinueEnum == FALSE) {
      return ContinueEnum;
    }
  }

  // Iterate through the variable list of arguments.
  StartLink = AmlNodeGetVariableArgList (Node);
  if (StartLink != NULL) {
    CurrentLink = StartLink->ForwardLink;
    while (CurrentLink != StartLink) {
      ContinueEnum = AmlEnumTree (
                       (AML_NODE_HEADER*)CurrentLink,
                       CallBack,
                       Context,
                       Status
                       );
      if (ContinueEnum == FALSE) {
        return ContinueEnum;
      }
      CurrentLink = CurrentLink->ForwardLink;
    } // while
  }

  return ContinueEnum;
}
