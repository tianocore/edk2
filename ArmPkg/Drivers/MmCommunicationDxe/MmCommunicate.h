/** @file

  Copyright (c) 2016-2018, ARM Limited. All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#if !defined _MM_COMMUNICATE_H_
#define _MM_COMMUNICATE_H_

#define MM_MAJOR_VER_MASK        0xEFFF0000
#define MM_MINOR_VER_MASK        0x0000FFFF
#define MM_MAJOR_VER_SHIFT       16

#define MM_MAJOR_VER(x) (((x) & MM_MAJOR_VER_MASK) >> MM_MAJOR_VER_SHIFT)
#define MM_MINOR_VER(x) ((x) & MM_MINOR_VER_MASK)

#define MM_CALLER_MAJOR_VER      0x1UL
#define MM_CALLER_MINOR_VER      0x0

#endif /* _MM_COMMUNICATE_H_ */
