/** @file
  AML Debug Print.

  Copyright (c) 2010 - 2018, Intel Corporation. All rights reserved. <BR>
  Copyright (c) 2019 - 2020, Arm Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef AML_PRINT_H_
#define AML_PRINT_H_

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

#if !defined (MDEPKG_NDEBUG)

#include <AmlInclude.h>

/**
  @defgroup DbgPrintApis Print APIs for debugging.
  @ingroup AMLLib
  @{
    Print APIs provide a way to print:
     - A buffer;
     - A (root/object/data) node;
     - An AML tree/branch;
     - The AML NameSpace from the root node.
  @}
*/

/** This function performs a raw data dump of the ACPI table.

  @param  [in]  Ptr     Pointer to the start of the table buffer.
  @param  [in]  Length  The length of the buffer.
**/
VOID
EFIAPI
AmlDbgDumpRaw (
  IN  CONST UINT8   * Ptr,
  IN        UINT32    Length
  );

/** Print Size chars at Buffer address.

  @ingroup DbgPrintApis

  @param  [in]  ErrorLevel    Error level for the DEBUG macro.
  @param  [in]  Buffer        Buffer containing the chars.
  @param  [in]  Size          Number of chars to print.
**/
VOID
EFIAPI
AmlDbgPrintChars (
  IN        UINT32      ErrorLevel,
  IN  CONST CHAR8     * Buffer,
  IN        UINT32      Size
  );

/** Print an AML NameSeg.
    Don't print trailing underscores ('_').

  @param  [in] Buffer   Buffer containing an AML NameSeg.
**/
VOID
EFIAPI
AmlDbgPrintNameSeg (
  IN  CONST CHAR8   * Buffer
  );

/** Print an AML NameString.

  @param  [in] Buffer   Buffer containing an AML NameString.
  @param  [in] NewLine  Print a newline char at the end of the NameString.
**/
VOID
EFIAPI
AmlDbgPrintNameString (
  IN  CONST CHAR8   * Buffer,
  IN        BOOLEAN   NewLine
  );

/** Print Node information.

  @ingroup DbgPrintApis

  @param  [in]  Node    Pointer to the Node to print.
                        Can be a root/object/data node.
**/
VOID
EFIAPI
AmlDbgPrintNode (
  IN  AML_NODE_HANDLE   Node
  );

/** Recursively print the subtree under the Node.

  @ingroup DbgPrintApis

  @param  [in]  Node    Pointer to the root of the subtree to print.
                        Can be a root/object/data node.
**/
VOID
EFIAPI
AmlDbgPrintTree (
  IN  AML_NODE_HANDLE   Node
  );

/** Print the absolute pathnames in the AML namespace of
    all the nodes in the tree starting from the Root node.

  @ingroup DbgPrintApis

  @param  [in]  RootNode    Pointer to a root node.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_BUFFER_TOO_SMALL    No space left in the buffer.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_OUT_OF_RESOURCES    Out of memory.
**/
EFI_STATUS
EFIAPI
AmlDbgPrintNameSpace (
  IN  AML_ROOT_NODE_HANDLE  RootNode
  );

/* Macros to encapsulate Aml Debug Print APIs.
*/

#define AMLDBG_DUMP_RAW(Ptr, Length)                  \
          AmlDbgDumpRaw (Ptr, Length)

#define AMLDBG_PRINT_CHARS(ErrorLevel, Buffer, Size)  \
          AmlDbgPrintChars (ErrorLevel, Buffer, Size)

#define AMLDBG_PRINT_NAMESEG(Buffer)                  \
          AmlDbgPrintNameSeg (Buffer)

#define AMLDBG_PRINT_NAMESTR(Buffer,NewLine)          \
          AmlDbgPrintNameString (Buffer,NewLine)

#define AMLDBG_PRINT_NODE(Node)                       \
          AmlDbgPrintNode (Node)

#define AMLDBG_PRINT_TREE(Node)                       \
          AmlDbgPrintTree (Node)

#define AMLDBG_PRINT_NAMESPACE(RootNode)              \
          AmlDbgPrintNameSpace (RootNode)

#else

#define AMLDBG_DUMP_RAW(Ptr, Length)

#define AMLDBG_PRINT_CHARS(ErrorLevel, Buffer, Size)

#define AMLDBG_PRINT_NAMESEG(Buffer)

#define AMLDBG_PRINT_NAMESTR(Buffer,NewLine)

#define AMLDBG_PRINT_NODE(Node)

#define AMLDBG_PRINT_TREE(Node)

#define AMLDBG_PRINT_NAMESPACE(RootNode)

#endif // MDEPKG_NDEBUG

#endif // AML_PRINT_H_
