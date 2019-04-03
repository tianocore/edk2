/** @file
QNC PCI Express initialization entry

Copyright (c) 2013-2015 Intel Corporation.

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "CommonHeader.h"

#define PCIEXP_ROOT_PORT_URE_ENABLE    BIT0   //  unsupported request reporting enable
#define PCIEXP_ROOT_PORT_FEE_ENABLE    BIT1   //  Fatal Error Reporting Enable
#define PCIEXP_ROOT_PORT_NFE_ENABLE    BIT2   //  Non-Fatal Error Reporting Enable
#define PCIEXP_ROOT_PORT_CEE_ENABLE    BIT3   //  Correctable Error Reporting Enable
#define PCIEXP_ROOT_PORT_SFE_ENABLE    BIT4   //  System Error on Fatal Error Enable
#define PCIEXP_ROOT_PORT_SNE_ENABLE    BIT5   //  System Error on Non-Fatal Error Enable
#define PCIEXP_ROOT_PORT_SCE_ENABLE    BIT6   //  System Error on Correctable Error Enable

EFI_STATUS
PcieStall (
  IN UINTN              Microseconds
  )
{
  MicroSecondDelay (Microseconds);
  return EFI_SUCCESS;
}

/**

  Find the Offset to a given Capabilities ID
    CAPID list:
      0x01 = PCI Power Management Interface
      0x04 = Slot Identification
      0x05 = MSI Capability
      0x10 = PCI Express Capability

  @param[in]  Bus                     Bus number of the interested device
  @param[in]  Device                  Device number of the interested device
  @param[in]  Function                Function number of the interested device
  @param[in]  CapId                   Capability ID to be scanned

  @retval Offset of desired CAPID

**/
UINT32
PcieFindCapId (
  UINT8   Bus,
  UINT8   Device,
  UINT8   Function,
  UINT8   CapId
  )
{
  UINT8    CapHeader;

  //
  // Always start at Offset 0x34
  //
  CapHeader = QNCMmPci8 (0, Bus, Device, Function, R_QNC_PCIE_CAP_PTR);

  if (CapHeader == 0xFF) {
     return 0;
  }

  while (CapHeader != 0) {
    if (QNCMmPci8 (0, Bus, Device, Function, CapHeader) == CapId) {
      return CapHeader;
    }
    CapHeader = QNCMmPci8 (0, Bus, Device, Function, CapHeader + 1);
  }
  return 0;
}

/**

  Search and return the offset of desired Pci Express Capability ID
    CAPID list:
      0x0001 = Advanced Error Rreporting Capability
      0x0002 = Virtual Channel Capability
      0x0003 = Device Serial Number Capability
      0x0004 = Power Budgeting Capability

  @param[in]  Bus                     Bus number of the interested device
  @param[in]  Device                  Device number of the interested device
  @param[in]  Function                Function number of the interested device
  @param[in]  CapId                   Capability ID to be scanned

  @retval Offset of desired CAPID

**/
UINT32
PcieFindExtendedCapId (
  UINT8   Bus,
  UINT8   Device,
  UINT8   Function,
  UINT16  CapId
  )
{
  UINT16    CapHeaderOffset;
  UINT16    CapHeaderId;

  // Start to search at Offset 0x100
  // Get Capability Header
  CapHeaderId = 0;
  CapHeaderOffset = PCIE_CAP_EXT_HEARDER_OFFSET;

  while (CapHeaderOffset != 0 && CapHeaderId != 0xFFFF) {
    CapHeaderId = QNCMmPci16 (0, Bus, Device, Function, CapHeaderOffset);
    if (CapHeaderId == CapId) {
      return CapHeaderOffset;
    }
    CapHeaderOffset = (QNCMmPci16 (0, Bus, Device, Function, CapHeaderOffset + 2) >> 4);
  }
  return 0;
}

/**

  Map Vc on both root port and downstream device

  @param[in]  Bus1                    Bus number of the root port
  @param[in]  Device1                 Device number of the root port
  @param[in]  Function1               Function number of the root port
  @param[in]  Bus2                    Bus number of the downstream device
  @param[in]  Device2                 Device number of the downstream device
  @param[in]  Function2               Function number of the downstream device

  @retval EFI_SUCCESS    Map Vc successful

**/
EFI_STATUS
PcieInitTcxVc0 (
  IN UINT8   Bus1,
  IN UINT8   Device1,
  IN UINT8   Function1,
  IN UINT8   Bus2,
  IN UINT8   Device2,
  IN UINT8   Function2
  )
{
  UINT32  Offset;

  //
  // Initialize TCx-VC0 value on the port to only use TC0
  //
  Offset = PcieFindExtendedCapId (Bus1, Device1, Function1, 2);
  if (Offset == 0) {
    return EFI_UNSUPPORTED;
  }
  QNCMmPci8AndThenOr (0, Bus1, Device1, Function1, (Offset + PCIE_SLOT_CAP_OFFSET), ~0xF, 1);

  // Set TCx-VC0 value on the Endpoint

  Offset = PcieFindExtendedCapId (Bus2, Device2, Function2, 2);
  if (Offset == 0) {
    return EFI_UNSUPPORTED;
  }
  QNCMmPci8AndThenOr (0, Bus2, Device2, Function2, (Offset + PCIE_SLOT_CAP_OFFSET), ~0xF, 1);

  return EFI_SUCCESS;
}

