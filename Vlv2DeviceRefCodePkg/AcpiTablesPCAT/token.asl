/**************************************************************************;
;*                                                                        *;
;*                                                                        *;
;*    Intel Corporation - ACPI Reference Code for the Sandy Bridge        *;
;*    Family of Customer Reference Boards.                                *;
;*                                                                        *;
;*                                                                        *;
;*    Copyright (c) 2012  - 2014, Intel Corporation. All rights reserved    *;
;
; This program and the accompanying materials are licensed and made available under
; the terms and conditions of the BSD License that accompanies this distribution.
; The full text of the license may be found at
; http://opensource.org/licenses/bsd-license.php.
;
; THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
; WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
;
;*                                                                        *;
;*                                                                        *;
;**************************************************************************/

Name(PMBS, 0x400)       // ASL alias for ACPI I/O base address.
Name(SMIP, 0xb2)        // I/O port to trigger SMI
Name(GPBS, 0x500)       // GPIO Register Block address
Name(APCB, 0xfec00000)  // Default I/O APIC(s) memory start address, 0x0FEC00000 - default, 0 - I/O APIC's disabled
Name(APCL, 0x1000)      // I/O APIC(s) memory decoded range, 0x1000 - default, 0 - I/O APIC's not decoded
Name(PFDR, 0xfed03034)  // PMC Function Disable Register
Name(PMCB, 0xfed03000)  // PMC Base Address
Name(PCLK, 0xfed03060)  // PMC Clock Control Register
Name(PUNB, 0xfed05000)  // PUNIT Base Address
Name(IBAS, 0xfed08000)  // ILB Base Address
Name(SRCB, 0xfed1c000)  // RCBA (Root Complex Base Address)
Name(SRCL, 0x1000)      // RCBA length
Name(HPTB, 0xfed00000)  // Same as HPET_BASE_ADDRESS for ASL use
Name(PEBS, 0xe0000000)  // PCIe Base
Name(PELN, 0x10000000)  //
Name(FMBL, 0x1) // Platform Flavor - Mobile flavor for ASL code.
Name(FDTP, 0x2) // Platform Flavor - Desktop flavor for ASL code.
Name(SDGV, 0x1c)        // UHCI Controller HOST_ALERT's bit offset within the GPE block. GPIO[0:15] corresponding to GPE[16:31]
Name(PEHP, 0x1) // _OSC: Pci Express Native Hot Plug Control
Name(SHPC, 0x0) // _OSC: Standard Hot Plug Controller (SHPC) Native Hot Plug control
Name(PEPM, 0x1) // _OSC: Pci Express Native Power Management Events control
Name(PEER, 0x1) // _OSC: Pci Express Advanced Error Reporting control
Name(PECS, 0x1) // _OSC: Pci Express Capability Structure control

