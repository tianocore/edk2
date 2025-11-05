/** @file
  Arm Gic cpu parser.

  Copyright (c) 2021 - 2022, Arm Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Reference(s):
  - linux/Documentation/devicetree/bindings/interrupt-controller/arm,gic.yaml
  - linux/Documentation/devicetree/bindings/interrupt-controller/arm,gic-v3.yaml
**/

#ifndef ARM_GICC_PARSER_H_
#define ARM_GICC_PARSER_H_

/* According to BSA 1.0 s3.6 PPI assignments, PMU IRQ ID is 23.
*/
#define BSA_PMU_IRQ  23

/** CM_ARM_GICC_INFO parser function.

  This parser expects FdtBranch to be the "\cpus" node node.
  At most one CmObj is created.
  The following structure is populated:
  typedef struct CmArmGicCInfo {
    UINT32  CPUInterfaceNumber;               // {Populated}
    UINT32  AcpiProcessorUid;                 // {Populated}
    UINT32  Flags;                            // {Populated}
    UINT32  ParkingProtocolVersion;           // {default = 0}
    UINT32  PerformanceInterruptGsiv;         // {Populated}
    UINT64  ParkedAddress;                    // {default = 0}
    UINT64  PhysicalBaseAddress;              // {Populated}
    UINT64  GICV;                             // {Populated}
    UINT64  GICH;                             // {Populated}
    UINT32  VGICMaintenanceInterrupt;         // {Populated}
    UINT64  GICRBaseAddress;                  // {default = 0}
    UINT64  MPIDR;                            // {Populated}
    UINT8   ProcessorPowerEfficiencyClass;    // {default = 0}
    UINT16  SpeOverflowInterrupt;             // {default = 0}
    UINT32  ProximityDomain;                  // {default = 0}
    UINT32  ClockDomain;                      // {default = 0}
    UINT32  AffinityFlags;                    // {default = 0}
  } CM_ARM_GICC_INFO;

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
ArmGicCInfoParser (
  IN  CONST FDT_HW_INFO_PARSER_HANDLE  FdtParserHandle,
  IN        INT32                      FdtBranch
  );

#endif // ARM_GICC_PARSER_H_