/**

  Map Traffic Class x to Vc0 on both root port and downstream device

  @param[in]  Bus1                    Bus number of the root port
  @param[in]  Device1                 Device number of the root port
  @param[in]  Function1               Function number of the root port
  @param[in]  Bus2                    Bus number of the downstream device
  @param[in]  Device2                 Device number of the downstream device
  @param[in]  Function2               Function number of the downstream device
  @param[in]  TCx                     Traffic Class to be mapped to vc0

  @retval EFI_SUCCESS    Map Tcx to Vc0 successful

**/
EFI_STATUS
PcieMapTcxVc0 (
  IN UINT8   Bus1,
  IN UINT8   Device1,
  IN UINT8   Function1,
  IN UINT8   Bus2,
  IN UINT8   Device2,
  IN UINT8   Function2,
  IN UINT8   TCx
  )
{
  UINT32  Offset;

  //
  // Set TCx-VC0 value on the port
  //

  Offset = PcieFindExtendedCapId (Bus1, Device1, Function1, 2);
  if (Offset == 0) {
    return EFI_UNSUPPORTED;
  }
  QNCMmPci8 (0, Bus1, Device1, Function1, (Offset + PCIE_SLOT_CAP_OFFSET)) = (UINT8)(1 << TCx);

  // Set TCx-VC0 value on the Endpoint

  Offset = PcieFindExtendedCapId (Bus2, Device2, Function2, 2);
  if (Offset == 0) {
    return EFI_UNSUPPORTED;
  }
  QNCMmPci8 (0, Bus2, Device2, Function2, (Offset + PCIE_SLOT_CAP_OFFSET)) = (UINT8)(1 << TCx);

  return EFI_SUCCESS;
}

/**

  Set common clock for both root port and downstream device.

  @param[in]  Bus1                    Bus number of the root port
  @param[in]  Device1                 Device number of the root port
  @param[in]  Function1               Function number of the root port
  @param[in]  Bus2                    Device number of the downstream device
  @param[in]  Device2                 Function number of the downstream device

  @retval EFI_SUCCESS    Set common clock successful

**/
EFI_STATUS
PcieSetCommonClock (
  IN UINT8   Bus1,
  IN UINT8   Device1,
  IN UINT8   Function1,
  IN UINT8   Bus2,
  IN UINT8   Device2
 )
{
  UINT32      CapOffset1;
  UINT32      CapOffset2;
  UINT8       Function2;
  UINT8       CommonClock;
  EFI_STATUS  Status;

  //
  // Get the pointer to the Port PCI Express Capability Structure.
  //
  CommonClock = 0;
  CapOffset1 = PcieFindCapId (Bus1, Device1, Function1, PCIE_CAPID);
  if (CapOffset1 == 0) {
    return EFI_UNSUPPORTED;
  }

  //
  // Step 1
  // Read the Slot Clock Configuration bit of the Link status register of the root port and the endpoint device connected to the port
  // If both components have this bit set to 1, then System BIOS should set the "Common Clock Configuration" bit in the Link Control Registers
  // for both components at both sides of the link to indicate that components at both ends
  // of the link use a common clock source
  //

  //
  // Check the Port Slot Clock Configuration Bit.
  //
  if ((QNCMmPci16 (0, Bus1, Device1, Function1, (CapOffset1 + PCIE_LINK_STS_OFFSET)) & B_QNC_PCIE_LSTS_SCC) == 0) {
    return EFI_UNSUPPORTED;
  }

  for (Function2 = 0; Function2 < 8; Function2++) {
    //
    // Check the Endpoint Slot Clock Configuration Bit.
    //
    CapOffset2 = PcieFindCapId (Bus2, Device2, Function2, PCIE_CAPID);
    if ((CapOffset2 != 0) &&
       ((QNCMmPci16 (0, Bus2, Device2, Function2, (CapOffset2 + PCIE_LINK_STS_OFFSET)) & B_QNC_PCIE_LSTS_SCC) != 0)) {

      //
      // Common clock is supported, set common clock bit on root port
      // and the endpoint
      //
      if (CommonClock == 0) {
        QNCMmPci8Or (0, Bus1, Device1, Function1, (CapOffset1 + PCIE_LINK_CNT_OFFSET), B_QNC_PCIE_LCTL_CCC);
        CommonClock++;
      }
      QNCMmPci8Or (0, Bus2, Device2, Function2, (CapOffset2 + PCIE_LINK_CNT_OFFSET), B_QNC_PCIE_LCTL_CCC);
    }
  }

  //
  // Step 2   If the Common Clock Configuration bit was changed by BIOS in step 1,
  // System BIOS should initiate a link training by setting the Retrain Link bit
  // in the Link Control register of the root port (D28:F0/F1 offset
  // 50h [5]) to "1b" and then poll the Link Training bit in the Link Status
  // register of the root port (D28:F0/F1/F2/F3/F4/F5 offset 52h [11]) until it is
  // "0b".
  //
  if (CommonClock == 0) {
    Status = EFI_UNSUPPORTED;
  } else {
    //
    // Retrain the Link per PCI Express Specification.
    //
    QNCMmPci8Or (0, Bus1, Device1, Function1, (CapOffset1 + PCIE_LINK_CNT_OFFSET), B_QNC_PCIE_LCTL_RL);

    //
    // Wait until Re-Training has completed.
    //
    while ((QNCMmPci16 (0, Bus1, Device1, Function1, (CapOffset1 + PCIE_LINK_STS_OFFSET)) & B_QNC_PCIE_LSTS_LT) != 0);
    Status = EFI_SUCCESS;
  }

  return Status;
}

