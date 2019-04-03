/** @file
*
*  Copyright (c) 2012-2014, ARM Limited. All rights reserved.
*
*  SPDX-License-Identifier: BSD-2-Clause-Patent
*
**/

#ifndef __LAN9118_DXE_UTIL_H__
#define __LAN9118_DXE_UTIL_H__

// Most common CRC32 Polynomial for little endian machines
#define CRC_POLYNOMIAL               0xEDB88320

/**
  This internal function reverses bits for 32bit data.

  @param  Value                 The data to be reversed.

  @return                       Data reversed.

**/
UINT32
ReverseBits (
  UINT32  Value
  );

// Create an Ethernet CRC
UINT32
GenEtherCrc32 (
  IN    EFI_MAC_ADDRESS *Mac,
  IN    UINT32 AddrLen
  );

UINT32
Lan9118RawMmioRead32(
  UINTN Address,
  UINTN Delay
  );
#define Lan9118MmioRead32(a) \
	Lan9118RawMmioRead32(a, a ## _RD_DELAY)

UINT32
Lan9118RawMmioWrite32(
  UINTN Address,
  UINT32 Value,
  UINTN Delay
  );
#define Lan9118MmioWrite32(a, v) \
	Lan9118RawMmioWrite32(a, v, a ## _WR_DELAY)

/* ------------------ MAC CSR Access ------------------- */

// Read from MAC indirect registers
UINT32
IndirectMACRead32 (
  UINT32 Index
  );


// Write to indirect registers
UINT32
IndirectMACWrite32 (
  UINT32 Index,
  UINT32 Value
  );


/* --------------- PHY Registers Access ---------------- */

// Read from MII register (PHY Access)
UINT32
IndirectPHYRead32(
  UINT32 Index
  );


// Write to the MII register (PHY Access)
UINT32
IndirectPHYWrite32(
  UINT32 Index,
  UINT32 Value
  );

/* ---------------- EEPROM Operations ------------------ */

// Read from EEPROM memory
UINT32
IndirectEEPROMRead32 (
  UINT32 Index
  );

// Write to EEPROM memory
UINT32
IndirectEEPROMWrite32 (
  UINT32 Index,
  UINT32 Value
  );

/* ---------------- General Operations ----------------- */

VOID
Lan9118SetMacAddress (
  EFI_MAC_ADDRESS             *Mac,
  EFI_SIMPLE_NETWORK_PROTOCOL *Snp
  );

// Initialise the LAN9118
EFI_STATUS
Lan9118Initialize (
  IN  EFI_SIMPLE_NETWORK_PROTOCOL *Snp
  );

// Flags for software reset
#define SOFT_RESET_CHECK_MAC_ADDR_LOAD                  BIT0
#define SOFT_RESET_CLEAR_INT                            BIT1
#define SOFT_RESET_SELF_TEST                            BIT2

// Perform software reset on the LAN9118
EFI_STATUS
SoftReset (
  UINT32 Flags,
  EFI_SIMPLE_NETWORK_PROTOCOL *Snp
  );

// Flags for PHY reset
#define PHY_RESET_PMT                                   BIT0
#define PHY_RESET_BCR                                   BIT1
#define PHY_SOFT_RESET_CLEAR_INT                        BIT2

// Perform PHY software reset
EFI_STATUS
PhySoftReset (
  UINT32 Flags,
  EFI_SIMPLE_NETWORK_PROTOCOL *Snp
  );

// Flags for Hardware configuration
#define HW_CONF_USE_LEDS                                BIT0

// Configure hardware for LAN9118
EFI_STATUS
ConfigureHardware (
  UINT32 Flags,
  EFI_SIMPLE_NETWORK_PROTOCOL *Snp
  );

// Configure flow control
EFI_STATUS
ConfigureFlow (
  UINT32 Flags,
  UINT32 HighTrig,
  UINT32 LowTrig,
  UINT32 BPDuration,
  EFI_SIMPLE_NETWORK_PROTOCOL *Snp
  );

// Flags for auto negotiation
#define AUTO_NEGOTIATE_COLLISION_TEST         BIT0
#define AUTO_NEGOTIATE_ADVERTISE_ALL          BIT1

// Do auto-negotiation
EFI_STATUS
AutoNegotiate (
  UINT32 Flags,
  EFI_SIMPLE_NETWORK_PROTOCOL *Snp
  );

// Check the Link Status and take appropriate action
EFI_STATUS
CheckLinkStatus (
  UINT32 Flags,
  EFI_SIMPLE_NETWORK_PROTOCOL *Snp
  );

// Stop transmitter flags
#define STOP_TX_MAC                       BIT0
#define STOP_TX_CFG                       BIT1
#define STOP_TX_CLEAR                     BIT2

// Stop the transmitter
EFI_STATUS
StopTx (
  UINT32 Flags,
  EFI_SIMPLE_NETWORK_PROTOCOL *Snp
  );

// Stop receiver flags
#define STOP_RX_CLEAR                     BIT0

// Stop the receiver
EFI_STATUS
StopRx (
  UINT32 Flags,
  EFI_SIMPLE_NETWORK_PROTOCOL *Snp
  );

// Start transmitter flags
#define START_TX_MAC                      BIT0
#define START_TX_CFG                      BIT1
#define START_TX_CLEAR                    BIT2

// Start the transmitter
EFI_STATUS
StartTx (
  UINT32 Flags,
  EFI_SIMPLE_NETWORK_PROTOCOL *Snp
  );

// Stop receiver flags
#define START_RX_CLEAR                     BIT0

// Start the receiver
EFI_STATUS
StartRx (
  UINT32 Flags,
  EFI_SIMPLE_NETWORK_PROTOCOL *Snp
  );

// Check Tx Data available space
UINT32
TxDataFreeSpace (
  UINT32 Flags,
  EFI_SIMPLE_NETWORK_PROTOCOL *Snp
  );

// Check Tx Status used space
UINT32
TxStatusUsedSpace (
  UINT32 Flags,
  EFI_SIMPLE_NETWORK_PROTOCOL *Snp
  );

// Check Rx Data used space
UINT32
RxDataUsedSpace (
  UINT32 Flags,
  EFI_SIMPLE_NETWORK_PROTOCOL *Snp
  );

// Check Rx Status used space
UINT32
RxStatusUsedSpace (
  UINT32 Flags,
  EFI_SIMPLE_NETWORK_PROTOCOL *Snp
  );


// Flags for FIFO allocation
#define ALLOC_USE_DEFAULT                 BIT0
#define ALLOC_USE_FIFOS                   BIT1
#define ALLOC_USE_DMA                     BIT2

// FIFO min and max sizes
#define TX_FIFO_MIN_SIZE            0x00000600
#define TX_FIFO_MAX_SIZE            0x00003600
//#define RX_FIFO_MIN_SIZE
//#define RX_FIFO_MAX_SIZE

// Change the allocation of FIFOs
EFI_STATUS
ChangeFifoAllocation (
  IN      UINT32 Flags,
  IN  OUT UINTN  *TxDataSize    OPTIONAL,
  IN  OUT UINTN  *RxDataSize    OPTIONAL,
  IN  OUT UINT32 *TxStatusSize  OPTIONAL,
  IN  OUT UINT32 *RxStatusSize  OPTIONAL,
  IN  OUT EFI_SIMPLE_NETWORK_PROTOCOL *Snp
  );

VOID
Lan9118ReadMacAddress (
  OUT EFI_MAC_ADDRESS *Mac
  );

#endif // __LAN9118_DXE_UTIL_H__
