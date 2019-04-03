/** @file
Private Header file for Usb Host Controller PEIM

Copyright (c) 2010 - 2018, Intel Corporation. All rights reserved.<BR>

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _EFI_EHCI_MEM_H_
#define _EFI_EHCI_MEM_H_

#include <Uefi.h>
#include <IndustryStandard/Pci22.h>

#define USB_HC_BIT(a)                  ((UINTN)(1 << (a)))

#define USB_HC_BIT_IS_SET(Data, Bit)   \
          ((BOOLEAN)(((Data) & USB_HC_BIT(Bit)) == USB_HC_BIT(Bit)))

#define USB_HC_HIGH_32BIT(Addr64)    \
          ((UINT32)(RShiftU64((UINTN)(Addr64), 32) & 0XFFFFFFFF))

typedef struct _USBHC_MEM_BLOCK USBHC_MEM_BLOCK;

struct _USBHC_MEM_BLOCK {
  UINT8                   *Bits;    // Bit array to record which unit is allocated
  UINTN                   BitsLen;
  UINT8                   *Buf;
  UINT8                   *BufHost;
  UINTN                   BufLen;   // Memory size in bytes
  VOID                    *Mapping;
  USBHC_MEM_BLOCK         *Next;
};

//
// USBHC_MEM_POOL is used to manage the memory used by USB
// host controller. EHCI requires the control memory and transfer
// data to be on the same 4G memory.
//
typedef struct _USBHC_MEM_POOL {
  BOOLEAN                 Check4G;
  UINT32                  Which4G;
  USBHC_MEM_BLOCK         *Head;
} USBHC_MEM_POOL;

//
// Memory allocation unit, must be 2^n, n>4
//
#define USBHC_MEM_UNIT           64

#define USBHC_MEM_UNIT_MASK      (USBHC_MEM_UNIT - 1)
#define USBHC_MEM_DEFAULT_PAGES  16

#define USBHC_MEM_ROUND(Len)  (((Len) + USBHC_MEM_UNIT_MASK) & (~USBHC_MEM_UNIT_MASK))

//
// Advance the byte and bit to the next bit, adjust byte accordingly.
//
#define NEXT_BIT(Byte, Bit)   \
          do {                \
            (Bit)++;          \
            if ((Bit) > 7) {  \
              (Byte)++;       \
              (Bit) = 0;      \
            }                 \
          } while (0)


/**
  Calculate the corresponding pci bus address according to the Mem parameter.

  @param  Pool           The memory pool of the host controller.
  @param  Mem            The pointer to host memory.
  @param  Size           The size of the memory region.

  @return the pci memory address
**/
EFI_PHYSICAL_ADDRESS
UsbHcGetPciAddressForHostMem (
  IN USBHC_MEM_POOL       *Pool,
  IN VOID                 *Mem,
  IN UINTN                Size
  );

#endif