/**

  Enables the CLKREQ# PM on all the end point functions

  @param[in]  Bus                Bus number of the downstream device
  @param[in]  Device             Device number of the downstream device

  @retval None

**/
VOID
PcieSetClkreq (
  IN  UINT8   Bus,
  IN  UINT8   Device
 )
{
  UINT8  Function;
  UINT32 CapOffset;

  //
  // Parse thro all the functions of the endpoint and find the PCIe Cap ID (offset 10h) and if
  // exists then enable the CLKREQ# bit (BIT8) on that function
  //
  for (Function = 0; Function < 8; Function++) {
    //
    // Find the PCIe Cap Id (offset 10h)
    //
    CapOffset = PcieFindCapId (Bus, Device, Function, PCIE_CAPID);
    if (CapOffset == 0) {
       continue;
    }

    //
    // Check if CLKREQ# is supported by the endpoints
    //
    if ((QNCMmPci32 (0, Bus, Device, Function, (CapOffset + PCIE_LINK_CAP_OFFSET))
      & B_QNC_PCIE_LCAP_CPM) != B_QNC_PCIE_LCAP_CPM) {
      //
      // CLKREQ# is not supported so dont do anything
      //
      return;
    }
  }

  //
  // Now enable the CLKREQ#
  //
  for (Function = 0; Function < 8; Function++) {
    //
    // Find the PCIe Cap Id (offset 10h)
    //
    CapOffset = PcieFindCapId (Bus, Device, Function, PCIE_CAPID);
    if (CapOffset == 0) {
       continue;
    }

    QNCMmPci16Or (0, Bus, Device, Function, (CapOffset + PCIE_LINK_CNT_OFFSET), BIT8);
  }
}

