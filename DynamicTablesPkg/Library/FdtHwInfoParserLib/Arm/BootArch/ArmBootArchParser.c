/** @file
  Arm boot architecture parser.

  Copyright (c) 2021, ARM Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Reference(s):
  - linux/Documentation/devicetree/bindings/arm/psci.yaml
**/

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include "FdtHwInfoParser.h"
#include "CmObjectDescUtility.h"
#include "Arm/BootArch/ArmBootArchParser.h"

/** List of "compatible" property values for Psci nodes.

  Other "compatible" values are not supported by this module.
*/
STATIC CONST COMPATIBILITY_STR  PsciCompatibleStr[] = {
  { "arm,psci-0.2" },
  { "arm,psci"     }
};

/** COMPATIBILITY_INFO structure for the PsciCompatibleInfo.
*/
STATIC CONST COMPATIBILITY_INFO  PsciCompatibleInfo = {
  ARRAY_SIZE (PsciCompatibleStr),
  PsciCompatibleStr
};

/** List of PSCI method strings.
*/
STATIC CONST CHAR8  *PsciMethod[] = {
  "smc",
  "hvc"
};

/** Parse a Psci node.

  @param [in]  Fdt            Pointer to a Flattened Device Tree (Fdt).
  @param [in]  PsciNode       Offset of a Psci node.
  @param [in]  BootArchInfo   The CM_ARM_BOOT_ARCH_INFO to populate.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_ABORTED             An error occurred.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
**/
STATIC
EFI_STATUS
EFIAPI
PsciNodeParser (
  IN  CONST VOID                   *Fdt,
  IN        INT32                  PsciNode,
  IN        CM_ARM_BOOT_ARCH_INFO  *BootArchInfo
  )
{
  CONST VOID  *Data;
  INT32       DataSize;

  if ((Fdt == NULL) ||
      (BootArchInfo == NULL))
  {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  // Default to parking protocol
  BootArchInfo->BootArchFlags = 0;

  Data = fdt_getprop (Fdt, PsciNode, "method", &DataSize);
  if ((Data == NULL) || (DataSize < 0)) {
    ASSERT (0);
    return EFI_ABORTED;
  }

  // Check PSCI conduit.
  if (AsciiStrnCmp (Data, PsciMethod[0], DataSize) == 0) {
    BootArchInfo->BootArchFlags = EFI_ACPI_6_3_ARM_PSCI_COMPLIANT;
  } else if (AsciiStrnCmp (Data, PsciMethod[1], DataSize) == 0) {
    BootArchInfo->BootArchFlags = (EFI_ACPI_6_3_ARM_PSCI_COMPLIANT |
                                   EFI_ACPI_6_3_ARM_PSCI_USE_HVC);
  }

  return EFI_SUCCESS;
}

/** CM_ARM_BOOT_ARCH_INFO parser function.

  The following structure is populated:
  typedef struct CmArmBootArchInfo {
    UINT16  BootArchFlags;                    // {Populated}
  } CM_ARM_BOOT_ARCH_INFO;

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
ArmBootArchInfoParser (
  IN  CONST FDT_HW_INFO_PARSER_HANDLE  FdtParserHandle,
  IN        INT32                      FdtBranch
  )
{
  EFI_STATUS             Status;
  INT32                  PsciNode;
  CM_ARM_BOOT_ARCH_INFO  BootArchInfo;

  if (FdtParserHandle == NULL) {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  ZeroMem (&BootArchInfo, sizeof (CM_ARM_BOOT_ARCH_INFO));

  PsciNode = FdtBranch;
  Status   = FdtGetNextCompatNodeInBranch (
               FdtParserHandle->Fdt,
               FdtBranch,
               &PsciCompatibleInfo,
               &PsciNode
               );
  if (EFI_ERROR (Status)) {
    // Error, or no node found.
    ASSERT (Status == EFI_NOT_FOUND);
    return Status;
  }

  // Parse the psci node.
  Status = PsciNodeParser (FdtParserHandle->Fdt, PsciNode, &BootArchInfo);
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    return Status;
  }

  // Add the CmObj to the Configuration Manager.
  Status = AddSingleCmObj (
             FdtParserHandle,
             CREATE_CM_ARM_OBJECT_ID (EArmObjBootArchInfo),
             &BootArchInfo,
             sizeof (CM_ARM_BOOT_ARCH_INFO),
             NULL
             );
  ASSERT_EFI_ERROR (Status);
  return Status;
}
