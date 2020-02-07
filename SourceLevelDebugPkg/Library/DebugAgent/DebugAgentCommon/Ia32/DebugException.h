/** @file
  Exception definitions.

  Copyright (c) 2010, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _DEBUG_EXCEPTION_H_
#define _DEBUG_EXCEPTION_H_

#define DEBUG_EXCEPT_DIVIDE_ERROR             0
#define DEBUG_EXCEPT_DEBUG                    1
#define DEBUG_EXCEPT_NMI                      2
#define DEBUG_EXCEPT_BREAKPOINT               3
#define DEBUG_EXCEPT_OVERFLOW                 4
#define DEBUG_EXCEPT_BOUND                    5
#define DEBUG_EXCEPT_INVALID_OPCODE           6
#define DEBUG_EXCEPT_DOUBLE_FAULT             8
#define DEBUG_EXCEPT_INVALID_TSS             10
#define DEBUG_EXCEPT_SEG_NOT_PRESENT         11
#define DEBUG_EXCEPT_STACK_FAULT             12
#define DEBUG_EXCEPT_GP_FAULT                13
#define DEBUG_EXCEPT_PAGE_FAULT              14
#define DEBUG_EXCEPT_FP_ERROR                16
#define DEBUG_EXCEPT_ALIGNMENT_CHECK         17
#define DEBUG_EXCEPT_MACHINE_CHECK           18
#define DEBUG_EXCEPT_SIMD                    19

#endif
