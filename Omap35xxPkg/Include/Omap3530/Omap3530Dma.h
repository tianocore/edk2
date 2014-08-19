/** @file

  Copyright (c) 2008 - 2010, Apple Inc. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __OMAP3530DMA_H__
#define __OMAP3530DMA_H__


#define DMA4_MAX_CHANNEL 31

#define DMA4_IRQENABLE_L(_i)  (0x48056018 + (0x4*(_i)))

#define DMA4_CCR(_i)  (0x48056080 + (0x60*(_i)))
#define DMA4_CICR(_i) (0x48056088 + (0x60*(_i)))
#define DMA4_CSR(_i)  (0x4805608c + (0x60*(_i)))
#define DMA4_CSDP(_i) (0x48056090 + (0x60*(_i)))
#define DMA4_CEN(_i)  (0x48056094 + (0x60*(_i)))
#define DMA4_CFN(_i)  (0x48056098 + (0x60*(_i)))
#define DMA4_CSSA(_i) (0x4805609c + (0x60*(_i)))
#define DMA4_CDSA(_i) (0x480560a0 + (0x60*(_i)))
#define DMA4_CSEI(_i) (0x480560a4 + (0x60*(_i)))
#define DMA4_CSFI(_i) (0x480560a8 + (0x60*(_i)))
#define DMA4_CDEI(_i) (0x480560ac + (0x60*(_i)))
#define DMA4_CDFI(_i) (0x480560b0 + (0x60*(_i)))

#define DMA4_GCR      (0x48056078)

// Channel Source Destination parameters
#define DMA4_CSDP_DATA_TYPE8    0
#define DMA4_CSDP_DATA_TYPE16   1
#define DMA4_CSDP_DATA_TYPE32   2

#define DMA4_CSDP_SRC_PACKED      BIT6
#define DMA4_CSDP_SRC_NONPACKED   0

#define DMA4_CSDP_SRC_BURST_EN    (0x0 << 7)
#define DMA4_CSDP_SRC_BURST_EN16  (0x1 << 7)
#define DMA4_CSDP_SRC_BURST_EN32  (0x2 << 7)
#define DMA4_CSDP_SRC_BURST_EN64  (0x3 << 7)

#define DMA4_CSDP_DST_PACKED      BIT13
#define DMA4_CSDP_DST_NONPACKED   0

#define DMA4_CSDP_BURST_EN        (0x0 << 14)
#define DMA4_CSDP_BURST_EN16      (0x1 << 14)
#define DMA4_CSDP_BURST_EN32      (0x2 << 14)
#define DMA4_CSDP_BURST_EN64      (0x3 << 14)

#define DMA4_CSDP_WRITE_MODE_NONE_POSTED      (0x0 << 16)
#define DMA4_CSDP_WRITE_MODE_POSTED           (0x1 << 16)
#define DMA4_CSDP_WRITE_MODE_LAST_NON_POSTED  (0x2 << 16)

#define DMA4_CSDP_DST_ENDIAN_LOCK_LOCK    BIT18
#define DMA4_CSDP_DST_ENDIAN_LOCK_ADAPT   0

#define DMA4_CSDP_DST_ENDIAN_BIG          BIT19
#define DMA4_CSDP_DST_ENDIAN_LITTLE       0

#define DMA4_CSDP_SRC_ENDIAN_LOCK_LOCK    BIT20
#define DMA4_CSDP_SRC_ENDIAN_LOCK_ADAPT   0

#define DMA4_CSDP_SRC_ENDIAN_BIG          BIT21
#define DMA4_CSDP_SRC_ENDIAN_LITTLE       0

// Channel Control
#define DMA4_CCR_SYNCHRO_CONTROL_MASK     0x1f

#define DMA4_CCR_FS_ELEMENT     (0    | 0)
#define DMA4_CCR_FS_BLOCK       (0    | BIT18)
#define DMA4_CCR_FS_FRAME       (BIT5 | 0)
#define DMA4_CCR_FS_PACKET      (BIT5 | BIT18)

#define DMA4_CCR_READ_PRIORITY_HIGH   BIT6
#define DMA4_CCR_READ_PRIORITY_LOW    0

#define DMA4_CCR_ENABLE               BIT7
#define DMA4_CCR_DISABLE              0

#define DMA4_CCR_SUSPEND_SENSITIVE_IGNORE BIT8
#define DMA4_CCR_SUSPEND_SENSITIVE        0

#define DMA4_CCR_RD_ACTIVE                BIT9
#define DMA4_CCR_WR_ACTIVE                BIT10

#define DMA4_CCR_SRC_AMODE                (0     | 0)
#define DMA4_CCR_SRC_AMODE_POST_INC       (0     | BIT12)
#define DMA4_CCR_SRC_AMODE_SINGLE_INDEX   (BIT13 | 0)
#define DMA4_CCR_SRC_AMODE_DOUBLE_INDEX   (BIT13 | BIT12)

#define DMA4_CCR_DST_AMODE                (0     | 0)
#define DMA4_CCR_DST_AMODE_POST_INC       (0     | BIT14)
#define DMA4_CCR_DST_AMODE_SINGLE_INDEX   (BIT15 | 0)
#define DMA4_CCR_DST_AMODE_DOUBLE_INDEX   (BIT15 | BIT14)

#define DMA4_CCR_CONST_FILL_ENABLE        BIT16
#define DMA4_CCR_TRANSPARENT_COPY_ENABLE  BIT17

#define DMA4_CCR_SEL_SRC_DEST_SYNC_SOURCE BIT24

#define DMA4_CSR_DROP                     BIT1
#define DMA4_CSR_HALF                     BIT2
#define DMA4_CSR_FRAME                    BIT3
#define DMA4_CSR_LAST                     BIT4
#define DMA4_CSR_BLOCK                    BIT5
#define DMA4_CSR_SYNC                     BIT6
#define DMA4_CSR_PKT                      BIT7
#define DMA4_CSR_TRANS_ERR                BIT8
#define DMA4_CSR_SECURE_ERR               BIT9
#define DMA4_CSR_SUPERVISOR_ERR           BIT10
#define DMA4_CSR_MISALIGNED_ADRS_ERR      BIT11
#define DMA4_CSR_DRAIN_END                BIT12
#define DMA4_CSR_RESET                    0x1FE
#define DMA4_CSR_ERR                      (DMA4_CSR_TRANS_ERR | DMA4_CSR_SECURE_ERR | DMA4_CSR_SUPERVISOR_ERR | DMA4_CSR_MISALIGNED_ADRS_ERR)

// same mapping as CSR except for SYNC. Enable all since we are polling
#define DMA4_CICR_ENABLE_ALL              0x1FBE


#endif

