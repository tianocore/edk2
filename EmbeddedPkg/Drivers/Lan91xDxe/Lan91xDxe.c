/** @file
*  SMSC LAN91x series Network Controller Driver.
*
*  Copyright (c) 2013-2017 Linaro.org
*
*  Derived from the LAN9118 driver. Original sources
*  Copyright (c) 2012-2013, ARM Limited. All rights reserved.
*
*  SPDX-License-Identifier: BSD-2-Clause-Patent
*
**/

#include <Uefi.h>
#include <Uefi/UefiSpec.h>
#include <Base.h>

// Protocols used by this driver
#include <Protocol/SimpleNetwork.h>
#include <Protocol/ComponentName2.h>
#include <Protocol/PxeBaseCode.h>
#include <Protocol/DevicePath.h>

// Libraries used by this driver
#include <Library/UefiLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/IoLib.h>
#include <Library/PcdLib.h>
#include <Library/NetLib.h>
#include <Library/DevicePathLib.h>

// Hardware register definitions
#include "Lan91xDxeHw.h"

// Debugging output options
//#define LAN91X_PRINT_REGISTERS 1
//#define LAN91X_PRINT_PACKET_HEADERS 1
//#define LAN91X_PRINT_RECEIVE_FILTERS 1

// Chip power-down option -- UNTESTED
//#define LAN91X_POWER_DOWN 1

/*---------------------------------------------------------------------------------------------------------------------

  LAN91x Information Structure

---------------------------------------------------------------------------------------------------------------------*/
typedef struct _LAN91X_DRIVER {
  // Driver signature
  UINT32            Signature;
  EFI_HANDLE        ControllerHandle;

  // EFI SNP protocol instances
  EFI_SIMPLE_NETWORK_PROTOCOL Snp;
  EFI_SIMPLE_NETWORK_MODE SnpMode;

  // EFI Snp statistics instance
  EFI_NETWORK_STATISTICS Stats;

  // Transmit Buffer recycle queue

  LIST_ENTRY TransmitQueueHead;

  // Register access variables
  UINTN             IoBase;             // I/O Base Address
  UINT8             Revision;           // Chip Revision Number
  INT8              PhyAd;              // Phy Address
  UINT8             BankSel;            // Currently selected register bank

} LAN91X_DRIVER;

#define LAN91X_NO_PHY (-1)              // PhyAd value if PHY not detected

#define LAN91X_SIGNATURE                        SIGNATURE_32('S', 'M', '9', '1')
#define INSTANCE_FROM_SNP_THIS(a)               CR(a, LAN91X_DRIVER, Snp, LAN91X_SIGNATURE)

#define LAN91X_STALL              2
#define LAN91X_MEMORY_ALLOC_POLLS 100   // Max times to poll for memory allocation
#define LAN91X_PKT_OVERHEAD       6     // Overhead bytes in packet buffer

// Synchronization TPLs
#define LAN91X_TPL  TPL_CALLBACK

// Most common CRC32 Polynomial for little endian machines
#define CRC_POLYNOMIAL               0xEDB88320


typedef struct {
  MAC_ADDR_DEVICE_PATH      Lan91x;
  EFI_DEVICE_PATH_PROTOCOL  End;
} LAN91X_DEVICE_PATH;

LAN91X_DEVICE_PATH Lan91xPathTemplate =  {
  {
    {
      MESSAGING_DEVICE_PATH, MSG_MAC_ADDR_DP,
      { (UINT8) (sizeof(MAC_ADDR_DEVICE_PATH)), (UINT8) ((sizeof(MAC_ADDR_DEVICE_PATH)) >> 8) }
    },
    { { 0 } },
    0
  },
  {
    END_DEVICE_PATH_TYPE,
    END_ENTIRE_DEVICE_PATH_SUBTYPE,
    { sizeof(EFI_DEVICE_PATH_PROTOCOL), 0 }
  }
};

// Chip ID numbers and name strings
#define CHIP_9192       3
#define CHIP_9194       4
#define CHIP_9195       5
#define CHIP_9196       6
#define CHIP_91100      7
#define CHIP_91100FD    8
#define CHIP_91111FD    9

STATIC CHAR16 CONST * CONST ChipIds[ 16 ] =  {
  NULL, NULL, NULL,
  /* 3 */ L"SMC91C90/91C92",
  /* 4 */ L"SMC91C94",
  /* 5 */ L"SMC91C95",
  /* 6 */ L"SMC91C96",
  /* 7 */ L"SMC91C100",
  /* 8 */ L"SMC91C100FD",
  /* 9 */ L"SMC91C11xFD",
  NULL, NULL, NULL,
  NULL, NULL, NULL
};

/* ------------------ TxBuffer Queue structures ------------------- */

typedef struct {
  VOID            *Buf;
  UINTN           Length;
} MSK_SYSTEM_BUF;

typedef struct {
  UINTN           Signature;
  LIST_ENTRY      Link;
  MSK_SYSTEM_BUF  SystemBuf;
} MSK_LINKED_SYSTEM_BUF;

#define TX_MBUF_SIGNATURE  SIGNATURE_32 ('t','x','m','b')

/* ------------------ MAC Address Hash Calculations ------------------- */

/*
**  Generate a hash value from a multicast address
**
**  This uses the Ethernet standard CRC32 algorithm
**
**  INFO USED:
**    1: http://en.wikipedia.org/wiki/Cyclic_redundancy_check
**
**    2: http://www.erg.abdn.ac.uk/~gorry/eg3567/dl-pages/crc.html
**
**    3: http://en.wikipedia.org/wiki/Computation_of_CRC
*/
STATIC
UINT32
MulticastHash (
  IN    EFI_MAC_ADDRESS *Mac,
  IN    UINT32 AddrLen
  )
{
  UINT32 Iter;
  UINT32 Remainder;
  UINT32 Crc32;
  UINT8 *Addr;

  // 0xFFFFFFFF is standard seed for Ethernet
  Remainder = 0xFFFFFFFF;

  // Generate the remainder byte-by-byte (LSB first)
  Addr = &Mac->Addr[0];
  while (AddrLen-- > 0) {
    Remainder ^= *Addr++;
    for (Iter = 0; Iter < 8; ++Iter) {
      // Check if exponent is set
      if ((Remainder & 1) != 0) {
        Remainder = (Remainder >> 1) ^ CRC_POLYNOMIAL;
      } else {
        Remainder = (Remainder >> 1) ^ 0;
      }
    }
  }

  // Reverse the bits of the remainder
  Crc32 = 0;
  for (Iter = 0; Iter < 32; ++Iter) {
    Crc32 <<= 1;
    Crc32 |= Remainder & 1;
    Remainder >>= 1;
  }
  return Crc32;
}


/* ---------------- Banked Register Operations ------------------ */

// Select the proper I/O bank
STATIC
VOID
SelectIoBank (
  LAN91X_DRIVER   *LanDriver,
  UINTN            Register
  )
{
  UINT8   Bank;

  Bank = RegisterToBank (Register);

  // Select the proper I/O bank
  if (LanDriver->BankSel != Bank) {
    MmioWrite16 (LanDriver->IoBase + LAN91X_BANK_OFFSET, Bank);
    LanDriver->BankSel = Bank;
  }
}

// Read a 16-bit I/O-space register
STATIC
UINT16
ReadIoReg16 (
  LAN91X_DRIVER   *LanDriver,
  UINTN            Register
  )
{
  UINT8   Offset;

  // Select the proper I/O bank
  SelectIoBank (LanDriver, Register);

  // Read the requested register
  Offset = RegisterToOffset (Register);
  return MmioRead16 (LanDriver->IoBase + Offset);
}

// Write a 16-bit I/O-space register
STATIC
UINT16
WriteIoReg16 (
  LAN91X_DRIVER   *LanDriver,
  UINTN            Register,
  UINT16           Value
  )
{
  UINT8   Offset;

  // Select the proper I/O bank
  SelectIoBank (LanDriver, Register);

  // Write the requested register
  Offset = RegisterToOffset (Register);
  return MmioWrite16 (LanDriver->IoBase + Offset, Value);
}

// Read an 8-bit I/O-space register
STATIC
UINT8
ReadIoReg8 (
  LAN91X_DRIVER   *LanDriver,
  UINTN            Register
  )
{
  UINT8   Offset;

  // Select the proper I/O bank
  SelectIoBank (LanDriver, Register);

  // Read the requested register
  Offset = RegisterToOffset (Register);
  return MmioRead8 (LanDriver->IoBase + Offset);
}

// Write an 8-bit I/O-space register
STATIC
UINT8
WriteIoReg8 (
  LAN91X_DRIVER   *LanDriver,
  UINTN            Register,
  UINT8            Value
  )
{
  UINT8   Offset;

  // Select the proper I/O bank
  SelectIoBank (LanDriver, Register);

  // Write the requested register
  Offset = RegisterToOffset (Register);
  return MmioWrite8 (LanDriver->IoBase + Offset, Value);
}


/* ---------------- MII/PHY Access Operations ------------------ */

#define LAN91X_MDIO_STALL   1

STATIC
VOID
MdioOutput (
  LAN91X_DRIVER   *LanDriver,
  UINTN            Bits,
  UINT32           Value
  )
{
  UINT16          MgmtReg;
  UINT32          Mask;

  MgmtReg = ReadIoReg16 (LanDriver, LAN91X_MGMT);
  MgmtReg &= ~MGMT_MCLK;
  MgmtReg |= MGMT_MDOE;

  for (Mask = (1 << (Bits - 1)); Mask != 0; Mask >>= 1) {
    if ((Value & Mask) != 0) {
      MgmtReg |= MGMT_MDO;
    } else {
      MgmtReg &= ~MGMT_MDO;
    }

    WriteIoReg16 (LanDriver, LAN91X_MGMT, MgmtReg);
    gBS->Stall (LAN91X_MDIO_STALL);
    WriteIoReg16 (LanDriver, LAN91X_MGMT, MgmtReg | MGMT_MCLK);
    gBS->Stall (LAN91X_MDIO_STALL);
  }
}
#define PHY_OUTPUT_TIME (2 * LAN91X_MDIO_STALL)

