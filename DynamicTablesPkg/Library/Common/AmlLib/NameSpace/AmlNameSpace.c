/** @file
  AML NameSpace.

  Copyright (c) 2019 - 2021, Arm Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

/* Lexicon:

  NameSeg:
  - An ASL NameSeg is a name made of at most 4 chars.
    Cf. ACPI 6.3 specification, s19.2.2 'Name and Pathname Terms'.
  - An AML NameSeg is a name made of 4 chars.
    Cf. ACPI 6.3 specification, s20.2.2 'Name Objects Encoding'.

  NameString:
  A NameString is analogous to a pathname. It is made of 0 to 255 NameSegs.
  A NameString can be prefixed by a root char ('\') or 0 to 255 carets ('^').

  A NameString can be ASL or AML encoded.
  AML NameStrings can have a NameString prefix (dual or multi-name prefix)
  between the root/carets and the list of NameSegs. If the prefix is the
  multi-name prefix, then the number of NameSegs is encoded on one single byte.
  Cf. ACPI 6.3 specification, s19.2.2 'Name and Pathname Terms'.
  Cf. ACPI 6.3 specification, s20.2.2 'Name Objects Encoding'.

  Namespace level:
  One level in the AML Namespace level corresponds to one NameSeg. In ASL,
  objects names are NameStrings. This means a device can have a name which
  spans multiple levels.
  E.g.: The ASL code: Device (CLU0.CPU0) corresponds to 2 levels.

  Namespace node:
  A namespace node is an object node which has an associated name, and which
  changes the current scope.
  E.g.:
    1. The "Device ()" ASL statement adds a name to the AML namespace and
       changes the current scope to the device scope, this is a namespace node.
    2. The "Scope ()" ASL statement changes the current scope, this is a
       namespace node.
    3. A method invocation has a name, but does not add nor change the current
       AML scope. This is not a namespace node.

  - Object nodes with the AML_IN_NAMESPACE attribute are namespace nodes.
    Buffers (), Packages (), etc. are not part of the namespace. It is however
    possible to associate them with a name with the Name () ASL statement.
  - The root node is considered as being part of the namespace.
  - Some resource data elements can have a name when defining them in
    an ASL statement. However, this name is stripped by the ASL compiler.
    Thus, they don't have a name in the AML bytestream, and are therefore
    not part of the AML namespace.
  - Field list elements are part of the namespace.
    Fields created by an CreateXXXField () ASL statement are part of the
    namespace. The name of these node can be found in the third or fourth
    fixed argument. The exact index of the name can be found in the NameIndex
    field of the AML_BYTE_ENCODING array.
    Field are at the same level as their ASL statement in the namespace.
    E.g:
    Scope (\) {
      OperationRegion (REG0, SystemIO, 0x100, 0x100)
      Field (REG0, ByteAcc, NoLock, Preserve) {
        FIE0, 1,
        FIE1, 5
      }

      Name (BUF0, Buffer (100) {})
      CreateField (BUF0, 5, 2, MEM0)
    }

    produces this namespace:
    \ (Root)
    \-REG0
    \-FIE0
    \-FIE1
    \-BUF0
    \-MEM0

  Raw AML pathname or Raw AML NameString:
  In order to easily manipulate AML NameStrings, the non-NameSegs chars are
  removed in raw pathnames/NameStrings. Non-NameSegs chars are the
  root char ('\'), carets ('^') and NameString prefixes (Dual/Multi name char).
  E.g. The following terminology is defined in this AML Library.
  ASL absolute path:  "[RootChar]AAAA.BBBB.CCCC\0"
  AML absolute path:  "[RootChar][MultiNamePrefix][3(NameSegs)]AAAABBBBCCCC"
  Raw absolute path:  "AAAABBBBCCCC"

  Multi-name:
  A NameString with at least 2 NameSegs. A node can have a name which spans
  multiple namespace levels.
*/

#include <NameSpace/AmlNameSpace.h>

#include <AmlCoreInterface.h>
#include <AmlDbgPrint/AmlDbgPrint.h>
#include <String/AmlString.h>
#include <Tree/AmlNode.h>
#include <Tree/AmlTree.h>
#include <Tree/AmlTreeTraversal.h>

