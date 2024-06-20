/** @file
  Flattened Device Tree parser library for RISC-V.

  Copyright (c) 2024, Ventana Micro Systems Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Library/FdtLib.h>
#include "FdtHwInfoParser.h"
#include "Pci/PciConfigSpaceParser.h"
#include "Serial/SerialPortParser.h"
#include "Intc/RiscVIntcParser.h"
#include "HartInfo/RiscVHartInfoDispatcher.h"

/** Ordered table of parsers/dispatchers.

  A parser parses a Device Tree to populate a specific CmObj type. None,
  one or many CmObj can be created by the parser.
  The created CmObj are then handed to the parser's caller through the
  HW_INFO_ADD_OBJECT interface.
  This can also be a dispatcher. I.e. a function that not parsing a
  Device Tree but calling other parsers.
*/
STATIC CONST FDT_HW_INFO_PARSER_FUNC  HwInfoParserTable[] = {
  RiscVIntcDispatcher,
  RiscVHartInfoDispatcher,
  PciConfigInfoParser,
  SerialPortDispatcher
};

/** Main dispatcher: sequentially call the parsers/dispatchers
    of the HwInfoParserTable.

  A parser parses a Device Tree to populate a specific CmObj type. None,
  one or many CmObj can be created by the parser.
  The created CmObj are then handed to the parser's caller through the
  HW_INFO_ADD_OBJECT interface.
  This can also be a dispatcher. I.e. a function that not parsing a
  Device Tree but calling other parsers.

  @param [in]  FdtParserHandle A handle to the parser instance.
  @param [in]  FdtBranch       When searching for DT node name, restrict
                               the search to this Device Tree branch.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_ABORTED             An error occurred.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_NOT_FOUND           Not found.
  @retval EFI_UNSUPPORTED         Unsupported.
**/
EFI_STATUS
EFIAPI
ArchFdtHwInfoMainDispatcher (
  IN  CONST FDT_HW_INFO_PARSER_HANDLE  FdtParserHandle,
  IN        INT32                      FdtBranch
  )
{
  EFI_STATUS  Status;
  UINT32      Index;

  if (FdtCheckHeader (FdtParserHandle->Fdt) < 0) {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  for (Index = 0; Index < ARRAY_SIZE (HwInfoParserTable); Index++) {
    Status = HwInfoParserTable[Index](
                                      FdtParserHandle,
                                      FdtBranch
                                      );
    if (EFI_ERROR (Status)  &&
        (Status != EFI_NOT_FOUND))
    {
      // If EFI_NOT_FOUND, the parser didn't find information in the DT.
      // Don't trigger an error.
      ASSERT (0);
      return Status;
    }
  } // for

  return EFI_SUCCESS;
}
