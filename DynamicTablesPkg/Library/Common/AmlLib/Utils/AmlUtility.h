/** @file
  AML Utility.

  Copyright (c) 2019 - 2020, Arm Limited. All rights reserved.<BR>

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

#endif // AML_UTILITY_H_