/**

  Configure ASPM automatically for both root port and downstream device.

  @param[in]  RootBus                    Bus number of the root port
  @param[in]  RootDevice                 Device number of the root port
  @param[in]  RootFunction               Function number of the root port
  @param[in]  EndpointBus                Bus number of the downstream device
  @param[in]  EndpointDevice             Device number of the downstream device
  @param[in]  EndpointFunction           Function number of the downstream device
  @param[in]  LinkAspmVal                Currently used ASPM setting

  @retval EFI_SUCCESS    Configure ASPM successful

**/
EFI_STATUS
PcieSetAspmAuto (
  IN  UINT8   RootBus,
  IN  UINT8   RootDevice,
  IN  UINT8   RootFunction,
  IN  UINT8   EndpointBus,
  IN  UINT8   EndpointDevice,
  IN  UINT8   EndpointFunction,
  OUT UINT16  *LinkAspmVal
 )
{
  UINT32    RootPcieCapOffset;
  UINT32    EndpointPcieCapOffset;
  UINT16    RootPortAspm;
  UINT16    EndPointAspm;
  UINT16    AspmVal;
  UINT32    PortLxLat;
  UINT32    EndPointLxLat;
  UINT32    LxLat;

  //
  // Get the pointer to the Port PCI Express Capability Structure.
  //
  RootPcieCapOffset = PcieFindCapId (RootBus, RootDevice, RootFunction, PCIE_CAPID);
  if (RootPcieCapOffset == 0) {
    return EFI_UNSUPPORTED;
  }

  //
  // Get the pointer to the Endpoint PCI Express Capability Structure.
  //
  EndpointPcieCapOffset = PcieFindCapId (EndpointBus, EndpointDevice, EndpointFunction, PCIE_CAPID);
  if (EndpointPcieCapOffset == 0) {
    return EFI_UNSUPPORTED;
  }

  //
  // Obtain initial ASPM settings from respective port capability registers.
  //
  RootPortAspm  = (QNCMmPci16 (0, RootBus, RootDevice, RootFunction, (RootPcieCapOffset + PCIE_LINK_CAP_OFFSET)) & B_QNC_PCIE_LCAP_APMS_MASK) >> V_QNC_PCIE_LCAP_APMS_OFFSET;

  //
  // Configure downstream device if present.
  //
  EndPointAspm  = (QNCMmPci16 (0, EndpointBus, EndpointDevice, EndpointFunction, (EndpointPcieCapOffset + PCIE_LINK_CAP_OFFSET)) & B_QNC_PCIE_LCAP_APMS_MASK) >> V_QNC_PCIE_LCAP_APMS_OFFSET;

  //
  // TODO: Mask APMC with values from lookup table.
  // RevID of 0xFF applies to all steppings.
  //

  // TODO: Mask with latency/acceptable latency comparison results.

  AspmVal = RootPortAspm;
  if (RootPortAspm > EndPointAspm) {
    AspmVal = EndPointAspm;
  }

  //
  // Check if L1 should be enabled based on port and endpoint L1 exit latency.
  //
  if(AspmVal & BIT1) {
    PortLxLat      = QNCMmPci32 (0, RootBus, RootDevice, RootFunction, (RootPcieCapOffset + PCIE_LINK_CAP_OFFSET)) & B_QNC_PCIE_LCAP_EL1_MASK;
    EndPointLxLat  = QNCMmPci32 (0, EndpointBus, EndpointDevice, EndpointFunction, (EndpointPcieCapOffset + PCIE_LINK_CAP_OFFSET)) & B_QNC_PCIE_LCAP_EL1_MASK;

    LxLat = PortLxLat;
    if(PortLxLat < EndPointLxLat) {
      LxLat = EndPointLxLat;
    }

    //
    // check if the value is bigger than endpoint L1 acceptable exit latency, if it is
    // larger than accepted value, then we should disable L1
    //
    LxLat >>= 6;
    if(LxLat > (QNCMmPci32 (0, EndpointBus, EndpointDevice, EndpointFunction, (EndpointPcieCapOffset + PCIE_DEV_CAP_OFFSET)) & B_QNC_PCIE_DCAP_E1AL)) {
      AspmVal &= ~BIT1;
    }
  }

  //
  // Check if L0s should be enabled based on port and endpoint L0s exit latency.
  //
  if(AspmVal & BIT0) {
    PortLxLat      = QNCMmPci32 (0, RootBus, RootDevice, RootFunction, (RootPcieCapOffset+ PCIE_LINK_CAP_OFFSET)) & B_QNC_PCIE_LCAP_EL0_MASK;
    EndPointLxLat  = QNCMmPci32 (0, EndpointBus, EndpointDevice, EndpointFunction, (EndpointPcieCapOffset + PCIE_LINK_CAP_OFFSET)) & B_QNC_PCIE_LCAP_EL0_MASK;

    LxLat = PortLxLat;
    if(PortLxLat < EndPointLxLat) {
      LxLat = EndPointLxLat;
    }

    //
    // check if the value is bigger than endpoint L0s acceptable exit latency, if it is
    // larger than accepted value, then we should disable L0s
    //
    LxLat >>= 6;
    if(LxLat > (QNCMmPci32 (0, EndpointBus, EndpointDevice, EndpointFunction, (EndpointPcieCapOffset + PCIE_DEV_CAP_OFFSET)) & B_QNC_PCIE_DCAP_E0AL)) {
      AspmVal &= ~BIT0;
    }
  }

  RootPortAspm = AspmVal;

  *LinkAspmVal = AspmVal;
  //
  // Set Endpoint Aspm
  //
  QNCMmPci16AndThenOr (0, EndpointBus, EndpointDevice, EndpointFunction, (EndpointPcieCapOffset + PCIE_LINK_CNT_OFFSET), 0xFFFC, AspmVal);


  //
  // Set Root Port Aspm
  //
  QNCMmPci16AndThenOr (0, RootBus, RootDevice, RootFunction, (RootPcieCapOffset + PCIE_LINK_CNT_OFFSET), 0xFFFC, RootPortAspm);

  return EFI_SUCCESS;
}

/**

  Configure ASPM based on the given setting for the interested device.

  @param[in]  Bus                    Bus number of the interested device
  @param[in]  Device                 Device number of the interested device
  @param[in]  Function               Function number of the interested device
  @param[in]  AspmSetting            Aspm setting
  @param[in]  LinkAspmVal            Currently used ASPM setting

  @retval EFI_SUCCESS    Configure ASPM successful

**/
EFI_STATUS
PcieSetAspmManual (
  IN  UINT8   Bus,
  IN  UINT8   Device,
  IN  UINT8   Function,
  IN  UINT8   AspmSetting,
  OUT UINT16  *LinkAspmVal
 )
{
  UINT32    PcieCapOffset;
  UINT16    PortAspm;

  //
  // Get the pointer to the Port PCI Express Capability Structure.
  //
  PcieCapOffset = PcieFindCapId (Bus, Device, Function, PCIE_CAPID);
  if (PcieCapOffset == 0) {
    return EFI_UNSUPPORTED;
  }

  // Read the Link Capability register's ASPM setting
  PortAspm = (QNCMmPci16 (0, Bus, Device, Function, (PcieCapOffset + PCIE_LINK_CAP_OFFSET)) & B_QNC_PCIE_LCAP_APMS_MASK) >> V_QNC_PCIE_LCAP_APMS_OFFSET;
  // Mask it with the Setup selection
  PortAspm &= AspmSetting;

  *LinkAspmVal = PortAspm;
  // Write it to the Link Control register
  QNCMmPci16AndThenOr (0, Bus, Device, Function, (PcieCapOffset + PCIE_LINK_CNT_OFFSET), 0xFFFC, PortAspm);

  return EFI_SUCCESS;
}

