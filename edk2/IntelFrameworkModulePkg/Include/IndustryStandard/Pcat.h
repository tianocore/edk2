/** @file
  Include file for PC-AT compatability.

Copyright (c) 2006, Intel Corporation. All rights reserved. 
This software and associated documentation (if any) is furnished
under a license and may only be used or copied in accordance
with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be
reproduced, stored in a retrieval system, or transmitted in any
form or by any means without the express written consent of
Intel Corporation.

**/

#ifndef _PC_AT_H_
#define _PC_AT_H_

//
// 8254 Timer
//
#define TIMER0_COUNT_PORT                         0x40
#define TIMER1_COUNT_PORT                         0x41
#define TIMER2_COUNT_PORT                         0x42
#define TIMER_CONTROL_PORT                        0x43

//
// 8259 PIC interrupt controller
//

#define PIC_CONTROL_REGISTER_MASTER               0x20
#define PIC_MASK_REGISTER_MASTER                  0x21
#define PIC_CONTROL_REGISTER_SLAVE                0xA0
#define PIC_MASK_REGISTER_SLAVE                   0xA1
#define PIC_EDGE_LEVEL_TRIGGERED_REGISTER_MASTER  0x4D0
#define PIC_EDGE_LEVEL_TRIGGERED_REGISTER_SLAVE   0x4D1

#define PIC_EOI                                   0x20

//
// 8237 DMA registers
//
#define R_8237_DMA_BASE_CA_CH0                    0x00
#define R_8237_DMA_BASE_CA_CH1                    0x02
#define R_8237_DMA_BASE_CA_CH2                    0x04
#define R_8237_DMA_BASE_CA_CH3                    0xd6
#define R_8237_DMA_BASE_CA_CH5                    0xc4
#define R_8237_DMA_BASE_CA_CH6                    0xc8
#define R_8237_DMA_BASE_CA_CH7                    0xcc

#define R_8237_DMA_BASE_CC_CH0                    0x01
#define R_8237_DMA_BASE_CC_CH1                    0x03
#define R_8237_DMA_BASE_CC_CH2                    0x05
#define R_8237_DMA_BASE_CC_CH3                    0xd7
#define R_8237_DMA_BASE_CC_CH5                    0xc6
#define R_8237_DMA_BASE_CC_CH6                    0xca
#define R_8237_DMA_BASE_CC_CH7                    0xce

#define R_8237_DMA_MEM_LP_CH0                     0x87
#define R_8237_DMA_MEM_LP_CH1                     0x83
#define R_8237_DMA_MEM_LP_CH2                     0x81
#define R_8237_DMA_MEM_LP_CH3                     0x82
#define R_8237_DMA_MEM_LP_CH5                     0x8B
#define R_8237_DMA_MEM_LP_CH6                     0x89
#define R_8237_DMA_MEM_LP_CH7                     0x8A


#define R_8237_DMA_COMMAND_CH0_3                  0x08
#define R_8237_DMA_COMMAND_CH4_7                  0xd0
#define   B_8237_DMA_COMMAND_GAP                  0x10
#define   B_8237_DMA_COMMAND_CGE                  0x04


#define R_8237_DMA_STA_CH0_3                      0xd8
#define R_8237_DMA_STA_CH4_7                      0xd0

#define R_8237_DMA_WRSMSK_CH0_3                   0x0a
#define R_8237_DMA_WRSMSK_CH4_7                   0xd4
#define   B_8237_DMA_WRSMSK_CMS                   0x04


#define R_8237_DMA_CHMODE_CH0_3                   0x0b
#define R_8237_DMA_CHMODE_CH4_7                   0xd6
#define   V_8237_DMA_CHMODE_DEMAND                0x00
#define   V_8237_DMA_CHMODE_SINGLE                0x40
#define   V_8237_DMA_CHMODE_CASCADE               0xc0
#define   B_8237_DMA_CHMODE_DECREMENT             0x20
#define   B_8237_DMA_CHMODE_INCREMENT             0x00
#define   B_8237_DMA_CHMODE_AE                    0x10
#define   V_8237_DMA_CHMODE_VERIFY                0
#define   V_8237_DMA_CHMODE_IO2MEM                0x04
#define   V_8237_DMA_CHMODE_MEM2IO                0x08

#define R_8237_DMA_CBPR_CH0_3                     0x0c
#define R_8237_DMA_CBPR_CH4_7                     0xd8

#define R_8237_DMA_MCR_CH0_3                      0x0d
#define R_8237_DMA_MCR_CH4_7                      0xda

#define R_8237_DMA_CLMSK_CH0_3                    0x0e
#define R_8237_DMA_CLMSK_CH4_7                    0xdc

#define R_8237_DMA_WRMSK_CH0_3                    0x0f
#define R_8237_DMA_WRMSK_CH4_7                    0xde

#endif
