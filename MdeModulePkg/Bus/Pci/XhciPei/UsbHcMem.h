/** @file
Private Header file for Usb Host Controller PEIM

Copyright (c) 2014, Intel Corporation. All rights reserved.<BR>

This program and the accompanying materials
are licensed and made available under the terms and conditions
of the BSD License which accompanies this distribution.  The
full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _EFI_PEI_XHCI_MEM_H_
#define _EFI_PEI_XHCI_MEM_H_

#include <Uefi.h>

#define USBHC_MEM_DEFAULT_PAGES 16

typedef struct _USBHC_MEM_BLOCK USBHC_MEM_BLOCK;

struct _USBHC_MEM_BLOCK {
  UINT8                 *Bits;  // Bit array to record which unit is allocated
  UINTN                 BitsLen;
  UINT8                 *Buf;
  UINT8                 *BufHost;
  UINTN                 BufLen; // Memory size in bytes
  USBHC_MEM_BLOCK       *Next;
};

//
// Memory allocation unit, must be 2^n, n>4
//
#define USBHC_MEM_UNIT          64

#define USBHC_MEM_UNIT_MASK     (USBHC_MEM_UNIT - 1)
#define USBHC_MEM_ROUND(Len)    (((Len) + USBHC_MEM_UNIT_MASK) & (~USBHC_MEM_UNIT_MASK))

#define USB_HC_BIT(a)           ((UINTN)(1 << (a)))

#define USB_HC_BIT_IS_SET(Data, Bit)   \
          ((BOOLEAN)(((Data) & USB_HC_BIT(Bit)) == USB_HC_BIT(Bit)))

//
// Advance the byte and bit to the next bit, adjust byte accordingly.
//
#define NEXT_BIT(Byte, Bit)  \
          do {               \
            (Bit)++;         \
            if ((Bit) > 7) { \
              (Byte)++;      \
              (Bit) = 0;     \
            }                \
          } while (0)

//
// USBHC_MEM_POOL is used to manage the memory used by USB
// host controller. XHCI requires the control memory and transfer
// data to be on the same 4G memory.
//
typedef struct _USBHC_MEM_POOL {
  BOOLEAN               Check4G;
  UINT32                Which4G;
  USBHC_MEM_BLOCK       *Head;
} USBHC_MEM_POOL;

/**
  Calculate the corresponding pci bus address according to the Mem parameter.

  @param  Pool          The memory pool of the host controller.
  @param  Mem           The pointer to host memory.
  @param  Size          The size of the memory region.

  @return               The pci memory address

**/
EFI_PHYSICAL_ADDRESS
UsbHcGetPciAddrForHostAddr (
  IN USBHC_MEM_POOL     *Pool,
  IN VOID               *Mem,
  IN UINTN              Size
  );

/**
  Calculate the corresponding host address according to the pci address.

  @param  Pool          The memory pool of the host controller.
  @param  Mem           The pointer to pci memory.
  @param  Size          The size of the memory region.

  @return               The host memory address

**/
EFI_PHYSICAL_ADDRESS
UsbHcGetHostAddrForPciAddr (
  IN USBHC_MEM_POOL     *Pool,
  IN VOID               *Mem,
  IN UINTN              Size
  );

/**
  Allocates pages at a specified alignment.

  If Alignment is not a power of two and Alignment is not zero, then ASSERT().

  @param  Pages                 The number of pages to allocate.
  @param  Alignment             The requested alignment of the allocation.  Must be a power of two.
  @param  HostAddress           The system memory address to map to the PCI controller.
  @param  DeviceAddress         The resulting map address for the bus master PCI controller to
                                use to access the hosts HostAddress.

  @retval EFI_SUCCESS           Success to allocate aligned pages.
  @retval EFI_INVALID_PARAMETER Pages or Alignment is not valid.
  @retval EFI_OUT_OF_RESOURCES  Do not have enough resources to allocate memory.

**/
EFI_STATUS
UsbHcAllocateAlignedPages (
  IN UINTN                      Pages,
  IN UINTN                      Alignment,
  OUT VOID                      **HostAddress,
  OUT EFI_PHYSICAL_ADDRESS      *DeviceAddress
  );

/**
  Frees memory that was allocated with UsbHcAllocateAlignedPages().

  @param  HostAddress           The system memory address to map to the PCI controller.
  @param  Pages                 The number of pages to free.

**/
VOID
UsbHcFreeAlignedPages (
  IN VOID               *HostAddress,
  IN UINTN              Pages
  );

#endif