/** Context of the path search callback function.

  The function finding a node from a path and a reference node enumerates
  the namespace nodes in the tree and compares their absolute path with the
  searched path. The enumeration function uses a callback function that can
  receive a context.
  This structure is used to store the context information required in the
  callback function.
*/
typedef struct AmlPathSearchContext {
  /// Backward stream holding the raw AML absolute searched path.
  AML_STREAM        * SearchPathBStream;

  /// An empty backward stream holding a pre-allocated buffer. This prevents
  /// from having to do multiple allocations during the search.
  /// This stream is used to query the raw AML absolute path of the node
  /// currently being probed.
  AML_STREAM        * CurrNodePathBStream;

  /// If the node being visited is the node being searched,
  /// i.e. its path and the searched path match,
  /// save its reference in this pointer.
  AML_NODE_HEADER   * OutNode;
} AML_PATH_SEARCH_CONTEXT;

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
  )
{
  if (!IS_AML_NODE_VALID (Node) ||
      (OutNode == NULL)) {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  // If Node is the root node, return NULL.
  if (IS_AML_ROOT_NODE (Node)) {
    *OutNode = NULL;
    return EFI_SUCCESS;
  } else {
    // Else, get the parent node.
    Node = AmlGetParent ((AML_NODE_HEADER*)Node);
    if (!IS_AML_NODE_VALID (Node)) {
      ASSERT (0);
      return EFI_INVALID_PARAMETER;
    }
  }

  // Continue getting the parent node while no namespace node is encountered.
  while (TRUE) {
    if (IS_AML_ROOT_NODE (Node)) {
      break;
    } else if (AmlNodeHasAttribute (
                 (CONST AML_OBJECT_NODE*)Node,
                 AML_IN_NAMESPACE
                 )) {
      break;
    } else {
      Node = AmlGetParent ((AML_NODE_HEADER*)Node);
      if (!IS_AML_NODE_VALID (Node)) {
        ASSERT (0);
        return EFI_INVALID_PARAMETER;
      }
    }
  } // while

  *OutNode = (AML_NODE_HEADER*)Node;
  return EFI_SUCCESS;
}

/** Climb up the AML namespace hierarchy.

  This function get the ancestor namespace node in the AML namespace.
  If Levels is not zero, skip Levels namespace nodes in the AML namespace.
  If Levels is zero, return the first ancestor namespace node.
    I.e. if Levels = n, this function returns the (n + 1) ancestor.

  @param  [in] Node         Pointer to an object node.
  @param  [in, out] Levels  Pointer holding a number of AML namespace levels:
                             - At entry, the number of levels to go up in
                               the AML namespace;
                             - At exit, the number of levels that still need
                               to be climbed in case of a multi-named node.
                               Indeed, if a node with a multi-name is found,
                               and Levels is less than the number of NameSegs
                               in this name, then the function returns with
                               the number of levels that still need to be
                               climbed.
                               E.g.: If the first ancestor node's name is
                                     "AAAA.BBBB.CCCC" and
                                     Levels = 2  -> i.e go up 3 levels
                                      \
                                      ...
                                      \-"AAAA.BBBB.CCCC"    <----- OutNode
                                         \-"DDDD"           <----- Node (Input)

                                     The function should ideally return a node
                                     with the name "AAAA". However, it is not
                                     possible to split the node name
                                     "AAAA.BBBB.CCCC" to "AAAA".
                                     Thus, OutNode is set to the input node,
                                     and Levels = 2.
                               In most cases the number of levels to climb
                               correspond to non multi-name node, and therefore
                               Levels = 0 at exit.
  @param  [out] HasRoot     The returned node in OutNode has an AML absolute
                            name, starting with a root char ('\'), or if OutNode
                            is the root node.
  @param  [out] OutNode     The Levels+1 namespace ancestor of the input node in
                            the AML namespace. Must be the root node or a
                            namespace node.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
**/
STATIC
EFI_STATUS
EFIAPI
AmlGetAncestorNameSpaceNode (
  IN      CONST AML_OBJECT_NODE   * Node,
  IN OUT        UINT32            * Levels,
     OUT        UINT32            * HasRoot,
     OUT  CONST AML_NODE_HEADER  ** OutNode
  )
{
  EFI_STATUS                Status;

  CONST AML_NODE_HEADER   * NameSpaceNode;
  CHAR8                   * NodeName;
  UINT32                    ParentCnt;

  UINT32                    Root;
  UINT32                    ParentPrefix;
  UINT32                    SegCount;

  if (!IS_AML_OBJECT_NODE (Node)    ||
      (Levels == NULL)              ||
      (HasRoot == NULL)             ||
      (OutNode == NULL)) {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  ParentCnt = *Levels;
  *HasRoot = 0;

  // ParentCnt namespace levels need to be climbed.
  do {
    // Get the next namespace node in the hierarchy.
    Status = AmlGetFirstAncestorNameSpaceNode (
               (CONST AML_NODE_HEADER*)Node,
               (AML_NODE_HEADER**)&NameSpaceNode
               );
    if (EFI_ERROR (Status)) {
      ASSERT (0);
      return Status;
    }

    Node = (CONST AML_OBJECT_NODE*)NameSpaceNode;

    if (IS_AML_ROOT_NODE (Node)) {
      // Node is the root node. It is not possible to go beyond.
      if (ParentCnt != 0) {
        ASSERT (0);
        return EFI_INVALID_PARAMETER;
      }
      *HasRoot = 1;
      break;
    }

    NodeName = AmlNodeGetName ((CONST AML_OBJECT_NODE*)Node);
    if (NodeName == NULL) {
      ASSERT (0);
      return EFI_INVALID_PARAMETER;
    }

    // Analyze the node name.
    Status = AmlParseNameStringInfo (
                NodeName,
                &Root,
                &ParentPrefix,
                &SegCount
                );
    if (EFI_ERROR (Status)) {
      ASSERT (0);
      return Status;
    }

    if (Root != 0) {
      // NodeName is an absolute pathname.
      *HasRoot = Root;

      // If the node has Root then it cannot have ParentPrefixes (Carets).
      if (ParentPrefix != 0) {
        ASSERT (0);
        return EFI_INVALID_PARAMETER;
      }

      if (SegCount == ParentCnt) {
        // There are exactly enough AML namespace levels to consume.
        // This means the root node was the searched node.
        Node = (CONST AML_OBJECT_NODE*)AmlGetRootNode (
                                         (CONST AML_NODE_HEADER*)Node
                                         );
        if (!IS_AML_ROOT_NODE (Node)) {
          ASSERT (0);
          return EFI_INVALID_PARAMETER;
        }

        ParentCnt = 0;
        break;
      } else if (ParentCnt < SegCount) {
        // There are too many AML namespace levels in this name.
        // ParentCnt has the right value, just return.
        break;
      } else {
        // ParentCnt > SegCount
        // Return error as there must be at least ParentCnt AML namespace
        // levels left in the absolute path.
        ASSERT (0);
        return EFI_INVALID_PARAMETER;
      }
    } else {
      // Root is 0.
      if (ParentCnt < SegCount) {
        // NodeName is a relative path.
        // NodeName has enough levels to consume all the ParentCnt.
        // Exit.
        break;
      } else if (SegCount == ParentCnt) {
        // There are exactly enough AML namespace levels to consume.
        if (ParentPrefix == 0) {
          // The node name doesn't have any carets. Get the next namespace
          // node and return.
          Status = AmlGetFirstAncestorNameSpaceNode (
                     (CONST AML_NODE_HEADER*)Node,
                     (AML_NODE_HEADER**)&NameSpaceNode
                     );
          if (EFI_ERROR (Status)) {
            ASSERT (0);
            return Status;
          }
          Node = (CONST AML_OBJECT_NODE*)NameSpaceNode;
          ParentCnt = 0;
          break;
        } else {
          // The node name has carets. Need to continue climbing the
          // AML namespace.
          ParentCnt = ParentPrefix;
        }
      } else {
        // ParentCnt > SegCount
        // NodeName doesn't have enough levels to consume all the ParentCnt.
        // Update ParentCnt: Consume SegCount levels and add ParentPrefix
        // levels. Continue climbing the tree.
        ParentCnt = ParentCnt + ParentPrefix - SegCount;
      }
    }
  } while (ParentCnt != 0);

  *OutNode = (CONST AML_NODE_HEADER*)Node;
  *Levels = ParentCnt;

  return EFI_SUCCESS;
}

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
  )
{
  EFI_STATUS          Status;

  AML_NODE_HEADER   * ParentNode;
  CHAR8             * NodeName;

  UINT32              Root;
  UINT32              ParentPrefix;
  UINT32              SegCount;
  CONST   CHAR8     * NameSeg;

  if ((!IS_AML_ROOT_NODE (Node)                 &&
       !AmlNodeHasAttribute (
         (CONST AML_OBJECT_NODE*)Node,
         AML_IN_NAMESPACE))                     ||
      !IS_STREAM (RawAbsPathBStream)            ||
      IS_END_OF_STREAM (RawAbsPathBStream)      ||
      !IS_STREAM_BACKWARD (RawAbsPathBStream)   ||
      (InputParent > MAX_UINT8)) {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  while (1) {
    if (IS_AML_ROOT_NODE (Node)) {
      break;
    }

    NodeName = AmlNodeGetName ((CONST AML_OBJECT_NODE*)Node);
    if (NodeName == NULL) {
      ASSERT (0);
      return EFI_INVALID_PARAMETER;
    }

    Status = AmlParseNameStringInfo (
               NodeName,
               &Root,
               &ParentPrefix,
               &SegCount
               );
    if (EFI_ERROR (Status)) {
      ASSERT (0);
      return Status;
    }

    if (SegCount > InputParent) {
      // 1.1. If the Node's name has enough levels to consume all the
      //      InputParent carets, write the levels that are left.
      NameSeg = AmlGetFirstNameSeg (NodeName, Root, ParentPrefix);
      Status = AmlStreamWrite (
                  RawAbsPathBStream,
                  (CONST UINT8*)NameSeg,
                  (SegCount - InputParent) * AML_NAME_SEG_SIZE
                  );
      if (EFI_ERROR (Status)) {
        ASSERT (0);
        return Status;
      }
      InputParent = 0;
    } else {
      // (SegCount <= InputParent)
      // 1.2. Else save the InputParent in TotalParent to climb
      //      them later.
      InputParent -= SegCount;
    }

    InputParent += ParentPrefix;

    if (Root != 0) {
    // 2. The Node's name is an absolute path.
    //    Exit, the root has been reached.
      if (InputParent != 0) {
        ASSERT (0);
        return EFI_NOT_FOUND;
      }
      break;
    }

    Status = AmlGetAncestorNameSpaceNode (
               (CONST AML_OBJECT_NODE*)Node,
               &InputParent,
               &Root,
               (CONST AML_NODE_HEADER**)&ParentNode
               );
    if (EFI_ERROR (Status)  ||
        (!IS_AML_NODE_VALID (ParentNode))) {
      ASSERT (0);
      return Status;
    }

    Node = ParentNode;

    if (IS_AML_ROOT_NODE (Node)) {
      // 3.1. If the root node has been found while climbing,
      //      no need to write NameSegs.
      //      Exit.
      break;
    } else if (Root != 0) {
      // 3.2. An absolute path has been found while climbing the tree.
      //      If (InputParent != 0), the raw pathname is not the root.
      //      Write the first [SegCount - InputParent] NameSegs of this
      //      absolute path.
      //      Then exit.
      if (InputParent != 0) {
        // Get the absolute pathname.
        NodeName = AmlNodeGetName ((CONST AML_OBJECT_NODE*)Node);
        if (NodeName == NULL) {
          ASSERT (0);
          return EFI_INVALID_PARAMETER;
        }

        // Analyze the absolute pathname.
        Status = AmlParseNameStringInfo (
                    NodeName,
                    &Root,
                    &ParentPrefix,
                    &SegCount
                    );
        if (EFI_ERROR (Status)) {
          ASSERT (0);
          return Status;
        }

        // Writing the n first NameSegs.
        // n = SegCount - InputParent
        NameSeg = AmlGetFirstNameSeg (NodeName, Root, ParentPrefix);
        Status = AmlStreamWrite (
                    RawAbsPathBStream,
                    (CONST UINT8*)NameSeg,
                    (SegCount - InputParent) * AML_NAME_SEG_SIZE
                    );
        if (EFI_ERROR (Status)) {
          ASSERT (0);
          return Status;
        }

        break;
      } // (InputParent != 0)

    }
  } // while

  return EFI_SUCCESS;
}

/** Add the RootChar and prefix byte to the raw AML NameString in the
    input Stream to create a valid absolute path.

  The prefix byte can be AML_DUAL_NAME_PREFIX, AML_MULTI_NAME_PREFIX
  or nothing.

  @param  [in, out] AmlPathBStream  The Stream initially contains a raw
                                    NameString (i.e. a list of NameSegs).
                                    The Stream can be empty (e.g.: for the
                                    root path).
                                    The stream must not be at its end.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_BUFFER_TOO_SMALL    No space left in the buffer.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
**/
STATIC
EFI_STATUS
EFIAPI
AmlAddPrefix (
  IN  OUT AML_STREAM    * AmlPathBStream
  )
{
  EFI_STATUS    Status;
  UINT32        NameSegCount;
  UINT32        NameSegSize;

  // At most 3 bytes are needed for: RootChar + MultiNamePrefix + SegCount.
  CHAR8         Prefix[3];
  UINT32        PrefixSize;

  // The Stream contains concatenated NameSegs.
  if (!IS_STREAM (AmlPathBStream)       ||
      IS_END_OF_STREAM (AmlPathBStream) ||
      !IS_STREAM_BACKWARD (AmlPathBStream)) {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  // Its size should be a multiple of AML_NAME_SEG_SIZE.
  // AML_NAME_SEG_SIZE = 4. Check the 2 lowest bits.
  NameSegSize = AmlStreamGetIndex (AmlPathBStream);
  if ((NameSegSize & (AML_NAME_SEG_SIZE - 1)) != 0) {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  // Each NameSeg is 4 bytes so divide the NameSegSize by 4.
  NameSegCount = NameSegSize >> 2;
  if (NameSegCount > MAX_UINT8) {
    // There can be at most 255 NameSegs.
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  Prefix[0] = AML_ROOT_CHAR;

  switch (NameSegCount) {
    case 0:
    {
      // Root and parents only NameString (no NameSeg(s)) end with '\0'.
      Prefix[1] = AML_ZERO_OP;
      PrefixSize = 2;
      break;
    }
    case 1:
    {
      PrefixSize = 1;
      break;
    }
    case 2:
    {
      Prefix[1] = AML_DUAL_NAME_PREFIX;
      PrefixSize = 2;
      break;
    }
    default:
    {
      Prefix[1] = AML_MULTI_NAME_PREFIX;
      Prefix[2] = (UINT8)NameSegCount;
      PrefixSize = 3;
      break;
    }
  }

  // Add the RootChar + prefix (if needed) at the beginning of the pathname.
  Status = AmlStreamWrite (AmlPathBStream, (CONST UINT8*)Prefix, PrefixSize);
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    return Status;
  }

  return Status;
}

/** Remove the prefix bytes of an AML NameString stored in a backward stream
    to get a raw NameString.

  The AML encoding for '\', '^', Dual name or multi-name prefix are
  stripped off.
  E.g: If the ASL path was "\AAAA.BBBB", the AML equivalent would be
       "{RootChar}{DualNamePrefix}AAAABBBB". So resultant raw NameString
       is "AAAABBBB".

  @param  [in, out] AmlPathBStream    Backward stream containing an AML
                                      NameString.
                                      The stream must not be at its end.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
**/
STATIC
EFI_STATUS
EFIAPI
AmlRemovePrefix (
  IN  OUT AML_STREAM  * AmlPathBStream
  )
{
  EFI_STATUS    Status;

  UINT32        TotalSize;
  UINT32        RewindSize;

  UINT32        Root;
  UINT32        ParentPrefix;
  UINT32        SegCount;

  if (!IS_STREAM (AmlPathBStream)         ||
      IS_END_OF_STREAM (AmlPathBStream)   ||
      !IS_STREAM_BACKWARD (AmlPathBStream)) {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  Status = AmlParseNameStringInfo (
             (CHAR8*)AmlStreamGetCurrPos (AmlPathBStream),
             &Root,
             &ParentPrefix,
             &SegCount
             );
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    return Status;
  }

  TotalSize = AmlComputeNameStringSize (Root, ParentPrefix, SegCount);
  if (TotalSize == 0) {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  // Rewind the stream of all the bytes that are not SegCounts
  // to drop the prefix.
  RewindSize = TotalSize - (SegCount * AML_NAME_SEG_SIZE);
  if (RewindSize != 0) {
    Status = AmlStreamRewind (AmlPathBStream, RewindSize);
    if (EFI_ERROR (Status)) {
      ASSERT (0);
      return Status;
    }
  }

  return EFI_SUCCESS;
}

/** Build the absolute ASL pathname to Node.

  BufferSize is always updated to the size of the pathname.

  If:
   - the content of BufferSize is >= to the size of the pathname AND;
   - Buffer is not NULL.
  then copy the pathname in the Buffer. A buffer of the size
  MAX_ASL_NAMESTRING_SIZE is big enough to receive any ASL pathname.

  @param  [in]      Node            Node to build the absolute path to.
                                    Must be a root node, or a namespace node.
  @param  [out]     Buffer          Buffer to write the path to.
                                    If NULL, only update *BufferSize.
  @param  [in, out] BufferSize      Pointer holding:
                                    - At entry, the size of the Buffer;
                                    - At exit, the size of the pathname.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_BUFFER_TOO_SMALL    No space left in the buffer.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_OUT_OF_RESOURCES    Out of memory.
**/
EFI_STATUS
EFIAPI
AmlGetAslPathName (
  IN      AML_NODE_HEADER   * Node,
      OUT CHAR8             * Buffer,
  IN  OUT UINT32            * BufferSize
  )
{
  EFI_STATUS      Status;

  // Backward stream used to build the raw AML absolute path to the node.
  AML_STREAM      RawAmlAbsPathBStream;
  CHAR8         * RawAmlAbsPathBuffer;
  UINT32          RawAmlAbsPathBufferSize;

  CHAR8         * AmlPathName;
  CHAR8         * AslPathName;
  UINT32          AslPathNameSize;

  UINT32          Root;
  UINT32          ParentPrefix;
  UINT32          SegCount;

  if ((!IS_AML_ROOT_NODE (Node)         &&
       !AmlNodeHasAttribute (
         (CONST AML_OBJECT_NODE*)Node,
         AML_IN_NAMESPACE))             ||
      (BufferSize == NULL)) {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  AslPathName = NULL;

  // Allocate a Stream to get the raw AML absolute pathname.
  RawAmlAbsPathBufferSize = MAX_AML_NAMESTRING_SIZE;
  RawAmlAbsPathBuffer = AllocateZeroPool (RawAmlAbsPathBufferSize);
  if (RawAmlAbsPathBuffer == NULL) {
    ASSERT (0);
    return EFI_OUT_OF_RESOURCES;
  }

  Status = AmlStreamInit (
             &RawAmlAbsPathBStream,
             (UINT8*)RawAmlAbsPathBuffer,
             RawAmlAbsPathBufferSize,
             EAmlStreamDirectionBackward
             );
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    goto exit_handler;
  }

  // Get the raw pathname of the Node. The raw pathname being an
  // AML NameString without the RootChar and prefix byte.
  // It is a list of concatenated NameSegs.
  Status = AmlGetRawNameSpacePath (Node, 0, &RawAmlAbsPathBStream);
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    goto exit_handler;
  }

  // Add the RootChar and prefix byte.
  Status = AmlAddPrefix (&RawAmlAbsPathBStream);
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    goto exit_handler;
  }

  AmlPathName = (CHAR8*)AmlStreamGetCurrPos (&RawAmlAbsPathBStream);

  // Analyze the NameString.
  Status = AmlParseNameStringInfo (
             (CONST CHAR8*)AmlPathName,
             &Root,
             &ParentPrefix,
             &SegCount
             );
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    goto exit_handler;
  }

  // Compute the size the ASL pathname will take.
  AslPathNameSize = AslComputeNameStringSize (Root, ParentPrefix, SegCount);
  if (AslPathNameSize == 0) {
    ASSERT (0);
    Status = EFI_INVALID_PARAMETER;
    goto exit_handler;
  }

  // Input Buffer is large enough. Copy the pathname if the Buffer is valid.
  if ((Buffer != NULL) && (AslPathNameSize <= *BufferSize)) {
    Status = ConvertAmlNameToAslName (AmlPathName, &AslPathName);
    if (EFI_ERROR (Status)) {
      ASSERT (0);
      Status = EFI_OUT_OF_RESOURCES;
      goto exit_handler;
    }

    CopyMem (Buffer, AslPathName, AslPathNameSize);
  }

  *BufferSize = AslPathNameSize;

exit_handler:
  // Free allocated memory.
  FreePool (RawAmlAbsPathBuffer);
  if (AslPathName != NULL) {
    FreePool (AslPathName);
  }

  return Status;
}

#if !defined (MDEPKG_NDEBUG)

/** Recursively print the pathnames in the AML namespace in Node's branch.

  @param  [in]  Node          Pointer to a node.
  @param  [in]  Context       An empty forward stream holding a pre-allocated
                              buffer. This prevents from having to do multiple
                              allocations during the enumeration.
  @param  [in, out] Status    At entry, contains the status returned by the
                              last call to this exact function during the
                              enumeration.
                              As exit, contains the returned status of the
                              call to this function.
                              Optional, can be NULL.

  @retval TRUE if the enumeration can continue or has finished without
          interruption.
  @retval FALSE if the enumeration needs to stopped or has stopped.
**/
STATIC
BOOLEAN
EFIAPI
AmlDbgPrintNameSpaceCallback (
  IN      AML_NODE_HEADER  * Node,
  IN      VOID             * Context,
  IN  OUT EFI_STATUS       * Status   OPTIONAL
  )
{
  BOOLEAN           ContinueEnum;
  EFI_STATUS        Status1;

  AML_STREAM      * CurrNodePathFStream;
  CHAR8           * CurrNodePathBuffer;
  UINT32            CurrNodePathBufferSize;

  ContinueEnum = TRUE;
  Status1 = EFI_SUCCESS;

  if (!IS_AML_NODE_VALID (Node) ||
      (Context == NULL)) {
    ASSERT (0);
    Status1 = EFI_INVALID_PARAMETER;
    ContinueEnum = FALSE;
    goto exit_handler;
  }

  if (!IS_AML_ROOT_NODE (Node)  &&
      !AmlNodeHasAttribute (
         (CONST AML_OBJECT_NODE*)Node,
         AML_IN_NAMESPACE)) {
    // Skip this node and continue enumeration.
    goto exit_handler;
  }

  if (IS_AML_ROOT_NODE (Node)) {
    DEBUG ((DEBUG_INFO, "\\\n"));
  } else if (AmlNodeHasAttribute (
               (CONST AML_OBJECT_NODE*)Node,
               AML_IN_NAMESPACE)) {

  CurrNodePathFStream = (AML_STREAM*)Context;

  // Check the Context's content.
  if (!IS_STREAM (CurrNodePathFStream)           ||
      IS_END_OF_STREAM (CurrNodePathFStream)     ||
      !IS_STREAM_FORWARD (CurrNodePathFStream)) {
    ASSERT (0);
    Status1 = EFI_INVALID_PARAMETER;
    ContinueEnum = FALSE;
    goto exit_handler;
  }

  CurrNodePathBuffer = (CHAR8*)AmlStreamGetBuffer (CurrNodePathFStream);
  CurrNodePathBufferSize = AmlStreamGetMaxBufferSize (CurrNodePathFStream);

  Status1 = AmlGetAslPathName (
              (AML_NODE_HEADER*)Node,
              CurrNodePathBuffer,
              &CurrNodePathBufferSize
              );
  if (EFI_ERROR (Status1)) {
    ASSERT (0);
    ContinueEnum = FALSE;
    goto exit_handler;
  }

  DEBUG ((DEBUG_INFO, "%a\n", CurrNodePathBuffer));

  } else {
    ASSERT (0);
    Status1 = EFI_INVALID_PARAMETER;
    ContinueEnum = FALSE;
  }

exit_handler:
  if (Status != NULL) {
    *Status = Status1;
  }

  return ContinueEnum;
}

/** Print the absolute pathnames in the AML namespace of
    all the nodes in the tree starting from the Root node.

  @param  [in]  RootNode    Pointer to a root node.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_BUFFER_TOO_SMALL    No space left in the buffer.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_OUT_OF_RESOURCES    Out of memory.
**/
EFI_STATUS
EFIAPI
AmlDbgPrintNameSpace (
  IN  AML_ROOT_NODE   * RootNode
  )
{
  EFI_STATUS      Status;

  AML_STREAM      CurrNodePathFStream;
  CHAR8         * CurrNodePathBuffer;
  UINT32          CurrNodePathBufferSize;

  if (!IS_AML_ROOT_NODE (RootNode)) {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  DEBUG ((DEBUG_INFO, "AmlNameSpace: AML namespace:\n"));

  // Allocate memory to build the absolute ASL path to each node.
  CurrNodePathBufferSize = MAX_AML_NAMESTRING_SIZE;
  CurrNodePathBuffer = AllocateZeroPool (CurrNodePathBufferSize);
  if (CurrNodePathBuffer == NULL) {
    ASSERT (0);
    return EFI_OUT_OF_RESOURCES;
  }

  // An empty forward stream holding a pre-allocated buffer is used
  // to avoid multiple allocations during the enumeration.
  Status = AmlStreamInit (
             &CurrNodePathFStream,
             (UINT8*)CurrNodePathBuffer,
             CurrNodePathBufferSize,
             EAmlStreamDirectionForward
             );
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    goto exit_handler;
  }

  AmlEnumTree (
    (AML_NODE_HEADER*)RootNode,
    AmlDbgPrintNameSpaceCallback,
    (VOID*)&CurrNodePathFStream,
    &Status
    );
  ASSERT_EFI_ERROR (Status);

exit_handler:
  FreePool (CurrNodePathBuffer);

  return Status;
}

#endif // MDEPKG_NDEBUG

/** Callback function to find the node corresponding to an absolute pathname.

  For each namespace node, build its raw AML absolute path. Then compare this
  path with the raw AML absolute path of the search node available in the
  Context.

  @param  [in]      Node      Pointer to the node to whose pathname is being
                              tested.
  @param  [in, out] Context   A pointer to AML_PATH_SEARCH_CONTEXT that has:
                               - The searched path stored in a stream;
                               - An empty stream to query the pathname of the
                                 probed node;
                               - A node pointer to store the searched node
                                 if found.
  @param  [in, out] Status    At entry, contains the status returned by the
                              last call to this exact function during the
                              enumeration.
                              As exit, contains the returned status of the
                              call to this function.
                              Optional, can be NULL.

  @retval TRUE if the enumeration can continue or has finished without
          interruption.
  @retval FALSE if the enumeration needs to stopped or has stopped.
**/
STATIC
BOOLEAN
EFIAPI
AmlEnumeratePathCallback (
  IN      AML_NODE_HEADER  * Node,
  IN  OUT VOID             * Context,
  IN  OUT EFI_STATUS       * Status   OPTIONAL
)
{
  BOOLEAN                     ContinueEnum;
  EFI_STATUS                  Status1;

  AML_PATH_SEARCH_CONTEXT   * PathSearchContext;

  AML_STREAM                * SearchPathBStream;

  AML_STREAM                * CurrNodePathBStream;
  UINT32                      CurrNodePathSize;

  ContinueEnum = TRUE;
  Status1 = EFI_SUCCESS;

  if (!IS_AML_NODE_VALID (Node) ||
      (Context == NULL)) {
    ASSERT (0);
    Status1 = EFI_INVALID_PARAMETER;
    ContinueEnum = FALSE;
    goto exit_handler;
  }

  if (!AmlNodeHasAttribute (
         (CONST AML_OBJECT_NODE*)Node,
         AML_IN_NAMESPACE)) {
    goto exit_handler;
  }

  PathSearchContext = (AML_PATH_SEARCH_CONTEXT*)Context;
  SearchPathBStream = PathSearchContext->SearchPathBStream;
  CurrNodePathBStream = PathSearchContext->CurrNodePathBStream;

  // Check the Context's content.
  if (!IS_STREAM (SearchPathBStream)            ||
      IS_END_OF_STREAM (SearchPathBStream)      ||
      !IS_STREAM_BACKWARD (SearchPathBStream)   ||
      !IS_STREAM (CurrNodePathBStream)          ||
      IS_END_OF_STREAM (CurrNodePathBStream)    ||
      !IS_STREAM_BACKWARD (CurrNodePathBStream)) {
    ASSERT (0);
    Status1 = EFI_INVALID_PARAMETER;
    ContinueEnum = FALSE;
    goto exit_handler;
  }

  CurrNodePathSize = AmlStreamGetMaxBufferSize (CurrNodePathBStream);
  if (CurrNodePathSize == 0) {
    ASSERT (0);
    Status1 = EFI_INVALID_PARAMETER;
    ContinueEnum = FALSE;
    goto exit_handler;
  }

  // Get the raw AML absolute pathname of the current node.
  Status1 = AmlGetRawNameSpacePath (Node, 0, CurrNodePathBStream);
  if (EFI_ERROR (Status1)) {
    ASSERT (0);
    ContinueEnum = FALSE;
    goto exit_handler;
  }

  DEBUG ((
    DEBUG_VERBOSE,
    "AmlNameSpace: "
    "Comparing search path with current node path.\n"
    ));
  DEBUG ((DEBUG_VERBOSE, "Search path:"));
  AMLDBG_PRINT_CHARS (
    DEBUG_VERBOSE,
    (CHAR8*)AmlStreamGetCurrPos (SearchPathBStream),
    AmlStreamGetIndex (SearchPathBStream)
    );
  DEBUG ((DEBUG_VERBOSE, "\nPath of the current node: "));
  AMLDBG_PRINT_CHARS (
    DEBUG_VERBOSE,
    (CHAR8*)AmlStreamGetCurrPos (CurrNodePathBStream),
    AmlStreamGetIndex (CurrNodePathBStream)
    );
  DEBUG ((DEBUG_VERBOSE, "\n"));

  // Compare the searched path and Node's path.
  if ((AmlStreamGetIndex (CurrNodePathBStream)  ==
         AmlStreamGetIndex (SearchPathBStream))     &&
      (CompareMem (
         AmlStreamGetCurrPos (CurrNodePathBStream),
         AmlStreamGetCurrPos (SearchPathBStream),
         AmlStreamGetIndex (SearchPathBStream)) == 0)) {
    Status1 = EFI_SUCCESS;
    ContinueEnum = FALSE;
    PathSearchContext->OutNode = Node;
  } else {
    // If the paths don't match, reset the CurrNodePathStream's content.
    Status1 = AmlStreamReset (CurrNodePathBStream);
    if (EFI_ERROR (Status1)) {
      ASSERT (0);
      ContinueEnum = FALSE;
    }
  }

exit_handler:
  if (Status != NULL) {
    *Status = Status1;
  }

  return ContinueEnum;
}

/** Build a raw AML absolute path from a reference node and a relative
    ASL path.

  The AslPath can be a relative path or an absolute path.
  Node must be a root node or a namespace node.
  A root node is expected to be at the top of the tree.

  @param  [in]  ReferenceNode               Reference node.
                                            If a relative path is given, the
                                            search is done from this node. If
                                            an absolute path is given, the
                                            search is done from the root node.
                                            Must be a root node or an object
                                            node which is part of the
                                            namespace.
  @param  [in]  AslPath                     ASL path to the searched node in
                                            the namespace. An ASL path name is
                                            NULL terminated. Can be a relative
                                            or absolute path.
                                            E.g.: "\\_SB.CLU0.CPU0".
  @param  [in, out] RawAmlAbsSearchPathBStream  Backward stream to write the
                                                raw absolute AML path of the
                                                searched node.
                                                The stream must not be at
                                                its end.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_BUFFER_TOO_SMALL    No space left in the buffer.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_OUT_OF_RESOURCES    Out of memory.
**/
STATIC
EFI_STATUS
EFIAPI
AmlBuildAbsoluteAmlPath (
  IN      AML_NODE_HEADER   * ReferenceNode,
  IN      CHAR8             * AslPath,
  IN  OUT AML_STREAM        * RawAmlAbsSearchPathBStream
  )
{
  EFI_STATUS    Status;
  CHAR8       * AmlPath;

  UINT32        AmlNameStringSize;
  UINT32        Root;
  UINT32        ParentPrefix;
  UINT32        SegCount;

  if ((!IS_AML_ROOT_NODE (ReferenceNode)              &&
       !AmlNodeHasAttribute (
         (CONST AML_OBJECT_NODE*)ReferenceNode,
         AML_IN_NAMESPACE))                           ||
      (AslPath == NULL)                               ||
      !IS_STREAM (RawAmlAbsSearchPathBStream)         ||
      IS_END_OF_STREAM (RawAmlAbsSearchPathBStream)   ||
      !IS_STREAM_BACKWARD (RawAmlAbsSearchPathBStream)) {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  // 1. Validate, analyze and convert the AslPath to an AmlPath.
  Status = ConvertAslNameToAmlName (AslPath, &AmlPath);
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    return Status;
  }

  Status = AmlParseNameStringInfo (AmlPath, &Root, &ParentPrefix, &SegCount);
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    goto exit_handler;
  }

  // Not possible to go beyond the root.
  if (IS_AML_ROOT_NODE (ReferenceNode) && (ParentPrefix != 0)) {
    Status = EFI_INVALID_PARAMETER;
    ASSERT (0);
    goto exit_handler;
  }

  AmlNameStringSize = AmlComputeNameStringSize (Root, ParentPrefix, SegCount);
  if (AmlNameStringSize == 0) {
    Status = EFI_INVALID_PARAMETER;
    ASSERT (0);
    goto exit_handler;
  }

  // 2.1. Write the AML path to the stream.
  Status = AmlStreamWrite (
              RawAmlAbsSearchPathBStream,
              (CONST UINT8*)AmlPath,
              AmlNameStringSize
              );
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    goto exit_handler;
  }

  // 2.2. Then remove the AML prefix (root char, parent prefix, etc.)
  //      to obtain a raw AML NameString. Raw AML NameString are easier
  //      to manipulate.
  Status = AmlRemovePrefix (RawAmlAbsSearchPathBStream);
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    goto exit_handler;
  }

  // 3. If AslPath is a relative path and the reference Node is not
  //    the root node, fill the Stream with the absolute path to the
  //    reference node.
  if ((Root == 0) && !IS_AML_ROOT_NODE (ReferenceNode)) {
    Status = AmlGetRawNameSpacePath (
               ReferenceNode,
               ParentPrefix,
               RawAmlAbsSearchPathBStream
               );
    if (EFI_ERROR (Status)) {
      ASSERT (0);
    }
  }

