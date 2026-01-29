/** @file
  AML Method Parser.

  Copyright (c) 2020, Arm Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Parser/AmlMethodParser.h>

#include <AmlCoreInterface.h>
#include <AmlDbgPrint/AmlDbgPrint.h>
#include <NameSpace/AmlNameSpace.h>
#include <Parser/AmlParser.h>
#include <Tree/AmlNode.h>
#include <Tree/AmlTree.h>
#include <String/AmlString.h>

/** Delete a namespace reference node and its pathname.

  It is the caller's responsibility to check the NameSpaceRefNode has been
  removed from any list the node is part of.

  @param  [in]  NameSpaceRefNode   Pointer to an AML_NAMESPACE_REF_NODE.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
**/
STATIC
EFI_STATUS
EFIAPI
AmlDeleteNameSpaceRefNode (
  IN  AML_NAMESPACE_REF_NODE  *NameSpaceRefNode
  )
{
  if (NameSpaceRefNode == NULL) {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  if (NameSpaceRefNode->RawAbsolutePath != NULL) {
    FreePool ((CHAR8 *)NameSpaceRefNode->RawAbsolutePath);
  } else {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  FreePool (NameSpaceRefNode);
  return EFI_SUCCESS;
}

/** Delete a list of namespace reference nodes.

  @param  [in]  NameSpaceRefList    List of namespace reference nodes.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
**/
EFI_STATUS
EFIAPI
AmlDeleteNameSpaceRefList (
  IN  LIST_ENTRY  *NameSpaceRefList
  )
{
  EFI_STATUS  Status;
  LIST_ENTRY  *CurrentLink;

  if (NameSpaceRefList == NULL) {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  while (!IsListEmpty (NameSpaceRefList)) {
    CurrentLink = NameSpaceRefList->ForwardLink;
    RemoveEntryList (CurrentLink);
    Status = AmlDeleteNameSpaceRefNode (
               (AML_NAMESPACE_REF_NODE *)CurrentLink
               );
    if (EFI_ERROR (Status)) {
      ASSERT (0);
      return Status;
    }
  } // while

  return EFI_SUCCESS;
}

/** Create an AML_NAMESPACE_REF_NODE.

  A Buffer is allocated to store the raw AML absolute path.

  @param  [in]  ObjectNode          Node being part of the namespace.
                                    Must be have the AML_IN_NAMESPACE
                                    attribute.
  @param  [in]  RawAbsolutePath     AML raw absolute path of the ObjectNode.
                                    A raw NameString is a concatenated list
                                    of 4 chars long names.
  @param  [in]  RawAbsolutePathSize Size of the RawAbsolutePath buffer.
  @param  [out] NameSpaceRefNodePtr The created AML_METHOD_REF_NODE.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_OUT_OF_RESOURCES    Could not allocate memory.
**/
STATIC
EFI_STATUS
EFIAPI
AmlCreateMethodRefNode (
  IN  CONST AML_OBJECT_NODE         *ObjectNode,
  IN  CONST CHAR8                   *RawAbsolutePath,
  IN        UINT32                  RawAbsolutePathSize,
  OUT       AML_NAMESPACE_REF_NODE  **NameSpaceRefNodePtr
  )
{
  AML_NAMESPACE_REF_NODE  *NameSpaceRefNode;

  if (!AmlNodeHasAttribute (ObjectNode, AML_IN_NAMESPACE) ||
      (RawAbsolutePath == NULL)                           ||
      (RawAbsolutePathSize == 0)                          ||
      (NameSpaceRefNodePtr == NULL))
  {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  NameSpaceRefNode = AllocateZeroPool (sizeof (AML_NAMESPACE_REF_NODE));
  if (NameSpaceRefNode == NULL) {
    ASSERT (0);
    return EFI_OUT_OF_RESOURCES;
  }

  NameSpaceRefNode->RawAbsolutePathSize = RawAbsolutePathSize;
  NameSpaceRefNode->RawAbsolutePath     = AllocateCopyPool (
                                            RawAbsolutePathSize,
                                            RawAbsolutePath
                                            );
  if (NameSpaceRefNode->RawAbsolutePath == NULL) {
    FreePool (NameSpaceRefNode);
    ASSERT (0);
    return EFI_OUT_OF_RESOURCES;
  }

  InitializeListHead (&NameSpaceRefNode->Link);

  NameSpaceRefNode->NodeRef = ObjectNode;
  *NameSpaceRefNodePtr      = NameSpaceRefNode;

  return EFI_SUCCESS;
}

#if !defined (MDEPKG_NDEBUG)

/** Print the list of raw absolute paths of the NameSpace reference list.

  @param  [in]  NameSpaceRefList    List of NameSpace reference nodes.
**/
VOID
EFIAPI
AmlDbgPrintNameSpaceRefList (
  IN  CONST LIST_ENTRY  *NameSpaceRefList
  )
{
  LIST_ENTRY              *CurrLink;
  AML_NAMESPACE_REF_NODE  *CurrNameSpaceNode;

  if (NameSpaceRefList == NULL) {
    ASSERT (0);
    return;
  }

  DEBUG ((DEBUG_INFO, "AmlMethodParser: List of available raw AML paths:\n"));

  CurrLink = NameSpaceRefList->ForwardLink;
  while (CurrLink != NameSpaceRefList) {
    CurrNameSpaceNode = (AML_NAMESPACE_REF_NODE *)CurrLink;

    AMLDBG_PRINT_CHARS (
      DEBUG_INFO,
      CurrNameSpaceNode->RawAbsolutePath,
      CurrNameSpaceNode->RawAbsolutePathSize
      );
    DEBUG ((DEBUG_INFO, "\n"));

    CurrLink = CurrLink->ForwardLink;
  }

  DEBUG ((DEBUG_INFO, "\n"));
}

#endif // MDEPKG_NDEBUG

/** From a forward stream pointing to a NameString,
    initialize a raw backward stream.

        StartOfStream
  Fstream: CurrPos                                 EndOfStream
             v                                        v
             +-----------------------------------------+
             |^^^[Multi-name prefix]AAAA.BBBB.CCCC     |
             +-----------------------------------------+
                                    ^            ^
  RawPathNameBStream:           EndOfStream    CurrPos
                                            StartOfStream

  No memory is allocated when initializing the stream.

  @param  [in]  FStream             Forward stream pointing to a NameString.
                                    The stream must not be at its end.
  @param  [out] RawPathNameBStream  Backward stream containing the
                                    raw AML path.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
**/
STATIC
EFI_STATUS
EFIAPI
AmlInitRawPathBStream (
  IN  CONST AML_STREAM  *FStream,
  OUT       AML_STREAM  *RawPathNameBStream
  )
{
  EFI_STATUS  Status;

  UINT8        *RawPathBuffer;
  CONST CHAR8  *Buffer;

  UINT32  Root;
  UINT32  ParentPrefix;
  UINT32  SegCount;

  if (!IS_STREAM (FStream)          ||
      IS_END_OF_STREAM (FStream)    ||
      !IS_STREAM_FORWARD (FStream) ||
      (RawPathNameBStream == NULL))
  {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  Buffer = (CONST CHAR8 *)AmlStreamGetCurrPos (FStream);
  if (Buffer == NULL) {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  // Parse the NameString information.
  Status = AmlParseNameStringInfo (
             Buffer,
             &Root,
             &ParentPrefix,
             &SegCount
             );
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    return Status;
  }

  // Get the beginning of the raw NameString.
  RawPathBuffer = (UINT8 *)AmlGetFirstNameSeg (
                             Buffer,
                             Root,
                             ParentPrefix
                             );
  if (RawPathBuffer == NULL) {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  // Initialize a backward stream containing the raw path.
  Status = AmlStreamInit (
             RawPathNameBStream,
             RawPathBuffer,
             (SegCount * AML_NAME_SEG_SIZE),
             EAmlStreamDirectionBackward
             );
  ASSERT_EFI_ERROR (Status);

  return Status;
}

/** Get the first node in the ParentNode branch that is part of the
    AML namespace and has its name defined.

  This is different from getting the first namespace node. This function is
  necessary because an absolute path is built while the tree is not complete
  yet. The parsing is ongoing.

  For instance, the ASL statement "CreateXXXField ()" adds a field in the
  AML namespace, but the name it defines is the last fixed argument of the
  corresponding object.
  If an AML path is referenced in its first fixed argument, it is not
  possible to resolve the name of the CreateXXXField object. However, the AML
  path is not part of the scope created by the CreateXXXField object, so this
  scope can be skipped.

  In the following ASL code, the method invocation to MET0 is done in the
  "CreateField" statement. The "CreateField" statement defines the "FIEL"
  path in the AML namespace. However, MET0 must be not be resolved in the
  "CreateField" object scope. It needs to be resolved in its parent.
  ASL code:
  Method (MET0, 0,,, BuffObj) {
    Return (Buffer (0x1000) {})
  }
  CreateField (MET0(), 0x100, 0x4, FIEL)

  @param  [in]  Node          Node to get the first named node from, in
                              its hierarchy.
  @param  [out] OutNamedNode  First named node in Node's hierarchy.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
**/
STATIC
EFI_STATUS
EFIAPI
AmlGetFirstNamedAncestorNode (
  IN  CONST AML_NODE_HEADER  *Node,
  OUT       AML_NODE_HEADER  **OutNamedNode
  )
{
  EFI_STATUS             Status;
  CONST AML_NODE_HEADER  *NameSpaceNode;

  if ((!IS_AML_OBJECT_NODE (Node)   &&
       !IS_AML_ROOT_NODE (Node))    ||
      (OutNamedNode == NULL))
  {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  // If Node is not the root node and doesn't have a name defined yet,
  // get the ancestor NameSpace node.
  while (!IS_AML_ROOT_NODE (Node)             &&
         !(AmlNodeHasAttribute (
             (CONST AML_OBJECT_NODE *)Node,
             AML_IN_NAMESPACE
             )               &&
           AmlNodeGetName ((CONST AML_OBJECT_NODE *)Node) != NULL))
  {
    Status = AmlGetFirstAncestorNameSpaceNode (
               Node,
               (AML_NODE_HEADER **)&NameSpaceNode
               );
    if (EFI_ERROR (Status)) {
      ASSERT (0);
      return Status;
    }

    // The NameSpaceNode may not have its name defined as yet. In this
    // case get the next ancestor node.
    Node = NameSpaceNode;
  }

  *OutNamedNode = (AML_NODE_HEADER *)Node;

  return EFI_SUCCESS;
}

/** From a ParentNode and a forward stream pointing to a relative path,
    build a raw AML absolute path and return it in a backward stream.

  No memory is allocated in this function, the out stream must be initialized
  with a buffer long enough to hold any raw absolute AML path.

  @param  [in]  ParentNode                  Parent node of the namespace
                                            node from which the absolute
                                            path is built. ParentNode isn't
                                            necessarily a namespace node.
                                            Must be a root or an object node.
  @param  [in]  PathnameFStream             Forward stream pointing to the
                                            beginning of a pathname (any
                                            NameString).
                                            The stream must not be at its end.
  @param  [in, out] AbsolutePathBStream     Backward stream where the raw
                                            absolute path is written. The
                                            stream must be already initialized.
                                            The stream must not be at its end.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_BUFFER_TOO_SMALL    No space left in the buffer.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
**/
STATIC
EFI_STATUS
EFIAPI
AmlBuildRawMethodAbsolutePath (
  IN      CONST AML_NODE_HEADER  *ParentNode,
  IN      CONST AML_STREAM       *PathnameFStream,
  IN  OUT       AML_STREAM       *AbsolutePathBStream
  )
{
  EFI_STATUS  Status;

  AML_NODE_HEADER  *NamedParentNode;
  UINT8            *RawPathBuffer;
  CONST CHAR8      *CurrPos;

  UINT32  Root;
  UINT32  ParentPrefix;
  UINT32  SegCount;

  if ((!IS_AML_OBJECT_NODE (ParentNode)         &&
       !IS_AML_ROOT_NODE (ParentNode))          ||
      !IS_STREAM (PathnameFStream)              ||
      IS_END_OF_STREAM (PathnameFStream)        ||
      !IS_STREAM_FORWARD (PathnameFStream)      ||
      !IS_STREAM (AbsolutePathBStream)          ||
      IS_END_OF_STREAM (AbsolutePathBStream)    ||
      !IS_STREAM_BACKWARD (AbsolutePathBStream))
  {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  CurrPos = (CONST CHAR8 *)AmlStreamGetCurrPos (PathnameFStream);
  if (CurrPos == NULL) {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  // Parse the NameString information.
  Status = AmlParseNameStringInfo (
             CurrPos,
             &Root,
             &ParentPrefix,
             &SegCount
             );
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    return Status;
  }

  // Copy the method invocation raw relative path at the end of the Stream.
  if (SegCount != 0) {
    // Get the beginning of the raw NameString.
    RawPathBuffer = (UINT8 *)AmlGetFirstNameSeg (
                               CurrPos,
                               Root,
                               ParentPrefix
                               );

    Status = AmlStreamWrite (
               AbsolutePathBStream,
               RawPathBuffer,
               SegCount * AML_NAME_SEG_SIZE
               );
    if (EFI_ERROR (Status)) {
      ASSERT (0);
      return Status;
    }
  }

  // If the pathname contained an absolute path, this is finished, return.
  if (Root) {
    return Status;
  }

  // Get the first named node of the parent node in its hierarchy.
  Status = AmlGetFirstNamedAncestorNode (ParentNode, &NamedParentNode);
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    return Status;
  }

  // Build the raw absolute path of the namespace node.
  Status = AmlGetRawNameSpacePath (
             NamedParentNode,
             ParentPrefix,
             AbsolutePathBStream
             );
  ASSERT_EFI_ERROR (Status);

  return Status;
}

/** Compare two raw NameStrings stored in forward streams.
    Compare them NameSeg by NameSeg (a NameSeg is 4 bytes long).

  The two raw NameStrings can be of different size.

  @param  [in]  RawFStream1     First forward stream to compare.
                                Points to the beginning of the raw NameString.
  @param  [in]  RawFStream2     Second forward stream to compare.
                                Points to the beginning of the raw NameString.
  @param  [out] CompareCount    Count of identic bytes.
                                Must be a multiple of 4 (size of a NameSeg).

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
**/
STATIC
EFI_STATUS
EFIAPI
AmlCompareRawNameString (
  IN  CONST AML_STREAM  *RawFStream1,
  IN  CONST AML_STREAM  *RawFStream2,
  OUT       UINT32      *CompareCount
  )
{
  EFI_STATUS  Status;
  UINT32      Index;

  AML_STREAM  RawFStream1Clone;
  AML_STREAM  RawFStream2Clone;
  UINT32      Stream1Size;
  UINT32      Stream2Size;
  UINT32      CompareLen;

  // Raw NameStrings have a size that is a multiple of the size of NameSegs.
  if (!IS_STREAM (RawFStream1)          ||
      IS_END_OF_STREAM (RawFStream1)    ||
      !IS_STREAM_FORWARD (RawFStream1)  ||
      !IS_STREAM (RawFStream2)          ||
      IS_END_OF_STREAM (RawFStream2)    ||
      (CompareCount == NULL))
  {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  Stream1Size = AmlStreamGetFreeSpace (RawFStream1);
  if ((Stream1Size & (AML_NAME_SEG_SIZE - 1)) != 0) {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  Stream2Size = AmlStreamGetFreeSpace (RawFStream2);
  if ((Stream2Size & (AML_NAME_SEG_SIZE - 1)) != 0) {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  Status = AmlStreamClone (RawFStream1, &RawFStream1Clone);
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    return Status;
  }

  Status = AmlStreamClone (RawFStream2, &RawFStream2Clone);
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    return Status;
  }

  CompareLen = MIN (Stream1Size, Stream2Size);
  Index      = 0;
  // Check there is enough space for a NameSeg in both Stream1 and Stream2.
  while (Index < CompareLen) {
    if (!AmlStreamCmp (
           &RawFStream1Clone,
           &RawFStream2Clone,
           AML_NAME_SEG_SIZE
           )
        )
    {
      // NameSegs are different. Break.
      break;
    }

    Status = AmlStreamProgress (&RawFStream1Clone, AML_NAME_SEG_SIZE);
    if (EFI_ERROR (Status)) {
      ASSERT (0);
      return Status;
    }

    Status = AmlStreamProgress (&RawFStream2Clone, AML_NAME_SEG_SIZE);
    if (EFI_ERROR (Status)) {
      ASSERT (0);
      return Status;
    }

    Index += AML_NAME_SEG_SIZE;
  }

  *CompareCount = Index;

  return EFI_SUCCESS;
}

/** Check whether an alias can be resolved to a method definition.

  Indeed, the following ASL code must be handled:
    Method (MET0, 1) {
      Return (0x9)
    }
    Alias (\MET0, \ALI0)
    Alias (\ALI0, \ALI1)
    \ALI1(0x5)
  When searching for \ALI1 in the AML NameSpace, it resolves to \ALI0.
  When searching for \ALI0 in the AML NameSpace, it resolves to \MET0.
  When searching for \MET0 in the AML NameSpace, it resolves to a method
  definition.

  This method is a wrapper to recursively call AmlFindMethodDefinition.

  @param  [in]  AliasNode             Pointer to an Alias object node.
  @param  [in]  NameSpaceRefList      List of NameSpaceRef nodes.
  @param  [out] OutNameSpaceRefNode   If success, and if the alias is resolved
                                      to a method definition (this can go
                                      through other alias indirections),
                                      containing the corresponding
                                      NameSpaceRef node.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
**/
STATIC
EFI_STATUS
EFIAPI
AmlResolveAliasMethod (
  IN  CONST AML_OBJECT_NODE         *AliasNode,
  IN  CONST LIST_ENTRY              *NameSpaceRefList,
  OUT       AML_NAMESPACE_REF_NODE  **OutNameSpaceRefNode
  )
{
  EFI_STATUS           Status;
  AML_STREAM           SourceAliasFStream;
  CONST AML_DATA_NODE  *DataNode;

  if (!AmlNodeCompareOpCode (AliasNode, AML_ALIAS_OP, 0)  ||
      (NameSpaceRefList == NULL)                          ||
      (OutNameSpaceRefNode == NULL))
  {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  // The aliased NameString (the source name) is the first fixed argument,
  // cf. ACPI6.3 spec, s19.6.4: Alias (SourceObject, AliasObject)
  DataNode = (CONST AML_DATA_NODE *)AmlGetFixedArgument (
                                      (AML_OBJECT_NODE *)AliasNode,
                                      EAmlParseIndexTerm0
                                      );
  if (DataNode == NULL) {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  // Initialize a stream on the source alias NameString.
  Status = AmlStreamInit (
             &SourceAliasFStream,
             DataNode->Buffer,
             DataNode->Size,
             EAmlStreamDirectionForward
             );
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    return Status;
  }

  // Recursively check whether the source alias NameString
  // is a method invocation.
  Status = AmlIsMethodInvocation (
             AmlGetParent ((AML_NODE_HEADER *)AliasNode),
             &SourceAliasFStream,
             NameSpaceRefList,
             OutNameSpaceRefNode
             );
  if (EFI_ERROR (Status)) {
    ASSERT (0);
  }

  return Status;
}

/** Iterate through the MethodList to find the best namespace resolution.
    If the pathname resolves to a method definition, returns it.

  For instance, if the AML namespace is:
  \
  \-MET0         <- Device definition, absolute path: \MET0
  \-AAAA
    \-MET0       <- Method definition, absolute path: \AAAA.MET0
    \-MET1       <- Method definition, absolute path: \AAAA.MET1
    \-BBBB
      \-CCCC
        \-DDDD
          \-MET0 <- Method definition, absolute path: \AAAA.BBBB.CCCC.DDDD.MET0

  The list of the available pathnames is:
  [NameSpaceRefList]
   - \MET0                          <-  Device definition
   - \AAAA
   - \AAAA.MET0                     <-  Method definition
   - \AAAA.MET1                     <-  Method definition
   - \AAAA.BBBB
   - \AAAA.BBBB.CCCC
   - \AAAA.BBBB.CCCC.DDDD
   - \AAAA.BBBB.CCCC.DDDD.MET0      <-  Method definition

  Depending on where the method invocation is done, the method definition
  referenced changes. If the method call "MET0" is done from
  \AAAA.BBBB.CCCC:
    1. Identify which pathnames end with "MET0":
     - \MET0                          <-  Device definition
     - \AAAA.MET0                     <-  Method definition
     - \AAAA.BBBB.CCCC.DDDD.MET0      <-  Method definition
    2. Resolve the method invocation:
     - \AAAA.MET0                     <-  Method definition
    3. \AAAA.MET0 is a method definition, so return the corresponding
       reference node.

  @param  [in]  RawAbsolutePathFStream    Forward stream pointing to a raw
                                          absolute path.
                                          The stream must not be at its end.
  @param  [in]  RawPathNameBStream        Backward stream pointing to a raw
                                          pathname. This raw pathname is the
                                          raw NameString of namespace node.
                                          The stream must not be at its end.
  @param  [in]  NameSpaceRefList          List of NameSpaceRef nodes.
  @param  [out] OutNameSpaceRefNode       If the two input paths are
                                          referencing a method definition,
                                          returns the corresponding
                                          NameSpaceRef node.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
**/
STATIC
EFI_STATUS
EFIAPI
AmlFindMethodDefinition (
  IN  CONST AML_STREAM              *RawAbsolutePathFStream,
  IN  CONST AML_STREAM              *RawPathNameBStream,
  IN  CONST LIST_ENTRY              *NameSpaceRefList,
  OUT       AML_NAMESPACE_REF_NODE  **OutNameSpaceRefNode
  )
{
  EFI_STATUS  Status;

  LIST_ENTRY  *NextLink;

  // To resolve a pathname, scope levels need to be compared.
  UINT32  NameSegScopeCount;
  UINT32  PathNameSegScopeCount;
  UINT32  ProbedScopeCount;
  UINT32  BestScopeCount;

  AML_STREAM  ProbedRawAbsoluteFStream;
  AML_STREAM  ProbedRawAbsoluteBStream;

  AML_NAMESPACE_REF_NODE  *ProbedNameSpaceRefNode;
  AML_NAMESPACE_REF_NODE  *BestNameSpaceRefNode;

  if (!IS_STREAM (RawAbsolutePathFStream)                               ||
      IS_END_OF_STREAM (RawAbsolutePathFStream)                         ||
      !IS_STREAM_FORWARD (RawAbsolutePathFStream)                       ||
      ((AmlStreamGetIndex (RawAbsolutePathFStream) &
        (AML_NAME_SEG_SIZE - 1)) != 0)                                  ||
      !IS_STREAM (RawPathNameBStream)                                   ||
      IS_END_OF_STREAM (RawPathNameBStream)                             ||
      !IS_STREAM_BACKWARD (RawPathNameBStream)                          ||
      ((AmlStreamGetIndex (RawPathNameBStream) &
        (AML_NAME_SEG_SIZE - 1)) != 0)                                  ||
      (NameSpaceRefList == NULL)                                        ||
      (OutNameSpaceRefNode == NULL))
  {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  DEBUG ((DEBUG_VERBOSE, "AmlMethodParser: Checking absolute name: "));
  AMLDBG_PRINT_CHARS (
    DEBUG_VERBOSE,
    (CONST CHAR8 *)AmlStreamGetCurrPos (RawAbsolutePathFStream),
    AmlStreamGetMaxBufferSize (RawAbsolutePathFStream)
    );
  DEBUG ((DEBUG_VERBOSE, ".\n"));

  BestNameSpaceRefNode  = NULL;
  BestScopeCount        = 0;
  NameSegScopeCount     = AmlStreamGetMaxBufferSize (RawAbsolutePathFStream);
  PathNameSegScopeCount = AmlStreamGetMaxBufferSize (RawPathNameBStream);

  // Iterate through the raw AML absolute path to find the best match.
  DEBUG ((DEBUG_VERBOSE, "AmlMethodParser: Comparing with: "));
  NextLink = NameSpaceRefList->ForwardLink;
  while (NextLink != NameSpaceRefList) {
    ProbedNameSpaceRefNode = (AML_NAMESPACE_REF_NODE *)NextLink;

    // Print the raw absolute path of the probed node.
    AMLDBG_PRINT_CHARS (
      DEBUG_VERBOSE,
      ProbedNameSpaceRefNode->RawAbsolutePath,
      ProbedNameSpaceRefNode->RawAbsolutePathSize
      );
    DEBUG ((DEBUG_VERBOSE, "; "));

    // If the raw AML absolute path of the probed node is longer than the
    // searched pathname, continue.
    // E.g.: The method call \MET0 cannot resolve to a method defined at
    //       \AAAA.MET0. The method definition is out of scope.
    if (PathNameSegScopeCount > ProbedNameSpaceRefNode->RawAbsolutePathSize) {
      NextLink = NextLink->ForwardLink;
      continue;
    }

    // Initialize a backward stream for the probed node.
    // This stream is used to compare the ending of the pathnames.
    // E.g. if the method searched ends with "MET0", pathnames not ending with
    //      "MET0" should be skipped.
    Status = AmlStreamInit (
               &ProbedRawAbsoluteBStream,
               (UINT8 *)ProbedNameSpaceRefNode->RawAbsolutePath,
               ProbedNameSpaceRefNode->RawAbsolutePathSize,
               EAmlStreamDirectionBackward
               );
    if (EFI_ERROR (Status)) {
      ASSERT (0);
      return Status;
    }

    // Compare the pathname endings. If they don't match, continue.
    if (!AmlStreamCmp (
           RawPathNameBStream,
           &ProbedRawAbsoluteBStream,
           AmlStreamGetMaxBufferSize (RawPathNameBStream)
           ))
    {
      NextLink = NextLink->ForwardLink;
      continue;
    }

    // Initialize a forward stream for the probed node.
    // This stream is used to count how many scope levels from the root
    // are common with the probed node. The more there are, the better it is.
    // E.g.: For the method invocation \AAAA.BBBB.MET0, if there are 2
    //       pathnames ending with MET0:
    //        - \AAAA.MET0 has 1 NameSeg in common with \AAAA.BBBB.MET0
    //          from the root (this is "AAAA");
    //        - \MET0 has 0 NameSeg in common with \AAAA.BBBB.MET0
    //          from the root;
    //       Thus, the best match is \AAAA.MET0.
    Status = AmlStreamInit (
               &ProbedRawAbsoluteFStream,
               (UINT8 *)ProbedNameSpaceRefNode->RawAbsolutePath,
               ProbedNameSpaceRefNode->RawAbsolutePathSize,
               EAmlStreamDirectionForward
               );
    if (EFI_ERROR (Status)) {
      ASSERT (0);
      return Status;
    }

    // Count how many namespace levels are in common from the root.
    Status = AmlCompareRawNameString (
               RawAbsolutePathFStream,
               &ProbedRawAbsoluteFStream,
               &ProbedScopeCount
               );
    if (EFI_ERROR (Status)) {
      ASSERT (0);
      return EFI_INVALID_PARAMETER;
    }

    if (ProbedScopeCount == NameSegScopeCount) {
      // This is a perfect match. Exit the loop.
      BestNameSpaceRefNode = ProbedNameSpaceRefNode;
      break;
    } else if (ProbedScopeCount > BestScopeCount) {
      // The probed node has more scope levels in common than the
      // last best match. Update the best match.
      BestScopeCount       = ProbedScopeCount;
      BestNameSpaceRefNode = ProbedNameSpaceRefNode;
    } else if (ProbedScopeCount == BestScopeCount) {
      // The probed node has the same number of scope levels in
      // common as the last best match.
      if (ProbedScopeCount == 0) {
        // There was not best match previously. Set it.
        BestNameSpaceRefNode = ProbedNameSpaceRefNode;
      } else {
        // (ProbedScopeCount != 0)
        // If there is an equivalent candidate, the best has the shortest
        // absolute path. Indeed, a similar ProbedScopeCount and a longer
        // path means the definition is out of the scope.
        // E.g.: For the method invocation \AAAA.BBBB.MET0, if there are 2
        //       pathnames ending with MET0:
        //        - \AAAA.MET0 has 1 NameSegs in common with \AAAA.BBBB.MET0
        //          from the root (this is "AAAA");
        //        - \AAAA.CCCC.MET0 has 1 NameSegs in common with
        //          \AAAA.BBBB.MET0 from the root (this is "AAAA");
        //       As \AAAA.CCCC.MET0 is longer than \AAAA.MET0, it means that
        //       the pathname could have matched on more NameSegs, but it
        //       didn't because it is out of scope.
        //       Thus, the best match is \AAAA.MET0.
        if (AmlStreamGetIndex (&ProbedRawAbsoluteFStream) <
            BestNameSpaceRefNode->RawAbsolutePathSize)
        {
          BestScopeCount       = ProbedScopeCount;
          BestNameSpaceRefNode = ProbedNameSpaceRefNode;
        } else if (AmlStreamGetIndex (&ProbedRawAbsoluteFStream) ==
                   BestNameSpaceRefNode->RawAbsolutePathSize)
        {
          ASSERT (0);
          return EFI_INVALID_PARAMETER;
        }
      }
    }

    NextLink = NextLink->ForwardLink;
  }

  DEBUG ((DEBUG_VERBOSE, "\n"));

  // Check whether the BestNameSpaceRefNode is a method definition.
  if (BestNameSpaceRefNode != NULL) {
    if (AmlIsMethodDefinitionNode (BestNameSpaceRefNode->NodeRef)) {
      *OutNameSpaceRefNode = BestNameSpaceRefNode;
    } else if (AmlNodeCompareOpCode (
                 BestNameSpaceRefNode->NodeRef,
                 AML_ALIAS_OP,
                 0
                 ))
    {
      // The path matches an alias. Resolve the alias and check whether
      // this is a method defintion.
      Status = AmlResolveAliasMethod (
                 BestNameSpaceRefNode->NodeRef,
                 NameSpaceRefList,
                 OutNameSpaceRefNode
                 );
      if (EFI_ERROR (Status)) {
        ASSERT (0);
        return Status;
      }
    }
  } else {
    // If no, return NULL, even if a matching pathname has been found.
    *OutNameSpaceRefNode = NULL;
  }

  return EFI_SUCCESS;
}

/** Check whether a pathname is a method invocation.

  If there is a matching method definition, returns the corresponding
  NameSpaceRef node.

  To do so, the NameSpaceRefList is keeping track of every namespace node
  and its raw AML absolute path.
  To check whether a pathname is a method invocation, a corresponding raw
  absolute pathname is built. This raw absolute pathname is then compared
  to the list of available pathnames. If a pathname defining a method
  matches the scope of the input pathname, return.

  @param  [in]  ParentNode          Parent node. Node to which the node to be
                                    created will be attached.
  @param  [in]  FStream             Forward stream pointing to the NameString
                                    to find.
  @param  [in]  NameSpaceRefList    List of NameSpaceRef nodes.
  @param  [out] OutNameSpaceRefNode If the NameString pointed by FStream is
                                    a method invocation, OutNameSpaceRefNode
                                    contains the NameSpaceRef corresponding
                                    to the method definition.
                                    NULL otherwise.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
**/
EFI_STATUS
EFIAPI
AmlIsMethodInvocation (
  IN  CONST AML_NODE_HEADER   *ParentNode,
  IN  CONST AML_STREAM        *FStream,
  IN  CONST LIST_ENTRY        *NameSpaceRefList,
  OUT AML_NAMESPACE_REF_NODE  **OutNameSpaceRefNode
  )
{
  EFI_STATUS  Status;

  AML_STREAM  RawPathNameBStream;
  AML_STREAM  RawAbsolutePathFStream;

  AML_STREAM  RawAbsolutePathBStream;
  UINT8       *RawAbsolutePathBuffer;
  UINT32      RawAbsolutePathBufferSize;

  AML_NAMESPACE_REF_NODE  *NameSpaceRefNode;

  if ((!IS_AML_OBJECT_NODE (ParentNode) &&
       !IS_AML_ROOT_NODE (ParentNode))  ||
      !IS_STREAM (FStream)              ||
      IS_END_OF_STREAM (FStream)        ||
      !IS_STREAM_FORWARD (FStream)      ||
      (NameSpaceRefList == NULL)        ||
      (OutNameSpaceRefNode == NULL))
  {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  // There cannot be a method invocation in a field list. Return.
  if (AmlNodeHasAttribute (
        (CONST AML_OBJECT_NODE *)ParentNode,
        AML_HAS_FIELD_LIST
        ))
  {
    *OutNameSpaceRefNode = NULL;
    return EFI_SUCCESS;
  }

  // Allocate memory for the raw absolute path.
  RawAbsolutePathBufferSize = MAX_AML_NAMESTRING_SIZE;
  RawAbsolutePathBuffer     = AllocateZeroPool (RawAbsolutePathBufferSize);
  if (RawAbsolutePathBuffer == NULL) {
    ASSERT (0);
    return EFI_OUT_OF_RESOURCES;
  }

  // Initialize a backward stream to get the raw absolute path.
  Status = AmlStreamInit (
             &RawAbsolutePathBStream,
             RawAbsolutePathBuffer,
             RawAbsolutePathBufferSize,
             EAmlStreamDirectionBackward
             );
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    goto exit_handler;
  }

  // Build the raw AML absolute path of the namespace node.
  Status = AmlBuildRawMethodAbsolutePath (
             ParentNode,
             FStream,
             &RawAbsolutePathBStream
             );
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    goto exit_handler;
  }

  // If this is the root path: it cannot be a method invocation. Just return.
  if (AmlStreamGetIndex (&RawAbsolutePathBStream) == 0) {
    DEBUG ((
      DEBUG_VERBOSE,
      "AmlMethodParser: "
      "Root node cannot be a method invocation\n"
      ));
    *OutNameSpaceRefNode = NULL;
    Status               = EFI_SUCCESS;
    goto exit_handler;
  }

  // Create a forward stream for the raw absolute path.
  // This forward stream only contains the raw absolute path with
  // no extra free space.
  Status = AmlStreamInit (
             &RawAbsolutePathFStream,
             AmlStreamGetCurrPos (&RawAbsolutePathBStream),
             AmlStreamGetIndex (&RawAbsolutePathBStream),
             EAmlStreamDirectionForward
             );
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    goto exit_handler;
  }

  // Create a backward stream for the node name.
  Status = AmlInitRawPathBStream (
             FStream,
             &RawPathNameBStream
             );
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    return Status;
  }

  // Go through the NameSpaceRefList elements to check for
  // a corresponding method definition.
  NameSpaceRefNode = NULL;
  Status           = AmlFindMethodDefinition (
                       &RawAbsolutePathFStream,
                       &RawPathNameBStream,
                       NameSpaceRefList,
                       &NameSpaceRefNode
                       );
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    goto exit_handler;
  }

 #if !defined (MDEPKG_NDEBUG)
  // Print whether a method definition has been found.
  if (NameSpaceRefNode != NULL) {
    DEBUG ((
      DEBUG_VERBOSE,
      "AmlMethodParser: Corresponding method definition: "
      ));
    AMLDBG_PRINT_CHARS (
      DEBUG_VERBOSE,
      NameSpaceRefNode->RawAbsolutePath,
      NameSpaceRefNode->RawAbsolutePathSize
      );
    DEBUG ((DEBUG_VERBOSE, ".\n"));
  } else {
    DEBUG ((DEBUG_VERBOSE, "AmlMethodParser: No method definition found.\n"));
  }

 #endif // MDEPKG_NDEBUG

  *OutNameSpaceRefNode = NameSpaceRefNode;

exit_handler:
  // Free allocated memory.
  FreePool (RawAbsolutePathBuffer);
  return Status;
}

/** Create a namespace reference node and add it to the NameSpaceRefList.

  When a namespace node is encountered, the namespace it defines must be
  associated to the node. This allow to keep track of the nature of each
  name present in the AML namespace.

  In the end, this allows to recognize method invocations and parse the right
  number of arguments after the method name.

  @param [in]       Node              Namespace node.
  @param [in, out]  NameSpaceRefList  List of namespace reference nodes.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
**/
EFI_STATUS
EFIAPI
AmlAddNameSpaceReference (
  IN      CONST AML_OBJECT_NODE  *Node,
  IN  OUT       LIST_ENTRY       *NameSpaceRefList
  )
{
  EFI_STATUS              Status;
  AML_NAMESPACE_REF_NODE  *NameSpaceRefNode;

  AML_STREAM           NodeNameFStream;
  EAML_PARSE_INDEX     NameIndex;
  CONST AML_DATA_NODE  *NameNode;

  AML_STREAM  RawAbsolutePathBStream;
  UINT32      RawAbsolutePathBStreamSize;

  CHAR8   *AbsolutePathBuffer;
  UINT32  AbsolutePathBufferSize;

  CONST AML_NODE_HEADER  *ParentNode;

  if (!AmlNodeHasAttribute (Node, AML_IN_NAMESPACE)   ||
      (NameSpaceRefList == NULL))
  {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  // Allocate a buffer to get the raw AML absolute pathname of the
  // namespace node.
  AbsolutePathBufferSize = MAX_AML_NAMESTRING_SIZE;
  AbsolutePathBuffer     = AllocateZeroPool (AbsolutePathBufferSize);
  if (AbsolutePathBuffer == NULL) {
    ASSERT (0);
    return EFI_OUT_OF_RESOURCES;
  }

  Status = AmlStreamInit (
             &RawAbsolutePathBStream,
             (UINT8 *)AbsolutePathBuffer,
             AbsolutePathBufferSize,
             EAmlStreamDirectionBackward
             );
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    Status = EFI_INVALID_PARAMETER;
    goto exit_handler;
  }

  // Get the index where the name of the Node is stored in its
  // fixed list of arguments.
  Status = AmlNodeGetNameIndex (Node, &NameIndex);
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    Status = EFI_INVALID_PARAMETER;
    goto exit_handler;
  }

  // Get the Node name.
  NameNode = (CONST AML_DATA_NODE *)AmlGetFixedArgument (
                                      (AML_OBJECT_NODE *)Node,
                                      NameIndex
                                      );
  if (!IS_AML_DATA_NODE (NameNode)    ||
      (NameNode->DataType != EAmlNodeDataTypeNameString))
  {
    ASSERT (0);
    Status = EFI_INVALID_PARAMETER;
    goto exit_handler;
  }

  // Initialize a stream on the node name of the namespace node.
  // This is an AML NameString.
  Status = AmlStreamInit (
             &NodeNameFStream,
             NameNode->Buffer,
             NameNode->Size,
             EAmlStreamDirectionForward
             );
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    Status = EFI_INVALID_PARAMETER;
    goto exit_handler;
  }

  ParentNode = AmlGetParent ((AML_NODE_HEADER *)Node);
  if (ParentNode == NULL) {
    ASSERT (0);
    Status = EFI_INVALID_PARAMETER;
    goto exit_handler;
  }

  // Build the raw AML absolute path of the namespace node.
  Status = AmlBuildRawMethodAbsolutePath (
             ParentNode,
             &NodeNameFStream,
             &RawAbsolutePathBStream
             );
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    goto exit_handler;
  }

  RawAbsolutePathBStreamSize = AmlStreamGetIndex (&RawAbsolutePathBStream);
  // This is the root path: this cannot be a method invocation.
  if (RawAbsolutePathBStreamSize == 0) {
    Status = EFI_SUCCESS;
    goto exit_handler;
  }

  // Create a NameSpace reference node.
  Status = AmlCreateMethodRefNode (
             Node,
             (CONST CHAR8 *)AmlStreamGetCurrPos (&RawAbsolutePathBStream),
             RawAbsolutePathBStreamSize,
             &NameSpaceRefNode
             );
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    goto exit_handler;
  }

  // Add the created NameSpaceRefNode to the list.
  InsertTailList (NameSpaceRefList, &NameSpaceRefNode->Link);

  DEBUG ((
    DEBUG_VERBOSE,
    "AmlMethodParser: Adding namespace reference with name:\n"
    ));
  AMLDBG_PRINT_CHARS (
    DEBUG_VERBOSE,
    (CONST CHAR8 *)AmlStreamGetCurrPos (&RawAbsolutePathBStream),
    AmlStreamGetIndex (&RawAbsolutePathBStream)
    );
  DEBUG ((DEBUG_VERBOSE, "\n"));

exit_handler:
  // Free allocated memory.
  FreePool (AbsolutePathBuffer);

  return Status;
}

/** Create a method invocation node.

  The AML grammar does not attribute an OpCode/SubOpCode couple for
  method invocations. This library is representing method invocations
  as if they had one.

  The AML encoding for method invocations in the ACPI specification 6.3 is:
    MethodInvocation := NameString TermArgList
  In this library, it is:
    MethodInvocation := MethodInvocationOp NameString ArgumentCount TermArgList
    ArgumentCount    := ByteData

  When computing the size of a tree or serializing it, the additional data is
  not taken into account (i.e. the MethodInvocationOp and the ArgumentCount).

  Method invocation nodes have the AML_METHOD_INVOVATION attribute.

  @param  [in]  NameSpaceRefNode          NameSpaceRef node pointing to the
                                          the definition of the invoked
                                          method.
  @param  [in]  MethodInvocationName      Data node containing the method
                                          invocation name.
  @param  [out] MethodInvocationNodePtr   Created method invocation node.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_OUT_OF_RESOURCES    Could not allocate memory.
**/
EFI_STATUS
EFIAPI
AmlCreateMethodInvocationNode (
  IN  CONST AML_NAMESPACE_REF_NODE  *NameSpaceRefNode,
  IN        AML_DATA_NODE           *MethodInvocationName,
  OUT       AML_OBJECT_NODE         **MethodInvocationNodePtr
  )
{
  EFI_STATUS  Status;

  UINT8            ArgCount;
  AML_DATA_NODE    *ArgCountNode;
  AML_NODE_HEADER  **FixedArgs;
  AML_OBJECT_NODE  *MethodDefinitionNode;
  AML_OBJECT_NODE  *MethodInvocationNode;

  if ((NameSpaceRefNode == NULL)                                      ||
      !AmlIsMethodDefinitionNode (NameSpaceRefNode->NodeRef)          ||
      !IS_AML_DATA_NODE (MethodInvocationName)                        ||
      (MethodInvocationName->DataType != EAmlNodeDataTypeNameString)  ||
      (MethodInvocationNodePtr == NULL))
  {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  // Get the number of arguments of the method.
  MethodDefinitionNode = (AML_OBJECT_NODE *)NameSpaceRefNode->NodeRef;
  FixedArgs            = MethodDefinitionNode->FixedArgs;
  // The method definition is an actual method definition.
  if (AmlNodeCompareOpCode (MethodDefinitionNode, AML_METHOD_OP, 0)) {
    // Cf ACPI 6.3 specification:
    //  DefMethod := MethodOp PkgLength NameString MethodFlags TermList
    //  MethodOp := 0x14
    //  MethodFlags := ByteData  bit 0-2: ArgCount (0-7)
    //                           bit 3: SerializeFlag
    //                                    0 NotSerialized
    //                                    1 Serialized
    //                           bit 4-7: SyncLevel (0x00-0x0f)

    // Read the MethodFlags to decode the ArgCount.
    ArgCountNode = (AML_DATA_NODE *)FixedArgs[EAmlParseIndexTerm1];
    ArgCount     = *((UINT8 *)ArgCountNode->Buffer) & 0x7;
  } else if (AmlNodeCompareOpCode (MethodDefinitionNode, AML_EXTERNAL_OP, 0)) {
    // The method definition is an external statement.
    // Cf ACPI 6.3 specification:
    //  DefExternal := ExternalOp NameString ObjectType ArgumentCount
    //  ExternalOp := 0x15
    //  ObjectType := ByteData
    //  ArgumentCount := ByteData (0 - 7)

    // Read the ArgumentCount.
    ArgCountNode = (AML_DATA_NODE *)FixedArgs[EAmlParseIndexTerm2];
    ArgCount     = *((UINT8 *)ArgCountNode->Buffer);
  } else {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  // Create the object node for the method invocation.
  // MethodInvocation := MethodInvocationOp NameString ArgumentCount
  // MethodInvocationOp := Pseudo Opcode for Method Invocation
  // NameString := Method Name
  // ArgumentCount := ByteData (0 - 7)
  Status = AmlCreateObjectNode (
             AmlGetByteEncodingByOpCode (AML_METHOD_INVOC_OP, 0),
             0,
             &MethodInvocationNode
             );
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    return Status;
  }

  // The first fixed argument is the method name.
  Status = AmlSetFixedArgument (
             MethodInvocationNode,
             EAmlParseIndexTerm0,
             (AML_NODE_HEADER *)MethodInvocationName
             );
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    goto error_handler;
  }

  // Create a data node holding the number of arguments
  // of the method invocation.
  ArgCountNode = NULL;
  Status       = AmlCreateDataNode (
                   EAmlNodeDataTypeUInt,
                   &ArgCount,
                   sizeof (UINT8),
                   &ArgCountNode
                   );
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    goto error_handler;
  }

  // The second fixed argument is the number of arguments.
  Status = AmlSetFixedArgument (
             MethodInvocationNode,
             EAmlParseIndexTerm1,
             (AML_NODE_HEADER *)ArgCountNode
             );
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    goto error_handler;
  }

  *MethodInvocationNodePtr = MethodInvocationNode;
  return Status;

error_handler:
  // Delete the sub-tree: the method invocation name is already attached.
  AmlDeleteTree ((AML_NODE_HEADER *)MethodInvocationNode);
  if (ArgCountNode != NULL) {
    AmlDeleteNode ((AML_NODE_HEADER *)ArgCountNode);
  }

  return Status;
}