/**

  Perform Initialization on one PCI Express root port.

  @param[in]  RootPortIndex          Index of PCI Express root port
  @param[in]  RootPortConfig         Pointer to the given pcie root port configuration
  @param[in]  PciExpressBar          Base address of pcie space
  @param[in]  QNCRootComplexBar       Base address of root complex
  @param[in]  QNCPmioBase             Base address of PM IO space
  @param[in]  QNCGpeBase              Base address of gpe IO space

  @retval EFI_SUCCESS    Initialization successful

**/
EFI_STATUS
QNCRootPortInit (
  IN UINT32                                    RootPortIndex,
  IN PCIEXP_ROOT_PORT_CONFIGURATION            *RootPortConfig,
  IN UINT64                                    PciExpressBar,
  IN UINT32                                    QNCRootComplexBar,
  IN UINT32                                    QNCPmioBase,
  IN UINT32                                    QNCGpeBase
  )
{
  UINT64            RPBase;
  UINT64            EndPointBase;
  UINT16            AspmVal;
  UINT16            SlotStatus;
  UINTN             Index;
  UINT32            CapOffset;
  UINT32            DwordReg;

  RPBase = PciExpressBar + (((PCI_BUS_NUMBER_QNC << 8) + ((PCI_DEVICE_NUMBER_PCIE_ROOTPORT) << 3) + ((PCI_FUNCTION_NUMBER_PCIE_ROOTPORT_0 + RootPortIndex) << 0)) << 12);
  CapOffset = PcieFindCapId (PCI_BUS_NUMBER_QNC, (UINT8)(PCI_DEVICE_NUMBER_PCIE_ROOTPORT), (UINT8)(PCI_FUNCTION_NUMBER_PCIE_ROOTPORT_0 + RootPortIndex), PCIE_CAPID);

  if (CapOffset == 0) {
    return EFI_UNSUPPORTED;
  }

  //
  // Initialize "Slot Implmemented Bit" for this root port
  //
  if (RootPortConfig[RootPortIndex].Bits.SlotImplemented) {
    QNCMmio16Or (RPBase, R_QNC_PCIE_XCAP, B_QNC_PCIE_XCAP_SI);
  }

  //
  // For Root Port Slots Numbering on the CRBs.
  //  Root Port 0 = Slot 1
  //  Root Port 1 = Slot 2
  //  Root Port 2 = Slot 3
  //  Root Port 3 = Slot 4
  //
  DwordReg = QNCMmio32 (RPBase, R_QNC_PCIE_SLCAP);
  DwordReg &= B_QNC_PCIE_SLCAP_MASK_RSV_VALUE;
  DwordReg |= (V_QNC_PCIE_SLCAP_SLV << V_QNC_PCIE_SLCAP_SLV_OFFSET);
  DwordReg |= ((RootPortConfig[RootPortIndex].Bits.PhysicalSlotNumber) << V_QNC_PCIE_SLCAP_PSN_OFFSET) ;
  QNCMmio32 (RPBase, R_QNC_PCIE_SLCAP) = DwordReg;

  //
  // Check for a Presence Detect Change.
  //
  SlotStatus = QNCMmio16 (RPBase, R_QNC_PCIE_SLSTS);
  if ((SlotStatus & (B_QNC_PCIE_SLSTS_PDS + B_QNC_PCIE_SLSTS_PDC)) == 0) {
    return EFI_NOT_FOUND;
  }

  //
  // Temporarily Hardcode the Root Port Bridge Number to 2.
  //
  // This Endpoint check should immediately pass.  Howerver, a 900ms delay
  // has been added to match the timing requirements of the PCI Express Base
  // Specification, Revision 1.0A, Section 6.6 ("...software must allow 1.0s
  // after a reset of a device, before it may determine that a device which
  // fails to return a Successful Completion status for a valid Configuration
  // Request is a broken device").  Note that a 100ms delay was already added
  // after the Root Ports were first taken out of reset.
  //
  QNCMmio32AndThenOr (RPBase, R_QNC_PCIE_BNUM, 0xFF0000FF, 0x00020200);
  //
  // Only do this when a downstream device is present
  //
  EndPointBase = PciExpressBar + (((2 << 8) + (0 << 3) + (0 << 0)) << 12);
  if ((SlotStatus & B_QNC_PCIE_SLSTS_PDS) != 0) {
    for (Index = 0; Index < V_PCIE_MAX_TRY_TIMES; Index++){
      if (QNCMmio16 (EndPointBase, 0x0) != 0xFFFF) {
        break;
      }
      PcieStall (15);
    }
    if (Index >= V_PCIE_MAX_TRY_TIMES) {
      //
      // Clear Bus Numbers.
      //
      QNCMmio32And (RPBase, R_QNC_PCIE_BNUM, 0xFF0000FF);
      return EFI_NOT_FOUND;
    }
  }

  //
  // PCI Express* Virtual Channels
  // Clear TC1-7 Traffic classes.
  // Map TC0-VC0
  //
  PcieInitTcxVc0 (PCI_BUS_NUMBER_QNC, (UINT8)(PCI_DEVICE_NUMBER_PCIE_ROOTPORT), (UINT8)(PCI_FUNCTION_NUMBER_PCIE_ROOTPORT_0 + RootPortIndex), 2, 0, 0);
  PcieMapTcxVc0 (PCI_BUS_NUMBER_QNC, (UINT8)(PCI_DEVICE_NUMBER_PCIE_ROOTPORT), (UINT8)(PCI_FUNCTION_NUMBER_PCIE_ROOTPORT_0 + RootPortIndex), 2, 0, 0, 0x0);

  //
  // Set Common Clock for inserted cards
  //
  if ((SlotStatus & B_QNC_PCIE_SLSTS_PDS) != 0) {
    PcieSetCommonClock (PCI_BUS_NUMBER_QNC, (UINT8)(PCI_DEVICE_NUMBER_PCIE_ROOTPORT), (UINT8)(PCI_FUNCTION_NUMBER_PCIE_ROOTPORT_0 + RootPortIndex), 2, 0);
  }

  //
  // Flow for Enabling ASPM
  //
  if (RootPortConfig[RootPortIndex].Bits.AspmEnable) {
    if (RootPortConfig[RootPortIndex].Bits.AspmAutoEnable) {
      PcieSetAspmAuto (PCI_BUS_NUMBER_QNC, (UINT8)(PCI_DEVICE_NUMBER_PCIE_ROOTPORT), (UINT8)(PCI_FUNCTION_NUMBER_PCIE_ROOTPORT_0 + RootPortIndex), 2, 0, 0, &AspmVal);
    } else {
      //
      // Set ASPM values according to setup selections, masked by capabilities
      //
      PcieSetAspmManual (
        PCI_BUS_NUMBER_QNC,
        (UINT8) (PCI_DEVICE_NUMBER_PCIE_ROOTPORT),
        (UINT8) (PCI_FUNCTION_NUMBER_PCIE_ROOTPORT_0 + RootPortIndex),
        (UINT8) ((RootPortConfig[RootPortIndex].Bits.AspmL0sEnable & 0x01) | (RootPortConfig[RootPortIndex].Bits.AspmL1Enable << 1)),
        &AspmVal
        );
    }
  }

  //
  // Enable the PCIe CLKREQ#
  //
  if ((SlotStatus & B_QNC_PCIE_SLSTS_PDS) != 0) {
    PcieSetClkreq (2, 0);
  }

  //
  // Clear Bus Numbers
  //
  QNCMmio32And (RPBase, R_QNC_PCIE_BNUM, 0xFF0000FF);

  //
  // Additional configurations
  //

  //
  // PCI-E Unsupported Request Reporting Enable
  //
  if (RootPortConfig[RootPortIndex].Bits.PortErrorMask & PCIEXP_ROOT_PORT_URE_ENABLE) {
    QNCMmio16Or (RPBase, R_QNC_PCIE_DCTL, B_QNC_PCIE_DCTL_URE);
  }

  //
  // Device Fatal Error Reporting Enable
  //
  if (RootPortConfig[RootPortIndex].Bits.PortErrorMask & PCIEXP_ROOT_PORT_FEE_ENABLE) {
    QNCMmio16Or (RPBase, R_QNC_PCIE_DCTL, B_QNC_PCIE_DCTL_FEE);
  }

  //
  // Device Non Fatal Error Reporting Enable
  //
  if (RootPortConfig[RootPortIndex].Bits.PortErrorMask & PCIEXP_ROOT_PORT_NFE_ENABLE) {
    QNCMmio16Or (RPBase, R_QNC_PCIE_DCTL, B_QNC_PCIE_DCTL_NFE);
  }

  //
  // Device Correctable Error Reporting Enable
  //
  if (RootPortConfig[RootPortIndex].Bits.PortErrorMask & PCIEXP_ROOT_PORT_CEE_ENABLE) {
    QNCMmio16Or (RPBase, R_QNC_PCIE_DCTL, B_QNC_PCIE_DCTL_CEE);
  }
  //
  // Root PCI-E PME Interrupt Enable
  //
  if (RootPortConfig[RootPortIndex].Bits.PmeInterruptEnable) {
    QNCMmio16Or (RPBase, R_QNC_PCIE_RCTL, B_QNC_PCIE_RCTL_PIE);
  }
  //
  // Root PCI-E System Error on Fatal Error Enable
  //
  if (RootPortConfig[RootPortIndex].Bits.PortErrorMask & PCIEXP_ROOT_PORT_SFE_ENABLE) {
    QNCMmio16Or (RPBase, R_QNC_PCIE_RCTL, B_QNC_PCIE_RCTL_SFE);
  }

  //
  // Root PCI-E System Error on Non-Fatal Error Enable
  //
  if (RootPortConfig[RootPortIndex].Bits.PortErrorMask & PCIEXP_ROOT_PORT_SNE_ENABLE) {
    QNCMmio16Or (RPBase, R_QNC_PCIE_RCTL, B_QNC_PCIE_RCTL_SNE);
  }

  //
  // Root PCI-E System Error on Correctable Error Enable
  //
  if (RootPortConfig[RootPortIndex].Bits.PortErrorMask & PCIEXP_ROOT_PORT_SCE_ENABLE) {
    QNCMmio16Or (RPBase, R_QNC_PCIE_RCTL, B_QNC_PCIE_RCTL_SCE);
  }

  //
  // Root PCI-E Powermanagement SCI Enabled
  //
  if (RootPortConfig[RootPortIndex].Bits.PmSciEnable) {
    //
    // Make sure that PME Interrupt Enable bit of Root Control register
    // of PCI Express Capability struceture is cleared
    //
    QNCMmio32And (RPBase, R_QNC_PCIE_RCTL, (~B_QNC_PCIE_RCTL_PIE));
    QNCMmio32AndThenOr (RPBase, R_QNC_PCIE_MPC, (~B_QNC_PCIE_MPC_PMME), B_QNC_PCIE_MPC_PMCE);

    //
    // Make sure GPE0 Stutus RW1C Bit is clear.
    //
    DwordReg = IoRead32 (QNCGpeBase + R_QNC_GPE0BLK_GPE0S);
    if ((DwordReg & B_QNC_GPE0BLK_GPE0S_PCIE) != 0) {
      IoWrite32 (QNCGpeBase + R_QNC_GPE0BLK_GPE0S, B_QNC_GPE0BLK_GPE0S_PCIE);
    }
  }

  //
  // PCIe Hot Plug SCI Enable
  //
  if (RootPortConfig[RootPortIndex].Bits.HotplugSciEnable) {
    //
    // Write clear for :
    // Attention Button Pressed (bit0)
    // Presence Detect Changed (bit3)
    //
    QNCMmio32Or (RPBase, R_QNC_PCIE_SLSTS, (B_QNC_PCIE_SLSTS_PDC | B_QNC_PCIE_SLSTS_ABP));

    //
    // Sequence 2: Program the following bits in Slot Control register at offset 18h
    // of PCI Express* Capability structure:
    // Attention Button Pressed Enable (bit0) = 1b
    // Presence Detect Changed Enable (bit3) = 1b
    // Hot Plug Interrupt Enable (bit5) = 0b
    //
    QNCMmio32AndThenOr (RPBase, R_QNC_PCIE_SLCTL, (~B_QNC_PCIE_SLCTL_HPE), (B_QNC_PCIE_SLCTL_PDE | B_QNC_PCIE_SLCTL_ABE));

    //
    // Sequence 3: Program Misc Port Config (MPC) register at PCI config space offset
    // D8h as follows:
    // Hot Plug SCI Enable (HPCE, bit30) = 1b
    // Hot Plug SMI Enable (HPME, bit1) = 0b
    //
    QNCMmio32AndThenOr (RPBase, R_QNC_PCIE_MPC, (~B_QNC_PCIE_MPC_HPME), B_QNC_PCIE_MPC_HPCE);
  }


  return EFI_SUCCESS;
}


