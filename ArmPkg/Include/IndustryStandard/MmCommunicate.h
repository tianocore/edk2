/** @file
  Header file for Management Mode Interface via SMC.
  This header file is used in normal world only.

  Copyright (c) 2016-2024, Arm Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Glossary:
    - MM - MM Management mode.

  @par Reference(s):
    -  ARM Management Mode Interface Specification
    [https://developer.arm.com/documentation/den0060/latest/]

**/

#ifndef MM_COMMUNICATE_H_
#define MM_COMMUNICATE_H_

#define MM_MAJOR_VER_MASK   0xEFFF0000
#define MM_MINOR_VER_MASK   0x0000FFFF
#define MM_MAJOR_VER_SHIFT  16

#define MM_MAJOR_VER(x)  (((x) & MM_MAJOR_VER_MASK) >> MM_MAJOR_VER_SHIFT)
#define MM_MINOR_VER(x)  ((x) & MM_MINOR_VER_MASK)

#define MM_CALLER_MAJOR_VER  0x1UL
#define MM_CALLER_MINOR_VER  0x0

#endif /* MM_COMMUNICATE_H_ */
