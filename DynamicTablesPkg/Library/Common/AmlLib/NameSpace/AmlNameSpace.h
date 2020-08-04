/** @file
  AML NameSpace.

  Copyright (c) 2019 - 2020, Arm Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef AML_NAMESPACE_H_
#define AML_NAMESPACE_H_

#include <AmlNodeDefines.h>
#include <Stream/AmlStream.h>

/** Return the first AML namespace node up in the parent hierarchy.

    Return the root node if no namespace node is found is the hierarchy.

  @param  [in]  Node      Node to look at the parents from.
                          If Node is the root node, OutNode is NULL.
  @param  [out] OutNode   If a namespace node is found, pointer to the
                          first namespace node of Node's parents.
                          Stop at the root node otherwise.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  **/
EFI_STATUS
EFIAPI
AmlGetFirstAncestorNameSpaceNode (
  IN  CONST AML_NODE_HEADER   * Node,
  OUT       AML_NODE_HEADER  ** OutNode
  );

/** Build the raw absolute AML pathname to Node and write it to a stream.

  A raw AML pathname is an AML pathname where the root char ('\'),
  prefix chars ('^') and NameString prefix byte (e.g.: DualNamePrefix)
  have been removed. A raw AML pathname is a list of concatenated
  NameSegs.

  E.g.:
  ASL absolute path:  "[RootChar]AAAA.BBBB.CCCC\0"
  AML absolute path:  "[RootChar][MultiNamePrefix][3(NameSegs)]AAAABBBBCCCC"
  Raw absolute path:  "AAAABBBBCCCC"

  @param  [in]   Node         Node to build the raw absolute path to
                              Must be a root node, or a namespace node.
  @param  [in]  InputParent   Skip InputParent AML namespace levels before
                              starting building the raw absolute pathname.
                              E.g.: - Node's name being "^AAAA.BBBB.CCCC";
                                    - InputParent = 2;
                                    "BBBB.CCCC" will be skipped (2
                                    levels), and "^AAAA" will remain. The
                                    first caret is not related to InputParent.
  @param  [out]  RawAbsPathBStream  Backward stream to write the raw
                                    pathname to.
                                    If Node is the root node, the Stream data
                                    Buffer will stay empty.
                                    The stream must not be at its end.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_BUFFER_TOO_SMALL    No space left in the buffer.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
**/
EFI_STATUS
EFIAPI
AmlGetRawNameSpacePath (
  IN  CONST AML_NODE_HEADER   * Node,
  IN        UINT32              InputParent,
  OUT       AML_STREAM        * RawAbsPathBStream
  );

#endif // AML_NAMESPACE_H_