/**
  Perform Initialization of the Downstream Root Ports
**/
VOID
QNCDownStreamPortsInit (
  IN PCIEXP_ROOT_PORT_CONFIGURATION             *RootPortConfig,
  IN QNC_DEVICE_ENABLES                      *QNCDeviceEnables,
  IN UINT64                                     PciExpressBar,
  IN UINT32                                     QNCRootComplexBar,
  IN UINT32                                     QNCPmioBase,
  IN UINT32                                     QNCGpeBase,
  OUT UINTN                                     *RpEnableMask
  )
{
  EFI_STATUS     Status;
  UINT32         Index;

  //
  // Initialize every root port and downstream device
  //
  for (Index = 0;Index < MAX_PCI_EXPRESS_ROOT_PORTS;Index++) {
    if ((QNCDeviceEnables->Uint32 & (1 << Index)) != 0) {
      Status = QNCRootPortInit (
               Index,
               RootPortConfig,
               PciExpressBar,
               QNCRootComplexBar,
               QNCPmioBase,
               QNCGpeBase
               );

      if (!EFI_ERROR (Status)) {
        (*RpEnableMask) |= LShiftU64(1, Index);
        DEBUG ((EFI_D_INFO, " Root Port %x device found, enabled. RpEnableMask: 0x%x\n", Index + 1, *RpEnableMask));
      }
    }
  }
}

