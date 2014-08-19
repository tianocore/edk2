//
// Copyright (c) 2008 - 2009, Apple Inc. All rights reserved.<BR>
//
//  This program and the accompanying materials
//  are licensed and made available under the terms and conditions of the BSD License
//  which accompanies this distribution.  The full text of the license may be found at
//  http://opensource.org/licenses/bsd-license.php
//
//  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
//  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
//

error = continue
unload
error = abort

setreg @CP15_CONTROL = 0x0005107E
setreg @cpsr=0x000000D3

; General clock settings.
setmem /32 0x48307270=0x00000080
setmem /32 0x48306D40=0x00000003
setmem /32 0x48005140=0x03020A50

;Clock configuration
setmem /32 0x48004A40=0x0000030A
setmem /32 0x48004C40=0x00000015

;DPLL3 (Core) settings
setmem /32 0x48004D00=0x00370037
setmem /32 0x48004D30=0x00000000
setmem /32 0x48004D40=0x094C0C00

;DPLL4 (Peripheral) settings
setmem /32 0x48004D00=0x00370037
setmem /32 0x48004D30=0x00000000
setmem /32 0x48004D44=0x0001B00C
setmem /32 0x48004D48=0x00000009

;DPLL1 (MPU) settings
setmem /32 0x48004904=0x00000037
setmem /32 0x48004934=0x00000000
setmem /32 0x48004940=0x0011F40C
setmem /32 0x48004944=0x00000001
setmem /32 0x48004948=0x00000000

;RAM setup.
setmem /16 0x6D000010=0x0000
setmem /16 0x6D000040=0x0001
setmem /16 0x6D000044=0x0100
setmem /16 0x6D000048=0x0000
setmem /32 0x6D000060=0x0000000A
setmem /32 0x6D000070=0x00000081
setmem /16 0x6D000040=0x0003
setmem /32 0x6D000080=0x02D04011
setmem /16 0x6D000084=0x0032
setmem /16 0x6D00008C=0x0000
setmem /32 0x6D00009C=0xBA9DC4C6
setmem /32 0x6D0000A0=0x00012522
setmem /32 0x6D0000A4=0x0004E201
setmem /16 0x6D000040=0x0003
setmem /32 0x6D0000B0=0x02D04011
setmem /16 0x6D0000B4=0x0032
setmem /16 0x6D0000BC=0x0000
setmem /32 0x6D0000C4=0xBA9DC4C6
setmem /32 0x6D0000C8=0x00012522
setmem /32 0x6D0000D4=0x0004E201
