/*++

Copyright (c) 2004, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  PalApi.h

Abstract:

  Main PAL API's defined in PAL specification. 


Revision History:

--*/

#ifndef _PALPROC_H
#define _PALPROC_H

#include "Tiano.h"

#define PAL_CACHE_FLUSH       0x0001
#define PAL_CACHE_INFO        0x0002
#define PAL_CACHE_INIT        0x0003
#define PAL_CACHE_SUMMARY     0x0004
#define PAL_MEM_ATTRIB        0x0005
#define PAL_PTCE_INFO         0x0006
#define PAL_VM_INFO           0x0007
#define PAL_VM_SUMMARY        0x0008
#define PAL_BUS_GET_FEATURES  0x0009
#define PAL_BUS_SET_FEATURES  0x000a
#define PAL_DEBUG_INFO        0x000b
#define PAL_FIXED_ADDR        0x000c
#define PAL_FREQ_BASE         0x000d
#define PAL_FREQ_RATIOS       0x000e
#define PAL_PERF_MON_INFO     0x000f
#define PAL_PLATFORM_ADDR     0x0010
#define PAL_PROC_GET_FEATURES 0x0011
#define PAL_PROC_SET_FEATURES 0x0012
#define PAL_RSE_INFO          0x0013
#define PAL_VERSION           0x0014

#define PAL_MC_CLEAR_LOG      0x0015
#define PAL_MC_DRAIN          0x0016
#define PAL_MC_EXPECTED       0x0017
#define PAL_MC_DYNAMIC_STATE  0x0018
#define PAL_MC_ERROR_INFO     0x0019
#define PAL_MC_RESUME         0x001a
#define PAL_MC_REGISTER_MEM   0x001b
#define PAL_HALT              0x001c
#define PAL_HALT_LIGHT        0x001d
#define PAL_COPY_INFO         0x001e
#define PAL_SHUTDOWN          0x002c
#define PAL_AUTH              0x0209
#define PAL_SINGL_DISPERSAL   0x0226  // dec. 550
#define PAL_HALT_INFO         0x0101
#define PAL_CACHE_LINE_INIT   0x001f
#define PAL_PMI_ENTRYPOINT    0x0020
#define PAL_ENTER_IA_32_ENV   0x0021
#define PAL_VM_PAGE_SIZE      0x0022
#define PAL_MEM_FOR_TEST      0x0025
#define PAL_CACHE_PROT_INFO   0x0026

#define PAL_COPY_PAL          0x0100
#define PAL_CACHE_READ        0x0103
#define PAL_CACHE_WRITE       0x0104
#define PAL_TEST_PROC         0x0102

#define PAL_DEBUG_FEATURE     0x0063  // vp1
typedef UINT64  EFI_PAL_STATUS;

//
//  Return values from PAL
//
typedef struct {
  EFI_PAL_STATUS  Status; // register r8
  UINT64          r9;
  UINT64          r10;
  UINT64          r11;
} PAL_RETURN_REGS;

//
// PAL equates for other parameters.
//
#define PAL_SUCCESS             0x0
#define PAL_CALL_ERROR          0xfffffffffffffffd
#define PAL_CALL_UNIMPLEMENTED  0xffffffffffffffff
#define PAL_CACHE_TYPE_I        0x1
#define PAL_CACHE_TYPE_D        0x2
#define PAL_CACHE_TYPE_I_AND_D  0x3
#define PAL_CACHE_NO_INT        0x0
#define PAL_CACHE_INT           0x2
//
// #define PAL_CACHE_PLAT_ACK                              0x4
//
#define PAL_CACHE_NO_PLAT_ACK               0x0
#define PAL_CACHE_INVALIDATE                0x1
#define PAL_CACHE_NO_INVALIDATE             0x0
#define PAL_CACHE_ALL_LEVELS                - 0x1

#define PAL_FEATURE_ENABLE                  0x1
#define PAL_ENABLE_BERR_BIT                 63
#define PAL_ENABLE_MCA_BINIT_BIT            61
#define PAL_ENABLE_CMCI_MCA_BIT             60
#define PAL_CACHE_DISABLE_BIT               59
#define PAL_DISABLE_COHERENCY_BIT           58

#define PAL_DIS_BUS_DATA_ERR_CHECK_BIT      63
#define PAL_DIS_BUS_ADDR_ERR_CHECK_BIT      61
#define PAL_DIS_BUS_INIT_EVENT_SIGNAL_BIT   60
#define PAL_DIS_BUS_REQ_ERR_SIGNAL_BIT      58
#define PAL_DIS_BUS_REQ_INT_ERR_SIGNAL_BIT  57
#define PAL_DIS_BUS_REQ_ERR_CHECK_BIT       56
#define PAL_DIS_BUS_RESP_ERR_CHECK_BIT      55

#define PAL_COPY_BSP_TOKEN                  0x0
#define PAL_COPY_AP_TOKEN                   0x1

#define PAL_CODE_TOKEN                      0x0
#define PAL_IA32EMU_CODE_TOKEN              0x1

#define PAL_INTERRUPT_BLOCK_TOKEN           0x0
#define PAL_IO_BLOCK_TOKEN                  0x1

#endif
