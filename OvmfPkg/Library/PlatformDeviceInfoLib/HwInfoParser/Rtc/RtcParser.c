/** @file
  RTC Parser.

  Copyright (c) 2021 - 2024, Arm Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Reference(s):
  - Documentation/devicetree/bindings/rtc/arm,pl031.yaml
  - Documentation/devicetree/bindings/rtc/rtc-cmos.txt
**/

#include "FdtUtility.h"
#include "FdtInfoParser.h"
#include "Common/DeviceParser.h"

/** List of "compatible" property values for RTC nodes.

  Any other "compatible" value is not supported by this module.
*/
STATIC CONST COMPATIBILITY_STR  RtcCompatibleStr[] = {
  { "motorola,mc146818" },
  { "arm,pl031"         }
};

/** COMPATIBILITY_INFO structure for the RtcCompatible.
*/
CONST COMPATIBILITY_INFO  RtcCompatibleInfo = {
  ARRAY_SIZE (RtcCompatibleStr),
  RtcCompatibleStr
};

/** RTC dispatcher.

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
RtcDispatcher (
  IN  CONST FDT_HW_INFO_PARSER_HANDLE  FdtParserHandle,
  IN        INT32                      FdtBranch
  )
{
  if (FdtParserHandle == NULL) {
    ASSERT (FALSE);
    return EFI_INVALID_PARAMETER;
  }

  return DeviceDispatcher (
           FdtParserHandle,
           FdtBranch,
           &RtcCompatibleInfo,
           "RTC"
           );
}
