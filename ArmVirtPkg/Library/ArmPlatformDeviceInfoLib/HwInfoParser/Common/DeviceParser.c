/** @file
  Device Node Address Range Parser.

  Copyright (c) 2021 - 2024, Arm Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "FdtUtility.h"
#include "FdtInfoParser.h"

/** Parse a Device node.

  @param [in]  FdtParserHandle  A handle to the parser instance.
  @param [in]  DeviceNode       Offset of a device node.
  @param [in]  Index            Index of the device if there are multiple
                                instances of the same device type.
  @param [in]  DevDesc          A NULL terminated string containing a short
                                description of the device.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_ABORTED             An error occurred.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_UNSUPPORTED         Unsupported.
**/
STATIC
EFI_STATUS
EFIAPI
DeviceNodeParser (
  IN  CONST FDT_HW_INFO_PARSER_HANDLE  FdtParserHandle,
  IN  INT32                            DeviceNode,
  IN  CONST UINT32                     Index,
  IN  CONST CHAR8                      *DevDesc
  )
{
  EFI_STATUS   Status;
  CONST UINT8  *SizeValue;

  INT32  AddressCells;
  INT32  SizeCells;

  CONST UINT8  *Data;
  INT32        DataSize;

  // The physical base address for the device
  UINT64  BaseAddress;
  // The Base address length
  UINT64  BaseAddressLength;
  VOID    *Fdt;

  if (FdtParserHandle == NULL) {
    ASSERT (FALSE);
    return EFI_INVALID_PARAMETER;
  }

  Fdt = FdtParserHandle->Fdt;

  Status = FdtGetParentAddressInfo (
             Fdt,
             DeviceNode,
             &AddressCells,
             &SizeCells
             );
  if (EFI_ERROR (Status)) {
    ASSERT (FALSE);
    return Status;
  }

  // Don't support more than 64 bits and less than 32 bits addresses.
  if ((AddressCells < 1) ||
      (AddressCells > 2) ||
      (SizeCells < 1) ||
      (SizeCells > 2))
  {
    ASSERT (FALSE);
    return EFI_ABORTED;
  }

  Data = fdt_getprop (Fdt, DeviceNode, "reg", &DataSize);
  if ((Data == NULL) ||
      (DataSize < (INT32)(sizeof (UINT32) *
                          GET_DT_REG_ADDRESS_OFFSET (1, AddressCells, SizeCells)) - 1))
  {
    // If error or not enough space.
    ASSERT (FALSE);
    return EFI_ABORTED;
  }

  if (AddressCells == 2) {
    BaseAddress = FdtReadUnaligned64 ((UINT64 *)Data);
  } else {
    BaseAddress = FdtReadUnaligned32 ((UINT32 *)Data);
  }

  SizeValue = Data + (sizeof (UINT32) *
                      GET_DT_REG_SIZE_OFFSET (0, AddressCells, SizeCells));
  if (SizeCells == 2) {
    BaseAddressLength = FdtReadUnaligned64 ((UINT64 *)SizeValue);
  } else {
    BaseAddressLength = FdtReadUnaligned32 ((UINT32 *)SizeValue);
  }

  if (FdtParserHandle->HwAddressInfo != NULL) {
    Status = FdtParserHandle->HwAddressInfo (
                                FdtParserHandle->Context,
                                DevDesc,
                                BaseAddress,
                                BaseAddressLength
                                );
    ASSERT_EFI_ERROR (Status);
  }

  return Status;
}

/** Device dispatcher.

  @param [in]  FdtParserHandle  A handle to the parser instance.
  @param [in]  FdtBranch        When searching for DT node name, restrict
                                the search to this Device Tree branch.
  @param [in]  CompatibleInfo   List of "compatible" property values for
                                the device.
  @param [in]  DevDesc          A NULL terminated string containing a short
                                description of the device.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_ABORTED             An error occurred.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_NOT_FOUND           Not found.
  @retval EFI_UNSUPPORTED         Unsupported.
**/
EFI_STATUS
EFIAPI
DeviceDispatcher (
  IN  CONST FDT_HW_INFO_PARSER_HANDLE  FdtParserHandle,
  IN        INT32                      FdtBranch,
  IN  CONST COMPATIBILITY_INFO         *CompatibleInfo,
  IN  CONST CHAR8                      *DevDesc
  )
{
  EFI_STATUS  Status;
  INT32       DevNode;
  UINT32      Index;
  UINT32      DevNodeCount;
  VOID        *Fdt;

  if (FdtParserHandle == NULL) {
    ASSERT (FALSE);
    return EFI_INVALID_PARAMETER;
  }

  Fdt = FdtParserHandle->Fdt;

  // Count the number of device nodes.
  Status = FdtCountCompatNodeInBranch (
             Fdt,
             FdtBranch,
             CompatibleInfo,
             &DevNodeCount
             );
  if (EFI_ERROR (Status)) {
    ASSERT (FALSE);
    return Status;
  }

  if (DevNodeCount == 0) {
    return EFI_NOT_FOUND;
  }

  DevNode = FdtBranch;
  for (Index = 0; Index < DevNodeCount; Index++) {
    // Search the next device node in the branch.
    Status = FdtGetNextCompatNodeInBranch (
               Fdt,
               FdtBranch,
               CompatibleInfo,
               &DevNode
               );
    if (EFI_ERROR (Status)) {
      ASSERT (FALSE);
      if (Status == EFI_NOT_FOUND) {
        // Should have found the node.
        Status = EFI_ABORTED;
      }

      goto exit_handler;
    }

    Status = DeviceNodeParser (
               FdtParserHandle,
               DevNode,
               Index,
               DevDesc
               );
    if (EFI_ERROR (Status)) {
      ASSERT (FALSE);
      goto exit_handler;
    }
  } // for

exit_handler:
  return Status;
}