STATIC
UINT32
MdioInput (
  LAN91X_DRIVER   *LanDriver,
  UINTN            Bits
  )
{
  UINT16          MgmtReg;
  UINT32          Mask;
  UINT32          Value;

  MgmtReg = ReadIoReg16 (LanDriver, LAN91X_MGMT);
  MgmtReg &= ~(MGMT_MDOE | MGMT_MCLK | MGMT_MDO);
  WriteIoReg16 (LanDriver, LAN91X_MGMT, MgmtReg);

  Value = 0;
  for (Mask = (1 << (Bits - 1)); Mask != 0; Mask >>= 1) {
    if ((ReadIoReg16 (LanDriver, LAN91X_MGMT) & MGMT_MDI) != 0) {
       Value |= Mask;
    }

    WriteIoReg16 (LanDriver, LAN91X_MGMT, MgmtReg);
    gBS->Stall (LAN91X_MDIO_STALL);
    WriteIoReg16 (LanDriver, LAN91X_MGMT, MgmtReg | MGMT_MCLK);
    gBS->Stall (LAN91X_MDIO_STALL);
  }

  return Value;
}
#define PHY_INPUT_TIME (2 * LAN91X_MDIO_STALL)

STATIC
VOID
MdioIdle (
  LAN91X_DRIVER   *LanDriver
  )
{
  UINT16          MgmtReg;

  MgmtReg = ReadIoReg16 (LanDriver, LAN91X_MGMT);
  MgmtReg &= ~(MGMT_MDOE | MGMT_MCLK | MGMT_MDO);
  WriteIoReg16 (LanDriver, LAN91X_MGMT, MgmtReg);
}

// Write to a PHY register
STATIC
VOID
WritePhyReg16 (
  LAN91X_DRIVER   *LanDriver,
  UINTN            RegAd,
  UINT16           Value
  )
{
  // Bit-bang the MII Serial Frame write operation
  MdioOutput (LanDriver, 32, 0xffffffff);       // Send 32 Ones as a preamble
  MdioOutput (LanDriver,  2, 0x01);             // Send Start (01)
  MdioOutput (LanDriver,  2, 0x01);             // Send Write (01)
  MdioOutput (LanDriver,  5, LanDriver->PhyAd); // Send PHYAD[4:0]
  MdioOutput (LanDriver,  5, RegAd);            // Send REGAD[4:0]
  MdioOutput (LanDriver,  2, 0x02);             // Send TurnAround (10)
  MdioOutput (LanDriver, 16, Value);            // Write 16 data bits

  // Idle the MDIO bus
  MdioIdle (LanDriver);
}
// Calculate approximate time to write a PHY register in microseconds
#define PHY_WRITE_TIME  ((32 + 2 + 2 + 5 + 5 + 2 + 16) * PHY_OUTPUT_TIME)

// Read from a PHY register
STATIC
UINT16
ReadPhyReg16 (
  LAN91X_DRIVER   *LanDriver,
  UINTN            RegAd
  )
{
  UINT32 Value;

  // Bit-bang the MII Serial Frame read operation
  MdioOutput (LanDriver, 32, 0xffffffff);       // Send 32 Ones as a preamble
  MdioOutput (LanDriver,  2, 0x01);             // Send Start (01)
  MdioOutput (LanDriver,  2, 0x02);             // Send Read (10)
  MdioOutput (LanDriver,  5, LanDriver->PhyAd); // Send PHYAD[4:0]
  MdioOutput (LanDriver,  5, RegAd);            // Send REGAD[4:0]

  (VOID)  MdioInput (LanDriver, 2);             // Discard TurnAround bits
  Value = MdioInput (LanDriver, 16);            // Read 16 data bits

  // Idle the MDIO bus
  MdioIdle (LanDriver);

  return (Value & 0xffff);
}
// Calculate approximate time to read a PHY register in microseconds
#define PHY_READ_TIME  (((32 + 2 + 2 + 5 + 5) * PHY_OUTPUT_TIME) + \
                        ((2 + 16) * PHY_INPUT_TIME))


/* ---------------- Debug Functions ------------------ */

#ifdef LAN91X_PRINT_REGISTERS
STATIC
VOID
PrintIoRegisters (
  IN  LAN91X_DRIVER   *LanDriver
  )
{
  UINTN   Bank;
  UINTN   Offset;
  UINT16  Value;

  DEBUG ((DEBUG_ERROR, "\nLAN91x I/O Register Dump:\n"));

  // Print currrent bank select register
  Value = MmioRead16 (LanDriver->IoBase + LAN91X_BANK_OFFSET);
  DEBUG ((DEBUG_ERROR, "  BankSel: %d  Bank Register %04x (%d)\n",
      LanDriver->BankSel, Value, Value & 0x0007));

  // Print all I/O registers
  for (Offset = 0; Offset < 0x0e; Offset += 2) {
    DEBUG ((DEBUG_ERROR, "  %02x:", Offset));
    for (Bank = 0; Bank <= 3; ++Bank) {
      DEBUG ((DEBUG_ERROR, "  %04x", ReadIoReg16 (LanDriver, MakeRegister (Bank, Offset))));
    }
    DEBUG ((DEBUG_ERROR, "\n"));
  }
}

STATIC
VOID
PrintPhyRegisters (
  IN  LAN91X_DRIVER   *LanDriver
  )
{
  UINTN   RegNum;

  DEBUG ((DEBUG_ERROR, "\nLAN91x Phy %d Register Dump:\n", LanDriver->PhyAd));

  // Print all Phy registers
  for (RegNum = 0; RegNum <= 5; ++RegNum) {
    DEBUG ((DEBUG_ERROR, "  %2d:  %04x\n",
           RegNum,
           ReadPhyReg16 (LanDriver, RegNum)
    ));
  }
  for (RegNum = 16; RegNum <= 20; ++RegNum) {
    DEBUG ((DEBUG_ERROR, "  %2d:  %04x\n",
           RegNum,
           ReadPhyReg16 (LanDriver, RegNum)
    ));
  }
}
#endif

#if LAN91X_PRINT_PACKET_HEADERS
STATIC
VOID
PrintIpDgram (
  IN  CONST VOID  *DstMac,
  IN  CONST VOID  *SrcMac,
  IN  CONST VOID  *Proto,
  IN  CONST VOID  *IpDgram
  )
{
  CONST UINT8   *Ptr;
  UINT16         SrcPort;
  UINT16         DstPort;

  Ptr = DstMac;
  DEBUG ((DEBUG_ERROR, "  Dst: %02x-%02x-%02x",
         Ptr[0], Ptr[1], Ptr[2]));
  DEBUG ((DEBUG_ERROR, "-%02x-%02x-%02x",
         Ptr[3], Ptr[4], Ptr[5]));

  Ptr = SrcMac;
  DEBUG ((DEBUG_ERROR, "  Src: %02x-%02x-%02x",
         Ptr[0], Ptr[1], Ptr[2]));
  DEBUG ((DEBUG_ERROR, "-%02x-%02x-%02x",
         Ptr[3], Ptr[4], Ptr[5]));

  Ptr = Proto;
  DEBUG ((DEBUG_ERROR, "  Proto: %02x%02x\n",
         Ptr[0], Ptr[1]));

  Ptr = IpDgram;
  switch (Ptr[9]) {
  case EFI_IP_PROTO_ICMP:
    DEBUG ((DEBUG_ERROR, "  ICMP"));
    break;
  case EFI_IP_PROTO_TCP:
    DEBUG ((DEBUG_ERROR, "  TCP"));
    break;
  case EFI_IP_PROTO_UDP:
    DEBUG ((DEBUG_ERROR, "  UDP"));
    break;
  default:
    DEBUG ((DEBUG_ERROR, "  IpProto %d\n", Ptr[9]));
    return;
  }

  DEBUG ((DEBUG_ERROR, "  SrcIp: %d.%d.%d.%d",
         Ptr[12], Ptr[13], Ptr[14], Ptr[15]));
  DEBUG ((DEBUG_ERROR, "  DstIp: %d.%d.%d.%d",
         Ptr[16], Ptr[17], Ptr[18], Ptr[19]));

  SrcPort = (Ptr[20] << 8) | Ptr[21];
  DstPort = (Ptr[22] << 8) | Ptr[23];
  DEBUG ((DEBUG_ERROR, "  SrcPort: %d  DstPort: %d\n", SrcPort, DstPort));
}
#endif


/* ---------------- PHY Management Operations ----------------- */

STATIC
EFI_STATUS
PhyDetect (
  IN  LAN91X_DRIVER *LanDriver
  )
{
  UINT16  PhyId1;
  UINT16  PhyId2;

  for (LanDriver->PhyAd = 0x1f; LanDriver->PhyAd >= 0 ; --LanDriver->PhyAd) {
    PhyId1 = ReadPhyReg16 (LanDriver, PHY_INDEX_ID1);
    PhyId2 = ReadPhyReg16 (LanDriver, PHY_INDEX_ID2);

    if ((PhyId1 != 0x0000) && (PhyId1 != 0xffff) &&
        (PhyId2 != 0x0000) && (PhyId2 != 0xffff)) {
      if ((PhyId1 == 0x0016) && ((PhyId2 & 0xfff0) == 0xf840)) {
        DEBUG ((DEBUG_ERROR, "LAN91x: PHY type LAN83C183 (LAN91C111 Internal)\n"));
      } else if ((PhyId1 == 0x0282) && ((PhyId2 & 0xfff0) == 0x1c50)) {
        DEBUG ((DEBUG_ERROR, "LAN91x: PHY type LAN83C180\n"));
      } else {
        DEBUG ((DEBUG_ERROR, "LAN91x: PHY id %04x:%04x\n", PhyId1, PhyId2));
      }
      return EFI_SUCCESS;
    }
  }

  DEBUG ((DEBUG_ERROR, "LAN91x: PHY detection failed\n"));
  return EFI_NO_MEDIA;
}


