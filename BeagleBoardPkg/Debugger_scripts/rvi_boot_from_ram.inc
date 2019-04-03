//
// Copyright (c) 2008 - 2009, Apple Inc. All rights reserved.<BR>
//
//  SPDX-License-Identifier: BSD-2-Clause-Patent
//
error = continue
unload
error = abort

setreg @CP15_CONTROL = 0x0005107E
setreg @pc=0x80008208
setreg @cpsr=0x000000D3
dis/D
readfile,raw,nowarn "ZZZZZZ/FV/BEAGLEBOARD_EFI.fd"=0x80008000

