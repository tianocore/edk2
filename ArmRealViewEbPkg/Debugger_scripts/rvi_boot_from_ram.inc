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
setreg @pc=0x80008208
setreg @cpsr=0x000000D3
dis/D
readfile,raw,nowarn "ZZZZZZ/FV/BEAGLEBOARD_EFI.fd"=0x80008000

