/** @file
  AML Utility.

  Copyright (c) 2019 - 2021, Arm Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef AML_UTILITY_H_
#define AML_UTILITY_H_

#include <AmlNodeDefines.h>

/** This function computes and updates the ACPI table checksum.

  @param  [in]  AcpiTable   Pointer to an Acpi table.

  @retval EFI_SUCCESS   The function completed successfully.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
**/
EFI_STATUS
EFIAPI
AcpiPlatformChecksum (
  IN  EFI_ACPI_DESCRIPTION_HEADER  * AcpiTable
  );

/** Compute the size of a tree/sub-tree.

  @param  [in]      Node      Node to compute the size.
  @param  [in, out] Size      Pointer holding the computed size.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
**/
EFI_STATUS
EFIAPI
AmlComputeSize (
  IN      CONST AML_NODE_HEADER   * Node,
  IN  OUT       UINT32            * Size
  );

/** Get the value contained in an integer node.

  @param  [in]  Node    Pointer to an integer node.
                        Must be an object node.
  @param  [out] Value   Value contained in the integer node.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
**/
EFI_STATUS
EFIAPI
AmlNodeGetIntegerValue (
  IN  AML_OBJECT_NODE   * Node,
  OUT UINT64            * Value
  );

/** Set the value contained in an integer node.

  The OpCode is updated accordingly to the new value
  (e.g.: If the original value was a UINT8 value, then the OpCode
         would be AML_BYTE_PREFIX. If it the new value is a UINT16
         value then the OpCode will be updated to AML_WORD_PREFIX).

  @param  [in]  Node            Pointer to an integer node.
                                Must be an object node.
  @param  [in]  NewValue        New value to write in the integer node.
  @param  [out] ValueWidthDiff  Difference in number of bytes used to store
                                the new value.
                                Can be negative.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_OUT_OF_RESOURCES    Could not allocate memory.
**/
EFI_STATUS
EFIAPI
AmlNodeSetIntegerValue (
  IN  AML_OBJECT_NODE   * Node,
  IN  UINT64              NewValue,
  OUT INT8              * ValueWidthDiff
  );

/** Propagate information up the tree.

  The information can be a new size, a new number of arguments.

  @param  [in]  Node          Pointer to a node.
                              Must be a root node or an object node.
  @param  [in]  IsIncrement   Choose the operation to do:
                               - TRUE:  Increment the Node's size and
                                        the Node's count;
                               - FALSE: Decrement the Node's size and
                                        the Node's count.
  @param  [in]  Diff          Value to add/subtract to the Node's size.
  @param  [in]  NodeCount     Number of nodes added/removed.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
**/
EFI_STATUS
EFIAPI
AmlPropagateInformation (
  IN  AML_NODE_HEADER   * Node,
  IN  BOOLEAN             IsIncrement,
  IN  UINT32              Diff,
  IN  UINT8               NodeCount
  );

/** Find and set the EndTag's Checksum of a list of Resource Data elements.

  Lists of Resource Data elements end with an EndTag (most of the time). This
  function finds the EndTag (if present) in a list of Resource Data elements
  and sets the checksum.

  ACPI 6.4, s6.4.2.9 "End Tag":
  "This checksum is generated such that adding it to the sum of all the data
  bytes will produce a zero sum."
  "If the checksum field is zero, the resource data is treated as if the
  checksum operation succeeded. Configuration proceeds normally."

  To avoid re-computing checksums, if a new resource data elements is
  added/removed/modified in a list of resource data elements, the AmlLib
  resets the checksum to 0.

  @param [in]  BufferOpNode   Node having a list of Resource Data elements.
  @param [in]  CheckSum       CheckSum to store in the EndTag.
                              To ignore/avoid computing the checksum,
                              give 0.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_NOT_FOUND           No EndTag found.
**/
EFI_STATUS
EFIAPI
AmlSetRdListCheckSum (
  IN  AML_OBJECT_NODE   * BufferOpNode,
  IN  UINT8               CheckSum
  );

#endif // AML_UTILITY_H_

