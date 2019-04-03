/** @file
Register initialization table for Ich.

Copyright (c) 2013-2015 Intel Corporation.

SPDX-License-Identifier: BSD-2-Clause-Patent


**/


#include "CommonHeader.h"

VOID
PlatformInitQNCRegs (
  VOID
  )
{
  //
  // All devices on bus 0.
  // Device 0:
  //    FNC 0: Host Bridge
  // Device 20:
  //    FNC 0: IOSF2AHB Bridge
  // Device 21:
  //    FNC 0: IOSF2AHB Bridge
  // Device 23:
  //    FNC 0: PCIe Port 0
  // Device 24:
  //    FNC 0: PCIe Port 1

  // Device 31:
  //    FNC 0: PCI-LPC Bridge
  //
  S3PciWrite32 (PCI_LIB_ADDRESS (PCI_BUS_NUMBER_QNC, PCI_DEVICE_NUMBER_QNC_LPC, PCI_FUNCTION_NUMBER_QNC_LPC, R_QNC_LPC_FWH_BIOS_DEC),
                B_QNC_LPC_FWH_BIOS_DEC_F0 | B_QNC_LPC_FWH_BIOS_DEC_F8 |
                B_QNC_LPC_FWH_BIOS_DEC_E0 | B_QNC_LPC_FWH_BIOS_DEC_E8 |
                B_QNC_LPC_FWH_BIOS_DEC_D0 | B_QNC_LPC_FWH_BIOS_DEC_D8 |
                B_QNC_LPC_FWH_BIOS_DEC_C0 | B_QNC_LPC_FWH_BIOS_DEC_C8
                );

  //
  // Program SCI Interrupt for IRQ9
  //
  S3PciWrite8 (PCI_LIB_ADDRESS (PCI_BUS_NUMBER_QNC, PCI_DEVICE_NUMBER_QNC_LPC, PCI_FUNCTION_NUMBER_QNC_LPC, R_QNC_LPC_ACTL),
               V_QNC_LPC_ACTL_SCIS_IRQ9
               );

  //
  // Program Quark Interrupt Route Registers
  //
  S3MmioWrite16 ((UINTN)PcdGet64(PcdRcbaMmioBaseAddress) + R_QNC_RCRB_AGENT0IR,
             PcdGet16(PcdQuarkAgent0IR)
             );
  S3MmioWrite16 ((UINTN)PcdGet64(PcdRcbaMmioBaseAddress) + R_QNC_RCRB_AGENT1IR,
             PcdGet16(PcdQuarkAgent1IR)
             );
  S3MmioWrite16 ((UINTN)PcdGet64(PcdRcbaMmioBaseAddress) + R_QNC_RCRB_AGENT2IR,
             PcdGet16(PcdQuarkAgent2IR)
             );
  S3MmioWrite16 ((UINTN)PcdGet64(PcdRcbaMmioBaseAddress) + R_QNC_RCRB_AGENT3IR,
             PcdGet16(PcdQuarkAgent3IR)
             );

  //
  // Program SVID and SID for QNC PCI devices. In order to boost performance, we
  // combine two 16 bit PCI_WRITE into one 32 bit PCI_WRITE. The programmed LPC SVID
  // will reflect on all internal devices's SVID registers
  //
  S3PciWrite32 (PCI_LIB_ADDRESS (PCI_BUS_NUMBER_QNC, PCI_DEVICE_NUMBER_QNC_LPC, PCI_FUNCTION_NUMBER_QNC_LPC, R_EFI_PCI_SVID),
                (UINT32)(V_INTEL_VENDOR_ID + (QUARK_V_LPC_DEVICE_ID_0 << 16))
                );

  //
  // Write once on Element Self Description Register before OS boot
  //
  QNCMmio32And (PcdGet64(PcdRcbaMmioBaseAddress), 0x04, 0xFF00FFFF);

  return;
}
