//
//  Copyright (c) 2011, ARM Limited. All rights reserved.
//
//  This program and the accompanying materials
//  are licensed and made available under the terms and conditions of the BSD License
//  which accompanies this distribution.  The full text of the license may be found at
//  http://opensource.org/licenses/bsd-license.php
//
//  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
//  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
//
//

#include <Library/PcdLib.h>

    EXPORT  MonitorVectorTable

    PRESERVE8
    AREA    MonitoVectorTableArea, CODE, READONLY, CODEALIGN, ALIGN=5

MonitorVectorTable

_MonitorResetEntry
  b   _MonitorResetEntry
_MonitorUndefinedEntry
  b   _MonitorUndefinedEntry
_MonitorSmcEntry
  b   _MonitorSmcEntry
_MonitorPrefetchEntry
  b   _MonitorPrefetchEntry
_MonitorDataAbortEntry
  b   _MonitorDataAbortEntry
_MonitorReservedEntry
  b   _MonitorReservedEntry
_MonitorIrqEntry
  b   _MonitorIrqEntry
_MonitorFiqEntry
  b   _MonitorFiqEntry

  END
