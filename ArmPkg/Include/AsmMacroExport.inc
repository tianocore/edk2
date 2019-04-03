;%HEADER%
;/** @file
;  Macros to centralize the EXPORT, AREA, and definition of an assembly
;  function.  The AREA prefix is required to put the function in its own
;  section so that removal of unused functions in the final link is performed.
;  This provides  equivalent functionality to the compiler's --split-sections
;  option.
;
;  Copyright (c) 2015 HP Development Company, L.P.
;
;  SPDX-License-Identifier: BSD-2-Clause-Patent
;
;**/


  MACRO
  RVCT_ASM_EXPORT $func
    EXPORT  $func
    AREA s_$func, CODE, READONLY
$func
  MEND

  END
