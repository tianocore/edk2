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

#define DMA4_IRQENABLE_L(_i)  (0x48056018 + (0x4*(i)))

#define DMA4_CCR(_i)  (0x48056080 + (0x60*(i)))
#define DMA4_CICR(_i) (0x48056088 + (0x60*(i)))
#define DMA4_CSDP(_i) (0x48056090 + (0x60*(i)))
#define DMA4_CEN(_i)  (0x48056094 + (0x60*(i)))
#define DMA4_CFN(_i)  (0x48056098 + (0x60*(i)))
#define DMA4_CSSA(_i) (0x4805609c + (0x60*(i)))
#define DMA4_CDSA(_i) (0x480560a0 + (0x60*(i)))
#define DMA4_CSE(_i)  (0x480560a4 + (0x60*(i)))
#define DMA4_CSF(_i)  (0x480560a8 + (0x60*(i)))
#define DMA4_CDE(_i)  (0x480560ac + (0x60*(i)))



#endif 