/** Get the number of arguments of a method invocation node.

  This function also allow to identify whether a node is a method invocation
  node. If the input node is not a method invocation node, just return.

  @param  [in]  MethodInvocationNode  Method invocation node.
  @param  [out] IsMethodInvocation    Boolean stating whether the input
                                      node is a method invocation.
  @param  [out] ArgCount              Number of arguments of the method
                                      invocation.
                                      Set to 0 if MethodInvocationNode
                                      is not a method invocation.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_BUFFER_TOO_SMALL    No space left in the buffer.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_OUT_OF_RESOURCES    Could not allocate memory.
**/
EFI_STATUS
EFIAPI
AmlGetMethodInvocationArgCount (
  IN  CONST AML_OBJECT_NODE  *MethodInvocationNode,
  OUT       BOOLEAN          *IsMethodInvocation,
  OUT       UINT8            *ArgCount
  )
{
  AML_DATA_NODE  *NumArgsNode;

  if (!IS_AML_NODE_VALID (MethodInvocationNode) ||
      (IsMethodInvocation == NULL)              ||
      (ArgCount == NULL))
  {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  // Check whether MethodInvocationNode is a method invocation.
  if (!AmlNodeCompareOpCode (MethodInvocationNode, AML_METHOD_INVOC_OP, 0)) {
    *IsMethodInvocation = FALSE;
    *ArgCount           = 0;
    return EFI_SUCCESS;
  }

  // MethodInvocation := MethodInvocationOp NameString ArgumentCount
  // MethodInvocationOp := Pseudo Opcode for Method Invocation
  // NameString := Method Name
  // ArgumentCount := ByteData (0 - 7)
  NumArgsNode = (AML_DATA_NODE *)AmlGetFixedArgument (
                                   (AML_OBJECT_NODE *)MethodInvocationNode,
                                   EAmlParseIndexTerm1
                                   );
  if (!IS_AML_NODE_VALID (NumArgsNode)                ||
      (NumArgsNode->Buffer == NULL)                   ||
      (NumArgsNode->DataType != EAmlNodeDataTypeUInt) ||
      (NumArgsNode->Size != 1))
  {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  *ArgCount = *NumArgsNode->Buffer;

  *IsMethodInvocation = TRUE;
  return EFI_SUCCESS;
}
