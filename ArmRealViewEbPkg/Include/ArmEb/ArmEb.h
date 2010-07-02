/*++

Copyright (c) 2010, Apple Inc. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.  

--*/

#ifndef __ARM_EB_H__
#define __ARM_EB_H__

#include <ArmEb/ArmEbUart.h>
#include <ArmEb/ArmEbTimer.h>

///
/// ARM EB Memory Map
///
// 0x00000000 - 0x0FFFFFFF SDRAM 256MB
// 0x10000000 - 0x100FFFFF System FPGA (config registers) 1MB
//   0x10000000–0x10000FFF  4KB    System registers 
//   0x10001000–0x10001FFF  4KB    System controller 
//   0x10002000–0x10002FFF  4KB    Two-Wire Serial Bus Interface 
//   0x10003000–0x10003FFF  4KB    Reserved 
//   0x10004000–0x10004FFF  4KB    Advanced Audio CODEC Interface 
//   0x10005000–0x10005FFF  4KB    MultiMedia Card Interface (MCI) 
//   0x10006000–0x10006FFF  4KB    Keyboard/Mouse Interface 0 
//   0x10007000–0x10007FFF  4KB    Keyboard/Mouse Interface 1 
//   0x10008000–0x10008FFF  4KB    Character LCD Interface 
//   0x10009000–0x10009FFF  4KB    UART 0 Interface 
//   0x1000A000–0x1000AFFF  4KB    UART 1 Interface 
//   0x1000B000–0x1000BFFF  4KB    UART 2 Interface 
//   0x1000C000–0x1000CFFF  4KB    UART 3 Interface 
//   0x1000D000–0x1000DFFF  4KB    Synchronous Serial Port Interface 
//   0x1000E000–0x1000EFFF  4KB    Smart Card Interface 
//   0x1000F000–0x1000FFFF  4KB    Reserved 
//   0x10010000–0x10010FFF  4KB    Watchdog Interface 
//   0x10011000–0x10011FFF  4KB    Timer modules 0 and 1 interface (Timer 1 starts at 0x10011020)
//   0x10012000–0x10012FFF  4KB    Timer modules 2 and 3 interface (Timer 3 starts at 0x10012020)
//   0x10013000–0x10013FFF  4KB    GPIO Interface 0 
//   0x10014000–0x10014FFF  4KB    GPIO Interface 1 
//   0x10015000–0x10015FFF  4KB    GPIO Interface 2 (miscellaneous onboard I/O) 
//   0x10016000–0x10016FFF  4KB    Reserved 
//   0x10017000–0x10017FFF  4KB    Real Time Clock Interface 
//   0x10018000–0x10018FFF  4KB    Dynamic Memory Controller configuration 
//   0x10019000–0x10019FFF  4KB    PCI controller configuration registers 
//   0x1001A000–0x1001FFFF  24KB   Reserved 
//   0x10020000–0x1002FFFF  64KB   Color LCD Controller 
//   0x10030000–0x1003FFFF  64KB   DMA Controller configuration registers 
//   0x10040000–0x1004FFFF  64KB   Generic Interrupt Controller 1 (nIRQ for tile 1) 
//   0x10050000–0x1005FFFF  64KB   Generic Interrupt Controller 2 (nFIQ for tile 1) 
//   0x10060000–0x1006FFFF  64KB   Generic Interrupt Controller 3 (nIRQ for tile 2) 
//   0x10070000–0x1007FFFF  64KB   Generic Interrupt Controller 4 (nFIQ for tile 2) 
//   0x10080000–0x1008FFFF  64KB   Static Memory Controller configuration registers 
//   0x100A0000–0x100EFFFF  448MB  Reserved 
//   0x10090000–0x100FFFFF  64KB   Debug Access Port (DAP) 
// 0x10100000 - 0x100FFFFF Reserved 3MB
// 0x10400000 - 0x17FFFFFF System FPGA 124MB
// 0x18000000 - 0x1FFFFFFF Logic Tile 1 128MB
// 0x20000000 - 0x3FFFFFFF Reserved 512MB
// 0x40000000 - 0x7FFFFFFF System FPGA 1GB
//   0x40000000–0x43FFFFFF   CS0 NOR flash (nNOR_CS1) 
//   0x44000000–0x47FFFFFF   CS1 NOR flash (nNOR_CS2) 
//   0x48000000–0x4BFFFFFF   CS2 SRAM (nSRAMCS) 
//   0x4C000000–0x4DFFFFFF   CS3 Config flash 
//   0x4E000000–0x4EFFFFFF   Ethernet 
//   0x4F000000–0x4FFFFFFF   USB 
//   0x50000000–0x53FFFFFF   CS4 (nEXPCS) PISMO (nCS0) 
//   0x54000000–0x57FFFFFF   CS5 (nSTATICCS4) PISMO (nCS1) 
//   0x58000000–0x5BFFFFFF   CS6 (nSTATICCS5) PISMO (nCS2) 
//   0x5C000000–0x5FFFFFFF   CS7 (nSTATICCS6) PISMO (nCS3) 
//   0x61000000–0x61FFFFFF   PCI SelfCfg window 
//   0x62000000–0x62FFFFFF   PCI Cfg window 
//   0x63000000–0x63FFFFFF   PCI I/O window
//   0x64000000–0x67FFFFFF   PCI memory window 0 
//   0x68000000–0x6BFFFFFF   PCI memory window 1 
//   0x6C000000–0x6FFFFFFF   PCI memory window 2 
//   0x70000000 - 0x7FFFFFFF   DRAM Mirror
// 0x80000000 - 0xFFFFFFFF Logic Tile site 2 2GB

//
// At reset EB_DRAM_BASE is alaised to EB_CS0_NOR_BASE
//
#define EB_DRAM_BASE          0x00000000   // 256 MB DRAM
#define EB_CONFIG_BASE        0x10000000

#define EB_CSO_NOR_BASE       0x40000000   // 64 MB NOR FLASH
#define EB_CS1_NOR_BASE       0x44000000   // 64 MB NOR FLASH
#define EB_CS2_SRAM           0x48000000   //  2 MB of SRAM
#define EB_CS3_CONFIG_FLASH   0x4c000000   //  8 MB Config FLASH for FPGA. Not to be used by application code
#define EB_CS3_ETHERNET       0x4e000000   // 16 MB Ethernet controller
#define EB_CS4_PISMO_CS0      0x50000000   // Expansion  CS0
#define EB_CS5_PISMO_CS0      0x54000000   // Expansion  CS0
#define EB_CS6_PISMO_CS0      0x58000000   // Expansion  CS0

#define EB_DRAM_REMAP_BASE    0x70000000   // if REMAPSTAT is HIGH alais of EB_DRAM_BASE

#endif 
