/** @file
  Various defines related to Cloud Hypervisor

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __CLOUDHV_H__
#define __CLOUDHV_H__

//
// Host Bridge Device ID
//
#define CLOUDHV_DEVICE_ID  0x0d57

//
// ACPI timer address
//
#define CLOUDHV_ACPI_TIMER_IO_ADDRESS  0x0608

//
// ACPI shutdown device address
//
#define CLOUDHV_ACPI_SHUTDOWN_IO_ADDRESS  0x0600

//
// 32-bit MMIO memory hole base address
//
#define CLOUDHV_MMIO_HOLE_ADDRESS  0xc0000000

//
// 32-bit MMIO memory hole size
//
#define CLOUDHV_MMIO_HOLE_SIZE  0x38000000

//
// SMBIOS address
//
#define CLOUDHV_SMBIOS_ADDRESS  0xf0000

#endif // __CLOUDHV_H__