// Check the Link Status and take appropriate action
STATIC
BOOLEAN
CheckLinkStatus (
  IN  LAN91X_DRIVER *LanDriver
  )
{
  UINT16  PhyStatus;

  // Get the PHY Status
  PhyStatus = ReadPhyReg16 (LanDriver, PHY_INDEX_BASIC_STATUS);

  return (PhyStatus & PHYSTS_LINK_STS) != 0;
}


// Do auto-negotiation
STATIC
EFI_STATUS
PhyAutoNegotiate (
  IN  LAN91X_DRIVER *LanDriver
  )
{
  UINTN  Retries;
  UINT16 PhyControl;
  UINT16 PhyStatus;
  UINT16 PhyAdvert;

  // If there isn't a PHY, don't try to reset it
  if (LanDriver->PhyAd == LAN91X_NO_PHY) {
    return EFI_SUCCESS;
  }

  // Next check that auto-negotiation is supported
  PhyStatus = ReadPhyReg16 (LanDriver, PHY_INDEX_BASIC_STATUS);
  if ((PhyStatus & PHYSTS_AUTO_CAP) == 0) {
    return EFI_SUCCESS;
  }

  // Translate capabilities to advertise
  PhyAdvert = PHYANA_CSMA;

  if ((PhyStatus & PHYSTS_10BASET_HDPLX) != 0) {
    PhyAdvert |= PHYANA_10BASET;
  }
  if ((PhyStatus & PHYSTS_10BASET_FDPLX) != 0) {
    PhyAdvert |= PHYANA_10BASETFD;
  }
  if ((PhyStatus & PHYSTS_100BASETX_HDPLX) != 0) {
    PhyAdvert |= PHYANA_100BASETX;
  }
  if ((PhyStatus & PHYSTS_100BASETX_FDPLX) != 0) {
    PhyAdvert |= PHYANA_100BASETXFD;
  }
  if ((PhyStatus & PHYSTS_100BASE_T4) != 0) {
    PhyAdvert |= PHYANA_100BASET4;
  }

  // Set the capabilities to advertise
  WritePhyReg16 (LanDriver, PHY_INDEX_AUTO_NEG_ADVERT, PhyAdvert);
  (VOID) ReadPhyReg16 (LanDriver, PHY_INDEX_AUTO_NEG_ADVERT);

  // Restart Auto-Negotiation
  PhyControl = ReadPhyReg16 (LanDriver, PHY_INDEX_BASIC_CTRL);
  PhyControl &= ~(PHYCR_SPEED_SEL | PHYCR_DUPLEX_MODE);
  PhyControl |= PHYCR_AUTO_EN | PHYCR_RST_AUTO;
  WritePhyReg16 (LanDriver, PHY_INDEX_BASIC_CTRL, PhyControl);

  // Wait up to 2 seconds for the process to complete
  Retries = 2000000 / (PHY_READ_TIME + 100);
  while ((ReadPhyReg16 (LanDriver, PHY_INDEX_BASIC_STATUS) & PHYSTS_AUTO_COMP) == 0) {
    if (--Retries == 0) {
      DEBUG ((DEBUG_ERROR, "LAN91x: PHY auto-negotiation timed-out\n"));
      return EFI_TIMEOUT;
    }
    gBS->Stall (100);
  }

  return EFI_SUCCESS;
}


// Perform PHY software reset
STATIC
EFI_STATUS
PhySoftReset (
  IN  LAN91X_DRIVER *LanDriver
  )
{
  UINTN     Retries;

  // If there isn't a PHY, don't try to reset it
  if (LanDriver->PhyAd == LAN91X_NO_PHY) {
    return EFI_SUCCESS;
  }

  // Request a PHY reset
  WritePhyReg16 (LanDriver, PHY_INDEX_BASIC_CTRL, PHYCR_RESET);

  // The internal PHY will reset within 50ms. Allow 100ms.
  Retries = 100000 / (PHY_READ_TIME + 100);
  while (ReadPhyReg16 (LanDriver, PHY_INDEX_BASIC_CTRL) & PHYCR_RESET) {
    if (--Retries == 0) {
      DEBUG ((DEBUG_ERROR, "LAN91x: PHY reset timed-out\n"));
      return EFI_TIMEOUT;
    }
    gBS->Stall (100);
  }

  return EFI_SUCCESS;
}


/* ---------------- General Operations ----------------- */

STATIC
EFI_MAC_ADDRESS
GetCurrentMacAddress (
  IN  LAN91X_DRIVER *LanDriver
  )
{
  UINTN            RegNum;
  UINT8           *Addr;
  EFI_MAC_ADDRESS  MacAddress;

  SetMem (&MacAddress, sizeof(MacAddress), 0);

  Addr = &MacAddress.Addr[0];
  for (RegNum = LAN91X_IAR0; RegNum <= LAN91X_IAR5; ++RegNum) {
    *Addr = ReadIoReg8 (LanDriver, RegNum);
    ++Addr;
  }

  return MacAddress;
}

