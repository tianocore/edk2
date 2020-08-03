/** @file
  AML Tree Iterator.

  Copyright (c) 2019 - 2020, Arm Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <AmlNodeDefines.h>
#include <Tree/AmlTreeIterator.h>

#include <AmlCoreInterface.h>
#include <Tree/AmlTreeTraversal.h>

/** Iterator to traverse the tree.

  This is an internal structure.
*/
typedef struct AmlTreeInternalIterator {
  /// External iterator structure, containing the external APIs.
  /// Must be the first field.
  AML_TREE_ITERATOR         Iterator;

  // Note: The following members of this structure are opaque to the users
  //       of the Tree iterator APIs.

  /// Pointer to the node on which the iterator has been initialized.
  CONST  AML_NODE_HEADER  * InitialNode;

  /// Pointer to the current node.
  CONST  AML_NODE_HEADER  * CurrentNode;

  /// Iteration mode.
  /// Allow to choose how to traverse the tree/choose which node is next.
  EAML_ITERATOR_MODE        Mode;
} AML_TREE_ITERATOR_INTERNAL;

/** Get the current node of an iterator.

  @param  [in]  Iterator  Pointer to an iterator.
  @param  [out] OutNode   Pointer holding the current node.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
**/
STATIC
EFI_STATUS
EFIAPI
AmlIteratorGetNode (
  IN  AML_TREE_ITERATOR   * Iterator,
  OUT AML_NODE_HEADER    ** OutNode
  )
{
  AML_TREE_ITERATOR_INTERNAL  * InternalIterator;

  InternalIterator = (AML_TREE_ITERATOR_INTERNAL*)Iterator;

  // CurrentNode can be NULL, but InitialNode cannot.
  if ((OutNode == NULL)                                       ||
      (InternalIterator == NULL)                              ||
      (InternalIterator->Mode <= EAmlIteratorUnknown)         ||
      (InternalIterator->Mode >= EAmlIteratorModeMax)         ||
      !IS_AML_NODE_VALID (InternalIterator->InitialNode)      ||
      ((InternalIterator->CurrentNode != NULL)                &&
        !IS_AML_NODE_VALID (InternalIterator->CurrentNode))) {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  *OutNode = (AML_NODE_HEADER*)InternalIterator->CurrentNode;

  return EFI_SUCCESS;
}

/** Move the current node of the iterator to the next node,
    according to the iteration mode selected.

  If NextNode is not NULL, return the next node.

  @param  [in]  Iterator    Pointer to an iterator.
  @param  [out] NextNode    If not NULL, updated to the next node.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
**/
STATIC
EFI_STATUS
EFIAPI
AmlIteratorGetNextLinear (
  IN  AML_TREE_ITERATOR  * Iterator,
  OUT AML_NODE_HEADER   ** NextNode
  )
{
  AML_TREE_ITERATOR_INTERNAL  * InternalIterator;

  InternalIterator = (AML_TREE_ITERATOR_INTERNAL*)Iterator;

  // CurrentNode can be NULL, but InitialNode cannot.
  if ((InternalIterator == NULL)                              ||
      (InternalIterator->Mode != EAmlIteratorLinear)          ||
      !IS_AML_NODE_VALID (InternalIterator->InitialNode)      ||
      !IS_AML_NODE_VALID (InternalIterator->CurrentNode)) {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  // Get the next node according to the iteration mode.
  InternalIterator->CurrentNode = AmlGetNextNode (
                                    InternalIterator->CurrentNode
                                    );

  if (NextNode != NULL) {
    *NextNode = (AML_NODE_HEADER*)InternalIterator->CurrentNode;
  }
  return EFI_SUCCESS;
}

/** Move the current node of the iterator to the previous node,
    according to the iteration mode selected.

  If PrevNode is not NULL, return the previous node.

  @param  [in]  Iterator    Pointer to an iterator.
  @param  [out] PrevNode    If not NULL, updated to the previous node.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
**/
STATIC
EFI_STATUS
EFIAPI
AmlIteratorGetPreviousLinear (
  IN  AML_TREE_ITERATOR  * Iterator,
  OUT AML_NODE_HEADER   ** PrevNode
  )
{
  AML_TREE_ITERATOR_INTERNAL  * InternalIterator;

  InternalIterator = (AML_TREE_ITERATOR_INTERNAL*)Iterator;

  // CurrentNode can be NULL, but InitialNode cannot.
  if ((InternalIterator == NULL)                              ||
      (InternalIterator->Mode != EAmlIteratorLinear)          ||
      !IS_AML_NODE_VALID (InternalIterator->InitialNode)      ||
      !IS_AML_NODE_VALID (InternalIterator->CurrentNode)) {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  // Get the previous node according to the iteration mode.
  InternalIterator->CurrentNode = AmlGetPreviousNode (
                                    InternalIterator->CurrentNode
                                    );
  if (PrevNode != NULL) {
    *PrevNode = (AML_NODE_HEADER*)InternalIterator->CurrentNode;
  }
  return EFI_SUCCESS;
}

/** Move the current node of the iterator to the next node,
    according to the iteration mode selected.

  If NextNode is not NULL, return the next node.

  @param  [in]  Iterator    Pointer to an iterator.
  @param  [out] NextNode    If not NULL, updated to the next node.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
**/
STATIC
EFI_STATUS
EFIAPI
AmlIteratorGetNextBranch (
  IN  AML_TREE_ITERATOR  * Iterator,
  OUT AML_NODE_HEADER   ** NextNode
  )
{
  AML_TREE_ITERATOR_INTERNAL  * InternalIterator;
  AML_NODE_HEADER             * Node;

  InternalIterator = (AML_TREE_ITERATOR_INTERNAL*)Iterator;

  // CurrentNode can be NULL, but InitialNode cannot.
  if ((InternalIterator == NULL)                              ||
      (InternalIterator->Mode != EAmlIteratorBranch)          ||
      !IS_AML_NODE_VALID (InternalIterator->InitialNode)      ||
      !IS_AML_NODE_VALID (InternalIterator->CurrentNode)) {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  Node = AmlGetNextNode (InternalIterator->CurrentNode);
  // Check whether NextNode is a sibling of InitialNode.
  if (AmlGetParent (Node) ==
        AmlGetParent ((AML_NODE_HEADER*)InternalIterator->InitialNode)) {
    Node = NULL;
  }

  InternalIterator->CurrentNode = Node;

  if (NextNode != NULL) {
    *NextNode = Node;
  }
  return EFI_SUCCESS;
}

/** Move the current node of the iterator to the previous node,
    according to the iteration mode selected.

  If PrevNode is not NULL, return the previous node.

  @param  [in]  Iterator    Pointer to an iterator.
  @param  [out] PrevNode    If not NULL, updated to the previous node.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
**/
STATIC
EFI_STATUS
EFIAPI
AmlIteratorGetPreviousBranch (
  IN  AML_TREE_ITERATOR  * Iterator,
  OUT AML_NODE_HEADER   ** PrevNode
  )
{
  AML_TREE_ITERATOR_INTERNAL  * InternalIterator;
  AML_NODE_HEADER             * Node;

  InternalIterator = (AML_TREE_ITERATOR_INTERNAL*)Iterator;

  // CurrentNode can be NULL, but InitialNode cannot.
  if ((InternalIterator == NULL)                              ||
      (InternalIterator->Mode != EAmlIteratorBranch)          ||
      !IS_AML_NODE_VALID (InternalIterator->InitialNode)      ||
      !IS_AML_NODE_VALID (InternalIterator->CurrentNode)) {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  Node = AmlGetPreviousNode (InternalIterator->CurrentNode);
  // Check whether PreviousNode is a sibling of InitialNode.
  if (AmlGetParent (Node) ==
        AmlGetParent ((AML_NODE_HEADER*)InternalIterator->InitialNode)) {
    Node = NULL;
  }

  InternalIterator->CurrentNode = Node;

  if (PrevNode != NULL) {
    *PrevNode = Node;
  }
  return EFI_SUCCESS;
}

/** Initialize an iterator.

  Note: The caller must call AmlDeleteIterator () to free the memory
        allocated for the iterator.

  @param  [in]  Node          Pointer to the node.
  @param  [in]  IteratorMode  Selected mode to traverse the tree.
  @param  [out] IteratorPtr   Pointer holding the created iterator.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_OUT_OF_RESOURCES    Could not allocate memory.
**/
EFI_STATUS
EFIAPI
AmlInitializeIterator (
  IN   AML_NODE_HEADER      * Node,
  IN   EAML_ITERATOR_MODE     IteratorMode,
  OUT  AML_TREE_ITERATOR   ** IteratorPtr
  )
{
  AML_TREE_ITERATOR_INTERNAL * InternalIterator;

  if (!IS_AML_NODE_VALID (Node)             ||
      (IteratorMode <= EAmlIteratorUnknown) ||
      (IteratorMode >= EAmlIteratorModeMax) ||
      (IteratorPtr == NULL)) {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  *IteratorPtr = NULL;
  InternalIterator = (AML_TREE_ITERATOR_INTERNAL*)AllocateZeroPool (
                                                    sizeof (
                                                      AML_TREE_ITERATOR_INTERNAL
                                                      )
                                                    );
  if (InternalIterator == NULL) {
    ASSERT (0);
    return EFI_OUT_OF_RESOURCES;
  }

  InternalIterator->InitialNode = Node;
  InternalIterator->CurrentNode = Node;
  InternalIterator->Mode = IteratorMode;
  InternalIterator->Iterator.GetNode = AmlIteratorGetNode;

  switch (InternalIterator->Mode) {
    case EAmlIteratorLinear:
    {
      InternalIterator->Iterator.GetNext = AmlIteratorGetNextLinear;
      InternalIterator->Iterator.GetPrevious = AmlIteratorGetPreviousLinear;
      break;
    }
    case EAmlIteratorBranch:
    {
      InternalIterator->Iterator.GetNext = AmlIteratorGetNextBranch;
      InternalIterator->Iterator.GetPrevious = AmlIteratorGetPreviousBranch;
      break;
    }
    default:
    {
      ASSERT (0);
      FreePool (InternalIterator);
      return EFI_INVALID_PARAMETER;
    }
  } // switch

  *IteratorPtr = &InternalIterator->Iterator;

  return EFI_SUCCESS;
}

/** Delete an iterator.

  Note: The caller must have first initialized the iterator with the
        AmlInitializeIterator () function.

  @param  [in]  Iterator  Pointer to an iterator.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
**/
EFI_STATUS
EFIAPI
AmlDeleteIterator (
  IN  AML_TREE_ITERATOR   * Iterator
  )
{
  if (Iterator == NULL) {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  FreePool (Iterator);

  return EFI_SUCCESS;
}