exit_handler:
  // Free allocated memory.
  FreePool (AmlPath);

  return Status;
}

/** Find a node in the AML namespace, given an ASL path and a reference Node.

   - The AslPath can be an absolute path, or a relative path from the
     reference Node;
   - Node must be a root node or a namespace node;
   - A root node is expected to be at the top of the tree.

  E.g.:
  For the following AML namespace, with the ReferenceNode being the node with
  the name "AAAA":
   - the node with the name "BBBB" can be found by looking for the ASL
     path "BBBB";
   - the root node can be found by looking for the ASL relative path "^",
     or the absolute path "\\".

  AML namespace:
  \
  \-AAAA      <- ReferenceNode
    \-BBBB

  @param  [in]  ReferenceNode   Reference node.
                                If a relative path is given, the
                                search is done from this node. If
                                an absolute path is given, the
                                search is done from the root node.
                                Must be a root node or an object
                                node which is part of the
                                namespace.
  @param  [in]  AslPath         ASL path to the searched node in
                                the namespace. An ASL path name is
                                NULL terminated. Can be a relative
                                or absolute path.
                                E.g.: "\\_SB.CLU0.CPU0" or "^CPU0"
  @param  [out] OutNode         Pointer to the found node.
                                Contains NULL if not found.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_BUFFER_TOO_SMALL    No space left in the buffer.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_OUT_OF_RESOURCES    Out of memory.
**/
EFI_STATUS
EFIAPI
AmlFindNode (
  IN  AML_NODE_HEADER     * ReferenceNode,
  IN  CHAR8               * AslPath,
  OUT AML_NODE_HEADER    ** OutNode
  )
{
  EFI_STATUS                  Status;

  AML_PATH_SEARCH_CONTEXT     PathSearchContext;
  AML_ROOT_NODE             * RootNode;

  // Backward stream used to build the raw AML absolute path to the searched
  // node.
  AML_STREAM                  RawAmlAbsSearchPathBStream;
  CHAR8                     * RawAmlAbsSearchPathBuffer;
  UINT32                      RawAmlAbsSearchPathBufferSize;

  // Backward stream used to store the raw AML absolute path of the node
  // currently enumerated in the tree. This path can then be compared to the
  // RawAmlAbsSearchPath.
  AML_STREAM                  RawAmlAbsCurrNodePathBStream;
  CHAR8                     * RawAmlAbsCurrNodePathBuffer;
  UINT32                      RawAmlAbsCurrNodePathBufferSize;

  if ((!IS_AML_ROOT_NODE (ReferenceNode)        &&
       !AmlNodeHasAttribute (
         (CONST AML_OBJECT_NODE*)ReferenceNode,
         AML_IN_NAMESPACE))                     ||
      (AslPath == NULL)                         ||
      (OutNode == NULL)) {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  *OutNode = NULL;
  RawAmlAbsCurrNodePathBuffer = NULL;

  // 1. Build a raw absolute AML path from the reference node and the ASL
  //    path. For this:
  // 1.1. First initialize a backward stream.
  RawAmlAbsSearchPathBufferSize = MAX_AML_NAMESTRING_SIZE;
  RawAmlAbsSearchPathBuffer = AllocateZeroPool (RawAmlAbsSearchPathBufferSize);
  if (RawAmlAbsSearchPathBuffer == NULL) {
    ASSERT (0);
    return EFI_OUT_OF_RESOURCES;
  }

  Status = AmlStreamInit (
             &RawAmlAbsSearchPathBStream,
             (UINT8*)RawAmlAbsSearchPathBuffer,
             RawAmlAbsSearchPathBufferSize,
             EAmlStreamDirectionBackward
             );
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    goto exit_handler;
  }

  // 1.2. Then build the raw AML absolute path.
  Status = AmlBuildAbsoluteAmlPath (
             ReferenceNode,
             AslPath,
             &RawAmlAbsSearchPathBStream
             );
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    goto exit_handler;
  }

  // 2. Find the root node by climbing up the tree from the reference node.
  RootNode = AmlGetRootNode (ReferenceNode);
  if (RootNode == NULL) {
    ASSERT (0);
    Status = EFI_INVALID_PARAMETER;
    goto exit_handler;
  }

  // 3. If the searched node is the root node, return.
  //    For the Root Node there is no NameSegs so the length of
  //     the stream will be zero.
  if (AmlStreamGetIndex (&RawAmlAbsSearchPathBStream) == 0) {
    *OutNode = (AML_NODE_HEADER*)RootNode;
    Status = EFI_SUCCESS;
    goto exit_handler;
  }

  // 4. Create a backward stream large enough to hold the current node path
  //    during enumeration. This prevents from doing multiple allocation/free
  //    operations.
  RawAmlAbsCurrNodePathBufferSize = MAX_ASL_NAMESTRING_SIZE;
  RawAmlAbsCurrNodePathBuffer = AllocateZeroPool (
                                  RawAmlAbsCurrNodePathBufferSize
                                  );
  if (RawAmlAbsCurrNodePathBuffer == NULL) {
    ASSERT (0);
    Status = EFI_OUT_OF_RESOURCES;
    goto exit_handler;
  }

  Status = AmlStreamInit (
             &RawAmlAbsCurrNodePathBStream,
             (UINT8*)RawAmlAbsCurrNodePathBuffer,
             RawAmlAbsCurrNodePathBufferSize,
             EAmlStreamDirectionBackward
             );
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    goto exit_handler;
  }

  // 5. Fill a path search context structure with:
  //     - SearchPathStream: backward stream containing the raw absolute AML
  //       path to the searched node;
  //     - CurrNodePathStream: backward stream containing the raw absolute AML
  //       of the node currently being enumerated;
  //     - OutNode: node pointer to the store the potentially found node.
  PathSearchContext.SearchPathBStream = &RawAmlAbsSearchPathBStream;
  PathSearchContext.CurrNodePathBStream = &RawAmlAbsCurrNodePathBStream;
  PathSearchContext.OutNode = NULL;

  // 6. Iterate through the namespace nodes of the tree.
  //    For each namespace node, build its raw AML absolute path. Then compare
  //    it with the search path.
  AmlEnumTree (
    (AML_NODE_HEADER*)RootNode,
    AmlEnumeratePathCallback,
    (VOID*)&PathSearchContext,
    &Status
    );
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    goto exit_handler;
  }

  *OutNode = PathSearchContext.OutNode;
  if (*OutNode == NULL) {
    Status = EFI_NOT_FOUND;
  }

exit_handler:
  // Free allocated memory.
  FreePool (RawAmlAbsSearchPathBuffer);
  if (RawAmlAbsCurrNodePathBuffer != NULL) {
    FreePool (RawAmlAbsCurrNodePathBuffer);
  }

  return Status;
}
