/** @file
  AML Iterator.

  Copyright (c) 2019 - 2020, Arm Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef AML_ITERATOR_H_
#define AML_ITERATOR_H_

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

/**
  @defgroup IteratorLibrary Iterator library
  @ingroup NavigationApis
  @{
    The iterator library allow to navigate in the AML tree using an iterator.
    It is possible to initialize/delete an iterator.

    This iterator can progress in the tree by different orders:
     - Linear progression: Iterate following the AML bytestream order
                           (depth first).
     - Branch progression: Iterate following the AML bytestream order
                           (depth first), but stop iterating at the
                           end of the branch.

    An iterator has the following features:
     - GetNode:       Get the current node pointed by the iterator.
     - GetNext:       Move the current node of the iterator to the next
                      node, according to the iteration mode selected.
     - GetPrevious:   Move the current node of the iterator to the previous
                      node, according to the iteration mode selected.
  @}
*/

/**
  @defgroup IteratorApis Iterator APIs
  @ingroup IteratorLibrary
  @{
    Iterator APIs defines the action that can be done on an iterator:
     - Initialization;
     - Deletion;
     - Getting the node currently pointed by the iterator;
     - Moving to the next node;
     - Moving to the previous node.
  @}
*/

/**
  @defgroup IteratorStructures Iterator structures
  @ingroup IteratorLibrary
  @{
    Iterator structures define the enum/define values and structures related
    to iterators.
  @}
*/

/** Iterator mode.

  Modes to choose how the iterator is progressing in the tree.
  A
  \-B    <- Iterator initialized with this node.
  | \-C
  | | \-D
  | \-E
  |   \-F
  |   \-G
  \-H
    \-I

  @ingroup IteratorStructures
*/
typedef enum EAmlIteratorMode {
  EAmlIteratorUnknown,        ///< Unknown/Invalid AML IteratorMode
  EAmlIteratorLinear,         ///< Iterate following the AML bytestream order
                              ///  (depth first).
                              ///  The order followed by the iterator would be:
                              ///  B, C, D, E, F, G, H, I, NULL.
  EAmlIteratorBranch,         ///< Iterate through the node of a branch.
                              ///  The iteration follows the AML bytestream
                              ///  order but within the branch B.
                              ///  The order followed by the iterator would be:
                              ///  B, C, D, E, F, G, NULL.
  EAmlIteratorModeMax         ///< Max enum.
} EAML_ITERATOR_MODE;

/** Iterator.

  Allows to traverse the tree in different orders.

  @ingroup IteratorStructures
*/
typedef struct AmlTreeIterator AML_TREE_ITERATOR;

/** Function pointer to a get the current node of the iterator.

  @ingroup IteratorApis

  @param  [in]  Iterator  Pointer to an iterator.
  @param  [out] OutNode   Pointer holding the current node.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
*/
typedef
EFI_STATUS
(EFIAPI * EDKII_AML_TREE_ITERATOR_GET_NODE) (
  IN  AML_TREE_ITERATOR  * Iterator,
  OUT AML_NODE_HANDLE    * OutNode
  );

/** Function pointer to move the current node of the iterator to the
    next node, according to the iteration mode selected.

  If NextNode is not NULL, return the next node.

  @ingroup IteratorApis

  @param  [in]  Iterator    Pointer to an iterator.
  @param  [out] NextNode    If not NULL, updated to the next node.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
*/
typedef
EFI_STATUS
(EFIAPI * EDKII_AML_TREE_ITERATOR_GET_NEXT) (
  IN  AML_TREE_ITERATOR  * Iterator,
  OUT AML_NODE_HANDLE    * NextNode
  );

/** Function pointer to move the current node of the iterator to the
    previous node, according to the iteration mode selected.

  If PrevNode is not NULL, return the previous node.

  @ingroup IteratorApis

  @param  [in]  Iterator    Pointer to an iterator.
  @param  [out] PrevNode    If not NULL, updated to the previous node.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
*/
typedef
EFI_STATUS
(EFIAPI * EDKII_AML_TREE_ITERATOR_GET_PREVIOUS) (
  IN  AML_TREE_ITERATOR  * Iterator,
  OUT AML_NODE_HANDLE    * PrevNode
  );

/**  Iterator structure to traverse the tree.

  @ingroup IteratorStructures
*/
typedef struct AmlTreeIterator {
  /// Get the current node of the iterator.
  EDKII_AML_TREE_ITERATOR_GET_NODE      GetNode;

  /// Update the current node of the iterator with the next node.
  EDKII_AML_TREE_ITERATOR_GET_NEXT      GetNext;

  /// Update the current node of the iterator with the previous node.
  EDKII_AML_TREE_ITERATOR_GET_PREVIOUS  GetPrevious;
} AML_TREE_ITERATOR;


/** Initialize an iterator.

  Note: The caller must call AmlDeleteIterator () to free the memory
        allocated for the iterator.

  @ingroup IteratorApis

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
  IN   AML_NODE_HANDLE        Node,
  IN   EAML_ITERATOR_MODE     IteratorMode,
  OUT  AML_TREE_ITERATOR   ** IteratorPtr
  );

/** Delete an iterator.

  Note: The caller must have first initialized the iterator with the
        AmlInitializeIterator () function.

  @ingroup IteratorApis

  @param  [in]  Iterator  Pointer to an iterator.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
**/
EFI_STATUS
EFIAPI
AmlDeleteIterator (
  IN  AML_TREE_ITERATOR   * Iterator
  );

#endif // AML_ITERATOR_H_
