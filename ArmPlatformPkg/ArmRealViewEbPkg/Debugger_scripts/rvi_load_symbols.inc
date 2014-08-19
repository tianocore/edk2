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

include 'ZZZZZZ/rvi_symbols_macros.inc'

macro write_symbols_file("ZZZZZZ/rvi_symbols.tmp", 0x00000000, 0x10000000)

host "bash -o igncr ZZZZZZ/rvi_convert_symbols.sh ZZZZZZ/rvi_symbols.tmp ZZZZZZ/rvi_symbols.inc"
include 'ZZZZZZ/rvi_symbols.inc'
load /NI /NP 'ZZZZZZ/rvi_dummy.axf' ;.constdata
unload rvi_dummy.axf
delfile rvi_dummy.axf


