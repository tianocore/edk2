/** @file
*
*  Copyright (c) 2012-2014, ARM Limited. All rights reserved.
*
*  This program and the accompanying materials
*  are licensed and made available under the terms and conditions of the BSD License
*  which accompanies this distribution.  The full text of the license may be found at
*  http://opensource.org/licenses/bsd-license.php
*
*  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
*  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
*
**/

#include "Lan9118Dxe.h"


typedef struct {
  MAC_ADDR_DEVICE_PATH      Lan9118;
  EFI_DEVICE_PATH_PROTOCOL  End;
} LAN9118_DEVICE_PATH;

LAN9118_DEVICE_PATH Lan9118PathTemplate =  {
  {
    {
      MESSAGING_DEVICE_PATH, MSG_MAC_ADDR_DP,
      { (UINT8) (sizeof(MAC_ADDR_DEVICE_PATH)), (UINT8) ((sizeof(MAC_ADDR_DEVICE_PATH)) >> 8) }
    },
    { 0 },
    0
  },
  {
    END_DEVICE_PATH_TYPE,
    END_ENTIRE_DEVICE_PATH_SUBTYPE,
    sizeof(EFI_DEVICE_PATH_PROTOCOL),
    0
  }
};

/*
**  Entry point for the LAN9118 driver
**
*/
EFI_STATUS
Lan9118DxeEntry (
  IN EFI_HANDLE Handle,
  IN EFI_SYSTEM_TABLE *SystemTable
  )
{
  EFI_STATUS                   Status;
  LAN9118_DRIVER              *LanDriver;
  EFI_SIMPLE_NETWORK_PROTOCOL *Snp;
  EFI_SIMPLE_NETWORK_MODE     *SnpMode;
  LAN9118_DEVICE_PATH         *Lan9118Path;
  EFI_HANDLE                   ControllerHandle;

  // The PcdLan9118DxeBaseAddress PCD must be defined
  ASSERT (PcdGet32 (PcdLan9118DxeBaseAddress) != 0);

  // Allocate Resources
  LanDriver = AllocateZeroPool (sizeof (LAN9118_DRIVER));
  if (LanDriver == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }
  Lan9118Path = (LAN9118_DEVICE_PATH*)AllocateCopyPool (sizeof (LAN9118_DEVICE_PATH), &Lan9118PathTemplate);
  if (Lan9118Path == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  // Initialize pointers
  Snp = &(LanDriver->Snp);
  SnpMode = &(LanDriver->SnpMode);
  Snp->Mode = SnpMode;

  // Set the signature of the LAN Driver structure
  LanDriver->Signature = LAN9118_SIGNATURE;

  // Assign fields and func pointers
  Snp->Revision = EFI_SIMPLE_NETWORK_PROTOCOL_REVISION;
  Snp->WaitForPacket = NULL;
  Snp->Initialize = SnpInitialize;
  Snp->Start = SnpStart;
  Snp->Stop = SnpStop;
  Snp->Reset = SnpReset;
  Snp->Shutdown = SnpShutdown;
  Snp->ReceiveFilters = SnpReceiveFilters;
  Snp->StationAddress = SnpStationAddress;
  Snp->Statistics = SnpStatistics;
  Snp->MCastIpToMac = SnpMcastIptoMac;
  Snp->NvData = SnpNvData;
  Snp->GetStatus = SnpGetStatus;
  Snp->Transmit = SnpTransmit;
  Snp->Receive = SnpReceive;

  // Start completing simple network mode structure
  SnpMode->State = EfiSimpleNetworkStopped;
  SnpMode->HwAddressSize = NET_ETHER_ADDR_LEN; // HW address is 6 bytes
  SnpMode->MediaHeaderSize = sizeof(ETHER_HEAD); // Not sure of this
  SnpMode->MaxPacketSize = EFI_PAGE_SIZE; // Preamble + SOF + Ether Frame (with VLAN tag +4bytes)
  SnpMode->NvRamSize = 0;           // No NVRAM with this device
  SnpMode->NvRamAccessSize = 0; // No NVRAM with this device

  // Update network mode information
  SnpMode->ReceiveFilterMask = EFI_SIMPLE_NETWORK_RECEIVE_MULTICAST |
                                 EFI_SIMPLE_NETWORK_RECEIVE_UNICAST |
                                 EFI_SIMPLE_NETWORK_RECEIVE_BROADCAST |
                                 EFI_SIMPLE_NETWORK_RECEIVE_PROMISCUOUS;/* |
                                 EFI_SIMPLE_NETWORK_RECEIVE_PROMISCUOUS_MULTICAST;*/
  // Current allowed settings
  SnpMode->ReceiveFilterSetting = EFI_SIMPLE_NETWORK_RECEIVE_MULTICAST |
                                    EFI_SIMPLE_NETWORK_RECEIVE_UNICAST |
                                    EFI_SIMPLE_NETWORK_RECEIVE_BROADCAST;

  // LAN9118 has 64bit hash table, can filter 64 MCast MAC Addresses
  SnpMode->MaxMCastFilterCount = MAX_MCAST_FILTER_CNT;
  SnpMode->MCastFilterCount = 0;
  ZeroMem (&SnpMode->MCastFilter, MAX_MCAST_FILTER_CNT * sizeof(EFI_MAC_ADDRESS));

  // Set the interface type (1: Ethernet or 6: IEEE 802 Networks)
  SnpMode->IfType = NET_IFTYPE_ETHERNET;

  // Mac address is changeable as it is loaded from erasable memory
  SnpMode->MacAddressChangeable = TRUE;

  // Can only transmit one packet at a time
  SnpMode->MultipleTxSupported = FALSE;

  // MediaPresent checks for cable connection and partner link
  SnpMode->MediaPresentSupported = TRUE;
  SnpMode->MediaPresent = FALSE;

  // Set broadcast address
  SetMem (&SnpMode->BroadcastAddress, sizeof (EFI_MAC_ADDRESS), 0xFF);

  // Power up the device so we can find the MAC address
  Status = Lan9118Initialize (Snp);
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "Lan9118: Error initialising hardware\n"));
    return EFI_DEVICE_ERROR;
  }

  // Assign fields for device path
  CopyMem (&Lan9118Path->Lan9118.MacAddress, &Snp->Mode->CurrentAddress, NET_ETHER_ADDR_LEN);
  Lan9118Path->Lan9118.IfType = Snp->Mode->IfType;

  // Initialise the protocol
  ControllerHandle = NULL;
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &ControllerHandle,
                  &gEfiSimpleNetworkProtocolGuid, Snp,
                  &gEfiDevicePathProtocolGuid, Lan9118Path,
                  NULL
                  );
  // Say what the status of loading the protocol structure is
  if (EFI_ERROR(Status)) {
    FreePool (LanDriver);
  } else {
    LanDriver->ControllerHandle = ControllerHandle;
  }

  return Status;
}