STATIC
EFI_STATUS
SetCurrentMacAddress (
  IN  LAN91X_DRIVER   *LanDriver,
  IN  EFI_MAC_ADDRESS *MacAddress
  )
{
  UINTN            RegNum;
  UINT8           *Addr;

  Addr = &MacAddress->Addr[0];
  for (RegNum = LAN91X_IAR0; RegNum <= LAN91X_IAR5; ++RegNum) {
    WriteIoReg8 (LanDriver, RegNum, *Addr);
    ++Addr;
  }

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
MmuOperation (
  IN  LAN91X_DRIVER *LanDriver,
  IN  UINTN          MmuOp
  )
{
  UINTN   Polls;

  WriteIoReg16 (LanDriver, LAN91X_MMUCR, MmuOp);
  Polls = 100;
  while ((ReadIoReg16 (LanDriver, LAN91X_MMUCR) & MMUCR_BUSY) != 0) {
    if (--Polls == 0) {
      DEBUG ((DEBUG_ERROR, "LAN91x: MMU operation %04x timed-out\n", MmuOp));
      return EFI_TIMEOUT;
    }
    gBS->Stall (LAN91X_STALL);
  }

  return EFI_SUCCESS;
}

// Read bytes from the DATA register
STATIC
EFI_STATUS
ReadIoData (
  IN  LAN91X_DRIVER *LanDriver,
  IN  VOID          *Buffer,
  IN  UINTN          BufLen
  )
{
  UINT8     *Ptr;

  Ptr = Buffer;
  for (; BufLen > 0; --BufLen) {
    *Ptr = ReadIoReg8 (LanDriver, LAN91X_DATA0);
    ++Ptr;
  }

  return EFI_SUCCESS;
}

// Write bytes to the DATA register
STATIC
EFI_STATUS
WriteIoData (
  IN  LAN91X_DRIVER *LanDriver,
  IN  VOID          *Buffer,
  IN  UINTN          BufLen
  )
{
  UINT8     *Ptr;

  Ptr = Buffer;
  for (; BufLen > 0; --BufLen) {
    WriteIoReg8 (LanDriver, LAN91X_DATA0, *Ptr);
    ++Ptr;
  }

  return EFI_SUCCESS;
}

// Disable the interface
STATIC
EFI_STATUS
ChipDisable (
  IN  LAN91X_DRIVER *LanDriver
  )
{
#ifdef LAN91X_POWER_DOWN
  UINT16  Val16;
#endif

  // Stop Rx and Tx operations
  WriteIoReg16 (LanDriver, LAN91X_RCR, RCR_CLEAR);
  WriteIoReg16 (LanDriver, LAN91X_TCR, TCR_CLEAR);

#ifdef LAN91X_POWER_DOWN
  // Power-down the chip
  Val16 = ReadIoReg16 (LanDriver, LAN91X_CR);
  Val16 &= ~CR_EPH_POWER_EN;
  WriteIoReg16 (LanDriver, LAN91X_CR, Val16);
#endif

  return EFI_SUCCESS;
}

// Enable the interface
STATIC
EFI_STATUS
ChipEnable (
  IN  LAN91X_DRIVER *LanDriver
  )
{
#ifdef LAN91X_POWER_DOWN
  UINT16  Val16;

  // Power-up the chip
  Val16 = ReadIoReg16 (LanDriver, LAN91X_CR);
  Val16 |= CR_EPH_POWER_EN;
  WriteIoReg16 (LanDriver, LAN91X_CR, Val16);
  gBS->Stall (LAN91X_STALL);
#endif

  // Start Rx and Tx operations
  WriteIoReg16 (LanDriver, LAN91X_TCR, TCR_DEFAULT);
  WriteIoReg16 (LanDriver, LAN91X_RCR, RCR_DEFAULT);

  return EFI_SUCCESS;
}


// Perform software reset on the LAN91x
STATIC
EFI_STATUS
SoftReset (
  IN  LAN91X_DRIVER   *LanDriver
  )
{
  UINT16  Val16;

  // Issue the reset
  WriteIoReg16 (LanDriver, LAN91X_RCR, RCR_SOFT_RST);
  gBS->Stall (LAN91X_STALL);
  WriteIoReg16 (LanDriver, LAN91X_RCR, RCR_CLEAR);

  // Set the configuration register
  WriteIoReg16 (LanDriver, LAN91X_CR, CR_DEFAULT);
  gBS->Stall (LAN91X_STALL);

  // Stop Rx and Tx
  WriteIoReg16 (LanDriver, LAN91X_RCR, RCR_CLEAR);
  WriteIoReg16 (LanDriver, LAN91X_TCR, TCR_CLEAR);

  // Initialize the Control Register
  Val16 = ReadIoReg16 (LanDriver, LAN91X_CTR);
  Val16 |= CTR_AUTO_REL;
  WriteIoReg16 (LanDriver, LAN91X_CTR, Val16);

  // Reset the MMU
  MmuOperation (LanDriver, MMUCR_OP_RESET_MMU);

  return EFI_SUCCESS;
}

/*
**  Probe()
**
**  Validate that there is a LAN91x device.
**
*/
STATIC
EFI_STATUS
Probe (
  IN  LAN91X_DRIVER   *LanDriver
  )
{
  UINT16        Bank;
  UINT16        Val16;
  CHAR16 CONST *ChipId;
  UINTN         ResetTime;

  // First check that the Bank Select register is valid
  Bank = MmioRead16 (LanDriver->IoBase + LAN91X_BANK_OFFSET);
  if ((Bank & 0xff00) != 0x3300) {
    DEBUG ((DEBUG_ERROR, "LAN91x: signature error: expecting 33xx, read %04x\n", Bank));
    return EFI_DEVICE_ERROR;
  }

  // Try reading the revision register next
  LanDriver->BankSel = 0xff;
  Val16 = ReadIoReg16 (LanDriver, LAN91X_REV);

  Bank = MmioRead16 (LanDriver->IoBase + LAN91X_BANK_OFFSET);
  if ((Bank & 0xff03) != 0x3303) {
    DEBUG ((DEBUG_ERROR, "LAN91x: signature error: expecting 33x3, read %04x\n", Bank));
    return EFI_DEVICE_ERROR;
  }

  // Validate the revision register
  if ((Val16 & 0xff00) != 0x3300) {
    DEBUG ((DEBUG_ERROR, "LAN91x: revision error: expecting 33xx, read %04x\n", Val16));
    return EFI_DEVICE_ERROR;
  }

  ChipId = ChipIds[(Val16 >> 4) & 0x0f];
  if (ChipId == NULL) {
    DEBUG ((DEBUG_ERROR, "LAN91x: unrecognized revision: %04x\n", Val16));
    return EFI_DEVICE_ERROR;
  }
  DEBUG ((DEBUG_ERROR, "LAN91x: detected chip %s rev %d\n", ChipId, Val16 & 0xf));
  LanDriver->Revision = Val16 & 0xff;

  // Reload from EEPROM to get the hardware MAC address
  WriteIoReg16 (LanDriver, LAN91X_CTR, CTR_RESERVED | CTR_RELOAD);
  ResetTime = 1000;
  while ((ReadIoReg16 (LanDriver, LAN91X_CTR) & CTR_RELOAD) != 0) {
    if (--ResetTime == 0) {
      DEBUG ((DEBUG_ERROR, "LAN91x: reload from EEPROM timed-out\n"));
      WriteIoReg16 (LanDriver, LAN91X_CTR, CTR_RESERVED);
      return EFI_DEVICE_ERROR;
    }
    gBS->Stall (LAN91X_STALL);
  }

  // Read and save the Permanent MAC Address
  LanDriver->SnpMode.PermanentAddress = GetCurrentMacAddress (LanDriver);
  LanDriver->SnpMode.CurrentAddress = LanDriver->SnpMode.PermanentAddress;
  DEBUG ((DEBUG_ERROR, //DEBUG_NET | DEBUG_INFO,
         "LAN91x: HW MAC Address: %02x-%02x-%02x-%02x-%02x-%02x\n",
         LanDriver->SnpMode.PermanentAddress.Addr[0],
         LanDriver->SnpMode.PermanentAddress.Addr[1],
         LanDriver->SnpMode.PermanentAddress.Addr[2],
         LanDriver->SnpMode.PermanentAddress.Addr[3],
         LanDriver->SnpMode.PermanentAddress.Addr[4],
         LanDriver->SnpMode.PermanentAddress.Addr[5]
         ));

  // Reset the device
  SoftReset (LanDriver);

  // Try to detect a PHY
  if (LanDriver->Revision > (CHIP_91100 << 4)) {
    PhyDetect (LanDriver);
  } else {
    LanDriver->PhyAd = LAN91X_NO_PHY;
  }

  return EFI_SUCCESS;
}




/*------------------ Simple Network Driver entry point functions ------------------*/

// Refer to the Simple Network Protocol section (21.1)
// in the UEFI 2.3.1 Specification for documentation.

#define ReturnUnlock(s) do { Status = (s); goto exit_unlock; } while(0)


/*
**  UEFI Start() function
**
*/
EFI_STATUS
EFIAPI
SnpStart (
  IN        EFI_SIMPLE_NETWORK_PROTOCOL* Snp
 )
{
  EFI_SIMPLE_NETWORK_MODE *Mode;
  EFI_TPL                  SavedTpl;
  EFI_STATUS               Status;

  // Check Snp instance
  if (Snp == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  // Serialize access to data and registers
  SavedTpl = gBS->RaiseTPL (LAN91X_TPL);
  Mode = Snp->Mode;

  // Check state of the driver
  switch (Mode->State) {
  case EfiSimpleNetworkStopped:
    break;
  case EfiSimpleNetworkStarted:
  case EfiSimpleNetworkInitialized:
    DEBUG ((DEBUG_WARN, "LAN91x: Driver already started\n"));
    ReturnUnlock (EFI_ALREADY_STARTED);
  default:
    DEBUG ((DEBUG_ERROR, "LAN91x: Driver in an invalid state: %u\n",
          (UINTN)Snp->Mode->State));
    ReturnUnlock (EFI_DEVICE_ERROR);
  }


  // Change state
  Mode->State = EfiSimpleNetworkStarted;
  Status = EFI_SUCCESS;

  // Restore TPL and return
exit_unlock:
  gBS->RestoreTPL (SavedTpl);
  return Status;
}

/*
**  UEFI Stop() function
**
*/
EFI_STATUS
EFIAPI
SnpStop (
  IN        EFI_SIMPLE_NETWORK_PROTOCOL* Snp
  )
{
  LAN91X_DRIVER *LanDriver;
  EFI_TPL        SavedTpl;
  EFI_STATUS     Status;

  // Check Snp Instance
  if (Snp == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  // Serialize access to data and registers
  SavedTpl = gBS->RaiseTPL (LAN91X_TPL);

  // Check state of the driver
  switch (Snp->Mode->State) {
  case EfiSimpleNetworkStarted:
  case EfiSimpleNetworkInitialized:
    break;
  case EfiSimpleNetworkStopped:
    DEBUG ((DEBUG_WARN, "LAN91x: Driver not started\n"));
    ReturnUnlock (EFI_NOT_STARTED);
  default:
    DEBUG ((DEBUG_ERROR, "LAN91x: Driver in an invalid state: %u\n",
          (UINTN)Snp->Mode->State));
    ReturnUnlock (EFI_DEVICE_ERROR);
  }

  // Find the LanDriver structure
  LanDriver = INSTANCE_FROM_SNP_THIS(Snp);

  // Stop the Tx and Rx
  ChipDisable (LanDriver);

  // Change the state
  Snp->Mode->State = EfiSimpleNetworkStopped;
  Status = EFI_SUCCESS;

  // Restore TPL and return
exit_unlock:
  gBS->RestoreTPL (SavedTpl);
  return Status;
}

/*
**  UEFI Initialize() function
**
*/
EFI_STATUS
EFIAPI
SnpInitialize (
  IN  EFI_SIMPLE_NETWORK_PROTOCOL* Snp,
  IN  UINTN                        RxBufferSize    OPTIONAL,
  IN  UINTN                        TxBufferSize    OPTIONAL
  )
{
  LAN91X_DRIVER *LanDriver;
  EFI_TPL        SavedTpl;
  EFI_STATUS     Status;

  // Check Snp Instance
  if (Snp == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  // Serialize access to data and registers
  SavedTpl = gBS->RaiseTPL (LAN91X_TPL);

  // Check that driver was started but not initialised
  switch (Snp->Mode->State) {
  case EfiSimpleNetworkStarted:
    break;
  case EfiSimpleNetworkInitialized:
    DEBUG ((DEBUG_WARN, "LAN91x: Driver already initialized\n"));
    ReturnUnlock (EFI_SUCCESS);
  case EfiSimpleNetworkStopped:
    DEBUG ((DEBUG_WARN, "LAN91x: Driver not started\n"));
    ReturnUnlock (EFI_NOT_STARTED);
  default:
    DEBUG ((DEBUG_ERROR, "LAN91x: Driver in an invalid state: %u\n",
          (UINTN)Snp->Mode->State));
    ReturnUnlock (EFI_DEVICE_ERROR);
  }

  // Find the LanDriver structure
  LanDriver = INSTANCE_FROM_SNP_THIS(Snp);

  // Initiate a software reset
  Status = SoftReset (LanDriver);
  if (EFI_ERROR(Status)) {
    DEBUG ((DEBUG_WARN, "LAN91x: Soft reset failed\n"));
    ReturnUnlock (EFI_DEVICE_ERROR);
  }

  // Initiate a PHY reset
  if (PhySoftReset (LanDriver) < 0) {
    Snp->Mode->State = EfiSimpleNetworkStopped;
    DEBUG ((DEBUG_WARN, "LAN91x: PHY soft reset timeout\n"));
    ReturnUnlock (EFI_NOT_STARTED);
  }

  // Do auto-negotiation
  Status = PhyAutoNegotiate (LanDriver);
  if (EFI_ERROR(Status)) {
    DEBUG ((DEBUG_WARN, "LAN91x: PHY auto-negotiation failed\n"));
  }

  // Enable the receiver and transmitter
  ChipEnable (LanDriver);

  // Now acknowledge all interrupts
  WriteIoReg8 (LanDriver, LAN91X_IST, 0xFF);

  // Declare the driver as initialized
  Snp->Mode->State = EfiSimpleNetworkInitialized;
  Status = EFI_SUCCESS;

  // Restore TPL and return
exit_unlock:
  gBS->RestoreTPL (SavedTpl);
  return Status;
}

/*
**  UEFI Reset () function
**
*/
EFI_STATUS
EFIAPI
SnpReset (
  IN        EFI_SIMPLE_NETWORK_PROTOCOL* Snp,
  IN        BOOLEAN Verification
  )
{
  LAN91X_DRIVER *LanDriver;
  EFI_TPL        SavedTpl;
  EFI_STATUS     Status;

  // Check Snp Instance
  if (Snp == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  // Serialize access to data and registers
  SavedTpl = gBS->RaiseTPL (LAN91X_TPL);

  // Check that driver was started and initialised
  switch (Snp->Mode->State) {
  case EfiSimpleNetworkInitialized:
    break;
  case EfiSimpleNetworkStarted:
    DEBUG ((DEBUG_WARN, "LAN91x: Driver not yet initialized\n"));
    ReturnUnlock (EFI_DEVICE_ERROR);
  case EfiSimpleNetworkStopped:
    DEBUG ((DEBUG_WARN, "LAN91x: Driver not started\n"));
    ReturnUnlock (EFI_NOT_STARTED);
  default:
    DEBUG ((DEBUG_ERROR, "LAN91x: Driver in an invalid state: %u\n",
          (UINTN)Snp->Mode->State));
    ReturnUnlock (EFI_DEVICE_ERROR);
  }

  // Find the LanDriver structure
  LanDriver = INSTANCE_FROM_SNP_THIS(Snp);

  // Initiate a software reset
  if (EFI_ERROR (SoftReset (LanDriver))) {
    DEBUG ((DEBUG_WARN, "LAN91x: Soft reset failed\n"));
    ReturnUnlock (EFI_DEVICE_ERROR);
  }

  // Initiate a PHY reset
  if (EFI_ERROR (PhySoftReset (LanDriver))) {
    DEBUG ((DEBUG_WARN, "LAN91x: PHY soft reset failed\n"));
    ReturnUnlock (EFI_DEVICE_ERROR);
  }

  // Enable the receiver and transmitter
  Status = ChipEnable (LanDriver);

  // Restore TPL and return
exit_unlock:
  gBS->RestoreTPL (SavedTpl);
  return Status;
}

/*
**  UEFI Shutdown () function
**
*/
EFI_STATUS
EFIAPI
SnpShutdown (
  IN        EFI_SIMPLE_NETWORK_PROTOCOL* Snp
  )
{
  LAN91X_DRIVER *LanDriver;
  EFI_TPL        SavedTpl;
  EFI_STATUS     Status;

  // Check Snp Instance
  if (Snp == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  // Serialize access to data and registers
  SavedTpl = gBS->RaiseTPL (LAN91X_TPL);

  // First check that driver has already been initialized
  switch (Snp->Mode->State) {
  case EfiSimpleNetworkInitialized:
    break;
  case EfiSimpleNetworkStarted:
    DEBUG ((DEBUG_WARN, "LAN91x: Driver not yet initialized\n"));
    ReturnUnlock (EFI_DEVICE_ERROR);
  case EfiSimpleNetworkStopped:
    DEBUG ((DEBUG_WARN, "LAN91x: Driver in stopped state\n"));
    ReturnUnlock (EFI_NOT_STARTED);
  default:
    DEBUG ((DEBUG_ERROR, "LAN91x: Driver in an invalid state: %u\n",
          (UINTN)Snp->Mode->State));
    ReturnUnlock (EFI_DEVICE_ERROR);
  }

  // Find the LanDriver structure
  LanDriver = INSTANCE_FROM_SNP_THIS(Snp);

  // Disable the interface
  Status = ChipDisable (LanDriver);

  // Restore TPL and return
exit_unlock:
  gBS->RestoreTPL (SavedTpl);
  return Status;
}


/*
**  UEFI ReceiveFilters() function
**
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
#define MCAST_HASH_BYTES  8

  LAN91X_DRIVER           *LanDriver;
  EFI_SIMPLE_NETWORK_MODE *SnpMode;
  EFI_TPL        SavedTpl;
  EFI_STATUS     Status;
  UINTN          i;
  UINT32         Crc;
  UINT16         RcvCtrl;
  UINT8          McastHash[MCAST_HASH_BYTES];

  // Check Snp Instance
  if (Snp == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  // Serialize access to data and registers
  SavedTpl = gBS->RaiseTPL (LAN91X_TPL);

  // First check that driver has already been initialized
  switch (Snp->Mode->State) {
  case EfiSimpleNetworkInitialized:
    break;
  case EfiSimpleNetworkStarted:
    DEBUG ((DEBUG_WARN, "LAN91x: Driver not yet initialized\n"));
    ReturnUnlock (EFI_DEVICE_ERROR);
  case EfiSimpleNetworkStopped:
    DEBUG ((DEBUG_WARN, "LAN91x: Driver not started\n"));
    ReturnUnlock (EFI_NOT_STARTED);
  default:
    DEBUG ((DEBUG_ERROR, "LAN91x: Driver in an invalid state: %u\n",
          (UINTN)Snp->Mode->State));
    ReturnUnlock (EFI_DEVICE_ERROR);
  }

  // Find the LanDriver structure
  LanDriver = INSTANCE_FROM_SNP_THIS(Snp);
  SnpMode = Snp->Mode;

#ifdef LAN91X_PRINT_RECEIVE_FILTERS
  DEBUG ((DEBUG_ERROR, "LAN91x:SnpReceiveFilters()\n"));
  DEBUG ((DEBUG_ERROR, "  Enable     = %08x\n", Enable));
  DEBUG ((DEBUG_ERROR, "  Disable    = %08x\n", Disable));
  DEBUG ((DEBUG_ERROR, "  Reset      = %d\n",  Reset));
  DEBUG ((DEBUG_ERROR, "  NumMfilter = %d\n",  NumMfilter));
  for (i = 0; i < NumMfilter; ++i) {
    DEBUG ((DEBUG_ERROR,
           "    [%2d] = %02x-%02x-%02x-%02x-%02x-%02x\n",
           i,
           Mfilter[i].Addr[0],
           Mfilter[i].Addr[1],
           Mfilter[i].Addr[2],
           Mfilter[i].Addr[3],
           Mfilter[i].Addr[4],
           Mfilter[i].Addr[5]));
  }
#endif

  // Update the Multicast Hash registers
  if (Reset) {
    // Clear the hash table
    SetMem (McastHash, MCAST_HASH_BYTES, 0);
    SnpMode->MCastFilterCount = 0;
  } else {
    // Read the current hash table
    for (i = 0; i < MCAST_HASH_BYTES; ++i) {
      McastHash[i] = ReadIoReg8 (LanDriver, LAN91X_MT0 + i);
    }
    // Set the new additions
    for (i = 0; i < NumMfilter; ++i) {
      Crc = MulticastHash (&Mfilter[i], NET_ETHER_ADDR_LEN);
      McastHash[(Crc >> 29) & 0x3] |= 1 << ((Crc >> 26) & 0x3);
    }
    SnpMode->MCastFilterCount = NumMfilter;
  }
  // If the hash registers need updating, write them
  if (Reset || NumMfilter > 0) {
    for (i = 0; i < MCAST_HASH_BYTES; ++i) {
      WriteIoReg8 (LanDriver, LAN91X_MT0 + i, McastHash[i]);
    }
  }

  RcvCtrl = ReadIoReg16 (LanDriver, LAN91X_RCR);
  if ((Enable & EFI_SIMPLE_NETWORK_RECEIVE_PROMISCUOUS) != 0) {
    RcvCtrl |= RCR_PRMS;
    SnpMode->ReceiveFilterSetting |= EFI_SIMPLE_NETWORK_RECEIVE_PROMISCUOUS;
  }
  if ((Disable & EFI_SIMPLE_NETWORK_RECEIVE_PROMISCUOUS) != 0) {
    RcvCtrl &= ~RCR_PRMS;
    SnpMode->ReceiveFilterSetting &= ~EFI_SIMPLE_NETWORK_RECEIVE_PROMISCUOUS;
  }

  if ((Enable & EFI_SIMPLE_NETWORK_RECEIVE_PROMISCUOUS_MULTICAST) != 0) {
    RcvCtrl |= RCR_ALMUL;
    SnpMode->ReceiveFilterSetting |= EFI_SIMPLE_NETWORK_RECEIVE_PROMISCUOUS_MULTICAST;
  }
  if ((Disable & EFI_SIMPLE_NETWORK_RECEIVE_PROMISCUOUS_MULTICAST) != 0) {
    RcvCtrl &= ~RCR_ALMUL;
    SnpMode->ReceiveFilterSetting &= ~EFI_SIMPLE_NETWORK_RECEIVE_PROMISCUOUS_MULTICAST;
  }
  WriteIoReg16 (LanDriver, LAN91X_RCR, RcvCtrl);

  Status = SetCurrentMacAddress (LanDriver, &SnpMode->CurrentAddress);

  // Restore TPL and return
exit_unlock:
  gBS->RestoreTPL (SavedTpl);
  return Status;
}

/*
**  UEFI StationAddress() function
**
*/
EFI_STATUS
EFIAPI
SnpStationAddress (
  IN        EFI_SIMPLE_NETWORK_PROTOCOL *Snp,
  IN        BOOLEAN Reset,
  IN        EFI_MAC_ADDRESS *NewMac
)
{
  LAN91X_DRIVER *LanDriver;
  EFI_TPL        SavedTpl;
  EFI_STATUS     Status;

  // Check Snp instance
  if (Snp == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  // Serialize access to data and registers
  SavedTpl = gBS->RaiseTPL (LAN91X_TPL);

  // Check that driver was started and initialised
  switch (Snp->Mode->State) {
  case EfiSimpleNetworkInitialized:
    break;
  case EfiSimpleNetworkStarted:
    DEBUG ((DEBUG_WARN, "LAN91x: Driver not yet initialized\n"));
    ReturnUnlock (EFI_DEVICE_ERROR);
  case EfiSimpleNetworkStopped:
    DEBUG ((DEBUG_WARN, "LAN91x: Driver not started\n"));
    ReturnUnlock (EFI_NOT_STARTED);
  default:
    DEBUG ((DEBUG_ERROR, "LAN91x: Driver in an invalid state: %u\n",
          (UINTN)Snp->Mode->State));
    ReturnUnlock (EFI_DEVICE_ERROR);
  }

  // Find the LanDriver structure
  LanDriver = INSTANCE_FROM_SNP_THIS(Snp);

  if (Reset) {
    Snp->Mode->CurrentAddress = Snp->Mode->PermanentAddress;
  } else {
    if (NewMac == NULL) {
      ReturnUnlock (EFI_INVALID_PARAMETER);
    }
    Snp->Mode->CurrentAddress = *NewMac;
  }

  Status = SetCurrentMacAddress (LanDriver, &Snp->Mode->CurrentAddress);

  // Restore TPL and return
exit_unlock:
  gBS->RestoreTPL (SavedTpl);
  return Status;
}

/*
**  UEFI Statistics() function
**
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
  LAN91X_DRIVER *LanDriver;
  EFI_TPL        SavedTpl;
  EFI_STATUS     Status;

  // Check Snp instance
  if (Snp == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  // Check pointless condition
  if ((!Reset) && (StatSize == NULL) && (Statistics == NULL)) {
    return EFI_SUCCESS;
  }

  // Check the parameters
  if ((StatSize == NULL) && (Statistics != NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  // Serialize access to data and registers
  SavedTpl = gBS->RaiseTPL (LAN91X_TPL);

  // Check that driver was started and initialised
  switch (Snp->Mode->State) {
  case EfiSimpleNetworkInitialized:
    break;
  case EfiSimpleNetworkStarted:
    DEBUG ((DEBUG_WARN, "LAN91x: Driver not yet initialized\n"));
    ReturnUnlock (EFI_DEVICE_ERROR);
  case EfiSimpleNetworkStopped:
    DEBUG ((DEBUG_WARN, "LAN91x: Driver not started\n"));
    ReturnUnlock (EFI_NOT_STARTED);
  default:
    DEBUG ((DEBUG_ERROR, "LAN91x: Driver in an invalid state: %u\n",
          (UINTN)Snp->Mode->State));
    ReturnUnlock (EFI_DEVICE_ERROR);
  }

  // Find the LanDriver structure
  LanDriver = INSTANCE_FROM_SNP_THIS(Snp);

  // Do a reset if required
  if (Reset) {
    ZeroMem (&LanDriver->Stats, sizeof(EFI_NETWORK_STATISTICS));
  }

  // Check buffer size
  if (*StatSize < sizeof(EFI_NETWORK_STATISTICS)) {
    *StatSize = sizeof(EFI_NETWORK_STATISTICS);
    ReturnUnlock (EFI_BUFFER_TOO_SMALL);
    goto exit_unlock;
  }

  // Fill in the statistics
  CopyMem(&Statistics, &LanDriver->Stats, sizeof(EFI_NETWORK_STATISTICS));
  Status = EFI_SUCCESS;

  // Restore TPL and return
exit_unlock:
  gBS->RestoreTPL (SavedTpl);
  return Status;
}

/*
**  UEFI MCastIPtoMAC() function
**
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
  // Check Snp instance
  if (Snp == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  // Check parameters
  if ((McastMac == NULL) || (Ip == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  // Make sure MAC address is empty
  ZeroMem (McastMac, sizeof(EFI_MAC_ADDRESS));

  // If we need ipv4 address
  if (!IsIpv6) {
    // Most significant 25 bits of a multicast HW address are set
    McastMac->Addr[0] = 0x01;
    McastMac->Addr[1] = 0x00;
    McastMac->Addr[2] = 0x5E;

    // Lower 23 bits from ipv4 address
    McastMac->Addr[3] = (Ip->v4.Addr[1] & 0x7F); // Clear the ms bit (25th bit of MAC must be 0)
    McastMac->Addr[4] = Ip->v4.Addr[2];
    McastMac->Addr[5] = Ip->v4.Addr[3];
  } else {
    // Most significant 16 bits of multicast v6 HW address are set
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
**  UEFI NvData() function
**
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
  DEBUG ((DEBUG_ERROR, "LAN91x: Non-volatile storage not supported\n"));

  return EFI_UNSUPPORTED;
}


/*
**  UEFI GetStatus () function
**
*/
EFI_STATUS
EFIAPI
SnpGetStatus (
  IN        EFI_SIMPLE_NETWORK_PROTOCOL *Snp,
      OUT   UINT32   *IrqStat   OPTIONAL,
      OUT   VOID    **TxBuff    OPTIONAL
  )
{
  LAN91X_DRIVER         *LanDriver;
  EFI_TPL               SavedTpl;
  EFI_STATUS            Status;
  BOOLEAN               MediaPresent;
  UINT8                 IstReg;
  MSK_LINKED_SYSTEM_BUF *LinkedTXRecycleBuff;

  // Check preliminaries
  if (Snp == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  // Serialize access to data and registers
  SavedTpl = gBS->RaiseTPL (LAN91X_TPL);

  // Check that driver was started and initialised
  switch (Snp->Mode->State) {
  case EfiSimpleNetworkInitialized:
    break;
  case EfiSimpleNetworkStarted:
    DEBUG ((DEBUG_WARN, "LAN91x: Driver not yet initialized\n"));
    ReturnUnlock (EFI_DEVICE_ERROR);
  case EfiSimpleNetworkStopped:
    DEBUG ((DEBUG_WARN, "LAN91x: Driver not started\n"));
    ReturnUnlock (EFI_NOT_STARTED);
  default:
    DEBUG ((DEBUG_ERROR, "LAN91x: Driver in an invalid state: %u\n",
          (UINTN)Snp->Mode->State));
    ReturnUnlock (EFI_DEVICE_ERROR);
  }

  // Find the LanDriver structure
  LanDriver = INSTANCE_FROM_SNP_THIS(Snp);

  // Arbitrarily set the interrupt status to 0
  if (IrqStat != NULL) {
    *IrqStat = 0;
    IstReg = ReadIoReg8 (LanDriver, LAN91X_IST);
    if ((IstReg & IST_RCV) != 0) {
      *IrqStat |= EFI_SIMPLE_NETWORK_RECEIVE_INTERRUPT;
    }
    if ((IstReg & IST_TX) != 0) {
      *IrqStat |= EFI_SIMPLE_NETWORK_TRANSMIT_INTERRUPT;
    }
  }

  // Pass back the completed buffer address
  // The transmit buffer status is not read when TxBuf is NULL
  if (TxBuff != NULL) {
    *((UINT8 **) TxBuff) = (UINT8 *) 0;
    if( !IsListEmpty (&LanDriver->TransmitQueueHead))
    {
      LinkedTXRecycleBuff = CR (GetFirstNode (&LanDriver->TransmitQueueHead), MSK_LINKED_SYSTEM_BUF, Link, TX_MBUF_SIGNATURE);
      if(LinkedTXRecycleBuff != NULL) {
        *TxBuff = LinkedTXRecycleBuff->SystemBuf.Buf;
        RemoveEntryList (&LinkedTXRecycleBuff->Link);
        FreePool (LinkedTXRecycleBuff);
      }
    }
  }

  // Update the media status
  MediaPresent = CheckLinkStatus (LanDriver);
  if (MediaPresent != Snp->Mode->MediaPresent) {
    DEBUG ((DEBUG_WARN, "LAN91x: Link %s\n", MediaPresent ? L"up" : L"down"));
  }
  Snp->Mode->MediaPresent = MediaPresent;
  Status = EFI_SUCCESS;

  // Restore TPL and return
exit_unlock:
  gBS->RestoreTPL (SavedTpl);
  return Status;
}


/*
**  UEFI Transmit() function
**
*/
EFI_STATUS
EFIAPI
SnpTransmit (
  IN        EFI_SIMPLE_NETWORK_PROTOCOL *Snp,
  IN        UINTN            HdrSize,
  IN        UINTN            BufSize,
  IN        VOID            *BufAddr,
  IN        EFI_MAC_ADDRESS *SrcAddr    OPTIONAL,
  IN        EFI_MAC_ADDRESS *DstAddr    OPTIONAL,
  IN        UINT16          *Protocol   OPTIONAL
  )
{
  LAN91X_DRIVER   *LanDriver;
  EFI_TPL          SavedTpl;
  EFI_STATUS       Status;
  UINT8           *Ptr;
  UINTN            Len;
  UINTN            MmuPages;
  UINTN            Retries;
  UINT16           Proto;
  UINT8            PktNum;
  MSK_LINKED_SYSTEM_BUF   *LinkedTXRecycleBuff;


  // Check preliminaries
  if ((Snp == NULL) || (BufAddr == NULL)) {
    DEBUG ((DEBUG_ERROR, "LAN91x: SnpTransmit(): NULL Snp (%p) or BufAddr (%p)\n",
        Snp, BufAddr));
    return EFI_INVALID_PARAMETER;
  }

  // Serialize access to data and registers
  SavedTpl = gBS->RaiseTPL (LAN91X_TPL);

  // Check that driver was started and initialised
  switch (Snp->Mode->State) {
  case EfiSimpleNetworkInitialized:
    break;
  case EfiSimpleNetworkStarted:
    DEBUG ((DEBUG_WARN, "LAN91x: Driver not yet initialized\n"));
    ReturnUnlock (EFI_DEVICE_ERROR);
  case EfiSimpleNetworkStopped:
    DEBUG ((DEBUG_WARN, "LAN91x: Driver not started\n"));
    ReturnUnlock (EFI_NOT_STARTED);
  default:
    DEBUG ((DEBUG_ERROR, "LAN91x: Driver in an invalid state: %u\n",
          (UINTN)Snp->Mode->State));
    ReturnUnlock (EFI_DEVICE_ERROR);
  }

  // Find the LanDriver structure
  LanDriver = INSTANCE_FROM_SNP_THIS(Snp);

  // Ensure header is correct size if non-zero
  if (HdrSize != 0) {
    if (HdrSize != Snp->Mode->MediaHeaderSize) {
      DEBUG ((DEBUG_ERROR, "LAN91x: SnpTransmit(): Invalid HdrSize %d\n", HdrSize));
      ReturnUnlock (EFI_INVALID_PARAMETER);
    }

    if ((DstAddr == NULL) || (Protocol == NULL)) {
      DEBUG ((DEBUG_ERROR, "LAN91x: SnpTransmit(): NULL DstAddr %p or Protocol %p\n",
          DstAddr, Protocol));
      ReturnUnlock (EFI_INVALID_PARAMETER);
    }
  }

  // Before transmitting check the link status
  if (!Snp->Mode->MediaPresent) {
    DEBUG ((DEBUG_WARN, "LAN91x: SnpTransmit(): Link not ready\n"));
    ReturnUnlock (EFI_NOT_READY);
  }

  // Calculate the request size in 256-byte "pages" minus 1
  // The 91C111 ignores this, but some older devices need it.
  MmuPages = ((BufSize & ~1) + LAN91X_PKT_OVERHEAD - 1) >> 8;
  if (MmuPages > 7) {
    DEBUG ((DEBUG_WARN, "LAN91x: Tx buffer too large (%d bytes)\n", BufSize));
    LanDriver->Stats.TxOversizeFrames += 1;
    LanDriver->Stats.TxDroppedFrames += 1;
    ReturnUnlock (EFI_BAD_BUFFER_SIZE);
  }

  // Request allocation of a transmit buffer
  Status = MmuOperation (LanDriver, MMUCR_OP_TX_ALLOC | MmuPages);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "LAN91x: Tx buffer request failure: %d\n", Status));
    ReturnUnlock (EFI_DEVICE_ERROR);
  }

  // Wait for allocation request completion
  Retries = LAN91X_MEMORY_ALLOC_POLLS;
  while ((ReadIoReg8 (LanDriver, LAN91X_IST) & IST_ALLOC) == 0) {
    if (--Retries == 0) {
      DEBUG ((DEBUG_ERROR, "LAN91x: Tx buffer allocation timeout\n"));
      ReturnUnlock (EFI_TIMEOUT);
    }
  }

  // Check for successful allocation
  PktNum = ReadIoReg8 (LanDriver, LAN91X_ARR);
  if ((PktNum & ARR_FAILED) != 0) {
    DEBUG ((DEBUG_ERROR, "LAN91x: Tx buffer allocation failure: %02x\n", PktNum));
    ReturnUnlock (EFI_NOT_READY);
  }
  PktNum &= ARR_PACKET;

  // Check for the nature of the frame
  // If no destination address, it's ARP broadcast
  if(DstAddr != NULL)
  {
    if (DstAddr->Addr[0] == 0xFF) {
      LanDriver->Stats.TxBroadcastFrames += 1;
    } else if ((DstAddr->Addr[0] & 0x1) == 1) {
      LanDriver->Stats.TxMulticastFrames += 1;
    } else {
      LanDriver->Stats.TxUnicastFrames += 1;
    }
  } else {
    LanDriver->Stats.TxBroadcastFrames += 1;
  }

  // Set the Packet Number and Pointer registers
  WriteIoReg8 (LanDriver, LAN91X_PNR, PktNum);
  WriteIoReg16 (LanDriver, LAN91X_PTR, PTR_AUTO_INCR);

  // Set up mutable buffer information variables
  Ptr = BufAddr;
  Len = BufSize;

  // Write Status and Byte Count first
  WriteIoReg16 (LanDriver, LAN91X_DATA0, 0);
  WriteIoReg16 (LanDriver, LAN91X_DATA0, (Len + LAN91X_PKT_OVERHEAD) & BCW_COUNT);

  // This packet may come with a preconfigured Ethernet header.
  // If not, we need to construct one from optional parameters.
  if (HdrSize) {

    // Write the destination address
    WriteIoData (LanDriver, DstAddr, NET_ETHER_ADDR_LEN);

    // Write the Source Address
    if (SrcAddr != NULL) {
      WriteIoData (LanDriver, SrcAddr, NET_ETHER_ADDR_LEN);
    } else {
      WriteIoData (LanDriver, &LanDriver->SnpMode.CurrentAddress, NET_ETHER_ADDR_LEN);
    }

    // Write the Protocol word
    Proto = HTONS (*Protocol);
    WriteIoReg16 (LanDriver, LAN91X_DATA0, Proto);

    // Adjust the data start and length
    Ptr += sizeof(ETHER_HEAD);
    Len -= sizeof(ETHER_HEAD);
  }

  // Copy the remainder data buffer, except the odd byte
  WriteIoData (LanDriver, Ptr, Len & ~1);
  Ptr += Len & ~1;
  Len &= 1;

  // Write the Packet Control Word and odd byte
  WriteIoReg16 (LanDriver, LAN91X_DATA0,
      (Len != 0) ? (PCW_ODD | PCW_CRC | *Ptr) : PCW_CRC);

  // Release the packet for transmission
  Status = MmuOperation (LanDriver, MMUCR_OP_TX_PUSH);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "LAN91x: Tx buffer release failure: %d\n", Status));
    ReturnUnlock (EFI_DEVICE_ERROR);
  }

  // Update the Tx statistics
  LanDriver->Stats.TxTotalBytes += BufSize;
  LanDriver->Stats.TxGoodFrames += 1;

  // Update the Tx Buffer cache
  LinkedTXRecycleBuff = AllocateZeroPool (sizeof (MSK_LINKED_SYSTEM_BUF));
  if (LinkedTXRecycleBuff == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }
  LinkedTXRecycleBuff->Signature = TX_MBUF_SIGNATURE;
  //
  // Add the passed Buffer to the transmit queue. Don't copy.
  //
  LinkedTXRecycleBuff->SystemBuf.Buf = BufAddr;
  LinkedTXRecycleBuff->SystemBuf.Length = BufSize;
  InsertTailList (&LanDriver->TransmitQueueHead, &LinkedTXRecycleBuff->Link);

  Status = EFI_SUCCESS;

  // Dump the packet header
#if LAN91X_PRINT_PACKET_HEADERS
  Ptr = BufAddr;
  DEBUG ((DEBUG_ERROR, "LAN91X:SnpTransmit()\n"));
  DEBUG ((DEBUG_ERROR, "  HdrSize: %d, SrcAddr: %p, Length: %d, Last byte: %02x\n",
         HdrSize, SrcAddr, BufSize, Ptr[BufSize - 1]));
  PrintIpDgram (
      (HdrSize == 0) ? (EFI_MAC_ADDRESS *)&Ptr[0] : DstAddr,
      (HdrSize == 0) ? (EFI_MAC_ADDRESS *)&Ptr[6] : (SrcAddr != NULL) ? SrcAddr : &LanDriver->SnpMode.CurrentAddress,
      (HdrSize == 0) ? (UINT16 *)&Ptr[12] : &Proto,
      &Ptr[14]
      );
#endif

  // Restore TPL and return
exit_unlock:
  gBS->RestoreTPL (SavedTpl);
  return Status;
}


/*
**  UEFI Receive() function
**
*/
EFI_STATUS
EFIAPI
SnpReceive (
  IN        EFI_SIMPLE_NETWORK_PROTOCOL *Snp,
      OUT   UINTN           *HdrSize      OPTIONAL,
  IN  OUT   UINTN           *BuffSize,
      OUT   VOID            *Data,
      OUT   EFI_MAC_ADDRESS *SrcAddr      OPTIONAL,
      OUT   EFI_MAC_ADDRESS *DstAddr      OPTIONAL,
      OUT   UINT16 *Protocol              OPTIONAL
  )
{
  EFI_TPL        SavedTpl;
  EFI_STATUS     Status;
  LAN91X_DRIVER *LanDriver;
  UINT8         *DataPtr;
  UINT16         PktStatus;
  UINT16         PktLength;
  UINT16         PktControl;
  UINT8          IstReg;

  // Check preliminaries
  if ((Snp == NULL) || (Data == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  // Serialize access to data and registers
  SavedTpl = gBS->RaiseTPL (LAN91X_TPL);

  // Check that driver was started and initialised
  switch (Snp->Mode->State) {
  case EfiSimpleNetworkInitialized:
    break;
  case EfiSimpleNetworkStarted:
    DEBUG ((DEBUG_WARN, "LAN91x: Driver not yet initialized\n"));
    ReturnUnlock (EFI_DEVICE_ERROR);
  case EfiSimpleNetworkStopped:
    DEBUG ((DEBUG_WARN, "LAN91x: Driver not started\n"));
    ReturnUnlock (EFI_NOT_STARTED);
  default:
    DEBUG ((DEBUG_ERROR, "LAN91x: Driver in an invalid state: %u\n",
          (UINTN)Snp->Mode->State));
    ReturnUnlock (EFI_DEVICE_ERROR);
  }

  // Find the LanDriver structure
  LanDriver = INSTANCE_FROM_SNP_THIS(Snp);

  // Check for Rx Overrun
  IstReg = ReadIoReg8 (LanDriver, LAN91X_IST);
  if ((IstReg & IST_RX_OVRN) != 0) {
    LanDriver->Stats.RxTotalFrames += 1;
    LanDriver->Stats.RxDroppedFrames += 1;
    WriteIoReg8 (LanDriver, LAN91X_IST, IST_RX_OVRN);
    DEBUG ((DEBUG_WARN, "LAN91x: Receiver overrun\n"));
  }

  // Check for Rx data available
  if ((IstReg & IST_RCV) == 0) {
    ReturnUnlock (EFI_NOT_READY);
  }

  // Configure the PTR register for reading
  WriteIoReg16 (LanDriver, LAN91X_PTR, PTR_RCV | PTR_AUTO_INCR | PTR_READ);

  // Read the Packet Status and Packet Length words
  PktStatus = ReadIoReg16 (LanDriver, LAN91X_DATA0);
  PktLength = ReadIoReg16 (LanDriver, LAN91X_DATA0) & BCW_COUNT;

  // Check for valid received packet
  if ((PktStatus == 0) && (PktLength == 0)) {
    DEBUG ((DEBUG_WARN, "LAN91x: Received zero-length packet. IST=%04x\n", IstReg));
    ReturnUnlock (EFI_NOT_READY);
  }
  LanDriver->Stats.RxTotalFrames += 1;

  // Check if we got a CRC error
  if ((PktStatus & RX_BAD_CRC) != 0) {
    DEBUG ((DEBUG_WARN, "LAN91x: Received frame CRC error\n"));
    LanDriver->Stats.RxCrcErrorFrames += 1;
    LanDriver->Stats.RxDroppedFrames += 1;
    Status = EFI_DEVICE_ERROR;
    goto exit_release;
  }

  // Check if we got a too-short frame
  if ((PktStatus & RX_TOO_SHORT) != 0) {
    DEBUG ((DEBUG_WARN, "LAN91x: Received frame too short (%d bytes)\n", PktLength));
    LanDriver->Stats.RxUndersizeFrames += 1;
    LanDriver->Stats.RxDroppedFrames += 1;
    Status = EFI_DEVICE_ERROR;
    goto exit_release;
  }

   // Check if we got a too-long frame
  if ((PktStatus & RX_TOO_LONG) != 0) {
    DEBUG ((DEBUG_WARN, "LAN91x: Received frame too long (%d bytes)\n", PktLength));
    LanDriver->Stats.RxOversizeFrames += 1;
    LanDriver->Stats.RxDroppedFrames += 1;
    Status = EFI_DEVICE_ERROR;
    goto exit_release;
  }

   // Check if we got an alignment error
  if ((PktStatus & RX_ALGN_ERR) != 0) {
    DEBUG ((DEBUG_WARN, "LAN91x: Received frame alignment error\n"));
    // Don't seem to keep track of these specifically
    LanDriver->Stats.RxDroppedFrames += 1;
    Status = EFI_DEVICE_ERROR;
    goto exit_release;
  }

  // Classify the received fram
  if ((PktStatus & RX_MULTICAST) != 0) {
    LanDriver->Stats.RxMulticastFrames += 1;
  } else if ((PktStatus & RX_BROADCAST) != 0) {
    LanDriver->Stats.RxBroadcastFrames += 1;
  } else {
    LanDriver->Stats.RxUnicastFrames += 1;
  }

  // Calculate the received packet data length
  PktLength -= LAN91X_PKT_OVERHEAD;
  if ((PktStatus & RX_ODD_FRAME) != 0) {
    PktLength += 1;
  }

  // Check buffer size
  if (*BuffSize < PktLength) {
    DEBUG ((DEBUG_WARN, "LAN91x: Receive buffer too small for packet (%d < %d)\n",
        *BuffSize, PktLength));
    *BuffSize = PktLength;
    Status = EFI_BUFFER_TOO_SMALL;
    goto exit_release;
  }

  // Transfer the data bytes
  DataPtr = Data;
  ReadIoData (LanDriver, DataPtr, PktLength & ~0x0001);

  // Read the PktControl and Odd Byte from the FIFO
  PktControl = ReadIoReg16 (LanDriver, LAN91X_DATA0);
  if ((PktControl & PCW_ODD) != 0) {
    DataPtr[PktLength - 1] = PktControl & PCW_ODD_BYTE;
  }

  // Update buffer size
  *BuffSize = PktLength;

  if (HdrSize != NULL) {
    *HdrSize = LanDriver->SnpMode.MediaHeaderSize;
  }

  // Extract the destination address
  if (DstAddr != NULL) {
    CopyMem (DstAddr, &DataPtr[0], NET_ETHER_ADDR_LEN);
  }

  // Get the source address
  if (SrcAddr != NULL) {
    CopyMem (SrcAddr, &DataPtr[6], NET_ETHER_ADDR_LEN);
  }

  // Get the protocol
  if (Protocol != NULL) {
    *Protocol = NTOHS (*(UINT16*)(&DataPtr[12]));
  }

  // Update the Rx statistics
  LanDriver->Stats.RxTotalBytes += PktLength;
  LanDriver->Stats.RxGoodFrames += 1;
  Status = EFI_SUCCESS;

#if LAN91X_PRINT_PACKET_HEADERS
  // Dump the packet header
  DEBUG ((DEBUG_ERROR, "LAN91X:SnpReceive()\n"));
  DEBUG ((DEBUG_ERROR, "  HdrSize: %p, SrcAddr: %p, DstAddr: %p, Protocol: %p\n",
         HdrSize, SrcAddr, DstAddr, Protocol));
  DEBUG ((DEBUG_ERROR, "  Length: %d, Last byte: %02x\n", PktLength, DataPtr[PktLength - 1]));
  PrintIpDgram (&DataPtr[0], &DataPtr[6], &DataPtr[12], &DataPtr[14]);
#endif

  // Release the FIFO buffer
exit_release:
  MmuOperation (LanDriver, MMUCR_OP_RX_POP_REL);

  // Restore TPL and return
exit_unlock:
  gBS->RestoreTPL (SavedTpl);
  return Status;
}


/*------------------ Driver Execution Environment main entry point ------------------*/

/*
**  Entry point for the LAN91x driver
**
*/
EFI_STATUS
Lan91xDxeEntry (
  IN EFI_HANDLE Handle,
  IN EFI_SYSTEM_TABLE *SystemTable
  )
{
  EFI_STATUS Status;
  LAN91X_DRIVER *LanDriver;
  EFI_SIMPLE_NETWORK_PROTOCOL *Snp;
  EFI_SIMPLE_NETWORK_MODE *SnpMode;
  LAN91X_DEVICE_PATH *Lan91xPath;

  // The PcdLan91xDxeBaseAddress PCD must be defined
  ASSERT(PcdGet32 (PcdLan91xDxeBaseAddress) != 0);

  // Allocate Resources
  LanDriver = AllocateZeroPool (sizeof(LAN91X_DRIVER));
  Lan91xPath = AllocateCopyPool (sizeof(LAN91X_DEVICE_PATH), &Lan91xPathTemplate);

  // Initialize I/O Space access info
  LanDriver->IoBase = PcdGet32 (PcdLan91xDxeBaseAddress);
  LanDriver->PhyAd = LAN91X_NO_PHY;
  LanDriver->BankSel = 0xff;

  // Initialize pointers
  Snp = &(LanDriver->Snp);
  SnpMode = &(LanDriver->SnpMode);
  Snp->Mode = SnpMode;

  // Set the signature of the LAN Driver structure
  LanDriver->Signature = LAN91X_SIGNATURE;

  // Probe the device
  Status = Probe (LanDriver);
  if (EFI_ERROR(Status)) {
    DEBUG ((DEBUG_ERROR, "LAN91x:Lan91xDxeEntry(): Probe failed with status %d\n", Status));
    return Status;
  }

#ifdef LAN91X_PRINT_REGISTERS
  PrintIoRegisters (LanDriver);
  PrintPhyRegisters (LanDriver);
#endif

  // Initialize transmit queue
  InitializeListHead (&LanDriver->TransmitQueueHead);

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

  // Fill in simple network mode structure
  SnpMode->State = EfiSimpleNetworkStopped;
  SnpMode->HwAddressSize = NET_ETHER_ADDR_LEN;    // HW address is 6 bytes
  SnpMode->MediaHeaderSize = sizeof(ETHER_HEAD);  // Size of an Ethernet header
  SnpMode->MaxPacketSize = EFI_PAGE_SIZE;         // Ethernet Frame (with VLAN tag +4 bytes)

  // Supported receive filters
  SnpMode->ReceiveFilterMask = EFI_SIMPLE_NETWORK_RECEIVE_UNICAST |
                               EFI_SIMPLE_NETWORK_RECEIVE_MULTICAST |
                               EFI_SIMPLE_NETWORK_RECEIVE_BROADCAST |
                               EFI_SIMPLE_NETWORK_RECEIVE_PROMISCUOUS |
                               EFI_SIMPLE_NETWORK_RECEIVE_PROMISCUOUS_MULTICAST;

  // Initially-enabled receive filters
  SnpMode->ReceiveFilterSetting = EFI_SIMPLE_NETWORK_RECEIVE_UNICAST |
                                  EFI_SIMPLE_NETWORK_RECEIVE_MULTICAST |
                                  EFI_SIMPLE_NETWORK_RECEIVE_BROADCAST;

  // LAN91x has 64bit hash table. We can filter an infinite MACs, but
  // higher-level software must filter out any hash collisions.
  SnpMode->MaxMCastFilterCount = MAX_MCAST_FILTER_CNT;
  SnpMode->MCastFilterCount = 0;
  ZeroMem (&SnpMode->MCastFilter, MAX_MCAST_FILTER_CNT * sizeof(EFI_MAC_ADDRESS));

  // Set the interface type (1: Ethernet or 6: IEEE 802 Networks)
  SnpMode->IfType = NET_IFTYPE_ETHERNET;

  // Mac address is changeable
  SnpMode->MacAddressChangeable = TRUE;

  // We can only transmit one packet at a time
  SnpMode->MultipleTxSupported = FALSE;

  // MediaPresent checks for cable connection and partner link
  SnpMode->MediaPresentSupported = TRUE;
  SnpMode->MediaPresent = FALSE;

  //  Set broadcast address
  SetMem (&SnpMode->BroadcastAddress, sizeof (EFI_MAC_ADDRESS), 0xFF);

  // Assign fields for device path
  Lan91xPath->Lan91x.MacAddress = SnpMode->PermanentAddress;
  Lan91xPath->Lan91x.IfType = SnpMode->IfType;

  // Initialise the protocol
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &LanDriver->ControllerHandle,
                  &gEfiSimpleNetworkProtocolGuid, Snp,
                  &gEfiDevicePathProtocolGuid, Lan91xPath,
                  NULL
                  );

  // Say what the status of loading the protocol structure is
  if (EFI_ERROR(Status)) {
    FreePool (LanDriver);
  }

  return Status;
}
