/**************************************************************************;
;*                                                                        *;
;*                                                                        *;
;*    Intel Corporation - ACPI Reference Code for the Baytrail            *;
;*    Family of Customer Reference Boards.                                *;
;*                                                                        *;
;*                                                                        *;
;*    Copyright (c) 2012  - 2014, Intel Corporation. All rights reserved    *;
;
; SPDX-License-Identifier: BSD-2-Clause-Patent
;
;*                                                                        *;
;*                                                                        *;
;**************************************************************************/



Scope (\_SB.PCI0)
{

  Device(PDRC)   // PCI Device Resource Consumption
  {
    Name(_HID,EISAID("PNP0C02"))

    Name(_UID,1)

    Name(BUF0,ResourceTemplate()
    {
      //
      // PCI Express BAR _BAS and _LEN will be updated in _CRS below according to B0:D0:F0:Reg.60h
      // Forced hard code at the moment.
      //
      //Memory32Fixed(ReadWrite,0,0,PCIX)       // PCIEX BAR
      Memory32Fixed(ReadWrite,0x0E0000000,0x010000000,PCIX)

      //
      // SPI BAR. Check if the hard code meets the real configuration.
      // If not, dynamically update it like the _CRS method below.
      //
      Memory32Fixed(ReadWrite,0x0FED01000,0x01000,SPIB) // SPI BAR

      //
      // PMC BAR. Check if the hard code meets the real configuration.
      // If not, dynamically update it like the _CRS method below.
      //
      Memory32Fixed(ReadWrite,0x0FED03000,0x01000,PMCB) // PMC BAR

      //
      // SMB BAR. Check if the hard code meets the real configuration.
      // If not, dynamically update it like the _CRS method below.
      //
      Memory32Fixed(ReadWrite,0x0FED04000,0x01000,SMBB) // SMB BAR

      //
      // IO BAR. Check if the hard code meets the real configuration.
      // If not, dynamically update it like the _CRS method below.
      //
      Memory32Fixed(ReadWrite,0x0FED0C000,0x04000,IOBR) // IO BAR

      //
      // ILB BAR. Check if the hard code meets the real configuration.
      // If not, dynamically update it like the _CRS method below.
      //
      Memory32Fixed(ReadWrite,0x0FED08000,0x01000,ILBB) // ILB BAR

      //
      // RCRB BAR _BAS will be updated in _CRS below according to B0:D31:F0:Reg.F0h
      //
      Memory32Fixed(ReadWrite,0x0FED1C000,0x01000,RCRB) // RCRB BAR

      //
      // Local APIC range(0xFEE0_0000 to 0xFEEF_FFFF)
      //
      Memory32Fixed (ReadOnly, 0x0FEE00000, 0x0100000, LIOH)

      //
      // MPHY BAR. Check if the hard code meets the real configuration.
      // If not, dynamically update it like the _CRS method below.
      //
      Memory32Fixed(ReadWrite,0x0FEF00000,0x0100000,MPHB)       // MPHY BAR
    })

    Method(_CRS,0,Serialized)
    {

      Return(BUF0)
    }

  }
}