/*
 *  UEFI Start() function
 *
 *  Parameters:
 *
 *  @param Snp:  A pointer to the EFI_SIMPLE_NETWORK_PROTOCOL instance.
 *
 *  Description:
 *
 *    This function starts a network interface. If the network interface successfully starts, then
 *    EFI_SUCCESS will be returned.
 */
EFI_STATUS
EFIAPI
SnpStart (
  IN        EFI_SIMPLE_NETWORK_PROTOCOL* Snp
 )
{
  // Check Snp instance
  if (Snp == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  // Check state
  if ((Snp->Mode->State == EfiSimpleNetworkStarted) || (Snp->Mode->State == EfiSimpleNetworkInitialized)) {
    return EFI_ALREADY_STARTED;
  } else if (Snp->Mode->State == EfiSimpleNetworkMaxState) {
    return EFI_DEVICE_ERROR;
  }

  // Change state
  Snp->Mode->State = EfiSimpleNetworkStarted;
  return EFI_SUCCESS;
}

/*
 *  UEFI Stop() function
 *
 */
EFI_STATUS
EFIAPI
SnpStop (
  IN        EFI_SIMPLE_NETWORK_PROTOCOL* Snp
  )
{
  // Check Snp Instance
  if (Snp == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  // Check state of the driver
  if ((Snp->Mode->State == EfiSimpleNetworkStopped) || (Snp->Mode->State == EfiSimpleNetworkMaxState)) {
    return EFI_NOT_STARTED;
  }

  // Stop the Tx and Rx
  StopTx (STOP_TX_CFG | STOP_TX_MAC, Snp);
  StopRx (0, Snp);

  // Change the state
  switch (Snp->Mode->State) {
    case EfiSimpleNetworkStarted:
    case EfiSimpleNetworkInitialized:
      Snp->Mode->State = EfiSimpleNetworkStopped;
      break;
    default:
      return EFI_DEVICE_ERROR;
  }

  // Put the device into a power saving mode ?
  return EFI_SUCCESS;
}


// Allocated receive and transmit buffers
STATIC UINT32 gTxBuffer = 0;

/*
 *  UEFI Initialize() function
 *
 */
EFI_STATUS
EFIAPI
SnpInitialize (
  IN        EFI_SIMPLE_NETWORK_PROTOCOL* Snp,
  IN        UINTN                        RxBufferSize    OPTIONAL,
  IN        UINTN                        TxBufferSize    OPTIONAL
  )
{
  EFI_STATUS Status;
  UINT32     PmConf;
  INT32      AllocResult;
  UINT32     RxStatusSize;
  UINT32     TxStatusSize;

  // Initialize variables
  // Global variables to hold tx and rx FIFO allocation
  gTxBuffer = 0;

  // Check Snp Instance
  if (Snp == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  // First check that driver has not already been initialized
  if (Snp->Mode->State == EfiSimpleNetworkInitialized) {
    DEBUG ((EFI_D_WARN, "LAN9118 Driver already initialized\n"));
    return EFI_SUCCESS;
  } else
  if (Snp->Mode->State == EfiSimpleNetworkStopped) {
    DEBUG ((EFI_D_WARN, "LAN9118 Driver not started\n"));
    return EFI_NOT_STARTED;
  }

  // Initiate a PHY reset
  if (PhySoftReset (PHY_RESET_PMT | PHY_RESET_CHECK_LINK, Snp) < 0) {
    Snp->Mode->State = EfiSimpleNetworkStopped;
    DEBUG ((EFI_D_WARN, "Warning: Link not ready after TimeOut. Check ethernet cable\n"));
    return EFI_NOT_STARTED;
  }

  // Initiate a software reset
  Status = SoftReset (0, Snp);
  if (EFI_ERROR(Status)) {
    DEBUG ((EFI_D_WARN, "Soft Reset Failed: Hardware Error\n"));
    return EFI_DEVICE_ERROR;
  }

  // Read the PM register
  PmConf = MmioRead32 (LAN9118_PMT_CTRL);

  // MPTCTRL_WOL_EN: Allow Wake-On-Lan to detect wake up frames or magic packets
  // MPTCTRL_ED_EN:  Allow energy detection to allow lowest power consumption mode
  // MPTCTRL_PME_EN: Allow Power Management Events
  PmConf = 0;
  PmConf |= (MPTCTRL_WOL_EN | MPTCTRL_ED_EN | MPTCTRL_PME_EN);

  // Write the current configuration to the register
  MmioWrite32 (LAN9118_PMT_CTRL, PmConf);
  gBS->Stall (LAN9118_STALL);
  gBS->Stall (LAN9118_STALL);

  // Configure GPIO and HW
  Status = ConfigureHardware (HW_CONF_USE_LEDS, Snp);
  if (EFI_ERROR(Status)) {
    return Status;
  }

  // Assign the transmitter buffer size (default values)
  TxStatusSize = LAN9118_TX_STATUS_SIZE;
  RxStatusSize = LAN9118_RX_STATUS_SIZE;

  // Check that a buff size was specified
  if (TxBufferSize > 0) {
    if (RxBufferSize == 0) {
      RxBufferSize = LAN9118_RX_DATA_SIZE;
    }

    AllocResult = ChangeFifoAllocation (
                          ALLOC_USE_FIFOS,
                          &TxBufferSize,
                          &RxBufferSize,
                          &TxStatusSize,
                          &RxStatusSize,
                          Snp
                          );

    if (AllocResult < 0) {
      return EFI_OUT_OF_RESOURCES;
    }
  }

  // Do auto-negotiation if supported
  Status = AutoNegotiate (AUTO_NEGOTIATE_ADVERTISE_ALL, Snp);
  if (EFI_ERROR(Status)) {
    DEBUG ((EFI_D_WARN, "Lan9118: Auto Negociation not supported.\n"));
  }

  // Configure flow control depending on speed capabilities
  Status = ConfigureFlow (0, 0, 0, 0, Snp);
  if (EFI_ERROR(Status)) {
    return Status;
  }

  // Enable the receiver and transmitter
  Status = StartRx (0, Snp);
  if (EFI_ERROR(Status)) {
    return Status;
  }

  Status = StartTx (START_TX_MAC | START_TX_CFG, Snp);
  if (EFI_ERROR(Status)) {
    return Status;
  }

  // Now acknowledge all interrupts
  MmioWrite32 (LAN9118_INT_STS, ~0);

  // Declare the driver as initialized
  Snp->Mode->State = EfiSimpleNetworkInitialized;

  return Status;
}

/*
 *  UEFI Reset () function
 *
 */
EFI_STATUS
EFIAPI
SnpReset (
  IN        EFI_SIMPLE_NETWORK_PROTOCOL* Snp,
  IN        BOOLEAN Verification
  )
{
  UINT32 PmConf;
  UINT32 HwConf;
  UINT32 ResetFlags;

  PmConf = 0;
  HwConf = 0;
  ResetFlags = 0;

  // Check Snp Instance
  if (Snp == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  // First check that driver has not already been initialized
  if (Snp->Mode->State == EfiSimpleNetworkStarted) {
    DEBUG ((EFI_D_WARN, "Warning: LAN9118 Driver not yet initialized\n"));
    return EFI_DEVICE_ERROR;
  } else if (Snp->Mode->State == EfiSimpleNetworkStopped) {
    DEBUG ((EFI_D_WARN, "Warning: LAN9118 Driver not started\n"));
    return EFI_NOT_STARTED;
  }

  // Initiate a PHY reset
  if (PhySoftReset (PHY_RESET_PMT | PHY_RESET_CHECK_LINK, Snp) < 0) {
    Snp->Mode->State = EfiSimpleNetworkStopped;
    return EFI_NOT_STARTED;
  }

  // Initiate a software reset
  ResetFlags |= SOFT_RESET_CHECK_MAC_ADDR_LOAD | SOFT_RESET_CLEAR_INT;

  if (Verification) {
    ResetFlags |= SOFT_RESET_SELF_TEST;
  }

  if (SoftReset (ResetFlags, Snp) < 0) {
    DEBUG ((EFI_D_WARN, "Warning: Soft Reset Failed: Hardware Error\n"));
    return EFI_DEVICE_ERROR;
  }

  // Read the PM register
  PmConf = MmioRead32 (LAN9118_PMT_CTRL);

  // MPTCTRL_WOL_EN: Allow Wake-On-Lan to detect wake up frames or magic packets
  // MPTCTRL_ED_EN:  Allow energy detection to allow lowest power consumption mode
  // MPTCTRL_PME_EN: Allow Power Management Events
  PmConf |= (MPTCTRL_WOL_EN | MPTCTRL_ED_EN | MPTCTRL_PME_EN);

  // Write the current configuration to the register
  MmioWrite32 (LAN9118_PMT_CTRL, PmConf);
  gBS->Stall (LAN9118_STALL);

  // Check that a buffer size was specified in SnpInitialize
  if (gTxBuffer != 0) {
    HwConf = MmioRead32 (LAN9118_HW_CFG);        // Read the HW register
    HwConf &= ~HW_CFG_TX_FIFO_SIZE_MASK;         // Clear buffer bits first
    HwConf |= HW_CFG_TX_FIFO_SIZE(gTxBuffer);    // assign size chosen in SnpInitialize

    MmioWrite32 (LAN9118_HW_CFG, HwConf);        // Write the conf
    gBS->Stall (LAN9118_STALL);
  }

  // Enable the receiver and transmitter and clear their contents
  StartRx (START_RX_CLEAR, Snp);
  StartTx (START_TX_MAC | START_TX_CFG | START_TX_CLEAR, Snp);

  // Now acknowledge all interrupts
  MmioWrite32 (LAN9118_INT_STS, ~0);

  return EFI_SUCCESS;
}

/*
 *  UEFI Shutdown () function
 *
 */
EFI_STATUS
EFIAPI
SnpShutdown (
  IN        EFI_SIMPLE_NETWORK_PROTOCOL* Snp
  )
{
  // Check Snp Instance
  if (Snp == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  // First check that driver has not already been initialized
  if (Snp->Mode->State == EfiSimpleNetworkStarted) {
    DEBUG ((EFI_D_WARN, "Warning: LAN9118 Driver not yet initialized\n"));
    return EFI_DEVICE_ERROR;
  } else if (Snp->Mode->State == EfiSimpleNetworkStopped) {
    DEBUG ((EFI_D_WARN, "Warning: LAN9118 Driver in stopped state\n"));
    return EFI_NOT_STARTED;
  }

  // Initiate a PHY reset
  PhySoftReset (PHY_RESET_PMT, Snp);

  // Initiate a software reset
  if (SoftReset (0, Snp) < 0) {
    DEBUG ((EFI_D_WARN, "Warning: Soft Reset Failed: Hardware Error\n"));
    return EFI_DEVICE_ERROR;
  }

  return EFI_SUCCESS;
}


/*
 *  UEFI ReceiveFilters() function
 *
 */
EFI_STATUS
EFIAPI
SnpReceiveFilters (
  IN        EFI_SIMPLE_NETWORK_PROTOCOL* Snp,
  IN        UINT32 Enable,
  IN        UINT32 Disable,
  IN        BOOLEAN Reset,
  IN        UINTN NumMfilter          OPTIONAL,
  IN        EFI_MAC_ADDRESS *Mfilter  OPTIONAL
  )
{
  UINT32 MacCSRValue;
  UINT32 MultHashTableHigh;
  UINT32 MultHashTableLow;
  UINT32 Crc;
  UINT8 BitToSelect;
  UINT32 Count;

  MacCSRValue = 0;
  MultHashTableHigh = 0;
  MultHashTableLow = 0;
  Crc = 0xFFFFFFFF;
  BitToSelect = 0;
  Count = 0;

  // Check that driver was started and initialised
  if (Snp->Mode->State == EfiSimpleNetworkStarted) {
    DEBUG ((EFI_D_WARN, "Warning: LAN9118 Driver not initialized\n"));
    return EFI_DEVICE_ERROR;
  } else if (Snp->Mode->State == EfiSimpleNetworkStopped) {
    DEBUG ((EFI_D_WARN, "Warning: LAN9118 Driver in stopped state\n"));
    return EFI_NOT_STARTED;
  }

  // If reset then clear the filter registers
  if (Reset) {
    Enable |= EFI_SIMPLE_NETWORK_RECEIVE_MULTICAST;
    IndirectMACWrite32 (INDIRECT_MAC_INDEX_HASHL, 0x00000000);
    IndirectMACWrite32 (INDIRECT_MAC_INDEX_HASHH, 0x00000000);
  }

  // Set the hash tables
  if ((NumMfilter > 0) && (!Reset)) {

    // Read the Multicast High Hash Table
    MultHashTableHigh = IndirectMACRead32 (INDIRECT_MAC_INDEX_HASHH);

    // Read the Multicast Low Hash Table
    MultHashTableLow = IndirectMACRead32 (INDIRECT_MAC_INDEX_HASHL);

    // Go through each filter address and set appropriate bits on hash table
    for (Count = 0; Count < NumMfilter; Count++) {

      // Generate a 32-bit CRC for Ethernet
      Crc = GenEtherCrc32 (&Mfilter[Count],6);
      //gBS->CalculateCrc32 ((VOID*)&Mfilter[Count],6,&Crc); <-- doesn't work as desired

      // Get the most significant 6 bits to index hash registers
      BitToSelect = (Crc >> 26) & 0x3F;

      // Select hashlow register if MSB is not set
      if ((BitToSelect & 0x20) == 0) {
        MultHashTableLow |= (1 << BitToSelect);
      } else {
        MultHashTableHigh |= (1 << (BitToSelect & 0x1F));
      }
    }

    // Write the desired hash
    IndirectMACWrite32 (INDIRECT_MAC_INDEX_HASHL, MultHashTableLow);
    IndirectMACWrite32 (INDIRECT_MAC_INDEX_HASHH, MultHashTableHigh);
  }

  // Read MAC controller
  MacCSRValue = IndirectMACRead32 (INDIRECT_MAC_INDEX_CR);

  // Set the options for the MAC_CSR
  if (Enable & EFI_SIMPLE_NETWORK_RECEIVE_UNICAST) {
    StartRx (0, Snp);
    DEBUG ((DEBUG_NET, "Allowing Unicast Frame Reception\n"));
  }

  if (Disable & EFI_SIMPLE_NETWORK_RECEIVE_UNICAST) {
    StopRx (0, Snp);
    DEBUG ((DEBUG_NET, "Disabling Unicast Frame Reception\n"));
  }

  if (Enable & EFI_SIMPLE_NETWORK_RECEIVE_MULTICAST) {
    MacCSRValue |= MACCR_HPFILT;
    DEBUG ((DEBUG_NET, "Allowing Multicast Frame Reception\n"));
  }

  if (Disable & EFI_SIMPLE_NETWORK_RECEIVE_MULTICAST) {
    MacCSRValue &= ~MACCR_HPFILT;
    DEBUG ((DEBUG_NET, "Disabling Multicast Frame Reception\n"));
  }

  if (Enable & EFI_SIMPLE_NETWORK_RECEIVE_BROADCAST) {
    MacCSRValue &= ~(MACCR_BCAST);
    DEBUG ((DEBUG_NET, "Allowing Broadcast Frame Reception\n"));
  }

  if (Disable & EFI_SIMPLE_NETWORK_RECEIVE_BROADCAST) {
    MacCSRValue |= MACCR_BCAST;
    DEBUG ((DEBUG_NET, "Disabling Broadcast Frame Reception\n"));
  }

  if (Enable & EFI_SIMPLE_NETWORK_RECEIVE_PROMISCUOUS) {
    MacCSRValue |= MACCR_PRMS;
    DEBUG ((DEBUG_NET, "Enabling Promiscuous Mode\n"));
  }

  if (Disable & EFI_SIMPLE_NETWORK_RECEIVE_PROMISCUOUS) {
    MacCSRValue &= ~MACCR_PRMS;
    DEBUG ((DEBUG_NET, "Disabling Promiscuous Mode\n"));
  }

  if (Enable & EFI_SIMPLE_NETWORK_RECEIVE_PROMISCUOUS_MULTICAST) {
    MacCSRValue |= (MACCR_HPFILT | MACCR_PRMS);
    DEBUG ((DEBUG_NET, "Enabling Promiscuous Multicast Mode\n"));
  }

  if (Disable & EFI_SIMPLE_NETWORK_RECEIVE_PROMISCUOUS_MULTICAST) {
    MacCSRValue &= ~(MACCR_HPFILT | MACCR_PRMS);
    DEBUG ((DEBUG_NET, "Disabling Promiscuous Multicast Mode\n"));
  }

  // Write the options to the MAC_CSR
  IndirectMACWrite32 (INDIRECT_MAC_INDEX_CR, MacCSRValue);
  gBS->Stall (LAN9118_STALL);

  return EFI_SUCCESS;
}

/*
 *  UEFI StationAddress() function
 *
 */
EFI_STATUS
EFIAPI
SnpStationAddress (
  IN        EFI_SIMPLE_NETWORK_PROTOCOL *Snp,
  IN        BOOLEAN Reset,
  IN        EFI_MAC_ADDRESS *NewMac
)
{
  DEBUG ((DEBUG_NET, "SnpStationAddress()\n"));

  UINT32 Count;
  UINT8  PermAddr[6];
  UINT64 DefaultMacAddress;

  Count = 0;

  // Check Snp instance
  if (Snp == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  // Check that driver was started and initialised
  if (Snp->Mode->State == EfiSimpleNetworkStarted) {
    DEBUG ((EFI_D_WARN, "Warning: LAN9118 Driver not initialized\n"));
    return EFI_DEVICE_ERROR;
  } else if (Snp->Mode->State == EfiSimpleNetworkStopped) {
    DEBUG ((EFI_D_WARN, "Warning: LAN9118 Driver in stopped state\n"));
    return EFI_NOT_STARTED;
  }

  // Get the Permanent MAC address if need reset
  if (Reset) {
    // Try using EEPROM first. Read the first byte of data from EEPROM at the address 0x0
    if ((IndirectEEPROMRead32 (0) & 0xFF) == EEPROM_EXTERNAL_SERIAL_EEPROM) {
      for (Count = 1; Count < 7; Count++) {
        PermAddr[Count - 1] = IndirectEEPROMRead32 (Count);
      }

      // Write address
      Lan9118SetMacAddress ((EFI_MAC_ADDRESS *) PermAddr, Snp);
    } else {
      DEBUG ((EFI_D_ERROR, "Lan9118: Warning: No valid MAC address in EEPROM, using fallback\n"));
      DefaultMacAddress = FixedPcdGet64 (PcdLan9118DefaultMacAddress);
      Lan9118SetMacAddress ((EFI_MAC_ADDRESS *) &DefaultMacAddress, Snp);
    }
  } else {
    // Otherwise use the specified new MAC address
    if (NewMac == NULL) {
      return EFI_INVALID_PARAMETER;
    }

    // Write address
    Lan9118SetMacAddress (NewMac, Snp);
  }

  return EFI_SUCCESS;
}

/*
 *  UEFI Statistics() function
 *
 */
EFI_STATUS
EFIAPI
SnpStatistics (
  IN        EFI_SIMPLE_NETWORK_PROTOCOL* Snp,
  IN        BOOLEAN Reset,
  IN  OUT   UINTN *StatSize,
      OUT   EFI_NETWORK_STATISTICS *Statistics
  )
{
  LAN9118_DRIVER *LanDriver;

  LanDriver = INSTANCE_FROM_SNP_THIS (Snp);

  DEBUG ((DEBUG_NET, "SnpStatistics()\n"));

  // Check Snp instance
  if (Snp == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  // Check that driver was started and initialised
  if (Snp->Mode->State == EfiSimpleNetworkStarted) {
    DEBUG ((EFI_D_WARN, "Warning: LAN9118 Driver not initialized\n"));
    return EFI_DEVICE_ERROR;
  } else if (Snp->Mode->State == EfiSimpleNetworkStopped) {
    DEBUG ((EFI_D_WARN, "Warning: LAN9118 Driver in stopped state\n"));
    return EFI_NOT_STARTED;
  }

  // Check pointless condition
  if ((!Reset) && (StatSize == NULL) && (Statistics == NULL)) {
    return EFI_SUCCESS;
  }

  // Check the parameters
  if ((StatSize == NULL) && (Statistics != NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  // Do a reset if required
  if (Reset) {
    ZeroMem (&LanDriver->Stats, sizeof(EFI_NETWORK_STATISTICS));
  }

  // Check buffer size
  if (*StatSize < sizeof(EFI_NETWORK_STATISTICS)) {
    *StatSize = sizeof(EFI_NETWORK_STATISTICS);
    return EFI_BUFFER_TOO_SMALL;
  }

  // Fill in the statistics
  CopyMem(&Statistics, &LanDriver->Stats, sizeof(EFI_NETWORK_STATISTICS));

  return EFI_SUCCESS;
}

/*
 *  UEFI MCastIPtoMAC() function
 *
 */
EFI_STATUS
EFIAPI
SnpMcastIptoMac (
  IN        EFI_SIMPLE_NETWORK_PROTOCOL* Snp,
  IN        BOOLEAN IsIpv6,
  IN        EFI_IP_ADDRESS *Ip,
      OUT   EFI_MAC_ADDRESS *McastMac
  )
{
  DEBUG ((DEBUG_NET, "SnpMcastIptoMac()\n"));

  // Check Snp instance
  if (Snp == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  // Check that driver was started and initialised
  if (Snp->Mode->State == EfiSimpleNetworkStarted) {
    DEBUG ((EFI_D_WARN, "Warning: LAN9118 Driver not initialized\n"));
    return EFI_DEVICE_ERROR;
  } else if (Snp->Mode->State == EfiSimpleNetworkStopped) {
    DEBUG ((EFI_D_WARN, "Warning: LAN9118 Driver in stopped state\n"));
    return EFI_NOT_STARTED;
  }

  // Check parameters
  if ((McastMac == NULL) || (Ip == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  // Make sure MAC address is empty
  ZeroMem (McastMac, sizeof(EFI_MAC_ADDRESS));

  // If we need ipv4 address
  if (!IsIpv6) {
    // Most significant 25 bits of a multicast HW address are set.
    // 01-00-5E is the IPv4 Ethernet Multicast Address (see RFC 1112)
    McastMac->Addr[0] = 0x01;
    McastMac->Addr[1] = 0x00;
    McastMac->Addr[2] = 0x5E;

    // Lower 23 bits from ipv4 address
    McastMac->Addr[3] = (Ip->v4.Addr[1] & 0x7F); // Clear the most significant bit (25th bit of MAC must be 0)
    McastMac->Addr[4] = Ip->v4.Addr[2];
    McastMac->Addr[5] = Ip->v4.Addr[3];
  } else {
    // Most significant 16 bits of multicast v6 HW address are set
    // 33-33 is the IPv6 Ethernet Multicast Address (see RFC 2464)
    McastMac->Addr[0] = 0x33;
    McastMac->Addr[1] = 0x33;

    // lower four octets are taken from ipv6 address
    McastMac->Addr[2] = Ip->v6.Addr[8];
    McastMac->Addr[3] = Ip->v6.Addr[9];
    McastMac->Addr[4] = Ip->v6.Addr[10];
    McastMac->Addr[5] = Ip->v6.Addr[11];
  }

  return EFI_SUCCESS;
}

/*
 *  UEFI NvData() function
 *
 */
EFI_STATUS
EFIAPI
SnpNvData (
  IN        EFI_SIMPLE_NETWORK_PROTOCOL* pobj,
  IN        BOOLEAN read_write,
  IN        UINTN offset,
  IN        UINTN buff_size,
  IN  OUT   VOID *data
  )
{
  DEBUG ((DEBUG_NET, "SnpNvData()\n"));

  return EFI_UNSUPPORTED;
}


/*
 *  UEFI GetStatus () function
 *
 */
EFI_STATUS
EFIAPI
SnpGetStatus (
  IN        EFI_SIMPLE_NETWORK_PROTOCOL* Snp,
      OUT   UINT32 *IrqStat  OPTIONAL,
      OUT   VOID **TxBuff    OPTIONAL
  )
{
  UINT32          FifoInt;
  EFI_STATUS      Status;
  UINTN           NumTxStatusEntries;
  UINT32          TxStatus;
  UINT16          PacketTag;
  UINT32          Interrupts;
  LAN9118_DRIVER *LanDriver;

  LanDriver = INSTANCE_FROM_SNP_THIS (Snp);

  // Check preliminaries
  if (Snp == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (Snp->Mode->State != EfiSimpleNetworkInitialized) {
    return EFI_NOT_STARTED;
  }

  // Check and acknowledge TX Status interrupt (this will happen if the
  // consumer of SNP does not call GetStatus.)
  // TODO will we lose TxStatuses if this happens? Maybe in SnpTransmit we
  // should check for it and dump the TX Status FIFO.
  FifoInt = MmioRead32 (LAN9118_FIFO_INT);

  // Clear the TX Status FIFO Overflow
  if ((FifoInt & INSTS_TXSO) == 0) {
    FifoInt |= INSTS_TXSO;
    MmioWrite32 (LAN9118_FIFO_INT, FifoInt);
  }

  // Read interrupt status if IrqStat is not NULL
  if (IrqStat != NULL) {

    // Check for receive interrupt
    if (MmioRead32 (LAN9118_INT_STS) & INSTS_RSFL) { // Data moved from rx FIFO
      *IrqStat |= EFI_SIMPLE_NETWORK_RECEIVE_INTERRUPT;
      MmioWrite32 (LAN9118_INT_STS,INSTS_RSFL);
    } else {
      *IrqStat &= ~EFI_SIMPLE_NETWORK_RECEIVE_INTERRUPT;
    }

    // Check for transmit interrupt
    if (MmioRead32 (LAN9118_INT_STS) & INSTS_TSFL) {
      *IrqStat |= EFI_SIMPLE_NETWORK_TRANSMIT_INTERRUPT;
      MmioWrite32 (LAN9118_INT_STS,INSTS_TSFL);
    } else {
      *IrqStat &= ~EFI_SIMPLE_NETWORK_TRANSMIT_INTERRUPT;
    }

    // Check for software interrupt
    if (MmioRead32 (LAN9118_INT_STS) & INSTS_SW_INT) {
      *IrqStat |= EFI_SIMPLE_NETWORK_SOFTWARE_INTERRUPT;
      MmioWrite32 (LAN9118_INT_STS,INSTS_SW_INT);
    } else {
      *IrqStat &= ~EFI_SIMPLE_NETWORK_SOFTWARE_INTERRUPT;
    }
  }

  // Check Status of transmitted packets
  // (We ignore TXSTATUS_NO_CA has it might happen in Full Duplex)

  NumTxStatusEntries = MmioRead32(LAN9118_TX_FIFO_INF) & TXFIFOINF_TXSUSED_MASK;
  if (NumTxStatusEntries > 0) {
    TxStatus = MmioRead32 (LAN9118_TX_STATUS);
    PacketTag = TxStatus >> 16;
    TxStatus = TxStatus & 0xFFFF;
    if ((TxStatus & TXSTATUS_ES) && TxStatus != (TXSTATUS_ES | TXSTATUS_NO_CA)) {
      DEBUG ((EFI_D_ERROR, "LAN9118: There was an error transmitting. TxStatus=0x%08x:", TxStatus));
      if (TxStatus & TXSTATUS_NO_CA) {
        DEBUG ((EFI_D_ERROR, "- No carrier\n"));
      }
      if (TxStatus & TXSTATUS_DEF) {
        DEBUG ((EFI_D_ERROR, "- Packet tx was deferred\n"));
      }
      if (TxStatus & TXSTATUS_EDEF) {
        DEBUG ((EFI_D_ERROR, "- Tx ended because of excessive deferral\n"));
      }
      if (TxStatus & TXSTATUS_ECOLL) {
        DEBUG ((EFI_D_ERROR, "- Tx ended because of Excessive Collisions\n"));
      }
      if (TxStatus & TXSTATUS_LCOLL) {
        DEBUG ((EFI_D_ERROR, "- Packet Tx aborted after coll window of 64 bytes\n"));
      }
      if (TxStatus & TXSTATUS_LOST_CA) {
        DEBUG ((EFI_D_ERROR, "- Lost carrier during Tx\n"));
      }
      return EFI_DEVICE_ERROR;
    } else {
      LanDriver->Stats.TxTotalFrames += 1;
      *TxBuff = LanDriver->TxRing[PacketTag % LAN9118_TX_RING_NUM_ENTRIES];
    }
  }

  // Check for a TX Error interrupt
  Interrupts = MmioRead32 (LAN9118_INT_STS);
  if (Interrupts & INSTS_TXE) {
    DEBUG ((EFI_D_ERROR, "LAN9118: Transmitter error. Restarting..."));

    // Initiate a software reset
    if (SoftReset (0, Snp) < 0) {
      DEBUG ((EFI_D_ERROR, "\n\tSoft Reset Failed: Hardware Error\n"));
      return EFI_DEVICE_ERROR;
    }

    // Acknowledge the TXE
    MmioWrite32 (LAN9118_INT_STS, INSTS_TXE);
    gBS->Stall (LAN9118_STALL);

    // Restart the transmitter
    StartTx (START_TX_MAC | START_TX_CFG, Snp);
  }

  // Update the media status
  Status = CheckLinkStatus (0, Snp);
  if (EFI_ERROR(Status)) {
    Snp->Mode->MediaPresent = FALSE;
  } else {
    Snp->Mode->MediaPresent = TRUE;
  }

  return EFI_SUCCESS;
}


/*
 *  UEFI Transmit() function
 *
 */
EFI_STATUS
EFIAPI
SnpTransmit (
  IN  EFI_SIMPLE_NETWORK_PROTOCOL  *Snp,
  IN  UINTN                        HdrSize,
  IN  UINTN                        BuffSize,
  IN  VOID*                        Data,
  IN  EFI_MAC_ADDRESS              *SrcAddr  OPTIONAL,
  IN  EFI_MAC_ADDRESS              *DstAddr  OPTIONAL,
  IN  UINT16                       *Protocol OPTIONAL
  )
{
  LAN9118_DRIVER *LanDriver;
  UINT32 TxFreeSpace;
  UINT32 TxStatusSpace;
  INT32 Count;
  UINT32 CommandA;
  UINT32 CommandB;
  UINT16 LocalProtocol;
  UINT32 *LocalData;
  UINT16 PacketTag;

#if defined(EVAL_PERFORMANCE)
  UINT64 Perf;
  UINT64 StartClock;
  UINT64 EndClock;

  Perf = GetPerformanceCounterProperties (NULL, NULL);
  StartClock = GetPerformanceCounter ();
#endif

  LanDriver = INSTANCE_FROM_SNP_THIS (Snp);

  // Check preliminaries
  if ((Snp == NULL) || (Data == NULL)) {
    return EFI_INVALID_PARAMETER;
  }
  if (Snp->Mode->State != EfiSimpleNetworkInitialized) {
    return EFI_NOT_STARTED;
  }

  // Ensure header is correct size if non-zero
  if (HdrSize) {
    if (HdrSize != Snp->Mode->MediaHeaderSize) {
      return EFI_INVALID_PARAMETER;
    }

    if ((DstAddr == NULL) || (Protocol == NULL)) {
      return EFI_INVALID_PARAMETER;
    }
  }

  // Before transmitting check the link status
  /*if (CheckLinkStatus (0, Snp) < 0) {
    return EFI_NOT_READY;
  }*/

  // Get DATA FIFO free space in bytes
  TxFreeSpace = TxDataFreeSpace (0, Snp);
  if (TxFreeSpace < BuffSize) {
    return EFI_NOT_READY;
  }

  // Get STATUS FIFO used space in bytes
  TxStatusSpace = TxStatusUsedSpace (0, Snp);
  if (TxStatusSpace > 500) {
    return EFI_NOT_READY;
  }

  // If DstAddr is not provided, get it from Buffer (we trust that the caller
  // has provided a well-formed frame).
  if (DstAddr == NULL) {
    DstAddr = (EFI_MAC_ADDRESS *) Data;
  }

  // Check for the nature of the frame
  if ((DstAddr->Addr[0] & 0x1) == 1) {
    LanDriver->Stats.TxMulticastFrames += 1;
  } else {
    LanDriver->Stats.TxUnicastFrames += 1;
  }

  // Check if broadcast
  if (DstAddr->Addr[0] == 0xFF) {
    LanDriver->Stats.TxBroadcastFrames += 1;
  }

  PacketTag = LanDriver->NextPacketTag;
  LanDriver->NextPacketTag++;

  if (HdrSize) {

    // Format pointer
    LocalData = (UINT32*) Data;
    LocalProtocol = *Protocol;

    // Create first buffer to pass to controller (for the header)
    CommandA = TX_CMD_A_FIRST_SEGMENT | TX_CMD_A_BUFF_SIZE (HdrSize);
    CommandB = TX_CMD_B_PACKET_TAG (PacketTag) | TX_CMD_B_PACKET_LENGTH (BuffSize);

    // Write the commands first
    MmioWrite32 (LAN9118_TX_DATA, CommandA);
    MmioWrite32 (LAN9118_TX_DATA, CommandB);

    // Write the destination address
    MmioWrite32 (LAN9118_TX_DATA,
               (DstAddr->Addr[0]) |
               (DstAddr->Addr[1] << 8) |
               (DstAddr->Addr[2] << 16) |
               (DstAddr->Addr[3] << 24)
               );

    MmioWrite32 (LAN9118_TX_DATA,
               (DstAddr->Addr[4]) |
               (DstAddr->Addr[5] << 8) |
               (SrcAddr->Addr[0] << 16) | // Write the Source Address
               (SrcAddr->Addr[1] << 24)
               );

    MmioWrite32 (LAN9118_TX_DATA,
               (SrcAddr->Addr[2]) |
               (SrcAddr->Addr[3] << 8) |
               (SrcAddr->Addr[4] << 16) |
               (SrcAddr->Addr[5] << 24)
               );

    // Write the Protocol
    MmioWrite32 (LAN9118_TX_DATA, (UINT32)(HTONS (LocalProtocol)));

    // Next buffer is the payload
    CommandA = TX_CMD_A_LAST_SEGMENT | TX_CMD_A_BUFF_SIZE (BuffSize - HdrSize) | TX_CMD_A_COMPLETION_INT | TX_CMD_A_DATA_START_OFFSET (2); // 2 bytes beginning offset

    // Write the commands
    MmioWrite32 (LAN9118_TX_DATA, CommandA);
    MmioWrite32 (LAN9118_TX_DATA, CommandB);

    // Write the payload
    for (Count = 0; Count < ((BuffSize + 3) >> 2) - 3; Count++) {
      MmioWrite32 (LAN9118_TX_DATA, LocalData[Count + 3]);
    }
  } else {
    // Format pointer
    LocalData = (UINT32*) Data;

    // Create a buffer to pass to controller
    CommandA = TX_CMD_A_FIRST_SEGMENT | TX_CMD_A_LAST_SEGMENT | TX_CMD_A_BUFF_SIZE (BuffSize) | TX_CMD_A_COMPLETION_INT;
    CommandB = TX_CMD_B_PACKET_TAG (PacketTag) | TX_CMD_B_PACKET_LENGTH (BuffSize);

    // Write the commands first
    MmioWrite32 (LAN9118_TX_DATA, CommandA);
    MmioWrite32 (LAN9118_TX_DATA, CommandB);

    // Write all the data
    for (Count = 0; Count < ((BuffSize + 3) >> 2); Count++) {
      MmioWrite32 (LAN9118_TX_DATA, LocalData[Count]);
    }
  }

  // Save the address of the submitted packet so we can notify the consumer that
  // it has been sent in GetStatus. When the packet tag appears in the Tx Status
  // Fifo, we will return Buffer in the TxBuff parameter of GetStatus.
  LanDriver->TxRing[PacketTag % LAN9118_TX_RING_NUM_ENTRIES] = Data;

#if defined(EVAL_PERFORMANCE)
  EndClock = GetPerformanceCounter ();
  DEBUG ((EFI_D_ERROR, "Time processing: %d counts @ %d Hz\n", StartClock - EndClock,Perf));
#endif

  LanDriver->Stats.TxGoodFrames += 1;

  return EFI_SUCCESS;
}


/*
 *  UEFI Receive() function
 *
 */
EFI_STATUS
EFIAPI
SnpReceive (
  IN        EFI_SIMPLE_NETWORK_PROTOCOL* Snp,
      OUT   UINTN *HdrSize                OPTIONAL,
  IN  OUT   UINTN *BuffSize,
      OUT   VOID *Data,
      OUT   EFI_MAC_ADDRESS *SrcAddr      OPTIONAL,
      OUT   EFI_MAC_ADDRESS *DstAddr      OPTIONAL,
      OUT   UINT16 *Protocol              OPTIONAL
  )
{
  LAN9118_DRIVER *LanDriver;
  UINT32 RxFifoStatus;
  UINT32 NumPackets;
  UINT32 RxCfgValue;
  UINT32 PLength; // Packet length
  UINT32 ReadLimit;
  UINT32 Count;
  UINT32 Padding;
  UINT32 *RawData;
  EFI_MAC_ADDRESS Dst;
  EFI_MAC_ADDRESS Src;
  UINTN  DroppedFrames;

  LanDriver = INSTANCE_FROM_SNP_THIS (Snp);

#if defined(EVAL_PERFORMANCE)
  UINT64 Perf = GetPerformanceCounterProperties (NULL, NULL);
  UINT64 StartClock = GetPerformanceCounter ();
#endif

  // Check preliminaries
  if ((Snp == NULL) || (Data == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if (Snp->Mode->State != EfiSimpleNetworkInitialized) {
    return EFI_NOT_STARTED;
  }

  // Count dropped frames
  DroppedFrames = MmioRead32 (LAN9118_RX_DROP);
  LanDriver->Stats.RxDroppedFrames += DroppedFrames;

  NumPackets = RxStatusUsedSpace (0, Snp) / 4;
  if (!NumPackets) {
    return EFI_NOT_READY;
  }

  // Read Rx Status (only if not empty)
  RxFifoStatus = MmioRead32 (LAN9118_RX_STATUS);
  LanDriver->Stats.RxTotalFrames += 1;

  // First check for errors
  if ((RxFifoStatus & RXSTATUS_MII_ERROR) ||
      (RxFifoStatus & RXSTATUS_RXW_TO) ||
      (RxFifoStatus & RXSTATUS_FTL) ||
      (RxFifoStatus & RXSTATUS_LCOLL) ||
      (RxFifoStatus & RXSTATUS_LE) ||
      (RxFifoStatus & RXSTATUS_DB))
  {
    DEBUG ((EFI_D_WARN, "Warning: There was an error on frame reception.\n"));
    return EFI_DEVICE_ERROR;
  }

  // Check if we got a CRC error
  if (RxFifoStatus & RXSTATUS_CRC_ERROR) {
    DEBUG ((EFI_D_WARN, "Warning: Crc Error\n"));
    LanDriver->Stats.RxCrcErrorFrames += 1;
    LanDriver->Stats.RxDroppedFrames += 1;
    return EFI_DEVICE_ERROR;
  }

  // Check if we got a runt frame
  if (RxFifoStatus & RXSTATUS_RUNT) {
    DEBUG ((EFI_D_WARN, "Warning: Runt Frame\n"));
    LanDriver->Stats.RxUndersizeFrames += 1;
    LanDriver->Stats.RxDroppedFrames += 1;
    return EFI_DEVICE_ERROR;
  }

  // Check filtering status for this packet
  if (RxFifoStatus & RXSTATUS_FILT_FAIL) {
    DEBUG ((EFI_D_WARN, "Warning: Frame Failed Filtering\n"));
    // fast forward?
  }

  // Check if we got a broadcast frame
  if (RxFifoStatus & RXSTATUS_BCF) {
    LanDriver->Stats.RxBroadcastFrames += 1;
  }

  // Check if we got a multicast frame
  if (RxFifoStatus & RXSTATUS_MCF) {
    LanDriver->Stats.RxMulticastFrames += 1;
  }

  // Check if we got a unicast frame
  if ((RxFifoStatus & RXSTATUS_BCF) && ((RxFifoStatus & RXSTATUS_MCF) == 0)) {
    LanDriver->Stats.RxUnicastFrames += 1;
  }

  // Get the received packet length
  PLength = GET_RXSTATUS_PACKET_LENGTH(RxFifoStatus);
  LanDriver->Stats.RxTotalBytes += (PLength - 4);

  // Check buffer size
  if (*BuffSize < PLength) {
    *BuffSize = PLength;
    return EFI_BUFFER_TOO_SMALL;
  }

  // If padding is applied, read more DWORDs
  if (PLength % 4) {
    Padding = 4 - (PLength % 4);
    ReadLimit = (PLength + Padding)/4;
  } else {
    ReadLimit = PLength/4;
    Padding = 0;
  }

  // Set the amount of data to be transfered out of FIFO for THIS packet
  // This can be used to trigger an interrupt, and status can be checked
  RxCfgValue = MmioRead32 (LAN9118_RX_CFG);
  RxCfgValue &= ~(RXCFG_RX_DMA_CNT_MASK);
  RxCfgValue |= RXCFG_RX_DMA_CNT (ReadLimit);

  // Set end alignment to 4-bytes
  RxCfgValue &= ~(RXCFG_RX_END_ALIGN_MASK);
  MmioWrite32 (LAN9118_RX_CFG, RxCfgValue);

  // Update buffer size
  *BuffSize = PLength; // -4 bytes may be needed: Received in buffer as
                       // 4 bytes longer than packet actually is, unless
                       // packet is < 64 bytes

  if (HdrSize != NULL)
    *HdrSize = Snp->Mode->MediaHeaderSize;

  // Format the pointer
  RawData = (UINT32*)Data;

  // Read Rx Packet
  for (Count = 0; Count < ReadLimit; Count++) {
    RawData[Count] = MmioRead32 (LAN9118_RX_DATA);
  }

  // Check for Rx errors (worst possible error)
  if (MmioRead32 (LAN9118_INT_STS) & INSTS_RXE) {
    DEBUG ((EFI_D_WARN, "Warning: Receiver Error. Restarting...\n"));

    // Initiate a software reset
    if (SoftReset (0, Snp) < 0) {
      DEBUG ((EFI_D_ERROR, "Error: Soft Reset Failed: Hardware Error.\n"));
      return EFI_DEVICE_ERROR;
    }

    // Acknowledge the RXE
    MmioWrite32 (LAN9118_INT_STS, INSTS_RXE);
    gBS->Stall (LAN9118_STALL);

    // Restart the rx (and do not clear FIFO)
    StartRx (0, Snp);

    // Say that command could not be sent
    return EFI_DEVICE_ERROR;
  }

  // Get the destination address
  if (DstAddr != NULL) {
    Dst.Addr[0] = (RawData[0] & 0xFF);
    Dst.Addr[1] = (RawData[0] & 0xFF00) >> 8;
    Dst.Addr[2] = (RawData[0] & 0xFF0000) >> 16;
    Dst.Addr[3] = (RawData[0] & 0xFF000000) >> 24;
    Dst.Addr[4] = (RawData[1] & 0xFF);
    Dst.Addr[5] = (RawData[1] & 0xFF00) >> 8;
    CopyMem (DstAddr, &Dst, NET_ETHER_ADDR_LEN);
  }

  // Get the source address
  if (SrcAddr != NULL) {
    Src.Addr[0] = (RawData[1] & 0xFF0000) >> 16;
    Src.Addr[1] = (RawData[1] & 0xFF000000) >> 24;
    Src.Addr[2] = (RawData[2] & 0xFF);
    Src.Addr[3] = (RawData[2] & 0xFF00) >> 8;
    Src.Addr[4] = (RawData[2] & 0xFF0000) >> 16;
    Src.Addr[5] = (RawData[2] & 0xFF000000) >> 24;
    CopyMem (SrcAddr,&Src, NET_ETHER_ADDR_LEN);
  }

  // Get the protocol
  if (Protocol != NULL) {
    *Protocol = NTOHS (RawData[3] & 0xFFFF);
  }

#if defined(EVAL_PERFORMANCE)
  UINT64 EndClock = GetPerformanceCounter ();
  DEBUG ((EFI_D_ERROR, "Receive Time processing: %d counts @ %d Hz\n", StartClock - EndClock,Perf));
#endif

  LanDriver->Stats.RxGoodFrames += 1;

  return EFI_SUCCESS;
}