/**
  Do early init of pci express rootports on Soc.

**/

VOID
EFIAPI
PciExpressEarlyInit (
  VOID
  )
{
  //
  // Setup Message Bus Idle Counter (SBIC) values.
  //
  QNCMmPci8(0, PCI_BUS_NUMBER_QNC, PCI_DEVICE_NUMBER_PCIE_ROOTPORT, PCI_FUNCTION_NUMBER_PCIE_ROOTPORT_0, R_QNC_PCIE_IOSFSBCTL) = QNCMmPci8AndThenOr(0, PCI_BUS_NUMBER_QNC, PCI_DEVICE_NUMBER_PCIE_ROOTPORT, PCI_FUNCTION_NUMBER_PCIE_ROOTPORT_0, R_QNC_PCIE_IOSFSBCTL, (~B_QNC_PCIE_IOSFSBCTL_SBIC_MASK), V_PCIE_ROOT_PORT_SBIC_VALUE);
  QNCMmPci8(0, PCI_BUS_NUMBER_QNC, PCI_DEVICE_NUMBER_PCIE_ROOTPORT, PCI_FUNCTION_NUMBER_PCIE_ROOTPORT_1, R_QNC_PCIE_IOSFSBCTL) = QNCMmPci8AndThenOr(0, PCI_BUS_NUMBER_QNC, PCI_DEVICE_NUMBER_PCIE_ROOTPORT, PCI_FUNCTION_NUMBER_PCIE_ROOTPORT_1, R_QNC_PCIE_IOSFSBCTL, (~B_QNC_PCIE_IOSFSBCTL_SBIC_MASK), V_PCIE_ROOT_PORT_SBIC_VALUE);

  //
  // Program SVID/SID the same as VID/DID for Root ports.
  //
  QNCMmPci32(0, PCI_BUS_NUMBER_QNC, PCI_DEVICE_NUMBER_PCIE_ROOTPORT, PCI_FUNCTION_NUMBER_PCIE_ROOTPORT_0, R_QNC_PCIE_SVID) = QNCMmPci32(0, PCI_BUS_NUMBER_QNC, PCI_DEVICE_NUMBER_PCIE_ROOTPORT, PCI_FUNCTION_NUMBER_PCIE_ROOTPORT_0, PCI_VENDOR_ID_OFFSET);
  QNCMmPci32(0, PCI_BUS_NUMBER_QNC, PCI_DEVICE_NUMBER_PCIE_ROOTPORT, PCI_FUNCTION_NUMBER_PCIE_ROOTPORT_1, R_QNC_PCIE_SVID) = QNCMmPci32(0, PCI_BUS_NUMBER_QNC, PCI_DEVICE_NUMBER_PCIE_ROOTPORT, PCI_FUNCTION_NUMBER_PCIE_ROOTPORT_1, PCI_VENDOR_ID_OFFSET);

  //
  // Set the IPF bit in MCR2
  //
  QNCMmPci32(0, PCI_BUS_NUMBER_QNC, PCI_DEVICE_NUMBER_PCIE_ROOTPORT, PCI_FUNCTION_NUMBER_PCIE_ROOTPORT_0, R_QNC_PCIE_MPC2) = QNCMmPci32Or(0, PCI_BUS_NUMBER_QNC, PCI_DEVICE_NUMBER_PCIE_ROOTPORT, PCI_FUNCTION_NUMBER_PCIE_ROOTPORT_0, R_QNC_PCIE_MPC2, B_QNC_PCIE_MPC2_IPF);
  QNCMmPci32(0, PCI_BUS_NUMBER_QNC, PCI_DEVICE_NUMBER_PCIE_ROOTPORT, PCI_FUNCTION_NUMBER_PCIE_ROOTPORT_1, R_QNC_PCIE_MPC2) = QNCMmPci32Or(0, PCI_BUS_NUMBER_QNC, PCI_DEVICE_NUMBER_PCIE_ROOTPORT, PCI_FUNCTION_NUMBER_PCIE_ROOTPORT_1, R_QNC_PCIE_MPC2, B_QNC_PCIE_MPC2_IPF);

  //
  // Set up the Posted and Non Posted Request sizes for PCIe
  //
  QNCMmPci32(0, PCI_BUS_NUMBER_QNC, PCI_DEVICE_NUMBER_PCIE_ROOTPORT, PCI_FUNCTION_NUMBER_PCIE_ROOTPORT_0, R_QNC_PCIE_CCFG) = QNCMmPci32AndThenOr(0, PCI_BUS_NUMBER_QNC, PCI_DEVICE_NUMBER_PCIE_ROOTPORT, PCI_FUNCTION_NUMBER_PCIE_ROOTPORT_0, R_QNC_PCIE_CCFG, ~B_QNC_PCIE_CCFG_UPSD, (B_QNC_PCIE_CCFG_UNRS | B_QNC_PCIE_CCFG_UPRS));

  return;
}


