//
// Copyright (c) 2008 - 2009, Apple Inc. All rights reserved.<BR>
//
//  SPDX-License-Identifier: BSD-2-Clause-Patent
//

include 'ZZZZZZ/rvi_symbols_macros.inc'

macro write_symbols_file("ZZZZZZ/rvi_symbols.tmp", 0x00000000, 0x10000000)

host "bash -o igncr ZZZZZZ/rvi_convert_symbols.sh ZZZZZZ/rvi_symbols.tmp ZZZZZZ/rvi_symbols.inc"
include 'ZZZZZZ/rvi_symbols.inc'
load /NI /NP 'ZZZZZZ/rvi_dummy.axf' ;.constdata
unload rvi_dummy.axf
delfile rvi_dummy.axf


