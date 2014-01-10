/** @file
  Support for the latest PCI standard.

  Copyright (c) 2006 - 2014, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials                          
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

typedef struct {
  UINT32 CapabilityId:16;
  UINT32 CapabilityVersion:4;
  UINT32 NextCapabilityOffset:12;
} PCI_EXPRESS_EXTENDED_CAPABILITIES_HEADER;

#define PCI_EXP_EXT_HDR PCI_EXPRESS_EXTENDED_CAPABILITIES_HEADER

#define PCI_EXPRESS_EXTENDED_CAPABILITY_ADVANCED_ERROR_REPORTING_ID   0x0001
#define PCI_EXPRESS_EXTENDED_CAPABILITY_ADVANCED_ERROR_REPORTING_VER1 0x1
#define PCI_EXPRESS_EXTENDED_CAPABILITY_ADVANCED_ERROR_REPORTING_VER2 0x2

typedef struct {
  PCI_EXPRESS_EXTENDED_CAPABILITIES_HEADER  Header;
  UINT32                                    UncorrectableErrorStatus;
  UINT32                                    UncorrectableErrorMask;
  UINT32                                    UncorrectableErrorSeverity;
  UINT32                                    CorrectableErrorStatus;
  UINT32                                    CorrectableErrorMask;
  UINT32                                    AdvancedErrorCapabilitiesAndControl;
  UINT32                                    HeaderLog;
  UINT32                                    RootErrorCommand;
  UINT32                                    RootErrorStatus;
  UINT16                                    ErrorSourceIdentification;
  UINT16                                    CorrectableErrorSourceIdentification;
  UINT32                                    TlpPrefixLog[4];
} PCI_EXPRESS_EXTENDED_CAPABILITIES_ADVANCED_ERROR_REPORTING;

#define PCI_EXPRESS_EXTENDED_CAPABILITY_VIRTUAL_CHANNEL_ID    0x0002
#define PCI_EXPRESS_EXTENDED_CAPABILITY_VIRTUAL_CHANNEL_MFVC  0x0009
#define PCI_EXPRESS_EXTENDED_CAPABILITY_VIRTUAL_CHANNEL_VER1  0x1

typedef struct {
  UINT32                                    VcResourceCapability:24;
  UINT32                                    PortArbTableOffset:8;
  UINT32                                    VcResourceControl;
  UINT16                                    Reserved1;
  UINT16                                    VcResourceStatus;
} PCI_EXPRESS_EXTENDED_CAPABILITIES_VIRTUAL_CHANNEL_VC;

typedef struct {
  PCI_EXPRESS_EXTENDED_CAPABILITIES_HEADER              Header;
  UINT32                                                ExtendedVcCount:3;
  UINT32                                                PortVcCapability1:29;
  UINT32                                                PortVcCapability2:24;
  UINT32                                                VcArbTableOffset:8;
  UINT16                                                PortVcControl;
  UINT16                                                PortVcStatus;
  PCI_EXPRESS_EXTENDED_CAPABILITIES_VIRTUAL_CHANNEL_VC  Capability[1];
} PCI_EXPRESS_EXTENDED_CAPABILITIES_VIRTUAL_CHANNEL_CAPABILITY;

#define PCI_EXPRESS_EXTENDED_CAPABILITY_SERIAL_NUMBER_ID    0x0003
#define PCI_EXPRESS_EXTENDED_CAPABILITY_SERIAL_NUMBER_VER1  0x1

typedef struct {
  PCI_EXPRESS_EXTENDED_CAPABILITIES_HEADER  Header;
  UINT64                                    SerialNumber;
} PCI_EXPRESS_EXTENDED_CAPABILITIES_SERIAL_NUMBER;

#define PCI_EXPRESS_EXTENDED_CAPABILITY_LINK_DECLARATION_ID   0x0005
#define PCI_EXPRESS_EXTENDED_CAPABILITY_LINK_DECLARATION_VER1 0x1

typedef struct {
  PCI_EXPRESS_EXTENDED_CAPABILITIES_HEADER  Header;
  UINT32                                    ElementSelfDescription;
  UINT32                                    Reserved;
  UINT32                                    LinkEntry[1];
} PCI_EXPRESS_EXTENDED_CAPABILITIES_LINK_DECLARATION;

#define PCI_EXPRESS_EXTENDED_CAPABILITY_LINK_DECLARATION_GET_LINK_COUNT(LINK_DECLARATION) (UINT8)(((LINK_DECLARATION->ElementSelfDescription)&0x0000ff00)>>8)

#define PCI_EXPRESS_EXTENDED_CAPABILITY_LINK_CONTROL_ID   0x0006
#define PCI_EXPRESS_EXTENDED_CAPABILITY_LINK_CONTROL_VER1 0x1

typedef struct {
  PCI_EXPRESS_EXTENDED_CAPABILITIES_HEADER  Header;
  UINT32                                    RootComplexLinkCapabilities;
  UINT16                                    RootComplexLinkControl;
  UINT16                                    RootComplexLinkStatus;
} PCI_EXPRESS_EXTENDED_CAPABILITIES_INTERNAL_LINK_CONTROL;

#define PCI_EXPRESS_EXTENDED_CAPABILITY_POWER_BUDGETING_ID   0x0004
#define PCI_EXPRESS_EXTENDED_CAPABILITY_POWER_BUDGETING_VER1 0x1

typedef struct {
  PCI_EXPRESS_EXTENDED_CAPABILITIES_HEADER  Header;
  UINT32                                    DataSelect:8;
  UINT32                                    Reserved:24;
  UINT32                                    Data;
  UINT32                                    PowerBudgetCapability:1;
  UINT32                                    Reserved2:7;
  UINT32                                    Reserved3:24;
} PCI_EXPRESS_EXTENDED_CAPABILITIES_POWER_BUDGETING;

#define PCI_EXPRESS_EXTENDED_CAPABILITY_ACS_EXTENDED_ID   0x000D
#define PCI_EXPRESS_EXTENDED_CAPABILITY_ACS_EXTENDED_VER1 0x1

typedef struct {
  PCI_EXPRESS_EXTENDED_CAPABILITIES_HEADER  Header;
  UINT16                                    AcsCapability;
  UINT16                                    AcsControl;
  UINT8                                     EgressControlVectorArray[1];
} PCI_EXPRESS_EXTENDED_CAPABILITIES_ACS_EXTENDED;

#define PCI_EXPRESS_EXTENDED_CAPABILITY_ACS_EXTENDED_GET_EGRES_CONTROL(ACS_EXTENDED) (UINT8)(((ACS_EXTENDED->AcsCapability)&0x00000020))
#define PCI_EXPRESS_EXTENDED_CAPABILITY_ACS_EXTENDED_GET_EGRES_VECTOR_SIZE(ACS_EXTENDED) (UINT8)(((ACS_EXTENDED->AcsCapability)&0x0000FF00))

#define PCI_EXPRESS_EXTENDED_CAPABILITY_EVENT_COLLECTOR_ENDPOINT_ASSOCIATION_ID   0x0007
#define PCI_EXPRESS_EXTENDED_CAPABILITY_EVENT_COLLECTOR_ENDPOINT_ASSOCIATION_VER1 0x1

typedef struct {
  PCI_EXPRESS_EXTENDED_CAPABILITIES_HEADER  Header;
  UINT32                                    AssociationBitmap;
} PCI_EXPRESS_EXTENDED_CAPABILITIES_EVENT_COLLECTOR_ENDPOINT_ASSOCIATION;

#define PCI_EXPRESS_EXTENDED_CAPABILITY_MULTI_FUNCTION_VIRTUAL_CHANNEL_ID    0x0008
#define PCI_EXPRESS_EXTENDED_CAPABILITY_MULTI_FUNCTION_VIRTUAL_CHANNEL_VER1  0x1

typedef PCI_EXPRESS_EXTENDED_CAPABILITIES_VIRTUAL_CHANNEL_CAPABILITY PCI_EXPRESS_EXTENDED_CAPABILITIES_MULTI_FUNCTION_VIRTUAL_CHANNEL_CAPABILITY;

#define PCI_EXPRESS_EXTENDED_CAPABILITY_VENDOR_SPECIFIC_ID   0x000B
#define PCI_EXPRESS_EXTENDED_CAPABILITY_VENDOR_SPECIFIC_VER1 0x1

typedef struct {
  PCI_EXPRESS_EXTENDED_CAPABILITIES_HEADER  Header;
  UINT32                                    VendorSpecificHeader;
  UINT8                                     VendorSpecific[1];
} PCI_EXPRESS_EXTENDED_CAPABILITIES_VENDOR_SPECIFIC;

#define PCI_EXPRESS_EXTENDED_CAPABILITY_VENDOR_SPECIFIC_GET_SIZE(VENDOR) (UINT16)(((VENDOR->VendorSpecificHeader)&0xFFF00000)>>20)

#define PCI_EXPRESS_EXTENDED_CAPABILITY_RCRB_HEADER_ID   0x000A
#define PCI_EXPRESS_EXTENDED_CAPABILITY_RCRB_HEADER_VER1 0x1

typedef struct {
  PCI_EXPRESS_EXTENDED_CAPABILITIES_HEADER  Header;
  UINT16                                    VendorId;
  UINT16                                    DeviceId;
  UINT32                                    RcrbCapabilities;
  UINT32                                    RcrbControl;
  UINT32                                    Reserved;
} PCI_EXPRESS_EXTENDED_CAPABILITIES_RCRB_HEADER;

#define PCI_EXPRESS_EXTENDED_CAPABILITY_MULTICAST_ID   0x0012
#define PCI_EXPRESS_EXTENDED_CAPABILITY_MULTICAST_VER1 0x1

typedef struct {
  PCI_EXPRESS_EXTENDED_CAPABILITIES_HEADER  Header;
  UINT16                                    MultiCastCapability;
  UINT16                                    MulticastControl;
  UINT64                                    McBaseAddress;
  UINT64                                    McReceiveAddress;
  UINT64                                    McBlockAll;
  UINT64                                    McBlockUntranslated;
  UINT64                                    McOverlayBar;
} PCI_EXPRESS_EXTENDED_CAPABILITIES_MULTICAST;

#define PCI_EXPRESS_EXTENDED_CAPABILITY_RESIZABLE_BAR_ID    0x0015
#define PCI_EXPRESS_EXTENDED_CAPABILITY_RESIZABLE_BAR_VER1  0x1

typedef struct {
  UINT32                                                 ResizableBarCapability;
  UINT16                                                 ResizableBarControl;
  UINT16                                                 Reserved;
} PCI_EXPRESS_EXTENDED_CAPABILITIES_RESIZABLE_BAR_ENTRY;

typedef struct {
  PCI_EXPRESS_EXTENDED_CAPABILITIES_HEADER               Header;
  PCI_EXPRESS_EXTENDED_CAPABILITIES_RESIZABLE_BAR_ENTRY  Capability[1];
} PCI_EXPRESS_EXTENDED_CAPABILITIES_RESIZABLE_BAR;

#define GET_NUMBER_RESIZABLE_BARS(x) (((x->Capability[0].ResizableBarControl) & 0xE0) >> 5)

#define PCI_EXPRESS_EXTENDED_CAPABILITY_ARI_CAPABILITY_ID    0x000E
#define PCI_EXPRESS_EXTENDED_CAPABILITY_ARI_CAPABILITY_VER1  0x1

typedef struct {
  PCI_EXPRESS_EXTENDED_CAPABILITIES_HEADER                Header;
  UINT16                                                  AriCapability;
  UINT16                                                  AriControl;
} PCI_EXPRESS_EXTENDED_CAPABILITIES_ARI_CAPABILITY;

#define PCI_EXPRESS_EXTENDED_CAPABILITY_DYNAMIC_POWER_ALLOCATION_ID    0x0016
#define PCI_EXPRESS_EXTENDED_CAPABILITY_DYNAMIC_POWER_ALLOCATION_VER1  0x1

typedef struct {
  PCI_EXPRESS_EXTENDED_CAPABILITIES_HEADER                Header;
  UINT32                                                  DpaCapability;
  UINT32                                                  DpaLatencyIndicator;
  UINT16                                                  DpaStatus;
  UINT16                                                  DpaControl;
  UINT8                                                   DpaPowerAllocationArray[1];
} PCI_EXPRESS_EXTENDED_CAPABILITIES_DYNAMIC_POWER_ALLOCATION;

#define PCI_EXPRESS_EXTENDED_CAPABILITY_DYNAMIC_POWER_ALLOCATION_GET_SUBSTATE_MAX(POWER) (UINT16)(((POWER->DpaCapability)&0x0000000F))


#define PCI_EXPRESS_EXTENDED_CAPABILITY_LATENCE_TOLERANCE_REPORTING_ID    0x0018
#define PCI_EXPRESS_EXTENDED_CAPABILITY_LATENCE_TOLERANCE_REPORTING_VER1  0x1

typedef struct {
  PCI_EXPRESS_EXTENDED_CAPABILITIES_HEADER                Header;
  UINT16                                                  MaxSnoopLatency;
  UINT16                                                  MaxNoSnoopLatency;
} PCI_EXPRESS_EXTENDED_CAPABILITIES_LATENCE_TOLERANCE_REPORTING;

#define PCI_EXPRESS_EXTENDED_CAPABILITY_TPH_ID    0x0017
#define PCI_EXPRESS_EXTENDED_CAPABILITY_TPH_VER1  0x1

typedef struct {
  PCI_EXPRESS_EXTENDED_CAPABILITIES_HEADER                Header;
  UINT32                                                  TphRequesterCapability;
  UINT32                                                  TphRequesterControl;
  UINT16                                                  TphStTable[1];
} PCI_EXPRESS_EXTENDED_CAPABILITIES_TPH;

#define GET_TPH_TABLE_SIZE(x) ((x->TphRequesterCapability & 0x7FF0000)>>16) * sizeof(UINT16)

#endif
