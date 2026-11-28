/** @file
  Google Test mock for EFI_PCI_IO_PROTOCOL

  Copyright (C) 2026 Advanced Micro Devices, Inc. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <GoogleTest/Protocol/MockPciIoProtocol.h>

MOCK_INTERFACE_DEFINITION (MockPciIoProtocol);
MOCK_FUNCTION_DEFINITION (MockPciIoProtocol, PciRead, 5, EFIAPI);

EFI_PCI_IO_PROTOCOL  PCI_IO_PROTOCOL_INSTANCE = {
  NULL,                  // PollMem
  NULL,                  // PollIo
  { NULL,    NULL },     // Mem
  { NULL,    NULL },     // Io
  { PciRead, NULL },     // Pci
  NULL,                  // CopyMem
  NULL,                  // Map
  NULL,                  // Unmap
  NULL,                  // AllocateBuffer
  NULL,                  // FreeBuffer
  NULL,                  // Flush
  NULL,                  // GetLocation
  NULL,                  // Attributes
  NULL,                  // GetBarAttributes
  NULL,                  // SetBarAttributes
  0,                     // RomSize
  NULL                   // RomImage
};

extern "C" {
  EFI_PCI_IO_PROTOCOL  *gPciIoProtocol = &PCI_IO_PROTOCOL_INSTANCE;
}
