/** @file
  RISC-V PLIC/APLIC Map.

  Copyright (c) 2024, Ventana Micro Systems Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Reference(s):
  - TBD
**/

#include <Library/AcpiLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <ConfigurationManagerObject.h>
#include <ConfigurationManagerHelper.h>
#include <Protocol/ConfigurationManagerProtocol.h>

/** RISC-V SSDT PLIC/APLIC namespace device Generator.

Requirements:
  The following Configuration Manager Object(s) are used by
  this Generator:
  - ERiscVObjAplicInfo
  - ERiscVObjPlicInfo
*/

/** This macro expands to a function that retrieves the APLIC
    Information from the Configuration Manager.
*/
GET_OBJECT_LIST (
  EObjNameSpaceRiscV,
  ERiscVObjAplicInfo,
  CM_RISCV_APLIC_INFO
  );

/** This macro expands to a function that retrieves the PLIC
    Information from the Configuration Manager.
*/

GET_OBJECT_LIST (
  EObjNameSpaceRiscV,
  ERiscVObjPlicInfo,
  CM_RISCV_PLIC_INFO
  );

/**
  Get GSI number for the interrupt number.

  On RISC-V, GSIs are divided across PLICs or APLICs. So, the interrupt
  number from interrupt map in DT should be converted to appropriate GSI
  number using the interrupt controller phandle and GSI base of each PLIC
  or APLIC.

  @param  CfgMgrProtocol     Pointer to the Configuration Manager
                             Protocol interface.
  @param  IrqId              The IRQ number to convert to GSI.
  @param  IntcPhandle        Reference to the interrupt controller.

**/
UINT32
ArchGetGsiIrqId (
  IN      CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  CfgMgrProtocol,
  IN UINT32                                                   IrqId,
  IN INT32                                                    IntcPhandle
  )
{
  CM_RISCV_APLIC_INFO  *AplicInfo;
  CM_RISCV_PLIC_INFO   *PlicInfo;
  EFI_STATUS           Status;
  UINT32               Index, Count;

  Status = GetERiscVObjAplicInfo (
             CfgMgrProtocol,
             CM_NULL_TOKEN,
             &AplicInfo,
             &Count
             );
  if (!EFI_ERROR (Status)) {
    for (Index = 0; Index < Count; Index++) {
      if (AplicInfo[Index].Phandle == IntcPhandle) {
        return IrqId + AplicInfo[Index].GsiBase;
      }
    }

    ASSERT (0);
    return IrqId;
  } else if (Status == EFI_NOT_FOUND) {
    Status = GetERiscVObjPlicInfo (
               CfgMgrProtocol,
               CM_NULL_TOKEN,
               &PlicInfo,
               &Count
               );
    if (!EFI_ERROR (Status)) {
      for (Index = 0; Index < Count; Index++) {
        if (PlicInfo[Index].Phandle == IntcPhandle) {
          return IrqId + PlicInfo[Index].GsiBase;
        }
      }

      ASSERT (0);
      return IrqId;
    }
  } else {
    ASSERT (0);
  }

  return IrqId;
}
