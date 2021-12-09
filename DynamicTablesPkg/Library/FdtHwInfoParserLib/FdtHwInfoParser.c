/** @file
  Flattened Device Tree parser library for KvmTool.

  Copyright (c) 2021, ARM Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include "FdtHwInfoParser.h"
#include "BootArch/ArmBootArchParser.h"
#include "GenericTimer/ArmGenericTimerParser.h"
#include "Gic/ArmGicDispatcher.h"
#include "Pci/ArmPciConfigSpaceParser.h"
#include "Serial/ArmSerialPortParser.h"

/** Ordered table of parsers/dispatchers.

  A parser parses a Device Tree to populate a specific CmObj type. None,
  one or many CmObj can be created by the parser.
  The created CmObj are then handed to the parser's caller through the
  HW_INFO_ADD_OBJECT interface.
  This can also be a dispatcher. I.e. a function that not parsing a
  Device Tree but calling other parsers.
*/
STATIC CONST FDT_HW_INFO_PARSER_FUNC  HwInfoParserTable[] = {
  ArmBootArchInfoParser,
  ArmGenericTimerInfoParser,
  ArmGicDispatcher,
  ArmPciConfigInfoParser,
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
STATIC
EFI_STATUS
EFIAPI
MainDispatcher (
  IN  CONST FDT_HW_INFO_PARSER_HANDLE  FdtParserHandle,
  IN        INT32                      FdtBranch
  )
{
  EFI_STATUS  Status;
  UINT32      Index;

  if (fdt_check_header (FdtParserHandle->Fdt) < 0) {
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

/** Initialise the HwInfoParser.

  The HwInfoParser shall use the information provided by the HwDataSource
  to initialise the internal state of the parser or to index the data. This
  internal state shall be linked to the ParserHandle using an implementation
  defined mechanism.

  @param [in]   HwDataSource    Pointer to the blob containing the hardware
                                information. It can be a pointer to a Device
                                Tree, an XML file, etc. or any other data
                                structure defined by the HwInfoParser.
  @param [in]   Context         A pointer to the caller's context.
  @param [in]   HwInfoAdd       Function pointer called by the parser when
                                adding information.
  @param [out]  ParserHandle    A handle to the parser instance.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
**/
EFI_STATUS
EFIAPI
HwInfoParserInit (
  IN    VOID                   *HwDataSource,
  IN    VOID                   *Context,
  IN    HW_INFO_ADD_OBJECT     HwInfoAdd,
  OUT   HW_INFO_PARSER_HANDLE  *ParserHandle
  )
{
  FDT_HW_INFO_PARSER  *FdtParserHandle;

  if ((ParserHandle == NULL)  ||
      (HwInfoAdd == NULL)     ||
      (HwDataSource == NULL)  ||
      (fdt_check_header (HwDataSource) < 0))
  {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  FdtParserHandle = AllocateZeroPool (sizeof (FDT_HW_INFO_PARSER));
  if (FdtParserHandle == NULL) {
    *ParserHandle = NULL;
    return EFI_OUT_OF_RESOURCES;
  }

  // The HwDataSource is a pointer to the FDT data.
  FdtParserHandle->Fdt       = HwDataSource;
  FdtParserHandle->Context   = Context;
  FdtParserHandle->HwInfoAdd = HwInfoAdd;

  *ParserHandle = (HW_INFO_PARSER_HANDLE)FdtParserHandle;
  return EFI_SUCCESS;
}

/** Parse the data provided by the HwDataSource.

  @param [in]  ParserHandle    A handle to the parser instance.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_OUT_OF_RESOURCES    An allocation has failed.
**/
EFI_STATUS
EFIAPI
HwInfoParse (
  IN  HW_INFO_PARSER_HANDLE  ParserHandle
  )
{
  EFI_STATUS  Status;

  if (ParserHandle == NULL) {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  // Call all the parsers from the root node (-1).
  Status = MainDispatcher (
             (FDT_HW_INFO_PARSER_HANDLE)ParserHandle,
             -1
             );
  ASSERT_EFI_ERROR (Status);
  return Status;
}

/** Cleanup any internal state and resources that were allocated
    by the the HwInfoParser.

  @param [in]  ParserHandle    A handle to the parser instance.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
**/
EFI_STATUS
EFIAPI
HwInfoParserShutdown (
  IN  HW_INFO_PARSER_HANDLE  ParserHandle
  )
{
  if (ParserHandle == NULL) {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  FreePool (ParserHandle);

  return EFI_SUCCESS;
}
