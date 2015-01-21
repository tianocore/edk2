/*++

  Copyright (c) 2004  - 2014, Intel Corporation. All rights reserved.<BR>

  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.
  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.





Module Name:

  PlatformBaseAddresses.h

Abstract:



Revision History

++*/


#ifndef _PLATFORM_BASE_ADDRESSES_H
#define _PLATFORM_BASE_ADDRESSES_H

//
// Define some fixed platform device location information
//

//
// Define platform base
//

//
// SIO
//
#define SIO_BASE_ADDRESS                  0x0680
#define SIO_MONITORING_BASE_ADDRESS       0x0290
#define SIO_BASE_MASK                     0xFFF0
#define WINDBOND_ECIR_BASE_ADDRESS        0x0810
#define SIO_MAILBOX_BASE_ADDRESS          0x0360    // Used by EC controller
#define SIO_EC_CHANNEL2                   0x62      // Used by EC controller for offset 0x62 and 0x66


//
// South Cluster
//
#define ACPI_BASE_ADDRESS                 0x0400
#define GPIO_BASE_ADDRESS                 0x0500
#define SMBUS_BUS_DEV_FUNC                0x1F0300
#define SMBUS_BASE_ADDRESS                0xEFA0     // SMBus IO Base Address
#define SPI_BASE_ADDRESS                  0xFED01000 // SPI Memory Base Address
#define PMC_BASE_ADDRESS                  0xFED03000 // PMC Memory Base Address
#define SMBM_BASE_ADDRESS                 0xFED04000 // SMBus Memory Base Address
#define IO_BASE_ADDRESS                   0xFED0C000 // IO Memory Base Address
#define ILB_BASE_ADDRESS                  0xFED08000 // ILB Memory Base Address
#define HPET_BASE_ADDRESS                 0xFED00000 // HPET Base Address
#define RCBA_BASE_ADDRESS                 0xFED1C000 // Root Complex Base Address
#define MPHY_BASE_ADDRESS                 0xFEF00000 // MPHY Memory Base Address
#define PUNIT_BASE_ADDRESS                0xFED05000 // PUnit Memory Base Address

//
// GPIO GROUP OFFSET
//
#define GPIO_SCORE_OFFSET                 0x0000
#define GPIO_NCORE_OFFSET                 0x1000
#define GPIO_SSUS_OFFSET                  0x2000

//
// MCH/CPU
//
#define DMI_BASE_ADDRESS                  0xFED18000 // 4K, similar to IIO_RCBA // modify from bearlake -- cchew10
#define EP_BASE_ADDRESS                   0xFED19000
#define MC_MMIO_BASE                      0xFED14000 // Base Address for MMIO registers

//
// TPM
//
#define TPM_BASE_ADDRESS                  0xFED40000  // Base address for TPM

//
// Local and I/O APIC addresses.
//
#define IO_APIC_ADDRESS                   0xFEC00000
#define IIO_IOAPIC_ADDRESS                0xFEC90000
#define LOCAL_APIC_ADDRESS                0xFEE00000


#endif


