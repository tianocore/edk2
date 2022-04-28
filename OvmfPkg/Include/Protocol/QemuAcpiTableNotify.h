/** @file

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef QEMU_ACPI_TABLE_NOTIFY_H_
#define QEMU_ACPI_TABLE_NOTIFY_H_

#define QEMU_ACPI_TABLE_NOTIFY_GUID \
  { 0x928939b2, 0x4235, 0x462f, { 0x95, 0x80, 0xf6, 0xa2, 0xb2, 0xc2, 0x1a, 0x4f } };

///
/// Forward declaration
///
typedef struct _QEMU_ACPI_TABLE_NOTIFY_PROTOCOL QEMU_ACPI_TABLE_NOTIFY_PROTOCOL;

///
/// Protocol structure
///
struct _QEMU_ACPI_TABLE_NOTIFY_PROTOCOL {
  UINT8    Notify;
};

extern EFI_GUID  gQemuAcpiTableNotifyProtocolGuid;

#endif
