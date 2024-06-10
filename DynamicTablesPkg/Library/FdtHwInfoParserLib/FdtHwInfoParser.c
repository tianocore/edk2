/** @file
  Flattened Device Tree parser library for KvmTool.

  Copyright (c) 2021, ARM Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include "FdtHwInfoParser.h"

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
  Status = ArchFdtHwInfoMainDispatcher (
             (FDT_HW_INFO_PARSER_HANDLE)ParserHandle,
             -1
             );
  ASSERT_EFI_ERROR (Status);
  return Status;
}

/** Cleanup any internal state and resources that were allocated
    by the HwInfoParser.

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