/**
  Complete initialization all the pci express rootports on Soc.
**/
EFI_STATUS
EFIAPI
PciExpressInit (
  )
{
  UINT64                            PciExpressBar;
  UINT32                            QNCRootComplexBar;
  UINT32                            QNCPmioBase;
  UINT32                            QNCGpeBase;
  UINTN                             RpEnableMask;
  PCIEXP_ROOT_PORT_CONFIGURATION    *mRootPortConfig;
  QNC_DEVICE_ENABLES                mQNCDeviceEnables;

  //
  // Get BAR registers
  //
  QNCRootComplexBar  = QNC_RCRB_BASE;
  QNCPmioBase        = LpcPciCfg32 (R_QNC_LPC_PM1BLK) & B_QNC_LPC_PM1BLK_MASK;
  QNCGpeBase         = LpcPciCfg32 (R_QNC_LPC_GPE0BLK) & B_QNC_LPC_GPE0BLK_MASK;
  RpEnableMask = 0;                 // assume all root ports are disabled

  PciExpressBar = PcdGet64 (PcdPciExpressBaseAddress);

  //
  // Get platform information from PCD entries
  //
  mQNCDeviceEnables.Uint32 = PcdGet32 (PcdDeviceEnables);
  mRootPortConfig = (PCIEXP_ROOT_PORT_CONFIGURATION*) PcdGetPtr (PcdPcieRootPortConfiguration);

  DEBUG ((EFI_D_INFO, " mRootPortConfig: 0x%x,  value1: 0x%x, value2: 0x%x, value3: 0x%x, value4: 0x%x\n",
          mRootPortConfig, mRootPortConfig[0].Uint32, mRootPortConfig[1].Uint32,
          mRootPortConfig[2].Uint32, mRootPortConfig[3].Uint32));

  QNCDownStreamPortsInit (
                         mRootPortConfig,
                         &mQNCDeviceEnables,
                         PciExpressBar,
                         QNCRootComplexBar,
                         QNCPmioBase,
                         QNCGpeBase,
                         &RpEnableMask
                         );

  return EFI_SUCCESS;
}

