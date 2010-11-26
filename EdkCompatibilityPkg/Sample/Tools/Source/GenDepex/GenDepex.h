/*++
Copyright (c) 2004 - 2010, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

  Module Name:  
          GenDepex.h

  Abstract:
          This file contains the relevant declarations required
          to generate a binary Dependency File

  Complies with Tiano C Coding Standards Document, version 0.31, 12 Dec 2000.

--*/

#ifndef _EFI_GEN_DEPEX_H
#define _EFI_GEN_DEPEX_H

#include "TianoCommon.h"
#include "EfiDependency.h"
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <string.h>
#include <malloc.h>

#define DEPENDENCY_START            "DEPENDENCY_START"
#define OPERATOR_BEFORE             "BEFORE"
#define OPERATOR_AFTER              "AFTER"
#define OPERATOR_AND                "AND"
#define OPERATOR_OR                 "OR"
#define OPERATOR_NOT                "NOT"
#define OPERATOR_TRUE               "TRUE"
#define OPERATOR_FALSE              "FALSE"
#define OPERATOR_SOR                "SOR"
#define OPERATOR_END                "END"
#define OPERATOR_LEFT_PARENTHESIS   "("
#define OPERATOR_RIGHT_PARENTHESIS  ")"
#define DEPENDENCY_END              "DEPENDENCY_END"

#define DXE_DEP_LEFT_PARENTHESIS    0x0a
#define DXE_DEP_RIGHT_PARENTHESIS   0x0b

#define LINESIZE                    320
#define SIZE_A_SYMBOL               60
#define DEPENDENCY_OPCODE           UINT8
#define EVAL_STACK_SIZE             0x1024
#define BUFFER_SIZE                 0x100


#endif
