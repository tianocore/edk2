/** @file
  A Flattened Device Tree parser that scans the platform devices to
  retrieve the base address and the range.

  Copyright (c) 2021 - 2024, Arm Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Base.h>
#include <Library/BaseLib.h>
#include <Library/PlatformDeviceInfoLib.h>

#include "FdtUtility.h"
#include "FdtInfoParser.h"
#include "Arm/Gic/ArmGicDispatcher.h"
#include "PciConfigSpaceParser.h"
#include "SerialPortParser.h"
#include "RtcParser.h"

/** Function pointer to a parser function.

  @param [in]  ParserHandle    Handle to the parser instance.
  @param [in]  FdtBranch       When searching for DT node name, restrict
                               the search to this Device Tree branch.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_ABORTED             An error occurred.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_NOT_FOUND           Not found.
  @retval EFI_UNSUPPORTED         Unsupported.
**/
typedef
EFI_STATUS
(EFIAPI *FDT_HW_INFO_PARSER_FUNC)(
  IN  CONST FDT_HW_INFO_PARSER_HANDLE ParserHandle,
  IN        INT32                     FdtBranch
  );

/** Ordered table of parsers/dispatchers.

  A parser parses a Device Tree to retrieve relevant
  hardware information.

  This can also be a dispatcher, i.e. a function that
  is not parsing a Device Tree but calling other parsers.
*/
STATIC CONST FDT_HW_INFO_PARSER_FUNC  HwInfoParserTable[] = {
  ArmGicDispatcher,
  PciConfigInfoParser,
  SerialPortDispatcher,
  RtcDispatcher
};

/** Main dispatcher: sequentially call the parsers/dispatchers
    of the HwInfoParserTable.

  @param [in]  FdtParserHandle A handle to the parser instance.
  @param [in]  FdtBranch       When searching for DT node name, restrict
                               the search to this Device Tree branch.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_NOT_FOUND           Parser did not find any device information
                                  in the FDT.
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
    ASSERT (FALSE);
    return EFI_INVALID_PARAMETER;
  }

  for (Index = 0; Index < ARRAY_SIZE (HwInfoParserTable); Index++) {
    Status = HwInfoParserTable[Index](
                                      FdtParserHandle,
                                      FdtBranch
                                      );
    if (EFI_ERROR (Status) &&
        (Status != EFI_NOT_FOUND))
    {
      // If EFI_NOT_FOUND, the parser didn't find information in the DT.
      // Don't trigger an error.
      ASSERT (FALSE);
      return Status;
    }
  } // for

  return EFI_SUCCESS;
}

/** A callback function that is called with the information about
    the platform device found in the FDT and is used to populate
    the PLATFORM_DEVICE_INFO structure.

  @param [in]  Context    Pointer to the platform device information structure.
  @param [in]  Desc       A NULL terminated string describing the device.
  @param [in]  Base       Base address of the device.
  @param [in]  Range      Base address range of the device.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_OUT_OF_RESOURCES    Maximum platform device count exceeded.
**/
STATIC
EFI_STATUS
EFIAPI
HwAddressInfo (
  IN        VOID    *Context,
  IN  CONST CHAR8   *Desc,
  IN        UINT64  Base,
  IN        UINT64  Range
  )
{
  PLATFORM_DEVICE_INFO  *PlatInfo;
  UINTN                 Index;

  if (Context == NULL) {
    ASSERT (FALSE);
    return EFI_INVALID_PARAMETER;
  }

  DEBUG ((DEBUG_INFO, "%a: 0x%llx - 0x%llx\n", Desc, Base, Range));

  // The address range cannot be zero.
  if (Range == 0) {
    DEBUG ((
      DEBUG_ERROR,
      "Error: Invalid device range: %a: 0x%llx - 0x%llx\n",
      Desc,
      Base,
      Range
      ));
    return EFI_INVALID_PARAMETER;
  }

  PlatInfo = (PLATFORM_DEVICE_INFO *)Context;
  if (PlatInfo->MaxDevices >= MAX_PLAT_DEVICE_COUNT) {
    DEBUG ((DEBUG_ERROR, "Error: Maximum platform device count exceeded!\n"));
    ASSERT (FALSE);
    return EFI_OUT_OF_RESOURCES;
  }

  Index                            = PlatInfo->MaxDevices++;
  PlatInfo->Dev[Index].BaseAddress = Base;
  PlatInfo->Dev[Index].Length      = Range;
  AsciiStrnCpyS (PlatInfo->Dev[Index].Desc, 16, Desc, AsciiStrLen (Desc));

  return EFI_SUCCESS;
}

/** Parse the platform FDT and populate the platform device info.

  @param [in]  FdtBase     Pointer to the platform FDT.
  @param [in]  PlatInfo    Pointer to the platform device info to populate.

  @retval EFI_SUCCESS             Success.
  @retval EFI_INVALID_PARAMETER   A parameter is invalid.
  @retval EFI_NOT_FOUND           Parser did not find any device information
                                  in the FDT.
**/
EFI_STATUS
EFIAPI
ParsePlatformDeviceFdt (
  IN  VOID                  *FdtBase,
  IN  PLATFORM_DEVICE_INFO  *PlatInfo
  )
{
  FDT_HW_INFO_PARSER  Parser;

  if ((FdtBase == NULL) ||
      (PlatInfo == NULL) ||
      (fdt_check_header (FdtBase) != 0))
  {
    return EFI_INVALID_PARAMETER;
  }

  Parser.Fdt           = FdtBase;
  Parser.HwAddressInfo = HwAddressInfo;
  Parser.Context       = PlatInfo;

  return MainDispatcher (&Parser, 0);
}
