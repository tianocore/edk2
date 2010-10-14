/** @file
  ASCII String Literals with special meaning to Performance measurement and the Dp utility.

Copyright (c) 2009 - 2010, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __PERFORMANCE_TOKENS_H__
#define __PERFORMANCE_TOKENS_H__

#define SEC_TOK                         "SEC"             ///< SEC Phase
#define DXE_TOK                         "DXE"             ///< DEC Phase
#define SHELL_TOK                       "SHELL"           ///< Shell Phase
#define PEI_TOK                         "PEI"             ///< PEI Phase
#define BDS_TOK                         "BDS"             ///< BDS Phase
#define DRIVERBINDING_START_TOK         "DB:Start:"       ///< Driver Binding Start() function call
#define DRIVERBINDING_SUPPORT_TOK       "DB:Support:"     ///< Driver Binding Support() function call
#define LOAD_IMAGE_TOK                  "LoadImage:"      ///< Load a dispatched module
#define START_IMAGE_TOK                 "StartImage:"     ///< Dispatched Modules Entry Point execution

#endif  // __PERFORMANCE_TOKENS_H__
