/** @file
  AML Method Parser.

  Copyright (c) 2019 - 2020, Arm Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef AML_METHOD_PARSER_H_
#define AML_METHOD_PARSER_H_

#include <AmlNodeDefines.h>
#include <Stream/AmlStream.h>

/** AML namespace reference node.

  Namespace reference nodes allow to associate an AML absolute pathname
  to the tree node defining this object in the namespace.

  Namespace reference nodes are stored in a separate list. They are not part of
  the tree.
*/
typedef struct AmlNameSpaceRefNode {
  /// Double linked list.
  /// This must be the first field in this structure.
  LIST_ENTRY                Link;

  /// Node part of the AML namespace. It must have the AML_IN_NAMESPACE
  /// attribute.
  CONST AML_OBJECT_NODE   * NodeRef;

  /// Raw AML absolute pathname of the NodeRef.
  /// This is a raw AML NameString (cf AmlNameSpace.c: A concatenated list
  /// of 4 chars long names. The dual/multi NameString prefix have been
  /// stripped.).
  CONST CHAR8             * RawAbsolutePath;

  /// Size of the raw AML absolute pathname buffer.
  UINT32                    RawAbsolutePathSize;
} AML_NAMESPACE_REF_NODE;

/** Delete a list of namespace reference nodes.

  @param  [in]  NameSpaceRefList    List of namespace reference nodes.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
**/
EFI_STATUS
EFIAPI
AmlDeleteNameSpaceRefList (
  IN  LIST_ENTRY      * NameSpaceRefList
  );


#if !defined (MDEPKG_NDEBUG)
/** Print the list of raw absolute paths of the NameSpace reference list.

  @param  [in]    NameSpaceRefList    List of NameSpace reference nodes.
**/
VOID
EFIAPI
AmlDbgPrintNameSpaceRefList (
  IN  CONST LIST_ENTRY    * NameSpaceRefList
  );

#endif // MDEPKG_NDEBUG

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
  IN  CONST AML_NODE_HEADER     * ParentNode,
  IN  CONST AML_STREAM          * FStream,
  IN  CONST LIST_ENTRY          * NameSpaceRefList,
  OUT AML_NAMESPACE_REF_NODE   ** OutNameSpaceRefNode
  );

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
  IN      CONST AML_OBJECT_NODE   * Node,
  IN  OUT       LIST_ENTRY        * NameSpaceRefList
  );

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
  IN  CONST AML_NAMESPACE_REF_NODE   * NameSpaceRefNode,
  IN        AML_DATA_NODE            * MethodInvocationName,
  OUT       AML_OBJECT_NODE         ** MethodInvocationNodePtr
  );

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
*/
EFI_STATUS
EFIAPI
AmlGetMethodInvocationArgCount (
  IN  CONST AML_OBJECT_NODE   * MethodInvocationNode,
  OUT       BOOLEAN           * IsMethodInvocation,
  OUT       UINT8             * ArgCount
  );

#endif // AML_METHOD_PARSER_H_
