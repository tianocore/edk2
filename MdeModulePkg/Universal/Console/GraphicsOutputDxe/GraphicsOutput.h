/** @file
  Header file for a generic GOP driver.

Copyright (c) 2016, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.


**/
#ifndef _GRAPHICS_OUTPUT_DXE_H_
#define _GRAPHICS_OUTPUT_DXE_H_
#include <PiDxe.h>

#include <IndustryStandard/Pci.h>
#include <IndustryStandard/Acpi.h>
#include <Guid/GraphicsInfoHob.h>
#include <Protocol/DriverBinding.h>
#include <Protocol/PciIo.h>
#include <Protocol/DevicePath.h>
#include <Protocol/GraphicsOutput.h>
#include <Protocol/ComponentName.h>
#include <Protocol/ComponentName2.h>

#include <Library/BaseLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/HobLib.h>
#include <Library/DevicePathLib.h>
#include <Library/FrameBufferBltLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiLib.h>

#define MAX_PCI_BAR  6

typedef struct {
  UINT32                            Signature;
  EFI_HANDLE                        GraphicsOutputHandle;
  EFI_GRAPHICS_OUTPUT_PROTOCOL      GraphicsOutput;
  EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE GraphicsOutputMode;
  EFI_DEVICE_PATH_PROTOCOL          *DevicePath;
  EFI_PCI_IO_PROTOCOL               *PciIo;
  UINT64                            PciAttributes;
  FRAME_BUFFER_CONFIGURE            *FrameBufferBltLibConfigure;
  UINTN                             FrameBufferBltLibConfigureSize;
} GRAPHICS_OUTPUT_PRIVATE_DATA;

#define GRAPHICS_OUTPUT_PRIVATE_DATA_SIGNATURE  SIGNATURE_32 ('g', 'g', 'o', 'p')
#define GRAPHICS_OUTPUT_PRIVATE_FROM_THIS(a) \
  CR(a, GRAPHICS_OUTPUT_PRIVATE_DATA, GraphicsOutput, GRAPHICS_OUTPUT_PRIVATE_DATA_SIGNATURE)

extern EFI_COMPONENT_NAME_PROTOCOL  mGraphicsOutputComponentName;
extern EFI_COMPONENT_NAME2_PROTOCOL mGraphicsOutputComponentName2;
#endif
