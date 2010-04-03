/** @file
  Support for the latest PCI standard.

  Copyright (c) 2006 - 2009, Intel Corporation
  All rights reserved. This program and the accompanying materials                          
  are licensed and made available under the terms and conditions of the BSD License         
  which accompanies this distribution.  The full text of the license may be found at        
  http://opensource.org/licenses/bsd-license.php                                            

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

**/

#ifndef _PCIEXPRESS21_H_
#define _PCIEXPRESS21_H_

#define EFI_PCIE_CAPABILITY_BASE_OFFSET                             0x100
#define EFI_PCIE_CAPABILITY_ID_SRIOV_CONTROL_ARI_HIERARCHY          0x10
#define EFI_PCIE_CAPABILITY_DEVICE_CAPABILITIES_2_OFFSET            0x24
#define EFI_PCIE_CAPABILITY_DEVICE_CAPABILITIES_2_ARI_FORWARDING    0x20
#define EFI_PCIE_CAPABILITY_DEVICE_CONTROL_2_OFFSET                 0x28
#define EFI_PCIE_CAPABILITY_DEVICE_CONTROL_2_ARI_FORWARDING         0x20

//
// for SR-IOV
//
#define EFI_PCIE_CAPABILITY_ID_ARI        0x0E
#define EFI_PCIE_CAPABILITY_ID_ATS        0x0F
#define EFI_PCIE_CAPABILITY_ID_SRIOV      0x10
#define EFI_PCIE_CAPABILITY_ID_MRIOV      0x11

typedef struct {
  UINT32  CapabilityHeader;
  UINT32  Capability;
  UINT16  Control;
  UINT16  Status;
  UINT16  InitialVFs;
  UINT16  TotalVFs;
  UINT16  NumVFs;
  UINT8   FunctionDependencyLink;
  UINT8   Reserved0;
  UINT16  FirstVFOffset;
  UINT16  VFStride;
  UINT16  Reserved1;
  UINT16  VFDeviceID;
  UINT32  SupportedPageSize;
  UINT32  SystemPageSize;
  UINT32  VFBar[6];
  UINT32  VFMigrationStateArrayOffset;
} SR_IOV_CAPABILITY_REGISTER;

#define EFI_PCIE_CAPABILITY_ID_SRIOV_CAPABILITIES               0x04
#define EFI_PCIE_CAPABILITY_ID_SRIOV_CONTROL                    0x08
#define EFI_PCIE_CAPABILITY_ID_SRIOV_STATUS                     0x0A
#define EFI_PCIE_CAPABILITY_ID_SRIOV_INITIALVFS                 0x0C
#define EFI_PCIE_CAPABILITY_ID_SRIOV_TOTALVFS                   0x0E
#define EFI_PCIE_CAPABILITY_ID_SRIOV_NUMVFS                     0x10
#define EFI_PCIE_CAPABILITY_ID_SRIOV_FUNCTION_DEPENDENCY_LINK   0x12
#define EFI_PCIE_CAPABILITY_ID_SRIOV_FIRSTVF                    0x14
#define EFI_PCIE_CAPABILITY_ID_SRIOV_VFSTRIDE                   0x16
#define EFI_PCIE_CAPABILITY_ID_SRIOV_VFDEVICEID                 0x1A
#define EFI_PCIE_CAPABILITY_ID_SRIOV_SUPPORTED_PAGE_SIZE        0x1C
#define EFI_PCIE_CAPABILITY_ID_SRIOV_SYSTEM_PAGE_SIZE           0x20
#define EFI_PCIE_CAPABILITY_ID_SRIOV_BAR0                       0x24
#define EFI_PCIE_CAPABILITY_ID_SRIOV_BAR1                       0x28
#define EFI_PCIE_CAPABILITY_ID_SRIOV_BAR2                       0x2C
#define EFI_PCIE_CAPABILITY_ID_SRIOV_BAR3                       0x30
#define EFI_PCIE_CAPABILITY_ID_SRIOV_BAR4                       0x34
#define EFI_PCIE_CAPABILITY_ID_SRIOV_BAR5                       0x38
#define EFI_PCIE_CAPABILITY_ID_SRIOV_VF_MIGRATION_STATE         0x3C

#endif
